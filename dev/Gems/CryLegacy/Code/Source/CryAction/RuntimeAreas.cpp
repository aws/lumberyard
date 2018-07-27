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

#include "CryLegacy_precompiled.h"
#include "RuntimeAreas.h"
#include "GameObjects/RuntimeAreaObject.h"
#include <IGameObject.h>
#include "Components/IComponentArea.h"
#include <IAudioSystem.h>

CRuntimeAreaManager::CRuntimeAreaManager()
{
    gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

    IEntityClassRegistry::SEntityClassDesc runtimeObjectDesc;
    runtimeObjectDesc.sName = "RuntimeAreaObject";
    runtimeObjectDesc.sScriptFile = "";

    static IGameFramework::CGameObjectExtensionCreator<CRuntimeAreaObject> runtimeObjectCreator;
    gEnv->pGame->GetIGameFramework()->GetIGameObjectSystem()->RegisterExtension(runtimeObjectDesc.sName, &runtimeObjectCreator, &runtimeObjectDesc);

    FillAudioControls();
}

///////////////////////////////////////////////////////////////////////////
CRuntimeAreaManager::~CRuntimeAreaManager()
{
    gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
}

///////////////////////////////////////////////////////////////////////////
void CRuntimeAreaManager::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_LEVEL_UNLOAD:
        if (gEnv->IsEditor() == false)
        {
            DestroyAreas();
        }
        break;
    case ESYSTEM_EVENT_LEVEL_PRECACHE_END:
        if (gEnv->IsEditor() == false)
        {
            CreateAreas();
        }
        break;
    case ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED:
    case ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_CHANGED:
        if (wparam)
        {
            CreateAreas();
        }
        else
        {
            DestroyAreas();
        }
        break;
    }
}

char const* const CRuntimeAreaManager::SXMLTags::sMergedMeshSurfaceTypesRoot = "MergedMeshSurfaceTypes";
char const* const CRuntimeAreaManager::SXMLTags::sMergedMeshSurfaceTag = "SurfaceType";
char const* const CRuntimeAreaManager::SXMLTags::sAudioTag = "Audio";
char const* const CRuntimeAreaManager::SXMLTags::sNameAttribute = "name";
char const* const CRuntimeAreaManager::SXMLTags::sATLTriggerAttribute = "atl_trigger";
char const* const CRuntimeAreaManager::SXMLTags::sATLRtpcAttribute = "atl_rtpc";

