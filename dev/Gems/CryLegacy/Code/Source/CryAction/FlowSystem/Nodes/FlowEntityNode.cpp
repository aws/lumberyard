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
#include "FlowEntityNode.h"

#include <AzCore/Component/TransformBus.h>
#include <IActorSystem.h>
#include <IGameRulesSystem.h>
#include "CryAction.h"
#include "IEntityPoolManager.h"
#include "Components/IComponentScript.h"
#include "Components/IComponentPhysics.h"
#include "Components/IComponentRender.h"
#include <AzFramework/Physics/PhysicsComponentBus.h>

//////////////////////////////////////////////////////////////////////////
CFlowEntityClass::CFlowEntityClass(IEntityClass* pEntityClass)
{
    m_nRefCount = 0;
    //m_classname = pEntityClass->GetName();
    m_pEntityClass = pEntityClass;
}

//////////////////////////////////////////////////////////////////////////
CFlowEntityClass::~CFlowEntityClass()
{
    Reset();
}

//////////////////////////////////////////////////////////////////////////
void CFlowEntityClass::FreeStr(const char* src)
{
    if (src)
    {
        delete []src;
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowEntityClass::Reset()
{
    for (std::vector<SInputPortConfig>::iterator it = m_inputs.begin(), itEnd = m_inputs.end(); it != itEnd; ++it)
    {
        FreeStr(it->name);
    }

    for (std::vector<SOutputPortConfig>::iterator it = m_outputs.begin(), itEnd = m_outputs.end(); it != itEnd; ++it)
    {
        FreeStr(it->name);
    }

    stl::free_container(m_inputs);
    stl::free_container(m_outputs);
}

//////////////////////////////////////////////////////////////////////////
const char* CFlowEntityClass::CopyStr(const char* src)
{
    char* dst = 0;
    if (src)
    {
        int len = strlen(src);
        dst = new char[len + 1];
        azstrcpy(dst, len + 1, src);
    }
    return dst;
}

//////////////////////////////////////////////////////////////////////////
void MakeHumanName(SInputPortConfig& config)
{
    char* name = strchr((char*)config.name, '_');
    if (name != 0)
    {
        config.humanName = name + 1;
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowEntityClass::GetInputsOutputs(IEntityClass* pEntityClass)
{
    int nEvents = pEntityClass->GetEventCount();
    for (int i = 0; i < nEvents; i++)
    {
        IEntityClass::SEventInfo event = pEntityClass->GetEventInfo(i);
        if (event.bOutput)
        {
            // Output
            switch (event.type)
            {
            case IEntityClass::EVT_BOOL:
                m_outputs.push_back(OutputPortConfig<bool>(CopyStr(event.name)));
                break;
            case IEntityClass::EVT_INT:
                m_outputs.push_back(OutputPortConfig<int>(CopyStr(event.name)));
                break;
            case IEntityClass::EVT_FLOAT:
                m_outputs.push_back(OutputPortConfig<float>(CopyStr(event.name)));
                break;
            case IEntityClass::EVT_VECTOR:
                m_outputs.push_back(OutputPortConfig<Vec3>(CopyStr(event.name)));
                break;
            case IEntityClass::EVT_ENTITY:
                m_outputs.push_back(OutputPortConfig<FlowEntityId>(CopyStr(event.name)));
                break;
            case IEntityClass::EVT_STRING:
                m_outputs.push_back(OutputPortConfig<string>(CopyStr(event.name)));
                break;
            }
        }
        else
        {
            // Input
            switch (event.type)
            {
            case IEntityClass::EVT_BOOL:
                m_inputs.push_back(InputPortConfig<bool>(CopyStr(event.name)));
                MakeHumanName(m_inputs.back());
                break;
            case IEntityClass::EVT_INT:
                m_inputs.push_back(InputPortConfig<int>(CopyStr(event.name)));
                MakeHumanName(m_inputs.back());
                break;
            case IEntityClass::EVT_FLOAT:
                m_inputs.push_back(InputPortConfig<float>(CopyStr(event.name)));
                MakeHumanName(m_inputs.back());
                break;
            case IEntityClass::EVT_VECTOR:
                m_inputs.push_back(InputPortConfig<Vec3>(CopyStr(event.name)));
                MakeHumanName(m_inputs.back());
                break;
            case IEntityClass::EVT_ENTITY:
                m_inputs.push_back(InputPortConfig<FlowEntityId>(CopyStr(event.name)));
                MakeHumanName(m_inputs.back());
                break;
            case IEntityClass::EVT_STRING:
                m_inputs.push_back(InputPortConfig<string>(CopyStr(event.name)));
                MakeHumanName(m_inputs.back());
                break;
            }
        }
    }
    m_outputs.push_back(OutputPortConfig<bool>(0, 0));
    m_inputs.push_back(InputPortConfig<bool>(0, false));
}

//////////////////////////////////////////////////////////////////////////
void CFlowEntityClass::GetConfiguration(SFlowNodeConfig& config)
{
    m_inputs.clear();
    m_outputs.clear();
    GetInputsOutputs(m_pEntityClass);


    //if (m_inputs.empty() && m_outputs.empty())
    //  GetInputsOutputs( m_pEntityClass );

    static const SInputPortConfig in_config[] = {
        {0}
    };
    static const SOutputPortConfig out_config[] = {
        {0}
    };

    config.nFlags |= EFLN_TARGET_ENTITY | EFLN_HIDE_UI;
    config.SetCategory(EFLN_APPROVED); // all Entity flownodes are approved!

    config.pInputPorts = in_config;
    config.pOutputPorts = out_config;

    if (!m_inputs.empty())
    {
        config.pInputPorts = &m_inputs[0];
    }

    if (!m_outputs.empty())
    {
        config.pOutputPorts = &m_outputs[0];
    }
}

//////////////////////////////////////////////////////////////////////////
IFlowNodePtr CFlowEntityClass::Create(IFlowNode::SActivationInfo* pActInfo)
{
    return new CFlowEntityNode(this, pActInfo);
}

//////////////////////////////////////////////////////////////////////////
// CFlowEntityNode implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CFlowEntityNode::CFlowEntityNode(CFlowEntityClass* pClass, SActivationInfo* pActInfo)
{
    m_event = ENTITY_EVENT_SCRIPT_EVENT;
    m_pClass = pClass;
    //pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
    //  m_entityId = (FlowEntityId)pActInfo->m_pUserData;

    m_nodeID = pActInfo->myID;
    m_pGraph = pActInfo->pGraph;
    m_lastInitializeFrameId = -1;
}

IFlowNodePtr CFlowEntityNode::Clone(SActivationInfo* pActInfo)
{
    IFlowNode* pNode = new CFlowEntityNode(m_pClass, pActInfo);
    return pNode;
};

//////////////////////////////////////////////////////////////////////////
void CFlowEntityNode::GetConfiguration(SFlowNodeConfig& config)
{
    m_pClass->GetConfiguration(config);
}

//////////////////////////////////////////////////////////////////////////
void CFlowEntityNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    CFlowEntityNodeBase::ProcessEvent(event, pActInfo);

    switch (event)
    {
    case eFE_Activate:
    {
        IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
        if (!pEntity)
        {
            if (gEnv->pEntitySystem->GetIEntityPoolManager()->IsEntityBookmarked(m_entityId))
            {
                const char* entityName = gEnv->pEntitySystem->GetIEntityPoolManager()->GetBookmarkedEntityName(m_entityId);

                GameWarning("FlowEntityNode event called on entity '%s' (id: %08x) which is created through pool, but not spawn yet!\n"
                    "Please change the logic to only send these events after the entity has been created from the pool.",
                    entityName ? entityName : "<null>", m_entityId);
            }

            return;
        }

        IComponentScriptPtr scriptComponent = pEntity->GetComponent<IComponentScript>();
        if (!scriptComponent)
        {
            return;
        }

        // Check active ports, and send event to entity.
        for (int i = 0; i < m_pClass->m_inputs.size() - 1; i++)
        {
            if (IsPortActive(pActInfo, i))
            {
                EFlowDataTypes type = GetPortType(pActInfo, i);
                switch (type)
                {
                case eFDT_Int:
                    scriptComponent->CallEvent(m_pClass->m_inputs[i].name, (float)GetPortInt(pActInfo, i));
                    break;
                case eFDT_Float:
                    scriptComponent->CallEvent(m_pClass->m_inputs[i].name, GetPortFloat(pActInfo, i));
                    break;
                case eFDT_Double:
                    scriptComponent->CallEvent(m_pClass->m_inputs[i].name, GetPortDouble(pActInfo, i));
                    break;
                case eFDT_Bool:
                    scriptComponent->CallEvent(m_pClass->m_inputs[i].name, GetPortBool(pActInfo, i));
                    break;
                case eFDT_Vec3:
                    scriptComponent->CallEvent(m_pClass->m_inputs[i].name, GetPortVec3(pActInfo, i));
                    break;
                case eFDT_EntityId:
                    scriptComponent->CallEvent(m_pClass->m_inputs[i].name, FlowEntityId(GetPortEntityId(pActInfo, i)));
                    break;
                case eFDT_String:
                    scriptComponent->CallEvent(m_pClass->m_inputs[i].name, GetPortString(pActInfo, i).c_str());
                    break;
                case eFDT_CustomData:
                    // not currently supported
                    break;
                }
            }
        }
    }
    break;
    case eFE_Initialize:
        if (gEnv->IsEditor() && m_entityId)
        {
            UnregisterEvent(m_event);
            RegisterEvent(m_event);
        }
        const int frameId = gEnv->pRenderer->GetFrameID(false);
        if (frameId != m_lastInitializeFrameId)
        {
            m_lastInitializeFrameId = frameId;
            for (size_t i = 0; i < m_pClass->GetNumOutputs(); i++)
            {
                ActivateOutput(pActInfo, i, SFlowSystemVoid());
            }
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CFlowEntityNode::SerializeXML(SActivationInfo*, const XmlNodeRef& root, bool reading)
{
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CFlowEntityNode::Serialize(SActivationInfo*, TSerialize ser)
{
    if (ser.IsReading())
    {
        m_lastInitializeFrameId = -1;
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowEntityNode::OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
{
    if (!m_pGraph->IsEnabled() || m_pGraph->IsSuspended() || !m_pGraph->IsActive())
    {
        return;
    }

    switch (event.event)
    {
    case ENTITY_EVENT_DONE:
        // The entity being destroyed is not always the entity stored in the input of the node (which is what actually is Get/Set by those functions ).
        // In the case of AIWaves, when the node is forwarded to newly spawned entities, this event will be called when those spawned entities are destroyed. But
        //  but the input will still be the original entityId. And should not be zeroed, or the node will stop working after a reset (this only can happen on editor mode)
        if (m_pGraph->GetEntityId(m_nodeID) == pEntity->GetId())
        {
            m_pGraph->SetEntityId(m_nodeID, FlowEntityId());
        }
        break;

    case ENTITY_EVENT_SCRIPT_EVENT:
    {
        const char* sEvent = (const char*)event.nParam[0];
        // Find output port.
        for (int i = 0; i < m_pClass->m_outputs.size(); i++)
        {
            if (!m_pClass->m_outputs[i].name)
            {
                break;
            }
            if (strcmp(sEvent, m_pClass->m_outputs[i].name) == 0)
            {
                SFlowAddress addr(m_nodeID, i, true);
                switch (event.nParam[1])
                {
                case IEntityClass::EVT_INT:
                    assert(event.nParam[2] && "Attempt to activate FlowGraph port of type int without value");
                    m_pGraph->ActivatePort(addr, *(int*)event.nParam[2]);
                    break;
                case IEntityClass::EVT_FLOAT:
                    assert(event.nParam[2] && "Attempt to activate FlowGraph port of type float without value");
                    m_pGraph->ActivatePort(addr, *(float*)event.nParam[2]);
                    break;
                case IEntityClass::EVT_BOOL:
                    assert(event.nParam[2] && "Attempt to activate FlowGraph port of type bool without value");
                    m_pGraph->ActivatePort(addr, *(bool*)event.nParam[2]);
                    break;
                case IEntityClass::EVT_VECTOR:
                    assert(event.nParam[2] && "Attempt to activate FlowGraph port of type Vec3 without value");
                    m_pGraph->ActivatePort(addr, *(Vec3*)event.nParam[2]);
                    break;
                case IEntityClass::EVT_ENTITY:
                    assert(event.nParam[2] && "Attempt to activate FlowGraph port of type FlowEntityId without value");
                    m_pGraph->ActivatePort(addr, FlowEntityId((*(EntityId*)event.nParam[2])));
                    break;
                case IEntityClass::EVT_STRING:
                {
                    assert(event.nParam[2] && "Attempt to activate FlowGraph port of type string without value");
                    const char* str = (const char*)event.nParam[2];
                    m_pGraph->ActivatePort(addr, string(str));
                }
                break;
                }
                //m_pGraph->ActivatePort( addr,(bool)true );
                break;
            }
        }
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
// Flow node for entity position.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityPos
    : public CFlowEntityNodeBase
{
public:
    enum EInputs
    {
        IN_POS,
        IN_ROTATE,
        IN_SCALE,
        IN_COORDSYS,
    };
    enum EOutputs
    {
        OUT_POS,
        OUT_ROTATE,
        OUT_SCALE,
        OUT_YDIR,
        OUT_XDIR,
        OUT_ZDIR,
    };
    enum ECoordSys
    {
        CS_PARENT = 0,
        CS_WORLD,
    };

    enum
    {
        ALL_VALUES = ENTITY_XFORM_POS | ENTITY_XFORM_ROT | ENTITY_XFORM_SCL
    };

    CFlowNode_EntityPos(SActivationInfo* pActInfo)
        : CFlowEntityNodeBase()
        , m_pGraph(NULL)
        , m_nodeName(NULL)
    {
        m_event = ENTITY_EVENT_XFORM;
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { pActInfo->m_pUserData = (void*)(UINT_PTR)m_entityId; return new CFlowNode_EntityPos(pActInfo); };
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<Vec3>("pos", _HELP("Entity position vector")),
            InputPortConfig<Vec3>("rotate", _HELP("Entity rotation angle in degrees")),
            InputPortConfig<Vec3>("scale", _HELP("Entity scale vector")),
            InputPortConfig<int> ("CoordSys", 1, _HELP("In which coordinate system the values are expressed"), _HELP("CoordSys"), _UICONFIG("enum_int:Parent=0,World=1")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<Vec3>("pos", _HELP("Entity position vector")),
            OutputPortConfig<Vec3>("rotate", _HELP("Entity rotation angle in degrees")),
            OutputPortConfig<Vec3>("scale", _HELP("Entity scale vector")),
            OutputPortConfig<Vec3>("fwdDir", _HELP("Entity direction vector - Y axis")),
            OutputPortConfig<Vec3>("rightDir", _HELP("Entity direction vector - X axis")),
            OutputPortConfig<Vec3>("upDir", _HELP("Entity direction vector - Z axis")),
            {0}
        };
        config.sDescription = _HELP("Entity Position/Rotation/Scale");
        config.nFlags |= EFLN_TARGET_ENTITY | EFLN_AISEQUENCE_SUPPORTED;
        if (!ShouldShowInEditorList())
        {
            config.nFlags |= EFLN_HIDE_UI;
        }
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        CFlowEntityNodeBase::ProcessEvent(event, pActInfo);

        IEntity* pEntity = GetEntity();
        if (!pEntity)
        {
            return;
        }

        switch (event)
        {
        case eFE_Activate:
        {
            ECoordSys coorSys = (ECoordSys)GetPortInt(pActInfo, IN_COORDSYS);

            if (IsPortActive(pActInfo, IN_POS))
            {
                const Vec3* v = pActInfo->pInputPorts[IN_POS].GetPtr<Vec3>();
                if (coorSys == CS_WORLD)
                {
                    Matrix34 tm = pEntity->GetWorldTM();
                    tm.SetTranslation(*v);
                    pEntity->SetWorldTM(tm);
                }
                else
                {
                    Matrix34 tm = pEntity->GetLocalTM();
                    tm.SetTranslation(*v);
                    pEntity->SetLocalTM(tm);
                }
            }
            if (IsPortActive(pActInfo, IN_ROTATE))
            {
                const Vec3* v = pActInfo->pInputPorts[IN_ROTATE].GetPtr<Vec3>();
                Matrix34 tm = Matrix33(Quat::CreateRotationXYZ(Ang3(DEG2RAD(*v))));

                // Preserve the scale of the original transform matrix.
                Vec3 entityScale = pEntity->GetScale();

                if (coorSys == CS_WORLD)
                {
                    tm.SetTranslation(pEntity->GetWorldPos());
                    pEntity->SetWorldTM(tm);
                }
                else
                {
                    tm.SetTranslation(pEntity->GetPos());
                    pEntity->SetLocalTM(tm);
                }

                pEntity->SetScale(entityScale);
            }
            if (IsPortActive(pActInfo, IN_SCALE))
            {
                const Vec3* v = pActInfo->pInputPorts[IN_SCALE].GetPtr<Vec3>();
                Vec3 scale = *v;
                if (scale.x == 0)
                {
                    scale.x = 1.0f;
                }
                if (scale.y == 0)
                {
                    scale.y = 1.0f;
                }
                if (scale.z == 0)
                {
                    scale.z = 1.0f;
                }
                pEntity->SetScale(scale);
            }
        }
        break;
        case eFE_Initialize:
        {
            m_pGraph = pActInfo->pGraph;
            m_nodeName = m_pGraph->GetNodeName(pActInfo->myID);
            OutputValues(pActInfo, ALL_VALUES);
        }
        break;

        case eFE_SetEntityId:
            OutputValues(pActInfo, ALL_VALUES);
            break;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // IEntityEventListener
    virtual void OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
    {
        if (!m_pGraph || !m_pGraph->IsEnabled() || m_pGraph->IsSuspended() || !m_pGraph->IsActive())
        {
            return;
        }

        switch (event.event)
        {
        case ENTITY_EVENT_XFORM:
        {
            IFlowNode::SActivationInfo actInfo;
            if (!m_pGraph->GetActivationInfo(m_nodeName, actInfo))
            {
                return;
            }

            OutputValues(&actInfo, uint32(event.nParam[0]));
        }
        break;
        }
    }

    void OutputValues(IFlowNode::SActivationInfo* pActInfo, uint entityFlags)
    {
        ECoordSys coorSys = (ECoordSys)GetPortInt(pActInfo, IN_COORDSYS);

        bool bAny = (entityFlags & (ENTITY_XFORM_POS | ENTITY_XFORM_ROT | ENTITY_XFORM_SCL)) == 0;
        IEntity* pEntity = pActInfo->pEntity;
        if (!pEntity)
        {
            return;
        }

        switch (coorSys)
        {
        case CS_WORLD:
        {
            if (bAny || entityFlags & ENTITY_XFORM_POS)
            {
                ActivateOutput(pActInfo, OUT_POS, pEntity->GetWorldPos());
            }

            if (bAny || entityFlags & ENTITY_XFORM_ROT)
            {
                ActivateOutput(pActInfo, OUT_ROTATE, Vec3(RAD2DEG(Ang3(pEntity->GetWorldAngles()))));
                const Matrix34& mat = pEntity->GetWorldTM();
                ActivateOutput(pActInfo, OUT_YDIR, mat.GetColumn1());     // forward -> mat.TransformVector(Vec3(0,1,0)) );
                ActivateOutput(pActInfo, OUT_XDIR, mat.GetColumn0());     // right   -> mat.TransformVector(Vec3(1,0,0)) );
                ActivateOutput(pActInfo, OUT_ZDIR, mat.GetColumn2());     // up      -> mat.TransformVector(Vec3(0,0,1)) );
            }

            if (bAny || entityFlags & ENTITY_XFORM_SCL)
            {
                ActivateOutput(pActInfo, OUT_SCALE, pEntity->GetScale());
            }
            break;
        }

        case CS_PARENT:
        {
            if (bAny || entityFlags & ENTITY_XFORM_POS)
            {
                ActivateOutput(pActInfo, OUT_POS, pEntity->GetPos());
            }

            if (bAny || entityFlags & ENTITY_XFORM_ROT)
            {
                Matrix34 localTM = pEntity->GetLocalTM();
                localTM.OrthonormalizeFast();
                ActivateOutput(pActInfo, OUT_ROTATE, Vec3(RAD2DEG(Ang3::GetAnglesXYZ(localTM))));
                const Matrix34& mat = pEntity->GetLocalTM();
                ActivateOutput(pActInfo, OUT_YDIR, mat.GetColumn1());     // forward -> mat.TransformVector(Vec3(0,1,0)) );
                ActivateOutput(pActInfo, OUT_XDIR, mat.GetColumn0());     // right   -> mat.TransformVector(Vec3(1,0,0)) );
                ActivateOutput(pActInfo, OUT_ZDIR, mat.GetColumn2());     // up      -> mat.TransformVector(Vec3(0,0,1)) );
            }

            if (bAny || entityFlags & ENTITY_XFORM_SCL)
            {
                ActivateOutput(pActInfo, OUT_SCALE, pEntity->GetScale());
            }
            break;
        }
        }
    }

    virtual bool ShouldShowInEditorList() { return true; }


    //////////////////////////////////////////////////////////////////////////

protected:
    IFlowGraph* m_pGraph;
    const char* m_nodeName;
};

//////////////////////////////////////////////////////////////////////////
// Flow node for entity tagpoint.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityTagpoint
    : public CFlowNode_EntityPos
{
public:
    CFlowNode_EntityTagpoint(SActivationInfo* pActInfo)
        : CFlowNode_EntityPos(pActInfo)
    {
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { pActInfo->m_pUserData = (void*)(UINT_PTR)m_entityId; return new CFlowNode_EntityTagpoint(pActInfo); }

protected:
    virtual bool ShouldShowInEditorList() { return false; }
};

//////////////////////////////////////////////////////////////////////////
// Flow node for entity position.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityGetPos
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    enum EInputs
    {
        IN_GET,
        IN_COORDSYS,
    };
    enum EOutputs
    {
        OUT_POS = 0,
        OUT_ROTATE,
        OUT_SCALE,
        OUT_YDIR,
        OUT_XDIR,
        OUT_ZDIR,
    };

    enum ECoordSys
    {
        CS_PARENT = 0,
        CS_WORLD,
    };

    CFlowNode_EntityGetPos(SActivationInfo* pActInfo)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Get", _HELP("Trigger to get current values")),
            InputPortConfig<int> ("CoordSys", 1, _HELP("In which coordinate system the values are expressed"), _HELP("CoordSys"), _UICONFIG("enum_int:Parent=0,World=1")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<Vec3>("Pos", _HELP("Entity position vector")),
            OutputPortConfig<Vec3>("Rotate", _HELP("Entity rotation angle in degrees")),
            OutputPortConfig<Vec3>("Scale", _HELP("Entity scale vector")),
            OutputPortConfig<Vec3>("FwdDir", _HELP("Entity direction vector - Y axis")),
            OutputPortConfig<Vec3>("RightDir", _HELP("Entity direction vector - X axis")),
            OutputPortConfig<Vec3>("UpDir", _HELP("Entity direction vector - Z axis")),
            {0}
        };
        config.sDescription = _HELP("Get Entity Position/Rotation/Scale");
        config.nFlags |= EFLN_TARGET_ENTITY | EFLN_AISEQUENCE_SUPPORTED;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event != eFE_Activate || IsPortActive(pActInfo, IN_GET) == false)
        {
            return;
        }
        // only if IN_GET is activated.

        IEntity* pEntity = pActInfo->pEntity;
        if (pEntity == 0)
        {
            return;
        }

        ECoordSys coorSys = (ECoordSys)GetPortInt(pActInfo, IN_COORDSYS);


        switch (coorSys)
        {
        case CS_WORLD:
        {
            ActivateOutput(pActInfo, OUT_POS, pEntity->GetWorldPos());
            ActivateOutput(pActInfo, OUT_ROTATE, Vec3(RAD2DEG(Ang3(pEntity->GetWorldAngles()))));
            ActivateOutput(pActInfo, OUT_SCALE, pEntity->GetScale());

            const Matrix34& mat = pEntity->GetWorldTM();
            ActivateOutput(pActInfo, OUT_YDIR, mat.GetColumn1());      // forward -> mat.TransformVector(Vec3(0,1,0)) );
            ActivateOutput(pActInfo, OUT_XDIR, mat.GetColumn0());      // right   -> mat.TransformVector(Vec3(1,0,0)) );
            ActivateOutput(pActInfo, OUT_ZDIR, mat.GetColumn2());      // up      -> mat.TransformVector(Vec3(0,0,1)) );
            break;
        }

        case CS_PARENT:
        {
            ActivateOutput(pActInfo, OUT_POS, pEntity->GetPos());
            Matrix34 localTM = pEntity->GetLocalTM();
            localTM.OrthonormalizeFast();
            ActivateOutput(pActInfo, OUT_ROTATE, Vec3(RAD2DEG(Ang3::GetAnglesXYZ(localTM))));
            ActivateOutput(pActInfo, OUT_SCALE, pEntity->GetScale());

            const Matrix34& mat = pEntity->GetLocalTM();
            ActivateOutput(pActInfo, OUT_YDIR, mat.GetColumn1());      // forward -> mat.TransformVector(Vec3(0,1,0)) );
            ActivateOutput(pActInfo, OUT_XDIR, mat.GetColumn0());      // right   -> mat.TransformVector(Vec3(1,0,0)) );
            ActivateOutput(pActInfo, OUT_ZDIR, mat.GetColumn2());      // up      -> mat.TransformVector(Vec3(0,0,1)) );
            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};



//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityCheckDistance
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    enum EInputs
    {
        INP_CHECK,
        INP_MIN_DIST,
        INP_MAX_DIST,
        INP_ENT00,
        INP_ENT01,
        INP_ENT02,
        INP_ENT03,
        INP_ENT04,
        INP_ENT05,
        INP_ENT06,
        INP_ENT07,
        INP_ENT08,
        INP_ENT09,
        INP_ENT10,
        INP_ENT11,
        INP_ENT12,
        INP_ENT13,
        INP_ENT14,
        INP_ENT15,
    };
    enum EOutputs
    {
        OUT_NEAR_ENT = 0,
        OUT_NEAR_ENT_DIST,
        OUT_FAR_ENT,
        OUT_FAR_ENT_DIST,
        OUT_NOENTITIES_IN_RANGE,
    };

    CFlowNode_EntityCheckDistance(SActivationInfo* pActInfo)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Check", _HELP("Trigger to check distances")),
            InputPortConfig<float>("MinDistance", _HELP("Any entity that is nearer than this, will be ignored")),
            InputPortConfig<float>("MaxDistance", _HELP("Any entity that is farther than this, will be ignored (0=no limit)")),
            InputPortConfig<FlowEntityId>("Entity1", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity2", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity3", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity4", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity5", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity6", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity7", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity8", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity9", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity10", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity11", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity12", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity13", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity14", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity15", _HELP("EntityID")),
            InputPortConfig<FlowEntityId>("Entity16", _HELP("EntityID")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<FlowEntityId>("NearEntity", _HELP("Nearest entity")),
            OutputPortConfig<float>("NearEntityDist", _HELP("Nearest entity distance")),
            OutputPortConfig<FlowEntityId>("FarEntity", _HELP("Fartest entity")),
            OutputPortConfig<float>("FarEntityDist", _HELP("Fartest entity distance")),
            OutputPortConfig_AnyType("NoEntInRange", _HELP("Trigered when none of the entities were between Min and Max distance")),
            {0}
        };
        config.sDescription = _HELP("Check distance between the node entity and the entities defined in the inputs");
        config.nFlags |= EFLN_TARGET_ENTITY | EFLN_AISEQUENCE_SUPPORTED;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event != eFE_Activate || !IsPortActive(pActInfo, INP_CHECK))
        {
            return;
        }

        IEntity* pEntityNode = pActInfo->pEntity;
        if (!pEntityNode)
        {
            return;
        }

        float minRangeDist = GetPortFloat(pActInfo, INP_MIN_DIST);
        float maxRangeDist = GetPortFloat(pActInfo, INP_MAX_DIST);
        float minRangeDist2 = minRangeDist * minRangeDist;
        float maxRangeDist2 = maxRangeDist * maxRangeDist;
        if (maxRangeDist2 == 0)
        {
            maxRangeDist2 = FLT_MAX;
        }

        FlowEntityId minEnt = FlowEntityId();
        FlowEntityId maxEnt = FlowEntityId();
        float minDist2 = maxRangeDist2;
        float maxDist2 = minRangeDist2;
        bool anyEntityInRange = false;

        for (uint32 i = INP_ENT00; i <= INP_ENT15; ++i)
        {
            FlowEntityId entityIdCheck = FlowEntityId(GetPortEntityId(pActInfo, i));
            IEntity* pEntityCheck = gEnv->pEntitySystem->GetEntity(entityIdCheck);
            if (pEntityCheck)
            {
                float dist2 = pEntityCheck->GetPos().GetSquaredDistance(pEntityNode->GetPos());
                if (dist2 >= minRangeDist2 && dist2 <= maxRangeDist2)
                {
                    anyEntityInRange = true;
                    if (dist2 <= minDist2)
                    {
                        minDist2 = dist2;
                        minEnt = entityIdCheck;
                    }
                    if (dist2 >= maxDist2)
                    {
                        maxDist2 = dist2;
                        maxEnt = entityIdCheck;
                    }
                }
            }
        }

        if (anyEntityInRange)
        {
            ActivateOutput(pActInfo, OUT_NEAR_ENT, minEnt);
            ActivateOutput(pActInfo, OUT_NEAR_ENT_DIST, sqrtf(minDist2));

            ActivateOutput(pActInfo, OUT_FAR_ENT, maxEnt);
            ActivateOutput(pActInfo, OUT_FAR_ENT_DIST, sqrtf(maxDist2));
        }
        else
        {
            ActivateOutput(pActInfo, OUT_NOENTITIES_IN_RANGE, true);
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
// Flow node to get the AABB of an entity
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityGetBounds
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    enum EInputs
    {
        IN_GET,
        IN_COORDSYS
    };

    enum EOutputs
    {
        OUT_MIN,
        OUT_MAX
    };

    enum ECoordSys
    {
        CS_LOCAL,
        CS_WORLD,
    };

    CFlowNode_EntityGetBounds(SActivationInfo* pActInfo)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Get", _HELP("")),
            InputPortConfig<int>("CoordSys", 1, _HELP("In which coordinate system the values are expressed"), _HELP("CoordSys"), _UICONFIG("enum_int:Local=0,World=1")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<Vec3>("Min", _HELP("Min position of the AABB")),
            OutputPortConfig<Vec3>("Max", _HELP("Max position of the AABB")),
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.nFlags |= EFLN_TARGET_ENTITY | EFLN_AISEQUENCE_SUPPORTED;
        config.sDescription = _HELP("Gets the AABB");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, IN_GET) && pActInfo->pEntity)
            {
                AABB aabb(Vec3(0, 0, 0), Vec3(0, 0, 0));

                switch (GetPortInt(pActInfo, IN_COORDSYS))
                {
                case CS_LOCAL:
                    pActInfo->pEntity->GetLocalBounds(aabb);
                    break;

                case CS_WORLD:
                    pActInfo->pEntity->GetWorldBounds(aabb);
                    break;
                }

                ActivateOutput(pActInfo, OUT_MIN, aabb.min);
                ActivateOutput(pActInfo, OUT_MAX, aabb.max);
            }
            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
// Flow node for controlling the entity look at position.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityFaceAt
    : public CFlowEntityNodeBase
{
public:
    enum EInputs
    {
        IN_TARGET = 0,
        IN_VECTOR,
        IN_START,
        IN_STOP,
        IN_FWD_AXIS,
        IN_REFERENCE,
    };
    enum EOutputs
    {
        OUT_CURRENT = 0,
    };

    enum EFwdAxis
    {
        eFA_XPlus = 0,
        eFA_XMinus,
        eFA_YPlus,
        eFA_YMinus,
        eFA_ZPlus,
        eFA_ZMinus,
    };



    CFlowNode_EntityFaceAt(SActivationInfo* pActInfo)
        : CFlowEntityNodeBase()
        , m_targetPos(0.0f, 0.0f, 0.0f)
        , m_fwdAxis(eFA_XPlus)
        , m_referenceVector(0.0f, 0.0f, 0.0f)
        , m_entityId(0)
    {
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { pActInfo->m_pUserData = (void*)(UINT_PTR)m_entityId; return new CFlowNode_EntityFaceAt(pActInfo); }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<FlowEntityId>("target", _HELP("face direction of target [EntityID]"), _HELP("Target")),
            InputPortConfig<Vec3>("pos", Vec3(ZERO), _HELP("target this position [Vec3]")),
            InputPortConfig<SFlowSystemVoid>("Activate", _HELP("start trigger")),
            InputPortConfig<SFlowSystemVoid>("Deactivate", _HELP("stop trigger")),
            InputPortConfig<int>("FwdDir", eFA_XPlus, _HELP("Axis that will be made to point at the target"), _HELP("FwdDir"), "enum_int:X+=0,X-=1,Y+=2,Y-=3,Z+=4,Z-=5"),
            InputPortConfig<Vec3>("ReferenceVec", Vec3(0.0f, 0.0f, 1.0f), _HELP("This reference vector represents the desired Up (Z+), unless you're using Z+ or Z- as FwdDir, in which case this vector represents the right vector (X+)")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<Vec3>("current", _HELP("the current directional vector")),
            {0}
        };
        config.sDescription = _HELP("Entity Looks At");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void SnapToTarget(SActivationInfo* pActInfo)
    {
        IEntity* pEntity = GetEntity();
        if (!pEntity)
        {
            return;
        }

        IEntity* pLookAtEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
        if (pLookAtEntity)
        {
            m_targetPos = pLookAtEntity->GetWorldPos();
        }

        const Vec3& worldPos = pEntity->GetWorldPos();
        Vec3 facingDirection = (m_targetPos - pEntity->GetPos()).GetNormalized();

        Vec3 xAxis(1.0f, 0.0f, 0.0f);
        Vec3 yAxis(0.0f, 1.0f, 0.0f);
        Vec3 zAxis(0.0f, 0.0f, 1.0f);

        switch (m_fwdAxis)
        {
        case eFA_XMinus:
            facingDirection = -facingDirection;
        case eFA_XPlus:
            xAxis = facingDirection;
            yAxis = m_referenceVector ^ xAxis;
            zAxis = xAxis ^ yAxis;
            break;
        case eFA_YMinus:
            facingDirection = -facingDirection;
        case eFA_YPlus:
            yAxis = facingDirection;
            xAxis = yAxis ^ m_referenceVector;
            zAxis = xAxis ^ yAxis;
            break;
        case eFA_ZMinus:
            facingDirection = -facingDirection;
        case eFA_ZPlus:
            zAxis = facingDirection;
            yAxis = zAxis ^ m_referenceVector;
            xAxis = yAxis ^ zAxis;
            break;
        }

        Matrix34 worldMatrix = Matrix34::CreateFromVectors(xAxis.GetNormalized(), yAxis.GetNormalized(), zAxis.GetNormalized(), worldPos);
        pEntity->SetWorldTM(worldMatrix);

        ActivateOutput(pActInfo, OUT_CURRENT, Vec3(RAD2DEG(Ang3(pEntity->GetRotation()))));
    }

    virtual void OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
    {
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        CFlowEntityNodeBase::ProcessEvent(event, pActInfo);

        IEntity* pEntity = GetEntity();
        if (!pEntity)
        {
            return;
        }

        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, IN_STOP))     // Stop
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            }

            if (IsPortActive(pActInfo, IN_TARGET))
            {
                m_entityId = FlowEntityId(GetPortEntityId(pActInfo, IN_TARGET));
            }
            else if (IsPortActive(pActInfo, IN_VECTOR))
            {
                m_targetPos = GetPortVec3(pActInfo, IN_VECTOR);
            }

            if (IsPortActive(pActInfo, IN_FWD_AXIS))
            {
                m_fwdAxis = static_cast<EFwdAxis>(GetPortInt(pActInfo, IN_FWD_AXIS));
            }

            if (IsPortActive(pActInfo, IN_REFERENCE))
            {
                m_referenceVector = GetPortVec3(pActInfo, IN_REFERENCE);
                m_referenceVector.Normalize();
            }

            if (IsPortActive(pActInfo, IN_START))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            }
            break;
        }

        case eFE_Initialize:
        {
            m_entityId = FlowEntityId(GetPortEntityId(pActInfo, IN_TARGET));
            m_targetPos = GetPortVec3(pActInfo, IN_VECTOR);
            m_fwdAxis = static_cast< EFwdAxis >(GetPortInt(pActInfo, IN_FWD_AXIS));
            m_referenceVector = GetPortVec3(pActInfo, IN_REFERENCE);
            m_referenceVector.Normalize();
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            break;
        }

        case eFE_Update:
        {
            SnapToTarget(pActInfo);
            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

private:
    Vec3 m_targetPos;
    EFwdAxis m_fwdAxis;
    Vec3 m_referenceVector;
    FlowEntityId m_entityId;
};



//////////////////////////////////////////////////////////////////////////
// Flow node for entity material.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityMaterial
    : public CFlowEntityNodeBase
{
public:
    enum EInputs
    {
        IN_SET,
        IN_MATERIAL,
        IN_RESET,
    };
    CFlowNode_EntityMaterial(SActivationInfo* pActInfo)
        : CFlowEntityNodeBase()
    {
        m_pEditorMaterial = 0;
        m_entityId = (EntityId)(UINT_PTR)pActInfo->m_pUserData;
        // m_event = ENTITY_EVENT_MATERIAL;
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { pActInfo->m_pUserData = (void*)(UINT_PTR)m_entityId; return new CFlowNode_EntityMaterial(pActInfo); };
    virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        /*
        if (ser.IsReading())
        {
            bool bChangeMat = false;
            ser.Value("changeMat", bChangeMat);
            IEntity* pEntity = GetEntity();
            if (pEntity && bChangeMat)
            {
                ChangeMat(pActInfo, pEntity);
            }
            else
                m_pMaterial = 0;
        }
        else
        {
            bool bChangeMat = m_pMaterial != 0;
            ser.Value("changeMat", bChangeMat);
        }
        */
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Set", _HELP("Trigger to set the material")),
            InputPortConfig<string>("mat_Material", _HELP("Name of material to apply")),
            InputPortConfig_Void("Reset", _HELP("Trigger to reset the original material")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            {0}
        };
        config.sDescription = _HELP("Apply material to the entity");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_OBSOLETE);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        CFlowEntityNodeBase::ProcessEvent(event, pActInfo);

        switch (event)
        {
        case eFE_Initialize:
        {
            // workaround to the fact that the entity does not clear the material when going back to editor mode,
            // and to the fact that there is not any special event to signal when its going back to editor mode
            if (gEnv->IsEditor())
            {
                IEntity* pEntity = GetEntity();
                if (pEntity)
                {
                    if (gEnv->IsEditing())
                    {
                        m_pEditorMaterial = pEntity->GetMaterial();
                    }
                    else
                    {
                        pEntity->SetMaterial(m_pEditorMaterial);
                    }
                }
            }
            break;
        }

        case eFE_PrecacheResources:
        {
            // Preload target material.
            const string& mtlName = GetPortString(pActInfo, IN_MATERIAL);
            m_pMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(mtlName.c_str());
            break;
        }

        case eFE_Activate:
        {
            IEntity* pEntity = GetEntity();
            if (pEntity)
            {
                if (IsPortActive(pActInfo, IN_SET))
                {
                    ChangeMat(pActInfo, pEntity);
                }

                if (IsPortActive(pActInfo, IN_RESET))
                {
                    pEntity->SetMaterial(NULL);
                }
            }
        }
        break;
        }
    }

    void ChangeMat(SActivationInfo* pActInfo, IEntity* pEntity)
    {
        const string& mtlName = GetPortString(pActInfo, IN_MATERIAL);
        _smart_ptr<IMaterial> pMtl = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(mtlName.c_str());
        if (pMtl != NULL)
        {
            pEntity->SetMaterial(pMtl);
        }
    }

    void OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
    {
        /*
        if (event.event == ENTITY_EVENT_MATERIAL && pEntity && pEntity->GetId() == m_entityId)
        {
            _smart_ptr<IMaterial> pMat = (_smart_ptr<IMaterial>)event.nParam[0];
            if (pMat == 0 || (m_pMaterial != 0 && pMat != m_pMaterial))
                m_pMaterial = 0;
        }
        */
    }

    _smart_ptr<IMaterial> m_pMaterial;
    _smart_ptr<IMaterial> m_pEditorMaterial;
    //////////////////////////////////////////////////////////////////////////
};



//////////////////////////////////////////////////////////////////////////
// Same than CFlowNode_EntityMaterial, but it does serialize the material change
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityMaterialSerialize
    : public CFlowNode_EntityMaterial
{
public:
    CFlowNode_EntityMaterialSerialize(SActivationInfo* pActInfo)
        : CFlowNode_EntityMaterial(pActInfo)
    {
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { pActInfo->m_pUserData = (void*)(UINT_PTR)m_entityId; return new CFlowNode_EntityMaterialSerialize(pActInfo); };
    virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        if (ser.IsReading())
        {
            IEntity* pEntity = GetEntity();
            if (pEntity)
            {
                string matSavedName;
                ser.Value("material", matSavedName);

                if (matSavedName.empty())
                {
                    pEntity->SetMaterial(NULL);
                }
                else
                {
                    _smart_ptr<IMaterial> pMatCurr = pEntity->GetMaterial();
                    if (!pMatCurr || matSavedName != pMatCurr->GetName())
                    {
                        _smart_ptr<IMaterial> pMtl = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(matSavedName.c_str());
                        pEntity->SetMaterial(pMtl);
                    }
                }
            }
        }
        else
        {
            string matName;
            IEntity* pEntity = GetEntity();
            if (pEntity)
            {
                _smart_ptr<IMaterial> pMat = pEntity->GetMaterial();
                if (pMat)
                {
                    matName = pMat->GetName();
                }
            }
            ser.Value("material", matName);
        }
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Set", _HELP("Trigger to set the material")),
            InputPortConfig<string>("material", _HELP("Name of material to apply")),
            InputPortConfig_Void("Reset", _HELP("Trigger to reset the original material")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            {0}
        };
        config.sDescription = _HELP("Apply material to the entity. The change is serialized");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_OBSOLETE);
    }
};


//////////////////////////////////////////////////////////////////////////
class CFlowNode_BroadcastEntityEvent
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    enum EInputs
    {
        IN_SEND,
        IN_EVENT,
        IN_NAME,
    };
    CFlowNode_BroadcastEntityEvent(SActivationInfo* pActInfo) {}
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_AnyType("send", _HELP("When signal recieved on this input entity event is broadcasted")),
            InputPortConfig<string>("event", _HELP("Entity event to be sent")),
            InputPortConfig<string>("name", _HELP("Any part of the entity name to receive event")),
            {0}
        };
        config.sDescription = _HELP("This node broadcasts specified event to all entities which names contain sub string in parameter name");
        config.pInputPorts = in_config;
        config.pOutputPorts = 0;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event != eFE_Activate)
        {
            return;
        }

        if (IsPortActive(pActInfo, IN_SEND))
        {
            const string& sEvent = GetPortString(pActInfo, IN_EVENT);
            const string& sSubName = GetPortString(pActInfo, IN_NAME);
            IEntityItPtr pEntityIterator = gEnv->pEntitySystem->GetEntityIterator();
            pEntityIterator->MoveFirst();
            IEntity* pEntity = 0;
            while (pEntity = pEntityIterator->Next())
            {
                if (strstr(pEntity->GetName(), sSubName) != 0)
                {
                    IComponentScriptPtr scriptComponent = pEntity->GetComponent<IComponentScript>();
                    if (scriptComponent)
                    {
                        scriptComponent->CallEvent(sEvent);
                    }
                }
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
// Flow node for entity ID.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityId
    : public CFlowEntityNodeBase
{
    enum InputPorts
    {
        Activate = 0
    };

    enum OutputPorts
    {
        Id = 0
    };

public:
    CFlowNode_EntityId(SActivationInfo* pActInfo)
        : CFlowEntityNodeBase()
    {
        m_entityId = (EntityId)(UINT_PTR)pActInfo->m_pUserData;
    }
    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { pActInfo->m_pUserData = (void*)(UINT_PTR)m_entityId; return new CFlowNode_EntityId(pActInfo); };
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<FlowEntityId>("Id", _HELP("Entity ID")),
            {0}
        };
        config.sDescription = _HELP("Entity ID");
        config.nFlags |= EFLN_TARGET_ENTITY | EFLN_ACTIVATION_INPUT;
        config.pInputPorts = 0;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void OnEntityEvent(IEntity* pEntity, SEntityEvent& event) {}

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        CFlowEntityNodeBase::ProcessEvent(event, pActInfo);

        switch (event)
        {
        case IFlowNode::eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                FlowEntityId entityId = FlowEntityId(GetEntityId(pActInfo));
                if (entityId != FlowEntityId::s_invalidFlowEntityID)
                {
                    ActivateOutput(pActInfo, OutputPorts::Id, entityId);
                }
            }
            break;
        }
        case IFlowNode::eFE_SetEntityId:
        {
            FlowEntityId entityId = FlowEntityId(GetEntityId(pActInfo));
            if (entityId != FlowEntityId::s_invalidFlowEntityID)
            {
                ActivateOutput(pActInfo, OutputPorts::Id, entityId);
            }
            break;
        }
        default:
        {
            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    //////////////////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////
// Flow node for getting parent's entity ID.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_ParentId
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_ParentId(SActivationInfo* pActInfo) {}
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<FlowEntityId>("parentId", _HELP("Entity ID of the parent entity")),
            {0}
        };
        config.sDescription = _HELP("Parent entity ID");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = 0;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event != eFE_SetEntityId)
        {
            return;
        }
        IEntity* pEntity = pActInfo->pEntity;
        if (!pEntity)
        {
            return;
        }
        IEntity* pParentEntity = pEntity->GetParent();
        if (!pParentEntity)
        {
            return;
        }
        ActivateOutput(pActInfo, 0, pParentEntity->GetId());
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
// Flow node for setting an entity property
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntitySetProperty
    : public CFlowEntityNodeBase
{
public:
    enum EInputs
    {
        IN_ACTIVATE,
        IN_SMARTNAME,
        IN_VALUE,
        IN_PERARCHETYPE,
    };
    CFlowNode_EntitySetProperty(SActivationInfo* pActInfo)
        : CFlowEntityNodeBase()
    {
        m_entityId = (EntityId)(UINT_PTR)pActInfo->m_pUserData;
    }
    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { pActInfo->m_pUserData = (void*)(UINT_PTR)m_entityId; return new CFlowNode_EntitySetProperty(pActInfo); };
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<SFlowSystemVoid>("Set", _HELP("Trigger it to set the property.")),
            InputPortConfig<string>("entityProperties_Property", _HELP("select entity property"), 0, _UICONFIG("ref_entity=entityId")),
            InputPortConfig<string>("Value", _HELP("Property string Value")),
            InputPortConfig<bool>("perArchetype", true, _HELP("False: property is a per instance property True: property is a per archetype property.")),
            {0}
        };

        static const SOutputPortConfig out_config[] = {
            OutputPortConfig_AnyType("Error", _HELP("")),
            {0}
        };

        config.sDescription = _HELP("Change entity property value. WARNING!: THIS PROPERTY CHANGE WONT WORK WITH SAVELOAD");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    SmartScriptTable DoResolveScriptTable(IScriptTable* pTable, const string& key, string& outKey)
    {
        ScriptAnyValue value;

        string token;
        int pos = 0;
        token = key.Tokenize(".", pos);
        pTable->GetValueAny(token, value);

        token = key.Tokenize(".", pos);
        if (token.empty())
        {
            return 0;
        }

        string nextToken = key.Tokenize(".", pos);
        while (nextToken.empty() == false)
        {
            if (value.type != ANY_TTABLE)
            {
                return 0;
            }
            ScriptAnyValue temp;
            value.table->GetValueAny(token, temp);
            value = temp;
            token = nextToken;
            nextToken = token.Tokenize(".", pos);
        }
        outKey = token;
        return value.table;
    }

    SmartScriptTable ResolveScriptTable(IScriptTable* pTable, const char* sKey, bool bPerArchetype, string& outKey)
    {
        string key = bPerArchetype ? "Properties." : "PropertiesInstance.";
        key += sKey;
        return DoResolveScriptTable(pTable, key, outKey);
    }

    void OnEntityEvent(IEntity* pEntity, SEntityEvent& event) {}

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        CFlowEntityNodeBase::ProcessEvent(event, pActInfo);

        IEntity* pEntity = GetEntity();
        if (!pEntity)
        {
            return;
        }

        switch (event)
        {
        case eFE_Activate:
        {
            bool bValuePort = IsPortActive(pActInfo, IN_SMARTNAME);
            bool bTriggerPort = IsPortActive(pActInfo, IN_ACTIVATE);
            if (bValuePort || bTriggerPort)
            {
                bool bPerArchetype = GetPortBool(pActInfo, IN_PERARCHETYPE);

                IScriptTable* pTable = pEntity->GetScriptTable();
                if (!pTable)
                {
                    return;
                }

                string realKey;
                string inputValue        = GetPortString(pActInfo, IN_SMARTNAME);
                string selectedParameter = inputValue.substr(inputValue.find_first_of(":") + 1, (inputValue.length() - inputValue.find_first_of(":")));

                SmartScriptTable propTable;
                const char* sKey = (selectedParameter.replace(":", ".")).c_str();

                propTable = ResolveScriptTable(pTable, sKey, bPerArchetype, realKey);
                if (!propTable)
                {
                    GameWarning("[flow] CFlowNode_EntitySetProperty: Cannot resolve property '%s' in entity '%s'",
                        sKey, pEntity->GetName());
                    ActivateOutput(pActInfo, 0, 0);
                    return;
                }
                sKey = realKey.c_str();
                int scriptVarType = propTable->GetValueType(sKey);
                if (scriptVarType == svtNull)
                {
                    GameWarning("[flow] CFlowNode_EntitySetProperty: Cannot resolve property '%s' in entity '%s' -> Creating",
                        sKey, pEntity->GetName());
                    ActivateOutput(pActInfo, 0, 0);
                }

                const char* strValue = GetPortString(pActInfo, IN_VALUE);

                switch (scriptVarType)
                {
                case svtNumber:
                {
                    float fValue = (float)atof(strValue);
                    propTable->SetValue(sKey, fValue);
                    if (pTable->HaveValue("OnPropertyChange"))
                    {
                        Script::CallMethod(pTable, "OnPropertyChange");
                    }
                    break;
                }

                case svtBool:
                {
                    float fValue = (float)atof(strValue);
                    bool bVal = (bool)(fabs(fValue) > 0.001f);
                    propTable->SetValue(sKey, bVal);
                    if (pTable->HaveValue("OnPropertyChange"))
                    {
                        Script::CallMethod(pTable, "OnPropertyChange");
                    }
                    break;
                }

                case svtString:
                {
                    propTable->SetValue(sKey, strValue);
                    if (pTable->HaveValue("OnPropertyChange"))
                    {
                        Script::CallMethod(pTable, "OnPropertyChange");
                    }
                    break;
                }

                case svtObject:
                {
                    // property is a color type?
                    if (strcmp(selectedParameter.substr(selectedParameter.find_last_of(".") + 1, 3), "clr") == 0)
                    {
                        Vec3 clrValue(ZERO);
                        string stringToVec = strValue;

                        clrValue.x = (float)atof(stringToVec.substr(0, stringToVec.find_first_of(",")));
                        clrValue.y = (float)atof(stringToVec.substr(stringToVec.find_first_of(",") + 1, stringToVec.find_last_of(",")));
                        clrValue.z = (float)atof(stringToVec.substr(stringToVec.find_last_of(",") + 1, stringToVec.length()));

                        clrValue.x = fabs(min(clrValue.x, 255.f));
                        clrValue.y = fabs(min(clrValue.y, 255.f));
                        clrValue.z = fabs(min(clrValue.z, 255.f));

                        clrValue = clrValue / 255.f;
                        propTable->SetValue(sKey, clrValue);
                        if (pTable->HaveValue("OnPropertyChange"))
                        {
                            Script::CallMethod(pTable, "OnPropertyChange");
                        }
                    }
                    break;
                }
                }
            }
        }
        break;
        }
    }


    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
    //////////////////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////
// Flow node for getting an entity property value
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityGetProperty
    : public CFlowEntityNodeBase
{
public:
    enum EInputs
    {
        IN_ACTIVATE,
        IN_SMARTNAME,
        IN_PER_ARCHETYPE,
    };

    CFlowNode_EntityGetProperty(SActivationInfo* pActInfo)
        : CFlowEntityNodeBase()
    {
        m_entityId = (EntityId)(UINT_PTR)pActInfo->m_pUserData;
    }
    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { pActInfo->m_pUserData = (void*)(UINT_PTR)m_entityId; return new CFlowNode_EntityGetProperty(pActInfo); };
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<SFlowSystemVoid>("Get", _HELP("Trigger it to get the property!")),
            InputPortConfig<string>("entityProperties_Property", _HELP("select entity property"), 0, _UICONFIG("ref_entity=entityId")),
            InputPortConfig<bool>("perArchetype", true, _HELP("False: property is a per instance property True: property is a per archetype property.")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig_AnyType("Value", _HELP("Value of the property")),
            OutputPortConfig_AnyType("Error", _HELP("")),
            {0}
        };
        config.sDescription = _HELP("Retrieve entity property value");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void OnEntityEvent(IEntity* pEntity, SEntityEvent& event) {}

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        CFlowEntityNodeBase::ProcessEvent(event, pActInfo);

        IEntity* pEntity = GetEntity();
        if (!pEntity)
        {
            return;
        }

        if (event == eFE_Activate)
        {
            if (IsPortActive(pActInfo, IN_SMARTNAME) || IsPortActive(pActInfo, IN_ACTIVATE))
            {
                bool bPerArchetype = GetPortBool(pActInfo, IN_PER_ARCHETYPE);

                IScriptTable* pTable = pEntity->GetScriptTable();
                if (!pTable)
                {
                    return;
                }

                string inputValue = GetPortString(pActInfo, IN_SMARTNAME);
                string selectedParameter = inputValue.substr(inputValue.find_first_of(":") + 1, (inputValue.length() - inputValue.find_first_of(":")));

                const char* pKey = (selectedParameter.replace(":", ".")).c_str();
                const char* pPropertiesString = bPerArchetype ? "Properties" : "PropertiesInstance";

                // requested property is a vector?
                if (strcmp(selectedParameter.substr(selectedParameter.find_last_of(".") + 1, 3), "clr") == 0)
                {
                    Vec3 vecValue(ZERO);

                    for (int i = 0; i < 3; i++)
                    {
                        ScriptAnyValue value;
                        char newKey[32];

                        cry_strcpy(newKey, pKey);

                        if (i == 0)
                        {
                            cry_strcat(newKey, ".x");
                        }
                        if (i == 1)
                        {
                            cry_strcat(newKey, ".y");
                        }
                        if (i == 2)
                        {
                            cry_strcat(newKey, ".z");
                        }

                        if (ReadScriptValue(pTable, pPropertiesString, newKey, bPerArchetype, value))
                        {
                            float fVal;
                            value.CopyTo(fVal);

                            if (i == 0)
                            {
                                vecValue.x = fVal;
                            }
                            if (i == 1)
                            {
                                vecValue.y = fVal;
                            }
                            if (i == 2)
                            {
                                vecValue.z = fVal;
                            }
                        }
                        else
                        {
                            ActivateOutput(pActInfo, 1, 0);
                            return;
                        }
                    }
                    ActivateOutput(pActInfo, 0, vecValue * 255.f);
                    return;
                }

                ScriptAnyValue value;
                bool isValid = ReadScriptValue(pTable, pPropertiesString, pKey, bPerArchetype, value);

                if (!isValid)
                {
                    GameWarning("[flow] CFlowNode_EntityGetProperty: Could not find property '%s.%s' in entity '%s'", pPropertiesString, pKey, pEntity->GetName());
                    ActivateOutput(pActInfo, 1, isValid);
                    return;
                }

                switch (value.GetVarType())
                {
                case svtNumber:
                {
                    float fVal;
                    value.CopyTo(fVal);

                    // TODO: fix wrong number return type for booleans
                    if (strcmp(selectedParameter.substr(selectedParameter.find_last_of(".") + 1, 1), "b") == 0)
                    {
                        bool bVal = (bool)(fabs(fVal) > 0.001f);
                        ActivateOutput(pActInfo, 0, bVal);
                    }
                    else
                    {
                        ActivateOutput(pActInfo, 0, fVal);
                    }
                    break;
                }

                case svtBool:
                {
                    bool val;
                    value.CopyTo(val);
                    ActivateOutput(pActInfo, 0, val);
                    break;
                }

                case svtString:
                {
                    const char* pVal = NULL;
                    value.CopyTo(pVal);
                    ActivateOutput(pActInfo, 0, string(pVal));
                    break;
                }

                default:
                {
                    GameWarning("[flow] CFlowNode_EntityGetProperty: property '%s.%s' in entity '%s' is of unexpected type('%d')", pPropertiesString, pKey, pEntity->GetName(), value.GetVarType());
                    ActivateOutput(pActInfo, 1, 0);
                    return;
                }
                }
            }
        }
    }

    bool ReadScriptValue(IScriptTable* pTable, const char* pPropertiesString, const char* pKey, bool bPerArchetype, ScriptAnyValue& value)
    {
        pTable->GetValueAny(pPropertiesString, value);

        int pos = 0;
        string key = pKey;
        string nextToken = key.Tokenize(".", pos);
        while (!nextToken.empty() && value.type == ANY_TTABLE)
        {
            ScriptAnyValue temp;
            value.table->GetValueAny(nextToken, temp);
            value = temp;
            nextToken = key.Tokenize(".", pos);
        }

        return nextToken.empty() && (value.type == ANY_TNUMBER || value.type == ANY_TBOOLEAN || value.type == ANY_TSTRING);
    }


    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    //////////////////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////
// Flow node for Attaching child to parent.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityAttachChild
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_EntityAttachChild(SActivationInfo* pActInfo) {}

    enum InputPorts
    {
        Attach = 0,
        Child,
        KeepTransform,
        DisablePhysics
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Attach", _HELP("Trigger to attach entity")),
            InputPortConfig<FlowEntityId>("Child", _HELP("Child Entity to Attach")),
            InputPortConfig<bool>("KeepTransform", _HELP("Child entity will remain in the same transformation in world space")),
            InputPortConfig<bool>("DisablePhysics", false, _HELP("Force disable physics of child entity on attaching")),
            {0}
        };
        config.sDescription = _HELP("Attach Child Entity");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = 0;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event != eFE_Activate)
        {
            return;
        }

        if (IsPortActive(pActInfo, Attach))
        {
            FlowEntityType entityType = GetFlowEntityTypeFromFlowEntityId(pActInfo->entityId);

            if (entityType == FlowEntityType::Component)
            {
                AZ::EntityId childEntityId = AZ::EntityId(GetPortEntityId(pActInfo, Child).GetId());

                if (GetPortBool(pActInfo, DisablePhysics))
                {
                    EBUS_EVENT_ID(childEntityId, AzFramework::PhysicsComponentRequestBus, DisablePhysics);
                }

                if (GetPortBool(pActInfo, KeepTransform))
                {
                    EBUS_EVENT_ID(childEntityId, AZ::TransformBus, SetParent, pActInfo->entityId);
                }
                else
                {
                    EBUS_EVENT_ID(childEntityId, AZ::TransformBus, SetParentRelative, pActInfo->entityId);
                }
            }
            else if (entityType == FlowEntityType::Legacy)
            {
                if (!pActInfo->pEntity)
                {
                    return;
                }

                FlowEntityId nChildId = FlowEntityId(GetPortEntityId(pActInfo, Child));
                IEntity* pChild = gEnv->pEntitySystem->GetEntity(nChildId);
                if (pChild)
                {
                    if (GetPortBool(pActInfo, DisablePhysics))
                    {
                        IComponentPhysicsPtr pPhysicsComponent = pChild->GetComponent<IComponentPhysics>();
                        if (pPhysicsComponent)
                        {
                            pPhysicsComponent->EnablePhysics(false);
                        }
                    }

                    int nFlags = 0;
                    if (GetPortBool(pActInfo, KeepTransform))
                    {
                        nFlags |= IEntity::ATTACHMENT_KEEP_TRANSFORMATION;
                    }

                    pActInfo->pEntity->AttachChild(pChild, nFlags);
                }
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
// Flow node for detaching child from parent.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityDetachThis
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_EntityDetachThis(SActivationInfo* pActInfo) {}

    enum InputPorts
    {
        Detach = 0,
        KeepTransform,
        EnablePhysics
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Detach", _HELP("Trigger to detach entity from parent")),
            InputPortConfig<bool>("KeepTransform", _HELP("When attaching entity will stay in same transformation in world space")),
            InputPortConfig<bool>("EnablePhysics", false, _HELP("Force enable physics of entity after detaching")),
            {0}
        };
        config.sDescription = _HELP("Detach child from its parent");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = 0;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event != eFE_Activate)
        {
            return;
        }

        if (IsPortActive(pActInfo, Detach))
        {
            FlowEntityType entityType = GetFlowEntityTypeFromFlowEntityId(pActInfo->entityId);

            if (entityType == FlowEntityType::Component)
            {
                const AZ::EntityId invalidEntityId;

                if (GetPortBool(pActInfo, EnablePhysics))
                {
                    EBUS_EVENT_ID(pActInfo->entityId, AzFramework::PhysicsComponentRequestBus, EnablePhysics);
                }

                if (GetPortBool(pActInfo, KeepTransform))
                {
                    EBUS_EVENT_ID(pActInfo->entityId, AZ::TransformBus, SetParent, invalidEntityId);
                }
                else
                {
                    EBUS_EVENT_ID(pActInfo->entityId, AZ::TransformBus, SetParentRelative, invalidEntityId);
                }
            }
            else if (entityType == FlowEntityType::Legacy)
            {
                if (!pActInfo->pEntity)
                {
                    return;
                }

                int nFlags = 0;
                if (GetPortBool(pActInfo, KeepTransform))
                {
                    nFlags |= IEntity::ATTACHMENT_KEEP_TRANSFORMATION;
                }

                if (GetPortBool(pActInfo, EnablePhysics))
                {
                    IComponentPhysicsPtr pPhysicsComponent = pActInfo->pEntity->GetComponent<IComponentPhysics>();
                    if (pPhysicsComponent)
                    {
                        pPhysicsComponent->EnablePhysics(true);
                    }
                }
                pActInfo->pEntity->DetachThis(nFlags);
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
// Flow node for getting the player [deprecated].
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityGetPlayer
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_EntityGetPlayer(SActivationInfo* pActInfo) {}
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<FlowEntityId>("Player", _HELP("Outputs Id of the current player")),
            {0}
        };
        config.sDescription = _HELP("Retrieve current player entity - Deprecated Use Game:LocalPlayer!");
        config.pInputPorts = 0;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_OBSOLETE);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        IActor* pActor = CCryAction::GetCryAction()->GetClientActor();
        if (pActor)
        {
            IEntity* pLocalPlayer = pActor->GetEntity();
            if (pLocalPlayer)
            {
                ActivateOutput(pActInfo, 0, pLocalPlayer->GetId());
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
// Flow node for beaming an entity
//////////////////////////////////////////////////////////////////////////
class CFlowNode_BeamEntity
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_BeamEntity(SActivationInfo* pActInfo) {}

    enum EInputPorts
    {
        EIP_Beam = 0,
        EIP_Pos,
        EIP_Rot,
        EIP_UseZeroRot,
        EIP_Scale,
        EIP_Memo
    };

    enum EOutputPorts
    {
        EOP_Done = 0,
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Beam", _HELP("Trigger to beam the Entity")),
            InputPortConfig<Vec3>("Position", _HELP("Position in World Coords")),
            InputPortConfig<Vec3>("Rotation", _HELP("Rotation [Degrees] in World Coords. (0,0,0) leaves Rotation untouched, unless UseZeroRot is set to 1.")),
            InputPortConfig<bool>("UseZeroRot", false, _HELP("If true, rotation is applied even if is (0,0,0)")),
            InputPortConfig<Vec3>("Scale", _HELP("Scale. (0,0,0) leaves Scale untouched.")),
            InputPortConfig<string>("Memo", _HELP("Memo to log when position is zero")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig_Void ("Done", _HELP("Triggered when done")),
            {0}
        };

        config.sDescription = _HELP("Beam an Entity to a new Position");
        config.nFlags |= EFLN_TARGET_ENTITY | EFLN_AISEQUENCE_SUPPORTED;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event != eFE_Activate)
        {
            return;
        }
        if (!pActInfo->pEntity)
        {
            return;
        }
        if (IsPortActive(pActInfo, EIP_Beam))
        {
            const Vec3* vRot = pActInfo->pInputPorts[EIP_Rot].GetPtr<Vec3>();
            const Vec3 vPos = GetPortVec3(pActInfo, EIP_Pos);

            bool bUseZeroRot = GetPortBool(pActInfo, EIP_UseZeroRot);

            const char* memo = GetPortString(pActInfo, EIP_Memo).c_str();
            const char* entityName = pActInfo->pEntity->GetName();

            bool isPlayer = GetISystem()->GetIGame()->GetIGameFramework()->GetClientActorId() == pActInfo->pEntity->GetId();

            if ((memo == NULL || memo[0] == 0) && isPlayer == 0)
            {
                memo = "<no memo>";
            }

            if (vPos.IsZero())
            {
                CryWarning(VALIDATOR_MODULE_FLOWGRAPH, VALIDATOR_WARNING, "BeamEntity Teleported %s to vPos zero. %s", entityName, (memo != NULL && memo[0] != 0) ? memo : "<no memo>");
            }
            else if (memo != NULL && memo[0] != 0)
            {
                CryWarning(VALIDATOR_MODULE_FLOWGRAPH, VALIDATOR_COMMENT, "BeamEntity Teleported %s: %s", entityName, memo);
            }

            Matrix34 tm;
            bool applyRot = vRot != NULL && (!vRot->IsZero() || bUseZeroRot);
            if (!applyRot)
            {
                tm = pActInfo->pEntity->GetWorldTM();
            }
            else
            {
                tm = Matrix33(Quat::CreateRotationXYZ(Ang3(DEG2RAD(*vRot))));
            }

            tm.SetTranslation(vPos);
            pActInfo->pEntity->SetWorldTM(tm, ENTITY_XFORM_TRACKVIEW);

            const Vec3* vScale = pActInfo->pInputPorts[EIP_Scale].GetPtr<Vec3>();
            if (vScale && vScale->IsZero() == false)
            {
                pActInfo->pEntity->SetScale(*vScale, ENTITY_XFORM_TRACKVIEW);
            }

            // TODO: Maybe add some tweaks/checks wrt. physics/collisions
            ActivateOutput(pActInfo, EOP_Done, true);
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
// Flow node for getting entity info.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityGetInfo
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_EntityGetInfo(SActivationInfo* pActInfo) {}
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Get", _HELP("Trigger to get info")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<FlowEntityId> ("Id", _HELP("Entity Id")),
            OutputPortConfig<string>   ("Name", _HELP("Entity Name")),
            OutputPortConfig<string>   ("Class", _HELP("Entity Class")),
            OutputPortConfig<string>   ("Archetype", _HELP("Entity Archetype")),
            {0}
        };
        config.sDescription = _HELP("Get Entity Information");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event != eFE_Activate)
        {
            return;
        }
        IEntity* pEntity = pActInfo->pEntity;
        if (pEntity == 0)
        {
            return;
        }
        if (IsPortActive(pActInfo, 0))
        {
            string empty;
            ActivateOutput(pActInfo, 0, FlowEntityId(pEntity->GetId()));
            string temp (pEntity->GetName());
            ActivateOutput(pActInfo, 1, temp);
            temp = pEntity->GetClass()->GetName();
            ActivateOutput(pActInfo, 2, temp);
            IEntityArchetype* pArchetype = pEntity->GetArchetype();
            if (pArchetype)
            {
                temp = pArchetype->GetName();
            }
            ActivateOutput(pActInfo, 3, pArchetype ? temp : empty);
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
// Flow node for checking if an entity exists
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityGetExistence
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum InputPort
    {
        InputPort_Get,
        InputPort_Entity
    };

    enum OutputPort
    {
        OutputPort_Exists,
        OutputPort_True,
        OutputPort_False
    };

public:
    CFlowNode_EntityGetExistence(SActivationInfo* /*pActInfo*/)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_EntityGetExistence(pActInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void("Get", _HELP("Trigger to get info")),
            InputPortConfig<FlowEntityId>("EntityId", _HELP("Entity to check for")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<bool>("Exists", _HELP("Indicates true/false if this FlowEntityId exists")),
            OutputPortConfig_Void("True", _HELP("Output if this FlowEntityId exists")),
            OutputPortConfig_Void("False", _HELP("Output if this FlowEntityId doesn't exist")),
            { 0 }
        };
        config.sDescription = _HELP("Get Entity's Existence");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        if (event != eFE_Activate)
        {
            return;
        }

        if (IsPortActive(pActInfo, InputPort_Get))
        {
            FlowEntityId id = GetPortFlowEntityId(pActInfo, InputPort_Entity);
            if (gEnv->pEntitySystem->GetEntity(id))
            {
                ActivateOutput(pActInfo, OutputPort_Exists, true);
                ActivateOutput(pActInfo, OutputPort_True, 1);
            }
            else
            {
                ActivateOutput(pActInfo, OutputPort_Exists, false);
                ActivateOutput(pActInfo, OutputPort_False, 1);
            }
        }
    }

    void GetMemoryUsage(ICrySizer*) const override
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////////
// Flow node for setting entity render parameters (opacity, glow, motionblur, etc)
////////////////////////////////////////////////////////////////////////////////////////
class CFlowNodeRenderParams
    : public CFlowEntityNodeBase
{
public:
    CFlowNodeRenderParams(SActivationInfo* pActInfo)
        : CFlowEntityNodeBase()
    {
        m_entityId = (EntityId)(UINT_PTR)pActInfo->m_pUserData;
    }

    enum EInputPorts
    {
        EIP_Opacity = 0,
        EIP_GlowAmount,
    };

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { pActInfo->m_pUserData = (void*)(UINT_PTR)m_entityId; return new CFlowNodeRenderParams(pActInfo); };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<float>("Opacity", 1.0f, "Set opacity amount"),
            //InputPortConfig<float>( "GlowAmount", 1.0f, "Set glow amount" ),
            {0}
        };
        config.sDescription = _HELP("Set render specific parameters");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = 0;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        IEntity* pEntity = GetEntity();
        if (!pEntity)
        {
            return;
        }

        IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
        if (!pRenderComponent)
        {
            return;
        }

        if (ser.IsWriting())
        {
            float opacity = pRenderComponent->GetOpacity();
            ser.Value("opacity", opacity);
        }
        else
        {
            float opacity;
            ser.Value("opacity", opacity);
            pRenderComponent->SetOpacity(opacity);
        }
    }


    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        CFlowEntityNodeBase::ProcessEvent(event, pActInfo);

        if (event != eFE_Activate)
        {
            return;
        }

        IEntity* pEntity = GetEntity();

        if (!pEntity)
        {
            return;
        }

        IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
        if (!pRenderComponent)
        {
            return;
        }

        if (IsPortActive(pActInfo, EIP_Opacity))
        {
            pRenderComponent->SetOpacity(GetPortFloat(pActInfo, EIP_Opacity));
        }
    }

    void OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
    {
    }


    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

#if 0 // replace by CFlowNodeMaterialShaderParam
////////////////////////////////////////////////////////////////////////////////////////
// Flow node for setting entity render parameters (opacity, glow, motionblur, etc)
////////////////////////////////////////////////////////////////////////////////////////
class CFlowNodeMaterialParam
    : public CFlowEntityNodeBase
{
public:
    CFlowNodeMaterialParam(SActivationInfo* pActInfo)
        : CFlowEntityNodeBase()
    {
        m_entityId = (FlowEntityId)(UINT_PTR)pActInfo->m_pUserData;
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        pActInfo->m_pUserData = (void*)(UINT_PTR)m_entityId;
        return new CFlowNodeMaterialParam(pActInfo);
    };

    enum EInputPorts
    {
        EIP_Get = 0,
        EIP_Slot,
        EIP_SubMtlId,
        EIP_ParamFloat,
        EIP_Float,
        EIP_ParamColor,
        EIP_Color,
    };

    enum EOutputPorts
    {
        EOP_Float = 0,
        EOP_Color,
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void ("Get", _HELP("Trigger to get current value")),
            InputPortConfig<int> ("Slot", 0, _HELP("Material Slot")),
            InputPortConfig<int> ("SubMtlId", 0, _HELP("Sub Material Id")),
            InputPortConfig<string> ("ParamFloat", _HELP("Float Parameter to be set/get"), 0, _UICONFIG("enum_string:Glow=glow,Shininess=shininess,Alpha=alpha,Opacity=opacity")),
            InputPortConfig<float>("ValueFloat", 0.0f, _HELP("Trigger to set Float value")),
            InputPortConfig<string> ("ParamColor", _HELP("Color Parameter to be set/get"), 0, _UICONFIG("enum_string:Diffuse=diffuse,Specular=specular,Emissive=emissive")),
            InputPortConfig<Vec3>("color_ValueColor", _HELP("Trigger to set Color value")),
            {0}
        };

        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<float>("ValueFloat", _HELP("Current Float Value")),
            OutputPortConfig<Vec3> ("ValueColor",  _HELP("Current Color Value")),
            {0}
        };

        config.sDescription = _HELP("Set/Get Material Parameters");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        CFlowEntityNodeBase::ProcessEvent(event, pActInfo);

        if (event != eFE_Activate)
        {
            return;
        }

        const bool bGet = IsPortActive(pActInfo, EIP_Get);
        const bool bSetFloat = IsPortActive(pActInfo, EIP_Float);
        const bool bSetColor = IsPortActive(pActInfo, EIP_Color);

        if (!bGet && !bSetFloat && !bSetColor)
        {
            return;
        }

        IEntity* pEntity = GetEntity();
        if (pEntity == 0)
        {
            return;
        }

        IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent == 0)
        {
            return;
        }

        const int& slot = GetPortInt(pActInfo, EIP_Slot);
        _smart_ptr<IMaterial> pMtl = pRenderComponent->GetRenderMaterial(slot);
        if (pMtl == 0)
        {
            GameWarning("[flow] CFlowNodeSetMaterialParam: Entity '%s' [%d] has no material at slot %d", pEntity->GetName(), pEntity->GetId(), slot);
            return;
        }

        const int& subMtlId = GetPortInt(pActInfo, EIP_SubMtlId);
        pMtl = pMtl->GetSafeSubMtl(subMtlId);
        if (pMtl == 0)
        {
            GameWarning("[flow] CFlowNodeSetMaterialParam: Entity '%s' [%d] has no sub-material %d at slot %d", pEntity->GetName(), pEntity->GetId(), subMtlId, slot);
            return;
        }
        const string& paramNameFloat = GetPortString(pActInfo, EIP_ParamFloat);
        const string& paramNameVec3 = GetPortString(pActInfo, EIP_ParamColor);

        float floatValue = 0.0f;
        Vec3 vec3Value = Vec3(ZERO);
        if (bSetFloat)
        {
            floatValue = GetPortFloat(pActInfo, EIP_Float);
            pMtl->SetGetMaterialParamFloat(paramNameFloat, floatValue, false);
        }
        if (bSetColor)
        {
            vec3Value = GetPortVec3(pActInfo, EIP_Color);
            pMtl->SetGetMaterialParamVec3(paramNameVec3, vec3Value, false);
        }
        if (bGet)
        {
            if (pMtl->SetGetMaterialParamFloat(paramNameFloat, floatValue, true))
            {
                ActivateOutput(pActInfo, EOP_Float, floatValue);
            }
            if (pMtl->SetGetMaterialParamVec3(paramNameVec3, vec3Value, true))
            {
                ActivateOutput(pActInfo, EOP_Color, vec3Value);
            }
        }
    }

    void OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
    {
    }


    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Flow node for setting entity render parameters (opacity, glow, motionblur, etc)
////////////////////////////////////////////////////////////////////////////////////////

class CFlowNodeEntityMaterialShaderParam
    : public CFlowEntityNodeBase
{
public:
    CFlowNodeEntityMaterialShaderParam(SActivationInfo* pActInfo)
        : CFlowEntityNodeBase()
    {
        m_entityId = (EntityId)(UINT_PTR)pActInfo->m_pUserData;
        m_anyChangeDone = false;
        m_pMaterial = NULL;
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        pActInfo->m_pUserData = (void*)(UINT_PTR)m_entityId;
        return new CFlowNodeEntityMaterialShaderParam(pActInfo);
    };

    enum EInputPorts
    {
        EIP_Get = 0,
        EIP_Slot,
        EIP_SubMtlId,
        EIP_ParamFloat,
        EIP_Float,
        EIP_ParamColor,
        EIP_Color,
    };

    enum EOutputPorts
    {
        EOP_Float = 0,
        EOP_Color,
    };

    //////////////////////////////////////////////////////////////////////////
    /// this serialize is *NOT* perfect. If any of the following inputs: "slot, submtlid, paramFloat, paramColor" do change and the node is triggered more than once  with different values for them, the serializing wont be 100% correct.
    /// this does not happens in crysis2, so is good enough for now.
    /// to make it perfect, either the materials should be serialized at engine level, or either we would need to create an intermediate module to handle changes on materials from game side,
    /// make the flownodes to use that module instead of the materials directly, and serialize the changes there instead of in the flownodes.

    void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        // we do assume that those inputs never change, which is ok for crysis2, but of course is not totally right
        const int slot = GetPortInt(pActInfo, EIP_Slot);
        const int subMtlId = GetPortInt(pActInfo, EIP_SubMtlId);
        const string& paramNameFloat = GetPortString(pActInfo, EIP_ParamFloat);
        const string& paramNameColor = GetPortString(pActInfo, EIP_ParamColor);

        ser.Value("m_anyChangeDone", m_anyChangeDone);

        //---------------------
        if (ser.IsWriting())
        {
            _smart_ptr<IMaterial> pSubMtl;
            IRenderShaderResources* pRendShaderRes;
            if (!ObtainMaterialPtrs(slot, subMtlId, false, pSubMtl, pRendShaderRes))
            {
                return;
            }
            DynArrayRef<SShaderParam>& shaderParams = pRendShaderRes->GetParameters();

            if (!paramNameFloat.empty())
            {
                float floatValue = 0;
                ReadFloatValueFromMat(pSubMtl, pRendShaderRes, paramNameFloat, floatValue);
                ser.Value("floatValue", floatValue);
            }

            if (!paramNameColor.empty())
            {
                Vec3 colorValue(ZERO);
                ReadColorValueFromMat(pSubMtl, pRendShaderRes, paramNameColor, colorValue);
                ser.Value("colorValue", colorValue);
            }
        }

        //----------------
        if (ser.IsReading())
        {
            // if there was no change done, we still need to set values to the material, in case it was changed after the savegame.
            // But in that case, we dont need to create a clone, we just use the current material
            bool cloneMaterial = m_anyChangeDone;
            _smart_ptr<IMaterial> pSubMtl;
            IRenderShaderResources* pRendShaderRes;
            if (!ObtainMaterialPtrs(slot, subMtlId, cloneMaterial, pSubMtl, pRendShaderRes))
            {
                return;
            }
            DynArrayRef<SShaderParam>& shaderParams = pRendShaderRes->GetParameters();

            // if we *potentially* can change one of them, we have to restore them, even if we in theory didnt change any. that is why this checks the name of the parameters, instead of actualy changedone
            {
                bool bUpdateShaderConstants = false;
                if (!paramNameFloat.empty())
                {
                    float floatValue;
                    ser.Value("floatValue", floatValue);
                    if (pSubMtl->SetGetMaterialParamFloat(paramNameFloat, floatValue, false) == false)
                    {
                        UParamVal val;
                        val.m_Float = floatValue;
                        SShaderParam::SetParam(paramNameFloat, &shaderParams, val);
                        bUpdateShaderConstants = true;
                    }
                }
                if (!paramNameColor.empty())
                {
                    Vec3 colorValue;
                    ser.Value("colorValue", colorValue);
                    if (pSubMtl->SetGetMaterialParamVec3(paramNameColor, colorValue, false) == false)
                    {
                        UParamVal val;
                        val.m_Color[0] = colorValue[0];
                        val.m_Color[1] = colorValue[1];
                        val.m_Color[2] = colorValue[2];
                        val.m_Color[3] = 1.0f;
                        SShaderParam::SetParam(paramNameColor, &shaderParams, val);
                        bUpdateShaderConstants = true;
                    }
                }

                if (bUpdateShaderConstants)
                {
                    const SShaderItem& shaderItem = pSubMtl->GetShaderItem();
                    shaderItem.m_pShaderResources->UpdateConstants(shaderItem.m_pShader);
                }
            }
        }
    }


    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void ("Get", _HELP("Trigger to get current value")),
            InputPortConfig<int> ("Slot", 0, _HELP("Material Slot")),
            InputPortConfig<int> ("SubMtlId", 0, _HELP("Sub Material Id")),
            InputPortConfig<string> ("ParamFloat", _HELP("Float Parameter to be set/get"), 0, _UICONFIG("dt=matparamslot,slot_ref=Slot,sub_ref=SubMtlId,param=float")),
            InputPortConfig<float>("ValueFloat", 0.0f, _HELP("Trigger to set Float value")),
            InputPortConfig<string> ("ParamColor", _HELP("Color Parameter to be set/get"), 0, _UICONFIG("dt=matparamslot,slot_ref=Slot,sub_ref=SubMtlId,param=vec")),
            InputPortConfig<Vec3>("color_ValueColor", _HELP("Trigger to set Color value")),
            {0}
        };

        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<float>("ValueFloat", _HELP("Current Float Value")),
            OutputPortConfig<Vec3> ("ValueColor",  _HELP("Current Color Value")),
            {0}
        };

        config.sDescription = _HELP("Set/Get Entity's Material Shader Parameters");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_OBSOLETE);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        CFlowEntityNodeBase::ProcessEvent(event, pActInfo);

        if (event != eFE_Activate)
        {
            return;
        }

        const bool bGet = IsPortActive(pActInfo, EIP_Get);
        const bool bSetFloat = IsPortActive(pActInfo, EIP_Float);
        const bool bSetColor = IsPortActive(pActInfo, EIP_Color);
        const int slot = GetPortInt(pActInfo, EIP_Slot);
        const int subMtlId = GetPortInt(pActInfo, EIP_SubMtlId);
        const string& paramNameFloat = GetPortString(pActInfo, EIP_ParamFloat);
        const string& paramNameColor = GetPortString(pActInfo, EIP_ParamColor);

        if (!bGet && !bSetFloat && !bSetColor)
        {
            return;
        }

        bool cloneMaterial = bSetFloat || bSetColor;
        _smart_ptr<IMaterial> pSubMtl;
        IRenderShaderResources* pRendShaderRes;

        if (!ObtainMaterialPtrs(slot, subMtlId, cloneMaterial, pSubMtl, pRendShaderRes))
        {
            return;
        }

        DynArrayRef<SShaderParam>& shaderParams = pRendShaderRes->GetParameters();


        //..........sets.......
        {
            bool bUpdateShaderConstants = false;
            if (bSetFloat)
            {
                m_anyChangeDone = true;
                float floatValue = GetPortFloat(pActInfo, EIP_Float);
                if (pSubMtl->SetGetMaterialParamFloat(paramNameFloat, floatValue, false) == false)
                {
                    UParamVal val;
                    val.m_Float = floatValue;
                    SShaderParam::SetParam(paramNameFloat, &shaderParams, val);
                    bUpdateShaderConstants = true;
                }
            }
            if (bSetColor)
            {
                m_anyChangeDone = true;
                Vec3 colorValue = GetPortVec3(pActInfo, EIP_Color);
                if (pSubMtl->SetGetMaterialParamVec3(paramNameColor, colorValue, false) == false)
                {
                    UParamVal val;
                    val.m_Color[0] = colorValue[0];
                    val.m_Color[1] = colorValue[1];
                    val.m_Color[2] = colorValue[2];
                    val.m_Color[3] = 1.0f;
                    SShaderParam::SetParam(paramNameColor, &shaderParams, val);
                    bUpdateShaderConstants = true;
                }
            }

            if (bUpdateShaderConstants)
            {
                const SShaderItem& shaderItem = pSubMtl->GetShaderItem();
                shaderItem.m_pShaderResources->UpdateConstants(shaderItem.m_pShader);
            }
        }

        //........get
        if (bGet)
        {
            float floatValue;
            if (ReadFloatValueFromMat(pSubMtl, pRendShaderRes, paramNameFloat, floatValue))
            {
                ActivateOutput(pActInfo, EOP_Float, floatValue);
            }

            Vec3 colorValue;
            if (ReadColorValueFromMat(pSubMtl, pRendShaderRes, paramNameColor, colorValue))
            {
                ActivateOutput(pActInfo, EOP_Color, colorValue);
            }
        }
    }


    bool ReadFloatValueFromMat(_smart_ptr<IMaterial> pSubMtl, IRenderShaderResources* pRendShaderRes, const char* paramName, float& floatValue)
    {
        if (pSubMtl->SetGetMaterialParamFloat(paramName, floatValue, true))
        {
            return true;
        }

        DynArrayRef<SShaderParam>& shaderParams = pRendShaderRes->GetParameters();

        for (int i = 0; i < shaderParams.size(); ++i)
        {
            SShaderParam& param = shaderParams[i];
            if (azstricmp(param.m_Name.c_str(), paramName) == 0)
            {
                floatValue = 0.0f;
                switch (param.m_Type)
                {
                case eType_BOOL:
                    floatValue = param.m_Value.m_Bool;
                    break;
                case eType_SHORT:
                    floatValue = param.m_Value.m_Short;
                    break;
                case eType_INT:
                    floatValue = (float)param.m_Value.m_Int;
                    break;
                case eType_FLOAT:
                    floatValue = param.m_Value.m_Float;
                    break;
                }
                return true;
            }
        }
        return false;
    }


    bool ReadColorValueFromMat(_smart_ptr<IMaterial> pSubMtl, IRenderShaderResources* pRendShaderRes, const char* paramName, Vec3& colorValue)
    {
        if (pSubMtl->SetGetMaterialParamVec3(paramName, colorValue, true))
        {
            return true;
        }

        DynArrayRef<SShaderParam>& shaderParams = pRendShaderRes->GetParameters();

        for (int i = 0; i < shaderParams.size(); ++i)
        {
            SShaderParam& param = shaderParams[i];
            if (azstricmp(param.m_Name.c_str(), paramName) == 0)
            {
                colorValue.Set(0, 0, 0);
                if (param.m_Type == eType_VECTOR)
                {
                    colorValue.Set(param.m_Value.m_Vector[0], param.m_Value.m_Vector[1], param.m_Value.m_Vector[2]);
                }
                if (param.m_Type == eType_FCOLOR)
                {
                    colorValue.Set(param.m_Value.m_Color[0], param.m_Value.m_Color[1], param.m_Value.m_Color[2]);
                }
                return true;
            }
        }

        return false;
    }


    void OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
    {
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    //////////////////////////////////////////////////////////////////////////
    bool ObtainMaterialPtrs(int slot, int subMtlId, bool cloneMaterial, _smart_ptr<IMaterial>& pSubMtl, IRenderShaderResources*& pRenderShaderRes)
    {
        const bool RETFAIL = false;
        const bool RETOK = true;

        IEntity* pEntity = GetEntity();
        if (pEntity == 0)
        {
            return RETFAIL;
        }

        IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent == 0)
        {
            return RETFAIL;
        }

        _smart_ptr<IMaterial> pMaterial = pRenderComponent->GetRenderMaterial(slot);
        if (pMaterial == 0)
        {
            GameWarning("[flow] CFlowNodeEntityMaterialShaderParam: Entity '%s' [%d] has no material at slot %d", pEntity->GetName(), pEntity->GetId(), slot);
            return RETFAIL;
        }

        if (cloneMaterial && m_pMaterial != pMaterial)
        {
            pMaterial = gEnv->p3DEngine->GetMaterialManager()->CloneMultiMaterial(pMaterial);
            if (!pMaterial)
            {
                GameWarning("[flow] CFlowNodeEntityMaterialShaderParam: Entity '%s' [%d] Can't clone material at slot %d", pEntity->GetName(), pEntity->GetId(), slot);
                return RETFAIL;
            }
            pRenderComponent->SetSlotMaterial(slot, pMaterial);
            m_pMaterial = pMaterial;
        }


        pSubMtl = pMaterial->GetSafeSubMtl(subMtlId);
        if (pSubMtl == 0)
        {
            GameWarning("[flow] CFlowNodeEntityMaterialShaderParam: Entity '%s' [%d] has no sub-material %d at slot %d", pEntity->GetName(), pEntity->GetId(), subMtlId, slot);
            return RETFAIL;
        }

        const SShaderItem& shaderItem = pSubMtl->GetShaderItem();
        pRenderShaderRes = shaderItem.m_pShaderResources;
        if (!pRenderShaderRes)
        {
            GameWarning("[flow] CFlowNodeEntityMaterialShaderParam: Entity '%s' [%d] Material's '%s' sub-material [%d] '%s' has not shader-resources", GetEntity()->GetName(), GetEntity()->GetId(), pMaterial->GetName(), subMtlId, pSubMtl->GetName());
            return RETFAIL;
        }

        return RETOK;
    }


protected:
    bool m_anyChangeDone;
    _smart_ptr<IMaterial> m_pMaterial;// this is only used to know if we need to clone or not. And is in purpose not reseted on flow initialize, and also not serialized.
    // the situation is:
    // if we already cloned the material, it will stay cloned even if we load a checkpoint previous to the point where the change happened, so we dont need to clone it again.
    // On the other side, if we are doing a 'resumegame", we need to clone the material even if we already cloned it when the checkpoint happened.
    // By not serializing or reseting this member, we cover those cases correctly
};


