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


#include "pch.h"
#include "Strings.h"
#include <ICryAnimation.h>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QMessageBox>
#include <QMimeData>
#include "AnimationList.h"
#include "Serialization.h"
#include "EntryListImpl.h"
#include "IEditorFileMonitor.h"
#include "IEditor.h"
#include "Util/PathUtil.h"
#include "SkeletonList.h"
#include "Expected.h"
#include "Serialization/BinArchive.h"
#include "Explorer.h"
#include "../Cry3DEngine/CGF/CGFLoader.h"
#include "IAnimationCompressionManager.h"
#include <ICryPak.h>
#include "AnimEventFootstepGenerator.h"
#include "../EditorCommon/QPropertyTree/QPropertyDialog.h"
#include "../EditorCommon/QPropertyTree/ContextList.h"
#include "../EditorCommon/ListSelectionDialog.h"
#include "IResourceSelectorHost.h"
#include "CharacterDocument.h"
#include "CharacterToolSystem.h"
#include "IBackgroundTaskManager.h"
#include "CharacterToolSystem.h"
#include "AnimationCompressionManager.h"
#include <ActionOutput.h>

#include <Util/PathUtil.h> // for getting the game folder
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>

namespace CharacterTool {
    struct UpdateAnimationSizesTask
        : IBackgroundTask
    {
        System* m_system;
        vector<ExplorerEntryId> m_entries;
        vector<string> m_animationPaths;
        int m_column;
        vector<unsigned int> m_newSizes;

        void Delete() override
        {
            delete this;
        }

        ETaskResult Work() override
        {
            m_newSizes.resize(m_animationPaths.size());
            for (size_t i = 0; i < m_animationPaths.size(); ++i)
            {
                const char* animationPath = m_animationPaths[i].c_str();
                if (!EXPECTED(animationPath[0] != '\0'))
                {
                    m_newSizes.clear();
                    return eTaskResult_Failed;
                }

                AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(animationPath, "rb");
                if (fileHandle != AZ::IO::InvalidHandle)
                {
                    m_newSizes[i] = gEnv->pCryPak->FGetSize(fileHandle);
                    gEnv->pCryPak->FClose(fileHandle);
                }
                else
                {
                    m_newSizes[i] = 0;
                }
            }

            return eTaskResult_Completed;
        }

        UpdateAnimationSizesTask(const vector<ExplorerEntryId>& entries, const vector<string>& animationPaths, int column, System* system)
            : m_system(system)
            , m_entries(entries)
            , m_column(column)
            , m_animationPaths(animationPaths)
        {
        }

        UpdateAnimationSizesTask(const ExplorerEntryId& entryId, const string& animationPath, int column, System* system)
            : m_system(system)
            , m_entries(1, entryId)
            , m_column(column)
            , m_animationPaths(1, animationPath)
        {
        }

        void Finalize() override
        {
            if (!m_newSizes.empty())
            {
                if (m_system->explorer.get())
                {
                    m_system->explorer->BeginBatchChange(SUBTREE_ANIMATIONS);
                    size_t num = min(m_entries.size(), m_newSizes.size());
                    for (size_t i = 0; i < m_entries.size(); ++i)
                    {
                        m_system->explorer->SetEntryColumn(m_entries[i], m_column, m_newSizes[i], true);
                    }
                    m_system->explorer->EndBatchChange(SUBTREE_ANIMATIONS);
                }
            }
        }
    };

    // ---------------------------------------------------------------------------

    static vector<string> LoadJointNames(const char* skeletonPath)
    {
        CLoaderCGF cgfLoader;

        CChunkFile chunkFile;
        AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
        std::unique_ptr<CContentCGF> cgf(cgfLoader.LoadCGF(skeletonPath, chunkFile, 0));
        AZ_POP_DISABLE_WARNING
        if (!cgf.get())
        {
            return vector<string>();
        }

        if (const CSkinningInfo* skinningInfo = cgf->GetSkinningInfo())
        {
            vector<string> result;
            result.reserve(skinningInfo->m_arrBonesDesc.size());
            for (size_t i = 0; i < skinningInfo->m_arrBonesDesc.size(); ++i)
            {
                result.push_back(skinningInfo->m_arrBonesDesc[i].m_arrBoneName);
            }
            return result;
        }

        return vector<string>();
    }


    static bool LoadAnimEvents(AZ::ActionOutput* output, AnimEvents* animEvents, const char* animEventsFilename, const char* animationPath)
    {
        animEvents->clear();
        AZStd::string resolvedAnimEventsFullPath = Path::GamePathToFullPath(animEventsFilename).toUtf8().data();
        XmlNodeRef root = GetIEditor()->GetSystem()->LoadXmlFromFile(resolvedAnimEventsFullPath.c_str());

        if (!root)
        {
            if (output)
            {
                output->AddError("Failed to load animevents database", animEventsFilename);
            }
            return false;
        }

        XmlNodeRef animationNode;
        {
            int nodeCount = root->getChildCount();
            for (int i = 0; i < nodeCount; ++i)
            {
                XmlNodeRef node = root->getChild(i);
                if (!node->isTag("animation"))
                {
                    continue;
                }

                if (azstricmp(node->getAttr("name"), animationPath) == 0)
                {
                    animationNode = node;
                    break;
                }
            }
        }

        if (animationNode)
        {
            for (int i = 0; i < animationNode->getChildCount(); ++i)
            {
                XmlNodeRef eventNode = animationNode->getChild(i);

                AnimEvent ev;
                ev.LoadFromXMLNode(eventNode);
                animEvents->push_back(ev);
            }
        }

        return true;
    }

    static bool LoadAnimationEntry(SEntry<AnimationContent>* entry, SkeletonList* skeletonList, IAnimationSet* animationSet, const string& defaultSkeleton, const char* animEventsFilename)
    {
        string cafPath = entry->path;
        cafPath.MakeLower();

        entry->content.events.clear();

        bool result = true;
        if (animEventsFilename[0] != '\0' && !LoadAnimEvents(0, &entry->content.events, animEventsFilename, cafPath.c_str()))
        {
            result = false;
        }

        AZStd::string entryFullPath = Path::GamePathToFullPath(entry->path.c_str()).toUtf8().data();
        if (entry->content.type == AnimationContent::BLEND_SPACE)
        {
            string errorMessage;
            XmlNodeRef node = gEnv->pSystem->LoadXmlFromFile(entryFullPath.c_str());
            if (node)
            {
                BlendSpace& bspace = entry->content.blendSpace;
                bspace = BlendSpace();
                result = bspace.LoadFromXml(errorMessage, node, animationSet);
                if (!bspace.HasVGridData())
                {
                    // VGrid data does not exist in the .bspace xml.  Any saves
                    // to the blendspace will write the vgrid, but saves are
                    // disabled unless the entry has been modified.  Mark this
                    // entry as modified to enable saving to write the vgrid
                    // data.
                    entry->modified = true;
                }
            }
            else
            {
                result = false;
            }
        }
        else if (entry->content.type == AnimationContent::COMBINED_BLEND_SPACE)
        {
            string errorMessage;
            XmlNodeRef node = gEnv->pSystem->LoadXmlFromFile(entryFullPath.c_str());
            if (node)
            {
                CombinedBlendSpace& cbspace = entry->content.combinedBlendSpace;
                cbspace = CombinedBlendSpace();
                result = cbspace.LoadFromXml(errorMessage, node, animationSet);
            }
            else
            {
                result = false;
            }
        }
        else
        {
            AZStd::string fullFilePath = Path::GamePathToFullPath(entry->path.c_str()).toUtf8().data();
            fullFilePath = SAnimSettings::GetAnimSettingsFilename(fullFilePath.c_str());
            if (!gEnv->pCryPak->IsFileExist(fullFilePath.c_str()))
            {
                entry->content.importState = AnimationContent::COMPILED_BUT_NO_ANIMSETTINGS;
            }
            else
            {
                if (entry->content.settings.Load(fullFilePath.c_str(), vector<string>(), 0, 0))
                {
                    if (entry->content.importState == AnimationContent::COMPILED_BUT_NO_ANIMSETTINGS)
                    {
                        entry->content.importState = AnimationContent::IMPORTED;
                    }
                }
                else
                {
                    entry->content.settings = SAnimSettings();
                    entry->content.settings.build.skeletonAlias = defaultSkeleton;
                    entry->content.newAnimationSkeleton = defaultSkeleton;
                    result = false;
                }
            }

            if (entry->content.settings.build.compression.m_usesNameContainsInPerBoneSettings)
            {
                string skeletonPath = skeletonList->FindSkeletonPathByName(entry->content.settings.build.skeletonAlias.c_str());
                if (!skeletonPath.empty())
                {
                    vector<string> jointNames = LoadJointNames(skeletonPath.c_str());
                    if (!jointNames.empty())
                    {
                        if (!entry->content.settings.Load(fullFilePath.c_str(), jointNames, 0, 0))
                        {
                            result = false;
                        }
                    }
                }
            }
        }

        return result;
    }

