/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "stdafx.h"

#include "AnimationCompiler.h"
#include "AnimationInfoLoader.h"
#include "../CryEngine/Cry3DEngine/CGF/CGFLoader.h"
#include "../CryEngine/Cry3DEngine/CGF/ReadOnlyChunkFile.h"
#include "CGF/CGFSaver.h"
#include "CGF/ChunkData.h"
#include "FileUtil.h"
#include "StringHelpers.h"
#include "SkeletonInfo.h"
#include "AnimationLoader.h"
#include "AnimationManager.h"
#include "IConfig.h"
#include "TrackStorage.h"
#include "UpToDateFileHelpers.h"

#include "IPakSystem.h"
#include "TempFilePakExtraction.h"
#include "../../../CryXML/ICryXML.h"
#include "MathHelpers.h"
#include "StringHelpers.h"
#include "StealingThreadPool.h"
#include "PakXmlFileBufferSource.h"
#include "DBAEnumerator.h"
#include "DBATableEnumerator.h"
#include "Plugins/EditorAnimation/Shared/CompressionPresetTable.h"
#include "MetricsScope.h"
#include <RcFile.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/base.h>
#include <AzCore/std/time.h>
#include <AzCore/std/functional.h>

#include <SceneAPI/SceneData/Groups/AnimationGroup.h>

// Returns -1 in case of fail. Would be nice to add this to CSkinningInfo.
static int32 FindAIMBlendIndexByAnimation(const CSkinningInfo& info, const string& animationPath)
{
    const size_t numBlends = info.m_AimDirBlends.size();
    for (size_t d = 0; d < numBlends; ++d)
    {
        const string& animToken = info.m_AimDirBlends[d].m_AnimToken;
        if (StringHelpers::ContainsIgnoreCase(animationPath, animToken))
        {
            return d;
        }
    }
    return -1;
}

static int32 FindLookBlendIndexByAnimation(const CSkinningInfo& info, const string& animationPath)
{
    const size_t numBlends = info.m_LookDirBlends.size();
    for (size_t d = 0; d < numBlends; ++d)
    {
        const string& animToken = info.m_LookDirBlends[d].m_AnimToken;
        if (StringHelpers::ContainsIgnoreCase(animationPath, animToken))
        {
            return d;
        }
    }
    return -1;
}

static bool IsLookAnimation(const CSkinningInfo& skeletonInfo, const string& animationPath)
{
    return FindLookBlendIndexByAnimation(skeletonInfo, animationPath) != -1;
}

static bool IsAimAnimation(const CSkinningInfo& skeletonInfo, const string& animationPath)
{
    if (FindAIMBlendIndexByAnimation(skeletonInfo, animationPath) != -1)
    {
        return true;
    }
    return StringHelpers::ContainsIgnoreCase(PathHelpers::GetFilename(animationPath), "AimPoses");
}

static bool ScanDirectoryRecursive(
    const string& root, const string& path, const string& file, std::vector<string>& files, bool recursive,
    const std::vector<string>& dirsToIgnore, std::vector<uint32>& sizes);


string UnifiedPath(const string& path)
{
    return PathHelpers::ToUnixPath(path);
}


string RelativePath(const string& path, const string& relativeToPath)
{
    const string relPath = PathHelpers::GetShortestRelativeAsciiPath(relativeToPath, path);
    if (!relPath.empty())
    {
        // we don't allow path to point outside of base path (or to base path itself)
        if ((relPath[0] == '.' && (relPath.length() == 1 || relPath[1] == '.')) ||
            !PathHelpers::IsRelative(relPath))
        {
            return string();
        }
    }
    return relPath;
}


static bool CheckIfRefreshNeeded(const char* sourcePath, const char* destinationPath, FILETIME* sourceFileTimestamp = 0)
{
    const FILETIME sourceStamp = FileUtil::GetLastWriteFileTime(sourcePath);
    if (sourceFileTimestamp)
    {
        *sourceFileTimestamp = sourceStamp;
    }

    if (!FileUtil::FileTimeIsValid(sourceStamp))
    {
        return true;
    }

    const FILETIME targetStamp = FileUtil::GetLastWriteFileTime(destinationPath);
    if (!FileUtil::FileTimeIsValid(targetStamp))
    {
        return true;
    }

    return !FileUtil::FileTimesAreEqual(targetStamp, sourceStamp);
}

static bool HasAnimationSettingsFileChanged(const string& animationSourceFilename, const string& animationTargetFilename)
{
    const string sourceAnimationSettingsFile = SAnimationDefinition::GetAnimationSettingsFilename(animationSourceFilename, SAnimationDefinition::ePASF_OVERRIDE_FILE);
    const string targetAnimationSettingsFile = SAnimationDefinition::GetAnimationSettingsFilename(animationTargetFilename, SAnimationDefinition::ePASF_UPTODATECHECK_FILE);
    const bool refreshNeeded = CheckIfRefreshNeeded(sourceAnimationSettingsFile, targetAnimationSettingsFile);
    return refreshNeeded;
}

//////////////////////////////////////////////////////////////////////////
class AutoDeleteFile
    : IResourceCompiler::IExitObserver
{
public:
    AutoDeleteFile(IResourceCompiler* pRc, const string& filename)
        : m_pRc(pRc)
        , m_filename(filename)
    {
        if (m_pRc)
        {
            m_pRc->AddExitObserver(this);
        }
    }

    virtual ~AutoDeleteFile()
    {
        Delete();
        if (m_pRc)
        {
            m_pRc->RemoveExitObserver(this);
        }
    }

    virtual void OnExit()
    {
        Delete();
    }

private:
    void Delete()
    {
        if (!m_filename.empty())
        {
#if defined(AZ_PLATFORM_WINDOWS)
            SetFileAttributesA(m_filename.c_str(), FILE_ATTRIBUTE_ARCHIVE);
#endif
            AZ::IO::LocalFileIO().Remove(m_filename.c_str());
        }
    }

private:
    IResourceCompiler* m_pRc;
    string m_filename;
};


//////////////////////////////////////////////////////////////////////////
CAnimationConvertor::CAnimationConvertor(ICryXML* pXMLParser, IPakSystem* pPakSystem, IResourceCompiler* pRC)
    : m_pPakSystem(pPakSystem)
    , m_pXMLParser(pXMLParser)
    , m_config(0)
    , m_tempPath(pRC->GetTmpPath())
    , m_fancyAnimationIndex(0)
    , m_changedAnimationCount(0)
    , m_rc(pRC)
    , m_platform(-1)
    , m_usingIntermediateCAF(false)
    , m_compressionPresetTable(new SCompressionPresetTable())
{
}

void CAnimationConvertor::Init(const ConvertorInitContext& context)
{
    m_fancyAnimationIndex = 0;
    m_changedAnimationCount = 0;
    m_platform = -1;
    m_cbaEntries.clear();
    m_cbaLoader.reset();

    m_config = context.config;

    m_sourceGameFolderPath = context.config->GetAsString("sourceroot", "", "");
    if (m_sourceGameFolderPath.empty())
    {
        m_sourceGameFolderPath = context.config->GetAsString("gameroot", "", "");
    }

    m_sourceWatchFolderPath = context.config->GetAsString("watchfolder", "", "");
    if (m_sourceWatchFolderPath.empty())
    {
        m_sourceWatchFolderPath = m_sourceGameFolderPath;
    }

    m_configSubfolder = context.config->GetAsString("animConfigFolder", "Animations", "Animations");

    const bool bIgnorePresets = context.config->GetAsBool("ignorepresets", false, true);
    if (!bIgnorePresets)
    {
        const string compressionSettingsPath = UnifiedPath(PathHelpers::Join(PathHelpers::Join(m_sourceGameFolderPath, m_configSubfolder), "CompressionPresets.json"));
        if (FileUtil::FileExists(compressionSettingsPath))
        {
            if (!m_compressionPresetTable->Load(compressionSettingsPath.c_str()))
            {
                RCLogError("Failed to load compression presets table: %s", compressionSettingsPath.c_str());
            }
        }
    }

    RCLog("Looking for used skeletons...");
    string cbaPath;
    std::set<string> usedSkeletons;
    {
        for (size_t i = 0; i < context.inputFileCount; ++i)
        {
            const char* const filename = context.inputFiles[i].m_sourceInnerPathAndName.c_str();
            string extension = PathHelpers::FindExtension(filename);
            bool isIntermediateCAF = StringHelpers::EqualsIgnoreCase(extension, "i_caf");
            if (isIntermediateCAF)
            {
                if (isIntermediateCAF)
                {
                    m_usingIntermediateCAF = true;

                    const string animSettingsPath = context.config->GetAsString("animSettingsFile", "", "");
                    if (!animSettingsPath.empty())
                    {
                        SAnimationDefinition::SetOverrideAnimationSettingsFilename(animSettingsPath);
                    }
                }

                const string fullPath = PathHelpers::Join(context.inputFiles[i].m_sourceLeftPath, context.inputFiles[i].m_sourceInnerPathAndName);
                if (m_sourceGameFolderPath.empty())
                {
                    m_sourceGameFolderPath = PathHelpers::GetDirectory(PathHelpers::GetAbsoluteAsciiPath(fullPath));
                }

                SAnimationDesc desc;
                bool bErrorReported = false;
                if (SAnimationDefinition::GetDescFromAnimationSettingsFile(&desc, &bErrorReported, 0, m_pPakSystem, m_pXMLParser, fullPath.c_str(), std::vector<string>()))
                {
                    if (!desc.m_skeletonName.empty())
                    {
                        usedSkeletons.insert(desc.m_skeletonName.c_str());
                    }
                }
            }
            else if (StringHelpers::EqualsIgnoreCase(extension, "cba"))
            {
                const string fullPath = PathHelpers::Join(context.inputFiles[i].m_sourceLeftPath, context.inputFiles[i].m_sourceInnerPathAndName);
                cbaPath = PathHelpers::GetAbsoluteAsciiPath(fullPath);
            }
        }
    }

    if (!cbaPath.empty())
    {
        if (!m_usingIntermediateCAF)
        {
            const bool singleFileMode = context.config->HasKey("file");
            const bool loadAllCharacters = !singleFileMode;

            if (!LoadCBA(cbaPath.c_str(), loadAllCharacters))
            {
                RCLogError("Unable to load CBA (animation description file): %s", cbaPath);
            }
            // Use parent of Animations as a game folder, as cbapath is expected to be Animations/Animations.cba
            m_sourceGameFolderPath = PathHelpers::GetDirectory(PathHelpers::GetAbsoluteAsciiPath(cbaPath));
        }
        else
        {
            RCLogError("CBA file is specified together with i_caf. i_caf conversion expects /animConfigFolder option.");
        }
    }

    if (m_sourceGameFolderPath.empty())
    {
        RCLogError("Failed to locate game folder.");
    }

    const int verbosityLevel = m_rc->GetVerbosityLevel();
    if (verbosityLevel > 0)
    {
        RCLog("Source game folder path: %s", m_sourceGameFolderPath.c_str());
    }

    if (m_usingIntermediateCAF)
    {
        InitSkeletonManager(usedSkeletons);

        if (!InLocalUpdateMode())
        {
            InitDBAManager();
        }
    }
}

