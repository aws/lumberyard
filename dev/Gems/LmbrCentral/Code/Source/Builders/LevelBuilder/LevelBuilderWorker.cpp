/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <LmbrCentral_precompiled.h>
#include "LevelBuilderWorker.h"
#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/Archive/ArchiveAPI.h>
#include <AzCore/XML/rapidxml.h>
#include "AzFramework/Asset/SimpleAsset.h"
#include "AzCore/Component/Entity.h"
#include <AzCore/std/string/utf8/unchecked.h>

namespace LevelBuilder
{
    const char s_materialExtension[] = ".mtl";
    const char s_audioControlFilesLevelPath[] = "@devassets@/libs/gameaudio/wwise/levels/%s";
    const char s_audioControlFilter[] = "*.xml";

    AZ::u64 readXmlDataLength(AZ::IO::GenericStream* stream, int& charSize)
    {
        // This code is replicated from readStringLength method found in .\dev\Code\Sandbox\Editor\Util\EditorUtils.h file
        // such that we do not have any Cry or Qt related dependencies.
        // The basic algorithm is that it reads in an 8 bit int, and if the length is less than 2^8,
        // then that's the length. Next it reads in a 16 bit int, and if the length is less than 2^16,
        // then that's the length. It does the same thing for 32 bit values and finally for 64 bit values.
        // The 16 bit length also indicates whether or not it's a UCS2 / wide-char Windows string, if it's
        // 0xfffe, but that comes after the first byte marker indicating there's a 16 bit length value.
        // So, if the first 3 bytes are: 0xFF, 0xFF, 0xFE, it's a 2 byte string being read in, and the real
        // length follows those 3 bytes (which may still be an 8, 16, or 32 bit length).

        // default to one byte strings
        charSize = 1;

        AZ::u8 len8;
        stream->Read(sizeof(AZ::u8), &len8);
        if (len8 < 0xff)
        {
            return len8;
        }

        AZ::u16 len16;
        stream->Read(sizeof(AZ::u16), &len16);
        if (len16 == 0xfffe)
        {
            charSize = 2;

            stream->Read(sizeof(AZ::u8), &len8);
            if (len8 < 0xff)
            {
                return len8;
            }

            stream->Read(sizeof(AZ::u16), &len16);
        }

        if (len16 < 0xffff)
        {
            return len16;
        }

        AZ::u32 len32;
        stream->Read(sizeof(AZ::u32), &len32);

        if (len32 < 0xffffffff)
        {
            return len32;
        }

        AZ::u64 len64;
        stream->Read(sizeof(AZ::u64), &len64);

        return len64;
    }



    void LevelBuilderWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void LevelBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor descriptor;
            descriptor.m_jobKey = "Level Builder Job";
            descriptor.m_critical = true;
            descriptor.SetPlatformIdentifier(info.m_identifier.c_str());
            response.m_createJobOutputs.push_back(descriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void LevelBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "LevelBuilderWorker Starting Job.\n");

        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
        AssetBuilderSDK::ProductPathDependencySet productPathDependencies;

