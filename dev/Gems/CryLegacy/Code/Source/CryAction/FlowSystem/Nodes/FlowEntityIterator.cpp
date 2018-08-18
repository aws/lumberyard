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
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <IEntitySystem.h>
#include <IActorSystem.h>
#include <IVehicleSystem.h>
#include <IItemSystem.h>
#include "Components/IComponentArea.h"


#define ENTITY_TYPE_ENUM ("enum_int:All=0,AI=1,Actor=2,Vehicle=3,Item=4,Other=5")

#define ADD_BASE_INPUTS_ENTITY_ITER()                                                                                             \
    InputPortConfig_Void("Next", _HELP("Activate to get next Entity found")),                                                     \
    InputPortConfig<int>("Limit", 0, _HELP("Limit how many Entities are returned. Use 0 to get all entities")),                   \
    InputPortConfig<bool>("Immediate", false, _HELP("TRUE to iterate immediately through results, without having to call Next")), \
    InputPortConfig<int>("Type", 0, _HELP("Type of entity to iterate"), 0, _UICONFIG(ENTITY_TYPE_ENUM)),                          \
    InputPortConfig<string>("ArchetypeFilter", _HELP("If not empty, will only return entities of this archetype"))

#define ADD_BASE_OUTPUTS_ENTITY_ITER()                                                                                                  \
    OutputPortConfig<FlowEntityId>("OutEntityId", _HELP("Activated each time an Entity is found, with the Id of the Entity returned")), \
    OutputPortConfig<int>("Count", _HELP("Activated each time an Entity is found, with the current count returned")),                   \
    OutputPortConfig<int>("Done", _HELP("Activated when all Entities have been found, with the total count returned"))


enum EEntityType
{
    eET_Unknown = 0x00,
    eET_Valid = 0x01,
    eET_AI = 0x02,
    eET_Actor = 0x04,
    eET_Vehicle = 0x08,
    eET_Item = 0x10,
};

bool IsValidType(int requested, const EEntityType& type)
{
    bool bValid = false;

    if (requested == 0)
    {
        bValid = (type & eET_Valid) == eET_Valid;
    }
    else if (requested == 1)
    {
        bValid = (type & eET_AI) == eET_AI;
    }
    else if (requested == 2)
    {
        bValid = (type & eET_Actor) == eET_Actor;
    }
    else if (requested == 3)
    {
        bValid = (type & eET_Vehicle) == eET_Vehicle;
    }
    else if (requested == 4)
    {
        bValid = (type & eET_Item) == eET_Item;
    }
    else if (requested == 5)
    {
        bValid = (type == eET_Valid);
    }

    return bValid;
}

EEntityType GetEntityType(FlowEntityId id)
{
    int type = eET_Unknown;

    IEntitySystem* pEntitySystem = gEnv->pEntitySystem;
    if (pEntitySystem)
    {
        IEntity* pEntity = pEntitySystem->GetEntity(id);
        if (pEntity)
        {
            type = eET_Valid;

            IEntityClass* pClass = pEntity->GetClass();
            if (pClass)
            {
                const char* className = pClass->GetName();

                // Check AI
                if (pEntity->GetAI())
                {
                    type |= eET_AI;
                }

                // Check actor
                IActorSystem* pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
                if (pActorSystem)
                {
                    IActor* pActor = pActorSystem->GetActor(id);
                    if (pActor)
                    {
                        type |= eET_Actor;
                    }
                }

                // Check vehicle
                IVehicleSystem* pVehicleSystem = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem();
                if (pVehicleSystem)
                {
                    if (pVehicleSystem->IsVehicleClass(className))
                    {
                        type |= eET_Vehicle;
                    }
                }

                // Check item
                IItemSystem* pItemSystem = gEnv->pGame->GetIGameFramework()->GetIItemSystem();
                if (pItemSystem)
                {
                    if (pItemSystem->IsItemClass(className))
                    {
                        type |= eET_Item;
                    }
                }
            }
        }
    }

    return (EEntityType)type;
}