    string GetRelativePath(const string& fullPath, bool bRelativeToGameFolder);

    // ---------------------------------------------------------------------------

    AnimationList::AnimationList(System* system, int explorerColumnFrames, int explorerColumnSize, int explorerColumnAudio, int explorerColumnPak)
        : m_system(system)
        , m_animationSet(0)
        , m_importEntriesInitialized(false)
        , m_explorerColumnFrames(explorerColumnFrames)
        , m_explorerColumnSize(explorerColumnSize)
        , m_explorerColumnAudio(explorerColumnAudio)
        , m_explorerColumnPak(explorerColumnPak)
        , m_character()
        , m_isSaveRunnerInMultiSaveMode(false)
    {
        GetIEditor()->GetFileMonitor()->RegisterListener(this, "", "caf");
        GetIEditor()->GetFileMonitor()->RegisterListener(this, "", "i_caf");
        GetIEditor()->GetFileMonitor()->RegisterListener(this, "", "animsettings");
        GetIEditor()->GetFileMonitor()->RegisterListener(this, "", "bspace");
        GetIEditor()->GetFileMonitor()->RegisterListener(this, "", "comb");
    }

    AnimationList::~AnimationList()
    {
        if (GetIEditor() && GetIEditor()->GetFileMonitor())
        {
            GetIEditor()->GetFileMonitor()->UnregisterListener(this);
        }
    }

    static bool IsInternalAnimationName(const char* name, const char* animationPath)
    {
        // if (name[0] == '_')
        //  return true;
        if (strncmp(name, "InternalPara", 12) == 0)
        {
            return true;
        }
        const char* editorPrefix = "animations/editor/";
        if (animationPath && _strnicmp(animationPath, editorPrefix, strlen(editorPrefix)) == 0)
        {
            return true;
        }
        return false;
    }

    static bool AnimationHasAudioEvents(const char* animationPath)
    {
        IAnimEvents* animEvents = gEnv->pCharacterManager->GetIAnimEvents();
        if (IAnimEventList* animEventList = animEvents->GetAnimEventList(animationPath))
        {
            int count = animEventList->GetCount();
            for (int i = 0; i < count; ++i)
            {
                CAnimEventData& animEventData = animEventList->GetByIndex(i);
                if (IsAudioEventType(animEventData.GetName()))
                {
                    return true;
                }
            }
        }
        return false;
    }

    static int GetAnimationPakState(const char* animationPath)
    {
        int result = 0;
        const char* ext = PathUtil::GetExt(animationPath);
        if (azstricmp(ext, "caf") == 0)
        {
            if (gEnv->pCryPak->IsFileExist(animationPath, ICryPak::eFileLocation_OnDisk))
            {
                result |= PAK_STATE_LOOSE_FILES;
            }
            else if (gEnv->pCryPak->IsFileExist(animationPath, ICryPak::eFileLocation_InPak))
            {
                result |= PAK_STATE_PAK;
            }

            AZStd::string sourceFullFilePath = Path::GamePathToFullPath(animationPath).toUtf8().data();

            //there is no concept of i_caf being in a pak file (paks only contain game ready assets.)
            //don't need to check for animsettings because it will match the icaf or will not exist
            if (!sourceFullFilePath.empty() && gEnv->pCryPak->IsFileExist(sourceFullFilePath.c_str(), ICryPak::eFileLocation_OnDisk))
            {
                result |= PAK_STATE_LOOSE_FILES;
                sourceFullFilePath = SAnimSettings::GetAnimSettingsFilename(sourceFullFilePath.c_str());
            }
        }
        else
        {
            // bspace/comb etc.
            if (gEnv->pCryPak->IsFileExist(animationPath, ICryPak::eFileLocation_OnDisk))
            {
                result |= PAK_STATE_LOOSE_FILES;
            }
            else if (gEnv->pCryPak->IsFileExist(animationPath, ICryPak::eFileLocation_InPak))
            {
                result |= PAK_STATE_PAK;
            }
        }
        return result;
    }

    void AnimationList::Populate(ICharacterInstance* character, const char* defaultSkeletonAlias, const AnimationSetFilter& filter, const char* animEventsFilename)
    {
        IAnimationSet* animationSet = nullptr;
        if (character)
        {
            animationSet = character->GetIAnimationSet();
            if (animationSet)
            {
                animationSet->RegisterListener(this);
            }
        }

        m_animationSet = animationSet;
        m_animEventsFilename = animEventsFilename;
        m_filter = filter;
        m_defaultSkeletonAlias = defaultSkeletonAlias;
        m_character = character;

        ReloadAnimationList();
    }

    void AnimationList::SetAnimationFilterAndScan(const AnimationSetFilter& filter)
    {
        m_filter = filter;
        ReloadAnimationList();
    }

    void AnimationList::SetAnimEventsFile(const string& fileName)
    {
        m_animEventsFilename = fileName;
    }

    void AnimationList::ReloadAnimationList()
    {
        m_animations.Clear();
        m_aliasToId.clear();

        if (m_character == nullptr)
        {
            SignalBeginBatchChange(SUBTREE_ANIMATIONS);
            SignalSubtreeReset(SUBTREE_ANIMATIONS);
            SignalEndBatchChange(SUBTREE_ANIMATIONS);
            return;
        }

        IAnimEvents* animEvents = gEnv->pCharacterManager->GetIAnimEvents();

        std::vector<std::pair<ExplorerEntryId, unsigned int> > audioColumnValues;
        std::vector<std::pair<ExplorerEntryId, int> > pakColumnValues;
        std::vector<std::pair<ExplorerEntryId, int> > framesColumnValues;

        IDefaultSkeleton& skeleton = m_character->GetIDefaultSkeleton();

        std::vector<string> animationPaths;
        std::vector<ExplorerEntryId> entries;

        int numAnimations = m_animationSet ? m_animationSet->GetAnimationCount() : 0;
        for (int i = 0; i < numAnimations; ++i)
        {
            const char* name = m_animationSet->GetNameByAnimID(i);
            const char* animationPath = m_animationSet->GetFilePathByID(i);
            if (IsInternalAnimationName(name, animationPath))
            {
                continue;
            }
            if (!m_filter.Matches(animationPath))
            {
                continue;
            }

            AnimationContent::Type type = AnimationContent::ANIMATION;
            int flags = m_animationSet->GetAnimationFlags(i);
            if (flags & CA_ASSET_LMG)
            {
                if (m_animationSet->IsCombinedBlendSpace(i))
                {
                    type = AnimationContent::COMBINED_BLEND_SPACE;
                }
                else
                {
                    type = AnimationContent::BLEND_SPACE;
                }
            }
            else if (flags & CA_ASSET_TCB)
            {
                type = AnimationContent::ANM;
            }
            else if (flags & CA_AIMPOSE)
            {
                if (m_animationSet->IsAimPose(i, skeleton))
                {
                    type = AnimationContent::AIMPOSE;
                }
                else if (m_animationSet->IsLookPose(i, skeleton))
                {
                    type = AnimationContent::LOOKPOSE;
                }
            }

            SEntry<AnimationContent>* entry = m_animations.AddEntry(0, animationPath, name, false);
            entry->content.type = type;
            entry->content.loadedInEngine = true;
            entry->content.loadedAsAdditive = (flags & CA_ASSET_ADDITIVE) != 0;
            entry->content.importState = AnimationContent::IMPORTED;
            entry->content.animationId = i;
            entry->content.system = m_system;
            if (entry->content.settings.build.skeletonAlias.empty())
            {
                entry->content.settings.build.skeletonAlias = m_defaultSkeletonAlias;
            }

            ExplorerEntryId entryId(SUBTREE_ANIMATIONS, entry->id);
            animationPaths.push_back(animationPath);
            entries.push_back(entryId);

            m_animations.EntryReverted(0, entry);
            m_aliasToId[entry->name] = entry->id;

            bool gotAudioEvents = AnimationHasAudioEvents(animationPath);
            audioColumnValues.push_back(std::make_pair(entryId, gotAudioEvents ? ENTRY_AUDIO_PRESENT : ENTRY_AUDIO_NONE));

            int pakState = GetAnimationPakState(animationPath);
            pakColumnValues.push_back(std::make_pair(entryId, pakState));

            float durationSeconds = m_animationSet->GetDuration_sec(i);
            const uint32 animationSamplingFrequencyHz = 30;
            int frameCount = 1 + uint32(durationSeconds * animationSamplingFrequencyHz + 0.5f);
            framesColumnValues.push_back(std::make_pair(entryId, frameCount));

            if (type == AnimationContent::BLEND_SPACE)
            {
                // Pre-load blend spaces, so we know if they have VGrids or not
                // This allows the UI to mark them as modified, because a save
                // to a blendspace file that doesn't have VGrid data will write
                // the VGrid. Note that if the .bspace file on disk doesn't
                // have a VGrid node, and the user never clicks on that bspace
                // to view it in Geppetteo, the vgrid data is never calculated.
                LoadOrGetChangedEntry(entry->id);
            }
        }

        UpdateAnimationSizesTask* sizesTask = new UpdateAnimationSizesTask(entries, animationPaths, m_explorerColumnSize, m_system);
        GetIEditor()->GetBackgroundTaskManager()->AddTask(sizesTask, eTaskPriority_BackgroundScan, eTaskThreadMask_IO);

        SignalBeginBatchChange(SUBTREE_ANIMATIONS);

        ScanForImportEntries(&pakColumnValues, true);
        SignalSubtreeReset(SUBTREE_ANIMATIONS);

        for (size_t i = 0; i < framesColumnValues.size(); ++i)
        {
            m_system->explorer->SetEntryColumn(framesColumnValues[i].first, m_explorerColumnFrames, framesColumnValues[i].second, true);
        }
        for (size_t i = 0; i < audioColumnValues.size(); ++i)
        {
            m_system->explorer->SetEntryColumn(audioColumnValues[i].first, m_explorerColumnAudio, audioColumnValues[i].second, true);
        }
        for (size_t i = 0; i < pakColumnValues.size(); ++i)
        {
            m_system->explorer->SetEntryColumn(pakColumnValues[i].first, m_explorerColumnPak, pakColumnValues[i].second, true);
        }

        SignalEndBatchChange(SUBTREE_ANIMATIONS);
    }

