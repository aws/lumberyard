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

// Description : Flowgraph Nodes for module usage


#include "CryLegacy_precompiled.h"
#include "FlowModuleNodes.h"
#include "IFlowSystem.h"
#include "ModuleManager.h"
#include "ILevelSystem.h"
#include "FlowSystem/FlowSystem.h"


//////////////////////////////////////////////////////////////////////////
//
// Start Node factory
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CFlowModuleStartNodeFactory::CFlowModuleStartNodeFactory(TModuleId moduleId)
{
    m_nRefCount = 0;
    m_moduleId = moduleId;
}

//////////////////////////////////////////////////////////////////////////
CFlowModuleStartNodeFactory::~CFlowModuleStartNodeFactory()
{
    Reset();
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleStartNodeFactory::Reset()
{
    stl::free_container(m_outputs);
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleStartNodeFactory::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] =
    {
        {0}
    };
    static const SOutputPortConfig out_config[] =
    {
        {0}
    };

    config.nFlags |= EFLN_HIDE_UI | EFLN_UNREMOVEABLE;
    config.SetCategory(EFLN_APPROVED);

    config.pInputPorts = in_config;
    config.pOutputPorts = out_config;

    IFlowGraphModule* pModule = gEnv->pFlowSystem->GetIModuleManager()->GetModule(m_moduleId);
    assert(pModule);

    if (pModule)
    {
        stl::free_container(m_outputs);

        SOutputPortConfig start;
        start.type = eFDT_Any;
        start.name = "Start";
        start.description = "Activates when the module starts";
        start.humanName = NULL;
        m_outputs.push_back(start);

        SOutputPortConfig update;
        update.type = eFDT_Any;
        update.name = "Update";
        update.description = "Activates when the module is updated";
        update.humanName = NULL;
        m_outputs.push_back(update);

        SOutputPortConfig cancel;
        cancel.type = eFDT_Any;
        cancel.name = "Cancel";
        cancel.description = "Activates when the module is cancelled";
        cancel.humanName = NULL;
        m_outputs.push_back(cancel);

        size_t count = pModule->GetModulePortCount();
        for (size_t i = 0; i < count; ++i)
        {
            const IFlowGraphModule::SModulePortConfig* pPort = pModule->GetModulePort(i);

            // inputs to the module should be outputs for the Start node
            if (pPort && pPort->input)
            {
                SOutputPortConfig out;
                out.type = pPort->type;
                out.name = pPort->name;
                out.description = NULL;
                out.humanName = NULL;
                m_outputs.push_back(out);
            }
        }

        m_outputs.push_back(OutputPortConfig<bool>(0, 0));
    }

    if (!m_outputs.empty())
    {
        config.pOutputPorts = &m_outputs[0];
    }
}

//////////////////////////////////////////////////////////////////////////
IFlowNodePtr CFlowModuleStartNodeFactory::Create(IFlowNode::SActivationInfo* pActInfo)
{
    return new CFlowModuleStartNode(this, pActInfo);
}

//////////////////////////////////////////////////////////////////////////
//
// Start Node
//
//////////////////////////////////////////////////////////////////////////
CFlowModuleStartNode::CFlowModuleStartNode(CFlowModuleStartNodeFactory* pClass, SActivationInfo* pActInfo)
    : m_pClass(pClass)
    , m_bStarted(false)
{
}

//////////////////////////////////////////////////////////////////////////
IFlowNodePtr CFlowModuleStartNode::Clone(SActivationInfo* pActInfo)
{
    IFlowNode* pNode = new CFlowModuleStartNode(m_pClass, pActInfo);
    return pNode;
};

