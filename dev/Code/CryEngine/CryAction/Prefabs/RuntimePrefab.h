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
#ifndef CRYINCLUDE_CRYACTION_RUNTIMEPREFAB_H
#define CRYINCLUDE_CRYACTION_RUNTIMEPREFAB_H
#pragma once

//////////////////////////////////////////////////////////////////////////
class CRuntimePrefab
{
public:
    explicit CRuntimePrefab(EntityId id);
    ~CRuntimePrefab();

    void Spawn(const std::shared_ptr<CPrefab>& prefab);
    void Spawn(IEntity* entity, const std::shared_ptr<CPrefab>& prefab, const Matrix34& localTransform, AABB& box);
    void Move();
    void Hide(bool hide);

    std::shared_ptr<CPrefab> GetSourcePrefab() { return m_sourcePrefab; }

    Matrix34 m_worldToLocal; // local matrix

protected:

    void SpawnEntities(const std::shared_ptr<CPrefab>& prefab, const Matrix34& matSource, AABB& box);
    void SpawnBrushes(const std::shared_ptr<CPrefab>& prefab, const Matrix34& matSource, AABB& box);

    void Clear();
    void Move(const Matrix34& matOff);
    void HideComponents(bool hide);

    std::shared_ptr<CPrefab> m_sourcePrefab;
    EntityId m_id;                              // original entity who spawned this prefab
    std::vector<EntityId> m_spawnedIds;            // list of entities spawned by this prefab
    std::vector<IRenderNode*> m_spawnedNodes;      // list of brushes/decals spawned by this prefab
    std::vector<std::shared_ptr<CRuntimePrefab> > m_spawnedPrefabs; // list of prefabs  spawned by this prefab
};

#endif // CRYINCLUDE_CRYACTION_RUNTIMEPREFAB_H