    void AnimationList::ScanForImportEntries(std::vector<std::pair<ExplorerEntryId, int> >* pakColumnValues, bool resetFollows)
    {
        typedef std::set<string, stl::less_stricmp<string> > UnusedAnimations;
        UnusedAnimations unusedAnimations;
        vector<unsigned int> idsToRemove;
        for (size_t i = 0; i < m_animations.Count(); ++i)
        {
            SEntry<AnimationContent>* entry = m_animations.GetByIndex(i);
            if (entry->content.importState == AnimationContent::NEW_ANIMATION)
            {
                unusedAnimations.insert(PathUtil::ReplaceExtension(entry->path.c_str(), ".caf"));
            }
        }

        if (!m_importEntriesInitialized)
        {
            m_importEntries.clear();
            std::vector< string > candidateAnimationsForImport;
            const char* const filePattern = "*.i_caf";

            //  Store the game data folder for later.
            const AZStd::string editFolder = Path::GetEditingGameDataFolder();
            SDirectoryEnumeratorHelper dirHelper;
            dirHelper.ScanDirectoryRecursive(editFolder.c_str(), "", filePattern, candidateAnimationsForImport);

            //collect a list of .i_caf files that have no .animsettings file
            for (size_t i = 0; i < candidateAnimationsForImport.size(); ++i)
            {
                //  Removed the call to Path::GamePathToFullPath. It has been redirected to the Asset Procesor and that
                //  is a 150ms+ round trip. Since we are looking in the source assets folder, we should continue to look in the source folder.
                //  We already got that folder to run the recursive search, above. Unless a new version is doing more than swapping
                //  extensions and checking if they exist, try to keep this version of the loop.
                const string animationFile = PathUtil::Make(editFolder.c_str(), candidateAnimationsForImport[i]);

                const string fullFilePath = PathUtil::ToNativePath(PathUtil::ReplaceExtension(animationFile.c_str(), ".animsettings"));
                if (gEnv->pCryPak->IsFileExist(fullFilePath.c_str(), ICryPak::eFileLocation_OnDisk))
                {
                    continue;
                }
                m_importEntries.push_back(animationFile);
            }
            m_importEntriesInitialized = true;
        }

        for (size_t i = 0; i < m_importEntries.size(); ++i)
        {
            const string& animationFile = m_importEntries[i];

            string animSettingsPath = PathUtil::ReplaceExtension(animationFile.c_str(), ".animsettings");
            AZStd::string fullFilePath = Path::GamePathToFullPath(animSettingsPath.c_str()).toUtf8().data();
            if (gEnv->pCryPak->IsFileExist(fullFilePath.c_str(), ICryPak::eFileLocation_OnDisk))
            {
                continue;
            }

            string animationPath = PathUtil::ReplaceExtension(animationFile.c_str(), ".caf");
            if (m_filter.Matches(animationPath.c_str()))
            {
                unusedAnimations.erase(animationPath);
                bool isAdded = false;
                SEntry<AnimationContent>* entry = m_animations.AddEntry(&isAdded, animationPath.c_str(), 0, false);
                if (!resetFollows)
                {
                    if (isAdded)
                    {
                        SignalEntryAdded(SUBTREE_ANIMATIONS, entry->id);
                    }
                    else
                    {
                        EntryModifiedEvent ev;
                        ev.subtree = SUBTREE_ANIMATIONS;
                        ev.id = entry->id;
                        SignalEntryModified(ev);
                    }
                }
                UpdateImportEntry(entry);
                m_animations.EntryReverted(0, entry);

                int pakState = GetAnimationPakState(animationPath.c_str());
                if (pakColumnValues)
                {
                    pakColumnValues->push_back(std::make_pair(ExplorerEntryId(SUBTREE_ANIMATIONS, entry->id), pakState));
                }
                else
                {
                    m_system->explorer->SetEntryColumn(ExplorerEntryId(SUBTREE_ANIMATIONS, entry->id), m_explorerColumnPak, pakState, true);
                }
            }
        }

        for (UnusedAnimations::iterator it = unusedAnimations.begin(); it != unusedAnimations.end(); ++it)
        {
            if (unsigned int id = m_animations.RemoveByPath(it->c_str()))
            {
                SignalEntryRemoved(SUBTREE_ANIMATIONS, id);
            }
        }
    }

    string GetPath(const char* path);
    const char* GetFilename(const char* path);

    bool AnimationList::LoadOrGetChangedEntry(unsigned int id)
    {
        if (SEntry<AnimationContent>* entry = m_animations.GetById(id))
        {
            if (entry->content.importState == AnimationContent::NEW_ANIMATION)
            {
                if (UpdateImportEntry(entry))
                {
                    EntryModifiedEvent ev;
                    ev.subtree = SUBTREE_ANIMATIONS;
                    ev.id = entry->id;
                    SignalEntryModified(ev);
                }
            }
            else if (!entry->loaded)
            {
                entry->dataLostDuringLoading = !LoadAnimationEntry(entry, m_system->compressionSkeletonList, m_animationSet, m_defaultSkeletonAlias, m_animEventsFilename.c_str());
                entry->StoreSavedContent();
                entry->lastContent = entry->savedContent;
            }
            entry->loaded = true;
        }
        return true;
    }
    bool AnimationList::IsRunningAsyncSaveOperation()
    {
        return m_saveRunner != nullptr;
    }

    SEntry<AnimationContent>* AnimationList::GetEntry(unsigned int id) const
    {
        return m_animations.GetById(id);
    }

    bool AnimationList::IsNewAnimation(unsigned int id) const
    {
        if (SEntry<AnimationContent>* entry = m_animations.GetById(id))
        {
            return entry->content.importState == AnimationContent::NEW_ANIMATION;
        }
        return false;
    }


    SEntry<AnimationContent>* AnimationList::FindEntryByPath(const char* animationPath)
    {
        return m_animations.GetByPath(animationPath);
    }

    unsigned int AnimationList::FindIdByAlias(const char* animationName)
    {
        AliasToId::iterator it = m_aliasToId.find(animationName);
        if (it == m_aliasToId.end())
        {
            return 0;
        }
        return it->second;
    }

    bool AnimationList::ResaveAnimSettings(const char* filePath)
    {
        SEntry<AnimationContent> fakeEntry;
        fakeEntry.path = PathUtil::ReplaceExtension(filePath, "caf");
        fakeEntry.name = PathUtil::GetFileName(fakeEntry.path.c_str());

        if (!LoadAnimationEntry(&fakeEntry, m_system->compressionSkeletonList, m_animationSet, m_defaultSkeletonAlias, m_animEventsFilename.c_str()))
        {
            return false;
        }

        {
            char buffer[ICryPak::g_nMaxPath] = "";
            const char* realPath = gEnv->pCryPak->AdjustFileName(filePath, buffer, AZ_ARRAY_SIZE(buffer), 0);
            QFile::setPermissions(realPath, QFile::permissions(realPath) | QFile::WriteOther);
        }

        if (!fakeEntry.content.settings.SaveUsingAssetPath(filePath))
        {
            return false;
        }

        return true;
    }

    void AnimationList::CheckIfModified(unsigned int id, const char* reason, bool continuousChange)
    {
        if (SEntry<AnimationContent>* entry = m_animations.GetById(id))
        {
            EntryModifiedEvent ev;
            ev.continuousChange = continuousChange;
            if (continuousChange || m_animations.EntryChanged(&ev.previousContent, entry))
            {
                ev.subtree = SUBTREE_ANIMATIONS;
                ev.id = entry->id;
                if (reason)
                {
                    ev.reason = reason;
                    ev.contentChanged = true;
                }
                if (!continuousChange)
                {
                    bool gotAudioEvents = entry->content.HasAudioEvents();
                    m_system->explorer->SetEntryColumn(ExplorerEntryId(SUBTREE_ANIMATIONS, id), m_explorerColumnAudio,
                        gotAudioEvents ? ENTRY_AUDIO_PRESENT : ENTRY_AUDIO_NONE, false);
                }
                SignalEntryModified(ev);
            }
        }
    }