void CAnimationConvertor::InitSkeletonManager(const std::set<string>& usedSkeletons)
{
    m_skeletonManager.reset(new SkeletonManager(m_pPakSystem, m_pXMLParser, m_rc));

    string skeletonListPath = UnifiedPath(PathHelpers::Join(PathHelpers::Join(m_sourceGameFolderPath, m_configSubfolder), "SkeletonList.xml"));
    m_skeletonManager->LoadSkeletonList(skeletonListPath, m_sourceGameFolderPath, usedSkeletons);
}


void CAnimationConvertor::InitDBAManager()
{
    string newDbaTable = PathHelpers::Join(PathHelpers::Join(m_sourceGameFolderPath, m_configSubfolder), "DBATable.json");
    if (FileUtil::FileExists(newDbaTable.c_str()))
    {
        DBATableEnumerator* dbaTable = new DBATableEnumerator();
        string configFolderPath = PathHelpers::Join(m_sourceGameFolderPath, m_configSubfolder);
        if (!dbaTable->LoadDBATable(configFolderPath, m_sourceGameFolderPath, m_pPakSystem, m_pXMLParser))
        {
            RCLogError("Failed to load DBATable.json: %s", newDbaTable.c_str());
        }
        m_dbaTableEnumerator.reset(dbaTable);
    }
    else
    {
        m_dbaManager.reset(new DBAManager(m_pPakSystem, m_pXMLParser));
        string dbaTablePath = PathHelpers::Join(PathHelpers::Join(m_sourceGameFolderPath, m_configSubfolder), "DBATable.xml");
        m_dbaManager->LoadDBATable(dbaTablePath);
    }
}


void CAnimationConvertor::DeInit()
{
    RebuildDatabases();
}

void FindCAFsForDefinition(std::vector<CAFFileItem>* items, const string& sourceFolder, const SAnimationDefinition& definition, const CAnimationInfoLoader& loader)
{
    const string& animationPath(definition.GetAnimationPath());

    std::vector<string> ignoreFolders;
    size_t numDefinitions = loader.m_ADefinitions.size();
    for (size_t i = 0; i < numDefinitions; ++i)
    {
        const string& path = loader.m_ADefinitions[i].GetAnimationPath();
        if (path.size() == animationPath.size())
        {
            continue;
        }

        if (StringHelpers::StartsWithIgnoreCase(path, animationPath)) // subpath
        {
            ignoreFolders.push_back(PathHelpers::ToDosPath(string(path.begin() + animationPath.size(), path.end())));
        }
    }
    string sourceDir = PathHelpers::AddSeparator(PathHelpers::Join(sourceFolder, definition.GetAnimationPathWithoutSlash()));

    std::vector<string> fileList;
    std::vector<uint32> sizes;

    ScanDirectoryRecursive(sourceDir.c_str(), string(""), string("*.caf"), fileList, true, ignoreFolders, sizes);

    items->resize(fileList.size());
    for (size_t i = 0; i < fileList.size(); ++i)
    {
        CAFFileItem& item = (*items)[i];
        item.m_name = fileList[i];
        item.m_size = sizes[i];
    }

    std::sort(items->begin(), items->end());
}

static vector<string> GetJointNames(const CSkeletonInfo* skeleton)
{
    vector<string> result;
    for (int i = 0; i < skeleton->m_SkinningInfo.m_arrBonesDesc.size(); ++i)
    {
        result.push_back(skeleton->m_SkinningInfo.m_arrBonesDesc[i].m_arrBoneName);
    }

    return result;
}