///////////////////////////////////////////////////////////////////////////
void CRuntimeAreaManager::FillAudioControls()
{
    XmlNodeRef const pXMLRoot(gEnv->pSystem->LoadXmlFromFile("libs/materialeffects/mergedmeshsurfacetypes.xml"));

    if (pXMLRoot && azstricmp(pXMLRoot->getTag(), SXMLTags::sMergedMeshSurfaceTypesRoot) == 0)
    {
        size_t const nSurfaceCount = static_cast<size_t>(pXMLRoot->getChildCount());

        for (size_t i = 0; i < nSurfaceCount; ++i)
        {
            XmlNodeRef const pSurfaceTypeNode(pXMLRoot->getChild(i));

            if (pSurfaceTypeNode && azstricmp(pSurfaceTypeNode->getTag(), SXMLTags::sMergedMeshSurfaceTag) == 0)
            {
                char const* const sSurfaceName = pSurfaceTypeNode->getAttr(SXMLTags::sNameAttribute);

                if ((sSurfaceName != NULL) && (sSurfaceName[0] != '\0'))
                {
                    Audio::TAudioControlID nTriggerID = INVALID_AUDIO_CONTROL_ID;
                    Audio::TAudioControlID nRtpcID = INVALID_AUDIO_CONTROL_ID;

                    XmlNodeRef const pAudioNode(pSurfaceTypeNode->findChild(SXMLTags::sAudioTag));
                    if (pAudioNode)
                    {
                        char const* const sATLTriggerName = pAudioNode->getAttr(SXMLTags::sATLTriggerAttribute);

                        if ((sATLTriggerName != NULL) && (sATLTriggerName[0] != '\0'))
                        {
                            Audio::AudioSystemRequestBus::BroadcastResult(nTriggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, sATLTriggerName);
                        }

                        char const* const sATLRtpcName = pAudioNode->getAttr(SXMLTags::sATLRtpcAttribute);

                        if ((sATLRtpcName != NULL) && (sATLRtpcName[0] != '\0'))
                        {
                            Audio::AudioSystemRequestBus::BroadcastResult(nRtpcID, &Audio::AudioSystemRequestBus::Events::GetAudioRtpcID, sATLRtpcName);
                        }

                        if ((nTriggerID != INVALID_AUDIO_CONTROL_ID) && (nRtpcID != INVALID_AUDIO_CONTROL_ID))
                        {
                            CRuntimeAreaObject::m_cAudioControls[CCrc32::ComputeLowercase(sSurfaceName)] =
                                CRuntimeAreaObject::SAudioControls(nTriggerID, nRtpcID);
                        }
                    }
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////
void CRuntimeAreaManager::DestroyAreas()
{
    IEntitySystem* const entity_system = gEnv->pEntitySystem;
    for (size_t i = 0, end = m_areaObjects.size(); i < end; entity_system->RemoveEntity(m_areaObjects[i++], true))
    {
        ;
    }
    for (size_t i = 0, end = m_areas.size(); i < end; entity_system->RemoveEntity(m_areas[i++], true))
    {
        ;
    }
    m_areaObjects.clear();
    m_areas.clear();
}

///////////////////////////////////////////////////////////////////////////
void CRuntimeAreaManager::CreateAreas()
{
    IMergedMeshesManager* mmrm_manager = gEnv->p3DEngine->GetIMergedMeshesManager();
    mmrm_manager->CalculateDensity();

    DynArray<IMergedMeshesManager::SMeshAreaCluster> clusters;
    mmrm_manager->CompileAreas(clusters, IMergedMeshesManager::CLUSTER_CONVEXHULL_GRAHAMSCAN);

    size_t const nClusterCount =  clusters.size();
    m_areas.reserve(nClusterCount);
    m_areaObjects.reserve(nClusterCount);

    std::vector<Vec3> points;
    for (size_t i = 0; i < nClusterCount; ++i)
    {
        char szName[1024];
        azsprintf(szName, "RuntimeArea_%03d", (int)i);

        IMergedMeshesManager::SMeshAreaCluster& cluster = clusters[i];
        SEntitySpawnParams areaSpawnParams;
        areaSpawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("AreaShape");
        areaSpawnParams.nFlags = ENTITY_FLAG_VOLUME_SOUND | ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_NO_SAVE;
        areaSpawnParams.sLayerName = "global";
        areaSpawnParams.sName = szName;
        areaSpawnParams.vPosition = cluster.extents.GetCenter() - Vec3(0.0f, 0.0f, cluster.extents.GetSize().z * 0.5f);//??

        IComponentAreaPtr pAreaComponent = NULL;
        IEntity* pNewAreaEntity = gEnv->pEntitySystem->SpawnEntity(areaSpawnParams);
        if (pNewAreaEntity)
        {
            EntityId const nAreaEntityID = pNewAreaEntity->GetId();

            pAreaComponent = pNewAreaEntity->GetOrCreateComponent<IComponentArea>();
            if (pAreaComponent)
            {
                size_t const nPointCount = cluster.boundary_points.size();
                DynArray<bool> abObstructSound(nPointCount + 2, false);
                points.resize(nPointCount);
                for (size_t j = 0; j < points.size(); ++j)
                {
                    points[j] = Vec3(
                            cluster.boundary_points[j].x,
                            cluster.boundary_points[j].y,
                            areaSpawnParams.vPosition.z) - areaSpawnParams.vPosition;
                }

                pAreaComponent->SetPoints(&points[0], &abObstructSound[0], points.size(), cluster.extents.GetSize().z);
                pAreaComponent->SetID(11001100 + i);
                pAreaComponent->SetPriority(100);
                pAreaComponent->SetGroup(110011);

                SEntitySpawnParams areaObjectSpawnParams;
                areaObjectSpawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("RuntimeAreaObject");
                areaObjectSpawnParams.nFlags = ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_NO_SAVE;
                areaObjectSpawnParams.sLayerName = "global";

                IEntity* pNewAreaObjectEntity = gEnv->pEntitySystem->SpawnEntity(areaObjectSpawnParams);

                if (pNewAreaObjectEntity != NULL)
                {
                    EntityId const nAreaObjectEntityID = pNewAreaObjectEntity->GetId();

                    pAreaComponent->AddEntity(nAreaObjectEntityID);

                    m_areaObjects.push_back(nAreaObjectEntityID);
                    m_areas.push_back(nAreaEntityID);
                }
                else
                {
                    gEnv->pEntitySystem->RemoveEntity(nAreaEntityID, true);
                }
            }
        }
    }
}