    void AnimationList::OnAnimationSetAddAnimation(const char* animationPath, const char* name)
    {
        if (IsInternalAnimationName(name, animationPath))
        {
            return;
        }
        AnimationContent::Type type = AnimationContent::ANIMATION;
        int id = m_animationSet->GetAnimIDByName(name);
        if (id < 0)
        {
            return;
        }
        int flags = m_animationSet->GetAnimationFlags(id);
        if (flags & CA_ASSET_LMG)
        {
            if (m_animationSet->IsCombinedBlendSpace(id))
            {
                type = AnimationContent::COMBINED_BLEND_SPACE;
            }
            else
            {
                type = AnimationContent::BLEND_SPACE;
            }
        }

        bool newEntry;
        bool modified = false;
        SEntry<AnimationContent>* entry = m_animations.AddEntry(&newEntry, animationPath, name, false);
        entry->content.loadedInEngine = true;
        if (entry->content.importState == AnimationContent::WAITING_FOR_CHRPARAMS_RELOAD)
        {
            modified = true;
        }
        entry->content.importState = AnimationContent::IMPORTED;

        m_aliasToId[name] = entry->id;

        UpdateAnimationSizesTask* task = new UpdateAnimationSizesTask(ExplorerEntryId(SUBTREE_ANIMATIONS, id), animationPath, m_explorerColumnSize, m_system);
        GetIEditor()->GetBackgroundTaskManager()->AddTask(task, eTaskPriority_BackgroundScan, eTaskThreadMask_IO);

        EntryModifiedEvent ev;
        if (m_animations.EntryReverted(&ev.previousContent, entry))
        {
            modified = true;
        }
        if (newEntry)
        {
            SignalEntryAdded(SUBTREE_ANIMATIONS, entry->id);
        }
        else if (modified)
        {
            ev.subtree = SUBTREE_ANIMATIONS;
            ev.id = entry->id;
            ev.reason = "Reload";
            ev.contentChanged = true;
            SignalEntryModified(ev);
        }
    }

    void AnimationList::OnAnimationSetReload()
    {
        ReloadAnimationList();
    }

    AZStd::string AnimationList::CreateAnimEventsPathFromFilter()
    {
        string path = PathUtil::Make(m_filter.folders[0].path.c_str(), m_defaultSkeletonAlias.c_str(), ".animevents");
        return AZStd::string(path.c_str());
    }

    bool AnimationList::AddBSpaceSaveOperation(AZStd::shared_ptr<AZ::SaveOperationController> saveEntryController, SEntry<AnimationContent>* entry, const AZStd::shared_ptr<AZ::ActionOutput>& output, AZ::SaveCompleteCallback onSaveComplete) const
    {
        AZStd::string entryFullPath = Path::GamePathToFullPath(entry->path.c_str()).toUtf8().data();
        XmlNodeRef root = entry->content.blendSpace.SaveToXml();
        char path[ICryPak::g_nMaxPath] = "";
        gEnv->pCryPak->AdjustFileName(entryFullPath.c_str(), path, AZ_ARRAY_SIZE(path), 0);

        SEntry<AnimationContent>* animation = GetEntry(entry->id);
        if (!animation)
        {
            if (output)
            {
                output->AddError("Missing character entry.", entry->path.c_str());
            }
            if (onSaveComplete)
            {
                onSaveComplete(false);
            }
            return false;
        }

        ICharacterInstance* character = m_system->document->CompressedCharacter();
        if (!character || !character->GetISkeletonAnim())
        {
            if (output)
            {
                output->AddError("A character should be loaded for VEG computation to work.", nullptr);
            }
            if (onSaveComplete)
            {
                onSaveComplete(false);
            }
            return false;
        }
        ISkeletonAnim* skeleton = character->GetISkeletonAnim();

        saveEntryController->AddSaveOperation(path,
            [root, skeleton, entry](const AZStd::string& outputPath, const AZStd::shared_ptr<AZ::ActionOutput>& actionOutput) -> bool
            {
                if (!root->saveToFile(outputPath.c_str()))
                {
                    actionOutput->AddError("Failed to save file.", outputPath.c_str());
                    return false;
                }

                if (!skeleton->ExportVGrid(entry->name.c_str(), outputPath.c_str()))
                {
                    actionOutput->AddError("Failed to save VGrid data to file.", outputPath.c_str());
                    return false;
                }
                return true;
            }
            );
        return true;
    }

    bool SaveEmptyAnimEventsXML(const AZStd::shared_ptr<AZ::ActionOutput>& output, AZStd::string& resolvedAnimEventsFullPath)
    {
        XmlNodeRef root = GetIEditor()->GetSystem()->CreateXmlNode("anim_event_list");

        char realPath[ICryPak::g_nMaxPath];
        gEnv->pCryPak->AdjustFileName(resolvedAnimEventsFullPath.c_str(), realPath, AZ_ARRAY_SIZE(realPath), ICryPak::FLAGS_FOR_WRITING);
        {
            string path;
            string filename;
            PathUtil::Split(realPath, path, filename);
            QDir().mkpath(QString::fromLocal8Bit(path.c_str()));
        }

        if (!root->saveToFile(realPath))
        {
            if (output)
            {
                output->AddError("Failed to save animevents file", realPath);
            }
            return false;
        }
        return true;
    }

    bool PatchAnimEvents(const AZStd::shared_ptr<AZ::ActionOutput>& output, const char* animEventsFilename, const char* animationPath, const AnimEvents& events)
    {
        AZStd::string resolvedAnimEventsFullPath = Path::GamePathToFullPath(animEventsFilename).toUtf8().data();
        XmlNodeRef root = GetIEditor()->GetSystem()->LoadXmlFromFile(resolvedAnimEventsFullPath.c_str());

        if (!root)
        {
            if (output)
            {
                output->AddError("Failed to load existing animevents database", animEventsFilename);
            }
            return false;
        }

        XmlNodeRef animationNode;

        int nodeCount = root->getChildCount();
        for (int i = 0; i < nodeCount; ++i)
        {
            XmlNodeRef node = root->getChild(i);
            if (!node->isTag("animation"))
            {
                continue;
            }

            if (azstricmp(node->getAttr("name"), animationPath) == 0)
            {
                animationNode = node;
                break;
            }
        }

        if (!animationNode)
        {
            animationNode = root->newChild("animation");
            animationNode->setAttr("name", animationPath);
        }

        AnimEvents existingEvents;
        {
            for (int i = 0; i < animationNode->getChildCount(); ++i)
            {
                XmlNodeRef eventNode = animationNode->getChild(i);

                AnimEvent ev;
                ev.LoadFromXMLNode(eventNode);
                existingEvents.push_back(ev);
            }
        }

        animationNode->removeAllChilds();

        AnimEvents sortedEvents = events;
        std::stable_sort(sortedEvents.begin(), sortedEvents.end());
        for (size_t i = 0; i < sortedEvents.size(); ++i)
        {
            XmlNodeRef eventNode = animationNode->newChild("event");
            CAnimEventData animEventData;
            sortedEvents[i].ToData(&animEventData);
            gEnv->pCharacterManager->GetIAnimEvents()->SaveAnimEventToXml(animEventData, eventNode);
        }

        bool needToSave = false;
        {
            Serialization::BinOArchive arExisting;
            arExisting(existingEvents);
            Serialization::BinOArchive arNew;
            arNew(sortedEvents);

            if (arExisting.length() != arNew.length())
            {
                needToSave = true;
            }
            else
            {
                needToSave = memcmp(arExisting.buffer(), arNew.buffer(), arNew.length()) != 0;
            }
        }

        if (needToSave)
        {
            char realPath[ICryPak::g_nMaxPath];
            gEnv->pCryPak->AdjustFileName(resolvedAnimEventsFullPath.c_str(), realPath, AZ_ARRAY_SIZE(realPath), ICryPak::FLAGS_FOR_WRITING);
            {
                string path;
                string filename;
                PathUtil::Split(realPath, path, filename);
                QDir().mkpath(QString::fromLocal8Bit(path.c_str()));
            }

            if (!root->saveToFile(realPath))
            {
                if (output)
                {
                    output->AddError("Failed to save animevents file", realPath);
                }
                return false;
            }
        }
        return true;
    }

    static void CreateFolderForFile(const char* gameFilename)
    {
        QString file(Path::GamePathToFullPath(gameFilename));
        QDir().mkpath(QString::fromLocal8Bit(PathUtil::ToUnixPath(PathUtil::GetParentDirectory(file.toUtf8().data())).c_str()));
    }