bool CAnimationConvertor::RebuildDatabases()
{
    if (m_config->GetAsBool("SkipDba", false, true))
    {
        RCLog("/SkipDba specified. Skipping database update...");
        return true;
    }

    if (m_changedAnimationCount == 0)
    {
        RCLog("No animations changed. Skipping database update...");
        return true;
    }

    const bool bStreamPrepare = m_config->GetAsBool("dbaStreamPrepare", false, true);

    string targetGameFolderPath;
    {
        const string targetRoot = PathHelpers::RemoveSeparator(m_config->GetAsString("targetroot", "", ""));

        string destinationDir;
        if (!targetRoot.empty())
        {
            if (m_usingIntermediateCAF)
            {
                targetGameFolderPath = PathHelpers::ToDosPath(PathHelpers::AddSeparator(targetRoot));
            }
            else
            {
                targetGameFolderPath = PathHelpers::ToDosPath(PathHelpers::AddSeparator(PathHelpers::GetDirectory(targetRoot)));
            }
        }
        else
        {
            targetGameFolderPath = PathHelpers::ToDosPath(PathHelpers::AddSeparator(PathHelpers::GetDirectory(m_cbaPath)));
            ;
        }
    }

    string devRoot = gEnv->pFileIO->GetAlias("@devroot@");
    string sourceFolder = PathHelpers::Join(devRoot, m_sourceGameFolderPath);
    targetGameFolderPath = PathHelpers::Join(devRoot, targetGameFolderPath);

    CAnimationManager animationManager;

    size_t totalInputSize = 0;
    size_t totalOutputSize = 0;
    size_t totalAnimCount = 0;

    std::set<string> unusedDBAs;
    {
        std::vector<string> allDBAs;

        FileUtil::ScanDirectory(targetGameFolderPath, "*.dba", allDBAs, true, string());
        for (size_t i = 0; i < allDBAs.size(); ++i)
        {
            unusedDBAs.insert(UnifiedPath(allDBAs[i]));
        }
    }

    RCLog("Rebuilding DBAs...");

    std::unique_ptr<IDBAEnumerator> enumeratorAuto;
    IDBAEnumerator* enumerator = 0;
    if (m_usingIntermediateCAF)
    {
        if (m_dbaTableEnumerator.get())
        {
            enumerator = m_dbaTableEnumerator.get();
        }
        else
        {
            enumeratorAuto.reset(new DBAManagerEnumerator(m_dbaManager.get(), m_sourceGameFolderPath));
            enumerator = enumeratorAuto.get();
        }
    }
    else
    {
        if (!m_cbaLoader.get())
        {
            RCLogError("CBA was not loaded.");
            return false;
        }
        enumeratorAuto.reset(new CBAEnumerator(m_cbaLoader.get(), m_cbaEntries, m_sourceGameFolderPath));
        enumerator = enumeratorAuto.get();
    }

    const PlatformInfo* pPlatformInfo = m_rc->GetPlatformInfo(m_platform);
    const bool bigEndianOutput = pPlatformInfo->bBigEndian;
    const int pointerSize = pPlatformInfo->pointerSize;

    string dbaInputFile;
    if (m_usingIntermediateCAF)
    {
        dbaInputFile = PathHelpers::Join(PathHelpers::Join(m_sourceGameFolderPath, m_configSubfolder), "DBATable.json");
        if (!FileUtil::FileExists(dbaInputFile.c_str()))
        {
            dbaInputFile = PathHelpers::Join(PathHelpers::Join(m_sourceGameFolderPath, m_configSubfolder), "DBATable.xml");
        }
    }
    else
    {
        dbaInputFile = m_cbaPath;
    }

    const size_t dbaCount = enumerator->GetDBACount();
    for (size_t dbaIndex = 0; dbaIndex < dbaCount; ++dbaIndex)
    {
        EnumeratedDBA dba;
        enumerator->GetDBA(&dba, dbaIndex);

        const string& dbaInnerPath = dba.innerPath;


        std::unique_ptr<CTrackStorage> storage;
        if (!dbaInnerPath.empty())
        {
            storage.reset(new CTrackStorage(bigEndianOutput));
        }

        std::vector<AnimationJob> animations;
        animations.resize(dba.animationCount);

        size_t dbaInputSize = 0;
        size_t dbaAnimCount = 0;

        for (size_t anim = 0; anim < dba.animationCount; ++anim)
        {
            EnumeratedCAF caf;
            enumerator->GetCAF(&caf, dbaIndex, anim);

            string compressedCAFPath = PathHelpers::Join(targetGameFolderPath, caf.path);
            compressedCAFPath = PathHelpers::ReplaceExtension(compressedCAFPath, "$caf"); // .caf -> .$caf

            if (!FileUtil::FileExists(compressedCAFPath.c_str()))
            {
                RCLogWarning("Compressed CAF '%s' not found in target folder '%s'. Please recompile '%s'.",
                    compressedCAFPath.c_str(), targetGameFolderPath.c_str(), caf.path.c_str());
                continue;
            }

            CChunkFile chunkFile;
            GlobalAnimationHeaderCAF globalAnimationHeader;
            if (!globalAnimationHeader.LoadCAF(compressedCAFPath.c_str(), eCAFLoadCompressedOnly, &chunkFile))
            {
                RCLogError("Failed to load a compressed animation '%s'.", compressedCAFPath.c_str());
                continue;
            }


            const string animationPath = UnifiedPath(caf.path);
            if (animationPath.empty())
            {
                RCLogError("Unexpected error with animation %s. Root animation folder is: %s", caf.path.c_str(), m_sourceGameFolderPath.c_str());
                continue;
            }

            const bool isAIM = chunkFile.FindChunkByType(ChunkType_GlobalAnimationHeaderAIM) != 0;
            if (isAIM)
            {
                GlobalAnimationHeaderAIM gaim;
                if (!gaim.LoadAIM(compressedCAFPath.c_str(), eCAFLoadCompressedOnly))
                {
                    RCLogError("Unable to load AIM/LOOK: '%s'.", compressedCAFPath.c_str());
                }
                else if (animationPath != gaim.m_FilePath)
                {
                    RCLogError("AIM loaded with wrong m_FilePath: '%s' (should be '%s').",
                        gaim.m_FilePath.c_str(), animationPath.c_str());
                }
                else if (!gaim.IsValid())
                {
                    RCLogError("AIM header contains invalid values (NANs): '%s'.", compressedCAFPath.c_str());
                }
                else
                {
                    if (!animationManager.AddAIMHeaderOnly(gaim))
                    {
                        RCLogWarning("CAF(AIM) file was added to more than one DBA: '%s'", animationPath.c_str());
                    }
                }
                globalAnimationHeader.OnAimpose(); // we need this info to skip aim-poses in the IMG file
            }

            globalAnimationHeader.SetFilePathCAF(animationPath.c_str());
            const bool saveInDBA = !isAIM && !dbaInnerPath.empty() && !caf.skipDBA;

            if (saveInDBA)
            {
                const string dbaPath = UnifiedPath(dbaInnerPath);
                globalAnimationHeader.SetFilePathDBA(dbaPath);

                globalAnimationHeader.OnAssetLoaded();
                globalAnimationHeader.ClearAssetOnDemand();
            }
            else
            {
                globalAnimationHeader.OnAssetOnDemand();
                globalAnimationHeader.ClearAssetLoaded();
            }

            if (!animationManager.AddCAFHeaderOnly(globalAnimationHeader))
            {
                RCLogWarning("CAF file was added to more than one DBA: '%s'",
                    globalAnimationHeader.m_FilePath.c_str());
            }

            if (storage.get() && saveInDBA)
            {
                storage->AddAnimation(globalAnimationHeader);

                dbaInputSize += (size_t)FileUtil::GetFileSize(compressedCAFPath.c_str());
                ++dbaAnimCount;
            }
        }

        {
            string unifiedPath = PathHelpers::RemoveSeparator(dbaInnerPath);
            UnifyPath(unifiedPath);
            std::set<string>::iterator it = unusedDBAs.find(unifiedPath);
            if (it != unusedDBAs.end())
            {
                unusedDBAs.erase(it);
            }
        }

        if (storage.get())
        {
            if (dbaAnimCount != 0)
            {
                const string dbaOutPath = PathHelpers::Join(targetGameFolderPath, dbaInnerPath);
                storage->SaveDataBase905(dbaOutPath.c_str(), bStreamPrepare, pointerSize);

                size_t dbaOutputSize = 0;
                if (FileUtil::FileExists(dbaOutPath.c_str()))
                {
                    dbaOutputSize = (size_t)FileUtil::GetFileSize(dbaOutPath.c_str());
                }

                RCLog("DBA %i KB -> %i KB (%02.1f%%) anims: %i '%s'",
                    dbaInputSize / 1024, dbaOutputSize / 1024,
                    double(dbaOutputSize) / dbaInputSize * 100.0, dbaAnimCount, dbaInnerPath.c_str());

                if (m_rc)
                {
                    m_rc->AddInputOutputFilePair(dbaInputFile, dbaOutPath.c_str());
                }

                totalInputSize += dbaInputSize;
                totalOutputSize += dbaOutputSize;
                totalAnimCount += dbaAnimCount;
            }
            else
            {
                RCLogWarning("No valid animations found which belong to the DBA '%s', so it's not created.", dbaInnerPath.c_str());
            }
        }
    }

    if (m_usingIntermediateCAF)
    {
        RCLog("Looking for non dba animations for creating animation .img files.");
        {
            std::vector<string> cafFileList;
            FileUtil::ScanDirectory(sourceFolder, string(m_usingIntermediateCAF ? "*.i_caf" : "*.caf"), cafFileList, true, "");

            for (size_t i = 0; i < cafFileList.size(); ++i)
            {
                const string& filePath = cafFileList[i];
                string animationPath = filePath;
                if (m_usingIntermediateCAF)
                {
                    animationPath = PathHelpers::ReplaceExtension(animationPath, "caf");
                }
                UnifyPath(animationPath);

                string compressedCAFPath = PathHelpers::Join(targetGameFolderPath, filePath);
                compressedCAFPath = PathHelpers::ReplaceExtension(compressedCAFPath, "$caf"); // .caf -> .$caf

                if (!FileUtil::FileExists(compressedCAFPath.c_str()))
                {
                    RCLogWarning("Compressed CAF '%s' not found in target folder '%s'. Please recompile it.",
                        compressedCAFPath.c_str(), targetGameFolderPath.c_str());
                    continue;
                }

                CChunkFile chunkFile;
                GlobalAnimationHeaderCAF globalAnimationHeader;
                if (!globalAnimationHeader.LoadCAF(compressedCAFPath.c_str(), eCAFLoadCompressedOnly, &chunkFile))
                {
                    RCLogError("Failed to load a compressed animation '%s'.", compressedCAFPath.c_str());
                    continue;
                }

                const string fullAnimationPath = PathHelpers::Join(sourceFolder, filePath);
                SAnimationDesc animDesc;
                bool bErrorReported = false;
                bool bUsesNameContains = false;
                if (!SAnimationDefinition::GetDescFromAnimationSettingsFile(&animDesc, &bErrorReported, &bUsesNameContains, m_pPakSystem, m_pXMLParser, fullAnimationPath, vector<string>()))
                {
                    if (!bErrorReported)
                    {
                        RCLogError("Failed to load animation settings for animation: %s", fullAnimationPath.c_str());
                    }
                    continue;
                }
                const CSkeletonInfo* pSkeleton = m_skeletonManager->LoadSkeleton(animDesc.m_skeletonName);
                if (!pSkeleton)
                {
                    RCLogError("Failed to load skeleton with name '%s' for animation with name '%s'. If this animation is an aimpose or lookpose, it will not be correctly added to the .img files.",
                        animDesc.m_skeletonName.c_str(), filePath.c_str());
                    continue;
                }
                if (bUsesNameContains && !SAnimationDefinition::GetDescFromAnimationSettingsFile(&animDesc, &bErrorReported, 0, m_pPakSystem, m_pXMLParser, fullAnimationPath, GetJointNames(pSkeleton)))
                {
                    if (!bErrorReported)
                    {
                        RCLogError("Failed to load animation settings for animation: %s", fullAnimationPath.c_str());
                    }
                    continue;
                }

                const bool isAIM = IsAimAnimation(pSkeleton->m_SkinningInfo, animationPath);
                const bool isLook = IsLookAnimation(pSkeleton->m_SkinningInfo, animationPath);
                if (isLook || isAIM)
                {
                    GlobalAnimationHeaderAIM gaim;
                    if (!gaim.LoadAIM(compressedCAFPath.c_str(), eCAFLoadCompressedOnly))
                    {
                        RCLogError("Unable to load AIM/LOOK: '%s'.", compressedCAFPath.c_str());
                    }
                    else if (animationPath != gaim.m_FilePath)
                    {
                        RCLogError("AIM loaded with wrong m_FilePath:	'%s' (should be '%s').",
                            gaim.m_FilePath.c_str(), animationPath.c_str());
                    }
                    else
                    {
                        if (!animationManager.HasAIMHeader(gaim))
                        {
                            if (!animationManager.AddAIMHeaderOnly(gaim))
                            {
                                RCLogError("AIM header for '%s' already exists in CAnimationManager.",
                                    animationPath.c_str());
                            }
                        }
                    }
                    globalAnimationHeader.OnAimpose();     // we need this info to skip aim-poses in the IMG file
                }
                else
                {
                    globalAnimationHeader.SetFilePathCAF(animationPath.c_str());
                    string dbaPath;

                    if (m_dbaManager.get())
                    {
                        if (const char* dbaPathFound = m_dbaManager->FindDBAPath(animationPath.c_str()))
                        {
                            dbaPath = dbaPathFound;
                        }
                    }
                    if (m_dbaTableEnumerator.get())
                    {
                        if (const char* dbaPathFound = m_dbaTableEnumerator->FindDBAPath(animationPath.c_str(), animDesc.m_skeletonName.c_str(), animDesc.m_tags))
                        {
                            dbaPath = dbaPathFound;
                        }
                    }

                    globalAnimationHeader.SetFilePathDBA(dbaPath);
                    globalAnimationHeader.OnAssetOnDemand();
                    globalAnimationHeader.ClearAssetLoaded();

                    if (!animationManager.HasCAFHeader(globalAnimationHeader))
                    {
                        if (!dbaPath.empty())
                        {
                            RCLogError("Animation is expected to be added as a part of a DBA: %s", animationPath.c_str());
                        }
                        else
                        {
                            animationManager.AddCAFHeaderOnly(globalAnimationHeader);
                        }
                    }
                }
            }
        }
    }


    const string sAnimationsImgFilename = PathHelpers::Join(targetGameFolderPath, "Animations\\Animations.img");
    RCLog("Saving %s...", sAnimationsImgFilename);
    const FILETIME latest = FileUtil::GetLastWriteFileTime(targetGameFolderPath);
    if (!animationManager.SaveCAFImage(sAnimationsImgFilename, latest, bigEndianOutput))
    {
        RCLogError("Error saving Animations.img");
        return false;
    }

    string imageInputFile = m_cbaPath;
    if (imageInputFile.empty())
    {
        imageInputFile = PathHelpers::Join(PathHelpers::Join(m_sourceGameFolderPath, m_configSubfolder), "DBATable.json");
        if (!FileUtil::FileExists(imageInputFile.c_str()))
        {
            imageInputFile = PathHelpers::Join(PathHelpers::Join(m_sourceGameFolderPath, m_configSubfolder), "DBATable.xml");
        }
    }

    if (m_rc)
    {
        m_rc->AddInputOutputFilePair(imageInputFile.c_str(), sAnimationsImgFilename);
    }

    const string sDirectionalBlendsImgFilename = PathHelpers::Join(targetGameFolderPath, "Animations\\DirectionalBlends.img");
    RCLog("Saving %s...", sDirectionalBlendsImgFilename);
    if (!animationManager.SaveAIMImage(sDirectionalBlendsImgFilename, latest, bigEndianOutput))
    {
        RCLogError("Error saving DirectionalBlends.img");
        return false;
    }
    if (m_rc)
    {
        m_rc->AddInputOutputFilePair(imageInputFile.c_str(), sDirectionalBlendsImgFilename);
    }

    if (!unusedDBAs.empty())
    {
        RCLog("Following DBAs are not used anymore:");

        for (std::set<string>::iterator it = unusedDBAs.begin(); it != unusedDBAs.end(); ++it)
        {
            RCLog("    %s", it->c_str());
            if (m_rc)
            {
                const string removedFile = PathHelpers::Join(targetGameFolderPath, *it);
                m_rc->MarkOutputFileForRemoval(removedFile.c_str());
            }
        }
    }

    return true;
}