        AZStd::string tempUnpackDirectory;
        AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), "LevelUnpack", tempUnpackDirectory);
        AZ::IO::LocalFileIO fileIO;
        fileIO.DestroyPath(tempUnpackDirectory.c_str());
        fileIO.CreatePath(tempUnpackDirectory.c_str());
        PopulateProductDependencies(request.m_fullPath, request.m_sourceFile, tempUnpackDirectory, productDependencies, productPathDependencies);

        // level.pak needs to be copied into the cache, emitting the source as a product will have the
        // asset processor take care of that.
        AssetBuilderSDK::JobProduct jobProduct(request.m_fullPath);
        jobProduct.m_dependencies = AZStd::move(productDependencies);
        jobProduct.m_pathDependencies = AZStd::move(productPathDependencies);
        jobProduct.m_dependenciesHandled = true; // We've populated the dependencies immediately above so it's OK to tell the AP we've handled dependencies
        response.m_outputProducts.push_back(jobProduct);
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    void LevelBuilderWorker::PopulateProductDependencies(
        const AZStd::string& levelPakFile,
        const AZStd::string& sourceRelativeFile,
        const AZStd::string& tempDirectory,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const
    {
        PopulateOptionalLevelDependencies(sourceRelativeFile, productPathDependencies);

        AZStd::binary_semaphore extractionCompleteSemaphore;
        auto extractResponseLambda = [&](bool success) {
            AZStd::string levelsubfolder;
            AzFramework::StringFunc::Path::Join(tempDirectory.c_str(), "level", levelsubfolder);

            PopulateLevelSliceDependencies(levelsubfolder, productDependencies, productPathDependencies);
            PopulateLevelDataDependencies(levelPakFile, levelsubfolder, productPathDependencies);
            PopulateMissionDependencies(levelPakFile, levelsubfolder, productPathDependencies);
            PopulateLevelAudioControlDependencies(levelPakFile, productPathDependencies);
            PopulateVegetationMapDataDependencies(levelPakFile, productPathDependencies);

            extractionCompleteSemaphore.release();
        };

        AZ::Uuid handle = AZ::Uuid::Create();
        AzToolsFramework::ArchiveCommands::Bus::Broadcast(
            &AzToolsFramework::ArchiveCommands::ExtractArchive,
            levelPakFile,
            tempDirectory,
            handle,
            extractResponseLambda);

        const int archiveExtractSleepMS = 20;
        bool extractionCompleted = false;
        while (!extractionCompleted)
        {
            extractionCompleted = extractionCompleteSemaphore.try_acquire_for(AZStd::chrono::milliseconds(archiveExtractSleepMS));
            // When the archive extraction is completed, the response lambda is queued on the the tick bus.
            // This loop will keep executing queued events on the tickbus until the response unlocks the semaphore.
            AZ::TickBus::ExecuteQueuedEvents();
        }
    }

    AZStd::string GetLastFolderFromPath(const AZStd::string& path)
    {
        AZStd::string result(path);
        // AzFramework::StringFunc::Path::GetFolder gives different results in debug and profile, so get the last folder from the path another way.
        size_t lastSeparator(result.find_last_of(AZ_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR));
        // If it ends with a slash, strip it and try again.
        if (lastSeparator == result.length() - 1)
        {
            result = result.substr(0, result.length()-1);
            lastSeparator = result.find_last_of(AZ_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR);
        }
        return result.substr(lastSeparator + 1);
    }

    void LevelBuilderWorker::PopulateOptionalLevelDependencies(
        const AZStd::string& sourceRelativeFile,
        AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const
    {
        AZStd::string sourceLevelPakPath = sourceRelativeFile;
        AzFramework::StringFunc::Path::StripFullName(sourceLevelPakPath);

        AZStd::string levelFolderName(GetLastFolderFromPath(sourceLevelPakPath));

        // Hardcoded paths are used here instead of the defines because:
        //  The defines exist in CryEngine code that we can't link from here.
        //  We want to minimize engine changes to make it easier for game teams to incorporate these dependency improvements.

        //  LevelSystem.cpp, CLevelSystem::PopulateLevels stores all pak files in the level folder
        //  to load them during opening a level. AFAIK apart from level.pak, terraintexture.pak is the only pak file that might be present in the cached level folder.
        //  string paks = levelPath + string("/*.pak");
        AddLevelRelativeSourcePathProductDependency("terraintexture.pak", sourceLevelPakPath, productPathDependencies);

        // TerrainCompile.cpp, CTerrain::Load attempts to load the cover.ctc file for the terrain.
        //      OpenTerrainTextureFile(COMPILED_TERRAIN_TEXTURE_FILE_NAME)
        AddLevelRelativeSourcePathProductDependency("terrain/cover.ctc", sourceLevelPakPath, productPathDependencies);

        // C3DEngine::LoadParticleEffects attempts to load the particle library "@PreloadLibs".
        // CParticleManager::LoadLibrary treats any attempted library load that starts with the @ character as a preload library.
        // When CParticleManager::LoadLibrary attempts to load a preload library, it checks the current level first, before the global preload.
        AddLevelRelativeSourcePathProductDependency("preloadlibs.txt", sourceLevelPakPath, productPathDependencies);

        // C3DEngine::LoadParticleEffects attempts to load the level specific particle file, if it exists.
        //      m_pPartManager->LoadLibrary("Level", GetLevelFilePath(PARTICLES_FILE), true);
        AddLevelRelativeSourcePathProductDependency("levelparticles.xml", sourceLevelPakPath, productPathDependencies);

        // CCullThread::LoadLevel attempts to load the occluder mesh, if it exists.
        //      AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen((string(pFolderName) + "/occluder.ocm").c_str(), "rbx");
        AddLevelRelativeSourcePathProductDependency("occluder.ocm", sourceLevelPakPath, productPathDependencies);

        // C3DEngine::LoadLevel attempts to load this file for the current level, if it exists.
        //      GetISystem()->LoadConfiguration(GetLevelFilePath(LEVEL_CONFIG_FILE));
        AddLevelRelativeSourcePathProductDependency("level.cfg", sourceLevelPakPath, productPathDependencies);

        // CResourceManager::PrepareLevel attemps to load this file for the current level, if it exists.
        //      string filename = PathUtil::Make(sLevelFolder, AUTO_LEVEL_RESOURCE_LIST);
        //      if (!pResList->Load(filename.c_str()))
        AddLevelRelativeSourcePathProductDependency("auto_resourcelist.txt", sourceLevelPakPath, productPathDependencies);

        // CMergedMeshesManager::PreloadMeshes attempts to load this file for the current level, if it exists.
        //      AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(Get3DEngine()->GetLevelFilePath(COMPILED_MERGED_MESHES_BASE_NAME COMPILED_MERGED_MESHES_LIST), "r");
        AddLevelRelativeSourcePathProductDependency("terrain/merged_meshes_sectors/mmrm_used_meshes.lst", sourceLevelPakPath, productPathDependencies);

        // CLevelInfo::ReadMetaData() constructs a string based on levelName/LevelName.xml, and attempts to read that file.
        AZStd::string levelXml(AZStd::string::format("%s.xml", levelFolderName.c_str()));
        AddLevelRelativeSourcePathProductDependency(levelXml, sourceLevelPakPath, productPathDependencies);
    }

    void LevelBuilderWorker::AddLevelRelativeSourcePathProductDependency(
        const AZStd::string& optionalDependencyRelativeToLevel,
        const AZStd::string& sourceLevelPakPath,
        AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const
    {
        AZStd::string sourceDependency;
        AzFramework::StringFunc::Path::Join(
            sourceLevelPakPath.c_str(),
            optionalDependencyRelativeToLevel.c_str(),
            sourceDependency,
            false);
        productPathDependencies.emplace(sourceDependency, AssetBuilderSDK::ProductPathDependencyType::SourceFile);
    }

    void LevelBuilderWorker::PopulateLevelSliceDependencies(
        const AZStd::string& levelPath,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const
    {
        const char levelDynamicSliceFileName[] = "mission0.entities_xml";

        AZStd::string entityFilename;
        AzFramework::StringFunc::Path::Join(levelPath.c_str(), levelDynamicSliceFileName, entityFilename);

        AZ::IO::FileIOStream fileStream;

        if (fileStream.Open(entityFilename.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            PopulateLevelSliceDependenciesHelper(&fileStream, productDependencies, productPathDependencies);
        }
    }

    void LevelBuilderWorker::PopulateLevelSliceDependenciesHelper(AZ::IO::GenericStream* assetStream, AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies, AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ::SliceAssetHandler assetHandler(context);
        AZ::Data::Asset<AZ::SliceAsset> tempLevelSliceAsset;
        tempLevelSliceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        assetHandler.LoadAssetData(tempLevelSliceAsset, assetStream, &AZ::Data::AssetFilterNoAssetLoading);

        AZ::Entity* entity = tempLevelSliceAsset.Get()->GetEntity();

        AssetBuilderSDK::GatherProductDependencies(*context, entity, productDependencies, productPathDependencies);

    }

    void LevelBuilderWorker::PopulateLevelDataDependencies(
        const AZStd::string& levelPakFile,
        const AZStd::string& levelPath,
        AssetBuilderSDK::ProductPathDependencySet& productDependencies) const
    {
        const char levelDataFileName[] = "leveldata.xml";

        AZStd::string levelDataFullPath;
        AzFramework::StringFunc::Path::Join(levelPath.c_str(), levelDataFileName, levelDataFullPath);

        AZ::IO::FileIOStream fileStream;
        
        if (fileStream.Open(levelDataFullPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            PopulateLevelDataDependenciesHelper(&fileStream, productDependencies);
        }
    }

    bool LevelBuilderWorker::PopulateLevelDataDependenciesHelper(AZ::IO::GenericStream* stream,
        AssetBuilderSDK::ProductPathDependencySet& productDependencies) const
    {
        if (!stream)
        {
            return false;
        }

        AZ::IO::SizeType length = stream->GetLength();

        if (length == 0)
        {
            return false;
        }

        AZStd::vector<char> charBuffer;
        
        charBuffer.resize_no_construct(length + 1);
        stream->Read(length, charBuffer.data());
        charBuffer.back() = 0;
        
        AZ::rapidxml::xml_document<char> xmlDoc;
        xmlDoc.parse<AZ::rapidxml::parse_no_data_nodes>(charBuffer.data());
        auto levelDataNode = xmlDoc.first_node("LevelData");

        if(!levelDataNode)
        {
            return false;
        }

        auto surfaceTypesNode = levelDataNode->first_node("SurfaceTypes");

        if (!surfaceTypesNode)
        {
            return false;
        }

        auto surfaceTypeNode = surfaceTypesNode->first_node();

        while(surfaceTypeNode)
        {
            auto detailMatrialAttr = surfaceTypeNode->first_attribute("DetailMaterial");

            if (detailMatrialAttr)
            {
                // Add on the material file extension since it isn't included in the xml
                productDependencies.emplace(AZStd::string(detailMatrialAttr->value()) + s_materialExtension, AssetBuilderSDK::ProductPathDependencyType::ProductFile);
            }

            surfaceTypeNode = surfaceTypeNode->next_sibling();
        }

        return true;
    }

    void LevelBuilderWorker::PopulateVegetationMapDataDependencies(
        const AZStd::string& levelPakFile,
        AssetBuilderSDK::ProductPathDependencySet& productDependencies) const
    {
        const char vegetationMapDataFileName[] = "leveldata/VegetationMap.dat";
        AZStd::string vegetationMapDataFullPath;
        AZStd::string levelPakDir;
        AzFramework::StringFunc::Path::GetFullPath(levelPakFile.c_str(), levelPakDir);

        AzFramework::StringFunc::Path::Join(levelPakDir.c_str(), vegetationMapDataFileName, vegetationMapDataFullPath);

        AZ::IO::FileIOStream fileStream;

        if (fileStream.Open(vegetationMapDataFullPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            if (!PopulateVegetationMapDataDependenciesHelper(&fileStream, productDependencies))
            {
                AZ_Error("LevelBuilder", false, "Unable to retreive product dependencies from file (%s).", vegetationMapDataFullPath.c_str());
            }
        }
        else
        {
            AZ_Error("LevelBuilder", false, "Failed to open file (%s).", vegetationMapDataFullPath.c_str());
        }
    }

    bool LevelBuilderWorker::PopulateVegetationMapDataDependenciesHelper(AZ::IO::GenericStream* stream,
        AssetBuilderSDK::ProductPathDependencySet& productDependencies) const
    {
        if (!stream)
        {
            return false;
        }

        int charSize = 1;
        AZ::IO::SizeType length = static_cast<AZ::IO::SizeType>(readXmlDataLength(stream, charSize));

        if (length == 0)
        {
            return false;
        }
        AZStd::string xmlString;

        if (charSize == 1)
        {
            xmlString.resize(length);
            stream->Read(length, xmlString.data());
        }
        else
        {
            AZStd::vector<AZ::u16> dataBuffer;
            dataBuffer.resize(length);
            stream->Read(length, dataBuffer.data());
            Utf8::Unchecked::utf16to8(dataBuffer.begin(), dataBuffer.end(), AZStd::back_inserter(xmlString));
        }

        AZ::rapidxml::xml_document<char> xmlDoc;
        xmlDoc.parse<AZ::rapidxml::parse_no_data_nodes>(xmlString.data());
        auto vegetationMapDataNode = xmlDoc.first_node("VegetationMap");

        if (!vegetationMapDataNode)
        {
            return false;
        }

        auto objectsTypeNode = vegetationMapDataNode->first_node("Objects");

        if (!objectsTypeNode)
        {
            return false;
        }

        auto objectTypeNode = objectsTypeNode->first_node();

        while (objectTypeNode)
        {
            auto fileNameAttr = objectTypeNode->first_attribute("FileName");

            if (fileNameAttr)
            {
                productDependencies.emplace(AZStd::string(fileNameAttr->value()), AssetBuilderSDK::ProductPathDependencyType::ProductFile);
            }

            objectTypeNode = objectTypeNode->next_sibling();
        }

        return true;
    }

    void LevelBuilderWorker::PopulateMissionDependencies(
        const AZStd::string& levelPakFile,
        const AZStd::string& levelPath,
        AssetBuilderSDK::ProductPathDependencySet& productDependencies) const
    {
        const char* fileName = "mission_mission0.xml";

        AZStd::string fileFullPath;
        AzFramework::StringFunc::Path::Join(levelPath.c_str(), fileName, fileFullPath);

        AZ::IO::FileIOStream fileStream;

        if (fileStream.Open(fileFullPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            PopulateMissionDependenciesHelper(&fileStream, productDependencies);
        }
    }

    void LevelBuilderWorker::PopulateLevelAudioControlDependenciesHelper(
        const AZStd::string& levelName, 
        AssetBuilderSDK::ProductPathDependencySet& productDependencies) const
    {
        AZStd::string levelScopedControlsPath = AZStd::string::format(s_audioControlFilesLevelPath, levelName.c_str());

        // remove the alias at the front of path passed in to get the path relative to the cache.
        AZStd::string relativePath = levelScopedControlsPath;
        AzFramework::StringFunc::RKeep(relativePath, relativePath.find_first_of('/'));

        productDependencies.emplace(relativePath + "/" + s_audioControlFilter, AssetBuilderSDK::ProductPathDependencyType::ProductFile);
    }

    void LevelBuilderWorker::PopulateLevelAudioControlDependencies(
        const AZStd::string& levelPakFile, 
        AssetBuilderSDK::ProductPathDependencySet& productDependencies) const
    {
        AZStd::string normalizedPakPath = levelPakFile;
        AzFramework::StringFunc::Path::Normalize(normalizedPakPath);

        AZStd::string levelName;
        AzFramework::StringFunc::Path::GetFolder(normalizedPakPath.c_str(), levelName);

        // modify the level name to the scope name that the audio controls editor would use
        AZStd::to_lower(levelName.begin(), levelName.end());

        PopulateLevelAudioControlDependenciesHelper(levelName, productDependencies);
    }

    bool GetAttribute(const AZ::rapidxml::xml_node<char>* parentNode, const char* childNodeName, const char* attributeName, const char*& outValue)
    {
        const auto* childNode = parentNode->first_node(childNodeName);

        if (!childNode)
        {
            return false;
        }

        const auto* attribute = childNode->first_attribute(attributeName);

        if (!attribute)
        {
            return false;
        }

        outValue = attribute->value();

        return true;
    }

    bool AddAttribute(const AZ::rapidxml::xml_node<char>* parentNode, const char* childNodeName, const char* attributeName, bool required, const char* extensionToAppend, AssetBuilderSDK::ProductPathDependencySet& dependencySet)
    {
        const char* attributeValue = nullptr;

        if (!GetAttribute(parentNode, childNodeName, attributeName, attributeValue) && required)
        {
            return false;
        }

        if (attributeValue && strlen(attributeValue))
        {
            dependencySet.emplace(AZStd::string(attributeValue) + (extensionToAppend ? extensionToAppend : ""), AssetBuilderSDK::ProductPathDependencyType::ProductFile);
        }

        return true;
    }

    bool LevelBuilderWorker::PopulateMissionDependenciesHelper(AZ::IO::GenericStream* stream,
        AssetBuilderSDK::ProductPathDependencySet& productDependencies) const
    {
        if (!stream)
        {
            return false;
        }

        AZ::IO::SizeType length = stream->GetLength();

        if (length == 0)
        {
            return false;
        }

        AZStd::vector<char> charBuffer;

        charBuffer.resize_no_construct(length + 1);
        stream->Read(length, charBuffer.data());
        charBuffer.back() = 0;

        AZ::rapidxml::xml_document<char> xmlDoc;
        xmlDoc.parse<AZ::rapidxml::parse_no_data_nodes>(charBuffer.data());
        
        const auto* missionNode = xmlDoc.first_node("Mission");

        if (!missionNode)
        {
            return false;
        }

        const auto* environmentNode = missionNode->first_node("Environment");

        if (!environmentNode)
        {
            return false;
        }

        if(!AddAttribute(environmentNode, "SkyBox", "Material", true, s_materialExtension, productDependencies)
            || !AddAttribute(environmentNode, "SkyBox", "MaterialLowSpec", true, s_materialExtension, productDependencies)
            || !AddAttribute(environmentNode, "Ocean", "Material", true, s_materialExtension, productDependencies)
            || !AddAttribute(environmentNode, "Moon", "Texture", false, nullptr, productDependencies)
            || !AddAttribute(environmentNode, "CloudShadows", "CloudShadowTexture", false, nullptr, productDependencies))
        {
            return false;
        }

        return true;
    }
}