    void AnimationList::SaveAnimationEntry(const AZStd::shared_ptr<AZ::ActionOutput>& output, unsigned int id, bool notifyOfChange, AZ::SaveCompleteCallback onSaveComplete)
    {
        SEntry<AnimationContent>* entry = m_animations.GetById(id);
        if (!entry)
        {
            if (output)
            {
                output->AddError("Passed in entry was null", nullptr);
            }
            if (onSaveComplete)
            {
                onSaveComplete(false);
            }

            return;
        }

        if (IsRunningAsyncSaveOperation() && !m_isSaveRunnerInMultiSaveMode)
        {
            if (output)
            {
                output->AddError("Geppetto - AnimationList::SaveAnimationEntry failed, asnyc save operation is already running", entry->name.c_str());
            }

            if (onSaveComplete)
            {
                onSaveComplete(false);
            }

            return;
        }

        if (!m_saveRunner)
        {
            m_saveRunner = AZStd::make_shared<AZ::AsyncSaveRunner>();
        }

        AZStd::string entryPath = entry->path.c_str();

        bool allSpecialSaveOperationsInitiated = true;
        AZStd::shared_ptr<AZ::SaveOperationController> saveEntryController = m_saveRunner->GenerateController();
        AZStd::string animSettingsFilename = SAnimSettings::GetAnimSettingsFilename(entry->path.c_str()).c_str();
        if (!animSettingsFilename.empty())
        {
            animSettingsFilename = Path::GamePathToFullPath(animSettingsFilename.c_str()).toUtf8().data();
        }

        if (entry->content.importState == AnimationContent::NEW_ANIMATION)
        {
            saveEntryController->AddSaveOperation(animSettingsFilename.c_str(),
                [entryPath, this](const AZStd::string& outputPath, const AZStd::shared_ptr<AZ::ActionOutput>& actionOutput) -> bool
                {
                    SAnimSettings settings;
                    SEntry<AnimationContent>* entry = m_animations.GetByPath(entryPath.c_str());
                    if (entry)
                    {
                        settings.build.skeletonAlias = entry->content.newAnimationSkeleton;
                        return settings.SaveUsingFullPath(outputPath.c_str());
                    }
                    return false;
                }
                );
        }
        else
        {
            std::stable_sort(entry->content.events.begin(), entry->content.events.end());

            if (!m_animEventsFilename.empty())
            {
                AZStd::string animEventsFilename = Path::GamePathToFullPath(m_animEventsFilename.c_str()).toUtf8().data();
                saveEntryController->AddSaveOperation(animEventsFilename,
                    [entryPath, this](const AZStd::string& outputPath, const AZStd::shared_ptr<AZ::ActionOutput>& actionOutput) -> bool
                    {
                        SEntry<AnimationContent>* entry = m_animations.GetByPath(entryPath.c_str());
                        if (entry)
                        {
                            return PatchAnimEvents(actionOutput, outputPath.c_str(), entry->path.c_str(), entry->content.events);
                        }
                        return false;
                    }
                    );

            }
            else if (entry->content.events.size() > 0)
            {
                //need to save out a new animevents file
                AZStd::string animEventsPath = CreateAnimEventsPathFromFilter();
                AZStd::string animEventsFullPath = Path::GamePathToFullPath(animEventsPath.c_str()).toUtf8().data();

                //determine if path file exists
                bool animEventsExists = gEnv->pCryPak->IsFileExist(animEventsFullPath.c_str(), ICryPak::eFileLocation_OnDisk);
                if (!animEventsExists)
                {
                    if (output)
                    {
                        if (SaveEmptyAnimEventsXML(output, animEventsFullPath))
                        {
                            animEventsExists = true;
                        }
                    }
                }
                else
                {
                    //animevents file exists but is not defined in the chrparams data
                    //this file could be under source control
                    //for now - fail
                    AZStd::string errorMessage = "Geppetto - Save failed";
                    AZStd::string details = AZStd::string::format("Automatic animation events file generation (%s) already exists and is not assigned in current .chrparams file.  Assigning the file in current character's .chrparams will allow animation to save event data.", animEventsPath.c_str());
                    output->AddError(errorMessage, details);

                    if (onSaveComplete)
                    {
                        onSaveComplete(false);
                    }
                    m_saveRunner = nullptr;
                    return;
                }

                if (animEventsExists)
                {
                    saveEntryController->AddSaveOperation(animEventsFullPath,
                        [entryPath, animEventsPath, this](const AZStd::string& outputPath, const AZStd::shared_ptr<AZ::ActionOutput>& actionOutput) -> bool
                        {
                            m_animEventsFilename = animEventsPath.c_str();
                            //assign the file in chrparams
                            if (!m_system->document->SetAnimEventsFile(m_animEventsFilename))
                            {
                                if (actionOutput)
                                {
                                    AZStd::string errorMessage = "Geppetto - Save failed";
                                    AZStd::string details = AZStd::string::format("unable to assign animation events file (%s) to current character's .chrparams", m_animEventsFilename.c_str());
                                    actionOutput->AddError(errorMessage, details);
                                }
                                return false;
                            }

                            //save the file now
                            SEntry<AnimationContent>* entry = m_animations.GetByPath(entryPath.c_str());
                            if (entry)
                            {
                                return PatchAnimEvents(actionOutput, outputPath.c_str(), entry->path.c_str(), entry->content.events);
                            }
                            return false;
                        }
                        );
                }
                else
                {
                    allSpecialSaveOperationsInitiated = false;
                }
            }

            if (entry->content.type == AnimationContent::BLEND_SPACE)
            {
                if (!AddBSpaceSaveOperation(saveEntryController, entry, output, onSaveComplete))
                {
                    return;
                }

            }
            else if (entry->content.type == AnimationContent::COMBINED_BLEND_SPACE)
            {
                AZStd::string entryFullPath = Path::GamePathToFullPath(entry->path.c_str()).toUtf8().data();
                XmlNodeRef root = entry->content.combinedBlendSpace.SaveToXml();
                char path[ICryPak::g_nMaxPath] = "";
                gEnv->pCryPak->AdjustFileName(entryFullPath.c_str(), path, AZ_ARRAY_SIZE(path), 0);

                saveEntryController->AddSaveOperation(path,
                    [root](const AZStd::string& outputPath, const AZStd::shared_ptr<AZ::ActionOutput>& actionOutput) -> bool
                    {
                        return root->saveToFile(outputPath.c_str());
                    }
                    );
            }
            else if (entry->content.type == AnimationContent::ANIMATION)
            {
                if (entry->content.importState != AnimationContent::COMPILED_BUT_NO_ANIMSETTINGS)
                {
                    saveEntryController->AddSaveOperation(animSettingsFilename,
                        [entryPath, this](const AZStd::string& outputPath, const AZStd::shared_ptr<AZ::ActionOutput>& actionOutput) -> bool
                        {
                            CreateFolderForFile(outputPath.c_str());
                            SEntry<AnimationContent>* entry = m_animations.GetByPath(entryPath.c_str());
                            if (entry)
                            {
                                return entry->content.settings.SaveUsingFullPath(outputPath.c_str());
                            }
                            return false;
                        }
                        );
                }
            }
        }

        saveEntryController->SetOnCompleteCallback(
            [notifyOfChange, entryPath, allSpecialSaveOperationsInitiated, this](bool success)
            {
                if (!success || !allSpecialSaveOperationsInitiated)
                {
                    return;
                }
                
                SEntry<AnimationContent>* entry = m_animations.GetByPath(entryPath.c_str());
                if (entry)
                {
                    m_animations.EntrySaved(entry);

                    if (notifyOfChange)
                    {
                        EntryModifiedEvent ev;
                        ev.subtree = SUBTREE_ANIMATIONS;
                        ev.id = entry->id;
                        SignalEntryModified(ev);
                    }

                    int pakState = GetAnimationPakState(entry->path.c_str());
                    m_system->explorer->SetEntryColumn(ExplorerEntryId(SUBTREE_ANIMATIONS, entry->id), m_explorerColumnPak, pakState, true);
                }
            }
            );

        // If we aren't running save all, then this is the end, so run all the save operations.
        //  Otherwise what we've been doing is basically building up the save runner to be run
        //  by the SaveAll function later.
        if (!m_isSaveRunnerInMultiSaveMode)
        {
            m_saveRunner->Run(output,
                [onSaveComplete, allSpecialSaveOperationsInitiated, this](bool success)
                {
                    if (onSaveComplete)
                    {
                        onSaveComplete(success && allSpecialSaveOperationsInitiated);
                    }

                    m_saveRunner = nullptr;
                }, AZ::AsyncSaveRunner::ControllerOrder::Random);
        }
    }

    void AnimationList::SaveEntry(const AZStd::shared_ptr<AZ::ActionOutput>& output, unsigned int id, AZ::SaveCompleteCallback onSaveComplete)
    {
        return SaveAnimationEntry(output, id, true, onSaveComplete);
    }

