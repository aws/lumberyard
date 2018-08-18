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

// Description : Entity layer container.


#ifndef CRYINCLUDE_CRYENTITYSYSTEM_ENTITYLAYER_H
#define CRYINCLUDE_CRYENTITYSYSTEM_ENTITYLAYER_H

#pragma once

#include <StlUtils.h>

struct SEntityLayerGarbage
{
    SEntityLayerGarbage(IGeneralMemoryHeap* _pHeap, const string& _layerName)
        : pHeap(_pHeap)
        , layerName(_layerName)
        , nAge(0)
    {
    }

    IGeneralMemoryHeap* pHeap;
    string layerName;
    int nAge;
};
class CEntityLayer;

//////////////////////////////////////////////////////////////////////////
// Structure for deferred layer activation/deactivation processing
// The operations are queued during serialization, then sorted to ensure deactivation
// happens prior to activation.
struct SPostSerializeLayerActivation
{
    typedef void (CEntityLayer::* ActivationFunc)(bool);
    CEntityLayer* m_layer;
    ActivationFunc m_func;
    bool enable;
};

typedef std::vector<SPostSerializeLayerActivation> TLayerActivationOpVec;

class CEntityLayer
{
    struct EntityProp
    {
        EntityProp()
            : m_id(0)
            , m_bIsNoAwake(false)
            , m_bIsHidden(false)
            , m_bIsActive(false)
        {
        }

        EntityProp(EntityId id, bool bIsNoAwake, bool bIsHidden, bool bIsActive)
            : m_id(id)
            , m_bIsNoAwake(bIsNoAwake)
            , m_bIsHidden(bIsHidden)
            , m_bIsActive(bIsActive)
        {
        }

        bool operator==(const EntityProp& other) const
        {
            return (m_id == other.m_id);
        }

        EntityId m_id;
        bool m_bIsNoAwake : 1;
        bool m_bIsHidden : 1;
        bool m_bIsActive : 1;
    };

    struct EntityPropFindPred
    {
        EntityPropFindPred(EntityId _idToFind)
            : idToFind(_idToFind) {}
        bool operator () (const EntityProp& entityProp) { return entityProp.m_id == idToFind; }
        EntityId idToFind;
    };

public:
    typedef std::vector<SEntityLayerGarbage> TGarbageHeaps;

public:
    CEntityLayer(const char* name, uint16 id, bool havePhysics, int specs, bool defaultLoaded, TGarbageHeaps& garbageHeaps);
    virtual ~CEntityLayer();

    ILINE void SetParentName(const char* parent)
    {
        if (parent)
        {
            m_parentName = parent;
        }
    }
    ILINE void AddChild(CEntityLayer* pLayer) { return m_childs.push_back(pLayer); }
    ILINE int GetNumChildren() const { return m_childs.size(); }
    ILINE CEntityLayer* GetChild(int idx) const { return m_childs[idx]; }
    void AddObject(EntityId id);
    void RemoveObject(EntityId id);
    void Enable(bool isEnable, bool isSerialized = true, bool bAllowRecursive = true);
    ILINE bool IsEnabled() const { return (m_isEnabled | m_isEnabledBrush); }
    ILINE bool IsEnabledBrush() const { return m_isEnabledBrush; }
    ILINE bool IsSerialized() const { return m_isSerialized; }
    ILINE bool IsDefaultLoaded() const { return m_defaultLoaded; }
    ILINE bool IncludesEntity(EntityId id) const { return m_entities.find(id) != m_entities.end(); }
    ILINE const string& GetName() const { return m_name; }
    ILINE const string& GetParentName() const { return m_parentName; }
    ILINE const uint16 GetId() const { return m_id; }

    void GetMemoryUsage(ICrySizer* pSizer, int* pOutNumEntities);
    void Serialize(TSerialize ser, TLayerActivationOpVec& deferredOps);
    bool IsSkippedBySpec() const;

private:

    void EnableBrushes(bool isEnable);
    void EnableEntities(bool isEnable);
    void ReEvalNeedForHeap();

private:
    typedef AZStd::unordered_map< EntityId, EntityProp > TEntityProps;

    int m_specs;
    string m_name;
    string m_parentName;
    bool m_isEnabled;
    bool m_isEnabledBrush;
    bool m_isSerialized;
    bool m_havePhysics;
    bool m_defaultLoaded;
    bool m_wasReEnabled;
    uint16 m_id;
    std::vector<CEntityLayer*> m_childs;
    TEntityProps m_entities;

    TGarbageHeaps* m_pGarbageHeaps;
    IGeneralMemoryHeap* m_pHeap;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_ENTITYLAYER_H