void CAnimationConvertor::Release()
{
    delete this;
}

ICompiler* CAnimationConvertor::CreateCompiler()
{
    return new CAnimationCompiler(this);
}

bool CAnimationConvertor::SupportsMultithreading() const
{
    return true;
}

void CAnimationConvertor::IncrementChangedAnimationCount()
{
    ++m_changedAnimationCount;
}

int CAnimationConvertor::IncrementFancyAnimationIndex()
{
    ++m_fancyAnimationIndex;
    return m_fancyAnimationIndex;
}

bool CAnimationConvertor::InLocalUpdateMode() const
{
    return m_config->GetAsBool("SkipDba", false, true);
}

const SAnimationDefinition* CAnimationConvertor::GetAnimationDefinition(const char* cafFilename) const
{
    if (!m_cbaLoader.get())
    {
        return 0;
    }

    return m_cbaLoader->GetAnimationDefinition(cafFilename);
}

void CAnimationConvertor::SetPlatform(int platform)
{
    if (m_platform < 0)
    {
        m_platform = platform;
    }
}

CBAEntry* CAnimationConvertor::GetEntryForDefinition(const SAnimationDefinition* definition)
{
    size_t index = definition - &m_cbaLoader->m_ADefinitions[0];
    if (index >= m_cbaLoader->m_ADefinitions.size())
    {
        assert(0 && "invalid definition");
        return 0;
    }

    return &m_cbaEntries[index];
}

const CBAEntry* CAnimationConvertor::GetEntryForDefinition(const SAnimationDefinition* definition) const
{
    return const_cast<CAnimationConvertor*>(this)->GetEntryForDefinition(definition);
}


const CSkeletonInfo* CAnimationConvertor::FindSkeleton(const SAnimationDesc* pAnimationDesc, const SAnimationDefinition* pDefinition) const
{
    assert(pAnimationDesc);

    if (!pAnimationDesc->m_skeletonName.empty())
    {
        return m_skeletonManager->FindSkeleton(pAnimationDesc->m_skeletonName);
    }
    else
    {
        if (!pDefinition)
        {
            return NULL;
        }

        const CBAEntry* const entry = GetEntryForDefinition(pDefinition);
        if (!entry)
        {
            return NULL;
        }

        if (!entry->m_skeleton.IsLoaded())
        {
            return NULL;
        }

        return &entry->m_skeleton.Skeleton();
    }
}


bool CAnimationConvertor::LoadCBA(const char* cbaPath, bool loadCharacters)
{
    m_cbaLoader.reset(new CAnimationInfoLoader(m_pXMLParser));

    const int verbosityLevel = m_rc->GetVerbosityLevel();

    // check if file missing and return silently
    if (!FileUtil::FileExists(cbaPath))
    {
        PakSystemFile* file = m_pPakSystem->Open(cbaPath, "rb");
        if (!file)
        {
            return false;
        }
        m_pPakSystem->Close(file);
    }

    RCLog("Loading CBA...");
    if (!m_cbaLoader->LoadDescription(cbaPath, m_pPakSystem))
    {
        return false;
    }

    const string platformName = m_config->GetAsString("p", "pc", "pc");

    const SPlatformAnimationSetup* pPlatformAnimationSetup = m_cbaLoader->GetPlatformSetup(platformName.c_str());
    if (pPlatformAnimationSetup)
    {
        m_platformAnimationSetup = *pPlatformAnimationSetup;
    }

    const size_t numDefinitions = m_cbaLoader->m_ADefinitions.size();
    m_cbaEntries.resize(numDefinitions);

    if (loadCharacters)
    {
        RCLog("Loading characters...");

        typedef std::map<string, const SkeletonLoader*> TSkeletonLoadersMap;
        TSkeletonLoadersMap loadedSkeletons;

        const string animationsFolder = PathHelpers::GetDirectory(cbaPath);
        for (size_t i = 0; i < numDefinitions; ++i)
        {
            CBAEntry& entry = m_cbaEntries[i];
            if (verbosityLevel >= 2)
            {
                RCLog("animationsFolder: %s", animationsFolder.c_str());
                RCLog("definition's m_Model: %s", m_cbaLoader->m_ADefinitions[i].m_Model.c_str());
            }
            string chrFilename = PathHelpers::Join(animationsFolder, m_cbaLoader->m_ADefinitions[i].m_Model);
            if (verbosityLevel >= 2)
            {
                RCLog("chrFilename: %s", chrFilename.c_str());
            }
            TSkeletonLoadersMap::const_iterator cit = loadedSkeletons.find(chrFilename);
            const bool bAlreadyLoadedSkeleton = (cit != loadedSkeletons.end());
            if (bAlreadyLoadedSkeleton)
            {
                entry.m_skeleton = *(cit->second);
            }
            else
            {
                entry.m_skeleton.Load(chrFilename, m_pPakSystem, m_pXMLParser, m_tempPath);
                loadedSkeletons[chrFilename] = &entry.m_skeleton;
            }
        }

        m_cbaPath = cbaPath;
    }
    return true;
}


//////////////////////////////////////////////////////////////////////////
CAnimationCompiler::CAnimationCompiler()
    : m_pXMLParser(nullptr)
    , m_convertor(nullptr)
{
}

CAnimationCompiler::CAnimationCompiler(CAnimationConvertor* convertor)
    : m_pXMLParser(convertor->GetXMLParser())
    , m_convertor(convertor)
{
}

CAnimationCompiler::~CAnimationCompiler()
{
}

string CAnimationCompiler::GetOutputFileNameOnly() const
{
    const bool localUpdateMode = Convertor()->InLocalUpdateMode();

    const string overwrittenFilename = m_CC.config->GetAsString("overwritefilename", m_CC.sourceFileNameOnly.c_str(), m_CC.sourceFileNameOnly.c_str());

    const string ext = PathHelpers::FindExtension(overwrittenFilename);
    if (StringHelpers::EqualsIgnoreCase(ext, "i_caf"))
    {
        if (localUpdateMode)
        {
            return PathHelpers::ReplaceExtension(overwrittenFilename, string("caf"));
        }
        else
        {
            return PathHelpers::ReplaceExtension(overwrittenFilename, string("$caf"));
        }
    }
    else
    {
        return PathHelpers::ReplaceExtension(overwrittenFilename, string("$") + ext);
    }
}

string CAnimationCompiler::GetOutputPath() const
{
    return PathHelpers::Join(m_CC.GetOutputFolder(), GetOutputFileNameOnly());
}

// ---------------------------------------------------------------------------

static void ProcessAnimationJob(AnimationJob* job);

static bool PrepareAnimationJob(
    AnimationJob* job,
    const char* sourcePath,
    const char* destinationPath,
    const char* sourceGameFolderPath,
    const SAnimationDesc& desc,
    const CSkeletonInfo* skeleton,
    const CInternalSkinningInfo* controllerSkinningInfo,
    const char* dbaPath,
    CAnimationCompiler* compiler)
{
    const ConvertContext& context = *(const ConvertContext*)compiler->GetConvertContext();

    bool bDebugCompression = context.config->GetAsBool("debugcompression", false, true);
    const bool bigEndianOutput = context.pRC->GetPlatformInfo(context.platform)->bBigEndian;
    bool bAlignTracks = context.config->GetAsBool("cafAlignTracks", false, true);

    string animationPath = UnifiedPath(RelativePath(sourcePath, sourceGameFolderPath));

    if (StringHelpers::Equals(PathHelpers::FindExtension(animationPath), "i_caf"))
    {
        animationPath = PathHelpers::ReplaceExtension(animationPath, "caf");
    }

    if (animationPath.empty())
    {
        RCLogError("Skipping animation %s because it's not in a subtree of the folder: %s  Is the animation somewhere in a folder that is part of your project or an enabled gem?", sourcePath, sourceGameFolderPath);
        return false;
    }

    job->m_compiler = compiler;
    job->m_singleFileMode = false;
    job->m_sourcePath = sourcePath;
    job->m_destinationPath = destinationPath;
    string animExt = PathHelpers::FindExtension(job->m_destinationPath);
    job->m_intermediatePath = PathHelpers::ReplaceExtension(job->m_destinationPath, string("$") + animExt); // e.g. .caf -> .$caf
    job->m_animationPath = animationPath;
    UnifyPath(job->m_animationPath);
    SAnimationDesc defaultAnimDesc;
    job->m_animDesc = desc;
    job->m_bigEndianOutput = bigEndianOutput;
    job->m_debugCompression = bDebugCompression;
    job->m_alignTracks = bAlignTracks;
    job->m_controllerSkinningInfo = controllerSkinningInfo;

    const bool bRefresh = context.config->GetAsBool("refresh", false, true);

    job->m_skeleton = skeleton;
    job->m_bigEndianOutput = bigEndianOutput;

    const bool bLocalUpdateMode = compiler->Convertor() ? compiler->Convertor()->InLocalUpdateMode() : true;
    const bool bUseDBA = (dbaPath != NULL && dbaPath[0] != '\0' && !job->m_animDesc.m_bSkipSaveToDatabase && !bLocalUpdateMode);
    if (bUseDBA)
    {
        job->m_dbaPath = UnifiedPath(dbaPath);
    }
    else
    {
        job->m_dbaPath = "";
    }

    job->m_writeIntermediateFile = !bLocalUpdateMode;
    job->m_writeDestinationFile = !bUseDBA;


    if (bRefresh ||
        (job->m_writeIntermediateFile && CheckIfRefreshNeeded(job->m_sourcePath.c_str(), job->m_intermediatePath.c_str())) ||
        (job->m_writeDestinationFile && CheckIfRefreshNeeded(job->m_sourcePath.c_str(), job->m_destinationPath.c_str())) ||
        (job->m_writeIntermediateFile && HasAnimationSettingsFileChanged(job->m_sourcePath, job->m_intermediatePath)) ||
        (job->m_writeDestinationFile && HasAnimationSettingsFileChanged(job->m_sourcePath, job->m_destinationPath)))
    {
        job->m_fileChanged = true;
    }

    return true;
}