    string AnimationList::GetSaveFilename(unsigned int id)
    {
        SEntry<AnimationContent>* entry = GetEntry(id);
        if (!entry)
        {
            return string();
        }

        return PathUtil::ReplaceExtension(entry->path, "animsettings");
    }

    void AnimationList::SaveAll(const AZStd::shared_ptr<AZ::ActionOutput>& output, AZ::SaveCompleteCallback onSaveComplete)
    {
        m_isSaveRunnerInMultiSaveMode = true;
        for (size_t i = 0; i < m_animations.Count(); ++i)
        {
            SEntry<AnimationContent>* entry = m_animations.GetByIndex(i);
            if (entry->modified)
            {
                SaveEntry(output, entry->id, static_cast<AZ::SaveCompleteCallback>(0));
            }
        }
        m_isSaveRunnerInMultiSaveMode = false;

        // Save runner is generated by SaveEntry if there are actually entries to save. If not
        //  then this save all is a no-op and we'll call the callback with true for success.
        if (m_saveRunner)
        {
            m_saveRunner->Run(output,
                [onSaveComplete, this](bool success)
                {
                    if (onSaveComplete)
                    {
                        onSaveComplete(success);
                    }   

                    m_saveRunner = nullptr;
                }, AZ::AsyncSaveRunner::ControllerOrder::Random);
        }
        else if (onSaveComplete)
        {
            onSaveComplete(true);
        }
    }

    void AnimationList::RevertEntry(unsigned int id)
    {
        if (SEntry<AnimationContent>* entry = m_animations.GetById(id))
        {
            entry->dataLostDuringLoading = !LoadAnimationEntry(entry, m_system->compressionSkeletonList, m_animationSet, m_defaultSkeletonAlias, m_animEventsFilename.c_str());
            entry->StoreSavedContent();

            EntryModifiedEvent ev;
            if (m_animations.EntryReverted(&ev.previousContent, entry))
            {
                ev.subtree = SUBTREE_ANIMATIONS;
                ev.id = entry->id;
                ev.contentChanged = ev.previousContent != entry->lastContent;
                ;
                ev.reason = "Revert";
                SignalEntryModified(ev);
            }

            int pakState = GetAnimationPakState(entry->path.c_str());
            m_system->explorer->SetEntryColumn(ExplorerEntryId(SUBTREE_ANIMATIONS, id), m_explorerColumnPak, pakState, true);
        }
    }

    bool AnimationList::UpdateImportEntry(SEntry<AnimationContent>* entry)
    {
        entry->name = PathUtil::GetFileName(entry->path.c_str());

        if (entry->content.importState == AnimationContent::NOT_SET)
        {
            entry->content.importState = AnimationContent::NEW_ANIMATION;
            entry->content.loadedInEngine = false;
            entry->content.newAnimationSkeleton = m_defaultSkeletonAlias;

            int pakState = GetAnimationPakState(entry->path.c_str());
            m_system->explorer->SetEntryColumn(ExplorerEntryId(SUBTREE_ANIMATIONS, entry->id), m_explorerColumnPak, pakState, true);
            return true;
        }
        return false;
    }

    void AnimationList::UpdateAnimationEntryByPath(const char* filename)
    {
        SEntry<AnimationContent>* entry = FindEntryByPath(filename);
        if (entry == 0)
        {
            return;
        }

        UpdateAnimationSizesTask* task = new UpdateAnimationSizesTask(ExplorerEntryId(SUBTREE_ANIMATIONS, entry->id), entry->path, m_explorerColumnSize, m_system);
        GetIEditor()->GetBackgroundTaskManager()->AddTask(task, eTaskPriority_BackgroundScan, eTaskThreadMask_IO);
    }

    static string StripExtension(const char* animationName)
    {
        const char* dot = strrchr(animationName, '.');
        if (dot == 0)
        {
            return animationName;
        }
        return string(animationName, dot);
    }

    bool AnimationList::ImportAnimation(AZStd::shared_ptr<AZ::ActionOutput>& output, unsigned int id)
    {
        SEntry<AnimationContent>* importEntry = m_animations.GetById(id);
        if (!importEntry)
        {
            AZStd::string idString;
            AZStd::to_string(idString, id);
            output->AddError("Unable to import animation by id ", idString.c_str());
            return false;
        }

        importEntry->content.settings.build.skeletonAlias = importEntry->content.newAnimationSkeleton;
        
        SaveEntry(output, importEntry->id, static_cast<AZ::SaveCompleteCallback>(0));

        importEntry->content.importState = AnimationContent::WAITING_FOR_CHRPARAMS_RELOAD;
        importEntry->name = PathUtil::GetFileName(importEntry->path.c_str());

        UpdateAnimationSizesTask* task = new UpdateAnimationSizesTask(ExplorerEntryId(SUBTREE_ANIMATIONS, id), importEntry->path, m_explorerColumnSize, m_system);
        GetIEditor()->GetBackgroundTaskManager()->AddTask(task, eTaskPriority_BackgroundScan, eTaskThreadMask_IO);
        return true;
    }

    bool AnimationList::IsLoaded(unsigned int id) const
    {
        SEntry<AnimationContent>* entry = GetEntry(id);
        if (!entry)
        {
            return false;
        }
        return entry->loaded;
    }

    void AnimationList::OnFileChange(const char* filename, EChangeType eType)
    {
        string filenameString = filename;
        string slashPath = PathUtil::MakeGamePath(filenameString);
        const string originalExt = PathUtil::GetExt(slashPath.c_str());
        string animationPath;
        const char* pakStateFilename = "";

        if (originalExt == "i_caf" ||
            originalExt == "animsettings")
        {
            string intermediatePath = PathUtil::ReplaceExtension(GetRelativePath(slashPath.c_str(), true), "i_caf");
            string aliasedIntermediatePath = PathUtil::Make("@devassets@", intermediatePath);
            bool intermediateExists = gEnv->pCryPak->IsFileExist(aliasedIntermediatePath.c_str(), ICryPak::eFileLocation_OnDisk);

            animationPath = PathUtil::ReplaceExtension(intermediatePath, "caf");
            bool animationExists = gEnv->pCryPak->IsFileExist(animationPath.c_str());

            string animSettingsPath = PathUtil::ReplaceExtension(intermediatePath, "animsettings");
            string aliasedAnimSettingsPath = PathUtil::Make("@devassets@", animSettingsPath);
            bool animSettingsExists = gEnv->pCryPak->IsFileExist(aliasedAnimSettingsPath.c_str(), ICryPak::eFileLocation_OnDisk);

            if (animationExists && intermediateExists && animSettingsExists)
            {
                UpdateAnimationEntryByPath(filename);
            }

            if (!animationExists && intermediateExists && animSettingsExists)
            {
                //when the .caf is generated it will do what it needs.
                if (m_filter.Matches(animationPath.c_str()))
                {
                    bool newEntry;
                    SEntry<AnimationContent>* entry = m_animations.AddEntry(&newEntry, animationPath, 0, false);
                    UpdateImportEntry(entry);

                    EntryModifiedEvent ev;
                    bool modified = m_animations.EntryChanged(&ev.previousContent, entry);
                    if (newEntry)
                    {
                        SignalEntryAdded(SUBTREE_ANIMATIONS, entry->id);
                    }
                    else if (modified)
                    {
                        ev.reason = "Reload";
                        ev.subtree = SUBTREE_ANIMATIONS;
                        ev.id = entry->id;
                        ev.contentChanged = true;
                        SignalEntryModified(ev);
                    }
                }
            }

            if (intermediateExists && !animSettingsExists)
            {
                m_importEntries.push_back(intermediatePath);
                std::sort(m_importEntries.begin(), m_importEntries.end());
                m_importEntries.erase(std::unique(m_importEntries.begin(), m_importEntries.end()), m_importEntries.end());

                if (m_filter.Matches(animationPath.c_str()))
                {
                    bool newEntry;
                    SEntry<AnimationContent>* entry = m_animations.AddEntry(&newEntry, animationPath, 0, false);
                    UpdateImportEntry(entry);

                    EntryModifiedEvent ev;
                    bool modified = m_animations.EntryChanged(&ev.previousContent, entry);
                    if (newEntry)
                    {
                        SignalEntryAdded(SUBTREE_ANIMATIONS, entry->id);
                    }
                    else if (modified)
                    {
                        ev.reason = "Reload";
                        ev.subtree = SUBTREE_ANIMATIONS;
                        ev.id = entry->id;
                        ev.contentChanged = true;
                        SignalEntryModified(ev);
                    }
                }
            }


            if (!intermediateExists)
            {
                //The intermediate file has been removed an no .caf was created for it so remove the animation from the list.
                if (SEntry<AnimationContent>* entry = m_animations.GetByPath(animationPath.c_str()))
                {
                    unsigned int id = entry->id;
                    if (m_animations.RemoveById(id))
                    {
                        SignalEntryRemoved(SUBTREE_ANIMATIONS, id);
                    }
                }
                std::vector<string>::iterator it = std::find(m_importEntries.begin(), m_importEntries.end(), intermediatePath.c_str());
                if (it != m_importEntries.end())
                {
                    m_importEntries.erase(it);
                }
            }

            pakStateFilename = animationPath.c_str();

            if (originalExt == "animsettings")
            {
                if (SEntry<AnimationContent>* entry = m_animations.GetByPath(animationPath.c_str()))
                {
                    if (!entry->modified &&
                        entry->content.importState != AnimationContent::WAITING_FOR_CHRPARAMS_RELOAD)
                    {
                        unsigned int id = entry->id;
                        RevertEntry(id);
                    }
                }
            }
        }
        else if (originalExt == "bspace" || originalExt == "comb")
        {
            bool fileExists = 
                !(eType == IFileChangeListener::eChangeType_Deleted) && 
                 ((eType == IFileChangeListener::eChangeType_Created) || gEnv->pCryPak->IsFileExist(slashPath, ICryPak::eFileLocation_Any));

            if (fileExists)
            {
                ExplorerEntryId entryId;
                
                bool newEntry;
                SEntry<AnimationContent>* entry = m_animations.AddEntry(&newEntry, slashPath, 0, false);
                EntryModifiedEvent ev;
                bool modified = m_animations.EntryChanged(&ev.previousContent, entry);
                if (newEntry)
                {
                    if (originalExt == "bspace")
                    {
                        entry->content.type = AnimationContent::BLEND_SPACE;
                    }
                    else if (originalExt == "comb")
                    {
                        entry->content.type = AnimationContent::COMBINED_BLEND_SPACE;
                    }

                    entry->content.importState = AnimationContent::IMPORTED;

                    LoadAnimationEntry(entry, 0, m_animationSet, 0, m_animEventsFilename.c_str());
                    SignalEntryAdded(SUBTREE_ANIMATIONS, entry->id);
                }
                if (modified)
                {
                    entryId = ExplorerEntryId(SUBTREE_ANIMATIONS, entry->id);
                    LoadAnimationEntry(entry, 0, m_animationSet, 0, m_animEventsFilename.c_str());
                    ev.subtree = SUBTREE_ANIMATIONS;
                    ev.id = entry->id;
                    ev.reason = "Reload";
                    ev.contentChanged = true;
                    SignalEntryModified(ev);
                }
            }
            else
            {
                EntryBase* entry = m_animations.GetBaseByPath(slashPath);
                if (entry)
                {
                    unsigned int id = entry->id;
                    if (m_animations.RemoveById(id))
                    {
                        SignalEntryRemoved(SUBTREE_ANIMATIONS, id);
                    }
                }
            }
            pakStateFilename = slashPath;
        }
        else
        {
            pakStateFilename = slashPath;
        }

        if (EntryBase* entry = m_animations.GetByPath(pakStateFilename))
        {
            int pakState = GetAnimationPakState(pakStateFilename);
            m_system->explorer->SetEntryColumn(ExplorerEntryId(SUBTREE_ANIMATIONS, entry->id), m_explorerColumnPak, pakState, true);
        }
    }

