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
#include "FlowEntityNode.h"
#include "Mannequin/Serialization.h"
#include "CryExtension/Impl/ClassWeaver.h"

#include <LmbrCentral/Animation/MannequinComponentBus.h>

//////////////////////////////////////////////////////////////////////////
// CFlowPlayMannequinFragment
//////////////////////////////////////////////////////////////////////////

namespace MannequinFlowNodeHelpers
{
    IActionController* GetEntityActionController(const FlowEntityId& id)
    {
        IActionController* actionController = nullptr;

        const AZ::EntityId entityId = id;
        if (IsLegacyEntityId(entityId))
        {
            IGameFramework* gameFramework = gEnv->pGame->GetIGameFramework();
            IGameObject* gameObject = gameFramework ? gameFramework->GetGameObject(id) : nullptr;
            IAnimatedCharacter* animChar = gameObject ? static_cast<IAnimatedCharacter*>(gameObject->QueryExtension("AnimatedCharacter")) : nullptr;
            if (animChar)
            {
                actionController = animChar->GetActionController();
            }
            AZ_Warning("Mannequin Flow Nodes", actionController, "Entity [%llu] does not have an AnimatedCharacter game object extension. Flow node will not be able to trigger fragments.", static_cast<AZ::u64>(entityId));
        }
        else
        {
            EBUS_EVENT_ID_RESULT(actionController, id, LmbrCentral::MannequinRequestsBus, GetActionController);
            AZ_Warning("Mannequin Flow Nodes", actionController, "Entity [%llu] does not have a mannequin component. Flow node will not be able to trigger fragments.", static_cast<AZ::u64>(entityId));
        }

        return actionController;
    }

} // namespace MannequinFlowNodeHelpers