//////////////////////////////////////////////////////////////////////////
// Flow node for entity pool usage
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityPool
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_EntityPool(SActivationInfo* pActInfo)
    {
    }

    enum EInputs
    {
        IN_PREPARE,
        IN_FREE,
    };

    enum EOutputs
    {
        OUT_READY,
        OUT_FREED,
        OUT_ERROR,
    };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void("Prepare", _HELP("Prepare the entity by bringing it into existence from the pool")),
            InputPortConfig_Void("Free", _HELP("Free the entity back to the pool to free resources")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<FlowEntityId>("Ready", _HELP("Outputs when the entity is fully prepared and ready to be used. FlowEntityId is guaranteed to be the same.")),
            OutputPortConfig_Void("Freed", _HELP("Outputs when the entity is freed up and returned back to the pool")),
            OutputPortConfig_Void("Error", _HELP("Called if an error occurred, such as the entity isn't configured for pool usage")),
            {0}
        };
        config.sDescription = _HELP("Used to prepare an entity from the pool, or free it back to the pool");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        FlowEntityId entityId = pActInfo->pGraph->GetEntityId(pActInfo->myID);
        if (event == eFE_Activate && entityId != 0)
        {
            if (IsPortActive(pActInfo, IN_PREPARE))
            {
                const bool bReady = gEnv->pEntitySystem->GetIEntityPoolManager()->PrepareFromPool(entityId);
                ActivateOutput(pActInfo, bReady ? OUT_READY : OUT_ERROR, true);
            }
            else if (IsPortActive(pActInfo, IN_FREE))
            {
                const bool bFreed = gEnv->pEntitySystem->GetIEntityPoolManager()->ReturnToPool(entityId);
                ActivateOutput(pActInfo, bFreed ? OUT_FREED : OUT_ERROR, true);
            }
        }
    }
};