    bool AnimationList::GetEntrySerializer(Serialization::SStruct* out, unsigned int id) const
    {
        if (SEntry<AnimationContent>* anim = m_animations.GetById(id))
        {
            *out = Serialization::SStruct(*anim);
            return true;
        }

        return false;
    }

    void AnimationList::SetMimeDataForEntries(QMimeData* mimeData, const std::vector<unsigned int>& entryIds) const
    {
        QByteArray encodedData;
        QDataStream stream(&encodedData, QIODevice::WriteOnly);

        for (auto id : entryIds)
        {
            if (SEntry<AnimationContent>* anim = m_animations.GetById(id))
            {
                stream << QString(anim->name);
            }
        }

        mimeData->setData(QStringLiteral("application/x-lumberyard-animation"), encodedData);
    }

    static const char* EntryIconByContentType(AnimationContent::Type type, bool isAdditive)
    {
        if (isAdditive)
        {
            return "Editor/Icons/animation/animation_additive.png";
        }
        switch (type)
        {
        case AnimationContent::BLEND_SPACE:
            return "Editor/Icons/animation/animation_bspace.png";
        case AnimationContent::COMBINED_BLEND_SPACE:
            return "Editor/Icons/animation/animation_comb.png";
        case AnimationContent::AIMPOSE:
            return "Editor/Icons/animation/animation_aimpose.png";
        case AnimationContent::LOOKPOSE:
            return "Editor/Icons/animation/animation_lookpose.png";
        default:
            return "Editor/Icons/animation/animation.png";
        }
    }

    static const char* GetContentTypeName(AnimationContent::Type type)
    {
        switch (type)
        {
        case AnimationContent::BLEND_SPACE:
            return "bspace";
        case AnimationContent::COMBINED_BLEND_SPACE:
            return "comb";
        case AnimationContent::AIMPOSE:
            return "aimpose";
        case AnimationContent::LOOKPOSE:
            return "lookpose";
        default:
            return "caf";
        }
    }

    void AnimationList::UpdateEntry(ExplorerEntry* entry)
    {
        if (SEntry<AnimationContent>* anim = m_animations.GetById(entry->id))
        {
            entry->name = anim->name;
            entry->path = anim->path;
            entry->modified = anim->modified;
            entry->icon = EntryIconByContentType(anim->content.type, anim->content.loadedAsAdditive);
            entry->isDragEnabled = true;
            if (anim->content.type == AnimationContent::ANIMATION)
            {
                if (anim->content.importState == AnimationContent::NEW_ANIMATION)
                {
                    entry->icon = "Editor/Icons/animation/animation_offline.png";
                }
            }
        }
    }

    int AnimationList::GetEntryType(int subtreeIndex) const
    {
        return ENTRY_ANIMATION;
    }

    void AnimationList::ActionImport(ActionContext& x)
    {
        m_isSaveRunnerInMultiSaveMode = true;
        for (size_t i = 0; i < x.entries.size(); ++i)
        {
            ExplorerEntry* entry = x.entries[i];
            if (entry && entry->type == ENTRY_ANIMATION)
            {
                ImportAnimation(x.output, entry->id);
            }
        }
        m_isSaveRunnerInMultiSaveMode = false;
        if (m_saveRunner)
        {
            m_saveRunner->Run(x.output,
                [x,this](bool success)
                {
                    if (!success)
                    {
                        QMessageBox::warning(0, "Import Failed", x.output->BuildErrorMessage().c_str());
                    }
                    m_saveRunner = nullptr;
                }, AZ::AsyncSaveRunner::ControllerOrder::Random);
        }
    }

    static void CollectExamplePaths(vector<string>* paths, AnimationList* animationList, SEntry<AnimationContent>* entry)
    {
        if (entry->content.type == AnimationContent::BLEND_SPACE)
        {
            for (size_t i = 0; i < entry->content.blendSpace.m_examples.size(); ++i)
            {
                const BlendSpaceExample& example = entry->content.blendSpace.m_examples[i];
                unsigned int id = animationList->FindIdByAlias(example.animation.c_str());
                if (!id)
                {
                    continue;
                }
                SEntry<AnimationContent>* entry = animationList->GetEntry(id);
                if (entry)
                {
                    paths->push_back(entry->path);
                }
            }
        }
        else if (entry->content.type == AnimationContent::COMBINED_BLEND_SPACE)
        {
            for (size_t i = 0; i < entry->content.combinedBlendSpace.m_blendSpaces.size(); ++i)
            {
                SEntry<AnimationContent>* combinedEntry = animationList->FindEntryByPath(entry->content.combinedBlendSpace.m_blendSpaces[i].path.c_str());
                if (!combinedEntry)
                {
                    continue;
                }
                animationList->LoadOrGetChangedEntry(combinedEntry->id);
                CollectExamplePaths(&*paths, animationList, combinedEntry);
            }
        }
    }

    void AnimationList::ActionCopyExamplePaths(ActionContext& x)
    {
        std::vector<string> paths;

        for (size_t i = 0; i < x.entries.size(); ++i)
        {
            SEntry<AnimationContent>* entry = m_animations.GetById(x.entries[i]->id);
            if (!entry)
            {
                return;
            }

            CollectExamplePaths(&paths, this, entry);
        }

        QString text;
        for (size_t i = 0; i < paths.size(); ++i)
        {
            text += QString::fromLocal8Bit(paths[i].c_str());
            text += "\n";
        }

        QApplication::clipboard()->setText(text);
    }