//////////////////////////////////////////////////////////////////////////
void CFlowModuleStartNode::GetConfiguration(SFlowNodeConfig& config)
{
    m_pClass->GetConfiguration(config);
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleStartNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    if (eFE_Initialize == event)
    {
        m_actInfo = *pActInfo;
        m_bStarted = false;
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleStartNode::OnActivate(TModuleParams const& params)
{
    // number of params passed in should match the number of outputs-eOP_Param1-1
    //   (since one output is the normal Start port, one is Cancel, and one is a 'dummy'    null port)
    assert(params.size() == m_pClass->m_outputs.size() - eOP_Param1 - 1);

    ActivateOutput(&m_actInfo, m_bStarted ? eOP_Update : eOP_Start, true);

    for (size_t i = 0; i < params.size(); ++i)
    {
        ActivateOutput(&m_actInfo, eOP_Param1 + i, params[i]);
    }
    m_bStarted = true;
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleStartNode::OnCancel()
{
    ActivateOutput(&m_actInfo, eOP_Cancel, true);
}

//////////////////////////////////////////////////////////////////////////
//
// Return node factory
//
//////////////////////////////////////////////////////////////////////////

CFlowModuleReturnNodeFactory::CFlowModuleReturnNodeFactory(TModuleId moduleId)
{
    m_nRefCount = 0;
    m_moduleId = moduleId;
}

//////////////////////////////////////////////////////////////////////////
CFlowModuleReturnNodeFactory::~CFlowModuleReturnNodeFactory()
{
    Reset();
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleReturnNodeFactory::Reset()
{
    stl::free_container(m_inputs);
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleReturnNodeFactory::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] =
    {
        {0}
    };
    static const SOutputPortConfig out_config[] =
    {
        {0}
    };

    config.nFlags |= EFLN_HIDE_UI | EFLN_UNREMOVEABLE;
    config.SetCategory(EFLN_APPROVED);

    config.pInputPorts = in_config;
    config.pOutputPorts = out_config;

    IFlowGraphModule* pModule = gEnv->pFlowSystem->GetIModuleManager()->GetModule(m_moduleId);
    assert(pModule);

    if (pModule)
    {
        stl::free_container(m_inputs);

        m_inputs.push_back(InputPortConfig_AnyType("Success", "Activate to end this module (success)"));
        m_inputs.push_back(InputPortConfig_AnyType("Cancel", "Activate to end this module (canceled)"));

        size_t count = pModule->GetModulePortCount();
        for (size_t i = 0; i < count; ++i)
        {
            const IFlowGraphModule::SModulePortConfig* pPort = pModule->GetModulePort(i);

            // outputs from the module should be inputs for the Return node
            if (pPort && !pPort->input)
            {
                switch (pPort->type)
                {
                case eFDT_Any:
                {
                    m_inputs.push_back(InputPortConfig_AnyType(pPort->name));
                    break;
                }
                case eFDT_Bool:
                {
                    m_inputs.push_back(InputPortConfig<bool>(pPort->name));
                    break;
                }
                case eFDT_Int:
                {
                    m_inputs.push_back(InputPortConfig<int>(pPort->name));
                    break;
                }
                case eFDT_Float:
                {
                    m_inputs.push_back(InputPortConfig<float>(pPort->name));
                    break;
                }
                case eFDT_Double:
                {
                    m_inputs.push_back(InputPortConfig<double>(pPort->name));
                    break;
                }
                case eFDT_Vec3:
                {
                    m_inputs.push_back(InputPortConfig<Vec3>(pPort->name));
                    break;
                }
                case eFDT_EntityId:
                {
                    m_inputs.push_back(InputPortConfig<FlowEntityId>(pPort->name));
                    break;
                }
                case eFDT_String:
                {
                    m_inputs.push_back(InputPortConfig<string>(pPort->name));
                    break;
                }
                default:
                {
                    assert(false);
                    break;
                }
                }
            }
        }

        m_inputs.push_back(InputPortConfig<bool>(0, false));
    }

    if (!m_inputs.empty())
    {
        config.pInputPorts = &m_inputs[0];
    }
}

//////////////////////////////////////////////////////////////////////////
IFlowNodePtr CFlowModuleReturnNodeFactory::Create(IFlowNode::SActivationInfo* pActInfo)
{
    return new CFlowModuleReturnNode(this, pActInfo, m_moduleId);
}

//////////////////////////////////////////////////////////////////////////
//
// Return node
//
//////////////////////////////////////////////////////////////////////////
CFlowModuleReturnNode::CFlowModuleReturnNode(CFlowModuleReturnNodeFactory* pClass, SActivationInfo* pActInfo, TModuleId moduleId)
{
    m_pClass = pClass;
    m_ownerId = moduleId;
    m_instanceId = MODULEINSTANCE_INVALID;
}

//////////////////////////////////////////////////////////////////////////
IFlowNodePtr CFlowModuleReturnNode::Clone(SActivationInfo* pActInfo)
{
    IFlowNode* pNode = new CFlowModuleReturnNode(m_pClass, pActInfo, m_ownerId);
    return pNode;
};

//////////////////////////////////////////////////////////////////////////
void CFlowModuleReturnNode::GetConfiguration(SFlowNodeConfig& config)
{
    m_pClass->GetConfiguration(config);
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleReturnNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    if (eFE_Initialize == event)
    {
        m_actInfo = *pActInfo;
    }
    else if (eFE_Activate == event)
    {
        if (IsPortActive(pActInfo, eIP_Success))
        {
            OnActivate(true);
        }
        else if (IsPortActive(pActInfo, eIP_Cancel))
        {
            OnActivate(false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleReturnNode::OnActivate(bool success)
{
    if (MODULEID_INVALID != m_ownerId)
    {
        CFlowGraphModuleManager* pModuleManager = static_cast<CFlowSystem*>(gEnv->pFlowSystem)->GetModuleManager();
        assert(pModuleManager);

        // Construct params from ports
        TModuleParams params;
        for (size_t i = eIP_Param1; i < m_pClass->m_inputs.size() - 1; ++i)
        {
            params.push_back(GetPortAny(&m_actInfo, i));
        }

        // Notify manager
        pModuleManager->OnModuleFinished(m_ownerId, m_instanceId, success, params);
        if (m_instanceId != MODULEINSTANCE_INVALID)
        {
            CFlowModuleUserIdNode::RemoveModuleInstance(m_instanceId);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//
// Call node factory
//
//////////////////////////////////////////////////////////////////////////

CFlowModuleCallNodeFactory::CFlowModuleCallNodeFactory(TModuleId moduleId)
{
    m_nRefCount = 0;
    m_moduleId = moduleId;
}

//////////////////////////////////////////////////////////////////////////
CFlowModuleCallNodeFactory::~CFlowModuleCallNodeFactory()
{
    Reset();
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleCallNodeFactory::Reset()
{
    stl::free_container(m_inputs);
    stl::free_container(m_outputs);
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleCallNodeFactory::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] =
    {
        {0}
    };
    static const SOutputPortConfig out_config[] =
    {
        {0}
    };

    config.SetCategory(EFLN_APPROVED);

    config.pInputPorts = in_config;
    config.pOutputPorts = out_config;

    IFlowGraphModule* pModule = gEnv->pFlowSystem->GetIModuleManager()->GetModule(m_moduleId);
    assert(pModule);
    if (pModule)
    {
        stl::free_container(m_inputs);
        stl::free_container(m_outputs);

        m_inputs.push_back(InputPortConfig_AnyType("Call", "Activate to call this module, if module is already started it will update the Start node with updated parameters"));
        m_inputs.push_back(InputPortConfig<bool>("Instanced", true, "Whether this module is instanced or not. Instanced modules can have multiple instances running"));
        m_inputs.push_back(InputPortConfig_AnyType("Cancel", "Activate to cancel this module (needs correct InstanceID if instanced)"));
        m_inputs.push_back(InputPortConfig<int>("InstanceID", -1, "InstanceID to identify instance. Can be -1 to create a new instance, otherwise it will update the given instance (only if instanced)"));

        SOutputPortConfig called;
        called.description = "Called when module was started (returns -1 if not instanced)";
        called.humanName = NULL;
        called.name = "OnCalled";
        called.type = eFDT_Int;
        m_outputs.push_back(called);

        SOutputPortConfig done;
        done.description = "Called when module returns with a successful status";
        done.humanName = NULL;
        done.name = "Done";
        done.type = eFDT_Any;
        m_outputs.push_back(done);

        SOutputPortConfig cancel;
        cancel.description = "Called when the module returns with a fail status";
        cancel.humanName = NULL;
        cancel.name = "Canceled";
        cancel.type = eFDT_Any;
        m_outputs.push_back(cancel);

        size_t count = pModule->GetModulePortCount();
        for (size_t i = 0; i < count; ++i)
        {
            const IFlowGraphModule::SModulePortConfig* pPort = pModule->GetModulePort(i);

            // inputs for the module should be inputs for the Call node, and vv
            if (pPort && pPort->input)
            {
                switch (pPort->type)
                {
                case eFDT_Any:
                {
                    m_inputs.push_back(InputPortConfig_AnyType(pPort->name));
                    break;
                }
                case eFDT_Bool:
                {
                    m_inputs.push_back(InputPortConfig<bool>(pPort->name));
                    break;
                }
                case eFDT_Int:
                {
                    m_inputs.push_back(InputPortConfig<int>(pPort->name));
                    break;
                }
                case eFDT_Float:
                {
                    m_inputs.push_back(InputPortConfig<float>(pPort->name));
                    break;
                }
                case eFDT_Double:
                {
                    m_inputs.push_back(InputPortConfig<double>(pPort->name));
                    break;
                }
                case eFDT_Vec3:
                {
                    m_inputs.push_back(InputPortConfig<Vec3>(pPort->name));
                    break;
                }
                case eFDT_EntityId:
                {
                    m_inputs.push_back(InputPortConfig<FlowEntityId>(pPort->name));
                    break;
                }
                case eFDT_String:
                {
                    m_inputs.push_back(InputPortConfig<string>(pPort->name));
                    break;
                }
                default:
                {
                    assert(false);
                    break;
                }
                }
            }
            else if (pPort)
            {
                SOutputPortConfig out;
                out.description = NULL;
                out.name = pPort->name;
                out.type = pPort->type;
                out.humanName = NULL;
                m_outputs.push_back(out);
            }
        }

        m_outputs.push_back(OutputPortConfig<bool>(0, 0));
        m_inputs.push_back(InputPortConfig<bool>(0, false));
    }

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
IFlowNodePtr CFlowModuleCallNodeFactory::Create(IFlowNode::SActivationInfo* pActInfo)
{
    return new CFlowModuleCallNode(this, pActInfo);
}

//////////////////////////////////////////////////////////////////////////
//
// Call node
//
//////////////////////////////////////////////////////////////////////////

CFlowModuleCallNode::CFlowModuleCallNode(CFlowModuleCallNodeFactory* pClass, SActivationInfo* pActInfo)
    : m_instanceId(MODULEINSTANCE_INVALID)
{
    m_pClass = pClass;
}

//////////////////////////////////////////////////////////////////////////
IFlowNodePtr CFlowModuleCallNode::Clone(SActivationInfo* pActInfo)
{
    IFlowNode* pNode = new CFlowModuleCallNode(m_pClass, pActInfo);
    return pNode;
};

//////////////////////////////////////////////////////////////////////////
void CFlowModuleCallNode::GetConfiguration(SFlowNodeConfig& config)
{
    m_pClass->GetConfiguration(config);
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleCallNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    if (eFE_Initialize == event)
    {
        m_actInfo = *pActInfo;
        m_instanceId = MODULEINSTANCE_INVALID;
    }
    else if (eFE_Activate == event)
    {
        const bool instanced = GetPortBool(pActInfo, eIP_Instanced);
        const TModuleInstanceId instanceId = static_cast<TModuleInstanceId>(GetPortInt(pActInfo, eIP_InstanceId));

        if (instanceId != MODULEINSTANCE_INVALID)
        {
            m_instanceId = instanceId;
        }

        if (IsPortActive(pActInfo, eIP_Call))
        {
            // Extract and load parameters
            TModuleParams params;

            for (size_t i = eIP_Param1; i < m_pClass->m_inputs.size() - 1; ++i)
            {
                params.push_back(GetPortAny(pActInfo, i));
            }

            CFlowGraphModuleManager* pModuleManager = static_cast<CFlowSystem*>(gEnv->pFlowSystem)->GetModuleManager();
            assert(pModuleManager);

            // If no existing module instance, create one
            if (instanced && m_instanceId == MODULEINSTANCE_INVALID)
            {
                m_instanceId = pModuleManager->CreateModuleInstance(m_pClass->m_moduleId, pActInfo->pGraph->GetGraphId(), pActInfo->myID, params, functor(*this, &CFlowModuleCallNode::OnReturn));
                ActivateOutput(pActInfo, eOP_OnCall, m_instanceId);
            }
            else if (m_instanceId != MODULEINSTANCE_INVALID)
            {
                // Update the existing instance with the new parameters
                pModuleManager->RefreshModuleInstance(m_pClass->m_moduleId, m_instanceId, params);
                ActivateOutput(pActInfo, eOP_OnCall, instanced ? m_instanceId : -1);
            }
        }
        else if (IsPortActive(pActInfo, eIP_Cancel))
        {
            CFlowGraphModuleManager* pModuleManager = static_cast<CFlowSystem*>(gEnv->pFlowSystem)->GetModuleManager();
            assert(pModuleManager);

            if (instanced && m_instanceId == MODULEINSTANCE_INVALID)
            {
                IFlowGraphModule* pModule = pModuleManager->GetModule(m_pClass->m_moduleId);
                GameWarning("Attempting to cancel a flowgraph module started with invalid instance Id: module is %s", pModule ? pModule->GetName() : "<not found>");
            }
            else if (m_instanceId != MODULEINSTANCE_INVALID)
            {
                pModuleManager->CancelModuleInstance(m_pClass->m_moduleId, m_instanceId);
                m_instanceId = MODULEINSTANCE_INVALID;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleCallNode::OnReturn(bool bSuccess, TModuleParams const& params)
{
    if (bSuccess)
    {
        ActivateOutput(&m_actInfo, eOP_Success, true);
    }
    else
    {
        ActivateOutput(&m_actInfo, eOP_Canceled, true);
    }

    assert(m_pClass->m_outputs.size() - eOP_Param1 - 1 == params.size());
    for (size_t i = eOP_Param1; i < m_pClass->m_outputs.size() - 1; ++i)
    {
        ActivateOutput(&m_actInfo, i, params[i - eOP_Param1]);
    }

    m_instanceId = MODULEINSTANCE_INVALID;
}

//////////////////////////////////////////////////////////////////////////
//
// Module ID map node
//
//////////////////////////////////////////////////////////////////////////
std::map<int, TModuleInstanceId> CFlowModuleUserIdNode::m_ids;

void CFlowModuleUserIdNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] =
    {
        InputPortConfig_Void("Get", "Get Module instance id for given user id"),
        InputPortConfig_Void("Set", "Set Module instance id for given user id"),
        InputPortConfig<int>("UserID", 0, "Custom user id (e.g. an entity id)"),
        InputPortConfig<int>("ModuleID", -1, "Module instance id"),
        {0}
    };

    static const SOutputPortConfig out_config[] =
    {
        OutputPortConfig<int>("ModuleId",   _HELP("Module instance id for given user id (returns -1 if not id was found)")),
        {0}
    };

    config.sDescription = "Node to map an user id to a module instance id";
    config.pInputPorts = in_config;
    config.pOutputPorts = out_config;
    config.SetCategory(EFLN_APPROVED);
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleUserIdNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    switch (event)
    {
    case eFE_Initialize:
    {
        m_ids.clear();
        break;
    }
    case eFE_Activate:
    {
        if (IsPortActive(pActInfo, InputPorts::Get))
        {
            const int userid = GetPortInt(pActInfo, InputPorts::UserId);
            std::map<int, TModuleInstanceId>::const_iterator it = m_ids.find(userid);
            ActivateOutput(pActInfo, OutputPorts::OutModuleId, it != m_ids.end() ? it->second : -1);
        }
        if (IsPortActive(pActInfo, InputPorts::Set))
        {
            const int moduleid = GetPortInt(pActInfo, InputPorts::ModuleId);
            const int userid = GetPortInt(pActInfo, InputPorts::UserId);
            m_ids[userid] = moduleid;
            ActivateOutput(pActInfo, OutputPorts::OutModuleId, moduleid);
        }
        break;
    }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowModuleUserIdNode::RemoveModuleInstance(TModuleInstanceId id)
{
    for (std::map<int, TModuleInstanceId>::iterator it = m_ids.begin(); it != m_ids.end(); ++it)
    {
        if (it->second == id)
        {
            m_ids.erase(it);
            return;
        }
    }
}


REGISTER_FLOW_NODE("Module:Utils:UserIDToModuleID", CFlowModuleUserIdNode);
