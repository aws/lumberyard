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
#include "IBackgroundTaskManager.h"
#include "LiveCreate/EditorLiveCreateHostInfo.h"
#include "LiveCreate/EditorLiveCreateManager.h"
#include "LiveCreate/EditorLiveCreateAreaSync.h"
#include "LiveCreate/EditorLiveCreateFileSync.h"
#include "LiveCreate/EditorLiveCreate.h"
#include "Objects/BaseObject.h"
#include "Objects/EntityObject.h"
#include "Objects/ObjectLayer.h"
#include "Objects/BrushObject.h"
#include "Objects/RoadObject.h"
#include "Objects/DecalObject.h"
#include "CryEditDoc.h"
#include "Mission.h"
#include "TerrainLighting.h"
#include "EntityPrototypeManager.h"
#include "Particles/ParticleManager.h"
#include "Material/Material.h"
#include "Objects/ObjectLayerManager.h"
#include "Vehicles/VehicleComp.h"

#ifndef NO_LIVECREATE

#define NO_LIVECREATE_COMMAND_IMPLEMENTATION
#include "../../CryEngine/CryLiveCreate/LiveCreateCommands.h"
#include "../../CryEngine/CryLiveCreate/LiveCreate_System.h"
#include "../../CryEngine/CryLiveCreate/LiveCreate_Objects.h"
#undef NO_LIVECREATE_COMMAND_IMPLEMENTATION

namespace LiveCreate
{
    bool ExportEntity(const CEntityObject& entity, string& outXML)
    {
        XmlNodeRef xmlNode = gEnv->pSystem->CreateXmlNode();
        if (NULL != xmlNode)
        {
            const_cast<CEntityObject&>(entity).Export("", xmlNode);
            if (xmlNode->getChildCount() > 0 && (NULL != xmlNode->getChild(0)))
            {
                xmlNode->getChild(0)->delAttr("EntityID");
                outXML = xmlNode->getChild(0)->getXML();
                return true;
            }
        }

        return false;
    }

    bool IsAllowedLightVar(IVariable* var)
    {
        if (0 == _stricmp(var->GetName(), "Radius"))
        {
            return true;
        }
        if (0 == _stricmp(var->GetName(), "Diffuse"))
        {
            return true;
        }
        if (0 == _stricmp(var->GetName(), "DiffuseMultiplier"))
        {
            return true;
        }
        if (0 == _stricmp(var->GetName(), "SpecularMultiplier"))
        {
            return true;
        }
        if (0 == _stricmp(var->GetName(), "ShadowBias"))
        {
            return true;
        }
        if (0 == _stricmp(var->GetName(), "ShadowSlopeBias"))
        {
            return true;
        }
        if (0 == _stricmp(var->GetName(), "ShadowUpdateMinRadius"))
        {
            return true;
        }
        if (0 == _stricmp(var->GetName(), "ShadowUpdateRatio"))
        {
            return true;
        }
        if (0 == _stricmp(var->GetName(), "SortPriority"))
        {
            return true;
        }
        return false;
    }

    template<typename T>
    void SetString(std::vector<T>& data, const char* str)
    {
        const size_t len = strlen(str); // DO NOT SEND THE ZERO AT THE END
        data.resize(len);
        memcpy(&data[0], str, len);
    }

    bool CEditorManager::SyncEnableLiveCreate()
    {
        if (CanSend())
        {
            CLiveCreateCmd_EnableLiveCreate command;

            // collect visible layers
            std::vector<CObjectLayer*> layers;
            GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers(layers);
            for (uint32 i = 0; i < layers.size(); ++i)
            {
                CObjectLayer* pLayer = layers[i];
                if (pLayer->IsVisible())
                {
                    command.m_visibleLayers.push_back((const char*)pLayer->GetName());
                }
                else
                {
                    command.m_hiddenLayers.push_back((const char*)pLayer->GetName());
                }
            }

            // send command
            if (!SendCommand(command))
            {
                return false;
            }

            SyncCameraFlag();
            return true;
        }

        return false;
    }