static bool GetFromDefinition(SAnimationDesc* desc, const CSkeletonInfo** skeleton, string* dbaPath,
    const SAnimationDefinition& definition, CAnimationConvertor* convertor, const char* fileName)
{
    *desc = definition.GetAnimationDesc(fileName);
    *skeleton = convertor->FindSkeleton(desc, &definition);

    if (!definition.m_DBName.empty())
    {
        *dbaPath = string("animations/") + definition.m_DBName;
    }
    else
    {
        dbaPath->clear();
    }
    return true;
}

static bool GetFromAnimSettings(SAnimationDesc* desc, const CSkeletonInfo** skeleton, bool* pbErrorReported, const char* sourcePath, const char* animationName, IPakSystem* pakSystem, ICryXML* xmlParser, SkeletonManager* skeletonManager)
{
    bool usesNameContains = false;
    if (!SAnimationDefinition::GetDescFromAnimationSettingsFile(desc, pbErrorReported, &usesNameContains, pakSystem, xmlParser, sourcePath, std::vector<string>()))
    {
        return false;
    }

    if (desc->m_skeletonName.empty())
    {
        RCLogError("No skeleton alias specified for animation: '%s'", sourcePath);
        *pbErrorReported = true;
        return false;
    }

    *skeleton = skeletonManager->FindSkeleton(desc->m_skeletonName);
    if (!*skeleton)
    {
        RCLogError("Missing skeleton alias '%s' for animation '%s'", desc->m_skeletonName.c_str(), sourcePath);
        *pbErrorReported = true;
        return false;
    }

    if (usesNameContains)
    {
        if (!SAnimationDefinition::GetDescFromAnimationSettingsFile(desc, pbErrorReported, 0, pakSystem, xmlParser, sourcePath, GetJointNames(*skeleton)))
        {
            return false;
        }
    }
    return true;
}

bool CAnimationCompiler::ProcessCBA()
{
    const int verbosityLevel = m_CC.pRC->GetVerbosityLevel();

    const bool bDebugCompression = m_CC.config->GetAsBool("debugcompression", false, true);

    bool bRefresh = m_CC.config->GetAsBool("refresh", false, true);

    string singleFile = m_CC.config->GetAsString("file", "", "");
    const bool bSingleFile = !singleFile.empty();

    if (!StringHelpers::EqualsIgnoreCase(m_CC.convertorExtension, "cba"))
    {
        RCLogError("Expected cba file as input");
        return false;
    }

    if (!bSingleFile)
    {
        FILETIME srcFileTimestamp;
        if (CheckIfRefreshNeeded(m_CC.GetSourcePath(), GetOutputPath(), &srcFileTimestamp))
        {
            // Force refresh if Animations.cba is changed
            bRefresh = true;
            AZ::IO::LocalFileIO().Copy(m_CC.GetSourcePath(), GetOutputPath());
            FileUtil::SetFileTimes(m_CC.GetSourcePath(), GetOutputPath());
        }
    }

    const bool bBigEndianOutput = m_CC.pRC->GetPlatformInfo(m_CC.platform)->bBigEndian;
    if (verbosityLevel >= 1 && bBigEndianOutput)
    {
        RCLog("Endian conversion specified");
    }

    const string cbaFilePath = m_CC.GetSourcePath();
    const string cbaFolderPath = UnifiedPath(PathHelpers::GetDirectory(PathHelpers::GetAbsoluteAsciiPath(cbaFilePath)));

    // Deduce from Animations/Animations.cba
    const string sourceGameFolderPath = UnifiedPath(PathHelpers::GetDirectory(cbaFolderPath));

    if (verbosityLevel >= 2)
    {
        RCLog("cbaFilePath: %s", cbaFilePath.c_str());
        RCLog("sourceGameFolderPath: %s", sourceGameFolderPath.c_str());
        RCLog("single file: %s", (bSingleFile ? "Yes" : "No"));
    }

    string destFolder;
    if (m_CC.config->HasKey("dest"))
    {
        RCLogWarning("Command-line option 'dest' is obsolete and will be removed soon. Please use 'targetroot' pointing to folder with .cba file instead.");
        if (m_CC.config->HasKey("targetroot"))
        {
            destFolder = m_CC.config->GetAsString("targetroot", "", "");
        }
        else
        {
            destFolder = m_CC.config->GetAsString("dest", "", "");
        }
    }
    else
    {
        destFolder = m_CC.config->GetAsString("targetroot", "", "");
    }

    if (m_CC.config->GetAsBool("report", false, true))
    {
        return true;
    }

    const bool bAlignTracks = m_CC.config->GetAsBool("cafAlignTracks", false, true);

    //------------------------------------------------------------------------------------
    if (bSingleFile)
    {
        if (!Convertor()->LoadCBA(cbaFilePath.c_str(), false))
        {
            RCLogError("Unable to load CBA (animation description file): %s", cbaFilePath.c_str());
            return false;
        }

        // Most probably we are exporting animation from DCC tool.
        const SAnimationDefinition* const definition = Convertor()->GetAnimationDefinition(singleFile.c_str());
        if (!definition)
        {
            RCLogError(
                "Unable to find animation definition for %s.\n"
                "Game data folder path: %s",
                singleFile.c_str(), sourceGameFolderPath.c_str());
            return false;
        }

        // file may be in a pakfile, so extract it if required
        string chrFilename = PathHelpers::Join(m_CC.sourceFolder, definition->m_Model);
        IPakSystem* const pPakSystem = Convertor()->GetPakSystem();
        SkeletonLoader chrLoader;
        CSkeletonInfo* const skeleton = chrLoader.Load(chrFilename.c_str(), pPakSystem, Convertor()->GetXMLParser(), Convertor()->GetTempPath_());
        if (!skeleton)
        {
            RCLogError("Unable to load CHR model %s from temporary file %s", chrFilename.c_str(), chrLoader.GetTempName());
            return false;
        }

        UnifyPath(singleFile);

        const string sourceAnimationsFolder = sourceGameFolderPath + "\\animations";
        const string fileNameRelativeToAnimations = UnifiedPath(RelativePath(singleFile, sourceAnimationsFolder));
        if (fileNameRelativeToAnimations.empty())
        {
            RCLogError("/file=%s lies outside animations (%s) folder", singleFile.c_str(), sourceAnimationsFolder.c_str());
            return false;
        }

        AnimationJob job;

        job.m_singleFileMode = true;
        job.m_debugCompression = true;
        job.m_compiler = this;
        job.m_sourcePath = singleFile;
        job.m_animationPath = string("animations/") + fileNameRelativeToAnimations;
        UnifyPath(job.m_animationPath);
        job.m_animDesc = definition->GetAnimationDesc(fileNameRelativeToAnimations);
        job.m_fileChanged = true;
        if (!definition->m_DBName.empty())
        {
            job.m_dbaPath = definition->m_DBName.empty() ? string() : (string("animations/") + definition->m_DBName);
            if (job.m_dbaPath.empty())
            {
                RCLogError("/file=%s uses dba %s which lies outside animations (%s) folder", singleFile.c_str(), definition->m_DBName.c_str(), sourceAnimationsFolder.c_str());
                return false;
            }
        }

        job.m_skeleton = skeleton;
        job.m_bigEndianOutput = bBigEndianOutput;
        job.m_writeIntermediateFile = true;
        job.m_writeDestinationFile = true;
        job.m_alignTracks = bAlignTracks;

        RCLog("Processing %s...", job.m_animationPath);
        // no multithreading, simple call
        ProcessAnimationJob(&job);

        if (job.m_resSize != 0)
        {
            RCLog("Animation processed.");
            return true;
        }
        else
        {
            AZ::IO::LocalFileIO().Remove(singleFile);
            RCLog("Failed to process animation");
            return false;
        }
    }
    else
    {
        if (!Convertor()->LoadCBA(cbaFilePath.c_str(), true))
        {
            const bool bSkipMissing = m_CC.config->GetAsBool("skipmissing", false, true);
            if (bSkipMissing)
            {
                RCLog("Missing CBA skipped: %s", cbaFilePath.c_str());
                return true;
            }
            else
            {
                RCLogError("Unable to load CBA (animation description file): %s", cbaFilePath.c_str());
                return false;
            }
        }

        CAnimationManager animationManager;
        ThreadUtils::StealingThreadPool pool(m_CC.threads, true);

        RCLog("Processing CBA, source folder: %s", m_CC.sourceFolder);

        std::list<AnimationJob> jobs;

        size_t numJobsSubmitted = 0;
        size_t numDefinitions = Convertor()->GetCBALoader()->m_ADefinitions.size();
        for (size_t i = 0; i < numDefinitions; ++i)
        {
            const SAnimationDefinition* definition = &Convertor()->GetCBALoader()->m_ADefinitions[i];

            std::vector<CAFFileItem> items;
            FindCAFsForDefinition(&items, m_CC.sourceFolder, *definition, *Convertor()->GetCBALoader());

            if (items.empty())
            {
                RCLogWarning(
                    "No animations found in %s (dba: \"%s\")",
                    definition->GetAnimationPathWithoutSlash().c_str(), definition->m_DBName.c_str());
                continue;
            }

            string sourceDir = PathHelpers::AddSeparator(PathHelpers::Join(m_CC.sourceFolder, definition->GetAnimationPathWithoutSlash()));

            const size_t numFiles = items.size();
            for (size_t anim = 0; anim < numFiles; ++anim)
            {
                const string& fileName = items[anim].m_name;
                string sourcePath = PathHelpers::ToUnixPath(PathHelpers::Join(sourceDir, fileName));

                string destinationPath = PathHelpers::Join(definition->GetAnimationPath(), PathHelpers::ReplaceExtension(fileName, "caf"));
                destinationPath = PathHelpers::Join(destFolder, destinationPath);

                jobs.insert(jobs.end(), AnimationJob());
                AnimationJob& job = jobs.back();

                SAnimationDesc animDesc;
                const CSkeletonInfo* skeleton;
                string dbaPath;

                if (!GetFromDefinition(&animDesc, &skeleton, &dbaPath,
                        *definition, Convertor(), fileName.c_str()))
                {
                    RCLogError("Can't get definition parameters for: %s", sourcePath.c_str());
                    continue;
                }

                if (!skeleton)
                {
                    RCLogError("Skipping animation '%s' because could not find the compile skeleton for it", fileName.c_str());
                    continue;
                }

                if (PrepareAnimationJob(&job,
                        sourcePath.c_str(),
                        destinationPath.c_str(),
                        sourceGameFolderPath.c_str(),
                        animDesc,
                        skeleton,
                        nullptr,
                        dbaPath, this))
                {
                    pool.Submit(&ProcessAnimationJob, &job);
                    ++numJobsSubmitted;
                }
            }
        }

        if (numJobsSubmitted > 0)
        {
            pool.Start();
            pool.WaitAllJobs();
        }
    }

    return true;
}


