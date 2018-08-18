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
#include <ICryAnimation.h>

#include "LiveMocapActor.h"
#include "../Animation/SkeletonMapperGraph.h"

/*

  CLiveMocapActor

*/

std::map<IEntity*, CLiveMocapActor*> CLiveMocapActor::s_entityToActor;

//

CLiveMocapActor* CLiveMocapActor::Create(const char* name, const LMSet* pSet)
{
    CLiveMocapActor* pActor = new CLiveMocapActor();
    pActor->m_name = name;
    pActor->m_pSet = pSet;
    return pActor;
}

//

CLiveMocapActor::CLiveMocapActor()
{
    m_pEntity = NULL;

    m_entityUpdate = new CVariable<bool>();
    m_entityUpdate->SetHumanName("Update");
    m_entityUpdate->Set(true);

    m_entityShowHierarchy = new CVariable<bool>();
    m_entityShowHierarchy->SetHumanName("Show Hierarchy");
    m_entityShowHierarchy->Set(false);

    m_locationsShow = new CVariable<bool>();
    m_locationsShow->SetHumanName("Show");
    m_locationsShow->Set(false);

    m_entityScale = new CVariable<Vec3>();
    m_entityScale->SetHumanName("Scale");
    m_entityScale->Set(Vec3(1.0f, 1.0f, 1.0f));

    m_locationsShowName = new CVariable<bool>();
    m_locationsShowName->SetHumanName("Show Name");
    m_locationsShowName->Set(false);

    m_locationsShowHierarchy = new CVariable<bool>();
    m_locationsShowHierarchy->SetHumanName("Show Hierarchy");
    m_locationsShowHierarchy->Set(false);

    m_locationsFreeze = new CVariable<bool>();
    m_locationsFreeze->SetHumanName("Freeze Input");
    m_locationsFreeze->Set(false);
}

CLiveMocapActor::~CLiveMocapActor()
{
    UnsetEntity();
}

//

CLiveMocapActor::SNode* CLiveMocapActor::GetNode(const char* name, bool bCreate)
{
    if (!name)
    {
        return NULL;
    }
    if (!::strlen(name))
    {
        return NULL;
    }

    for (size_t i = 0; i < m_nodes.size(); ++i)
    {
        if (::_stricmp(m_nodes[i].name, name) == 0)
        {
            return &m_nodes[i];
        }
    }

    if (!bCreate)
    {
        return NULL;
    }

    SNode node;
    node.name = name;
    node.pose = QuatT(IDENTITY);
    m_nodes.push_back(node);
    return &m_nodes.back();
}

void CLiveMocapActor::DrawNodes(const QuatT& origin)
{
    IRenderer* pRenderer = ::gEnv->pRenderer;
    IRenderAuxGeom* pRenderAuxGeom = pRenderer->GetIRenderAuxGeom();

    bool bShowLocations;
    m_locationsShow->Get(bShowLocations);
    bool bShowNames;
    m_locationsShowName->Get(bShowNames);

    uint32 nodeCount = uint32(m_nodes.size());
    for (uint32 i = 0; i < nodeCount; ++i)
    {
        if (bShowLocations)
        {
            pRenderAuxGeom->DrawSphere(
                origin.t + origin.q * m_nodes[i].pose.t, 0.01f, ColorB(0, 255, 0));
        }
        if (bShowNames)
        {
            pRenderer->DrawLabel(
                origin.t + origin.q * m_nodes[i].pose.t, 1.0f, m_nodes[i].name);
        }
    }
}