class CFlowNodeCharAttachmentMaterialShaderParam
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    CFlowNodeCharAttachmentMaterialShaderParam(SActivationInfo* pActInfo)
        : m_pMaterial(0)
        , m_bFailedCloningMaterial(false)
    {
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNodeCharAttachmentMaterialShaderParam(pActInfo);
    };

    void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        if (ser.IsReading())
        {
            // new chance on load
            m_bFailedCloningMaterial = false;
        }
    }

    enum EInputPorts
    {
        EIP_CharSlot = 0,
        EIP_Attachment,
        EIP_SetMaterial,
        EIP_ForcedMaterial,
        EIP_SubMtlId,
        EIP_Get,
        EIP_ParamFloat,
        EIP_Float,
        EIP_ParamColor,
        EIP_Color,
    };

    enum EOutputPorts
    {
        EOP_Float = 0,
        EOP_Color,
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<int> ("CharSlot", 0, _HELP("Character Slot within Entity")),
            InputPortConfig<string>("Attachment", _HELP("Attachment"), 0, _UICONFIG("dt=attachment,ref_entity=entityId")),
            InputPortConfig_Void ("SetMaterial", _HELP("Trigger to force setting a material [ForcedMaterial]")),
            InputPortConfig<string>("ForcedMaterial", _HELP("Material"), 0, _UICONFIG("dt=mat")),
            InputPortConfig<int> ("SubMtlId", 0, _HELP("Sub Material Id")),
            InputPortConfig_Void ("Get", _HELP("Trigger to get current value")),
            InputPortConfig<string> ("ParamFloat", _HELP("Float Parameter to be set/get"), 0, _UICONFIG("dt=matparamcharatt,charslot_ref=CharSlot,attachment_ref=Attachment,sub_ref=SubMtlId,param=float")),
            InputPortConfig<float>("ValueFloat", 0.0f, _HELP("Trigger to set Float value")),
            InputPortConfig<string> ("ParamColor", _HELP("Color Parameter to be set/get"), 0, _UICONFIG("dt=matparamcharatt,charslot_ref=CharSlot,attachment_ref=Attachment,sub_ref=SubMtlId,param=vec")),
            InputPortConfig<Vec3>("color_ValueColor", _HELP("Trigger to set Color value")),
            {0}
        };

        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<float>("ValueFloat", _HELP("Current Float Value")),
            OutputPortConfig<Vec3> ("ValueColor",  _HELP("Current Color Value")),
            {0}
        };

        config.sDescription = _HELP("[CUTSCENE ONLY] Set ShaderParams of Character Attachments.");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event != eFE_Activate)
        {
            return;
        }

        const bool bGet = IsPortActive(pActInfo, EIP_Get);
        const bool bSetFloat = IsPortActive(pActInfo, EIP_Float);
        const bool bSetColor = IsPortActive(pActInfo, EIP_Color);
        const bool bSetMaterial = IsPortActive(pActInfo, EIP_SetMaterial);

        if (!bGet && !bSetFloat && !bSetColor && !bSetMaterial)
        {
            return;
        }

        IEntity* pEntity = pActInfo->pEntity;
        if (pEntity == 0)
        {
            return;
        }

        const int charSlot = GetPortInt(pActInfo, EIP_CharSlot);
        ICharacterInstance* pChar = pActInfo->pEntity->GetCharacter(charSlot);
        if (pChar == 0)
        {
            return;
        }

        IAttachmentManager* pAttMgr = pChar->GetIAttachmentManager();
        if (pAttMgr == 0)
        {
            return;
        }

        const string& attachment = GetPortString(pActInfo, EIP_Attachment);
        IAttachment* pAttachment = pAttMgr->GetInterfaceByName(attachment.c_str());
        if (pAttachment == 0)
        {
            GameWarning("[flow] CFlowNodeCharAttachmentMaterialShaderParam: Entity '%s' [%d] Cannot access Attachment '%s' [CharSlot=%d]", pEntity->GetName(), pEntity->GetId(), attachment.c_str(), charSlot);
            return;
        }

        IAttachmentObject* pAttachObj = pAttachment->GetIAttachmentObject();
        if (pAttachObj == 0)
        {
            GameWarning("[flow] CFlowNodeCharAttachmentMaterialShaderParam: Entity '%s' [%d] Cannot access AttachmentObject at Attachment '%s' [CharSlot=%d]", pEntity->GetName(), pEntity->GetId(), attachment.c_str(), charSlot);
            return;
        }

        IMaterialManager* pMatMgr = gEnv->p3DEngine->GetMaterialManager();
        if (bSetMaterial)
        {
            const string& matName = GetPortString(pActInfo, EIP_ForcedMaterial);
            if (true /* m_pMaterial == 0 || matName != m_pMaterial->GetName() */) // always reload the mat atm
            {
                m_pMaterial = pMatMgr->LoadMaterial(matName.c_str());
                if (m_pMaterial)
                {
                    m_pMaterial = pMatMgr->CloneMultiMaterial(m_pMaterial);
                    m_bFailedCloningMaterial = false;
                }
                else
                {
                    m_bFailedCloningMaterial = true;
                }
            }
        }

        if (m_pMaterial == 0 && m_bFailedCloningMaterial == false)
        {
            _smart_ptr<IMaterial> pOldMtl = (_smart_ptr<IMaterial>)pAttachObj->GetBaseMaterial();
            if (pOldMtl)
            {
                m_pMaterial = pMatMgr->CloneMultiMaterial(pOldMtl);
                m_bFailedCloningMaterial = false;
            }
            else
            {
                m_bFailedCloningMaterial = true;
            }
        }

        _smart_ptr<IMaterial> pMtl = m_pMaterial;
        if (pMtl == 0)
        {
            GameWarning("[flow] CFlowNodeCharAttachmentMaterialShaderParam: Entity '%s' [%d] Cannot access material at Attachment '%s' [CharSlot=%d]", pEntity->GetName(), pEntity->GetId(), attachment.c_str(), charSlot);
            return;
        }

        const int& subMtlId = GetPortInt(pActInfo, EIP_SubMtlId);
        pMtl = pMtl->GetSafeSubMtl(subMtlId);
        if (pMtl == 0)
        {
            GameWarning("[flow] CFlowNodeCharAttachmentMaterialShaderParam: Entity '%s' [%d] Material '%s' has no sub-material %d", pEntity->GetName(), pEntity->GetId(), m_pMaterial->GetName(), subMtlId);
            return;
        }

        // set our material
        pAttachObj->SetReplacementMaterial(pMtl);

        const string& paramNameFloat = GetPortString(pActInfo, EIP_ParamFloat);
        const string& paramNameVec3 = GetPortString(pActInfo, EIP_ParamColor);

        const SShaderItem& shaderItem = pMtl->GetShaderItem();
        IRenderShaderResources* pRendRes = shaderItem.m_pShaderResources;
        if (pRendRes == 0)
        {
            GameWarning("[flow] CFlowNodeCharAttachmentMaterialShaderParam: Entity '%s' [%d] Material '%s' (sub=%d) at Attachment '%s' [CharSlot=%d] has no render resources",
                pEntity->GetName(), pEntity->GetId(), pMtl->GetName(), subMtlId, attachment.c_str(), charSlot);
            return;
        }
        DynArrayRef<SShaderParam>& params = pRendRes->GetParameters(); // pShader->GetPublicParams();

        bool bUpdateShaderConstants = false;
        float floatValue = 0.0f;
        Vec3 vec3Value = Vec3(ZERO);
        if (bSetFloat)
        {
            floatValue = GetPortFloat(pActInfo, EIP_Float);
            if (pMtl->SetGetMaterialParamFloat(paramNameFloat, floatValue, false) == false)
            {
                UParamVal val;
                val.m_Float = floatValue;
                SShaderParam::SetParam(paramNameFloat, &params, val);
                bUpdateShaderConstants = true;
            }
        }
        if (bSetColor)
        {
            vec3Value = GetPortVec3(pActInfo, EIP_Color);
            if (pMtl->SetGetMaterialParamVec3(paramNameVec3, vec3Value, false) == false)
            {
                UParamVal val;
                val.m_Color[0] = vec3Value[0];
                val.m_Color[1] = vec3Value[1];
                val.m_Color[2] = vec3Value[2];
                val.m_Color[3] = 1.0f;
                SShaderParam::SetParam(paramNameVec3, &params, val);
                bUpdateShaderConstants = true;
            }
        }
        if (bUpdateShaderConstants)
        {
            shaderItem.m_pShaderResources->UpdateConstants(shaderItem.m_pShader);
        }
        if (bGet)
        {
            if (pMtl->SetGetMaterialParamFloat(paramNameFloat, floatValue, true))
            {
                ActivateOutput(pActInfo, EOP_Float, floatValue);
            }
            if (pMtl->SetGetMaterialParamVec3(paramNameVec3, vec3Value, true))
            {
                ActivateOutput(pActInfo, EOP_Color, vec3Value);
            }

            for (int i = 0; i < params.size(); ++i)
            {
                SShaderParam& param = params[i];
                if (azstricmp(param.m_Name.c_str(), paramNameFloat.c_str()) == 0)
                {
                    float val = 0.0f;
                    switch (param.m_Type)
                    {
                    case eType_BOOL:
                        val = param.m_Value.m_Bool;
                        break;
                    case eType_SHORT:
                        val = param.m_Value.m_Short;
                        break;
                    case eType_INT:
                        val = (float)param.m_Value.m_Int;
                        break;
                    case eType_FLOAT:
                        val = param.m_Value.m_Float;
                        break;
                    default:
                        break;
                    }
                    ActivateOutput(pActInfo, EOP_Float, val);
                }
                if (azstricmp(param.m_Name.c_str(), paramNameVec3.c_str()) == 0)
                {
                    Vec3 val (ZERO);
                    if (param.m_Type == eType_VECTOR)
                    {
                        val.Set(param.m_Value.m_Vector[0], param.m_Value.m_Vector[1], param.m_Value.m_Vector[2]);
                    }
                    if (param.m_Type == eType_FCOLOR)
                    {
                        val.Set(param.m_Value.m_Color[0], param.m_Value.m_Color[1], param.m_Value.m_Color[2]);
                    }
                    ActivateOutput(pActInfo, EOP_Color, val);
                }
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    _smart_ptr<IMaterial> m_pMaterial;
    bool m_bFailedCloningMaterial;
};


////////////////////////////////////////////////////////////////////////////////////////
// Flow node for cloning an entity's material
////////////////////////////////////////////////////////////////////////////////////////
class CFlowNodeEntityCloneMaterialOld
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNodeEntityCloneMaterialOld(SActivationInfo* pActInfo)
    {
    }

    enum EInputPorts
    {
        EIP_Clone = 0,
        EIP_Reset,
        EIP_Slot,
    };

    enum EOutputPorts
    {
        EOP_Cloned = 0,
        EOP_Reset
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void ("Clone", _HELP("Trigger to clone material")),
            InputPortConfig_Void ("Reset", _HELP("Trigger to reset material")),
            InputPortConfig<int> ("Slot", 0, _HELP("Material Slot")),
            {0}
        };

        static const SOutputPortConfig out_config[] = {
            OutputPortConfig_Void ("Cloned", _HELP("Triggered when cloned.")),
            OutputPortConfig_Void ("Reset", _HELP("Triggered when reset.")),
            {0}
        };

        config.sDescription = _HELP("Clone an Entity's Material and Reset it back to original.");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_OBSOLETE);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            const int slot = GetPortInt(pActInfo, EIP_Slot);
            UnApplyMaterial(pActInfo->pEntity, slot);
        }
        break;

        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, EIP_Reset))
            {
                const int slot = GetPortInt(pActInfo, EIP_Slot);
                UnApplyMaterial(pActInfo->pEntity, slot);
                ActivateOutput(pActInfo, EOP_Reset, true);
            }
            if (IsPortActive(pActInfo, EIP_Clone))
            {
                const int slot = GetPortInt(pActInfo, EIP_Slot);
                UnApplyMaterial(pActInfo->pEntity, slot);
                CloneAndApplyMaterial(pActInfo->pEntity, slot);
                ActivateOutput(pActInfo, EOP_Cloned, true);
            }
        }
        break;
        default:
            break;
        }
    }

    void UnApplyMaterial(IEntity* pEntity, int slot)
    {
        if (pEntity == 0)
        {
            return;
        }
        IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent == 0)
        {
            return;
        }
        pRenderComponent->SetSlotMaterial(slot, 0);
    }

    void CloneAndApplyMaterial(IEntity* pEntity, int slot)
    {
        if (pEntity == 0)
        {
            return;
        }

        IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent == 0)
        {
            return;
        }

        _smart_ptr<IMaterial> pMtl = pRenderComponent->GetSlotMaterial(slot);
        if (pMtl)
        {
            return;
        }

        pMtl = pRenderComponent->GetRenderMaterial(slot);
        if (pMtl == 0)
        {
            GameWarning("[flow] CFlowNodeEntityCloneMaterial: Entity '%s' [%d] has no material at slot %d", pEntity->GetName(), pEntity->GetId(), slot);
            return;
        }

        pMtl = gEnv->p3DEngine->GetMaterialManager()->CloneMultiMaterial(pMtl);
        pRenderComponent->SetSlotMaterial(slot, pMtl);
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