void CAnimationCompiler::GetFilenamesForUpToDateCheck(std::vector<string>& upToDateSrcFilenames, std::vector<string>& upToDateCheckFilenames)
{
    upToDateSrcFilenames.clear();
    upToDateCheckFilenames.clear();

    const string sourcePath = m_CC.GetSourcePath();
    const string outputPath = GetOutputPath();

    // "Main" file
    upToDateSrcFilenames.push_back(sourcePath);
    if (!StringHelpers::EqualsIgnoreCase(PathHelpers::FindExtension(sourcePath), "cba") || m_CC.config->GetAsBool("cbaUpdate", false, true))
    {
        upToDateCheckFilenames.push_back(outputPath);
    }
    else
    {
        upToDateCheckFilenames.push_back(string());
    }

    const string sourcePathExt = PathHelpers::FindExtension(sourcePath);
    const bool isCafFile = StringHelpers::EqualsIgnoreCase(sourcePathExt, "caf") ||
        StringHelpers::EqualsIgnoreCase(sourcePathExt, "i_caf");

    if (isCafFile)
    {
        const string sourceAnimSettingsFilename = SAnimationDefinition::GetAnimationSettingsFilename(sourcePath, SAnimationDefinition::ePASF_OVERRIDE_FILE);
        if (FileUtil::FileExists(sourceAnimSettingsFilename))
        {
            const string animationSettingsFilenameCheck = SAnimationDefinition::GetAnimationSettingsFilename(outputPath, SAnimationDefinition::ePASF_UPTODATECHECK_FILE);

            upToDateSrcFilenames.push_back(sourceAnimSettingsFilename);
            upToDateCheckFilenames.push_back(animationSettingsFilenameCheck);
        }
    }
}


bool CAnimationCompiler::CheckIfAllFilesAreUpToDate(const std::vector<string>& upToDateSrcFilenames, const std::vector<string>& upToDateCheckFilenames)
{
    const int verbosityLevel = m_CC.pRC->GetVerbosityLevel();

    bool bAllFileTimesMatch = true;
    for (size_t i = 0; i < upToDateSrcFilenames.size() && bAllFileTimesMatch; ++i)
    {
        if (upToDateCheckFilenames[i].empty())
        {
            bAllFileTimesMatch = false;
        }
        else
        {
            bAllFileTimesMatch = UpToDateFileHelpers::FileExistsAndUpToDate(upToDateCheckFilenames[i], upToDateSrcFilenames[i]);
        }
    }

    if (bAllFileTimesMatch)
    {
        for (size_t i = 0; i < upToDateSrcFilenames.size(); ++i)
        {
            m_CC.pRC->AddInputOutputFilePair(upToDateSrcFilenames[i], GetOutputPath());
        }

        if (verbosityLevel > 0)
        {
            if (upToDateSrcFilenames.size() == 1)
            {
                RCLog("Skipping %s: File %s is up to date.", m_CC.GetSourcePath().c_str(), upToDateCheckFilenames[0].c_str());
            }
            else
            {
                RCLog("Skipping %s:", m_CC.GetSourcePath().c_str());
                for (size_t i = 0; i < upToDateSrcFilenames.size(); ++i)
                {
                    RCLog("  File %s is up to date", upToDateCheckFilenames[i].c_str());
                }
            }
        }
        return true;
    }

    return false;
}


// Update input-output file list, set source's time & date to output files.
void CAnimationCompiler::HandleUpToDateCheckFilesOnReturn(const std::vector<string>& upToDateSrcFilenames, const std::vector<string>& upToDateCheckFilenames)
{
    const int verbosityLevel = m_CC.pRC->GetVerbosityLevel();

    // 1) we need to call AddInputOutputFilePair() for each up-to-date-check file
    // 2) we need to enforce that every up-to-date-check file exists
    //    (except the main up-to-date-check file which expected to be created by Process())

    for (size_t i = 0; i < upToDateSrcFilenames.size(); ++i)
    {
        m_CC.pRC->AddInputOutputFilePair(upToDateSrcFilenames[i], GetOutputPath());

        if (!upToDateCheckFilenames[i].empty())
        {
            if (i > 0 && !FileUtil::FileExists(upToDateCheckFilenames[i].c_str()))
            {
                if (verbosityLevel > 0)
                {
                    RCLog("Creating file '%s' (used to check if '%s' is up to date).", upToDateCheckFilenames[i].c_str(), m_CC.GetSourcePath().c_str());
                }
                FILE* pEmptyFile = nullptr; 
                azfopen(&pEmptyFile, upToDateCheckFilenames[i].c_str(), "wt");
                if (pEmptyFile)
                {
                    fclose(pEmptyFile);
                }
                else
                {
                    RCLogWarning("Failed to create file '%s' (used to check if '%s' is up to date).", upToDateCheckFilenames[i].c_str(), m_CC.GetSourcePath().c_str());
                }
            }

            UpToDateFileHelpers::SetMatchingFileTime(upToDateCheckFilenames[i].c_str(), upToDateSrcFilenames[i].c_str());
        }
    }
}


static void ProcessAnimationJob(AnimationJob* job);

bool CAnimationCompiler::Process()
{
#if defined(AZ_PLATFORM_WINDOWS)
    MathHelpers::AutoFloatingPointExceptions autoFpe(0);  // TODO: it's better to replace it by autoFpe(~(_EM_INEXACT | _EM_UNDERFLOW)). it was tested and works.
#endif
    
    bool hasDccName = MetricsScope::HasDccName(m_CC.config);
    MetricsScope metrics = MetricsScope(hasDccName, "ResourceCompilerPC", static_cast<uint32>(-1));
    if (hasDccName)
    {
        metrics.RecordDccName(m_convertor, m_CC.config, "AnimationCompiler", GetOutputPath().c_str());
    }

    Convertor()->SetPlatform(m_CC.platform); // hackish

    const int verbosityLevel = m_CC.pRC->GetVerbosityLevel();

    std::vector<string> upToDateSrcFilenames;
    std::vector<string> upToDateCheckFilenames;
    GetFilenamesForUpToDateCheck(upToDateSrcFilenames, upToDateCheckFilenames);

    if (!m_CC.bForceRecompiling && CheckIfAllFilesAreUpToDate(upToDateSrcFilenames, upToDateCheckFilenames))
    {
        return true;
    }

    if (StringHelpers::EqualsIgnoreCase(m_CC.convertorExtension, "cba"))
    {
        // old way to invoke AnimationCompiler was used:
        // rc.exe Animations.cba /file=
        //   OR
        // rc.exe Animations.cba /file=single_animation.caf
        const bool bOk = ProcessCBA();
        if (bOk)
        {
            HandleUpToDateCheckFilesOnReturn(upToDateSrcFilenames, upToDateCheckFilenames);
        }
        return bOk;
    }

    if (!StringHelpers::EqualsIgnoreCase(m_CC.convertorExtension, "i_caf"))
    {
        RCLogError("Expected i_caf file as input");
        return false;
    }

    // new way to invoke rc:
    // rc.exe *.i_caf /animConfigFolder=Animations
    
    const string& sourceWatchFolderPath = Convertor()->GetSourceWatchFolderPath();
    const string fileName = UnifiedPath(RelativePath(m_CC.GetSourcePath(), sourceWatchFolderPath));
    const string animationName = PathHelpers::ReplaceExtension(fileName, "caf");
    if (fileName.empty())
    {
        RCLogError(
            "Animation %s cannot be processed, because it's not in a subtree of the folder: %s  Is the animation somewhere in a folder that is part of your project or an enabled gem?",
            m_CC.GetSourcePath().c_str(), sourceWatchFolderPath.c_str());

        return false;
    }

    const string outputPath = PathHelpers::ReplaceExtension(GetOutputPath(), "caf");

    if (verbosityLevel > 0)
    {
        RCLog("sourceWatchFolderPath: %s", sourceWatchFolderPath.c_str());
        RCLog("outputPath: %s", outputPath.c_str());
    }

    SAnimationDesc animDesc;
    const CSkeletonInfo* skeleton = 0;

    const string& sourcePath = m_CC.GetSourcePath();

    bool bErrorReported = false;

    string dbaPath;
    animDesc.m_bSkipSaveToDatabase = true;

    if (!GetFromAnimSettings(&animDesc, &skeleton, &bErrorReported,
            sourcePath.c_str(), animationName.c_str(),
            Convertor()->GetPakSystem(),
            Convertor()->GetXMLParser(),
            Convertor()->m_skeletonManager.get()))
    {
        const SAnimationDefinition* definition = Convertor()->GetAnimationDefinition(fileName);
        if (!definition ||
            !GetFromDefinition(&animDesc, &skeleton, &dbaPath, *definition, Convertor(), fileName.c_str()))
        {
            if (!bErrorReported)
            {
                RCLogWarning("Can't load animations settings for animation: %s", sourcePath.c_str());
            }
            return true;
        }
    }

    const bool isAIM = IsAimAnimation(skeleton->m_SkinningInfo, animationName.c_str());
    const bool isLook = IsLookAnimation(skeleton->m_SkinningInfo, animationName.c_str());
    if (!(isAIM || isLook) && (!Convertor()->InLocalUpdateMode() && Convertor()->m_dbaManager.get() || Convertor()->m_dbaTableEnumerator.get()))
    {
        const char* dbaPathFromTable = 0;
        if (Convertor()->m_dbaManager.get())
        {
            dbaPathFromTable = Convertor()->m_dbaManager->FindDBAPath(animationName);
        }
        if (Convertor()->m_dbaTableEnumerator.get())
        {
            dbaPathFromTable = Convertor()->m_dbaTableEnumerator->FindDBAPath(animationName, animDesc.m_skeletonName.c_str(), animDesc.m_tags);
        }

        if (dbaPathFromTable)
        {
            dbaPath = dbaPathFromTable;
            animDesc.m_bSkipSaveToDatabase = false;
        }
        else
        {
            dbaPath.clear();
            animDesc.m_bSkipSaveToDatabase = true;
        }
    }
    else
    {
        dbaPath.clear();
        animDesc.m_bSkipSaveToDatabase = true;
    }

    if (isAIM || isLook)
    {
        // aim and look caf-s are collection of posses and should not be compressed
        animDesc.format.oldFmt.m_CompressionQuality = 0;
        animDesc.format.oldFmt.m_fPOS_EPSILON = 0;
        animDesc.format.oldFmt.m_fROT_EPSILON = 0;
        animDesc.m_bNewFormat = false;
    }
    else if (Convertor()->m_compressionPresetTable.get())
    {
        const SCompressionPresetEntry* const preset = Convertor()->m_compressionPresetTable.get()->FindPresetForAnimation(animationName.c_str(), animDesc.m_tags, animDesc.m_skeletonName.c_str());
        if (preset)
        {
            const bool bDebugCompression = m_CC.config->GetAsBool("debugcompression", false, true);
            if (bDebugCompression)
            {
                RCLog("Applying compression preset '%s' to animation '%s'", preset->name.c_str(), animationName.c_str());
            }

            animDesc.format.oldFmt.m_CompressionQuality = preset->settings.m_compressionValue;
            animDesc.format.oldFmt.m_fPOS_EPSILON = preset->settings.m_positionEpsilon;
            animDesc.format.oldFmt.m_fROT_EPSILON = preset->settings.m_rotationEpsilon;
            animDesc.m_perBoneCompressionDesc.clear();
            for (size_t i = 0; i < preset->settings.m_controllerCompressionSettings.size(); ++i)
            {
                const string& jointName = preset->settings.m_controllerCompressionSettings[i].first;
                const SControllerCompressionSettings& presetJointSettings = preset->settings.m_controllerCompressionSettings[i].second;
                SBoneCompressionDesc jointSettings;
                jointSettings.m_namePattern = jointName;
                jointSettings.format.oldFmt.m_mult = presetJointSettings.multiply;
                jointSettings.m_eDeletePos = presetJointSettings.state == eCES_ForceDelete ? SBoneCompressionValues::eDelete_Yes : SBoneCompressionValues::eDelete_Auto;
                jointSettings.m_eDeleteRot = jointSettings.m_eDeletePos;
                animDesc.m_perBoneCompressionDesc.push_back(jointSettings);
            }

            // TODO: Using old format for now, to keep existing data working
            // should transition to a newer one.
            animDesc.m_bNewFormat = false;
        }
    }

    if (!skeleton)
    {
        RCLogError("Unable to load skeleton for animation: %s", sourcePath.c_str());
        return false;
    }

    AnimationJob job;

    if (PrepareAnimationJob(&job,
        sourcePath.c_str(),
        outputPath.c_str(),
        sourceWatchFolderPath.c_str(),
            animDesc,
            skeleton,
            nullptr,
            dbaPath,
            this))
    {
        ProcessAnimationJob(&job);
    }

    const bool bOk = (job.m_resSize != 0);
    if (bOk)
    {
        HandleUpToDateCheckFilesOnReturn(upToDateSrcFilenames, upToDateCheckFilenames);
    }
    return bOk;
}