void CLiveMocapActor::DrawMapper(const QuatT& origin, QuatT* pLocations)
{
    if (!m_skeletonRemapperGraph)
    {
        return;
    }

    Skeleton::CMapper& mapper = m_skeletonRemapperGraph->GetMapper();

    Skeleton::CHierarchy locations;
    if (!mapper.CreateLocationsHierarchy(locations))
    {
        return;
    }

    IRenderAuxGeom* pRenderAuxGeom = ::gEnv->pRenderer->GetIRenderAuxGeom();

    Skeleton::CHierarchy& hierarchy = mapper.GetHierarchy();
    uint32 count = locations.GetNodeCount();
    for (uint32 i = 0; i < count; ++i)
    {
        Skeleton::CHierarchy::SNode* pNode = locations.GetNode(i);
        if (!pNode)
        {
            continue;
        }

        if (pNode->parent < 0)
        {
            continue;
        }

        Skeleton::CHierarchy::SNode* pParent = locations.GetNode(pNode->parent);
        if (!pParent)
        {
            continue;
        }

        int32 index = hierarchy.FindNodeIndexByName(pNode->name);
        if (index < 0)
        {
            continue;
        }

        int32 parent = hierarchy.FindNodeIndexByName(pParent->name);
        if (parent < 0)
        {
            continue;
        }

        pRenderAuxGeom->DrawLine(
            origin.t + origin.q * pLocations[parent].t, ColorB(0, 0, 0),
            origin.t + origin.q * pLocations[index].t, ColorB(255, 255, 255), 8.0f);
    }
}

void CLiveMocapActor::SetEntity(IEntity* pEntity)
{
    UnsetEntity();

    std::map<IEntity*, CLiveMocapActor*>::iterator i = s_entityToActor.find(pEntity);
    if (i != s_entityToActor.end())
    {
        i->second->UnsetEntity();
    }

    if (pEntity)
    {
        s_entityToActor[pEntity] = this;

        // backup the original entity location for later
        m_entityLocBackup.t = pEntity->GetWorldPos();
        m_entityLocBackup.q = pEntity->GetWorldRotation();
    }
    m_pEntity = pEntity;

    if (m_skeletonRemapperGraph)
    {
        Skeleton::CMapper& mapper = m_skeletonRemapperGraph->GetMapper();
        if (IDefaultSkeleton* pIDefaultSkeleton = HaveSkeletonPose())
        {
            mapper.GetHierarchy().CreateFrom(pIDefaultSkeleton);
        }
        else
        {
            mapper.GetHierarchy().ClearNodes();
            mapper.GetHierarchy().AddNode("Root", IDENTITY);
        }
        mapper.CreateFromHierarchy();
        CreateNodes(*m_skeletonRemapperGraph);
    }
}

void CLiveMocapActor::UnsetEntity()
{
    if (!m_pEntity)
    {
        return;
    }

    if (ICharacterInstance* pCharacter = m_pEntity->GetCharacter(0))
    {
        if (ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose())
        {
            pSkeletonPose->SetDefaultPose();
        }
    }

    // Restore the original entity location
    m_pEntity->SetPos(m_entityLocBackup.t);
    m_pEntity->SetRotation(m_entityLocBackup.q);

    s_entityToActor.erase(m_pEntity);
    m_pEntity = NULL;
}

Skeleton::CMapperGraph* CLiveMocapActor::CreateSkeletonMapperGraph()
{
    if (!m_pEntity)
    {
        QMessageBox:::warning(nullptr, QString(), "Please assign an entity to this subject before creating a mapping graph.");
        return NULL;
    }

    m_skeletonRemapperGraph = Skeleton::CMapperGraph::Create();
    Skeleton::CMapper& mapper = m_skeletonRemapperGraph->GetMapper();
    Skeleton::CHierarchy& hierarchy = mapper.GetHierarchy();
    hierarchy.ClearNodes();
    if (IDefaultSkeleton* pIDefaultSkeleton = HaveSkeletonPose())
    {
        hierarchy.CreateFrom(pIDefaultSkeleton);
    }
    else
    {
        hierarchy.AddNode("Root", QuatT(IDENTITY));
    }

    mapper.CreateFromHierarchy();

    m_skeletonRemapperGraph->Initialize();
    uint32 nodeCount = uint32(m_nodes.size());
    for (uint32 i = 0; i < nodeCount; ++i)
    {
        m_skeletonRemapperGraph->AddLocation(m_nodes[i].name);
    }

    return m_skeletonRemapperGraph;
}