////////////////////////////////////////////////////////////////////////////////////////
// Flow node for setting entity render parameters (opacity, glow, motionblur, etc)
////////////////////////////////////////////////////////////////////////////////////////
class CFlowNodeMaterialShaderParam
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNodeMaterialShaderParam(SActivationInfo* pActInfo)
    {
    }

    enum EInputPorts
    {
        EIP_Get = 0,
        EIP_Name,
        EIP_SubMtlId,
        EIP_ParamFloat,
        EIP_Float,
        EIP_ParamColor,
        EIP_Color,
    };

    enum EOutputPorts
    {
        EOP_Float = 0,
        EOP_Color,
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void ("Get", _HELP("Trigger to get current value")),
            InputPortConfig<string> ("mat_Material", _HELP("Material Name")),
            InputPortConfig<int> ("SubMtlId", 0, _HELP("Sub Material Id")),
            InputPortConfig<string> ("ParamFloat", _HELP("Float Parameter to be set/get"), 0, _UICONFIG("dt=matparamname,name_ref=mat_Material,sub_ref=SubMtlId,param=float")),
            InputPortConfig<float>("ValueFloat", 0.0f, _HELP("Trigger to set Float value")),
            InputPortConfig<string> ("ParamColor", _HELP("Color Parameter to be set/get"), 0, _UICONFIG("dt=matparamname,name_ref=mat_Material,sub_ref=SubMtlId,param=vec")),
            InputPortConfig<Vec3>("color_ValueColor", _HELP("Trigger to set Color value")),
            {0}
        };

        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<float>("ValueFloat", _HELP("Current Float Value")),
            OutputPortConfig<Vec3> ("ValueColor",  _HELP("Current Color Value")),
            {0}
        };

        config.sDescription = _HELP("Set/Get Material's Shader Parameters");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_OBSOLETE);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event != eFE_Activate)
        {
            return;
        }

        const bool bGet = IsPortActive(pActInfo, EIP_Get);
        const bool bSetFloat = IsPortActive(pActInfo, EIP_Float);
        const bool bSetColor = IsPortActive(pActInfo, EIP_Color);

        if (!bGet && !bSetFloat && !bSetColor)
        {
            return;
        }

        const string& matName = GetPortString(pActInfo, EIP_Name);
        _smart_ptr<IMaterial> pMtl = gEnv->p3DEngine->GetMaterialManager()->FindMaterial(matName);
        if (pMtl == 0)
        {
            GameWarning("[flow] CFlowNodeMaterialParam: Material '%s' not found.", matName.c_str());
            return;
        }

        const int& subMtlId = GetPortInt(pActInfo, EIP_SubMtlId);
        pMtl = pMtl->GetSafeSubMtl(subMtlId);
        if (pMtl == 0)
        {
            GameWarning("[flow] CFlowNodeMaterialParam: Material '%s' has no sub-material %d", matName.c_str(), subMtlId);
            return;
        }

        const string& paramNameFloat = GetPortString(pActInfo, EIP_ParamFloat);
        const string& paramNameVec3 = GetPortString(pActInfo, EIP_ParamColor);

        const SShaderItem& shaderItem = pMtl->GetShaderItem();
        IRenderShaderResources* pRendRes = shaderItem.m_pShaderResources;
        if (pRendRes == 0)
        {
            GameWarning("[flow] CFlowNodeMaterialParam: Material '%s' (submtl %d) has no shader-resources.", matName.c_str(), subMtlId);
            return;
        }
        DynArrayRef<SShaderParam>& params = pRendRes->GetParameters(); // pShader->GetPublicParams();

        bool bUpdateShaderConstants = false;
        float floatValue = 0.0f;
        Vec3 vec3Value = Vec3(ZERO);
        if (bSetFloat)
        {
            floatValue = GetPortFloat(pActInfo, EIP_Float);
            if (pMtl->SetGetMaterialParamFloat(paramNameFloat, floatValue, false) == false)
            {
                UParamVal val;
                val.m_Float = floatValue;
                SShaderParam::SetParam(paramNameFloat, &params, val);
                bUpdateShaderConstants = true;
            }
        }
        if (bSetColor)
        {
            vec3Value = GetPortVec3(pActInfo, EIP_Color);
            if (pMtl->SetGetMaterialParamVec3(paramNameVec3, vec3Value, false) == false)
            {
                UParamVal val;
                val.m_Color[0] = vec3Value[0];
                val.m_Color[1] = vec3Value[1];
                val.m_Color[2] = vec3Value[2];
                val.m_Color[3] = 1.0f;
                SShaderParam::SetParam(paramNameVec3, &params, val);
                bUpdateShaderConstants = true;
            }
        }
        if (bUpdateShaderConstants)
        {
            shaderItem.m_pShaderResources->UpdateConstants(shaderItem.m_pShader);
        }

        if (bGet)
        {
            if (pMtl->SetGetMaterialParamFloat(paramNameFloat, floatValue, true))
            {
                ActivateOutput(pActInfo, EOP_Float, floatValue);
            }
            if (pMtl->SetGetMaterialParamVec3(paramNameVec3, vec3Value, true))
            {
                ActivateOutput(pActInfo, EOP_Color, vec3Value);
            }

            for (int i = 0; i < params.size(); ++i)
            {
                SShaderParam& param = params[i];
                if (azstricmp(param.m_Name.c_str(), paramNameFloat.c_str()) == 0)
                {
                    float val = 0.0f;
                    switch (param.m_Type)
                    {
                    case eType_BOOL:
                        val = param.m_Value.m_Bool;
                        break;
                    case eType_SHORT:
                        val = param.m_Value.m_Short;
                        break;
                    case eType_INT:
                        val = (float)param.m_Value.m_Int;
                        break;
                    case eType_FLOAT:
                        val = param.m_Value.m_Float;
                        break;
                    default:
                        break;
                    }
                    ActivateOutput(pActInfo, EOP_Float, val);
                }
                if (azstricmp(param.m_Name.c_str(), paramNameVec3.c_str()) == 0)
                {
                    Vec3 val (ZERO);
                    if (param.m_Type == eType_VECTOR)
                    {
                        val.Set(param.m_Value.m_Vector[0], param.m_Value.m_Vector[1], param.m_Value.m_Vector[2]);
                    }
                    if (param.m_Type == eType_FCOLOR)
                    {
                        val.Set(param.m_Value.m_Color[0], param.m_Value.m_Color[1], param.m_Value.m_Color[2]);
                    }
                    ActivateOutput(pActInfo, EOP_Color, val);
                }
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


////////////////////////////////////////////////////////////////////////////////////////
// Flow node for setting entity render parameters (opacity, glow, motionblur, etc). Same than CFlowNodeMaterialShaderParam, but it serializes the changes
////////////////////////////////////////////////////////////////////////////////////////
class CFlowNodeMaterialShaderParamSerialize
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNodeMaterialShaderParamSerialize(SActivationInfo* pActInfo)
    {
    }

    enum EInputPorts
    {
        EIP_Get = 0,
        EIP_Name,
        EIP_SubMtlId,
        EIP_ParamFloat,
        EIP_Float,
        EIP_ParamColor,
        EIP_Color,
    };

    enum EOutputPorts
    {
        EOP_Float = 0,
        EOP_Color,
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void ("Get", _HELP("Trigger to get current value")),
            InputPortConfig<string> ("mat_Material", _HELP("Material Name")),
            InputPortConfig<int> ("SubMtlId", 0, _HELP("Sub Material Id")),
            InputPortConfig<string> ("ParamFloat", _HELP("Float Parameter to be set/get"), 0, _UICONFIG("dt=matparamname,name_ref=mat_Material,sub_ref=SubMtlId,param=float")),
            InputPortConfig<float>("ValueFloat", 0.0f, _HELP("Trigger to set Float value")),
            InputPortConfig<string> ("ParamColor", _HELP("Color Parameter to be set/get"), 0, _UICONFIG("dt=matparamname,name_ref=mat_Material,sub_ref=SubMtlId,param=vec")),
            InputPortConfig<Vec3>("color_ValueColor", _HELP("Trigger to set Color value")),
            {0}
        };

        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<float>("ValueFloat", _HELP("Current Float Value")),
            OutputPortConfig<Vec3> ("ValueColor",  _HELP("Current Color Value")),
            {0}
        };

        config.sDescription = _HELP("Set/Get Material's Shader Parameters, with 'Set' serialization");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_OBSOLETE);
    }

    virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        if (ser.IsWriting())
        {
            bool validFloat = false;
            bool validColor = false;
            float floatValue = 0;
            Vec3 colorValue(0.f, 0.f, 0.f);

            GetValues(pActInfo, floatValue, colorValue, validFloat, validColor);
            ser.Value("validFloat", validFloat);
            ser.Value("validColor", validColor);
            ser.Value("floatValue", floatValue);
            ser.Value("colorValue", colorValue);
        }


        if (ser.IsReading())
        {
            bool validFloat = false;
            bool validColor = false;
            float floatValue = 0;
            Vec3 colorValue(0.f, 0.f, 0.f);

            ser.Value("validFloat", validFloat);
            ser.Value("validColor", validColor);
            ser.Value("floatValue", floatValue);
            ser.Value("colorValue", colorValue);

            SetValues(pActInfo, validFloat, validColor, floatValue, colorValue);
        }
    }


    _smart_ptr<IMaterial> ObtainMaterial(SActivationInfo* pActInfo)
    {
        const string& matName = GetPortString(pActInfo, EIP_Name);
        _smart_ptr<IMaterial> pMtl = gEnv->p3DEngine->GetMaterialManager()->FindMaterial(matName);
        if (pMtl == 0)
        {
            GameWarning("[flow] CFlowNodeMaterialShaderParamSerialize: Material '%s' not found.", matName.c_str());
            return NULL;
        }

        const int& subMtlId = GetPortInt(pActInfo, EIP_SubMtlId);
        pMtl = pMtl->GetSafeSubMtl(subMtlId);
        if (pMtl == 0)
        {
            GameWarning("[flow] CFlowNodeMaterialShaderParamSerialize: Material '%s' has no sub-material %d", matName.c_str(), subMtlId);
            return NULL;
        }

        return pMtl;
    }


    IRenderShaderResources* ObtainRendRes(SActivationInfo* pActInfo, _smart_ptr<IMaterial> pMtl)
    {
        assert(pMtl);
        const SShaderItem& shaderItem = pMtl->GetShaderItem();
        IRenderShaderResources* pRendRes = shaderItem.m_pShaderResources;
        if (pRendRes == 0)
        {
            const string& matName = GetPortString(pActInfo, EIP_Name);
            const int& subMtlId = GetPortInt(pActInfo, EIP_SubMtlId);
            GameWarning("[flow] CFlowNodeMaterialShaderParamSerialize: Material '%s' (submtl %d) has no shader-resources.", matName.c_str(), subMtlId);
        }
        return pRendRes;
    }



    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event != eFE_Activate)
        {
            return;
        }

        const bool bGet = IsPortActive(pActInfo, EIP_Get);
        const bool bSetFloat = IsPortActive(pActInfo, EIP_Float);
        const bool bSetColor = IsPortActive(pActInfo, EIP_Color);

        if (!bGet && !bSetFloat && !bSetColor)
        {
            return;
        }

        if (bSetFloat || bSetColor)
        {
            float floatValue = GetPortFloat(pActInfo, EIP_Float);
            Vec3 colorValue = GetPortVec3(pActInfo, EIP_Color);

            SetValues(pActInfo, bSetFloat, bSetColor, floatValue, colorValue);
        }

        if (bGet)
        {
            bool validFloat = false;
            bool validColor = false;
            float floatValue = 0;
            Vec3 colorValue(0.f, 0.f, 0.f);

            GetValues(pActInfo, floatValue, colorValue, validFloat, validColor);

            if (validFloat)
            {
                ActivateOutput(pActInfo, EOP_Float, floatValue);
            }
            if (validColor)
            {
                ActivateOutput(pActInfo, EOP_Color, colorValue);
            }
        }
    }


    void GetValues(SActivationInfo* pActInfo, float& floatValue, Vec3& colorValue, bool& validFloat, bool& validColor)
    {
        validFloat = false;
        validColor = false;
        const string& paramNameFloat = GetPortString(pActInfo, EIP_ParamFloat);
        const string& paramNameColor = GetPortString(pActInfo, EIP_ParamColor);

        _smart_ptr<IMaterial> pMtl = ObtainMaterial(pActInfo);
        if (!pMtl)
        {
            return;
        }
        IRenderShaderResources* pRendRes = ObtainRendRes(pActInfo, pMtl);
        if (!pRendRes)
        {
            return;
        }
        DynArrayRef<SShaderParam>& params = pRendRes->GetParameters();

        validFloat = pMtl->SetGetMaterialParamFloat(paramNameFloat, floatValue, true);
        validColor = pMtl->SetGetMaterialParamVec3(paramNameColor, colorValue, true);

        if (!validFloat)
        {
            for (int i = 0; i < params.size(); ++i)
            {
                SShaderParam& param = params[i];
                if (azstricmp(param.m_Name.c_str(), paramNameFloat.c_str()) == 0)
                {
                    switch (param.m_Type)
                    {
                    case eType_BOOL:
                        floatValue = param.m_Value.m_Bool;
                        validFloat = true;
                        break;
                    case eType_SHORT:
                        floatValue = param.m_Value.m_Short;
                        validFloat = true;
                        break;
                    case eType_INT:
                        floatValue = (float)param.m_Value.m_Int;
                        validFloat = true;
                        break;
                    case eType_FLOAT:
                        floatValue = param.m_Value.m_Float;
                        validFloat = true;
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        if (!validColor)
        {
            for (int i = 0; i < params.size(); ++i)
            {
                SShaderParam& param = params[i];

                if (azstricmp(param.m_Name.c_str(), paramNameColor.c_str()) == 0)
                {
                    if (param.m_Type == eType_VECTOR)
                    {
                        colorValue.Set(param.m_Value.m_Vector[0], param.m_Value.m_Vector[1], param.m_Value.m_Vector[2]);
                        validColor = true;
                    }
                    if (param.m_Type == eType_FCOLOR)
                    {
                        colorValue.Set(param.m_Value.m_Color[0], param.m_Value.m_Color[1], param.m_Value.m_Color[2]);
                        validColor = true;
                    }
                    break;
                }
            }
        }
    }



    void SetValues(SActivationInfo* pActInfo, bool bSetFloat, bool bSetColor, float floatValue, Vec3& colorValue)
    {
        _smart_ptr<IMaterial> pMtl = ObtainMaterial(pActInfo);
        if (!pMtl)
        {
            return;
        }

        IRenderShaderResources* pRendRes = ObtainRendRes(pActInfo, pMtl);
        if (!pRendRes)
        {
            return;
        }
        DynArrayRef<SShaderParam>& params = pRendRes->GetParameters();
        const SShaderItem& shaderItem = pMtl->GetShaderItem();

        const string& paramNameFloat = GetPortString(pActInfo, EIP_ParamFloat);
        const string& paramNameColor = GetPortString(pActInfo, EIP_ParamColor);

        bool bUpdateShaderConstants = false;
        if (bSetFloat)
        {
            if (pMtl->SetGetMaterialParamFloat(paramNameFloat, floatValue, false) == false)
            {
                UParamVal val;
                val.m_Float = floatValue;
                SShaderParam::SetParam(paramNameFloat, &params, val);
                bUpdateShaderConstants = true;
            }
        }
        if (bSetColor)
        {
            if (pMtl->SetGetMaterialParamVec3(paramNameColor, colorValue, false) == false)
            {
                UParamVal val;
                val.m_Color[0] = colorValue[0];
                val.m_Color[1] = colorValue[1];
                val.m_Color[2] = colorValue[2];
                val.m_Color[3] = 1.0f;
                SShaderParam::SetParam(paramNameColor, &params, val);
                bUpdateShaderConstants = true;
            }
        }
        if (bUpdateShaderConstants)
        {
            shaderItem.m_pShaderResources->UpdateConstants(shaderItem.m_pShader);
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
// Flow node for setting entity material layers.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EntityMaterialLayer
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_EntityMaterialLayer(SActivationInfo* pActInfo) {}

    enum INPUTS
    {
        EIP_Enable = 0,
        EIP_Disable,
        EIP_Layer
    };

    enum OUTPUTS
    {
        EOP_Done = 0,
    };

    static const char* GetUIConfig()
    {
        static CryFixedStringT<128> layers;
        if (layers.empty())
        {
            // make sure that string fits into 128 bytes, otherwise it is [safely] truncated
            layers.FormatFast("enum_int:Frozen=%d,Wet=%d,DynamicFrozen=%d", MTL_LAYER_FROZEN, MTL_LAYER_WET, MTL_LAYER_DYNAMICFROZEN);
        }
        return layers.c_str();
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Enable", _HELP("Trigger to enable the layer")),
            InputPortConfig_Void("Disable", _HELP("Trigger to disable the layer")),
            InputPortConfig<int> ("Layer", MTL_LAYER_FROZEN, _HELP("Layer to be enabled or disabled"), 0, _UICONFIG(GetUIConfig())),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig_Void ("Done", _HELP("Triggered when done")),
            {0}
        };
        config.sDescription = _HELP("Enable/Disable Entity Material Layers");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_OBSOLETE);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event != eFE_Activate)
        {
            return;
        }
        IEntity* pEntity = pActInfo->pEntity;
        if (pEntity == 0)
        {
            return;
        }
        const bool bEnable = IsPortActive(pActInfo, EIP_Enable);
        const bool bDisable = IsPortActive(pActInfo, EIP_Disable);
        if (bEnable || bDisable)
        {
            IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
            if (pRenderComponent)
            {
                uint8 layer = (uint8) GetPortInt(pActInfo, EIP_Layer);
                uint8 activeLayers = pRenderComponent->GetMaterialLayersMask();
                if (bEnable)
                {
                    activeLayers |= layer;
                }
                else
                {
                    activeLayers &= ~layer;
                }

                pRenderComponent->SetMaterialLayersMask(activeLayers);
            }
            ActivateOutput(pActInfo, EOP_Done, true);
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


class CFlowNode_SpawnEntity
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum EInputPorts
    {
        EIP_Spawn,
        EIP_ClassName,
        EIP_Name,
        EIP_Pos,
        EIP_Rot,
        EIP_Scale,
        EIP_Properties,
        EIP_PropertiesInstance,
    };

    enum EOutputPorts
    {
        EOP_Done,
        EOP_Succeeded,
        EOP_Failed,
    };

public:
    ////////////////////////////////////////////////////
    CFlowNode_SpawnEntity(SActivationInfo* pActInfo)
    {
    }

    ////////////////////////////////////////////////////
    virtual ~CFlowNode_SpawnEntity(void)
    {
    }

    ////////////////////////////////////////////////////
    virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
    }

    ////////////////////////////////////////////////////
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Spawn", _HELP("Spawn an entity using the values below")),
            InputPortConfig<string>("Class", "", _HELP("Entity class name i.e., \"BasicEntity\""), 0, 0),
            InputPortConfig<string>("Name", "", _HELP("Entity's name"), 0, 0),
            InputPortConfig<Vec3>("Pos", _HELP("Initial position")),
            InputPortConfig<Vec3>("Rot", _HELP("Initial rotation")),
            InputPortConfig<Vec3>("Scale", Vec3(1, 1, 1), _HELP("Initial scale")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Done", _HELP("Called when job is done")),
            OutputPortConfig<FlowEntityId>("Succeeded", _HELP("Called when entity is spawned")),
            OutputPortConfig_Void("Failed", _HELP("Called when entity fails to spawn")),
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Spawns an entity with the specified properties");
        config.SetCategory(EFLN_ADVANCED);
    }

    ////////////////////////////////////////////////////
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, EIP_Spawn))
            {
                // Get properties
                string className(GetPortString(pActInfo,     EIP_ClassName));
                string name(GetPortString(pActInfo,     EIP_Name));
                Vec3 pos(GetPortVec3(pActInfo,       EIP_Pos));
                Vec3 rot(GetPortVec3(pActInfo,       EIP_Rot));
                Vec3 scale(GetPortVec3(pActInfo,       EIP_Scale));

                // Define
                SEntitySpawnParams params;
                params.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(className.c_str());
                params.sName = name.c_str();
                params.vPosition = pos;
                params.vScale = scale;

                Matrix33 mat;
                Ang3 ang(DEG2RAD(rot.x), DEG2RAD(rot.y), DEG2RAD(rot.z));
                mat.SetRotationXYZ(ang);
                params.qRotation = Quat(mat);

                // Create
                IEntity* pEntity = gEnv->pEntitySystem->SpawnEntity(params);
                if (NULL == pEntity)
                {
                    ActivateOutput(pActInfo, EOP_Failed, true);
                }
                else
                {
                    ActivateOutput(pActInfo, EOP_Succeeded, pEntity->GetId());
                }
                ActivateOutput(pActInfo, EOP_Done, true);
            }
        }
        break;
        }
    }

    ////////////////////////////////////////////////////
    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