    void AnimationList::GetEntryActions(vector<ExplorerAction>* actions, unsigned int id, Explorer* explorer)
    {
        SEntry<AnimationContent>* entry = m_animations.GetById(id);

        bool newAnimation = entry && entry->content.importState == AnimationContent::NEW_ANIMATION;
        bool isBlendSpace = entry && entry->content.type == AnimationContent::BLEND_SPACE;
        bool isCombinedBlendSpace = entry && entry->content.type == AnimationContent::COMBINED_BLEND_SPACE;
        bool isAnimSettingsMissing = entry && entry->content.type == static_cast<CharacterTool::AnimationContent::Type>(AnimationContent::COMPILED_BUT_NO_ANIMSETTINGS);
        bool isAnm = entry && entry->content.type == AnimationContent::ANM;

        if (isBlendSpace || isCombinedBlendSpace)
        {
            actions->push_back(ExplorerAction("Copy Example Paths", 0, [=](ActionContext& x) { ActionCopyExamplePaths(x); }));
            actions->push_back(ExplorerAction());
        }

        if (newAnimation)
        {
            actions->push_back(ExplorerAction("Import", ACTION_IMPORTANT, [=](ActionContext& x) { ActionImport(x); }, "Editor/Icons/animation/import.png"));
        }
        else
        {
            actions->push_back(ExplorerAction("Revert", 0, [=](ActionContext& x) { explorer->ActionRevert(x); }, "Editor/Icons/animation/revert.png", "Reload file content from the disk, undoing all changes since last save."));
            bool saveEnabled = !isAnm && !isAnimSettingsMissing;
            actions->push_back(ExplorerAction("Save", saveEnabled ? 0 : ACTION_DISABLED, [=](ActionContext& x) { explorer->ActionSave(x); }, "Editor/Icons/animation/save.png"));
            if (!isAnm)
            {
                actions->push_back(ExplorerAction());
                actions->push_back(ExplorerAction("Export HTR+I_CAF (Lossy)", 0, [=](ActionContext& x) { ActionExportHTR(x); }));
            }
        }
        actions->push_back(ExplorerAction());
        if (!isAnm && !isBlendSpace && !isCombinedBlendSpace)
        {
            actions->push_back(ExplorerAction("Generate Footsteps", ACTION_NOT_STACKABLE, [=](ActionContext& x) { ActionGenerateFootsteps(x); }, "Editor/Icons/animation/footsteps.png", "Creates AnimEvents based on the animation data."));
        }
        actions->push_back(ExplorerAction());
        actions->push_back(ExplorerAction("Show in Explorer", ACTION_NOT_STACKABLE,  [=](ActionContext& x) { explorer->ActionShowInExplorer(x); }, "Editor/Icons/animation/show_in_explorer.png", "Locates file in Windows Explorer."));
    }

    int AnimationList::GetEntryCount(int subtree) const
    {
        return subtree == SUBTREE_ANIMATIONS ? m_animations.Count() : 0;
    }

    unsigned int AnimationList::GetEntryIdByIndex(int subtree, int index) const
    {
        if (subtree == SUBTREE_ANIMATIONS)
        {
            if (SEntry<AnimationContent>* entry = m_animations.GetByIndex(index))
            {
                return entry->id;
            }
        }
        return 0;
    }

    void AnimationList::ActionGenerateFootsteps(ActionContext& x)
    {
        ExplorerEntry* entry = x.entries.front();

        SEntry<AnimationContent>* animation = GetEntry(entry->id);
        if (!animation)
        {
            return;
        }

        FootstepGenerationParameters parameters;

        QPropertyDialog dialog(0);
        dialog.setSerializer(Serialization::SStruct(parameters));
        dialog.setWindowTitle("Footstep Generator");
        dialog.setWindowStateFilename("CharacterTool/FootstepGenerator.state");
        dialog.setSizeHint(QSize(600, 900));
        dialog.setArchiveContext(m_system->contextList->Tail());
        dialog.setStoreContent(true);

        if (dialog.exec() == QDialog::Accepted)
        {
            string errorMessage;
            if (!GenerateFootsteps(&animation->content, &errorMessage, m_system->document->CompressedCharacter(), animation->name.c_str(), parameters))
            {
                x.output->AddError(errorMessage.c_str(), animation->path.c_str());
            }
            else
            {
                CheckIfModified(entry->id, "Footstep Generation", false);
            }
        }
    }

    void AnimationList::ActionExportHTR(ActionContext& x)
    {
        QString gameFolder = QString::fromLocal8Bit(Path::GetEditingGameDataFolder().c_str());
        QDir gameFolderDir(QDir::fromNativeSeparators(gameFolder));

        ICharacterInstance* character = m_system->document->CompressedCharacter();
        if (!character)
        {
            x.output->AddError("Character has to be loaded for HTR export to function.", "");
            return;
        }
        IAnimationSet* animationSet = character->GetIAnimationSet();
        if (!animationSet)
        {
            x.output->AddError("Missing animation set", "");
            return;
        }

        for (size_t i = 0; i < x.entries.size(); ++i)
        {
            ExplorerEntry* entry = x.entries[i];

            const char* animationPath = entry->path.c_str();
            const char* animationName = entry->name.c_str();
            string initialPath = gameFolderDir.absoluteFilePath(PathUtil::GetParentDirectory(animationPath).c_str()).toLocal8Bit().data();
            string exportDirectory = QFileDialog::getExistingDirectory(0, "Export Directory", initialPath.c_str()).toLocal8Bit().data();
            if (exportDirectory.empty())
            {
                return;
            }

            if (exportDirectory[exportDirectory.size() - 1] != '\\' && exportDirectory[exportDirectory.size() - 1] != '/')
            {
                exportDirectory += "\\";
            }


            int animationId = animationSet->GetAnimIDByName(animationName);
            if (animationId < 0)
            {
                x.output->AddError("Missing animation in animationSet", animationName);
                return;
            }

            SEntry<AnimationContent>* e = GetEntry(entry->id);
            if (!e)
            {
                x.output->AddError("Missing animation entry.", entry->path.c_str());
                return;
            }

            if (e->content.type == AnimationContent::BLEND_SPACE || e->content.type == AnimationContent::COMBINED_BLEND_SPACE)
            {
                if (!gEnv->pCharacterManager->LMG_LoadSynchronously(character->GetIAnimationSet()->GetFilePathCRCByAnimID(animationId), animationSet))
                {
                    x.output->AddError("Failed to load blendspace synchronously", animationPath);
                    return;
                }
            }
            else
            {
                if (!gEnv->pCharacterManager->CAF_LoadSynchronously(character->GetIAnimationSet()->GetFilePathCRCByAnimID(animationId)))
                {
                    x.output->AddError("Failed to load CAF synchronously", animationPath);
                    return;
                }
            }

            ISkeletonAnim& skeletonAnimation = *character->GetISkeletonAnim();
            if (!skeletonAnimation.ExportHTRAndICAF(animationName, exportDirectory.c_str()))
            {
                x.output->AddError("Failed to export HTR.", exportDirectory.c_str());
            }
        }
    }

    // ---------------------------------------------------------------------------

    QString AnimationAliasSelector(const SResourceSelectorContext& x, const QString& previousValue, ICharacterInstance* character)
    {
        if (!character)
        {
            return previousValue;
        }

        QWidget parent(x.parentWidget);
        parent.setWindowModality(Qt::ApplicationModal);

        ListSelectionDialog dialog(&parent);
        dialog.setWindowTitle("Animation Alias Selection");
        dialog.setWindowIcon(QIcon(GetIEditor()->GetResourceSelectorHost()->ResourceIconPath(x.typeName)));

        IAnimationSet* animationSet = character->GetIAnimationSet();
        if (!animationSet)
        {
            return previousValue;
        }
        IDefaultSkeleton& skeleton = character->GetIDefaultSkeleton();

        dialog.SetColumnText(0, "Animation Alias");
        dialog.SetColumnText(1, "Asset Type");
        dialog.SetColumnWidth(1, 80);

        int count = animationSet->GetAnimationCount();
        for (int i = 0; i < count; ++i)
        {
            const char* name = animationSet->GetNameByAnimID(i);
            const char* animationPath = animationSet->GetFilePathByID(i);
            if (IsInternalAnimationName(name, animationPath))
            {
                continue;
            }

            AnimationContent::Type type = AnimationContent::ANIMATION;

            int flags = animationSet->GetAnimationFlags(i);
            if (flags & CA_ASSET_LMG)
            {
                if (animationSet->IsCombinedBlendSpace(i))
                {
                    type = AnimationContent::COMBINED_BLEND_SPACE;
                }
                else
                {
                    type = AnimationContent::BLEND_SPACE;
                }
            }
            else if (flags & CA_AIMPOSE)
            {
                if (animationSet->IsAimPose(i, skeleton))
                {
                    type = AnimationContent::AIMPOSE;
                }
                else if (animationSet->IsLookPose(i, skeleton))
                {
                    type = AnimationContent::LOOKPOSE;
                }
            }
            bool isAdditive = (flags & CA_ASSET_ADDITIVE) != 0;
            const char* icon = EntryIconByContentType(type, isAdditive);
            const char* typeName = GetContentTypeName(type);

            dialog.AddRow(name, QIcon(icon));
            dialog.AddRowColumn(typeName);
        }

        return dialog.ChooseItem(previousValue);
    }
    REGISTER_RESOURCE_SELECTOR("AnimationAlias", AnimationAliasSelector, "Editor/Icons/animation/animation.png")
}

#include <CharacterTool/AnimationList.moc>