bool CLiveMocapActor::SetSkeletonMapperGraph(Skeleton::CMapperGraph* pMapperGraph)
{
    m_skeletonRemapperGraph = pMapperGraph;
    if (!m_skeletonRemapperGraph)
    {
        return false;
    }

    Skeleton::CMapper& mapper = m_skeletonRemapperGraph->GetMapper();
    Skeleton::CHierarchy& hierarchy = mapper.GetHierarchy();
    hierarchy.ClearNodes();
    if (IDefaultSkeleton* pIDefaultSkeleton = HaveSkeletonPose())
    {
        hierarchy.CreateFrom(pIDefaultSkeleton);
    }
    else
    {
        hierarchy.AddNode("Root", QuatT(IDENTITY));
    }
    mapper.CreateFromHierarchy();

    CreateLocations(*pMapperGraph);
    CreateNodes(*pMapperGraph);
    return true;
}

IDefaultSkeleton* CLiveMocapActor::HaveSkeletonPose()
{
    if (!m_pEntity)
    {
        return NULL;
    }

    ICharacterInstance* pCharacter = m_pEntity->GetCharacter(0);
    if (!pCharacter)
    {
        return NULL;
    }

    return &pCharacter->GetIDefaultSkeleton();
}

void CLiveMocapActor::CreateLocations(Skeleton::CMapperGraph& mapperGraph)
{
    mapperGraph.ClearLocations();
    uint32 nodeCount = uint32(m_nodes.size());
    for (uint32 i = 0; i < nodeCount; ++i)
    {
        mapperGraph.AddLocation(m_nodes[i].name);
    }
}

void CLiveMocapActor::CreateNodes(Skeleton::CMapperGraph& mapperGraph)
{
    mapperGraph.ClearNodes();

    IDefaultSkeleton* pIDefaultSkeleton = HaveSkeletonPose();
    if (!pIDefaultSkeleton)
    {
        mapperGraph.AddNode("Root");
        return;
    }

    uint32 nodeCount = pIDefaultSkeleton->GetJointCount();
    for (uint32 i = 0; i < nodeCount; ++i)
    {
        mapperGraph.AddNode(pIDefaultSkeleton->GetJointNameByID(i));
    }
}

void CLiveMocapActor::UpdateLocations(Skeleton::CMapper& mapper)
{
    uint32 locationCount = mapper.GetLocationCount();
    for (uint32 i = 0; i < locationCount; ++i)
    {
        const char* name = mapper.GetLocation(i)->GetName();
        SNode* pNode = GetNode(name, false);
        if (!pNode)
        {
            continue;
        }

        mapper.GetLocation(i)->SetLocation(pNode->pose);
    }
}

bool CLiveMocapActor::UpdateSkeletonPose(ISkeletonPose& skeletonPose, IDefaultSkeleton& rIDefaultSkeleton, const QuatT& origin, Skeleton::CMapper& mapper)
{
    Skeleton::CHierarchy& hierarchy = mapper.GetHierarchy();
    uint32 nodeCount = hierarchy.GetNodeCount();
    for (uint32 i = 0; i < nodeCount; ++i)
    {
        Skeleton::CHierarchy::SNode* pNode = hierarchy.GetNode(i);
        if (!pNode)
        {
            continue;
        }

        int16 index = rIDefaultSkeleton.GetJointIDByName(pNode->name);
        if (index < 1)
        {
            continue;
        }

        pNode->pose = rIDefaultSkeleton.GetDefaultAbsJointByID(index);
    }

    std::vector<QuatT> result(nodeCount);
    mapper.Map(&result[0]);

    bool bShowHierarchy;
    m_locationsShowHierarchy->Get(bShowHierarchy);
    if (bShowHierarchy)
    {
        DrawMapper(origin, &result[0]);
    }

    bool bEntityUpdate;
    m_entityUpdate->Get(bEntityUpdate);
    if (!bEntityUpdate)
    {
        return true;
    }

    hierarchy.AbsoluteToRelative(&result[0], &result[0]);
    for (uint32 i = 0; i < nodeCount; ++i)
    {
        const Skeleton::CHierarchy::SNode* pNode = hierarchy.GetNode(i);
        if (!pNode)
        {
            continue;
        }

        int32 index = rIDefaultSkeleton.GetJointIDByName(pNode->name);
        if (index < 0)
        {
            continue;
        }

        skeletonPose.SetForceSkeletonUpdate(0x8000);

        // TODO: If this functionality is needed back, implement OperatorQueue PoseModifier.
        // skeletonPose.SetPostProcessQuat(index, result[i]);
    }

    return true;
}