/////////////////////////////////////////////////////////////////
class CFlowNode_SpawnArchetypeEntity
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum EInputPorts
    {
        EIP_Spawn,
        EIP_ArchetypeName,
        EIP_Name,
        EIP_Pos,
        EIP_Rot,
        EIP_Scale,
    };

    enum EOutputPorts
    {
        EOP_Done,
        EOP_Succeeded,
        EOP_Failed,
    };

public:
    ////////////////////////////////////////////////////
    CFlowNode_SpawnArchetypeEntity(SActivationInfo* pActInfo)
    {
    }

    ////////////////////////////////////////////////////
    virtual ~CFlowNode_SpawnArchetypeEntity(void)
    {
    }

    ////////////////////////////////////////////////////
    virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
    }

    ////////////////////////////////////////////////////
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Spawn", _HELP("Spawn an entity using the values below")),
            InputPortConfig<string>("Archetype", "", _HELP("Entity archetype name"), 0, _UICONFIG("enum_global:entity_archetypes")),
            InputPortConfig<string>("Name", "", _HELP("Entity's name"), 0, 0),
            InputPortConfig<Vec3>("Pos", _HELP("Initial position")),
            InputPortConfig<Vec3>("Rot", _HELP("Initial rotation")),
            InputPortConfig<Vec3>("Scale", Vec3(1, 1, 1), _HELP("Initial scale")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Done", _HELP("Called when job is done")),
            OutputPortConfig<FlowEntityId>("Succeeded", _HELP("Called when entity is spawned")),
            OutputPortConfig_Void("Failed", _HELP("Called when entity fails to spawn")),
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Spawns an archetype entity with the specified properties");
        config.SetCategory(EFLN_ADVANCED);
    }

    ////////////////////////////////////////////////////
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, EIP_Spawn))
            {
                // Get properties
                string archName(GetPortString(pActInfo,     EIP_ArchetypeName));
                string name(GetPortString(pActInfo,     EIP_Name));
                Vec3 pos(GetPortVec3(pActInfo,       EIP_Pos));
                Vec3 rot(GetPortVec3(pActInfo,       EIP_Rot));
                Vec3 scale(GetPortVec3(pActInfo,       EIP_Scale));

                // Define
                IEntity* pEntity = NULL;
                SEntitySpawnParams params;
                IEntityArchetype* pArchetype = gEnv->pEntitySystem->LoadEntityArchetype(archName.c_str());
                if (NULL != pArchetype)
                {
                    params.pArchetype = pArchetype;
                    params.sName = name.empty() ? pArchetype->GetName() : name.c_str();
                    params.vPosition = pos;
                    params.vScale = scale;

                    Matrix33 mat;
                    Ang3 ang(DEG2RAD(rot.x), DEG2RAD(rot.y), DEG2RAD(rot.z));
                    mat.SetRotationXYZ(ang);
                    params.qRotation = Quat(mat);

                    // Create
                    int nCastShadowMinSpec;
                    pArchetype->GetObjectVars()->getAttr("CastShadowMinSpec", nCastShadowMinSpec);

                    static ICVar* pObjShadowCastSpec = gEnv->pConsole->GetCVar("e_ObjShadowCastSpec");
                    if (nCastShadowMinSpec <= pObjShadowCastSpec->GetIVal())
                    {
                        params.nFlags  |= ENTITY_FLAG_CASTSHADOW;
                    }

                    bool bRecvWind;
                    pArchetype->GetObjectVars()->getAttr("RecvWind", bRecvWind);
                    if (bRecvWind)
                    {
                        params.nFlags  |= ENTITY_FLAG_RECVWIND;
                    }

                    bool bOutdoorOnly;
                    pArchetype->GetObjectVars()->getAttr("OutdoorOnly", bOutdoorOnly);
                    if (bRecvWind)
                    {
                        params.nFlags  |= ENTITY_FLAG_OUTDOORONLY;
                    }

                    bool bNoStaticDecals;
                    pArchetype->GetObjectVars()->getAttr("NoStaticDecals", bNoStaticDecals);
                    if (bNoStaticDecals)
                    {
                        params.nFlags  |= ENTITY_FLAG_NO_DECALNODE_DECALS;
                    }

                    pEntity = gEnv->pEntitySystem->SpawnEntity(params);
                    if (pEntity)
                    {
                        IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
                        if (pRenderComponent)
                        {
                            float fViewDistanceMultiplier = 1.0f;
                            pArchetype->GetObjectVars()->getAttr("ViewDistanceMultiplier", fViewDistanceMultiplier);
                            pRenderComponent->SetViewDistanceMultiplier(fViewDistanceMultiplier);

                            int nLodRatio = 100;
                            pArchetype->GetObjectVars()->getAttr("lodRatio", nLodRatio);
                            pRenderComponent->SetLodRatio(nLodRatio);
                        }

                        bool bHiddenInGame = false;
                        pArchetype->GetObjectVars()->getAttr("HiddenInGame", bHiddenInGame);
                        if (bHiddenInGame)
                        {
                            pEntity->Hide(true);
                        }
                    }
                }

                if (pEntity == NULL)
                {
                    ActivateOutput(pActInfo, EOP_Failed, true);
                }
                else
                {
                    ActivateOutput(pActInfo, EOP_Succeeded, pEntity->GetId());
                }
                ActivateOutput(pActInfo, EOP_Done, true);
            }
        }
        break;
        }
    }

    ////////////////////////////////////////////////////
    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