static void ProcessAnimationJob(AnimationJob* job)
{
    CAnimationConvertor* convertor = job->m_compiler->Convertor();
    const ConvertContext& context = *(const ConvertContext*)job->m_compiler->GetConvertContext();
    const bool bDebugCompression = job->m_debugCompression;

    const SPlatformAnimationSetup* platform = &convertor->GetPlatformSetup();
    const SAnimationDesc* animDesc = &job->m_animDesc;
    
    if (job->m_singleFileMode)
    {
        // Converting an animation exported from DCC Tool. Production mode.
        CAnimationCompressor compressor(*job->m_skeleton);

        if (!compressor.LoadCAF(job->m_sourcePath.c_str(), *platform, *animDesc, job->m_controllerSkinningInfo, eCAFLoadUncompressedOnly))
        {
            // We have enough logging in compressor
            return;
        }
        compressor.m_GlobalAnimationHeader.SetFilePathCAF(job->m_animationPath);

        compressor.EnableTrackAligning(job->m_alignTracks);

        if (!compressor.ProcessAnimation(bDebugCompression))
        {
            RCLogError("Error in processing %s", job->m_sourcePath.c_str());
            return;
        }

        job->m_resSize = compressor.AddCompressedChunksToFile(job->m_sourcePath.c_str(), job->m_bigEndianOutput);
        RCLog("Animation: %s (%s)", job->m_sourcePath.c_str(), job->m_animationPath.c_str());
    }
    else
    {
        int processStartTime = GetTickCount();

        // Full or incremental build without specified animation
        const bool fileChanged = job->m_fileChanged;
        const bool writeIntermediate = job->m_writeIntermediateFile;
        const bool writeDest = job->m_writeDestinationFile;
        const string& sourcePath = job->m_sourcePath;
        const string& intermediatePath = job->m_intermediatePath;
        const string& destPath = job->m_destinationPath;
        const string& reportFile = writeDest ? destPath : intermediatePath;

        const bool isAIM = IsAimAnimation(job->m_skeleton->m_SkinningInfo, job->m_animationPath);
        const bool isLook = IsLookAnimation(job->m_skeleton->m_SkinningInfo, job->m_animationPath);

        bool failedToLoadCompressed = false;

        GlobalAnimationHeaderAIM gaim;
        if (isAIM || isLook)
        {
            if (!fileChanged)
            {
                // trying to load processed AIM from .$caf-file
                if (!gaim.LoadAIM(intermediatePath.c_str(), eCAFLoadCompressedOnly))
                {
                    RCLogError("Unable to load AIM: %s", intermediatePath.c_str());
                    failedToLoadCompressed = true;
                    gaim = GlobalAnimationHeaderAIM();
                }
                else if (job->m_animationPath != gaim.m_FilePath)
                {
                    RCLogError("AIM loaded with wrong m_FilePath: \"%s\" (should be \"%s\")", gaim.m_FilePath.c_str(), job->m_animationPath.c_str());
                    failedToLoadCompressed = true;
                    gaim = GlobalAnimationHeaderAIM();
                }
            }

            if (fileChanged || failedToLoadCompressed)
            {
                gaim.SetFilePath(job->m_animationPath.c_str());
                if (!gaim.LoadAIM(sourcePath.c_str(), eCAFLoadUncompressedOnly))
                {
                    RCLogError("Unable to load AIM: %s", sourcePath.c_str());
                    return;
                }

                CAnimationCompressor compressor(*job->m_skeleton);
                compressor.SetIdentityLocatorLegacy(gaim);

                if (isAIM)
                {
                    if (!compressor.ProcessAimPoses(false, gaim))
                    {
                        RCLogError("Failed to process AIM pose for file: '%s'.", sourcePath.c_str());
                        return;
                    }

                    if (!gaim.IsValid())
                    {
                        RCLogError("ProcessAimPoses outputs NANs for file: '%s'.", sourcePath.c_str());
                        return;
                    }
                }

                if (isLook)
                {
                    if (!compressor.ProcessAimPoses(true, gaim))
                    {
                        RCLogError("Failed to process AIM pose for file: '%s'.", sourcePath.c_str());
                        return;
                    }
                    if (!gaim.IsValid())
                    {
                        RCLogError("ProcessAimPoses outputs NANs for file: '%s'.", sourcePath.c_str());
                        return;
                    }
                }
            }
        }

        CAnimationCompressor compressor(*job->m_skeleton);

        if (!fileChanged && !failedToLoadCompressed)
        {
            // update mode
            const char* loadFrom = writeIntermediate ? intermediatePath.c_str() : destPath.c_str();
            if (!compressor.LoadCAF(loadFrom, *platform, *animDesc, job->m_controllerSkinningInfo, eCAFLoadCompressedOnly))
            {
                RCLogError("Unable to load compressed animation from %s", loadFrom);
                failedToLoadCompressed = true;
                compressor = CAnimationCompressor(*job->m_skeleton);
            }
            compressor.m_GlobalAnimationHeader.SetFilePathCAF(job->m_animationPath);
            if (isAIM || isLook)
            {
                compressor.m_GlobalAnimationHeader.OnAimpose();
            }
        }

        if (fileChanged || failedToLoadCompressed)
        {
            if (failedToLoadCompressed)
            {
                RCLog("Trying to recompile animation %s", sourcePath.c_str());
            }
            // rebuild animation
            if (!compressor.LoadCAF(sourcePath.c_str(), *platform, *animDesc, job->m_controllerSkinningInfo, eCAFLoadUncompressedOnly))
            {
                // We have enough logging in compressor
                return;
            }
            compressor.EnableTrackAligning(job->m_alignTracks);
            compressor.m_GlobalAnimationHeader.SetFilePathCAF(job->m_animationPath);
            if (!compressor.ProcessAnimation(bDebugCompression))
            {
                RCLogError("Error in processing %s", sourcePath.c_str());
                return;
            }

            compressor.m_GlobalAnimationHeader.m_nCompressedControllerCount = compressor.m_GlobalAnimationHeader.CountCompressedControllers();

            if (isAIM || isLook)
            {
                compressor.m_GlobalAnimationHeader.OnAimpose(); // we need this info to skip aim-poses in the IMG file
            }

            // Create the directory (including intermediate ones), if not already there.
            if (!FileUtil::EnsureDirectoryExists(PathHelpers::GetDirectory(job->m_destinationPath).c_str()))
            {
                RCLogError("Failed to create directory for '%s'.", job->m_destinationPath.c_str());
                return;
            }

            const FILETIME sourceWriteTime = FileUtil::GetLastWriteFileTime(sourcePath.c_str());
            if (writeIntermediate)
            {
                GlobalAnimationHeaderAIM* gaimToSave = (isAIM || isLook) ? &gaim : 0;
                if (gaimToSave && !gaimToSave->IsValid())
                {
                    RCLogError("Trying to save AIM header with invalid values (NANs): '%s'.", intermediatePath.c_str());
                    return;
                }
                job->m_resSize = compressor.SaveOnlyCompressedChunksInFile(intermediatePath.c_str(), sourceWriteTime, gaimToSave, true, false); // little-endian format
                context.pRC->AddInputOutputFilePair(sourcePath, intermediatePath);
            }

            if (writeDest)
            {
                job->m_resSize = compressor.SaveOnlyCompressedChunksInFile(destPath.c_str(), sourceWriteTime, 0, false, job->m_bigEndianOutput);
                context.pRC->AddInputOutputFilePair(sourcePath, destPath);
            }

            convertor->IncrementChangedAnimationCount();
        }
        else
        {
            if (writeIntermediate)
            {
                job->m_resSize = (size_t)FileUtil::GetFileSize(intermediatePath.c_str());
            }

            if (writeDest)
            {
                job->m_resSize = (size_t)FileUtil::GetFileSize(destPath.c_str());
            }
        }

        if (!job->m_dbaPath.empty())
        {
            compressor.m_GlobalAnimationHeader.SetFilePathDBA(job->m_dbaPath);
        }

        if (context.pRC->GetVerbosityLevel() >= 1)
        {
            const int durationMS = GetTickCount() - processStartTime;

            const char* const animPath = compressor.m_GlobalAnimationHeader.m_FilePath.c_str();
            const char* const dbaPath = compressor.m_GlobalAnimationHeader.m_FilePathDBA.c_str();
            const uint32 crc32 = compressor.m_GlobalAnimationHeader.m_FilePathDBACRC32;
            const size_t compressedControllerCount = compressor.m_GlobalAnimationHeader.m_nCompressedControllerCount;

            char processTime[32];
            if (fileChanged || failedToLoadCompressed)
            {
                azsnprintf(processTime, sizeof(processTime), "in %.1f sec", float(durationMS) * 0.001f);
            }
            else
            {
                cry_strcpy(processTime, "is up to date");
            }
            const int fancyAnimationIndex = convertor->IncrementFancyAnimationIndex();
            const char* const aimState = isAIM || isLook ? "AIM" : "   ";
            RCLog("CAF-%04d %13s %s 0x%08X ctrls:%03d %s %s", fancyAnimationIndex, processTime, aimState, crc32, compressedControllerCount, animPath, dbaPath);
        }
    }
}