bool CLiveMocapActor::UpdateEntity(IEntity& entity, const QuatT& origin, Skeleton::CMapper& mapper)
{
    Skeleton::CHierarchy& hierarchy = mapper.GetHierarchy();
    int32 index = hierarchy.FindNodeIndexByName("Root");
    if (index < 0)
    {
        return false;
    }

    Skeleton::CHierarchy::SNode* pHierarchyNode = hierarchy.GetNode(index);
    if (!pHierarchyNode)
    {
        return false;
    }

    pHierarchyNode->pose = QuatT(IDENTITY);

    uint32 nodeCount = hierarchy.GetNodeCount();
    std::vector<QuatT> result(nodeCount);
    mapper.Map(&result[0]);

    entity.SetPos(origin.t + origin.q * result[index].t);
    entity.SetRotation(origin.q * result[index].q);

    return true;
}

void CLiveMocapActor::Reset()
{
    UnsetEntity();
}

void CLiveMocapActor::Update(QuatT& origin)
{
    bool bShowLocations;
    m_locationsShow->Get(bShowLocations);
    bool bShowLocationsName;
    m_locationsShowName->Get(bShowLocationsName);
    if (bShowLocations || bShowLocationsName)
    {
        DrawNodes(origin);
    }

    if (!m_pEntity)
    {
        return;
    }

    if (!m_skeletonRemapperGraph)
    {
        return;
    }

    Skeleton::CMapper& mapper = m_skeletonRemapperGraph->GetMapper();
    m_skeletonRemapperGraph->UpdateMapper();

    bool bLocationsFreeze;
    m_locationsFreeze->Get(bLocationsFreeze);
    if (!bLocationsFreeze)
    {
        UpdateLocations(mapper);
    }

    bool bEntityUpdate;
    m_entityUpdate->Get(bEntityUpdate);
    if (IDefaultSkeleton* pIDefaultSkeleton = HaveSkeletonPose())
    {
        ICharacterInstance* pCharacter = m_pEntity->GetCharacter(0);
        ISkeletonPose* pISkeletonPose = pCharacter->GetISkeletonPose();
        if (bEntityUpdate)
        {
            m_pEntity->SetPos(origin.t);
            m_pEntity->SetRotation(origin.q);

            Vec3 scale;
            m_entityScale->Get(scale);
            m_pEntity->SetScale(Vec3(scale.x, scale.x, scale.x)); // TEMP
        }

        m_pEntity->Activate(true);
        m_pEntity->PrePhysicsActivate(true);
        UpdateSkeletonPose(*pISkeletonPose, *pIDefaultSkeleton, origin, mapper);
        return;
    }

    if (bEntityUpdate)
    {
        UpdateEntity(*m_pEntity, origin, mapper);
    }
}

// ILMActor

void CLiveMocapActor::SetPosition(LMString name, LMu32 time, const LMVec3f32& position)
{
    SNode* pNode = GetNode(name, true);
    if (!pNode)
    {
        return;
    }

    // TEMP: Scaling should be done explicitly somewhere else!
    pNode->pose.t.x = position.x / 1000.0f;
    pNode->pose.t.y = position.y / 1000.0f;
    pNode->pose.t.z = position.z / 1000.0f;
}

void CLiveMocapActor::SetOrientation(LMString name, LMu32 time, const LMQuat32& orientation)
{
    SNode* pNode = GetNode(name, true);
    if (!pNode)
    {
        return;
    }

    pNode->pose.q = *(Quat*)&orientation;
}

void CLiveMocapActor::SetScale(LMString name, LMu32 time, const LMVec3f32& scale)
{
}

const char* CLiveMocapActor::GetEntityName() const
{
    static const char* noEntity = "<none>";
    if (m_pEntity)
    {
        return m_pEntity->GetName();
    }
    else
    {
        return noEntity;
    }
}