//////////////////////////////////////////////////////////////////////////
class CFlowBaseIteratorNode
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    virtual bool GetCustomConfiguration(SFlowNodeConfig& config)
    {
        return false;
    }

    virtual void OnIterStart(SActivationInfo* pActInfo)
    {
    }

    virtual void OnIterNext(SActivationInfo* pActInfo)
    {
    }

    CFlowBaseIteratorNode(SActivationInfo* pActInfo)
        : m_Count(0)
        , m_Iter(0)
    {
    }

    ~CFlowBaseIteratorNode() override
    {
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        // Define input ports here, in same order as EInputPorts
        static const SInputPortConfig inputs [] =
        {
            ADD_BASE_INPUTS_ENTITY_ITER(),
            { 0 }
        };

        // Define output ports here, in same oreder as EOutputPorts
        static const SOutputPortConfig outputs [] =
        {
            ADD_BASE_OUTPUTS_ENTITY_ITER(),
            { 0 }
        };

        // Fill in configuration
        if (!GetCustomConfiguration(config))
        {
            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Base iterator");
            config.nFlags |= EFLN_ACTIVATION_INPUT;
            config.SetCategory(EFLN_APPROVED);
        }
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                m_List.clear();
                m_ListIter = m_List.end();
                m_Iter = 0;
                m_Count = 0;

                OnIterStart(pActInfo);
                m_ListIter = m_List.begin();

                if (m_Count <= 0)
                {
                    // None found
                    ActivateOutput(pActInfo, OutputPorts::Done, 0);
                }
                else
                {
                    const bool bImmediate = GetPortBool(pActInfo, InputPorts::Immediate);
                    const bool bFirstResult = SendNext(pActInfo);
                    if (bImmediate && bFirstResult)
                    {
                        pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
                    }
                }
            }
            else if (IsPortActive(pActInfo, InputPorts::Next))
            {
                const bool bImmediate = GetPortBool(pActInfo, InputPorts::Immediate);
                if (!bImmediate)
                {
                    SendNext(pActInfo);
                }
            }
            break;
        }

        case eFE_Update:
        {
            if (!SendNext(pActInfo))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            }
            break;
        }
        }
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowBaseIteratorNode(pActInfo);
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void AddEntity(SActivationInfo* pActInfo, FlowEntityId id)
    {
        if (FilterByArchetype(pActInfo, id))
        {
            m_List.push_back(id);
            ++m_Count;
        }
    }

    bool SendNext(SActivationInfo* pActInfo)
    {
        bool bResult = false;

        if (m_Count > 0)
        {
            if (m_Iter < m_Count)
            {
                const int limit = GetPortInt(pActInfo, InputPorts::Limit);
                if (limit == 0 || m_Iter <= limit)
                {
                    OnIterNext(pActInfo);

                    FlowEntityId id = *m_ListIter;
                    ActivateOutput(pActInfo, OutputPorts::OutEntityId, id);
                    ActivateOutput(pActInfo, OutputPorts::Count, m_Iter);

                    ++m_Iter;
                    ++m_ListIter;

                    bResult = true;
                }
            }
        }

        if (!bResult)
        {
            ActivateOutput(pActInfo, OutputPorts::Done, m_Iter);
        }

        return bResult;
    }

private:

    bool FilterByArchetype(SActivationInfo* pActInfo, EntityId id)
    {
        const string& archetypeFilter = GetPortString(pActInfo, InputPorts::ArchetypeFilter);

        if (archetypeFilter == "")
        {
            return true;
        }

        IEntity* entity = gEnv->pEntitySystem->GetEntity(id);

        if (entity == nullptr)
        {
            return false;
        }

        IEntityArchetype* archetype = entity->GetArchetype();

        return archetype != nullptr && archetype->GetName() == archetypeFilter;
    }