static void ProcessAnimationJobWithInMemoryData(AnimationJob* job, CContentCGF* content, CInternalSkinningInfo* controllerSkinningInfo);

//
// Implemented by referencing to CAnimationCompiler::Process()
// Compress animation with in-memory animation data, not through triggering by animation convertor which loads i_caf and then does compression
//
bool CAnimationCompiler::ProcessInMemoryData(CContentCGF* content, CInternalSkinningInfo* controllerSkinningInfo, const string& outputFile, const AZ::SceneAPI::DataTypes::IAnimationGroup* animationGroup)
{
#if defined(AZ_PLATFORM_WINDOWS)
    MathHelpers::AutoFloatingPointExceptions autoFpe(0);  // TODO: it's better to replace it by autoFpe(~(_EM_INEXACT | _EM_UNDERFLOW)). it was tested and works.
#endif
    if (!StringHelpers::EqualsIgnoreCase(m_CC.convertorExtension, "fbx"))
    {
        RCLogError("Expected fbx file as input");
        return false;
    }

    const string sourceGameFolderPath = m_CC.sourceFolder;
    const string sourcePath = PathHelpers::ReplaceExtension(m_CC.GetSourcePath(), "i_caf");
    // outputPath should be an asset id so that caf will be output to cache folder
    const string outputPath = PathHelpers::ReplaceExtension(outputFile, "caf");

    AnimationJob job;
    SAnimationDesc animDesc(true);
    const CSkeletonInfo* skeleton = nullptr;
    string dbaPath;

    // Copy compression settings from IAnimationGroup to animDesc (Specific to FBX pipeline).
    if (animationGroup)
    {
        animDesc.m_bNewFormat = true; 

        const float defaultCompressionStrength = (animationGroup->GetDefaultCompressionStrength());

        animDesc.format.newFmt.m_autodeletePosEps = 0.0001f * defaultCompressionStrength * 1000.f;
        animDesc.format.newFmt.m_autodeleteRotEps = 0.0005f * defaultCompressionStrength * 1000.f;
        animDesc.format.newFmt.m_compressPosEps = 0.001f * defaultCompressionStrength * 1000.f;
        animDesc.format.newFmt.m_compressRotEps = 0.005f * defaultCompressionStrength * 1000.f;

        for (const auto& boneCompression : animationGroup->GetPerBoneCompression())
        {
            const float boneCompressionStrength = (boneCompression.m_compressionStrength);

            SBoneCompressionDesc boneDesc;
            boneDesc.m_eDeletePos = SBoneCompressionValues::eDelete_No; // Deletion relies on rig/skeleton
            boneDesc.m_eDeleteRot = SBoneCompressionValues::eDelete_No; // Deletion relies on rig/skeleton
            boneDesc.m_namePattern = boneCompression.m_boneNamePattern.c_str();
            boneDesc.format.newFmt.m_compressPosEps = 0.001f * boneCompressionStrength * 1000.f;
            boneDesc.format.newFmt.m_compressRotEps = 0.005f * boneCompressionStrength * 1000.f;
            animDesc.m_perBoneCompressionDesc.push_back(boneDesc);
        }
    }

    if (PrepareAnimationJob(&job, sourcePath.c_str(), outputPath.c_str(), sourceGameFolderPath.c_str(), animDesc, skeleton, controllerSkinningInfo, dbaPath, this))
    {
        ProcessAnimationJobWithInMemoryData(&job, content, controllerSkinningInfo);
    }

    const bool ok = (job.m_resSize != 0);
    return ok;
}

//
// Implemented by referencing to ProcessAnimationJob
//
static void ProcessAnimationJobWithInMemoryData(AnimationJob* job, CContentCGF* content, CInternalSkinningInfo* controllerSkinningInfo)
{
    const ConvertContext& context = *(const ConvertContext*)job->m_compiler->GetConvertContext();
    const bool bDebugCompression = job->m_debugCompression;
    const SAnimationDesc* animDesc = &job->m_animDesc;
    const string& sourcePath = job->m_sourcePath;
    const string& destPath = job->m_destinationPath;

    CAnimationCompressor compressor(controllerSkinningInfo);

    if (!compressor.LoadCAFFromMemory(sourcePath.c_str(), *animDesc, eCAFLoadUncompressedOnly, content, controllerSkinningInfo))
    {
        return;
    }
    compressor.EnableTrackAligning(job->m_alignTracks);
    compressor.m_GlobalAnimationHeader.SetFilePathCAF(job->m_animationPath);
    if (!compressor.ProcessAnimation(bDebugCompression))
    {
        RCLogError("Error in processing %s", sourcePath.c_str());
        return;
    }

    compressor.m_GlobalAnimationHeader.m_nCompressedControllerCount = compressor.m_GlobalAnimationHeader.CountCompressedControllers();

    // Create the directory (including intermediate ones), if not already there.
    if (!FileUtil::EnsureDirectoryExists(PathHelpers::GetDirectory(job->m_destinationPath).c_str()))
    {
        RCLogError("Failed to create directory for '%s'.", job->m_destinationPath.c_str());
        return;
    }

    
    
    
    FILETIME fileTime;
    AZ::u64 currentTime = AZStd::GetTimeNowSecond();
    FileUtil::UnixTime64BitToFiletime(currentTime, fileTime);
    
    job->m_resSize = compressor.SaveOnlyCompressedChunksInFile(destPath.c_str(), fileTime, 0, false, job->m_bigEndianOutput);

    context.pRC->AddInputOutputFilePair(sourcePath, destPath);
}

void CAnimationCompiler::Release()
{
    delete this;
}

//////////////////////////////////////////////////////////////////////////
// the paths must have trailing slash
static bool ScanDirectoryRecursive(
    const string& root, const string& path, const string& file, std::vector<string>& files, bool recursive,
    const std::vector<string>& dirsToIgnore, std::vector<uint32>& sizes)
{
    
    const size_t numDirsToIgnore = dirsToIgnore.size();
    bool anyFound = false;
    
    
    AZ::IO::LocalFileIO localFileIO;
    localFileIO.FindFiles(root.c_str(), file.c_str(), [&](const char* filePath) -> bool
    {
        const string foundFilename(filePath);
        if (StringHelpers::MatchesWildcardsIgnoreCase(foundFilename, file))
        {                        
            string filename = PathHelpers::Join(path, PathHelpers::GetFilename(filePath));
            
            bool skip = false;
            for (size_t i = 0; i < numDirsToIgnore; ++i)
            {
                if (StringHelpers::StartsWithIgnoreCase(foundFilename, dirsToIgnore[i]))
                {
                    skip = true;
                    break;
                }
            }
                    
            if (!skip)
            {
                anyFound = true;
                files.push_back(filename);
                        
                uint64 fileSize;
                localFileIO.Size(foundFilename, fileSize);
                sizes.push_back(static_cast<unsigned int>(fileSize));
            }
        }
        return true; // Keep iterating
    });
    
    if (recursive)
    {
        std::vector<string> subDirsToScan;
        // Find directories.
        localFileIO.FindFiles(root.c_str(), "*.*", [&](const char* filePath) -> bool
        {
            
            bool isDir = localFileIO.IsDirectory(filePath);
            if (isDir && strcmp(filePath, ".") && strcmp(filePath, ".."))
            {
                anyFound |= ScanDirectoryRecursive(filePath, PathHelpers::Join(path, PathHelpers::GetFilename(filePath)), file, files, recursive, dirsToIgnore, sizes);                
            }
            
            return true; // Keep iterating
        });
        
    }
    
    return anyFound;
}