class CFlowPlayMannequinFragment
    : public CFlowBaseNode<eNCT_Instanced>
{
private:
    enum EInputPorts
    {
        EIP_Play = 0,
        EIP_Fragment,
        EIP_Tags,
        EIP_Prio,
        EIP_Pause,
        EIP_Resume,
        EIP_ForceFinish,
    };

    enum EOutputPorts
    {
        EOP_Success = 0,
        EOP_Fail,
    };

    IActionPtr m_CurrentAction;
public:
    CFlowPlayMannequinFragment(SActivationInfo* pActInfo)
    {
        m_CurrentAction = NULL;
    }

    virtual ~CFlowPlayMannequinFragment()
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowPlayMannequinFragment(pActInfo);
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Play", _HELP("Play the fragment")),
            InputPortConfig<string>("Fragment", _HELP("Fragment name")),
            InputPortConfig<string>("Tags", _HELP("Tags, seperate by + sign")),
            InputPortConfig<int>("Priority", _HELP("Priority, higher number = higher priority")),
            InputPortConfig_Void("Pause", _HELP("Pauses the actionController")),
            InputPortConfig_Void("Resume", _HELP("Resumes this entity's actionController")),
            InputPortConfig_Void("ForceFinishLastQueued", _HELP("Finish the last queued action")),
            {0}
        };

        static const SOutputPortConfig outputs[] = {
            OutputPortConfig_Void     ("Success",  _HELP("Triggers if succeeded to pass the command")),
            OutputPortConfig_Void       ("Failed", _HELP("Triggers if anything went wrong")),
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Play a Mannequin Fragment on a given entity with given Tags");
        config.SetCategory(EFLN_ADVANCED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
            break;
        case eFE_Activate:
        {
            IActionController* actionController = MannequinFlowNodeHelpers::GetEntityActionController(pActInfo->entityId);
            bool bSuccess = false;
            if (IsPortActive(pActInfo, EIP_Play))
            {
                if (actionController)
                {
                    const string fragName = GetPortString(pActInfo, EIP_Fragment);
                    const int fragCRC = CCrc32::ComputeLowercase(fragName);
                    const FragmentID fragID = actionController->GetContext().controllerDef.m_fragmentIDs.Find(fragCRC);

                    string tagName = GetPortString(pActInfo, EIP_Tags);
                    TagState tagState = TAG_STATE_EMPTY;
                    if (!tagName.empty())
                    {
                        const CTagDefinition* tagDefinition = actionController->GetTagDefinition(fragID);
                        tagName.append("+");
                        while (tagName.find("+") != string::npos)
                        {
                            string::size_type pos = tagName.find("+");

                            string rest = tagName.substr(pos + 1, tagName.size());
                            string curTagName = tagName.substr(0, pos);
                            bool found = false;

                            if (tagDefinition)
                            {
                                const int tagCRC = CCrc32::ComputeLowercase(curTagName);
                                const TagID tagID = tagDefinition->Find(tagCRC);
                                found = tagID != TAG_ID_INVALID;
                                tagDefinition->Set(tagState, tagID, true);
                            }

                            if (!found)
                            {
                                const TagID tagID = actionController->GetContext().state.GetDef().Find(curTagName);
                                if (tagID != TAG_ID_INVALID)
                                {
                                    actionController->GetContext().state.Set(tagID, true);
                                }
                                else
                                {
                                    AZ_Warning("PlayMannequinFragment", false, "Failed to locate tag '%s' for assigned entity.", curTagName.c_str());
                                }
                            }

                            tagName = rest;
                        }
                    }

                    const int priority = GetPortInt(pActInfo, EIP_Prio);
                    m_CurrentAction = new TAction<SAnimationContext>(priority, fragID, tagState);
                    actionController->Queue(*m_CurrentAction);
                    bSuccess = true;
                }
            }
            else if (IsPortActive(pActInfo, EIP_Pause))
            {
                if (actionController)
                {
                    actionController->Pause();
                    bSuccess = true;
                }
            }
            else if (IsPortActive(pActInfo, EIP_Resume))
            {
                if (actionController)
                {
                    actionController->Resume();
                    bSuccess = true;
                }
            }
            else if (IsPortActive(pActInfo, EIP_ForceFinish))
            {
                if (m_CurrentAction)
                {
                    m_CurrentAction->ForceFinish();
                    bSuccess = true;
                }
            }
            ActivateOutput(pActInfo, bSuccess ? EOP_Success : EOP_Fail, 1);
        }
        break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
// CFlowEnslaveCharacter
//////////////////////////////////////////////////////////////////////////
class CFlowEnslaveCharacter
    : public CFlowBaseNode<eNCT_Instanced>
{
private:
    enum EInputPorts
    {
        EIP_Enslave = 0,
        EIP_UnEnslave,
        EIP_Slave,
        EIP_ScopeContext,
        EIP_DB,
    };

    enum EOutputPorts
    {
        EOP_Success = 0,
        EOP_Fail,
    };

public:
    CFlowEnslaveCharacter(SActivationInfo* pActInfo)
    {
    }

    virtual ~CFlowEnslaveCharacter()
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowEnslaveCharacter(pActInfo);
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Enslave", _HELP("Enslave the character")),
            InputPortConfig_Void("UnEnslave", _HELP("Remove Enslavement to the character")),
            InputPortConfig<FlowEntityId>("Slave", _HELP("Char to slave")),
            InputPortConfig<string>("ScopeContext", _HELP("Scope Context")),
            InputPortConfig<string>("DB", _HELP("optional DB")),
            {0}
        };

        static const SOutputPortConfig outputs[] = {
            OutputPortConfig_Void     ("Success",  _HELP("Triggers if enslaving succeeded")),
            OutputPortConfig_Void     ("Failed",  _HELP("Triggers if enslaving failed")),
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Enslave one character to another");
        config.SetCategory(EFLN_ADVANCED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
            break;
        case eFE_Activate:

            if (IsPortActive(pActInfo, EIP_Enslave) || IsPortActive(pActInfo, EIP_UnEnslave))
            {
                IActionController* actionController = MannequinFlowNodeHelpers::GetEntityActionController(pActInfo->entityId);

                if (actionController)
                {
                    const FlowEntityId slaveEntityId = FlowEntityId(GetPortEntityId(pActInfo, EIP_Slave));
                    IActionController* slaveActionController = MannequinFlowNodeHelpers::GetEntityActionController(slaveEntityId);

                    if (slaveActionController)
                    {
                        IAnimationDatabaseManager& dbManager = gEnv->pGame->GetIGameFramework()->GetMannequinInterface().GetAnimationDatabaseManager();
                        uint32 db_crc32 = CCrc32::ComputeLowercase(GetPortString(pActInfo, EIP_DB));
                        const IAnimationDatabase* db = dbManager.FindDatabase(db_crc32);

                        const string& scopeContextName = GetPortString(pActInfo, EIP_ScopeContext);
                        const string& requestedScopeContext = scopeContextName.empty() ? "SlaveChar" : scopeContextName;
                        const TagID scopeContext = actionController->GetContext().controllerDef.m_scopeContexts.Find(scopeContextName.c_str());

                        actionController->SetSlaveController(*slaveActionController, scopeContext, IsPortActive(pActInfo, EIP_Enslave) ? true : false, db);
                        ActivateOutput(pActInfo, EOP_Success, 1);
                    }
                    else
                    {
                        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "No GameObject or Animated character found for the slave");
                        ActivateOutput(pActInfo, EOP_Fail, 1);
                    }
                }
                else
                {
                    CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "No GameObject or AnimatedCharacter found");
                    ActivateOutput(pActInfo, EOP_Fail, 1);
                }
            }

            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
// CFlowProcClipEventListener
//////////////////////////////////////////////////////////////////////////
static const char* kFlowGraphEventsProcContextName = "FlowGraphEvents";
class CFlowGraphEventsProceduralContext
    : public IProceduralContext
{
public:
    PROCEDURAL_CONTEXT(CFlowGraphEventsProceduralContext, kFlowGraphEventsProcContextName, 0x334455925E8947FB, 0x8008B3F89CAC0C37);

    struct IProcClipEventListener
    {
        virtual void OnProcClipEvent(const string& eventName) = 0;
    };

private:
    virtual void Update(float timePassed) override {}

public:
    void SendEvent(const string& eventName)
    {
        for (CListenerNotifier<IProcClipEventListener*> notifier(m_listeners); notifier.IsValid(); notifier.Next())
        {
            notifier->OnProcClipEvent(eventName);
        }
    }

    void RegisterListener(IProcClipEventListener* pListener)
    {
        m_listeners.Add(pListener);
    }

    void UnregisterListener(IProcClipEventListener* pListener)
    {
        m_listeners.Remove(pListener);
    }

private:
    CListenerSet<IProcClipEventListener*> m_listeners;
};
CFlowGraphEventsProceduralContext::CFlowGraphEventsProceduralContext()
    : m_listeners(4){}
CFlowGraphEventsProceduralContext::~CFlowGraphEventsProceduralContext(){}
CRYREGISTER_CLASS(CFlowGraphEventsProceduralContext);

struct SProceduralClipFlowGraphEventParams
    : public IProceduralParams
{
    virtual void Serialize(Serialization::IArchive& ar)
    {
        ar(enterEventName, "EnterEventName", "Enter Event Name");
        ar(exitEventName, "ExitEventName", "Exit Event Name");
    }

    TProcClipString enterEventName;
    TProcClipString exitEventName;
};

class CProceduralClipFlowGraphEvent
    : public TProceduralContextualClip<CFlowGraphEventsProceduralContext, SProceduralClipFlowGraphEventParams>
{
    virtual void OnEnter(float blendTime, float duration, const SProceduralClipFlowGraphEventParams& params)
    {
        m_context->SendEvent(GetParams().enterEventName.c_str());
    }

    virtual void OnExit(float blendTime)
    {
        m_context->SendEvent(GetParams().exitEventName.c_str());
    }

    virtual void Update(float timePassed) {}
};
REGISTER_PROCEDURAL_CLIP(CProceduralClipFlowGraphEvent, "FlowGraphEvent");

class CFlowProcClipEventListener
    : public CFlowBaseNode<eNCT_Instanced>
    , public CFlowGraphEventsProceduralContext::IProcClipEventListener
{
public:
    enum EInputPorts
    {
        EIP_Start = 0,
        EIP_Stop,
        EIP_Filter
    };

    enum EOutputPorts
    {
        EOP_Event = 0
    };

    CFlowProcClipEventListener(SActivationInfo* pActInfo)
        : m_registeredEntityId(0)
    {
    }

    virtual ~CFlowProcClipEventListener()
    {
        RegisterListener(m_registeredEntityId, false);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowProcClipEventListener(pActInfo);
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Start", _HELP("Start listening for procedural clip events")),
            InputPortConfig_Void("Stop", _HELP("Stop listening for procedural clip events")),
            InputPortConfig<string>("Filter", _HELP("Filter")),
            {0}
        };

        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<string>("Event", _HELP("Event")),
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Listens for Mannequin procedural clip events");
        config.SetCategory(EFLN_ADVANCED);
    }

    virtual void ProcessEvent(EFlowEvent evt, SActivationInfo* pActInfo)
    {
        if (evt == eFE_Activate)
        {
            if (pActInfo->pEntity == nullptr)
            {
                return;
            }
            else if (IsPortActive(pActInfo, EIP_Start))
            {
                RegisterListener(FlowEntityId(pActInfo->pEntity->GetId()), true);
            }
            else if (IsPortActive(pActInfo, EIP_Stop))
            {
                RegisterListener(FlowEntityId(pActInfo->pEntity->GetId()), false);
            }
        }

        m_actInfo = *pActInfo;
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    void OnProcClipEvent(const string& eventName) override
    {
        if (!eventName.empty())
        {
            const string& filterString = GetPortString(&m_actInfo, EIP_Filter);
            if (filterString.empty() || eventName.find(filterString) != string::npos)
            {
                ActivateOutput(&m_actInfo, EOP_Event, eventName);
            }
        }
    }

private:
    void RegisterListener(const FlowEntityId entityId, const bool enable)
    {
        IGameFramework* const pGameFramework = gEnv->pGame->GetIGameFramework();
        IGameObject* const pGameObject = pGameFramework ? pGameFramework->GetGameObject(entityId) : NULL;
        IAnimatedCharacter* const pAnimChar = pGameObject ? (IAnimatedCharacter*) pGameObject->QueryExtension("AnimatedCharacter") : NULL;
        IActionController* const pActionController = pAnimChar ? pAnimChar->GetActionController() : NULL;
        if (pActionController)
        {
            CFlowGraphEventsProceduralContext* pContext = static_cast<CFlowGraphEventsProceduralContext*>(pActionController->FindOrCreateProceduralContext(kFlowGraphEventsProcContextName));
            CRY_ASSERT(pContext);
            if (enable)
            {
                if (m_registeredEntityId && m_registeredEntityId != entityId)
                {
                    // Unregister from previous context
                    RegisterListener(m_registeredEntityId, false);
                }
                pContext->RegisterListener(this);
                m_registeredEntityId = entityId;
            }
            else
            {
                pContext->UnregisterListener(this);
                m_registeredEntityId = 0;
            }
        }
    }

private:
    SActivationInfo m_actInfo;
    FlowEntityId m_registeredEntityId;
};

//////////////////////////////////////////////////////////////////////////
REGISTER_FLOW_NODE("Actor:PlayMannequinFragment", CFlowPlayMannequinFragment);
REGISTER_FLOW_NODE("Actor:EnslaveCharacter", CFlowEnslaveCharacter);
REGISTER_FLOW_NODE("Actor:ProcClipEventListener", CFlowProcClipEventListener);
