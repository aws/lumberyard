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

#include "StdAfx.h"

#include <IAISystem.h>
#include <IGame.h>
#include <I3DEngine.h>
#include <ITerrain.h>

#include "GameEngine.h"
#include "CryEditDoc.h"
#include "Mission.h"
#include "ShaderCache.h"

#include "ParticleExporter.h"

#include "Material/MaterialManager.h"
#include "Material/MaterialLibrary.h"
#include "Particles/ParticleManager.h"
#include "GameTokens/GameTokenManager.h"
#include "AI/NavDataGeneration/Navigation.h"

#include "VegetationMap.h"
#include "Terrain/TerrainManager.h"
#include "Terrain/Heightmap.h"
#include "Terrain/TerrainGrid.h"
#include "Terrain/MacroTextureExporter.h"

#include "Objects/ObjectLayerManager.h"
#include "Objects/ObjectPhysicsManager.h"
#include "Objects/EntityObject.h"
#include "LensFlareEditor/LensFlareManager.h"
#include "LensFlareEditor/LensFlareLibrary.h"
#include "LensFlareEditor/LensFlareItem.h"
#include "GameExporter.h"
#include "EntityPrototypeManager.h"
#include "Prefabs/PrefabManager.h"
#include "CheckOutDialog.h"
#include "Util/FileUtil.h"

#include <ILevelSystem.h>
#include <IGameFramework.h>
#include <StatObjBus.h>

#include <qfile.h>
#include <qdir.h>
#include <ScopeGuard.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include "QtUtil.h"

//////////////////////////////////////////////////////////////////////////
#define MUSIC_LEVEL_LIBRARY_FILE "Music.xml"
#define GAMETOKENS_LEVEL_LIBRARY_FILE "LevelGameTokens.xml"
#define MATERIAL_LEVEL_LIBRARY_FILE "Materials.xml"
#define RESOURCE_LIST_FILE "ResourceList.txt"
#define PERLAYER_RESOURCE_LIST_FILE "PerLayerResourceList.txt"
#define SHADER_LIST_FILE "ShadersList.txt"

#define GetAValue(rgb)      ((BYTE)((rgb) >> 24))

//////////////////////////////////////////////////////////////////////////
// SGameExporterSettings
//////////////////////////////////////////////////////////////////////////
SGameExporterSettings::SGameExporterSettings()
    : iExportTexWidth(4096)
    , nApplySS(1)
{
}


//////////////////////////////////////////////////////////////////////////
void SGameExporterSettings::SetLowQuality()
{
    iExportTexWidth = 4096;
    nApplySS = 0;
}


//////////////////////////////////////////////////////////////////////////
void SGameExporterSettings::SetHiQuality()
{
    iExportTexWidth = 16384;
    nApplySS = 1;
}

CGameExporter* CGameExporter::m_pCurrentExporter = NULL;

//////////////////////////////////////////////////////////////////////////
// CGameExporter
//////////////////////////////////////////////////////////////////////////
CGameExporter::CGameExporter()
    : m_bAutoExportMode(false)
{
    m_pCurrentExporter = this;
}