protected:
    enum InputPorts
    {
        Activate,
        Next,
        Limit,
        Immediate,
        Type,
        ArchetypeFilter,

        CustomInputsStart,
    };

    enum OutputPorts
    {
        OutEntityId,
        Count,
        Done,

        CustomOutputsStart,
    };

    typedef std::list<FlowEntityId> IterList;
    IterList m_List;
    IterList::iterator m_ListIter;
    int m_Count;
    int m_Iter;
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_GetEntitiesInSphere
    : public CFlowBaseIteratorNode
{
    enum CustomInputPorts
    {
        Center = InputPorts::CustomInputsStart,
        Radius,
    };

public:
    CFlowNode_GetEntitiesInSphere(SActivationInfo* pActInfo)
        : CFlowBaseIteratorNode(pActInfo)
    {
    }

    ~CFlowNode_GetEntitiesInSphere() override
    {
    }

    bool GetCustomConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs [] =
        {
            ADD_BASE_INPUTS_ENTITY_ITER(),
            InputPortConfig<Vec3>("Center", _HELP("Center point of sphere")),
            InputPortConfig<float>("Radius", 0.f, _HELP("Radius of sphere (distance from [Center]) to check for Entities")),
            { 0 }
        };

        static const SOutputPortConfig outputs [] =
        {
            ADD_BASE_OUTPUTS_ENTITY_ITER(),
            { 0 }
        };

        // Fill in configuration
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Finds and returns all entities that are inside the defined sphere.  Use [Activate] to start iteration.");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);

        return true;
    }

    void OnIterStart(SActivationInfo* pActInfo) override
    {
        const int type = GetPortInt(pActInfo, InputPorts::Type);
        const Vec3& center(GetPortVec3(pActInfo, CustomInputPorts::Center));
        const float range = GetPortFloat(pActInfo, CustomInputPorts::Radius);
        const float rangeSq = range * range;

        const Vec3 min(center.x - range, center.y - range, center.z - range);
        const Vec3 max(center.x + range, center.y + range, center.z + range);

        IPhysicalWorld* pWorld = gEnv->pPhysicalWorld;
        IPhysicalEntity** ppList = nullptr;
        int numEnts = pWorld->GetEntitiesInBox(min, max, ppList, ent_all);
        for (int i = 0; i < numEnts; ++i)
        {
            IEntity* entity = gEnv->pEntitySystem->GetEntityFromPhysics(ppList[i]);

            if (entity == nullptr)
            {
                continue;
            }

            FlowEntityId id = FlowEntityId(entity->GetId());

            const EEntityType entityType = GetEntityType(id);
            if (IsValidType(type, entityType))
            {
                AddEntity(pActInfo, id);
            }
        }
    }

    void OnIterNext(SActivationInfo* pActInfo) override
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_GetEntitiesInSphere(pActInfo);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_GetEntitiesInBox
    : public CFlowBaseIteratorNode
{
    enum CustomInputPorts
    {
        Min = InputPorts::CustomInputsStart,
        Max,
    };

public:
    CFlowNode_GetEntitiesInBox(SActivationInfo* pActInfo)
        : CFlowBaseIteratorNode(pActInfo)
    {
    }

    ~CFlowNode_GetEntitiesInBox() override
    {
    }

    bool GetCustomConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs [] =
        {
            ADD_BASE_INPUTS_ENTITY_ITER(),
            InputPortConfig<Vec3>("Min", _HELP("Minimum extents of AABB to test against")),
            InputPortConfig<Vec3>("Max", _HELP("Maximum extents of AABB to test against")),
            { 0 }
        };

        static const SOutputPortConfig outputs [] =
        {
            ADD_BASE_OUTPUTS_ENTITY_ITER(),
            { 0 }
        };

        // Fill in configuration
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Finds and returns all Entities inside the defined AABB.  Use [Activate] to start iteration.");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);

        return true;
    }

    void OnIterStart(SActivationInfo* pActInfo) override
    {
        const int type = GetPortInt(pActInfo, InputPorts::Type);
        const Vec3& min(GetPortVec3(pActInfo, CustomInputPorts::Min));
        const Vec3& max(GetPortVec3(pActInfo, CustomInputPorts::Max));

        IPhysicalWorld* pWorld = gEnv->pPhysicalWorld;
        IPhysicalEntity** ppList = nullptr;
        int numEnts = pWorld->GetEntitiesInBox(min, max, ppList, ent_all);
        for (int i = 0; i < numEnts; ++i)
        {
            const FlowEntityId id = FlowEntityId(pWorld->GetPhysicalEntityId(ppList[i]));
            const EEntityType entityType = GetEntityType(id);
            if (IsValidType(type, entityType))
            {
                AddEntity(pActInfo, id);
            }
        }
    }

    void OnIterNext(SActivationInfo* pActInfo) override
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_GetEntitiesInBox(pActInfo);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_GetEntitiesInArea
    : public CFlowBaseIteratorNode
{
    enum CustomInputPorts
    {
        Area = InputPorts::CustomInputsStart,
    };

public:
    CFlowNode_GetEntitiesInArea(SActivationInfo* pActInfo)
        : CFlowBaseIteratorNode(pActInfo)
    {
    }

    ~CFlowNode_GetEntitiesInArea() override
    {
    }

    bool GetCustomConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs [] =
        {
            ADD_BASE_INPUTS_ENTITY_ITER(),
            InputPortConfig<string>("Area", _HELP("Name of area shape to test against"), 0, 0),
            { 0 }
        };

        static const SOutputPortConfig outputs [] =
        {
            ADD_BASE_OUTPUTS_ENTITY_ITER(),
            { 0 }
        };

        // Fill in configuration
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Finds and returns all Entities inside the area shape with the given name.  Use [Activate] to start iteration.");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);

        return true;
    }

    void OnIterStart(SActivationInfo* pActInfo) override
    {
        const int type = GetPortInt(pActInfo, InputPorts::Type);
        const char* area = GetPortString(pActInfo, CustomInputPorts::Area);

        // Find the entity
        IEntitySystem* pEntitySystem = gEnv->pEntitySystem;
        if (pEntitySystem)
        {
            IEntity* pArea = pEntitySystem->FindEntityByName(area);
            if (pArea)
            {
                IComponentAreaPtr pAreaComponent = pArea->GetComponent<IComponentArea>();
                if (pAreaComponent)
                {
                    Vec3 min, max, worldPos(pArea->GetWorldPos());
                    min.Set(0.f, 0.f, 0.f);
                    max.Set(0.f, 0.f, 0.f);
                    EEntityAreaType areaType = pAreaComponent->GetAreaType();

                    // Construct bounding space around area
                    switch (areaType)
                    {
                    case ENTITY_AREA_TYPE_BOX:
                    {
                        pAreaComponent->GetBox(min, max);
                        min += worldPos;
                        max += worldPos;
                        break;
                    }
                    case ENTITY_AREA_TYPE_SPHERE:
                    {
                        Vec3 center;
                        float radius = 0.f;
                        pAreaComponent->GetSphere(center, radius);

                        min.Set(center.x - radius, center.y - radius, center.z - radius);
                        max.Set(center.x + radius, center.y + radius, center.z + radius);
                        break;
                    }
                    case ENTITY_AREA_TYPE_SHAPE:
                    {
                        const Vec3* points = pAreaComponent->GetPoints();
                        const int count = pAreaComponent->GetPointsCount();
                        if (count > 0)
                        {
                            Vec3 p = worldPos + points[0];
                            min = p;
                            max = p;
                            for (int i = 1; i < count; ++i)
                            {
                                p = worldPos + points[i];
                                if (p.x < min.x)
                                {
                                    min.x = p.x;
                                }
                                if (p.y < min.y)
                                {
                                    min.y = p.y;
                                }
                                if (p.z < min.z)
                                {
                                    min.z = p.z;
                                }
                                if (p.x > max.x)
                                {
                                    max.x = p.x;
                                }
                                if (p.y > max.y)
                                {
                                    max.y = p.y;
                                }
                                if (p.z > max.z)
                                {
                                    max.z = p.z;
                                }
                            }
                        }
                        break;
                    }
                    }

                    IPhysicalWorld* pWorld = gEnv->pPhysicalWorld;
                    IPhysicalEntity** ppList = nullptr;
                    int numEnts = pWorld->GetEntitiesInBox(min, max, ppList, ent_all);
                    for (int i = 0; i < numEnts; ++i)
                    {
                        const FlowEntityId id = FlowEntityId(pWorld->GetPhysicalEntityId(ppList[i]));
                        const EEntityType entityType = GetEntityType(id);
                        if (IsValidType(type, entityType))
                        {
                            // Sanity check - Test entity's position
                            IEntity* pEntity = pEntitySystem->GetEntity(id);
                            if (pEntity && pAreaComponent->CalcPointWithin(id, pEntity->GetWorldPos(), pAreaComponent->GetHeight() == 0))
                            {
                                AddEntity(pActInfo, id);
                            }
                        }
                    }
                }
            }
        }
    }

    void OnIterNext(SActivationInfo* pActInfo) override
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_GetEntitiesInArea(pActInfo);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_GetEntities
    : public CFlowBaseIteratorNode
{
public:
    CFlowNode_GetEntities(SActivationInfo* pActInfo)
        : CFlowBaseIteratorNode(pActInfo)
    {
    }

    ~CFlowNode_GetEntities() override
    {
    }

    bool GetCustomConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs [] =
        {
            ADD_BASE_INPUTS_ENTITY_ITER(),
            { 0 }
        };

        static const SOutputPortConfig outputs [] =
        {
            ADD_BASE_OUTPUTS_ENTITY_ITER(),
            { 0 }
        };

        // Fill in configuration
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Finds and returns all Entities in the world.  Use [Activate] to start iteration.");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);

        return true;
    }

    void OnIterStart(SActivationInfo* pActInfo) override
    {
        const int type = GetPortInt(pActInfo, InputPorts::Type);

        IEntitySystem* pEntitySystem = gEnv->pEntitySystem;
        if (pEntitySystem)
        {
            IEntityItPtr iter = pEntitySystem->GetEntityIterator();
            if (iter)
            {
                iter->MoveFirst();
                IEntity* pEntity = nullptr;
                while (!iter->IsEnd())
                {
                    pEntity = iter->Next();
                    if (pEntity)
                    {
                        const FlowEntityId id = FlowEntityId(pEntity->GetId());
                        const EEntityType entityType = GetEntityType(id);
                        if (IsValidType(type, entityType))
                        {
                            AddEntity(pActInfo, id);
                        }
                    }
                }
            }
        }
    }

    void OnIterNext(SActivationInfo* pActInfo) override
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_GetEntities(pActInfo);
    }
};


//////////////////////////////////////////////////////////////////////////
/// Iterator Flow Node Registration
//////////////////////////////////////////////////////////////////////////

REGISTER_FLOW_NODE("Iterator:GetEntitiesInSphere", CFlowNode_GetEntitiesInSphere);
REGISTER_FLOW_NODE("Iterator:GetEntitiesInBox", CFlowNode_GetEntitiesInBox);
REGISTER_FLOW_NODE("Iterator:GetEntitiesInArea", CFlowNode_GetEntitiesInArea);
REGISTER_FLOW_NODE("Iterator:GetEntities", CFlowNode_GetEntities);