// Call a LUA function on an entity
class CFlowNode_CallScriptFunction
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    enum EInputs
    {
        IN_CALL,
        IN_FUNCNAME,
        IN_ARG1,
        IN_ARG2,
        IN_ARG3,
        IN_ARG4,
        IN_ARG5,
    };
    enum EOutputs
    {
        OUT_SUCCESS,
        OUT_FAILED
    };

    CFlowNode_CallScriptFunction(SActivationInfo* pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_CallScriptFunction(pActInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config)
    {
        // declare input ports
        static const SInputPortConfig in_ports[] =
        {
            InputPortConfig_Void("call", _HELP("Call the function")),
            InputPortConfig<string>("FuncName", _HELP("Script function name")),
            InputPortConfig_AnyType("Argument1", _HELP("Argument 1")),
            InputPortConfig_AnyType("Argument2", _HELP("Argument 2")),
            InputPortConfig_AnyType("Argument3", _HELP("Argument 3")),
            InputPortConfig_AnyType("Argument4", _HELP("Argument 4")),
            InputPortConfig_AnyType("Argument5", _HELP("Argument 5")),
            {0}
        };
        static const SOutputPortConfig out_ports[] = {
            OutputPortConfig_Void("Success", _HELP("Script function was found and called")),
            OutputPortConfig_Void("Failed", _HELP("Script function was not found")),
            {0}
        };
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_ports;
        config.pOutputPorts = out_ports;
        config.sDescription = _HELP("Calls a script function on the entity");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (eFE_Activate == event && IsPortActive(pActInfo, IN_CALL))
        {
            if (pActInfo->pEntity)
            {
                //Get entity's scripttable
                IComponentScriptPtr scriptComponent = pActInfo->pEntity->GetComponent<IComponentScript>();
                if (scriptComponent)
                {
                    IScriptTable* pTable = scriptComponent->GetScriptTable();
                    if (pTable)
                    {
                        //Convert all inputports to arguments for lua
                        ScriptAnyValue arg1 = FillArgumentFromAnyPort(pActInfo, IN_ARG1);
                        ScriptAnyValue arg2 = FillArgumentFromAnyPort(pActInfo, IN_ARG2);
                        ScriptAnyValue arg3 = FillArgumentFromAnyPort(pActInfo, IN_ARG3);
                        ScriptAnyValue arg4 = FillArgumentFromAnyPort(pActInfo, IN_ARG4);
                        ScriptAnyValue arg5 = FillArgumentFromAnyPort(pActInfo, IN_ARG5);

                        Script::CallMethod(pTable, GetPortString(pActInfo, IN_FUNCNAME), arg1, arg2, arg3, arg4, arg5);

                        ActivateOutput(pActInfo, OUT_SUCCESS, true);
                        return;
                    }
                }
            }

            ActivateOutput(pActInfo, OUT_FAILED, true);
        }
    }

    ScriptAnyValue FillArgumentFromAnyPort(SActivationInfo* pActInfo, int port)
    {
        TFlowInputData inputData = GetPortAny(pActInfo, port);

        switch (inputData.GetType())
        {
        case eFDT_Int:
            return ScriptAnyValue((float)GetPortInt(pActInfo, port));
        case eFDT_EntityId:
        {
            ScriptHandle id;
            id.n = FlowEntityId(GetPortEntityId(pActInfo, port));
            return ScriptAnyValue(id);
        }
        case eFDT_Bool:
            return ScriptAnyValue(GetPortBool(pActInfo, port));
        case eFDT_Float:
            return ScriptAnyValue(GetPortFloat(pActInfo, port));
        case eFDT_Double:
            return ScriptAnyValue(GetPortDouble(pActInfo, port));
        case eFDT_String:
            return ScriptAnyValue(GetPortString(pActInfo, port));
            ;
        case eFDT_Vec3:
            return ScriptAnyValue(GetPortVec3(pActInfo, port));
            ;
        case eFDT_CustomData:
            // not currently supported
            return ScriptAnyValue();
        }

        //Type unknown
        assert(false);

        return ScriptAnyValue();
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
// Returns the entity ID of the gamerules
class CFlowNode_GetGameRulesEntityId
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    enum EInputs
    {
        IN_GET
    };
    enum EOutputs
    {
        OUT_ID
    };

    CFlowNode_GetGameRulesEntityId(SActivationInfo* pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_GetGameRulesEntityId(pActInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config)
    {
        // declare input ports
        static const SInputPortConfig in_ports[] =
        {
            InputPortConfig_Void("Get", _HELP("Retrieve the entityId")),
            {0}
        };
        static const SOutputPortConfig out_ports[] = {
            OutputPortConfig<FlowEntityId>("EntityId", _HELP("The entityId of the Gamerules")),
            {0}
        };

        config.pInputPorts = in_ports;
        config.pOutputPorts = out_ports;
        config.sDescription = _HELP("Calls a script function on the entity");
        config.SetCategory(EFLN_DEBUG);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (eFE_Activate == event && IsPortActive(pActInfo, IN_GET))
        {
            IEntity* pGameRules = CCryAction::GetCryAction()->GetIGameRulesSystem()->GetCurrentGameRulesEntity();
            ActivateOutput(pActInfo, OUT_ID, pGameRules->GetId());
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

class CFlowNode_FindEntityByName
    : public CFlowBaseNode< eNCT_Singleton >
{
public:
    CFlowNode_FindEntityByName(SActivationInfo* pActInfo)
    {
    }

    enum EInPorts
    {
        eIP_Set = 0,
        eIP_Name
    };
    enum EOutPorts
    {
        eOP_EntityId = 0
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void   ("Set", _HELP("Start search for entity with the specified name.")),
            InputPortConfig<string>("Name", string(), _HELP("Name of the entity to look for.")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<FlowEntityId>("EntityId", _HELP("Outputs the entity's ID if found, 0 otherwise.")),
            {0}
        };
        config.sDescription = _HELP("Searches for an entity using its name");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate)
        {
            if (IsPortActive(pActInfo, eIP_Set))
            {
                const string& name = GetPortString(pActInfo, eIP_Name);
                IEntity* pEntity = gEnv->pEntitySystem->FindEntityByName(name.c_str());
                ActivateOutput(pActInfo, eOP_EntityId, pEntity ? pEntity->GetId() : FlowEntityId(0));
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

class CFlowNode_RemoveEntity
    : public CFlowBaseNode < eNCT_Instanced >
{
public:

    enum class InputPorts
    {
        Activate,
        Entity
    };

    enum class OutputPorts
    {
        Done
    };

    CFlowNode_RemoveEntity(SActivationInfo* pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_RemoveEntity(pActInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<FlowEntityId>("Entity", _HELP("The entity to be removed from the world")),

            { 0 }
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_Void("Done", _HELP("Fired after telling the entity system to remove the entity")),

            { 0 }
        };

        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.sDescription = _HELP("Removes an entity from the world");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        if (event == eFE_Activate)
        {
            if (IsPortActive(pActInfo, (int)InputPorts::Activate))
            {
                EntityId entityId = GetPortEntityId(pActInfo, (int)InputPorts::Entity);
                gEnv->pEntitySystem->RemoveEntity(entityId);
                ActivateOutput(pActInfo, (int)OutputPorts::Done, 0);
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

REGISTER_FLOW_NODE("Entity:EntityPos", CFlowNode_EntityPos)
REGISTER_FLOW_NODE("Entity:GetPos", CFlowNode_EntityGetPos)
REGISTER_FLOW_NODE("Entity:BroadcastEvent", CFlowNode_BroadcastEntityEvent)
REGISTER_FLOW_NODE("Entity:EntityId", CFlowNode_EntityId)
REGISTER_FLOW_NODE("Entity:EntityInfo", CFlowNode_EntityGetInfo)
REGISTER_FLOW_NODE("Entity:GetEntityExistence", CFlowNode_EntityGetExistence)
REGISTER_FLOW_NODE("Entity:ParentId", CFlowNode_ParentId)
REGISTER_FLOW_NODE("Entity:PropertySet", CFlowNode_EntitySetProperty)
REGISTER_FLOW_NODE("Entity:PropertyGet", CFlowNode_EntityGetProperty)
REGISTER_FLOW_NODE("Entity:Material", CFlowNode_EntityMaterial)
REGISTER_FLOW_NODE("Entity:MaterialSerialize", CFlowNode_EntityMaterialSerialize)
REGISTER_FLOW_NODE("Entity:MaterialLayer", CFlowNode_EntityMaterialLayer)
REGISTER_FLOW_NODE("Entity:ChildAttach", CFlowNode_EntityAttachChild)
REGISTER_FLOW_NODE("Entity:ChildDetach", CFlowNode_EntityDetachThis)
REGISTER_FLOW_NODE("Entity:GetPlayer", CFlowNode_EntityGetPlayer)
REGISTER_FLOW_NODE("Entity:BeamEntity", CFlowNode_BeamEntity)
REGISTER_FLOW_NODE("Entity:RenderParams", CFlowNodeRenderParams)
// REGISTER_FLOW_NODE( "Entity:MaterialParam",CFlowNodeMaterialParam )
REGISTER_FLOW_NODE("Entity:MaterialParam", CFlowNodeEntityMaterialShaderParam)
REGISTER_FLOW_NODE("Entity:MaterialClone", CFlowNodeEntityCloneMaterialOld)
REGISTER_FLOW_NODE("entity:TagPoint", CFlowNode_EntityTagpoint)
REGISTER_FLOW_NODE("Entity:CheckDistance", CFlowNode_EntityCheckDistance)
REGISTER_FLOW_NODE("Entity:GetBounds", CFlowNode_EntityGetBounds)

REGISTER_FLOW_NODE("Entity:CharAttachmentMaterialParam", CFlowNodeCharAttachmentMaterialShaderParam)

REGISTER_FLOW_NODE("Entity:EntityPool", CFlowNode_EntityPool)
REGISTER_FLOW_NODE("Entity:EntityFaceAt", CFlowNode_EntityFaceAt)

REGISTER_FLOW_NODE("Entity:FindEntityByName", CFlowNode_FindEntityByName);
REGISTER_FLOW_NODE("Entity:RemoveEntity", CFlowNode_RemoveEntity);

// engine based
REGISTER_FLOW_NODE("Environment:MaterialParam", CFlowNodeMaterialShaderParam)
REGISTER_FLOW_NODE("Environment:MaterialParamSerialize", CFlowNodeMaterialShaderParamSerialize)

//spawn nodes
REGISTER_FLOW_NODE("Entity:Spawn", CFlowNode_SpawnEntity);
REGISTER_FLOW_NODE("Entity:SpawnArchetype", CFlowNode_SpawnArchetypeEntity);

//Call lua functions
REGISTER_FLOW_NODE("Entity:CallScriptFunction", CFlowNode_CallScriptFunction);
REGISTER_FLOW_NODE("Game:GetGameRulesEntityId", CFlowNode_GetGameRulesEntityId);