CGameExporter::~CGameExporter()
{
    m_pCurrentExporter = NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CGameExporter::Export(unsigned int flags, EEndian eExportEndian, const char* subdirectory)
{
    CAutoDocNotReady autoDocNotReady;
    CObjectManagerLevelIsExporting levelIsExportingFlag;
    QWaitCursor waitCursor;

    IEditor* pEditor = GetIEditor();
    CGameEngine* pGameEngine = pEditor->GetGameEngine();
    if (pGameEngine->GetLevelPath().isEmpty())
    {
        return false;
    }

    bool exportSuccessful = true;

    CrySystemEventBus::Broadcast(&CrySystemEventBus::Events::OnCryEditorBeginLevelExport);
    pEditor->Notify(eNotify_OnBeginExportToGame);

    CObjectManager* pObjectManager = static_cast<CObjectManager*>(pEditor->GetObjectManager());
    CObjectLayerManager* pObjectLayerManager = pObjectManager->GetLayersManager();

    QDir::setCurrent(pEditor->GetMasterCDFolder());

    // Close all Editor tools
    pEditor->SetEditTool(0);

    QString sLevelPath = Path::AddSlash(pGameEngine->GetLevelPath());
    if (subdirectory && subdirectory[0] && strcmp(subdirectory, ".") != 0)
    {
        sLevelPath = Path::AddSlash(sLevelPath + subdirectory);
        QDir().mkpath(sLevelPath);
    }

    m_levelPak.m_sPath = QString(sLevelPath) + GetLevelPakFilename();

    m_levelPath = Path::RemoveBackslash(sLevelPath);
    QString rootLevelPath = Path::AddSlash(pGameEngine->GetLevelPath());

    if (pObjectLayerManager->InitLayerSwitches())
    {
        pObjectLayerManager->SetupLayerSwitches(false, true);
    }

    // Exclude objects from layers without export flag
    pObjectManager->UnregisterNoExported();

    // Make sure we unload any unused CGFs before exporting so that they don't end up in
    // the level data.
    pEditor->Get3DEngine()->FreeUnusedCGFResources();

    CCryEditDoc* pDocument = pEditor->GetDocument();

    if (flags & eExp_Fast)
    {
        m_settings.SetLowQuality();
    }
    else if (m_bAutoExportMode)
    {
        m_settings.SetHiQuality();
    }

    CryAutoLock<CryMutex> autoLock(CGameEngine::GetPakModifyMutex());

    // Close this pak file.
    if (!CloseLevelPack(m_levelPak, true))
    {
        Error("Cannot close Pak file " + m_levelPak.m_sPath);
        exportSuccessful = false;
    }

    if (exportSuccessful)
    {
        if (m_bAutoExportMode)
        {
            // Remove read-only flags.
            CrySetFileAttributes(m_levelPak.m_sPath.toUtf8().data(), FILE_ATTRIBUTE_NORMAL);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    if (exportSuccessful)
    {
        if (!CFileUtil::OverwriteFile(m_levelPak.m_sPath))
        {
            Error("Cannot overwrite Pak file " + m_levelPak.m_sPath);
            exportSuccessful = false;
        }
    }

    if (exportSuccessful)
    {
        if (!OpenLevelPack(m_levelPak, false))
        {
            Error("Cannot open Pak file " + m_levelPak.m_sPath + " for writing.");
            exportSuccessful = false;
        }

    }

    ////////////////////////////////////////////////////////////////////////
    // Inform all objects that an export is about to begin
    ////////////////////////////////////////////////////////////////////////
    if (exportSuccessful)
    {
        GetIEditor()->GetObjectManager()->GetPhysicsManager()->PrepareForExport();
        GetIEditor()->GetObjectManager()->SendEvent(EVENT_PRE_EXPORT);
    }

    ////////////////////////////////////////////////////////////////////////
    // Export all data to the game
    ////////////////////////////////////////////////////////////////////////
    if (exportSuccessful)
    {
        if (!ExportMap(sLevelPath.toUtf8().data(), flags & eExp_SurfaceTexture, eExportEndian))
        {
            exportSuccessful = false;
        }
    }

    if (exportSuccessful)
    {
        ////////////////////////////////////////////////////////////////////////
        // Export the heightmap, store shadow informations in it
        ////////////////////////////////////////////////////////////////////////
        ExportHeightMap(sLevelPath.toUtf8().data(), eExportEndian);

        ////////////////////////////////////////////////////////////////////////
        // Exporting map setttings
        ////////////////////////////////////////////////////////////////////////
        ExportOcclusionMesh(sLevelPath.toUtf8().data());

        //////////////////////////////////////////////////////////////////////////
        // Export Particles.
        {
            CParticlesExporter partExporter;
            partExporter.ExportParticles(sLevelPath.toUtf8().data(), m_levelPath.toUtf8().data(), m_levelPak.m_pakFile);
        }

        // Export prototypes/archetypes
        {
            GetIEditor()->GetEntityProtManager()->ExportPrototypes(sLevelPath, m_levelPath, m_levelPak.m_pakFile);
        }

        //! Export Level data.
        CLogFile::WriteLine("Exporting LevelData.xml");
        ExportLevelData(sLevelPath);
        CLogFile::WriteLine("Exporting LevelData.xml done.");

        ExportLevelInfo(sLevelPath);

        // (MATT) Function copies bai files into PAK (and attempts to do other things) {2008/08/11}
        //if(bAI) bAI was always true
        {
            if (!(flags & eExp_Fast))
            {
                CLogFile::WriteLine("Regenerating AI data!");
                pGameEngine->GenerateAiAll();
            }
            ExportAI(sLevelPath, flags & eExp_CoverSurfaces);
        }

        CLogFile::WriteLine("Exporting Game Data...");
        ExportGameData(sLevelPath);
        CLogFile::WriteLine("Exporting Game Data done");

        //////////////////////////////////////////////////////////////////////////
        // Export Brushes.
        //////////////////////////////////////////////////////////////////////////
        ExportBrushes(sLevelPath);

        ExportLevelLensFlares(sLevelPath);
        ExportLevelResourceList(sLevelPath);
        ExportLevelPerLayerResourceList(sLevelPath);
        ExportLevelShaderCache(sLevelPath);
        //////////////////////////////////////////////////////////////////////////
        // Export list of entities to save/load during gameplay
        //////////////////////////////////////////////////////////////////////////
        CLogFile::WriteLine("Exporting serialization list");
        XmlNodeRef entityList = XmlHelpers::CreateXmlNode("EntitySerialization");
        pGameEngine->BuildEntitySerializationList(entityList);
        QString levelDataFile = sLevelPath + "Serialize.xml";
        XmlString xmlData = entityList->getXML();
        CCryMemFile file;
        file.Write(xmlData.c_str(), xmlData.length());
        m_levelPak.m_pakFile.UpdateFile(levelDataFile.toUtf8().data(), file);

        //////////////////////////////////////////////////////////////////////////
        // End Exporting Game data.
        //////////////////////////////////////////////////////////////////////////

        // Close all packs.
        CloseLevelPack(m_levelPak, false);
        //  m_texturePakFile.Close();

        pObjectManager->RegisterNoExported();

        pObjectLayerManager->InitLayerSwitches(true);

        ////////////////////////////////////////////////////////////////////////
        // Reload the level in the engine
        ////////////////////////////////////////////////////////////////////////
        if (flags & eExp_ReloadTerrain)
        {
            pEditor->SetStatusText(QObject::tr("Reloading Level..."));
            pGameEngine->ReloadLevel();
        }

        pEditor->SetStatusText(QObject::tr("Ready"));

        // Disabled, for now. (inside EncryptPakFile)
        EncryptPakFile(m_levelPak.m_sPath);

        // Reopen this pak file.
        if (!OpenLevelPack(m_levelPak, true))
        {
            Error("Cannot open Pak file " + m_levelPak.m_sPath);
            exportSuccessful = false;
        }
    }

    if (exportSuccessful)
    {
        // Commit changes to the disk.
        _flushall();

        // finally create filelist.xml
        QString levelName = Path::GetFileName(pGameEngine->GetLevelPath());
        ExportFileList(sLevelPath, levelName);

        pDocument->SetLevelExported(true);
    }

    // Always notify that we've finished exporting, whether it was successful or not.
    pEditor->Notify(eNotify_OnExportToGame);
    CrySystemEventBus::Broadcast(&CrySystemEventBus::Events::OnCryEditorEndLevelExport, exportSuccessful);

    if (exportSuccessful)
    {
        // Notify the level system that there's a new level, so that the level info is populated.
        gEnv->pSystem->GetILevelSystem()->Rescan("levels", ILevelSystem::TAG_MAIN);

        CLogFile::WriteLine("Exporting was successful.");
    }

    return exportSuccessful;
}

bool CGameExporter::ExportMap(const char* pszGamePath, bool bSurfaceTexture, EEndian eExportEndian)
{
    GetIEditor()->ShowConsole(true);

    QString ctcFilename;
    ctcFilename = QStringLiteral("%1%2").arg(pszGamePath, QStringLiteral(COMPILED_TERRAIN_TEXTURE_FILE_NAME));

    //Older versions of the code stored the cover.ctc file in the level.pak. This is no longer the case. Just in case we are re-saving an old
    //level remove any old cover.ctc files, that will be overwritten by the resource compiler anyway.
    m_levelPak.m_pakFile.RemoveFile(ctcFilename.toUtf8().data());

    // No need to generate texture if there are no layers or the caller does
    // not demand the texture to be generated
    if (GetIEditor()->GetTerrainManager()->GetLayerCount() == 0 || (!bSurfaceTexture && !GetIEditor()->GetTerrainManager()->GetRGBLayer()->GetNeedExportTexture()))
    {
        CLogFile::WriteLine("Skip exporting terrain texture.");
        return true;
    }

    CLogFile::FormatLine("Exporting data to game (%s)...", pszGamePath);

    ////////////////////////////////////////////////////////////////////////
    // Export the terrain texture
    ////////////////////////////////////////////////////////////////////////

    CLogFile::WriteLine("Exporting terrain texture.");
    GetIEditor()->SetStatusText("Exporting terrain texture (Generating)...");

    // Check dimensions
    CHeightmap* pHeightMap = GetIEditor()->GetHeightmap();
    if (pHeightMap->GetWidth() != pHeightMap->GetHeight() || pHeightMap->GetWidth() % 128)
    {
        assert(pHeightMap->GetWidth() % 128 == 0);
        CLogFile::WriteLine("Can't export a heightmap");
        Error("Can't export a heightmap with dimensions that can't be evenly divided by 128 or that are not square!");
        return false;
    }

    DWORD startTime = GetTickCount();

    gEnv->p3DEngine->CloseTerrainTextureFile();
    pHeightMap->GetTerrainGrid()->InitSectorGrid(pHeightMap->GetTerrainGrid()->GetNumSectors());

    if (!ExportTerrainTexture(ctcFilename.toUtf8().data()))
    {
        return false;
    }
    GetIEditor()->GetTerrainManager()->GetRGBLayer()->SetNeedExportTexture(false);

    CLogFile::WriteLine("Update terrain texture file...");
    GetIEditor()->GetTerrainManager()->GetRGBLayer()->CleanupCache();
    pHeightMap->SetSurfaceTextureSize(m_settings.iExportTexWidth, m_settings.iExportTexWidth);
    pHeightMap->ClearModSectors();

    CLogFile::FormatLine("Terrain Texture Exported in %u seconds.", (GetTickCount() - startTime) / 1000);
    return true;
}



//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportHeightMap(const char* pszGamePath, EEndian eExportEndian)
{
    char szFileOutputPath[_MAX_PATH];

    // export compiled terrain
    CLogFile::WriteLine("Exporting terrain...");
    IEditor* pEditor = GetIEditor();
    pEditor->SetStatusText("Exporting terrain...");

    // remove old files
    sprintf_s(szFileOutputPath, "%s%s", pszGamePath, COMPILED_HEIGHT_MAP_FILE_NAME);
    m_levelPak.m_pakFile.RemoveFile(szFileOutputPath);
    sprintf_s(szFileOutputPath, "%s%s", pszGamePath, COMPILED_VISAREA_MAP_FILE_NAME);
    m_levelPak.m_pakFile.RemoveFile(szFileOutputPath);

    SHotUpdateInfo* pExportInfo = NULL;

    SHotUpdateInfo exportInfo;
    I3DEngine* p3DEngine = pEditor->Get3DEngine();

    if (eExportEndian == GetPlatformEndian()) // skip second export, this data is common for PC and consoles
    {
        std::vector<struct IStatObj*>* pTempBrushTable = NULL;
        std::vector<_smart_ptr<IMaterial>>* pTempMatsTable = NULL;
        std::vector<struct IStatInstGroup*>* pTempVegGroupTable = NULL;

        if (ITerrain* pTerrain = p3DEngine->GetITerrain())
        {
            if (int nSize = pTerrain->GetCompiledDataSize(pExportInfo))
            { // get terrain data from 3dengine and save it into file
                uint8* pData = new uint8[nSize];
                pTerrain->GetCompiledData(pData, nSize, &pTempBrushTable, &pTempMatsTable, &pTempVegGroupTable, eExportEndian, pExportInfo);
                sprintf_s(szFileOutputPath, "%s%s", pszGamePath, COMPILED_HEIGHT_MAP_FILE_NAME);
                CCryMemFile hmapCompiledFile;
                hmapCompiledFile.Write(pData, nSize);
                m_levelPak.m_pakFile.UpdateFile(szFileOutputPath, hmapCompiledFile);
                delete[] pData;
            }
        }

        ////////////////////////////////////////////////////////////////////////
        // Export instance lists of pre-mergedmeshes
        ////////////////////////////////////////////////////////////////////////
        ExportMergedMeshInstanceSectors(pszGamePath, eExportEndian, pTempVegGroupTable);

        // export visareas
        CLogFile::WriteLine("Exporting indoors...");
        pEditor->SetStatusText("Exporting indoors...");
        if (IVisAreaManager* pVisAreaManager = p3DEngine->GetIVisAreaManager())
        {
            if (int nSize = pVisAreaManager->GetCompiledDataSize())
            { // get visareas data from 3dengine and save it into file
                uint8* pData = new uint8[nSize];
                pVisAreaManager->GetCompiledData(pData, nSize, &pTempBrushTable, &pTempMatsTable, &pTempVegGroupTable, eExportEndian);
                sprintf_s(szFileOutputPath, "%s%s", pszGamePath, COMPILED_VISAREA_MAP_FILE_NAME);
                CCryMemFile visareasCompiledFile;
                visareasCompiledFile.Write(pData, nSize);
                m_levelPak.m_pakFile.UpdateFile(szFileOutputPath, visareasCompiledFile);
                delete[] pData;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportMergedMeshInstanceSectors(const char* pszGamePath, EEndian eExportEndian, std::vector<struct IStatInstGroup*>* pVegGroupTable)
{
    char szFileOutputPath[_MAX_PATH];
    DynArray<IMergedMeshesManager::SInstanceSector> sectors;
    DynArray<string> usedMeshes;
    const char* status = "Exporting merged meshes sectors...";

    IEditor* pEditor = GetIEditor();
    I3DEngine* p3DEngine = pEditor->Get3DEngine();
    IMergedMeshesManager* mergedMeshes = p3DEngine->GetIMergedMeshesManager();

    CLogFile::WriteLine(status);
    pEditor->SetStatusText(status);

    // Nuke old directory if present to make sure that it's not present anymore
    sprintf_s(szFileOutputPath, "%s%s", pszGamePath, COMPILED_MERGED_MESHES_BASE_NAME);
    m_levelPak.m_pakFile.RemoveDir(szFileOutputPath);

    mergedMeshes->CompileSectors(sectors, pVegGroupTable);

    // Save out the compiled sector files into seperate files
    for (size_t i = 0; i < sectors.size(); ++i)
    {
        sprintf_s(szFileOutputPath, "%s%s\\%s.dat", pszGamePath, COMPILED_MERGED_MESHES_BASE_NAME, sectors[i].id.c_str());
        m_levelPak.m_pakFile.UpdateFile(szFileOutputPath, &sectors[i].data[0], sectors[i].data.size());
    }

    status = "Exporting merged meshes list...";
    CLogFile::WriteLine(status);
    pEditor->SetStatusText(status);

    mergedMeshes->GetUsedMeshes(usedMeshes);

    std::vector<char> byteArray;
    for (int i = 0; i < usedMeshes.size(); ++i)
    {
        byteArray.reserve(usedMeshes[i].size() + byteArray.size() + 1);
        byteArray.insert(byteArray.end(), usedMeshes[i].begin(), usedMeshes[i].end());
        byteArray.insert(byteArray.end(), '\n');
    }

    if (byteArray.size())
    {
        sprintf_s(szFileOutputPath, "%s%s\\%s", pszGamePath, COMPILED_MERGED_MESHES_BASE_NAME, COMPILED_MERGED_MESHES_LIST);
        m_levelPak.m_pakFile.UpdateFile(szFileOutputPath, &byteArray[0], byteArray.size());
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportOcclusionMesh(const char* pszGamePath)
{
    IEditor* pEditor = GetIEditor();
    pEditor->SetStatusText(QObject::tr("including Occluder Mesh \"occluder.ocm\" if available"));

    char resolvedLevelPath[AZ_MAX_PATH_LEN] = { 0 };
    AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(pszGamePath, resolvedLevelPath, AZ_MAX_PATH_LEN);
    QString levelDataFile = QString(resolvedLevelPath) + "occluder.ocm";
    QFile FileIn(levelDataFile);
    if (FileIn.open(QFile::ReadOnly))
    {
        CMemoryBlock Temp;
        const size_t Size   =   FileIn.size();
        Temp.Allocate(Size);
        FileIn.read(reinterpret_cast<char*>(Temp.GetBuffer()), Size);
        FileIn.close();
        CCryMemFile FileOut;
        FileOut.Write(Temp.GetBuffer(), Size);
        m_levelPak.m_pakFile.UpdateFile(levelDataFile.toUtf8().data(), FileOut);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportLevelData(const QString& path, bool bExportMission)
{
    IEditor* pEditor = GetIEditor();
    pEditor->SetStatusText(QObject::tr("Exporting LevelData.xml..."));

    char versionString[256];
    pEditor->GetFileVersion().ToString(versionString);

    XmlNodeRef root = XmlHelpers::CreateXmlNode("LevelData");
    root->setAttr("SandboxVersion", versionString);
    XmlNodeRef rootAction = XmlHelpers::CreateXmlNode("LevelDataAction");
    rootAction->setAttr("SandboxVersion", versionString);

    ExportMapInfo(root);

    //////////////////////////////////////////////////////////////////////////
    /// Export vegetation objects.
    XmlNodeRef vegetationNode = root->newChild("Vegetation");
    CVegetationMap* pVegetationMap = pEditor->GetVegetationMap();
    if (pVegetationMap)
    {
        pVegetationMap->SerializeObjects(vegetationNode);
    }

    //////////////////////////////////////////////////////////////////////////
    // Export materials.
    ExportMaterials(root, path);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Export particle manager.
    pEditor->GetParticleManager()->Export(root);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Export Level GameTokens.
    ExportGameTokens(root, path);
    //////////////////////////////////////////////////////////////////////////

    CCryEditDoc* pDocument = pEditor->GetDocument();
    CMission* pCurrentMission = 0;

    if (bExportMission)
    {
        pCurrentMission = pDocument->GetCurrentMission();
        // Save contents of current mission.
    }

    //////////////////////////////////////////////////////////////////////////
    // Export missions tag.
    //////////////////////////////////////////////////////////////////////////
    XmlNodeRef missionsNode = rootAction->newChild("Missions");
    QString missionFileName;
    QString currentMissionFileName;
    I3DEngine* p3DEngine = pEditor->Get3DEngine();
    for (int i = 0; i < pDocument->GetMissionCount(); i++)
    {
        CMission* pMission = pDocument->GetMission(i);

        QString name = pMission->GetName();
        name.replace(' ', '_');
        missionFileName = QStringLiteral("Mission_%1.xml").arg(name);

        XmlNodeRef missionDescNode = missionsNode->newChild("Mission");
        missionDescNode->setAttr("Name", pMission->GetName().toUtf8().data());
        missionDescNode->setAttr("File", missionFileName.toUtf8().data());
        missionDescNode->setAttr("CGFCount", p3DEngine->GetLoadedObjectCount());

        int nProgressBarRange = m_numExportedMaterials / 10 + p3DEngine->GetLoadedObjectCount();
        missionDescNode->setAttr("ProgressBarRange", nProgressBarRange);

        if (pMission == pCurrentMission)
        {
            currentMissionFileName = missionFileName;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Save Level Data XML
    //////////////////////////////////////////////////////////////////////////
    QString levelDataFile = path + "LevelData.xml";
    XmlString xmlData = root->getXML();
    CCryMemFile file;
    file.Write(xmlData.c_str(), xmlData.length());
    m_levelPak.m_pakFile.UpdateFile(levelDataFile.toUtf8().data(), file);

    QString levelDataActionFile = path + "LevelDataAction.xml";
    XmlString xmlDataAction = rootAction->getXML();
    CCryMemFile fileAction;
    fileAction.Write(xmlDataAction.c_str(), xmlDataAction.length());
    m_levelPak.m_pakFile.UpdateFile(levelDataActionFile.toUtf8().data(), fileAction);

    if (bExportMission)
    {
        XmlNodeRef objectsNode = NULL;
        //////////////////////////////////////////////////////////////////////////
        // Export current mission file.
        //////////////////////////////////////////////////////////////////////////
        XmlNodeRef missionNode = rootAction->createNode("Mission");
        pCurrentMission->Export(missionNode, objectsNode);

        missionNode->setAttr("CGFCount", p3DEngine->GetLoadedObjectCount());

        //if (!CFileUtil::OverwriteFile( path+currentMissionFileName ))
        //          return;

        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        bool savedEntities = false;
        EBUS_EVENT_RESULT(savedEntities, AzToolsFramework::EditorEntityContextRequestBus, SaveToStreamForGame, entitySaveStream, AZ::DataStream::ST_XML);
        if (savedEntities)
        {
            QString entitiesFile;
            entitiesFile = QStringLiteral("%1%2.entities_xml").arg(path, pCurrentMission ? pCurrentMission->GetName() : "");
            m_levelPak.m_pakFile.UpdateFile(entitiesFile.toUtf8().data(), entitySaveBuffer.begin(), entitySaveBuffer.size());
        }

        _smart_ptr<IXmlStringData> pXmlStrData = missionNode->getXMLData(5000000);

        CCryMemFile fileMission;
        fileMission.Write(pXmlStrData->GetString(), pXmlStrData->GetStringLength());
        m_levelPak.m_pakFile.UpdateFile((path + currentMissionFileName).toUtf8().data(), fileMission);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportLevelInfo(const QString& path)
{
    //////////////////////////////////////////////////////////////////////////
    // Export short level info xml.
    //////////////////////////////////////////////////////////////////////////
    IEditor* pEditor = GetIEditor();
    XmlNodeRef root = XmlHelpers::CreateXmlNode("LevelInfo");
    char versionString[256];
    pEditor->GetFileVersion().ToString(versionString);
    root->setAttr("SandboxVersion", versionString);

    QString levelName = pEditor->GetGameEngine()->GetLevelPath();
    root->setAttr("Name", levelName.toUtf8().data());
    root->setAttr("HeightmapSize", pEditor->GetHeightmap()->GetWidth());

    if (gEnv->p3DEngine->GetITerrain())
    {
        int compiledDataSize = gEnv->p3DEngine->GetITerrain()->GetCompiledDataSize();
        byte* pInfo = new byte[compiledDataSize];
        gEnv->p3DEngine->GetITerrain()->GetCompiledData(pInfo, compiledDataSize, 0, 0, 0, false);
        STerrainChunkHeader* pHeader = (STerrainChunkHeader*)pInfo;
        XmlNodeRef terrainInfo = root->newChild("TerrainInfo");
        int heightmapSize = GetIEditor()->GetHeightmap()->GetWidth();
        terrainInfo->setAttr("HeightmapSize", heightmapSize);
        terrainInfo->setAttr("UnitSize", pHeader->TerrainInfo.nUnitSize_InMeters);
        terrainInfo->setAttr("SectorSize", pHeader->TerrainInfo.nSectorSize_InMeters);
        terrainInfo->setAttr("SectorsTableSize", pHeader->TerrainInfo.nSectorsTableSize_InSectors);
        terrainInfo->setAttr("HeightmapZRatio", pHeader->TerrainInfo.fHeightmapZRatio);
        terrainInfo->setAttr("OceanWaterLevel", pHeader->TerrainInfo.fOceanWaterLevel);

        delete[] pInfo;
    }

    // Save all missions in this level.
    XmlNodeRef missionsNode = root->newChild("Missions");
    int numMissions = pEditor->GetDocument()->GetMissionCount();
    for (int i = 0; i < numMissions; i++)
    {
        CMission* pMission = pEditor->GetDocument()->GetMission(i);
        XmlNodeRef missionNode = missionsNode->newChild("Mission");
        missionNode->setAttr("Name", pMission->GetName().toUtf8().data());
        missionNode->setAttr("Description", pMission->GetDescription().toUtf8().data());
    }


    //////////////////////////////////////////////////////////////////////////
    // Save LevelInfo file.
    //////////////////////////////////////////////////////////////////////////
    QString filename = path + "LevelInfo.xml";
    XmlString xmlData = root->getXML();

    CCryMemFile file;
    file.Write(xmlData.c_str(), xmlData.length());
    m_levelPak.m_pakFile.UpdateFile(filename.toUtf8().data(), file);
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportMapInfo(XmlNodeRef& node)
{
    XmlNodeRef info = node->newChild("LevelInfo");

    IEditor* pEditor = GetIEditor();
    info->setAttr("Name", QFileInfo(pEditor->GetDocument()->GetTitle()).completeBaseName());

    CHeightmap* heightmap = pEditor->GetHeightmap();
    if (heightmap)
    {
        info->setAttr("HeightmapSize", heightmap->GetWidth());
        info->setAttr("HeightmapUnitSize", heightmap->GetUnitSize());
        info->setAttr("HeightmapMaxHeight", heightmap->GetMaxHeight());
        info->setAttr("WaterLevel", heightmap->GetOceanLevel());

        SSectorInfo sectorInfo;
        heightmap->GetSectorsInfo(sectorInfo);
        int nTerrainSectorSizeInMeters = sectorInfo.sectorSize;
        info->setAttr("TerrainSectorSizeInMeters", nTerrainSectorSizeInMeters);
    }

    // Serialize surface types.
    CXmlArchive xmlAr;
    xmlAr.bLoading = false;
    xmlAr.root = node;
    GetIEditor()->GetTerrainManager()->SerializeSurfaceTypes(xmlAr);

    GetIEditor()->GetObjectManager()->GetPhysicsManager()->SerializeCollisionClasses(xmlAr);

    pEditor->GetObjectManager()->GetLayersManager()->ExportLayerSwitches(node);
}

bool CGameExporter::ExportTerrainTexture(const char* ctcFilename)
{
    MacroTextureExporter exporter(GetIEditor()->GetTerrainManager()->GetRGBLayer());
    exporter.SetMaximumTotalSize(m_settings.iExportTexWidth);

    CWaitProgress progress("Generating Terrain Texture");
    exporter.SetProgressCallback([&progress](uint32 percent)
        {
            progress.Step(percent);
        });

    return exporter.Export(ctcFilename);
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportAINavigationData(const QString& path)
{
    if (!GetIEditor()->GetSystem()->GetAISystem())
    {
        return;
    }

    GetIEditor()->SetStatusText(QObject::tr("Exporting AI Navigation Data..."));

    QString fileNameNavigation = path + QStringLiteral("mnmnav%1.bai").arg(GetIEditor()->GetDocument()->GetCurrentMission()->GetName());

    char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
    AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(fileNameNavigation.toUtf8().data(), resolvedPath, AZ_MAX_PATH_LEN);
    QFile file(resolvedPath);
    if (file.open(QFile::ReadOnly))
    {
        CMemoryBlock mem;
        mem.Allocate(file.size());
        file.read(reinterpret_cast<char*>(mem.GetBuffer()), file.size());
        file.close();

        if (!m_levelPak.m_pakFile.UpdateFile(fileNameNavigation.toUtf8().data(), mem))
        {
            Warning("Failed to update pak file with %s", fileNameNavigation.toUtf8().data());
        }
        else
        {
            QFile::remove(resolvedPath);
        }
    }

    CLogFile::WriteLine("Exporting Navigation data done.");
}

void CGameExporter::ExportGameData(const QString& path)
{
    IGame* pGame = GetIEditor()->GetGame();
    if (pGame == NULL)
    {
        return;
    }

    GetIEditor()->SetStatusText(QObject::tr("Exporting Game Level data..."));

    CWaitProgress wait("Saving game level data");

    const QString& missionName = GetIEditor()->GetDocument()->GetCurrentMission()->GetName();

    const IGame::ExportFilesInfo exportedFiles = pGame->ExportLevelData(m_levelPath.toUtf8().data(), missionName.toUtf8().data());

    // Update pak files for every exported file (we expect the same order on the game side!)
    char fileName[512];
    for (uint32 fileIdx = 0; fileIdx < exportedFiles.GetFileCount(); ++fileIdx)
    {
        IGame::ExportFilesInfo::GetNameForFile(exportedFiles.GetBaseFileName(), fileIdx, fileName, sizeof(fileName));
        char resolvedFilePath[AZ_MAX_PATH_LEN] = { 0 };
        AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(fileName, resolvedFilePath, AZ_MAX_PATH_LEN);
        QFile file(resolvedFilePath);
        if (file.open(QFile::ReadOnly))
        {
            CMemoryBlock mem;
            mem.Allocate(file.size());
            file.read(reinterpret_cast<char*>(mem.GetBuffer()), file.size());
            file.close();

            if (!m_levelPak.m_pakFile.UpdateFile(fileName, mem))
            {
                Warning("Failed to update pak file with %s", fileName);
            }
            else
            {
                QFile::remove(resolvedFilePath);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
QString CGameExporter::ExportAIGraph(const QString& path)
{
    // (MATT) Only updates _some_ files in the AI but is only place that copies all bai files present into the PAK! {2008/08/12}

    IAISystem* pAISystem = GetISystem()->GetAISystem();
    if (!pAISystem)
    {
        return "No AI System available";
    }

    // AI file will be generated individually
    //  pEditor->GetGameEngine()->GenerateAiTriangulation();

    IEditor* pEditor = GetIEditor();
    pEditor->SetStatusText(QObject::tr("Exporting AI Graph..."));

    QString szMission = pEditor->GetDocument()->GetCurrentMission()->GetName();
    QString fileNameNav = path + QStringLiteral("net%1.bai").arg(szMission);
    QString fileNameVerts = path + QStringLiteral("verts%1.bai").arg(szMission);
    QString fileNameVolume = path + QStringLiteral("v3d%1.bai").arg(szMission);
    QString fileNameAreas = path + QStringLiteral("areas%1.bai").arg(szMission);
    QString fileNameFlight = path + QStringLiteral("fnav%1.bai").arg(szMission);
    QString fileNameRoads = path + QStringLiteral("roadnav%1.bai").arg(szMission);
    QString fileNameWaypoint3DSurface = path + QStringLiteral("waypt3Dsfc%1.bai").arg(szMission); // (MATT) This is currently unused {2008/08/07}

    QString result;

    CGraph* pGraph = pEditor->GetGameEngine()->GetNavigation()->GetGraph();
    if (pGraph)
    {
        // (MATT) On it's own, this function isn't adequate {2008/08/08}
        //pEditor->GetGameEngine()->ExportAiData(fileNameNav, fileNameAreas, fileNameRoads, fileNameWaypoint3DSurface);

        // (MATT) We don't check the result of this validation function - nor am I confident in it {2008/08/07}
        pGraph->Validate("Before updating pak", true);

        // Read these files back and put them to Pak file.
        char resolvedFilename[AZ_MAX_PATH_LEN] = { 0 };
        AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(fileNameNav.toUtf8().data(), resolvedFilename, AZ_MAX_PATH_LEN);
        QFile file(resolvedFilename);
        if (file.open(QFile::ReadOnly))
        {
            CMemoryBlock mem;
            mem.Allocate(file.size());
            file.read(reinterpret_cast<char*>(mem.GetBuffer()), file.size());
            file.close();
            if (false == m_levelPak.m_pakFile.UpdateFile(fileNameNav.toUtf8().data(), mem))
            {
                result += QString("Failed to update pak file with ") + fileNameNav + "\n";
            }
            else
            {
                QFile::remove(resolvedFilename);
            }
        }
        AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(fileNameVerts.toUtf8().data(), resolvedFilename, AZ_MAX_PATH_LEN);
        file.setFileName(resolvedFilename);
        if (file.open(QFile::ReadOnly))
        {
            CMemoryBlock mem;
            mem.Allocate(file.size());
            file.read(reinterpret_cast<char*>(mem.GetBuffer()), file.size());
            file.close();
            if (false == m_levelPak.m_pakFile.UpdateFile(fileNameVerts.toUtf8().data(), mem))
            {
                result += QString("Failed to update pak file with ") + fileNameVerts + "\n";
            }
            else
            {
                QFile::remove(resolvedFilename);
            }
        }
        AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(fileNameVolume.toUtf8().data(), resolvedFilename, AZ_MAX_PATH_LEN);
        file.setFileName(resolvedFilename);
        if (file.open(QFile::ReadOnly))
        {
            CMemoryBlock mem;
            mem.Allocate(file.size());
            file.read(reinterpret_cast<char*>(mem.GetBuffer()), file.size());
            file.close();
            if (false == m_levelPak.m_pakFile.UpdateFile(fileNameVolume.toUtf8().data(), mem))
            {
                result += QString("Failed to update pak file with ") + fileNameVolume + "\n";
            }
            else
            {
                QFile::remove(resolvedFilename);
            }
        }
        pEditor->GetGameEngine()->ExportAiData(0, fileNameAreas.toUtf8().data(), 0, 0, 0, 0);
        AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(fileNameAreas.toUtf8().data(), resolvedFilename, AZ_MAX_PATH_LEN);
        file.setFileName(resolvedFilename);
        if (file.open(QFile::ReadOnly))
        {
            CMemoryBlock mem;
            mem.Allocate(file.size());
            file.read(reinterpret_cast<char*>(mem.GetBuffer()), file.size());
            file.close();
            if (false == m_levelPak.m_pakFile.UpdateFile(fileNameAreas.toUtf8().data(), mem))
            {
                result += QString("Failed to update pak file with ") + fileNameAreas + "\n";
            }
            else
            {
                QFile::remove(resolvedFilename);
            }
        }
        AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(fileNameFlight.toUtf8().data(), resolvedFilename, AZ_MAX_PATH_LEN);
        file.setFileName(resolvedFilename);
        if (file.open(QFile::ReadOnly))
        {
            CMemoryBlock mem;
            mem.Allocate(file.size());
            file.read(reinterpret_cast<char*>(mem.GetBuffer()), file.size());
            file.close();
            if (false == m_levelPak.m_pakFile.UpdateFile(fileNameFlight.toUtf8().data(), mem))
            {
                result += QString("Failed to update pak file with ") + fileNameFlight + "\n";
            }
            else
            {
                QFile::remove(resolvedFilename);
            }
        }
        AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(fileNameRoads.toUtf8().data(), resolvedFilename, AZ_MAX_PATH_LEN);
        file.setFileName(resolvedFilename);
        if (file.open(QFile::ReadOnly))
        {
            CMemoryBlock mem;
            mem.Allocate(file.size());
            file.read(reinterpret_cast<char*>(mem.GetBuffer()), file.size());
            file.close();
            if (false == m_levelPak.m_pakFile.UpdateFile(fileNameRoads.toUtf8().data(), mem))
            {
                result += QString("Failed to update pak file with ") + fileNameRoads + "\n";
            }
            else
            {
                QFile::remove(resolvedFilename);
            }
        }
        AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(fileNameWaypoint3DSurface.toUtf8().data(), resolvedFilename, AZ_MAX_PATH_LEN);
        file.setFileName(resolvedFilename);
        if (file.open(QFile::ReadOnly))
        {
            CMemoryBlock mem;
            mem.Allocate(file.size());
            file.read(reinterpret_cast<char*>(mem.GetBuffer()), file.size());
            file.close();
            if (false == m_levelPak.m_pakFile.UpdateFile(fileNameWaypoint3DSurface.toUtf8().data(), mem))
            {
                result += QString("Failed to update pak file with ") + fileNameWaypoint3DSurface + "\n";
            }
            else
            {
                QFile::remove(resolvedFilename);
            }
        }
    }

    DynArray<CryStringT<char> > fileNames;
    pAISystem->GetINavigation()->GetVolumeRegionFiles(path.toUtf8().data(), szMission.toUtf8().data(), fileNames);

    for (unsigned i = 0; i < fileNames.size(); ++i)
    {
        const string& fileName = fileNames[i];

        char resolvedFilename[AZ_MAX_PATH_LEN] = { 0 };
        AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(fileName.c_str(), resolvedFilename, AZ_MAX_PATH_LEN);
        QFile file(resolvedFilename);
        if (file.open(QFile::ReadOnly))
        {
            CMemoryBlock mem;
            mem.Allocate(file.size());
            file.read(reinterpret_cast<char*>(mem.GetBuffer()), file.size());
            file.close();
            if (false == m_levelPak.m_pakFile.UpdateFile(fileName, mem))
            {
                result += QString("Failed to update pak file with ") + fileName.c_str() + "\n";
            }
            else
            {
                QFile::remove(resolvedFilename);
            }
        }
    }

    CLogFile::WriteLine("Exporting AI Graph done.");
    return result;
}


QString CGameExporter::ExportAICoverSurfaces(const QString& path)
{
    if (!GetISystem()->GetAISystem())
    {
        return "No AI System available";
    }

    IEditor* pEditor = GetIEditor();
    pEditor->SetStatusText(QObject::tr("Exporting AI Cover Surfaces..."));

    QDir szLevel = QDir(path);

    QString szMission = pEditor->GetDocument()->GetCurrentMission()->GetName();
    QString fileNameCoverSurfaces = path + QStringLiteral("cover%1.bai").arg(szMission);

    QString result;

    char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
    AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(fileNameCoverSurfaces.toUtf8().data(), resolvedPath, AZ_MAX_PATH_LEN);
    QFile file(resolvedPath);
    if (file.open(QFile::ReadOnly))
    {
        CMemoryBlock mem;
        mem.Allocate(file.size());
        file.read(reinterpret_cast<char*>(mem.GetBuffer()), file.size());
        file.close();

        if (!m_levelPak.m_pakFile.UpdateFile(fileNameCoverSurfaces.toUtf8().data(), mem))
        {
            result = QString("Failed to update pak file with ") + fileNameCoverSurfaces + "\n";
        }
        else
        {
            QFile::remove(resolvedPath);
        }
    }

    CLogFile::WriteLine("Exporting AI Cover Surfaces done.");
    return result;
}

//////////////////////////////////////////////////////////////////////////
QString CGameExporter::ExportAI(const QString& path, bool coverSurfaces)
{
    if (!GetISystem()->GetAISystem())
    {
        return "No AI System available!";
    }

    QString result = ExportAIGraph(path);

    ExportAICoverSurfaces(path);

    ExportAINavigationData(path);

    return result;
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportBrushes(const QString& path)
{
    GetIEditor()->SetStatusText(QObject::tr("Exporting Brushes..."));
    GetIEditor()->Notify(eNotify_OnExportBrushes);
}

void CGameExporter::EncryptPakFile(QString& rPakFilename)
{
    // Disabled, for now. (inside EncryptPakFile)
#if 0
    CString args;

    args.Format("/zip_encrypt=1 \"%s\"", rPakFilename.GetBuffer());
    Log("Encrypting PAK: %s", rPakFilename.GetBuffer());
    // Will need porting to QProcess when EncryptPakFile is enabled again
    ::ShellExecute(AfxGetMainWnd()->GetSafeHwnd(), "open", "Bin32\\rc\\rc.exe", args, "", SW_HIDE);
#endif
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportMaterials(XmlNodeRef& levelDataNode, const QString& path)
{
    //////////////////////////////////////////////////////////////////////////
    // Export materials manager.
    CMaterialManager* pManager = GetIEditor()->GetMaterialManager();
    pManager->Export(levelDataNode);

    QString filename = Path::Make(path, MATERIAL_LEVEL_LIBRARY_FILE);

    bool bHaveItems = true;

    int numMtls = 0;

    XmlNodeRef nodeMaterials = XmlHelpers::CreateXmlNode("MaterialsLibrary");
    // Export Materials local level library.
    for (int i = 0; i < pManager->GetLibraryCount(); i++)
    {
        XmlNodeRef nodeLib = nodeMaterials->newChild("Library");
        CMaterialLibrary* pLib = (CMaterialLibrary*)pManager->GetLibrary(i);
        if (pLib->GetItemCount() > 0)
        {
            bHaveItems = false;
            // Export this library.
            numMtls += pManager->ExportLib(pLib, nodeLib);
        }
    }
    if (!bHaveItems)
    {
        XmlString xmlData = nodeMaterials->getXML();

        CCryMemFile file;
        file.Write(xmlData.c_str(), xmlData.length());
        m_levelPak.m_pakFile.UpdateFile(filename.toUtf8().data(), file);
    }
    else
    {
        m_levelPak.m_pakFile.RemoveFile(filename.toUtf8().data());
    }
    m_numExportedMaterials = numMtls;
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportLevelLensFlares(const QString& path)
{
    GetIEditor()->SetStatusText(QObject::tr("Exporting Lens Flares..."));
    std::vector<CBaseObject*> objects;
    GetIEditor()->GetObjectManager()->FindObjectsOfType(&CEntityObject::staticMetaObject, objects);
    std::set<QString> flareNameSet;
    for (int i = 0, iObjectSize(objects.size()); i < iObjectSize; ++i)
    {
        CEntityObject* pEntity = (CEntityObject*)objects[i];
        if (!pEntity->IsLight())
        {
            continue;
        }
        QString flareName = pEntity->GetEntityPropertyString(CEntityObject::s_LensFlarePropertyName);
        if (flareName.isEmpty() || flareName == "@root")
        {
            continue;
        }
        flareNameSet.insert(flareName);
    }

    XmlNodeRef pRootNode = GetIEditor()->GetSystem()->CreateXmlNode("LensFlareList");
    pRootNode->setAttr("Version", FLARE_EXPORT_FILE_VERSION);

    CLensFlareManager* pLensManager = GetIEditor()->GetLensFlareManager();

    if (CLensFlareLibrary* pLevelLib = (CLensFlareLibrary*)pLensManager->GetLevelLibrary())
    {
        for (int i = 0; i < pLevelLib->GetItemCount(); i++)
        {
            CLensFlareItem* pItem = (CLensFlareItem*)pLevelLib->GetItem(i);

            if (flareNameSet.find(pItem->GetFullName()) == flareNameSet.end())
            {
                continue;
            }

            CBaseLibraryItem::SerializeContext ctx(pItem->CreateXmlData(), false);
            pRootNode->addChild(ctx.node);
            pItem->Serialize(ctx);
            flareNameSet.erase(pItem->GetFullName());
        }
    }

    std::set<QString>::iterator iFlareNameSet = flareNameSet.begin();
    for (; iFlareNameSet != flareNameSet.end(); ++iFlareNameSet)
    {
        QString flareName = *iFlareNameSet;
        XmlNodeRef pFlareNode = GetIEditor()->GetSystem()->CreateXmlNode("LensFlare");
        pFlareNode->setAttr("name", flareName.toUtf8().data());
        pRootNode->addChild(pFlareNode);
    }

    CCryMemFile lensFlareNames;
    lensFlareNames.Write(pRootNode->getXMLData()->GetString(), pRootNode->getXMLData()->GetStringLength());

    QString exportPathName = path + FLARE_EXPORT_FILE;
    m_levelPak.m_pakFile.UpdateFile(exportPathName.toUtf8().data(), lensFlareNames);
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportLevelResourceList(const QString& path)
{
    IResourceList* pResList = gEnv->pCryPak->GetResourceList(ICryPak::RFOM_Level);

    // Write resource list to file.
    CCryMemFile memFile;
    for (const char* filename = pResList->GetFirst(); filename; filename = pResList->GetNext())
    {
        memFile.Write(filename, strlen(filename));
        memFile.Write("\n", 1);
    }

    QString resFile = Path::Make(path, RESOURCE_LIST_FILE);

    m_levelPak.m_pakFile.UpdateFile(resFile.toUtf8().data(), memFile, true);
}

//////////////////////////////////////////////////////////////////////////

void CGameExporter::GatherLayerResourceList_r(CObjectLayer* pLayer, CUsedResources& resources)
{
    GetIEditor()->GetObjectManager()->GatherUsedResources(resources, pLayer);

    for (int i = 0; i < pLayer->GetChildCount(); i++)
    {
        CObjectLayer* pChildLayer = pLayer->GetChild(i);
        GatherLayerResourceList_r(pChildLayer, resources);
    }
}

void CGameExporter::ExportLevelPerLayerResourceList(const QString& path)
{
    // Write per layer resource list to file.
    CCryMemFile memFile;

    std::vector<CObjectLayer*> layers;
    GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers(layers);
    for (size_t i = 0; i < layers.size(); ++i)
    {
        CObjectLayer* pLayer = layers[i];

        // Only export topmost layers, and make sure they are flagged for exporting
        if (pLayer->GetParent() || !pLayer->IsExporLayerPak())
        {
            continue;
        }

        CUsedResources resources;
        GatherLayerResourceList_r(pLayer, resources);

        QString acLayerName = pLayer->GetName();

        for (CUsedResources::TResourceFiles::const_iterator it = resources.files.begin(); it != resources.files.end(); it++)
        {
            QString filePath = Path::MakeGamePath(*it).toLower();

            memFile.Write(acLayerName.toUtf8().data(), acLayerName.toUtf8().length());
            memFile.Write(";", 1);
            memFile.Write(filePath.toUtf8().data(), filePath.toUtf8().length());
            memFile.Write("\n", 1);
        }
    }

    QString resFile = Path::Make(path, PERLAYER_RESOURCE_LIST_FILE);

    m_levelPak.m_pakFile.UpdateFile(resFile.toUtf8().data(), memFile, true);
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportLevelShaderCache(const QString& path)
{
    QString buf;
    GetIEditor()->GetDocument()->GetShaderCache()->SaveBuffer(buf);
    CCryMemFile memFile;
    memFile.Write(buf.toUtf8().data(), buf.toUtf8().length());

    QString filename = Path::Make(path, SHADER_LIST_FILE);
    m_levelPak.m_pakFile.UpdateFile(filename.toUtf8().data(), memFile, true);
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportGameTokens(XmlNodeRef& levelDataNode, const QString& path)
{
    // Export game tokens
    CGameTokenManager* pGTM = GetIEditor()->GetGameTokenManager();
    // write only needed libs for this levels
    pGTM->Export(levelDataNode);

    QString gtPath = Path::AddPathSlash(path) + "GameTokens";
    QString filename = Path::Make(gtPath, GAMETOKENS_LEVEL_LIBRARY_FILE);

    bool bEmptyLevelLib = true;
    XmlNodeRef nodeLib = XmlHelpers::CreateXmlNode("GameTokensLibrary");
    // Export GameTokens local level library.
    for (int i = 0; i < pGTM->GetLibraryCount(); i++)
    {
        IDataBaseLibrary* pLib = pGTM->GetLibrary(i);
        if (pLib->IsLevelLibrary())
        {
            if (pLib->GetItemCount() > 0)
            {
                bEmptyLevelLib = false;
                // Export this library.
                pLib->Serialize(nodeLib, false);
                nodeLib->setAttr("LevelName", GetIEditor()->GetGameEngine()->GetLevelPath().toUtf8().data()); // we set the Name from "Level" to the realname
            }
        }
        else
        {
            // AlexL: Not sure if
            // (1) we should store all libs into the PAK file or
            // (2) just use references to the game global Game/Libs directory.
            // Currently we use (1)
            if (pLib->GetItemCount() > 0)
            {
                // Export this library to pak file.
                XmlNodeRef gtNodeLib = XmlHelpers::CreateXmlNode("GameTokensLibrary");
                pLib->Serialize(gtNodeLib, false);
                CCryMemFile file;
                XmlString xmlData = gtNodeLib->getXML();
                file.Write(xmlData.c_str(), xmlData.length());
                QString gtfilename = Path::Make(gtPath, Path::GetFile(pLib->GetFilename()));
                m_levelPak.m_pakFile.UpdateFile(gtfilename.toUtf8().data(), file);
            }
        }
    }
    if (!bEmptyLevelLib)
    {
        XmlString xmlData = nodeLib->getXML();

        CCryMemFile file;
        file.Write(xmlData.c_str(), xmlData.length());
        m_levelPak.m_pakFile.UpdateFile(filename.toUtf8().data(), file);
    }
    else
    {
        m_levelPak.m_pakFile.RemoveFile(filename.toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportFileList(const QString& path, const QString& levelName)
{
    // process the folder of the specified map name, producing a filelist.xml file
    //  that can later be used for map downloads
    struct _finddata_t fileinfo;
    intptr_t handle;
    string newpath;

    QString filename = levelName;
    string mapname = (filename + ".dds").toUtf8().data();
    string metaname = (filename + ".xml").toUtf8().data();

    XmlNodeRef rootNode = gEnv->pSystem->CreateXmlNode("download");
    rootNode->setAttr("name", filename.toUtf8().data());
    rootNode->setAttr("type", "Map");
    XmlNodeRef indexNode = rootNode->newChild("index");
    if (indexNode)
    {
        indexNode->setAttr("src", "filelist.xml");
        indexNode->setAttr("dest", "filelist.xml");
    }
    XmlNodeRef filesNode = rootNode->newChild("files");
    if (filesNode)
    {
        newpath = GetIEditor()->GetGameEngine()->GetLevelPath().toUtf8().data();
        newpath += "/*.*";
        handle = gEnv->pCryPak->FindFirst (newpath.c_str(), &fileinfo);
        if (handle == -1)
        {
            return;
        }
        do
        {
            // ignore "." and ".."
            if (fileinfo.name[0] == '.')
            {
                continue;
            }
            // do we need any files from sub directories?
            if (fileinfo.attrib & _A_SUBDIR)
            {
                continue;
            }

            // only need the following files for multiplayer downloads
            if (!_stricmp(fileinfo.name, GetLevelPakFilename())
                || !_stricmp(fileinfo.name, mapname.c_str())
                || !_stricmp(fileinfo.name, metaname.c_str()))
            {
                XmlNodeRef newFileNode = filesNode->newChild("file");
                if (newFileNode)
                {
                    // TEMP: this is just for testing. src probably needs to be blank.
                    //      string src = "http://server41/updater/";
                    //      src += m_levelName;
                    //      src += "/";
                    //      src += fileinfo.name;
                    newFileNode->setAttr("src", fileinfo.name);
                    newFileNode->setAttr("dest", fileinfo.name);
                    newFileNode->setAttr("size", fileinfo.size);

                    unsigned char md5[16];
                    string filename = GetIEditor()->GetGameEngine()->GetLevelPath().toUtf8().data();
                    filename += "/";
                    filename += fileinfo.name;
                    if (gEnv->pCryPak->ComputeMD5(filename, md5))
                    {
                        char md5string[33];
                        sprintf_s(md5string, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                            md5[0], md5[1], md5[2], md5[3],
                            md5[4], md5[5], md5[6], md5[7],
                            md5[8], md5[9], md5[10], md5[11],
                            md5[12], md5[13], md5[14], md5[15]);
                        newFileNode->setAttr("md5", md5string);
                    }
                    else
                    {
                        newFileNode->setAttr("md5", "");
                    }
                }
            }
        } while (gEnv->pCryPak->FindNext(handle, &fileinfo) != -1);

        gEnv->pCryPak->FindClose (handle);
    }

    // save filelist.xml
    newpath = path.toUtf8().data();
    newpath += "/filelist.xml";
    rootNode->saveToFile(newpath.c_str());
}

void CGameExporter::Error(const QString& error)
{
    if (m_bAutoExportMode)
    {
        CLogFile::WriteLine((QString("Export failed! ") + error).toUtf8().data());
    }
    else
    {
        Warning((QString("Export failed! ") + error).toUtf8().data());
    }
}


bool CGameExporter::OpenLevelPack(SLevelPakHelper& lphelper, bool bCryPak)
{
    bool bRet = false;

    assert(lphelper.m_bPakOpened == false);
    assert(lphelper.m_bPakOpenedCryPak == false);

    if (bCryPak)
    {
        assert(!lphelper.m_sPath.isEmpty());
        bRet = gEnv->pCryPak->OpenPack(lphelper.m_sPath.toUtf8().data());
        assert(bRet);
        lphelper.m_bPakOpenedCryPak = true;
    }
    else
    {
        bRet = lphelper.m_pakFile.Open(lphelper.m_sPath.toUtf8().data());
        assert(bRet);
        lphelper.m_bPakOpened = true;
    }
    return bRet;
}


bool CGameExporter::CloseLevelPack(SLevelPakHelper& lphelper, bool bCryPak)
{
    bool bRet = false;

    if (bCryPak)
    {
        assert(lphelper.m_bPakOpenedCryPak == true);
        assert(!lphelper.m_sPath.isEmpty());
        bRet = gEnv->pCryPak->ClosePack(lphelper.m_sPath.toUtf8().data());
        assert(bRet);
        lphelper.m_bPakOpenedCryPak = false;
    }
    else
    {
        assert(lphelper.m_bPakOpened == true);
        lphelper.m_pakFile.Close();
        bRet = true;
        lphelper.m_bPakOpened = false;
    }

    assert(lphelper.m_bPakOpened == false);
    assert(lphelper.m_bPakOpenedCryPak == false);
    return bRet;
}
