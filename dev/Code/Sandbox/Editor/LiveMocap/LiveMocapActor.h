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

#ifndef CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAPACTOR_H
#define CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAPACTOR_H
#pragma once


#include "../../SDKs/LiveMocap/LiveMocap.h"

#include "../Animation/SkeletonMapper.h"

//

struct IEntity;
namespace Skeleton {
    class CMapperGraph;
}

//

class CLiveMocapActor
    : public ILMActor
{
public:
    static CLiveMocapActor* Create(const char* name, const LMSet* pSet);

private:
    static std::map<IEntity*, CLiveMocapActor*> s_entityToActor;

public:
    struct SNode
    {
        string name;
        QuatT pose;
    };

private:
    CLiveMocapActor();
    ~CLiveMocapActor();

public:
    void Release() { delete this; }

    const char* GetName() const { return m_name; }
    const char* GetEntityName() const;

    uint32 GetNodeCount() const { return (uint32)m_nodes.size(); }
    const SNode* GetNode(uint32 index) const { return &m_nodes[index]; }

    void SetEntity(IEntity* pEntity);
    IEntity* GetEntity() { return m_pEntity; }

    Skeleton::CMapperGraph* CreateSkeletonMapperGraph();
    bool SetSkeletonMapperGraph(Skeleton::CMapperGraph* pMapperGraph);
    Skeleton::CMapperGraph* GetSkeletonMapperGraph() { return m_skeletonRemapperGraph; }

    void Reset();

    void Update(QuatT& origin);

private:
    SNode* GetNode(const char* name, bool bCreate = false);

    void UnsetEntity();

    IDefaultSkeleton* HaveSkeletonPose();

    void CreateLocations(Skeleton::CMapperGraph& mapperGraph);
    void CreateNodes(Skeleton::CMapperGraph& mapperGraph);

    void UpdateLocations(Skeleton::CMapper& mapper);
    bool UpdateSkeletonPose(ISkeletonPose& skeletonPose, IDefaultSkeleton& rIDefaultSkeleton, const QuatT& origin, Skeleton::CMapper& mapper);
    bool UpdateEntity(IEntity& entity, const QuatT& origin, Skeleton::CMapper& mapper);

    void DrawNodes(const QuatT& origin);
    void DrawMapper(const QuatT& origin, QuatT* pLocations);

    // ILMActor
public:
    LM_VIRTUAL void SetPosition(LMString name, LMu32 time, const LMVec3f32& position);
    LM_VIRTUAL void SetOrientation(LMString name, LMu32 time, const LMQuat32& orientation);
    LM_VIRTUAL void SetScale(LMString name, LMu32 time, const LMVec3f32& scale);

public:
    IVariablePtr m_entityUpdate;
    IVariablePtr m_entityShowHierarchy;
    IVariablePtr m_entityScale;

    IVariablePtr m_locationsShow;
    IVariablePtr m_locationsShowName;
    IVariablePtr m_locationsShowHierarchy;
    IVariablePtr m_locationsFreeze;

private:
    string m_name;
    const LMSet* m_pSet;

    std::vector<SNode> m_nodes;

    IEntity* m_pEntity;
    QuatT m_entityLocBackup;


    _smart_ptr<Skeleton::CMapperGraph> m_skeletonRemapperGraph;
};

#endif // CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAPACTOR_H