    bool CEditorManager::SyncDisableLiveCreate()
    {
        if (CanSend())
        {
            {
                CLiveCreateCmd_DisableCameraSync command;
                if (!SendCommand(command))
                {
                    return false;
                }
            }

            {
                CLiveCreateCmd_DisableLiveCreate command;
                command.m_bRestorePhysicsState = GetAdvancedSettings().bResumePhysicsOnDisconnect;
                command.m_bWakePhysicalObjects = GetAdvancedSettings().bAutoWakeUpPhysicalObjects;
                if (!SendCommand(command))
                {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    bool CEditorManager::SyncCameraTransform(const Matrix34& cameraMatrix)
    {
        m_lastCameraKnownPosition = cameraMatrix.GetTranslation();
        m_lastCameraKnownRotation = Quat::CreateRotationVDir(cameraMatrix.TransformVector(FORWARD_DIRECTION));

        if (CanSend())
        {
            const float eps = 0.001f;
            if (!m_lastCameraKnownPosition.IsEquivalent(m_lastCameraSyncPosition, eps) ||
                !m_lastCameraKnownRotation.v.IsEquivalent(m_lastCameraSyncRotation.v, eps))
            {
                m_lastCameraSyncPosition = m_lastCameraKnownPosition;
                m_lastCameraSyncRotation = m_lastCameraKnownRotation;

                CLiveCreateCmd_SetCameraPosition command;
                command.m_position = m_lastCameraSyncPosition;
                command.m_rotation = m_lastCameraSyncRotation;
                return SendCommand(command);
            }
        }

        return false;
    }

    bool CEditorManager::SyncCameraFOV(const float fov)
    {
        m_lastCameraKnownFOV = fov;

        if (CanSend())
        {
            const float eps = 0.001f;
            if (fabsf(m_lastCameraKnownFOV - m_lastCameraSyncFOV) > eps)
            {
                m_lastCameraSyncFOV = m_lastCameraKnownFOV;

                CLiveCreateCmd_SetCameraFOV command;
                command.m_fov = m_lastCameraSyncFOV;
                return SendCommand(command);
            }
        }

        return false;
    }

    bool CEditorManager::SyncObjectTransform(const CBaseObject& object)
    {
        bool bResult = false;

        if (CanSend() && object.IsKindOf(RUNTIME_CLASS(CEntityObject)))
        {
            const CEntityObject& entity = static_cast<const CEntityObject&>(object);

            // sync only entities that were created
            if (NULL != entity.GetIEntity())
            {
                CLiveCreateCmd_SetEntityTransform command;
                command.m_guid = entity.GetIEntity()->GetGuid();
                command.m_flags = 0;

                if (entity.GetEntityClass() == "Light")
                {
                    command.m_flags |= ILiveCreateCommand::eFlag_Light;
                }

                command.m_position = entity.GetPos();
                command.m_rotation = entity.GetRotation();
                command.m_scale = entity.GetScale();
                bResult = SendCommand(command);
            }
        }
        else
        {
            bResult = Sync3DObject(object);
        }

        if (bResult)
        {
            SyncSelection();
        }

        return bResult;
    }

    bool CEditorManager::SyncObjectCreated(const CBaseObject& object)
    {
        if (CanSend() && object.IsKindOf(RUNTIME_CLASS(CEntityObject)))
        {
            const CEntityObject& entity = static_cast<const CEntityObject&>(object);
            if (entity.GetIEntity() != NULL)
            {
                CLiveCreateCmd_EntityUpdate command;

                command.m_guid = entity.GetIEntity()->GetGuid();
                command.m_flags = 0;

                if (entity.GetEntityClass() == "Light")
                {
                    command.m_flags |= ILiveCreateCommand::eFlag_Light;
                }

                if (ExportEntity(entity, command.m_data))
                {
                    return SendCommand(command);
                }
            }
        }

        return false;
    }

    bool CEditorManager::SyncObjectDeleted(const CBaseObject& object)
    {
        if (CanSend() && object.IsKindOf(RUNTIME_CLASS(CEntityObject)))
        {
            const CEntityObject& entity = static_cast<const CEntityObject&>(object);

            CLiveCreateCmd_EntityDelete command;

            if (NULL != entity.GetIEntity())
            {
                command.m_guid = entity.GetIEntity()->GetGuid();
                command.m_flags = 0;

                if (entity.GetEntityClass() == "Light")
                {
                    command.m_flags |= ILiveCreateCommand::eFlag_Light;
                }

                return SendCommand(command);
            }
        }

        return false;
    }

    bool CEditorManager::SyncObjectFull(const CBaseObject& object)
    {
        if (CanSend())
        {
            if (object.IsKindOf(RUNTIME_CLASS(CEntityObject)))
            {
                const CEntityObject& entity = static_cast<const CEntityObject&>(object);

                CLiveCreateCmd_EntityUpdate command;

                if (NULL != entity.GetIEntity())
                {
                    command.m_guid = entity.GetIEntity()->GetGuid();
                    command.m_flags = 0;

                    if (entity.GetEntityClass() == "Light")
                    {
                        command.m_flags |= ILiveCreateCommand::eFlag_Light;
                    }

                    if (ExportEntity(entity, command.m_data))
                    {
                        return SendCommand(command);
                    }
                }
            }
            else
            {
                Sync3DObject(object);
            }
        }

        return false;
    }

    bool CEditorManager::SyncEntityProperty(const CEntityObject& entity, IVariable* var)
    {
        // special case for lights
        if (entity.GetEntityClass() == "Light" && (NULL != entity.GetIEntity()))
        {
            if (IsAllowedLightVar(var))
            {
                ILightSource* pLight = ILiveCreateCommand::FindLightSource(entity.GetEntityId());
                if (NULL != pLight)
                {
                    CLiveCreateCmd_UpdateLightParams command;
                    command.m_guid = entity.GetIEntity()->GetGuid();
                    command.m_Flags = pLight->GetLightProperties().m_Flags;
                    command.m_fRadius = pLight->GetLightProperties().m_fRadius;
                    command.m_Color = pLight->GetLightProperties().m_Color;
                    command.m_SpecMult = pLight->GetLightProperties().m_SpecMult;
                    command.m_fHDRDynamic = pLight->GetLightProperties().m_fHDRDynamic;
                    command.m_nAttenFalloffMax = pLight->GetLightProperties().m_nAttenFalloffMax;
                    command.m_nSortPriority = pLight->GetLightProperties().m_nSortPriority;
                    command.m_fShadowBias = pLight->GetLightProperties().m_fShadowBias;
                    command.m_fShadowSlopeBias = pLight->GetLightProperties().m_fShadowSlopeBias;
                    command.m_fShadowResolutionScale = pLight->GetLightProperties().m_fShadowResolutionScale;
                    command.m_fShadowUpdateMinRadius = pLight->GetLightProperties().m_fShadowUpdateMinRadius;
                    command.m_nShadowMinResolution = pLight->GetLightProperties().m_nShadowMinResolution;
                    command.m_nShadowUpdateRatio = pLight->GetLightProperties().m_nShadowUpdateRatio;
                    command.m_fBaseRadius = pLight->GetLightProperties().m_fBaseRadius;
                    command.m_BaseColor = pLight->GetLightProperties().m_BaseColor;
                    command.m_BaseSpecMult = pLight->GetLightProperties().m_BaseSpecMult;
                    return SendCommand(command);
                }
            }
        }

        // not synced using partial update
        return false;
    }

    bool CEditorManager::SyncObjectProperty(const CBaseObject& object, IVariable* var)
    {
        if (CanSend())
        {
            if (!GetAdvancedSettings().bAlwaysSyncFullEntities)
            {
                if (object.IsKindOf(RUNTIME_CLASS(CEntityObject)))
                {
                    const CEntityObject& entity = static_cast<const CEntityObject&>(object);
                    if (SyncEntityProperty(entity, var))
                    {
                        return true;
                    }
                }
            }

            // by default sync the full object
            return SyncObjectFull(object);
        }

        return false;
    }

    bool CEditorManager::SyncCVarChange(const ICVar* pCVar)
    {
        if (NULL != pCVar)
        {
            if (CanSend())
            {
                if (0 == (pCVar->GetFlags() & VF_LIVE_CREATE_SYNCED))
                {
                    CLiveCreateCmd_SetCVar command;
                    command.m_name = pCVar->GetName();
                    command.m_value = pCVar->GetString();
                    return SendCommand(command);
                }
            }
        }

        return false;
    }

    bool CEditorManager::SyncTimeOfDay(const float time)
    {
        if (CanSend())
        {
            CLiveCreateCmd_SetTimeOfDay command;
            command.m_time = time;
            return SendCommand(command);
        }

        return false;
    }

    bool CEditorManager::SyncTimeOfDayVariable(const int varIndex, const char* displayName, ISplineInterpolator* pSpline)
    {
        if (CanSend())
        {
            CLiveCreateCmd_SetTimeOfDayValue command;
            command.m_index = varIndex;
            command.m_name = displayName;

            XmlNodeRef xmlNode = gEnv->pSystem->CreateXmlNode();
            XmlNodeRef splineNode = xmlNode->newChild("Spline");
            xmlNode->addChild(splineNode);
            pSpline->SerializeSpline(splineNode, false);
            command.m_data = splineNode->getXML();

            return SendCommand(command);
        }

        return false;
    }

    bool CEditorManager::SyncTimeOfDayFull()
    {
        if (CanSend())
        {
            CLiveCreateCmd_SetTimeOfDayFull command;

            XmlNodeRef xmlNode = GetISystem()->CreateXmlNode("TimeOfDay");
            if (xmlNode)
            {
                gEnv->p3DEngine->GetTimeOfDay()->Serialize(xmlNode, false);
                SetString(command.m_data, xmlNode->getXML().c_str());
                if (!command.m_data.empty())
                {
                    return SendCommand(command);
                }
            }
        }

        return false;
    }

    bool CEditorManager::SyncEnvironmentFull()
    {
        if (CanSend())
        {
            CLiveCreateCmd_SetEnvironmentFull command;

            XmlNodeRef xmlNode = GetISystem()->CreateXmlNode("Environment");
            if (xmlNode)
            {
                CXmlTemplate::SetValues(GetIEditor()->GetDocument()->GetEnvironmentTemplate(), xmlNode);
                GetIEditor()->GetDocument()->GetCurrentMission()->GetLighting()->Serialize(xmlNode, false);
                SetString(command.m_data, xmlNode->getXML().c_str());
                if (!command.m_data.empty())
                {
                    return SendCommand(command);
                }
            }
        }

        return false;
    }

    bool CEditorManager::Sync3DEngine(const AABB& area, ELiveCreateObjectType type)
    {
        if (NULL == m_pObjectSync)
        {
            return false;
        }

        if (!m_bIsLoadingObjects)
        {
            m_pObjectSync->AddDirtyObjectArea(area, type);
            return true;
        }

        return false;
    }

    bool CEditorManager::Sync3DObject(const CBaseObject& object)
    {
        // Vehicle components are causing problems in LiveCreate
        if (object.IsKindOf(RUNTIME_CLASS(CVehicleComponent)))
        {
            return false;
        }

        AABB objectBox;
        const_cast<CBaseObject&>(object).GetBoundBox(objectBox);
        if (objectBox.IsNonZero())
        {
            if (object.IsKindOf(RUNTIME_CLASS(CBrushObject)))
            {
                return Sync3DEngine(objectBox, eLiveCreateObjectType_Brushes);
            }
            else if (object.IsKindOf(RUNTIME_CLASS(CDecalObject)))
            {
                return Sync3DEngine(objectBox, eLiveCreateObjectType_Decals);
            }
            else if (object.IsKindOf(RUNTIME_CLASS(CRoadObject)))
            {
                return Sync3DEngine(objectBox, eLiveCreateObjectType_Roads);
            }
        }

        return false;
    }

    bool CEditorManager::SyncArchetypesFull()
    {
        m_bArchetypesDirty = true;
        return true;
    }

    bool CEditorManager::SyncParticlesFull()
    {
        m_bParticlesDirty = true;
        return true;
    }

    bool CEditorManager::SyncTerrainTexture(const STerrainTextureInfo& texInfo)
    {
        /*gEnv->pLiveCreateManager->CommitData(kTerrainLayers, "data_size", &aDataSize);
        gEnv->pLiveCreateManager->CommitData(kTerrainLayers, "pos_x", &posx);
        gEnv->pLiveCreateManager->CommitData(kTerrainLayers, "pos_y", &posy);
        gEnv->pLiveCreateManager->CommitData(kTerrainLayers, "width", &width);
        gEnv->pLiveCreateManager->CommitData(kTerrainLayers, "height", &height);
        gEnv->pLiveCreateManager->CommitData(kTerrainLayers, "format", &eTFSrc);
        gEnv->pLiveCreateManager->CommitData(kTerrainLayers, "data", pData);
        gEnv->pLiveCreateManager->CommitData(kTerrainLayers, "send_layer_data", 0);*/
        return false;
    }

    bool CEditorManager::SyncFile(const char* szPath)
    {
        if (NULL != m_pFileSyncManager)
        {
            return m_pFileSyncManager->SyncFile(szPath);
        }

        return false;
    }

    bool CEditorManager::SyncSelection()
    {
        if (CanSend())
        {
            const bool bSendBoxes = GetGeneralSettings().bShowSelectionBoxes;
            const bool bSendNames = GetGeneralSettings().bShowSelectionNames;
            if (bSendNames || bSendBoxes)
            {
                CSelectionGroup* group = GetIEditor()->GetSelection();

                CLiveCreateCmd_SyncSelection command;
                const uint32 numObject = group->GetCount();
                for (uint32 i = 0; i < numObject; ++i)
                {
                    CBaseObject* pObject = group->GetObject(i);
                    if (NULL != pObject)
                    {
                        SSelectedObject info;
                        info.m_position = pObject->GetPos();

                        if (bSendNames)
                        {
                            info.m_flags |= 1;
                            info.m_name = pObject->GetName();
                        }

                        if (bSendBoxes)
                        {
                            info.m_flags |= 2;
                            info.m_matrix = pObject->GetLocalTM();
                            pObject->GetLocalBounds(info.m_box);
                        }

                        command.m_selection.push_back(info);
                    }
                }

                return SendCommand(command);
            }
        }

        return false;
    }

    bool CEditorManager::SyncCameraFlag()
    {
        if (CanSend())
        {
            if (m_bIsCameraSyncEnabled)
            {
                CLiveCreateCmd_EnableCameraSync command;
                command.m_position = m_lastCameraKnownPosition;
                command.m_rotation = m_lastCameraKnownRotation;
                return SendCommand(command);
            }
            else
            {
                CLiveCreateCmd_DisableCameraSync command;
                return SendCommand(command);
            }
        }

        return false;
    }

    bool CEditorManager::SyncObjectMaterial(const CBaseObject& object, CMaterial* pNewMaterial)
    {
        if (CanSend())
        {
            if (object.IsKindOf(RUNTIME_CLASS(CEntityObject)))
            {
                const CEntityObject& entity = static_cast<const CEntityObject&>(object);

                // sync only entities that were created
                if (NULL != entity.GetIEntity())
                {
                    CLiveCreateCmd_EntitySetMaterial command;
                    command.m_guid = entity.GetIEntity()->GetGuid();
                    command.m_materialName = pNewMaterial ? (const char*)pNewMaterial->GetName() : "";

                    return SendCommand(command);
                }
            }
            else
            {
                return Sync3DObject(object);
            }
        }

        return false;
    }

    bool CEditorManager::SyncLayerVisibility(const CObjectLayer& layer, bool bIsVisible)
    {
        if (CanSend())
        {
            CLiveCreateCmd_SyncLayerVisibility command;
            command.m_layerName = layer.GetName();
            command.m_bIsVisible = bIsVisible;
            return SendCommand(command);
        }

        return false;
    }


    //-----------------------------------------------------------------------------

    class CWindowsFileWriter
        : public IDataWriteStream
    {
    private:
        HANDLE m_hFileHandle;

    public:
        virtual void Write(const void* pData, const uint32 size) override
        {
            DWORD written = 0;
            WriteFile(m_hFileHandle, pData, size, &written, NULL);
        }

        virtual void Write8(const void* pData) override
        {
            Write(pData, sizeof(uint64));
        }

        virtual void Write4(const void* pData) override
        {
            Write(pData, sizeof(uint32));
        }

        virtual void Write2(const void* pData) override
        {
            Write(pData, sizeof(uint16));
        }

        virtual void Write1(const void* pData) override
        {
            Write(pData, sizeof(uint8));
        }

        virtual const uint32 GetSize() const
        {
            DWORD sizeHigh = 0;
            return GetFileSize(m_hFileHandle, &sizeHigh);
        }

        virtual struct IServiceNetworkMessage* BuildMessage() const
        {
            return NULL;
        }

        virtual void CopyToBuffer(void* pData) const
        {
        }

        virtual void Delete()
        {
            delete this;
        }

        void Seek(uint32 offset)
        {
            SetFilePointer(m_hFileHandle, offset, NULL, FILE_BEGIN);
        }

    private:
        CWindowsFileWriter(HANDLE hFileHandle)
            : m_hFileHandle(hFileHandle)
        {
        }

        ~CWindowsFileWriter()
        {
            CloseHandle(m_hFileHandle);
        }

    public:
        static CWindowsFileWriter* CreateWriter(const char* szPath)
        {
            HANDLE hFileHandle = CreateFileA(
                    szPath,
                    GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

            if (INVALID_HANDLE_VALUE != hFileHandle)
            {
                return new CWindowsFileWriter(hFileHandle);
            }

            return NULL;
        }
    };

    bool CEditorManager::BuildFullSyncFile(string& outPath)
    {
        const CTimeValue buildStartTime = gEnv->pTimer->GetAsyncTime();

        // allocate temporary file path
        char szTempFileName[MAX_PATH] = "LiveCreate_FullSync.tmp";
        ::GetTempFileName(".", "LCFS", 0, szTempFileName);

        // export data
        CWindowsFileWriter* pStream = CWindowsFileWriter::CreateWriter(szTempFileName);
        if (NULL == pStream)
        {
            return false;
        }

        // save header (placeholder)
        pStream->WriteUint32('LCFS');

        // export entities (Editor side - this is matched by LoadInternalState in IEntitySystem)
        GetIEditor()->GetObjectManager()->SaveEntitiesInternalState(*pStream);

        // layer translation table
        {
            std::vector<CObjectLayer*> layers;
            GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers(layers);

            std::vector<string> layerNames;
            std::vector<uint16> layerIds;
            for (uint32 i = 0; i < layers.size(); ++i)
            {
                CObjectLayer* pLayer = layers[i];
                if (NULL != pLayer)
                {
                    layerNames.push_back((const char*)pLayer->GetName());
                    layerIds.push_back(pLayer->GetLayerID());
                }
            }

            *pStream << layerNames;
            *pStream << layerIds;
        }

        // save 3D objects (brushes, terrain, decals & foliage)
        const AABB fullWorld(4096.0f);
        const bool bSaveTerrain = true;
        gEnv->p3DEngine->SaveInternalState(*pStream, fullWorld, bSaveTerrain, ~0);

        // export CVars
        gEnv->pConsole->SaveInternalState(*pStream);

        // export time of the day data
        gEnv->p3DEngine->GetTimeOfDay()->SaveInternalState(*pStream);

        {
            const float buildTime = gEnv->pTimer->GetAsyncTime().GetDifferenceInSeconds(buildStartTime);
            gEnv->pLog->Log("LiveCreate full sync file built in %1.2fs: %1.2fKB", buildTime, (float)pStream->GetSize() / 1024.0f);
        }

        // done
        pStream->Delete();

        // skip the "./" in the path
        outPath = szTempFileName + 2;
        return true;
    }

    bool CEditorManager::SendArchetypeData()
    {
        if (!CanSend())
        {
            return false;
        }

        CEntityPrototypeManager* poPrototypeManager = GetIEditor()->GetEntityProtManager();
        XmlNodeRef xmlNode = XmlHelpers::CreateXmlNode("EntityPrototypes");
        int nTotalLibraries = 0;
        int nCurrentLibrary = 0;
        int nTotalItems = 0;
        int nCurrentItem = 0;

        CEntityPrototype::SerializeContext stSerializeContext;

        nTotalLibraries = poPrototypeManager->GetLibraryCount();

        for (nCurrentLibrary = 0; nCurrentLibrary < nTotalLibraries; ++nCurrentLibrary)
        {
            IDataBaseLibrary* piLibrary(poPrototypeManager->GetLibrary(nCurrentLibrary));

            nTotalItems = piLibrary->GetItemCount();

            for (nCurrentItem = 0; nCurrentItem < nTotalItems; ++nCurrentItem)
            {
                XmlNodeRef xmlEntityNode = xmlNode->newChild("EntityPrototype");

                stSerializeContext.bCopyPaste = false;
                stSerializeContext.bIgnoreChilds = false;
                stSerializeContext.bLoading = false;
                stSerializeContext.bUndo = false;
                stSerializeContext.bUniqName = false;
                stSerializeContext.node = xmlEntityNode;

                CBaseLibraryItem* pItem = (CBaseLibraryItem*)piLibrary->GetItem(nCurrentItem);
                pItem->Serialize(stSerializeContext);
            }
        }

        CLiveCreateCmd_SetArchetypesFull command;
        SetString(command.m_data, xmlNode->getXML().c_str());

        LogMessagef(eLogType_Normal, "Archetype sync data size: %1.2fKB",
            (float)command.m_data.size() / 1024.0f);

        return SendCommand(command);
    }

    bool CEditorManager::SendParticlesData()
    {
        if (!CanSend())
        {
            return false;
        }

        XmlNodeRef xmlNode = GetISystem()->CreateXmlNode("Particles");
        GetIEditor()->GetParticleManager()->Export(xmlNode);

        CLiveCreateCmd_SetParticlesFull command;
        SetString(command.m_data, xmlNode->getXML().c_str());

        LogMessagef(eLogType_Normal, "Particle sync data size: %1.2fKB",
            (float)command.m_data.size() / 1024.0f);

        return SendCommand(command);
    }
} // namespace LiveCreate

#endif
