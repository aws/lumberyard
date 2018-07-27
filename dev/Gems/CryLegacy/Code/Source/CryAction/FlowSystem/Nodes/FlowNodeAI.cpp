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

// Description : place for miscellaneous AI related flow graph nodes


#include "CryLegacy_precompiled.h"
#include <IAIAction.h>
#include <IAISystem.h>
#include <IAgent.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "FlowEntityNode.h"
#include "AIProxy.h"

#include <IInterestSystem.h>
#include "INavigation.h"
#include "IAIObjectManager.h"
#include "IAIActor.h"
#include <IFactionMap.h>
#include <ICommunicationManager.h>
#include <LmbrCentral/Ai/NavigationSystemBus.h>
/*
//////////////////////////////////////////////////////////////////////////
// broadcast AI signal
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISignal : public CFlowBaseNode
{
public:
    CFlowNode_AISignal( SActivationInfo * pActInfo )
    {
    }
    virtual void GetConfiguration( SFlowNodeConfig &config )
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<bool>("send"),
            InputPortConfig<string>("signal",_HELP("AI signal to be send") ),
            InputPortConfig<Vec3>("pos" ),
            InputPortConfig<float>("radius" ),
            {0}
        };
        config.sDescription = _HELP( "broadcast AI signal" );
        config.pInputPorts = in_config;
        config.pOutputPorts = 0;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
    {
        if(event != eFE_Activate || !IsBoolPortActive(pActInfo, 0))
            return;
        string signalText = GetPortString(pActInfo,1);

        Vec3    pos = *pActInfo->pInputPorts[2].GetPtr<Vec3>();
        //      Vec3    pos = GetPortVec3(pActInfo,2);
        float   radius = GetPortFloat(pActInfo,3);
        IAISystem *pAISystem(gEnv->pAISystem);

        pAISystem->SendAnonymousSignal(1, signalText, pos, radius, NULL);
    }
private:
};
*/


//////////////////////////////////////////////////////////////////////////
// Counts how many AIs are active
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIActiveCount
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum EOutputPorts
    {
        eOUT_AICount = 0,
        eOUT_EnemyCount
    };
public:
    CFlowNode_AIActiveCount(SActivationInfo* pActInfo)
        : m_localAICount(0)
        , m_localAIEnemyCount(0)
    {
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_AIActiveCount(pActInfo); }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        ser.Value("LocalAICount", m_localAICount);
        ser.Value("LocalAIEnemyCount", m_localAIEnemyCount);

        if (ser.IsReading())
        {
            m_currentAICount = -1;
            m_currentAIEnemyCount = -1;
            m_lastUpdateframeID = -1;
        }
    }
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<int>("Total"),
            OutputPortConfig<int>("Enemy"),
            {0}
        };

        config.sDescription = _HELP("Counts how many AIs are active");
        config.pInputPorts = 0;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Initialize)
        {
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            m_localAICount = m_localAIEnemyCount = -1; // to force a local update
            m_currentAICount = m_currentAIEnemyCount = -1;
            m_lastUpdateframeID = -1;
        }

        if (event != eFE_Update)
        {
            return;
        }

        float currTime = gEnv->pTimer->GetCurrTime();

        RecalcAICounters();

        if (m_localAICount != m_currentAICount)
        {
            m_localAICount = m_currentAICount;
            ActivateOutput(pActInfo, eOUT_AICount, m_localAICount);
        }
        if (m_localAIEnemyCount != m_currentAIEnemyCount)
        {
            m_localAIEnemyCount = m_currentAIEnemyCount;
            ActivateOutput(pActInfo, eOUT_EnemyCount, m_localAIEnemyCount);
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
private:

    static void RecalcAICounters()
    {
        int frameID = gEnv->pRenderer->GetFrameID();

        if (frameID != m_lastUpdateframeID)
        {
            m_lastUpdateframeID = frameID;

            bool changed = false;
            int AICount   = 0;
            int AIEnemies = 0;

            const IActor*     pLocalPlayer    = CCryAction::GetCryAction()->GetClientActor();
            const IAIObject*  pAILocalPlayer  = pLocalPlayer ? pLocalPlayer->GetEntity()->GetAI() : NULL;

            IActorIteratorPtr pIter = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
            while (IActor* pActor = pIter->Next())
            {
                if (!pActor->IsPlayer())
                {
                    const IAIObject* pAIObject = pActor->GetEntity()->GetAI();
                    if (pAIObject)
                    {
                        const IAIActor* pAIActor = pAIObject->CastToIAIActor();
                        if (pAIActor->IsActive() && (pActor->GetHealth() > 0.0f))
                        {
                            AICount++;
                            if (pAIObject->IsHostile(pAILocalPlayer))
                            {
                                AIEnemies++;
                            }
                        }
                    }
                }
            }

            m_currentAICount = AICount;
            m_currentAIEnemyCount = AIEnemies;
        }
    }

private:
    // using statics instead a singleton node because we want the outputs to be activated only on change.
    int m_localAICount;
    int m_localAIEnemyCount;
    static int m_currentAICount;
    static int m_currentAIEnemyCount;
    static int m_lastUpdateframeID;
};

int CFlowNode_AIActiveCount::m_currentAICount = -1;
int CFlowNode_AIActiveCount::m_currentAIEnemyCount = -1;
int CFlowNode_AIActiveCount::m_lastUpdateframeID = -1;


//////////////////////////////////////////////////////////////////////////
// Node to count the number of currently active AIs in a faction.
//////////////////////////////////////////////////////////////////////////

class CFlowNode_AIActiveCountInFaction
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    enum
    {
        IN_Faction,
        IN_IncludeHumanPlayers
    };

    enum
    {
        OUT_Count,
        OUT_CountChanged
    };

    CFlowNode_AIActiveCountInFaction(SActivationInfo*)
        : m_factionID(0)
        , m_includeHumanPlayers(false)
        , m_currentCount(0)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_AIActiveCountInFaction(pActInfo);
    }

    virtual void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig<string>("Faction", "", _HELP("Faction to the count the active AIs"), 0, "enum_global:Faction"),
            InputPortConfig<bool>("IncludeHumanPlayers", false, _HELP("Include human players when counting active AIs in the specified faction")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<int>("Count", _HELP("The number of active AIs in the specified faction")),
            OutputPortConfig_Void("CountChanged", _HELP("Triggered whenever the number of active AIs in the specified faction changes"), "Changed"),
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.sDescription = _HELP("Counts the number of currently active AIs in a specific faction.");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    virtual void Serialize(SActivationInfo*, TSerialize ser)
    {
        ser.Value("m_factionID", m_factionID);
        ser.Value("m_includeHumanPlayers", m_includeHumanPlayers);
        ser.Value("m_currentCount", m_currentCount);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            m_currentCount = 0;
            m_factionID = gEnv->pAISystem->GetFactionMap().GetFactionID(GetPortString(pActInfo, IN_Faction).c_str());
            m_includeHumanPlayers = GetPortBool(pActInfo, IN_IncludeHumanPlayers);
            break;

        case eFE_Update:
        {
            int count = CountCurrentlyActiveAIsInSpecifiedFaction();
            ActivateOutput(pActInfo, OUT_Count, count);
            if (count != m_currentCount)
            {
                ActivateOutput(pActInfo, OUT_CountChanged, true);
            }
            m_currentCount = count;
        }
        break;
        }
    }

private:
    uint8 m_factionID;
    bool m_includeHumanPlayers;  // also include human players when counting AIs of the specified faction
    int m_currentCount;          // no. of currently active AIs in the specified faction

    int CountCurrentlyActiveAIsInSpecifiedFaction()
    {
        int count = 0;
        IActorIteratorPtr pIter = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
        while (IActor* pActor = pIter->Next())
        {
            if (!pActor->IsPlayer() || m_includeHumanPlayers)
            {
                if (IAIObject* pAIObject = pActor->GetEntity()->GetAI())
                {
                    if (IAIActor* pAIActor = pAIObject->CastToIAIActor())
                    {
                        if (pAIActor->IsActive() && pActor->GetHealth() > 0.0f && pAIObject->GetFactionID() == m_factionID)
                        {
                            count++;
                        }
                    }
                }
            }
        }
        return count;
    }
};


//////////////////////////////////////////////////////////////////////////
// Counts how many AIs are active
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIActiveCountMonitor
    : public CFlowBaseNode<eNCT_Instanced>
{
private:
    enum EInputs
    {
        eInput_StartMonitoring,
        eInput_StopMonitoring,
        eInput_MaxActiveAICount,
        eInput_Loop,
        eInput_LoopPeriod
    };

    enum EOutputs
    {
        eOutput_SlotsFree,
        eOutput_SlotsFull,
        eOutput_CurrentCount,
    };

    struct SActiveAIStatistics
    {
        SActiveAIStatistics()
            : m_lastUpdateFrameId(-1)
            , m_currentActiveAIs(0)
        {
        }

        void Reset()
        {
            m_lastUpdateFrameId = -1;
            m_currentActiveAIs = 0;
        }

        void Refresh()
        {
            const int frameID = gEnv->pRenderer->GetFrameID();

            //Check if some other flow node instance updated this frame to avoid multiple updates
            if (frameID != m_lastUpdateFrameId)
            {
                m_lastUpdateFrameId = frameID;

                int aiCount = 0;

                IActorIteratorPtr pIter = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
                while (IActor* pActor = pIter->Next())
                {
                    if (!pActor->IsPlayer() && !pActor->IsDead())
                    {
                        const IAIObject* pAIObject = pActor->GetEntity()->GetAI();
                        if (pAIObject && pAIObject->CastToIAIActor()->IsActive())
                        {
                            aiCount++;
                        }
                    }
                }

                m_currentActiveAIs = aiCount;
            }
        }

        ILINE int GetActiveAICount() const { return m_currentActiveAIs; }

    private:
        int m_lastUpdateFrameId;
        int m_currentActiveAIs;
    };

public:

    CFlowNode_AIActiveCountMonitor(SActivationInfo* pActInfo)
        : m_active(false)
        , m_monitorRefreshTimeout(2.0f)
    {
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_AIActiveCountMonitor(pActInfo); }

    //////////////////////////////////////////////////////////////////////////
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void("Start", _HELP("Starts monitoring, looping periodically if configured so")),
            InputPortConfig_Void("Stop",  _HELP("Stops monitoring loop")),
            InputPortConfig<int>("MaxActiveAIs", 0, _HELP("Maximum number of active AI which is being used as condition for the node outputs")),
            InputPortConfig<bool>("Loop", false, _HELP("Activates monitoring loop mode")),
            InputPortConfig<float>("LoopPeriod", 2.0f, _HELP("Period of time between check when loop mode is active")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_Void("SlotsFree", _HELP("Activated when the number of active AI goes bellow MaxActiveAI")),
            OutputPortConfig_Void("SlotsFull", _HELP("Activated when the number of active AI goes is equal or above MaxActiveAI")),
            OutputPortConfig<int>("CurrentActiveAIs", _HELP("Outputs number of active AIs")),
            {0}
        };

        config.sDescription = _HELP("Monitors active AI count against some limit, and outputs periodically the current state. When the condition is met, the monitor loop will stop automatically and it needs to be started manually again if desired.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    //////////////////////////////////////////////////////////////////////////
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Update:
        {
            CRY_ASSERT(m_active);

            const float frameTime = gEnv->pTimer->GetFrameTime();
            const float timeOut = m_monitorRefreshTimeout - frameTime;

            if (timeOut > 0.0f)
            {
                //No time for check yet, continue running time-out
                m_monitorRefreshTimeout = timeOut;
            }
            else
            {
                //Refresh statistics, and trigger outputs
                s_activeAIStatistics.Refresh();

                const int activeAICount = s_activeAIStatistics.GetActiveAICount();

                if (activeAICount < GetPortInt(pActInfo, eInput_MaxActiveAICount))
                {
                    //Trigger output and deactivate updates at this point, we are done until we are asked to start again
                    ActivateOutput(pActInfo, eOutput_SlotsFree, true);
                    DeactivateUpdate(pActInfo);
                }
                else
                {
                    //Trigger output and check if we need to loop or just stop
                    ActivateOutput(pActInfo, eOutput_SlotsFull, true);

                    const bool loopingMode = GetPortBool(pActInfo, eInput_Loop);
                    if (loopingMode)
                    {
                        m_monitorRefreshTimeout = GetPortFloat(pActInfo, eInput_LoopPeriod);
                    }
                    else
                    {
                        DeactivateUpdate(pActInfo);
                    }
                }

                ActivateOutput(pActInfo, eOutput_CurrentCount, activeAICount);
            }
        }
        break;

        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, eInput_StartMonitoring))
            {
                ActivateUpdate(pActInfo);
            }
            else if (IsPortActive(pActInfo, eInput_StopMonitoring))
            {
                DeactivateUpdate(pActInfo);
            }
        }
        break;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        ser.Value("MonitoringActive", m_active);
        ser.Value("MonitoringTimeout", m_monitorRefreshTimeout);

        if (ser.IsReading())
        {
            s_activeAIStatistics.Reset();

            //Make sure active state is restored properly
            if (m_active)
            {
                const float storedRefreshTime = m_monitorRefreshTimeout;
                ActivateUpdate(pActInfo);

                m_monitorRefreshTimeout = storedRefreshTime;
            }
            else
            {
                DeactivateUpdate(pActInfo);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

private:

    void ActivateUpdate(SActivationInfo* pActInfo)
    {
        pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
        m_active = true;

        m_monitorRefreshTimeout = 0.0f; //This is so first update is immediate after activation
    }

    void DeactivateUpdate(SActivationInfo* pActInfo)
    {
        pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
        m_active = false;
    }

    //Shared between all node instances
    static SActiveAIStatistics s_activeAIStatistics;

    float   m_monitorRefreshTimeout;
    bool    m_active;
};

CFlowNode_AIActiveCountMonitor::SActiveAIStatistics CFlowNode_AIActiveCountMonitor::s_activeAIStatistics;

//////////////////////////////////////////////////////////////////////////
// Filters the inputs depending on owner's AI alertness level.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIAlertnessFilter
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_AIAlertnessFilter(SActivationInfo* pActInfo) {}
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<int>("Threshold", 1, _HELP("The alertness level needed to output the values on the Input port to the High port.\nIf current alertness is less than this then the values are sent to the Low output port.")),
            InputPortConfig_AnyType("Input", _HELP("The group ID of the group to get the alertness from")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig_AnyType("Low", _HELP("Outputs here all values passed on Input if current alertness is less than threshold value")),
            OutputPortConfig_AnyType("High", _HELP("Outputs here all values passed on Input if current alertness is equal or greater than threshold value")),
            {0}
        };
        config.sDescription = _HELP("Filters the inputs depending on owner's AI alertness level.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.SetCategory(EFLN_ADVANCED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (!pActInfo->pEntity)
        {
            return;
        }

        if (event != eFE_Initialize && event != eFE_Activate)
        {
            return;
        }

        if (!IsPortActive(pActInfo, 1))
        {
            return;
        }

        EntityId entityID = pActInfo->pEntity->GetId();
        IActorSystem* pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
        IActor* pActor = pActorSystem->GetActor(entityID);
        if (!pActor)
        {
            return;
        }

        // read the input value...
        // ...and depending on threshold...
        /*int currentAlertness = pOldActor->GetCachedAIValues()->GetAlertnessState();
        // ... output it to one of two possible output ports

        int outValue = ( currentAlertness < GetPortInt(pActInfo,0) ) ? 0 : 1;
        ActivateOutput( pActInfo, outValue, GetPortAny(pActInfo, 1) );*/
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
// Group AI enemies alertness level.
// It represents the current alertness level of AI enemies, more precisely,
// the highest level of alertness of any active agent in a group.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIGroupAlertness
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    CFlowNode_AIGroupAlertness(SActivationInfo* pActInfo)
        : m_CurrentAlertness(0)
        , m_PlayerWasRecognized(false)
        , m_PendingNotifyOfInitialAlertnessState(true)
    {
    }
    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_AIGroupAlertness(pActInfo);
    }
    void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        ser.Value("CurrentAlertness", m_CurrentAlertness);
        ser.Value("PlayerWasRecognized", m_PlayerWasRecognized);
        ser.Value("PendingNotifyOfInitialAlertnessState", m_PendingNotifyOfInitialAlertnessState);
    }
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<int>("alertness", "0 - green\n1 - orange\n2 - red"),
            OutputPortConfig_Void("green"),
            OutputPortConfig_Void("orange"),
            OutputPortConfig_Void("red"),
            OutputPortConfig_Void("playerSighted"),
            {0}
        };

        static const SInputPortConfig in_config[] = {
            InputPortConfig<int>("groupID", _HELP("The group ID of the group to get the alertness from")),
            {0}
        };

        config.sDescription = _HELP("The highest level of alertness of any agent in the group.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Initialize)
        {
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            m_CurrentAlertness = 0;
            m_PlayerWasRecognized = false;
            m_PendingNotifyOfInitialAlertnessState = true;
        }
        else if (event == eFE_Update)
        {
            bool playerIsVisible = false;
            // Get the max alertness of the group.
            int maxAlertness = 0;
            IAISystem* pAISystem = gEnv->pAISystem;
            int groupID = GetPortInt(pActInfo, 0);      // group ID
            int n = pAISystem->GetGroupCount(groupID, IAISystem::GROUP_ENABLED);
            for (int i = 0; i < n; i++)
            {
                IAIObject*  pMember = pAISystem->GetGroupMember(groupID, i, IAISystem::GROUP_ENABLED);
                if (pMember)
                {
                    IAIActorProxy* pMemberProxy = pMember->GetProxy();
                    if (pMemberProxy)
                    {
                        maxAlertness = max(maxAlertness, pMemberProxy->GetAlertnessState());
                    }

                    if (!m_PlayerWasRecognized && !playerIsVisible)
                    {
                        if (IAIActor* aiActor = pMember->CastToIAIActor())
                        {
                            if (aiActor->GetAttentionTargetType() == AITARGET_VISUAL)
                            {
                                if (IAIObject* attentionTargetAIObject = aiActor->GetAttentionTarget())
                                {
                                    const EntityId localPlayerEntityId = CCryAction::GetCryAction()->GetClientActorId();
                                    const EntityId attentionTargetEntityId = attentionTargetAIObject->GetEntityID();

                                    playerIsVisible = (attentionTargetEntityId != 0 && localPlayerEntityId == attentionTargetEntityId);
                                }
                            }
                        }
                    }
                }
            }

            if (maxAlertness != m_CurrentAlertness || m_PendingNotifyOfInitialAlertnessState)
            {
                m_CurrentAlertness = maxAlertness;
                ActivateOutput(pActInfo, 0, m_CurrentAlertness);
                ActivateOutput(pActInfo, m_CurrentAlertness + 1, true);
                m_PendingNotifyOfInitialAlertnessState = false;
            }

            if (!m_PlayerWasRecognized)
            {
                const bool playerIsRecognized = (playerIsVisible && m_CurrentAlertness == 2);
                if (playerIsRecognized)
                {
                    m_PlayerWasRecognized = true;
                    const int targetVisibleOutputPort = 4;
                    ActivateOutput(pActInfo, targetVisibleOutputPort, true);
                }
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
private:
    int m_CurrentAlertness;
    bool m_PlayerWasRecognized;
    bool m_PendingNotifyOfInitialAlertnessState;
};

//////////////////////////////////////////////////////////////////////////
// AI:AIMergeGroups
// Merges two AI groups.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIMergeGroups
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_AIMergeGroups(SActivationInfo* pActInfo) {}
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_AnyType("sink", NULL, _HELP("Sync")),
            InputPortConfig<int>("fromID", 0, _HELP("The group which is to be merged.")),
            InputPortConfig<int>("toID", 0, _HELP("The group to merge to.")),
            InputPortConfig<bool>("enableFromGroup", true, _HELP("Set if the members of the 'from' group should be enabled before merging.")),
            {0}
        };

        config.sDescription = _HELP("Merges AI group 'from' to group 'to'.");
        config.pInputPorts = in_config;
        config.pOutputPorts = 0;
        config.SetCategory(EFLN_ADVANCED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, 0))
        {
            int fromID = GetPortInt(pActInfo, 1);
            int toID = GetPortInt(pActInfo, 2);
            bool enable = GetPortBool(pActInfo, 3);

            // Collect members
            std::vector<IAIObject*> members;
            AutoAIObjectIter it(gEnv->pAISystem->GetAIObjectManager()->GetFirstAIObject(OBJFILTER_GROUP, fromID));
            for (; it->GetObject(); it->Next())
            {
                members.push_back(it->GetObject());
            }

            // Enable entities if they are not currently enabled.
            if (enable)
            {
                for (unsigned i = 0, ni = members.size(); i < ni; ++i)
                {
                    IEntity* pEntity = members[i]->GetEntity();
                    if (pEntity)
                    {
                        IComponentScriptPtr scriptComponent = pEntity->GetComponent<IComponentScript>();
                        if (scriptComponent)
                        {
                            scriptComponent->CallEvent("Enable");
                        }
                    }
                }
            }

            // Change group IDs
            for (unsigned i = 0, ni = members.size(); i < ni; ++i)
            {
                members[i]->SetGroupId(toID);
            }

            // Send signals to notify about the change.
            for (unsigned i = 0, ni = members.size(); i < ni; ++i)
            {
                IAIActor* pAIActor = members[i]->CastToIAIActor();
                if (pAIActor)
                {
                    pAIActor->SetSignal(10, "OnGroupChanged", members[i]->GetEntity(), 0);
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
// AI:GroupBeacon node.
// This node returns the position of the group beacon.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIGroupBeacon
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    CFlowNode_AIGroupBeacon(SActivationInfo* pActInfo)
        : m_lastVal(ZERO)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_AIGroupBeacon(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser)
    {
        // forces re-triggering on load
        if (ser.IsReading())
        {
            m_lastVal.zero();
        }
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<Vec3>("pos", _HELP("Beacon position")),
            {0}
        };
        static const SInputPortConfig in_config[] = {
            InputPortConfig<int>("groupID", _HELP("The group ID of the group to get the beacon from")),
            {0}
        };
        config.sDescription = _HELP("Outputs AI Group's beacon position");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Initialize)
        {
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            m_lastVal.zero();
            ActivateOutput(pActInfo, 0, m_lastVal);
        }
        else if (event == eFE_Update)
        {
            IAISystem*  pAISystem = gEnv->pAISystem;
            int groupID = GetPortInt(pActInfo, 0);

            IAIObject* pBeacon = pAISystem->GetBeacon(groupID);
            Vec3    beacon(ZERO);
            if (pBeacon)
            {
                beacon = pBeacon->GetPos();
            }
            if (beacon != m_lastVal)
            {
                ActivateOutput(pActInfo, 0, beacon);
                m_lastVal = beacon;
            }
        }
    }
    Vec3    m_lastVal;

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
// AI:GroupID node.
// Returns the groupID of the entity.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIGroupID
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    CFlowNode_AIGroupID(SActivationInfo* pActInfo)
        : m_lastID(-1)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_AIGroupID(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser)
    {
        // forces re-triggering on load
        if (ser.IsReading())
        {
            m_lastID = -1;
        }
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<int>("groupID", _HELP("AI Group ID of the entity")),
            {0}
        };
        static const SInputPortConfig in_config[] = {
            InputPortConfig_AnyType("sink", NULL, _HELP("Sync")),
            InputPortConfig<int>("groupID", _HELP("New group ID to set to the entity.")),
            {0}
        };

        config.sDescription = _HELP("Sets and outputs AI's group ID");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Initialize)
        {
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
        }
        else if (event == eFE_SetEntityId || (event == eFE_Activate && IsPortActive(pActInfo, 0)))
        {
            IEntity* pEntity = pActInfo->pEntity;
            if (pEntity)
            {
                IAIActor* pAIActor = CastToIAIActorSafe(pEntity->GetAI());
                if (pAIActor)
                {
                    if (event == eFE_Activate)
                    {
                        // set group of ai object
                        AgentParameters ap = pAIActor->GetParameters();
                        ap.m_nGroup = GetPortInt(pActInfo, 1);
                        pAIActor->SetParameters(ap);
                    }
                    m_lastID = pAIActor->GetParameters().m_nGroup;
                    ActivateOutput(pActInfo, 0, pAIActor->GetParameters().m_nGroup);
                }
            }
        }
        else if (event == eFE_Update)
        {
            // Update the output when it changes.
            IEntity* pEntity = pActInfo->pEntity;
            if (pEntity)
            {
                IAIActor* pAIActor = CastToIAIActorSafe(pEntity->GetAI());
                if (pAIActor)
                {
                    int id = pAIActor->GetParameters().m_nGroup;
                    if (m_lastID != id)
                    {
                        m_lastID = id;
                        ActivateOutput(pActInfo, 0, id);
                    }
                }
            }
        }
    }
    int m_lastID;

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
// AI Group count.
// Returns the number of entities in a group.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIGroupCount
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    CFlowNode_AIGroupCount(SActivationInfo* pActInfo)
        : m_CurrentCount(-1)
        , m_decreasing(false)
    {
    }
    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_AIGroupCount(pActInfo);
    }
    void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        ser.Value("CurrentCount", m_CurrentCount);
    }
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<int>("count", _HELP("The number of agents in a group.")),
            OutputPortConfig_Void("empty", _HELP("Triggered when the whole group is wasted.")),
            {0}
        };

        static const SInputPortConfig in_config[] = {
            InputPortConfig<int>("groupID", _HELP("The group ID of the group to get the alertness from")),
            {0}
        };

        config.sDescription = _HELP("The highest level of alertness of any agent in the group.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Initialize)
        {
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            IAISystem* pAISystem = gEnv->pAISystem;
            int groupID = GetPortInt(pActInfo, 0);          // group ID
            int n = pAISystem->GetGroupCount(groupID, true);

            m_CurrentCount = n;
            ActivateOutput(pActInfo, 0, m_CurrentCount);
            m_decreasing = false;
        }
        else if (event == eFE_Update)
        {
            IAISystem* pAISystem = gEnv->pAISystem;
            int groupID = GetPortInt(pActInfo, 0);          // group ID
            int n = pAISystem->GetGroupCount(groupID, true);

            // Get the max alertness of the group.
            if (n != m_CurrentCount)
            {
                m_decreasing = n < m_CurrentCount;

                // If the group count has changed, update it.
                m_CurrentCount = n;
                ActivateOutput(pActInfo, 0, m_CurrentCount);
                // If the current count is zero activate the empty output.
                if (m_CurrentCount == 0 && m_decreasing)
                {
                    ActivateOutput(pActInfo, 1, true);
                }
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

private:
    int m_CurrentCount;
    bool    m_decreasing;
};

//////////////////////////////////////////////////////////////////////////
// AI:ActionStart node.
// This should be used as start node in AI Action graphs.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIActionStart
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_AIActionStart(SActivationInfo* pActInfo)
    {
    }
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void("restart", "Restarts execution of the whole AI Action from the beginning"),
            InputPortConfig_Void("cancel", "Cancels execution"),
            InputPortConfig_Void("suspend", "Suspends execution"),
            InputPortConfig_Void("resume", "Resumes execution if it was suspended"),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig< FlowEntityId >("UserId", "Entity ID of the agent executing AI Action"),
            OutputPortConfig< FlowEntityId >("ObjectId", "Entity ID of the object on which the agent is executing AI Action"),
            OutputPortConfig< Vec3 >("Position", "World position of smart object action."),
            {0}
        };
        config.pInputPorts = 0; // in_config;
        config.pOutputPorts = out_config;
        config.sDescription = _HELP("Represents the User and the used Object of an AI Action. Use this node to start the AI Action graph.");
        config.nFlags |= EFLN_AIACTION_START;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        IAIAction* pAIAction = pActInfo->pGraph->GetAIAction();
        switch (event)
        {
        case eFE_Update:
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            if (pAIAction)
            {
                IEntity* pUser = pAIAction->GetUserEntity();

                if (pUser && pUser->HasAI())
                {
                    // Field animFinalLocation always stayed ZERO, so I removed it. [5/21/2010 evgeny]
                    //IAIActor *pAIActor = pUser->GetAI()->CastToIAIActor();
                    //Vec3 vSOPosition = pAIActor->GetState()->actorTargetReq.animFinalLocation;
                    //ActivateOutput( pActInfo, 2, vSOPosition);
                    ActivateOutput(pActInfo, 2, Vec3_Zero);
                }

                ActivateOutput(pActInfo, 0, pUser ? pUser->GetId() : 0);
                IEntity* pTarget = pAIAction->GetObjectEntity();
                ActivateOutput(pActInfo, 1, pTarget ? pTarget->GetId() : 0);
            }
            break;
        case eFE_Initialize:
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            break;
        case eFE_Activate:
            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
// AI:ActionEnd node.
// This should be used as end node in AI Action graphs.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIActionEnd
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_AIActionEnd(SActivationInfo* pActInfo)
    {
    }
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void("end", _HELP("ends execution of AI Action reporting it as succeeded")),
            InputPortConfig_Void("cancel", _HELP("ends execution of AI Action reporting it as failed")),
            {0}
        };
        config.pInputPorts = in_config;
        config.pOutputPorts = NULL;
        config.sDescription = _HELP("Use this node to end the AI Action graph.");
        config.nFlags |= EFLN_AIACTION_END;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        IAIAction* pAIAction = pActInfo->pGraph->GetAIAction();
        if (!pAIAction)
        {
            return;
        }
        if (event != eFE_Activate)
        {
            return;
        }

        if (IsPortActive(pActInfo, 0))
        {
            pAIAction->EndAction();
        }
        else if (IsPortActive(pActInfo, 1))
        {
            pAIAction->CancelAction();
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
// AI:ActionAbort node.
// This node serves as a destructor in AI Action graphs.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIActionAbort
    : public CFlowBaseNode<eNCT_Instanced>
{
protected:
    bool bAborted;

public:
    CFlowNode_AIActionAbort(SActivationInfo* pActInfo)
    {
        bAborted = false;
    }
    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_AIActionAbort(pActInfo);
    }
    void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        ser.Value("Aborted", bAborted);
    }
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void("Abort", "Aborts execution of AI Action"),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig< FlowEntityId >("UserId", "Entity ID of the agent executing AI Action (activated on abort)"),
            OutputPortConfig< FlowEntityId >("ObjectId", "Entity ID of the object on which the agent is executing AI Action (activated on abort)"),
            {0}
        };
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.sDescription = _HELP("Use this node to define clean-up procedure executed when AI Action is aborted.");
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        IAIAction* pAIAction = pActInfo->pGraph->GetAIAction();
        switch (event)
        {
        case eFE_Update:
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            if (pAIAction && !bAborted)
            {
                // abort only once
                bAborted = true;

                IEntity* pUser = pAIAction->GetUserEntity();
                ActivateOutput(pActInfo, 0, pUser ? pUser->GetId() : 0);
                IEntity* pTarget = pAIAction->GetObjectEntity();
                ActivateOutput(pActInfo, 1, pTarget ? pTarget->GetId() : 0);
            }
            break;
        case eFE_Initialize:
            bAborted = false;
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            break;
        case eFE_Activate:
            if (!bAborted && pAIAction && IsPortActive(pActInfo, 0))
            {
                pAIAction->AbortAction();
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
// AI:AttTarget node.
// Returns the position of the attention target.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIAttTarget
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_AIAttTarget(SActivationInfo* pActInfo)
    {
    }

    enum INPUTS
    {
        eIP_Active = 0,
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<bool>("Active", true, _HELP("Activate/deactivate the node")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<Vec3>("pos", _HELP("Attention target position")),
            OutputPortConfig<FlowEntityId>("entityId", _HELP("Entity id of attention target")),
            OutputPortConfig_Void("none", _HELP("Activated when there's no attention target")),
            {0}
        };
        config.sDescription = _HELP("Outputs AI's attention target");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            bool needUpdate = (IsPortActive(pActInfo, -1) && GetPortBool(pActInfo, eIP_Active));
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, needUpdate);
            break;
        }

        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, eIP_Active))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, GetPortBool(pActInfo, eIP_Active));
            }
            break;
        }

        case eFE_Update:
        {
            IEntity* pEntity = pActInfo->pEntity;
            if (pEntity)
            {
                IAIActor* pAIActor = CastToIAIActorSafe(pEntity->GetAI());
                if (pAIActor)
                {
                    IAIObject* pTarget = pAIActor->GetAttentionTarget();
                    if (pTarget)
                    {
                        ActivateOutput(pActInfo, 0, pTarget->GetPos());

                        if (pTarget->CastToIPipeUser() || pTarget->CastToCAIPlayer())
                        {
                            pEntity = pTarget->GetEntity();
                            if (pEntity)
                            {
                                ActivateOutput(pActInfo, 1, pEntity->GetId());
                            }
                            else
                            {
                                ActivateOutput(pActInfo, 1, 0);
                            }
                        }
                        else
                        {
                            ActivateOutput(pActInfo, 1, 0);
                        }
                    }
                    else
                    {
                        ActivateOutput(pActInfo, 1, 0);
                        ActivateOutput(pActInfo, 2, true);
                    }
                }
            }
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
// AI:SmartObjectHelper node.
// This node gives position and orientation of any Smart Object Helper
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISOHelper
    : public CFlowEntityNodeBase
{
public:
    CFlowNode_AISOHelper(SActivationInfo* pActInfo)
        : CFlowEntityNodeBase()
    {
        m_event = ENTITY_EVENT_XFORM;
        m_nodeID = pActInfo->myID;
        m_pGraph = pActInfo->pGraph;
        m_entityId = (EntityId)(UINT_PTR)pActInfo->m_pUserData;
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { pActInfo->m_pUserData = (void*)(UINT_PTR)m_entityId; return new CFlowNode_AISOHelper(pActInfo); };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<string>("soclass_class", _HELP("Smart Object Class for which the helper should be defined")),
            InputPortConfig<string>("sonavhelper_helper", _HELP("Helper name")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<Vec3>("pos", _HELP("Position of the Smart Object Helper in world coordinates")),
            OutputPortConfig<Vec3>("fwd", _HELP("Forward direction of the Smart Object Helper in world coordinates")),
            OutputPortConfig<Vec3>("up", _HELP("Up direction of the Smart Object Helper in world coordinates")),
            {0}
        };
        config.sDescription = _HELP("Outputs AI's attention target parameters");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.SetCategory(EFLN_APPROVED);
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        CFlowEntityNodeBase::ProcessEvent(event, pActInfo);

        /*if ( event == eFE_Edited )
        {
            // sent from the editor when an input port is edited
            string className = GetPortString( pActInfo, 0 );
            string helperName = GetPortString( pActInfo, 1 );
            int pos = helperName.find(':');
            if ( pos <= 0 || className != helperName.substr(0,pos) )
            {
            }
            return;
        }*/

        if (event != eFE_SetEntityId)
        {
            return;
        }

        Compute(pActInfo->pEntity);
    }

    void Compute(IEntity* pEntity)
    {
        if (!pEntity)
        {
            return;
        }

        const string* pClassName = m_pGraph->GetInputValue(m_nodeID, 1)->GetPtr<string>();   // GetPortString( pActInfo, 0 );
        const string* pHelperName = m_pGraph->GetInputValue(m_nodeID, 2)->GetPtr<string>();   // GetPortString( pActInfo, 1 );
        if (!pClassName || !pHelperName)
        {
            return;
        }

        string helperName = *pHelperName;
        string className = *pClassName;

        helperName.erase(0, helperName.find(':') + 1);
        if (className.empty() || helperName.empty())
        {
            return;
        }

        SmartObjectHelper* pHelper = gEnv->pAISystem->GetSmartObjectManager()->GetSmartObjectHelper(className, helperName);
        if (!pHelper)
        {
            return;
        }

        // calculate position and output it
        Vec3 pos = pEntity->GetWorldTM().TransformPoint(pHelper->qt.t);
        //ActivateOutput( pActInfo, 0, pos );
        m_pGraph->ActivatePort(SFlowAddress(m_nodeID, 0, true), pos);

        // calculate forward direction and output it
        pos = pHelper->qt.q * FORWARD_DIRECTION;
        pos = pEntity->GetWorldTM().TransformVector(pos);
        pos.NormalizeSafe(FORWARD_DIRECTION);
        //ActivateOutput( pActInfo, 1, pos );
        m_pGraph->ActivatePort(SFlowAddress(m_nodeID, 1, true), pos);

        // calculate up direction and output it
        pos = pHelper->qt.q * Vec3(0, 0, 1);
        pos = pEntity->GetWorldTM().TransformVector(pos);
        pos.NormalizeSafe(Vec3(0, 0, 1));
        //ActivateOutput( pActInfo, 2, pos );
        m_pGraph->ActivatePort(SFlowAddress(m_nodeID, 2, true), pos);
    }


    //////////////////////////////////////////////////////////////////////////
    // IEntityEventListener
    virtual void OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
    {
        if (!m_pGraph->IsEnabled() || m_pGraph->IsSuspended() || !m_pGraph->IsActive())
        {
            return;
        }

        if (event.event == ENTITY_EVENT_XFORM)
        {
            Compute(pEntity);
        }
    };
    //////////////////////////////////////////////////////////////////////////

protected:
    IFlowGraph* m_pGraph;
    TFlowNodeId m_nodeID;
};


//////////////////////////////////////////////////////////////////////////
// AI:SmartObjectEvent node.
// Triggers a smart object event.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISmartObjectEvent
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_AISmartObjectEvent(SActivationInfo* pActInfo) {}

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<string>("soevent_event", _HELP("smart object event to be triggered")),
            InputPortConfig_Void("trigger", _HELP("triggers the smart object event")),
            InputPortConfig<FlowEntityId>("userId", _HELP("if used will limit the event on only one user")),
            InputPortConfig<FlowEntityId>("objectId", _HELP("if used will limit the event on only one object")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<FlowEntityId>("userId", _HELP("the used selected by smart object rules")),
            OutputPortConfig<FlowEntityId>("objectId", _HELP("the object selected by smart object rules")),
            OutputPortConfig_Void("ruleName", _HELP("activated if a matching rule was found"), _HELP("start")),
            OutputPortConfig_Void("noRule", _HELP("activated if no matching rule was found")),
            //  OutputPortConfig_Void( "Done", _HELP("activated after the executed action is finished") ),
            //  OutputPortConfig_Void( "Succeed", _HELP("activated after the executed action is successfully finished") ),
            //  OutputPortConfig_Void( "Fail", _HELP("activated if the executed action was failed") ),
            {0}
        };
        config.sDescription = _HELP("Triggers a smart object event");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
            break;

        case eFE_Activate:
            if (IsPortActive(pActInfo, 1))
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            }
            break;

        case eFE_Update:
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);

            FlowEntityId userId = FlowEntityId(GetPortEntityId(pActInfo, 2));
            FlowEntityId objectId = FlowEntityId(GetPortEntityId(pActInfo, 3));

            IEntity* pUser = userId ? gEnv->pEntitySystem->GetEntity(userId) : NULL;
            IEntity* pObject = objectId ? gEnv->pEntitySystem->GetEntity(objectId) : NULL;

            const string& actionName = GetPortString(pActInfo, 0);
            if (!actionName.empty())
            {
                bool highPriority = false;
                if (IAIAction* pAIAction = pActInfo->pGraph->GetAIAction())
                {
                    highPriority = pAIAction->IsHighPriority();
                }
                int id = gEnv->pAISystem->GetSmartObjectManager()->SmartObjectEvent(actionName, pUser, pObject, NULL, highPriority);
                if (id)
                {
                    if (pUser)
                    {
                        ActivateOutput(pActInfo, 0, pUser->GetId());
                    }
                    if (pObject)
                    {
                        ActivateOutput(pActInfo, 1, pObject->GetId());
                    }
                    ActivateOutput(pActInfo, 2, true);
                }
                else
                {
                    ActivateOutput(pActInfo, 3, true);
                }
            }
            break;
        }
    }
};

//////////////////////////////////////////////////////////////////////////
// Enable/Disable AI for an entity/
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIEnable
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    enum EInputs
    {
        IN_ENABLE,
        IN_DISABLE,
    };
    CFlowNode_AIEnable(SActivationInfo* pActInfo) {};

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_AnyType("Enable", _HELP("Enable AI for target entity")),
            InputPortConfig_AnyType("Disable", _HELP("Disable AI for target entity")),
            {0}
        };
        config.sDescription = _HELP("Plays an Animation");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = 0;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (!pActInfo->pEntity)
            {
                return;
            }

            if (IsPortActive(pActInfo, IN_ENABLE))
            {
                if (IAIObject* aiObject = pActInfo->pEntity->GetAI())
                {
                    aiObject->Event(AIEVENT_ENABLE, 0);
                }
            }
            if (IsPortActive(pActInfo, IN_DISABLE))
            {
                if (IAIObject* aiObject = pActInfo->pEntity->GetAI())
                {
                    aiObject->Event(AIEVENT_DISABLE, 0);
                }
            }
            break;
        }

        case eFE_Initialize:
            break;
        }
        ;
    };
};

//////////////////////////////////////////////////////////////////////////
// control auto disabling
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIAutoDisable
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    enum EInputs
    {
        IN_ON,
        IN_OFF,
    };
    CFlowNode_AIAutoDisable(SActivationInfo* pActInfo) {};

    /*
    IFlowNodePtr Clone( SActivationInfo * pActInfo )
    {
        return this; // no internal state! // new CFlowNode_AIAutoDisable(pActInfo);
    }
    */

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_AnyType("ON", _HELP("allow AutoDisable for target entity")),
            InputPortConfig_AnyType("OFF", _HELP("no AutoDisable for target entity")),
            {0}
        };
        config.sDescription = _HELP("Controls AutoDisable.");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = 0;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (!pActInfo->pEntity || !pActInfo->pEntity->GetAI() ||
                !pActInfo->pEntity->GetAI()->GetProxy())
            {
                return;
            }
            CAIProxy*   proxy = (CAIProxy*)pActInfo->pEntity->GetAI()->GetProxy();
            if (IsPortActive(pActInfo, IN_ON))
            {
                proxy->UpdateMeAlways(false);
            }
            if (IsPortActive(pActInfo, IN_OFF))
            {
                proxy->UpdateMeAlways(true);
            }
            break;
        }
        case eFE_Initialize:
            break;
        }
        ;
    };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};



//////////////////////////////////////////////////////////////////////////
// switching species hostility on/off (makes AI ignore enemies/be ignored)
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIIgnore
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    enum EInputs
    {
        IN_HOSTILE,
        IN_NOHOSTILE,
        IN_RESETPERCEPTION,
    };
    CFlowNode_AIIgnore(SActivationInfo* pActInfo) {};

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_AnyType("Hostile", _HELP("Default perception")),
            InputPortConfig_AnyType("Ignore", _HELP("Make it ignore enemies/be ignored")),
            InputPortConfig_AnyType("ResetPerception", _HELP("Reset current perception state")),
            {0}
        };
        config.sDescription = _HELP("Switching species hostility on/off (makes AI ignore enemies/be ignored).");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = 0;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (!pActInfo->pEntity)
            {
                return;
            }

            if (IsPortActive(pActInfo, IN_HOSTILE))
            {
                IAIActor* pAIActor = CastToIAIActorSafe(pActInfo->pEntity->GetAI());
                if (pAIActor)
                {
                    AgentParameters ap = pAIActor->GetParameters();
                    ap.m_bAiIgnoreFgNode = false;
                    pAIActor->SetParameters(ap);
                }
            }
            if (IsPortActive(pActInfo, IN_NOHOSTILE))
            {
                IAIActor* pAIActor = CastToIAIActorSafe(pActInfo->pEntity->GetAI());
                if (pAIActor)
                {
                    AgentParameters ap = pAIActor->GetParameters();
                    ap.m_bAiIgnoreFgNode = true;
                    pAIActor->SetParameters(ap);
                }
            }
            if (IsPortActive(pActInfo, IN_RESETPERCEPTION))
            {
                IAIActor* pAIActor = CastToIAIActorSafe(pActInfo->pEntity->GetAI());
                if (pAIActor)
                {
                    pAIActor->ResetPerception();
                }
            }
            break;
        }
        case eFE_Initialize:
            break;
        }
        ;
    };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
// switching species hostility on/off (makes AI ignore enemies/be ignored)
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIFactionReaction
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    enum
    {
        InFactionSource = 0,
        InFactionTarget,
        InReaction,
        InGet,
        InSet,
    };

    enum
    {
        OutNeutral = 0,
        OutFriendly,
        OutHostile,
    };

    CFlowNode_AIFactionReaction(SActivationInfo* pActInfo) {};

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<string>("Source", "", _HELP("Source faction"), 0, "enum_global:Faction"),
            InputPortConfig<string>("Target", "", _HELP("Target faction"), 0, "enum_global:Faction"),
            InputPortConfig<int>("Reaction", 0, _HELP("Source faction's reaction to target faction."),
                0, _UICONFIG("enum_int:(0)Hostile=0,(1)Neutral=1,(2)Friendly=2")),
            InputPortConfig_Void("Get", _HELP("Get specified faction reaction and trigger output.")),
            InputPortConfig_Void("Set", _HELP("Set specified faction reaction.")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<bool>("Neutral", "Source faction's reaction to target faction is neutral."),
            OutputPortConfig<bool>("Friendly", "Source faction's reaction to target faction is friendly."),
            OutputPortConfig<bool>("Hostile", "Source faction's reaction to target faction is hostile."),
            {0},
        };

        config.sDescription = _HELP("Set/Get Faction reaction info");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            bool output = false;
            IFactionMap::ReactionType reactionType;

            uint8 source = gEnv->pAISystem->GetFactionMap().GetFactionID(GetPortString(pActInfo, InFactionSource));
            uint8 target = gEnv->pAISystem->GetFactionMap().GetFactionID(GetPortString(pActInfo, InFactionTarget));

            if (IsPortActive(pActInfo, InGet))
            {
                reactionType = (IFactionMap::ReactionType)gEnv->pAISystem->GetFactionMap().GetReaction(source, target);
                output = true;
            }

            if (IsPortActive(pActInfo, InSet))
            {
                reactionType = (IFactionMap::ReactionType)GetPortInt(pActInfo, InReaction);
                gEnv->pAISystem->GetFactionMap().SetReaction(source, target, reactionType);
                output = true;
            }

            if (output)
            {
                switch (reactionType)
                {
                case IFactionMap::Neutral:
                    ActivateOutput<bool>(pActInfo, OutNeutral, true);
                    break;
                case IFactionMap::Friendly:
                    ActivateOutput<bool>(pActInfo, OutFriendly, true);
                    break;
                case IFactionMap::Hostile:
                    ActivateOutput<bool>(pActInfo, OutHostile, true);
                    break;
                default:
                    assert(0);
                    break;
                }
            }
            break;
        }
        case eFE_Initialize:
            break;
        }
        ;
    };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};



//////////////////////////////////////////////////////////////////////////
// Set the faction an AI character belongs to
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISetFaction
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    enum
    {
        InFaction,
        InSetToDefault,
        InSet
    };

    CFlowNode_AISetFaction(SActivationInfo* pActInfo) {}

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inConfig[] =
        {
            InputPortConfig<string>("Faction", "", _HELP("Faction to set."), 0, "enum_global:Faction"),
            InputPortConfig<bool>("SetToDefault", false, _HELP("Sets faction to default instead of specified one")),
            InputPortConfig_Void("Set", _HELP("Set the faction as specified.")),
            {0}
        };

        config.sDescription = _HELP("Set the faction an AI character belongs to.");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inConfig;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InSet))
            {
                IEntity* pEntity = pActInfo->pEntity;
                IAIObject* pAi = pEntity ? pEntity->GetAI() : NULL;

                if (pAi)
                {
                    const bool bSetToDefault = GetPortBool(pActInfo, InSetToDefault);
                    const char* szFactionName = NULL;

                    if (!bSetToDefault)
                    {
                        szFactionName = GetPortString(pActInfo, InFaction);
                    }
                    else
                    {
                        SmartScriptTable props;
                        IScriptTable* pScriptTable = pEntity->GetScriptTable();
                        if (pScriptTable && pScriptTable->GetValue("Properties", props))
                        {
                            props->GetValue("esFaction", szFactionName);
                        }
                    }

                    uint8 faction = gEnv->pAISystem->GetFactionMap().GetFactionID(szFactionName);
                    pAi->SetFactionID(faction);
                }
            }

            break;
        }

        case eFE_Initialize:
            break;
        }
        ;
    };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};



//////////////////////////////////////////////////////////////////////////
// scales down the perception of assigned AI
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIPerceptionScale
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_AIPerceptionScale(SActivationInfo* pActInfo) {};

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Trigger", _HELP("Desired perception scale is set when this node is activated")),
            InputPortConfig<float>("Scale", 1.0f, _HELP("Desired visual perception scale factor"), _HELP("Visual")),
            InputPortConfig<float>("Audio", 1.0f, _HELP("Desired audio perception scale factor")),
            {0}
        };
        config.sDescription = _HELP("Scales the agent's sensitivity");
        config.nFlags |= EFLN_TARGET_ENTITY | EFLN_AISEQUENCE_SUPPORTED;
        config.pInputPorts = in_config;
        config.pOutputPorts = 0;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, 0) && pActInfo->pEntity)
        {
            IAIActor* pAIActor = CastToIAIActorSafe(pActInfo->pEntity->GetAI());
            if (pAIActor)
            {
                AgentParameters ap = pAIActor->GetParameters();
                ap.m_PerceptionParams.perceptionScale.visual = GetPortFloat(pActInfo, 1);
                ap.m_PerceptionParams.perceptionScale.audio = GetPortFloat(pActInfo, 2);
                pAIActor->SetParameters(ap);
            }
        }
        ;
    };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
// changes parameter of assigned AI
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIChangeParameter
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum EAIParamNames
    {
        AIPARAM_FACTION = 0, // make sure they match the UICONFIG below
        AIPARAM_GROUPID,
        AIPARAM_FORGETFULNESS
    };

public:
    CFlowNode_AIChangeParameter(SActivationInfo* pActInfo)
    {
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void  ("Trigger", _HELP("Change the parameter of the AI")),
            InputPortConfig<int>  ("Param", AIPARAM_FACTION, _HELP("Parameter to change"), _HELP("Parameter"),
                _UICONFIG("enum_int:Faction=0,GroupId=1,Forgetfulness=2")),
            InputPortConfig<float>("Value", 0.0f, _HELP("Value")),
            InputPortConfig<string>("Faction", "", _HELP("Faction"), 0, _UICONFIG("enum_global:Faction")),
            {0}
        };
        config.sDescription = _HELP("Changes an AI's parameters dynamically");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = 0;
        config.SetCategory(EFLN_ADVANCED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, 0) && pActInfo->pEntity)
        {
            IAIActor* pAIActor = CastToIAIActorSafe(pActInfo->pEntity->GetAI());
            if (pAIActor)
            {
                int param = GetPortInt(pActInfo, 1);
                float val = GetPortFloat(pActInfo, 2);
                const string& faction = GetPortString(pActInfo, 3);

                AgentParameters ap = pAIActor->GetParameters();
                switch (param)
                {
                case AIPARAM_FACTION:
                    ap.factionID = gEnv->pAISystem->GetFactionMap().GetFactionID(faction.c_str());
                    break;
                case AIPARAM_GROUPID:
                    ap.m_nGroup = (int)val;
                    break;
                case AIPARAM_FORGETFULNESS:
                    ap.m_PerceptionParams.forgetfulness = val;
                    break;
                default:
                    break;
                }
                pAIActor->SetParameters(ap);
            }
        }
        ;
    };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
// Make hunter's freezing beam follow the player but with
// some limited speed to allow the player avoid the beam if he's
// running fast enough
//////////////////////////////////////////////////////////////////////////
class CFlowNode_HunterShootTarget
    : public CFlowBaseNode<eNCT_Instanced>
{
    IAIObject* m_pTarget;
    Vec3 m_targetPos;
    Vec3 m_lastPos;
    float m_startTime;
    float m_lastTime;
    float m_duration;

public:
    CFlowNode_HunterShootTarget(SActivationInfo* pActInfo) {};
    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_HunterShootTarget(pActInfo);
    }
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Trigger", _HELP("Starts shooting")),
            InputPortConfig<float>("Speed", _HELP("Speed of oscillation")),
            InputPortConfig<float>("Width", _HELP("Width factor of oscillation")),
            InputPortConfig<float>("Duration", _HELP("Duration")),
            InputPortConfig<float>("TargetAlignSpeed", _HELP("Speed of aligning to target")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<Vec3>("targetPos", _HELP("Target shooting position")),
            {0}
        };
        config.sDescription = _HELP("Switching species hostility on/off (makes AI ignore enemies/be ignored).");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_DEBUG);
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Update)
        {
            IAIObject* pAI = pActInfo->pEntity->GetAI();
            if (!pAI)
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                return;
            }

            float time = gEnv->pTimer->GetCurrTime();

            float relTime = time - m_startTime;
            float timeStep = time - m_lastTime;
            m_lastTime = time;

            if (relTime >= m_duration)
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                return;
            }

            Vec3 pos = pAI->GetPos();

            IAIActor* pAIActor = pAI->CastToIAIActor();
            if (!pAIActor)
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                return;
            }

            IAIObject* pTarget = pAIActor->GetAttentionTarget();

            Vec3 targetPos(m_lastPos);
            if (pTarget && (pTarget == m_pTarget || Distance::Point_PointSq(pTarget->GetPos(), m_lastPos) < 4 * 4))
            {
                targetPos = pTarget->GetPos();
            }

            Vec3 fireDir = pos - targetPos;
            Vec3 fireDir2d(fireDir.x, fireDir.y, 0);
            Vec3 sideDir = fireDir.Cross(fireDir2d);
            Vec3 upDir(0, 0, 1);// = fireDir.Cross( sideDir ).normalized();
            sideDir.NormalizeSafe();

            float dist = fireDir2d.GetLength2D();

            Vec3 diffTarget = targetPos - m_targetPos;
            float distTarget = diffTarget.GetLength();

            if (timeStep &&  distTarget > 0.05f)
            {
                float alignSpeed = GetPortFloat(pActInfo, 4);

                m_targetPos += diffTarget.GetNormalizedSafe() * timeStep * alignSpeed;
                /*
                                // make m_lastPos follow m_targetPos with limited speed
                                Vec3 aim = targetPos + (targetPos - m_targetPos) * 2.0f;
                                m_targetPos = targetPos;
                                Vec3 dir = aim - m_lastPos;
                                float maxDelta = GetPortFloat( pActInfo, 1 ) * timeStep;
                                if ( maxDelta > 0 )
                                {
                                    if ( dir.GetLengthSquared() > maxDelta*maxDelta )
                                        dir.SetLength( maxDelta );
                                    m_lastPos += dir;
                                }
                            */
            }



#ifndef PI
#define PI 3.14159265358979323f  /* You know this number, don't you? */
#endif
            const float maxAmp = 8;
            float a = max(0.f, (m_duration * 0.6f - relTime) / m_duration); //0.6f = 60% chasing target and 40% exact aiming
            float b = sinf(relTime / m_duration * GetPortFloat(pActInfo, 1) * 180.0f / PI);
            a *= a;


            fireDir = pos - m_targetPos;
            fireDir2d.x = fireDir.x;
            fireDir2d.y = fireDir.y;
            sideDir = fireDir.Cross(fireDir2d);
            sideDir.NormalizeSafe();

            Vec3 result = m_targetPos - upDir * a * dist * maxAmp;

            result.z -= 2.5f;

            Vec3 actualCenteredFireDir(result - pos);
            actualCenteredFireDir.NormalizeSafe();
            b *= GetPortFloat(pActInfo, 2) * dist * a * (1 - actualCenteredFireDir.Dot(-fireDir.GetNormalizedSafe()));

            result += sideDir * b;

            ActivateOutput(pActInfo, 0, result);

            return;
        }


        if (event != eFE_Activate)
        {
            return;
        }

        if (!pActInfo->pEntity)
        {
            return;
        }

        IAIObject* pAI = pActInfo->pEntity->GetAI();
        if (!pAI)
        {
            return;
        }

        IPipeUser* pPipeUser = pAI->CastToIPipeUser();
        if (!pPipeUser)
        {
            return;
        }

        if (IsPortActive(pActInfo, 0))
        {
            m_pTarget = pPipeUser->GetSpecialAIObject("atttarget");
            if (m_pTarget)
            {
                m_lastTime = m_startTime = gEnv->pTimer->GetCurrTime();
                m_lastPos = m_targetPos = m_pTarget->GetPos();
                m_duration = GetPortFloat(pActInfo, 3);
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            }
        }
    };
};





/**
 * Makes the Hunter's freeze beam sweep around without any specific target.
 */
// Spinned off of CFlowNode_HunterShootTarget.
class CFlowNode_HunterBeamSweep
    : public CFlowBaseNode<eNCT_Instanced>
{
    Vec3 m_targetPos;
    float m_startTime;
    float m_duration;

    void ProcessActivate(SActivationInfo* pActInfo)
    {
        if (!IsPortActive(pActInfo, 0))
        {
            return;
        }

        if (!pActInfo->pEntity)
        {
            return;
        }

        IAIObject* pAI = pActInfo->pEntity->GetAI();
        if (!pAI)
        {
            return;
        }

        m_startTime = gEnv->pTimer->GetCurrTime();
        m_targetPos = pAI->GetPos() + GetPortFloat(pActInfo, 1) * pAI->GetEntityDir();
        m_duration = GetPortFloat(pActInfo, 6);
        pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
    }

    void ProcessUpdate(SActivationInfo* pActInfo)
    {
        if (!pActInfo->pEntity)
        {
            return;
        }

        IAIObject* pAI = pActInfo->pEntity->GetAI();
        if (!pAI)
        {
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            return;
        }

        float time = gEnv->pTimer->GetCurrTime();
        float relTime = time - m_startTime;
        if (relTime >= m_duration)
        {
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            return;
        }

        Matrix34 worldTransform = pActInfo->pEntity->GetWorldTM();
        Vec3 fireDir = worldTransform.GetColumn1();
        Vec3 sideDir = worldTransform.GetColumn0();
        Vec3 upDir = worldTransform.GetColumn2();

        float baseDist = GetPortFloat(pActInfo, 1);
        float leftRightSpeed = GetPortFloat(pActInfo, 2);
        float width = GetPortFloat(pActInfo, 3);
        float nearFarSpeed = GetPortFloat(pActInfo, 4);
        float dist = GetPortFloat(pActInfo, 5);

        // runs from 0 to 2*PI and indicates where in the 2*PI long cycle we are now
        float t = 2 * PI * relTime / m_duration;

        // NOTE Mrz 31, 2007: <pvl> essetially a Lissajous curve, with some
        // irregularity added by a higher frequency harmonic and offset vertically
        // because we don't want the hunter to shoot at the sky.
        Vec3 result = pAI->GetPos() +
            baseDist * fireDir +
            dist  * sinf (nearFarSpeed * t) * upDir  +
            0.2f * dist * sinf (2.3f * nearFarSpeed * t + 1.0f /*phase shift*/) * upDir +
            -1.8f * dist * upDir +
            width * sinf (leftRightSpeed * t) * sideDir +
            0.2f * width * sinf (2.3f * leftRightSpeed * t + 1.0f /*phase shift*/) * sideDir;

        ActivateOutput(pActInfo, 0, result);
    }

public:
    CFlowNode_HunterBeamSweep(SActivationInfo* pActInfo) {};
    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_HunterBeamSweep(pActInfo);
    }
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Trigger", _HELP("Starts shooting")),
            InputPortConfig<float>("BaseDistance", _HELP("Average distance around which sweeping will oscillate")),
            InputPortConfig<float>("LeftRightSpeed", _HELP("Speed of width oscillation")),
            InputPortConfig<float>("Width", _HELP("Amplitude of width oscillation")),
            InputPortConfig<float>("NearFarSpeed", _HELP("Speed of distance oscillation")),
            InputPortConfig<float>("Distance", _HELP("Amplitude of distance oscillation")),
            InputPortConfig<float>("Duration", _HELP("Duration")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<Vec3>("targetPos", _HELP("Target shooting position")),
            {0}
        };
        config.sDescription = _HELP("Switching species hostility on/off (makes AI ignore enemies/be ignored).");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_DEBUG);
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Update)
        {
            ProcessUpdate(pActInfo);
        }
        else if (event == eFE_Activate)
        {
            ProcessActivate(pActInfo);
        }
    }
};






//////////////////////////////////////////////////////////////////////////
// Nav cost factor
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISetNavCostFactor
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_AISetNavCostFactor(IFlowNode::SActivationInfo* pActInfo) {}
    virtual void GetConfiguration(SFlowNodeConfig& config);
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo);

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

void CFlowNode_AISetNavCostFactor::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] = {
        InputPortConfig_Void("sink", _HELP("for synchronization only"), _HELP("sync")),
        InputPortConfig<float>("Factor", _HELP("Nav cost factor. <0 disables navigation. 0 has no effect. >0 discourages navigation")),
        InputPortConfig<string>("NavModifierName", _HELP("Name of the cost-factor navigation modifier")),
        {0}
    };
    static const SOutputPortConfig out_config[] = {
        OutputPortConfig<int>("Done", _HELP("Done")),
        {0}
    };
    config.sDescription = _HELP("Set navigation cost factor for travelling through a region");
    config.pInputPorts = in_config;
    config.pOutputPorts = out_config;
    config.SetCategory(EFLN_APPROVED);
}

//
//-------------------------------------------------------------------------------------------------------------
void CFlowNode_AISetNavCostFactor::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    if (event == eFE_Activate)
    {
        float factor = GetPortFloat(pActInfo, 1);
        const string& pathName = GetPortString(pActInfo, 2);

        gEnv->pAISystem->GetINavigation()->ModifyNavCostFactor(pathName.c_str(), factor);

        ActivateOutput(pActInfo, 0, 1);
    }
}


//////////////////////////////////////////////////////////////////////////
// Enable or Disable AIShape
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIEnableShape
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_AIEnableShape(IFlowNode::SActivationInfo* pActInfo) {}
    virtual void GetConfiguration(SFlowNodeConfig& config);
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo);

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

void CFlowNode_AIEnableShape::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] = {
        InputPortConfig_Void("enable", _HELP("Enables the AI shape"), _HELP("Enable")),
        InputPortConfig_Void("disable", _HELP("Disables the AI shape"), _HELP("Disable")),
        InputPortConfig<string>("ShapeName", _HELP("Name of the shape to set as the territory.")),
        {0}
    };
    config.sDescription = _HELP("Enable or Disable AIShape.");
    config.nFlags = 0;
    config.pInputPorts = in_config;
    config.pOutputPorts = 0;
    config.SetCategory(EFLN_ADVANCED);
}

//
//-------------------------------------------------------------------------------------------------------------
void CFlowNode_AIEnableShape::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    if (event == eFE_Activate)
    {
        if (IsPortActive(pActInfo, 0))
        {
            const string& shapeName = GetPortString(pActInfo, 2);
            gEnv->pAISystem->EnableGenericShape(shapeName.c_str(), true);
        }
        else if (IsPortActive(pActInfo, 1))
        {
            const string& shapeName = GetPortString(pActInfo, 2);
            gEnv->pAISystem->EnableGenericShape(shapeName.c_str(), false);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
// AI Event Listener
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIEventListener
    : public CFlowBaseNode<eNCT_Instanced>
    , IAIEventListener
{
public:
    CFlowNode_AIEventListener(SActivationInfo* pActInfo)
        : m_pos(0, 0, 0)
        , m_radius(0.0f)
        , m_thresholdSound(0.5f)
        , m_thresholdCollision(0.5f)
        , m_thresholdBullet(0.5f)
        , m_thresholdExplosion(0.5f)
    {
        m_nodeID = pActInfo->myID;
        m_pGraph = pActInfo->pGraph;
    }
    ~CFlowNode_AIEventListener()
    {
        if (gEnv->pAISystem)
        {
            gEnv->pAISystem->UnregisterAIEventListener(this);
        }
    }
    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_AIEventListener(pActInfo);
    }
    void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        ser.Value("pos", m_pos);
        ser.Value("radius", m_radius);
        ser.Value("m_thresholdSound", m_thresholdSound);
        ser.Value("m_thresholdCollision", m_thresholdCollision);
        ser.Value("m_thresholdBullet", m_thresholdBullet);
        ser.Value("m_thresholdExplosion", m_thresholdExplosion);
    }
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig_Void("Sound"),
            OutputPortConfig_Void("Collision"),
            OutputPortConfig_Void("Bullet"),
            OutputPortConfig_Void("Explosion"),
            {0}
        };
        static const SInputPortConfig in_config[] = {
            InputPortConfig<Vec3>("Pos", _HELP("Position of the listener")),
            InputPortConfig<float>("Radius", 0.0f, _HELP("Radius of the listener")),
            InputPortConfig<float>("ThresholdSound", 0.5f, _HELP("Sensitivity of the sound output")),
            InputPortConfig<float>("ThresholdCollision", 0.5f, _HELP("Sensitivity of the collision output")),
            InputPortConfig<float>("ThresholdBullet", 0.5f, _HELP("Sensitivity of the bullet output")),
            InputPortConfig<float>("ThresholdExplosion", 0.5f, _HELP("Sensitivity of the explosion output")),
            {0}
        };
        config.sDescription = _HELP("The highest level of alertness of any agent in the level");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_ADVANCED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        const int stimFlags = (1 << AISTIM_SOUND) | (1 << AISTIM_EXPLOSION) | (1 << AISTIM_BULLET_HIT) | (1 << AISTIM_COLLISION);

        if (event == eFE_Initialize)
        {
            m_pos = GetPortVec3(pActInfo, 0);
            m_radius = GetPortFloat(pActInfo, 1);
            gEnv->pAISystem->RegisterAIEventListener(this, m_pos, m_radius, stimFlags);

            m_thresholdSound = GetPortFloat(pActInfo, 2);
            m_thresholdCollision = GetPortFloat(pActInfo, 3);
            m_thresholdBullet = GetPortFloat(pActInfo, 4);
            m_thresholdExplosion = GetPortFloat(pActInfo, 5);
        }

        if (event == eFE_Activate)
        {
            if (IsPortActive(pActInfo, 0))
            {
                // Change pos
                Vec3 pos = GetPortVec3(pActInfo, 0);
                if (Distance::Point_PointSq(m_pos, pos) > sqr(0.01f))
                {
                    m_pos = pos;
                    gEnv->pAISystem->RegisterAIEventListener(this, m_pos, m_radius, stimFlags);
                }
            }
            else if (IsPortActive(pActInfo, 1))
            {
                // Change radius
                float rad = GetPortFloat(pActInfo, 1);
                if (fabsf(rad - m_radius) > 0.0001f)
                {
                    m_radius = rad;
                    gEnv->pAISystem->RegisterAIEventListener(this, m_pos, m_radius, stimFlags);
                }
            }
            else if (IsPortActive(pActInfo, 2))
            {
                // Change threshold
                m_thresholdSound = GetPortFloat(pActInfo, 2);
            }
            else if (IsPortActive(pActInfo, 3))
            {
                // Change threshold
                m_thresholdCollision = GetPortFloat(pActInfo, 3);
            }
            else if (IsPortActive(pActInfo, 4))
            {
                // Change threshold
                m_thresholdBullet = GetPortFloat(pActInfo, 4);
            }
            else if (IsPortActive(pActInfo, 5))
            {
                // Change threshold
                m_thresholdExplosion = GetPortFloat(pActInfo, 5);
            }
        }
    }
    virtual void OnAIEvent(EAIStimulusType type, const Vec3& pos, float radius, float threat, EntityId sender)
    {
        CRY_ASSERT(m_pGraph->IsEnabled() && !m_pGraph->IsSuspended() && m_pGraph->IsActive());

        float dist = Distance::Point_Point(m_pos, pos);
        float u = 0.0f;

        if (radius > 0.001f)
        {
            u = (1.0f - clamp_tpl((dist - m_radius) / radius, 0.0f, 1.0f)) * threat;
        }

        float thr = 0.0f;

        switch (type)
        {
        case AISTIM_SOUND:
            thr = m_thresholdSound;
            break;
        case AISTIM_COLLISION:
            thr = m_thresholdCollision;
            break;
        case AISTIM_BULLET_HIT:
            thr = m_thresholdBullet;
            break;
        case AISTIM_EXPLOSION:
            thr = m_thresholdExplosion;
            break;
        default:
            return;
        }

        if (thr > 0.0f && u > thr)
        {
            int i = 0;
            switch (type)
            {
            case AISTIM_SOUND:
                i = 0;
                break;
            case AISTIM_COLLISION:
                i = 1;
                break;
            case AISTIM_BULLET_HIT:
                i = 2;
                break;
            case AISTIM_EXPLOSION:
                i = 3;
                break;
            default:
                return;
            }
            SFlowAddress addr(m_nodeID, i, true);
            m_pGraph->ActivatePort(addr, true);
        }
    }
    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
private:
    Vec3 m_pos;
    float m_radius;
    float m_thresholdSound;
    float m_thresholdCollision;
    float m_thresholdBullet;
    float m_thresholdExplosion;
    IFlowGraph* m_pGraph;
    TFlowNodeId m_nodeID;
};

//////////////////////////////////////////////////////////////////////////
// Register an entity with the interest system
class CFlowNode_InterestingEntity
    : public CFlowBaseNode<eNCT_Instanced>
    , public IInterestListener
{
public:
    enum EInputs
    {
        IN_REGISTER_INTERESTING,
        IN_UNREGISTER_INTERESTING,
        IN_RADIUS,
        IN_INTEREST_LEVEL,
        IN_ACTION,
        IN_OFFSET,
        IN_PAUSE,
        IN_SHARED,
    };

    enum EOutputs
    {
        OUT_ACTOR_INTERESTED,
        OUT_ACTOR_NOT_INTERESTED,
        OUT_ACTOR_ACTION_COMPLETE,
        OUT_ACTOR_ACTION_ABORT,
    };

    CFlowNode_InterestingEntity(SActivationInfo* pActInfo)
    {
        m_nodeID = 0;
        m_pGraph = NULL;
        m_EntityId = 0;
    }

    virtual ~CFlowNode_InterestingEntity(void)
    {
        ICentralInterestManager* pCIM = gEnv->pAISystem->GetCentralInterestManager();
        if (m_EntityId != 0 && pCIM != NULL)
        {
            pCIM->UnRegisterListener(this, m_EntityId);
            m_EntityId = 0;
            m_pGraph = NULL;
        }
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_InterestingEntity(pActInfo);
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        ser.Value("m_EntityId", m_EntityId);
    }

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<FlowEntityId>("OnActorInterested", _HELP("Outputs when an AI Entity gets interested in this. It carries the FlowEntityId")),
            OutputPortConfig<FlowEntityId>("OnActorLoseInterest", _HELP("Outputs when an AI Entity is no longer interested in this. It carries the FlowEntityId")),
            OutputPortConfig<FlowEntityId>("OnActorCompleteAction", _HELP("Outputs when AI Entity completes an action associated with becoming interseted. It carries the FlowEntityId")),
            OutputPortConfig<FlowEntityId>("OnActorAbortAction", _HELP("Outputs when an action is aborted associated with this interest target. It carries the FlowEntityId")),
            {0}
        };

        static const SInputPortConfig in_ports[] =
        {
            InputPortConfig_Void("Register", _HELP("Register entity as interesting")),
            InputPortConfig_Void("Unregister", _HELP("Unregister entity as interesting")),
            InputPortConfig<float>("Radius",   -1.0f, _HELP("This object will be only interesting from this radius. Negative value will use previous setup")),
            InputPortConfig<float>("InterestLevel",    -1.0f, _HELP("Base interest level of object or minimum interest level of actor. Negative value will use previous setup")),
            InputPortConfig<string>("Action",  _HELP("AI Action. If empty it will be used the instance value if any"), 0),
            InputPortConfig<Vec3>("Offset", Vec3(ZERO),    _HELP("Offset from the pivot to determine visibility"), 0),
            InputPortConfig<float>("Pause", -1.f,  _HELP("Pause between successive uses of the same entity by the same AI"), 0),
            InputPortConfig<int>("Shared", -1, _HELP("1 if the entity can be used by more than one AI at the same time"), 0),
            {0}
        };
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_ports;
        config.pOutputPorts = out_config;
        config.sDescription = _HELP("Registers an entity as interesting");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        IEntity* pEntity = pActInfo->pEntity;
        if (!pEntity)
        {
            return;
        }

        ICentralInterestManager* pCIM = gEnv->pAISystem->GetCentralInterestManager();

        if (event == eFE_SetEntityId && (IsOutputConnected(pActInfo, OUT_ACTOR_INTERESTED) || IsOutputConnected(pActInfo, OUT_ACTOR_NOT_INTERESTED)
                                         || IsOutputConnected(pActInfo, OUT_ACTOR_ACTION_COMPLETE) || IsOutputConnected(pActInfo, OUT_ACTOR_ACTION_ABORT)))
        {
            if (m_EntityId)
            {
                pCIM->UnRegisterListener(this, m_EntityId);
            }
            pCIM->RegisterListener(this, pEntity->GetId());
            m_EntityId = pEntity->GetId();
            m_pGraph = pActInfo->pGraph;
            m_nodeID = pActInfo->myID;
        }

        if (event == eFE_Activate)
        {
            if (IsPortActive(pActInfo, IN_REGISTER_INTERESTING))
            {
                pCIM->RegisterListener(this, pEntity->GetId());
                float radius = GetPortFloat(pActInfo, IN_RADIUS);
                float interestLevel = GetPortFloat(pActInfo, IN_INTEREST_LEVEL);
                const string& action = GetPortString(pActInfo, IN_ACTION);
                Vec3 offset = GetPortVec3(pActInfo, IN_OFFSET);
                float pause = GetPortFloat(pActInfo, IN_PAUSE);
                int shared = GetPortInt(pActInfo, IN_SHARED);

                pCIM->ChangeInterestingEntityProperties(pEntity, radius, interestLevel, action.c_str(), offset, pause, shared);
            }
            else if (IsPortActive(pActInfo, IN_UNREGISTER_INTERESTING))
            {
                pCIM->DeregisterInterestingEntity(pEntity);
                // if there are pending notifications, and we unregister, they would be missed. Not unregistering here should not be an issue anyway.
                //              pCIM->UnRegisterListener(this, pEntity->GetId());
            }
        }
    }

    virtual void OnInterestEvent(EInterestEvent event, EntityId idInterestedActor, EntityId idInterestingEntity)
    {
        if (m_pGraph != NULL && m_nodeID != 0 && idInterestingEntity == m_EntityId)
        {
            CRY_ASSERT(m_pGraph->IsEnabled() && !m_pGraph->IsSuspended() && m_pGraph->IsActive());
            switch (event)
            {
            case IInterestListener::eIE_InterestStart:
                m_pGraph->ActivatePort(SFlowAddress(m_nodeID, OUT_ACTOR_INTERESTED, true), idInterestedActor);
                break;
            case IInterestListener::eIE_InterestStop:
                m_pGraph->ActivatePort(SFlowAddress(m_nodeID, OUT_ACTOR_NOT_INTERESTED, true), idInterestedActor);
                break;
            case IInterestListener::eIE_InterestActionComplete:
                m_pGraph->ActivatePort(SFlowAddress(m_nodeID, OUT_ACTOR_ACTION_COMPLETE, true), idInterestedActor);
                break;
            case IInterestListener::eIE_InterestActionAbort:
                m_pGraph->ActivatePort(SFlowAddress(m_nodeID, OUT_ACTOR_ACTION_ABORT, true), idInterestedActor);
                break;
            default:
                break;
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

private:

    IFlowGraph* m_pGraph;
    TFlowNodeId m_nodeID;
    FlowEntityId m_EntityId;
};


//////////////////////////////////////////////////////////////////////////
// Register an entity with the interest system
class CFlowNode_Interested
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    enum EInputs
    {
        IN_REGISTER_INTERESTED,
        IN_UNREGISTER_INTERESTED,
        IN_INTEREST_FILTER,
        IN_ANGLE
    };

    CFlowNode_Interested(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config)
    {
        // declare input ports
        static const SInputPortConfig in_ports[] =
        {
            InputPortConfig_Void("Register", _HELP("Register entity as interested")),
            InputPortConfig_Void("Unregister", _HELP("Unregister entity as interested")),
            InputPortConfig<float>("InterestFilter",   -1.0f, _HELP("Minimum interest level of actor. Negative value will use previous setup")),
            InputPortConfig<float>("Angle", -1.0f, _HELP("Actor will ignore object out of this angle. Negative value will use previous setup")),
            {0}
        };
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = in_ports;
        config.pOutputPorts = 0;
        config.sDescription = _HELP("Registers an entity with the interest system");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        IEntity* pEntity = pActInfo->pEntity;
        if (!pEntity)
        {
            return;
        }

        if (event == eFE_Activate)
        {
            ICentralInterestManager* pCIM = gEnv->pAISystem->GetCentralInterestManager();

            if (IsPortActive(pActInfo, IN_REGISTER_INTERESTED))
            {
                float interestLevel = GetPortFloat(pActInfo, IN_INTEREST_FILTER);

                float fAngleCos;
                float fAngleInDegrees = GetPortFloat(pActInfo, IN_ANGLE);
                if (fAngleInDegrees >= 0.f)
                {
                    clamp_tpl(fAngleInDegrees, 0.f, 360.f);
                    fAngleCos = cos(DEG2RAD(.5f * fAngleInDegrees));
                }
                else
                {
                    fAngleCos = -100;   // Leave the angle (cosine) intact
                }

                pCIM->ChangeInterestedAIActorProperties(pEntity, interestLevel, fAngleCos);
            }
            else if (IsPortActive(pActInfo, IN_UNREGISTER_INTERESTED))
            {
                pCIM->DeregisterInterestedAIActor(pEntity);
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////

class CFlowNode_AICommunication
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    CFlowNode_AICommunication(SActivationInfo* pActInfo)
        : m_playID(0)
    {
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_AICommunication(pActInfo);
    };

    void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        ser.Value("m_playID", m_playID);

        if (ser.IsReading() && (bool)m_playID)
        {
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
        }
    }

    enum EInputs
    {
        EIP_Start,
        EIP_Stop,
        EIP_CommName,
        EIP_ChannelName,
        EIP_ContextExpiry,
        EIP_SkipSound,
        EIP_SkipAnim,
        EIP_TargetId,
        EIP_TargetPos,
    };

    enum EOutputs
    {
        EOP_Done,
        EOP_Started,
        EOP_Stopped,
        EOP_Finished,
        EOP_Fail,
    };

    virtual bool GetPortGlobalEnum(uint32 portId, IEntity* pNodeEntity, const char* szName, string& outGlobalEnum) const
    {
        if (EIP_CommName == portId && pNodeEntity)
        {
            IAIObject* pAI = pNodeEntity->GetAI();
            IAIActorProxy* pAIProxy = pAI ? pAI->GetProxy() : NULL;
            if (pAIProxy)
            {
                const char* szCommConfigName = pAIProxy->GetCommunicationConfigName();
                outGlobalEnum.Format("%s_%s", szName, szCommConfigName);
                return true;
            }
        }

        return false;
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Start", _HELP("Starts the communication on the AI")),
            InputPortConfig_Void("Stop", _HELP("Stops the communication played with this node on the AI")),
            InputPortConfig<string>("Communication", "", _HELP("Name of the communication to play"), 0, _UICONFIG("enum_global_def:communications")),
            InputPortConfig<string>("Channel", "group", _HELP("Name of the channel to play the communication in"), 0, _UICONFIG("enum_string:=,Global=global,Group=group,Personal=personal")),
            InputPortConfig<float>("ContextExpirity", 0.0f, _HELP("The context expiration of the communication (how much time must past before it can be played again)")),
            InputPortConfig<bool>("SkipSound", false, _HELP("Force communication playback to skip sound component.")),
            InputPortConfig<bool>("SkipAnim", false, _HELP("Force communication playback to skip animation component.")),
            InputPortConfig<FlowEntityId>("TargetId", _HELP("Optional entity Id for whom the communication should be targeted at")),
            InputPortConfig<Vec3>("TargetPos", _HELP("Optional position for where the communication should be targeted at")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Done", _HELP("Outputs when any action has been carried out")),
            OutputPortConfig<FlowEntityId>("Started", _HELP("Outputs when communication is started")),
            OutputPortConfig<FlowEntityId>("Stopped", _HELP("Outputs when communication is stopped")),
            OutputPortConfig<FlowEntityId>("Finished", _HELP("Outputs when communication finishes playing")),
            OutputPortConfig<FlowEntityId>("Fail", _HELP("Outputs when an error occurs with the last operation")),
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.sDescription = _HELP("The specified AI plays the given communication");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (eFE_Update == event)
        {
            // Check if still playing
            if ((bool)m_playID)
            {
                ICommunicationManager* pCommunicationManager = gEnv->pAISystem->GetCommunicationManager();
                assert(pCommunicationManager);

                const bool bIsPlaying = pCommunicationManager->IsPlaying(m_playID);
                if (!bIsPlaying)
                {
                    IEntity* pEntity = pActInfo->pEntity;
                    ActivateOutput(pActInfo, EOP_Finished, pEntity ? pEntity->GetId() : 0);

                    m_playID.id = 0;

                    pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                }
            }
            else
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            }
        }
        else if (eFE_Activate == event)
        {
            if (IsPortActive(pActInfo, EIP_Start))
            {
                StartCommunication(pActInfo);
            }
            else if (IsPortActive(pActInfo, EIP_Stop))
            {
                StopCommunication(pActInfo);
            }
        }
    }

    void StartCommunication(SActivationInfo* pActInfo)
    {
        assert(pActInfo);

        ICommunicationManager* pCommunicationManager = gEnv->pAISystem->GetCommunicationManager();
        assert(pCommunicationManager);

        bool bFailed = true;

        IEntity* pEntity = pActInfo->pEntity;
        IAIObject* pAI = pEntity ? pEntity->GetAI() : NULL;
        IAIActorProxy* pAIProxy = pAI ? pAI->GetProxy() : NULL;
        if (pAIProxy)
        {
            SCommunicationRequest request;
            request.actorID = pEntity->GetId();
            request.channelID = pCommunicationManager->GetChannelID(GetPortString(pActInfo, EIP_ChannelName));
            request.commID = pCommunicationManager->GetCommunicationID(GetPortString(pActInfo, EIP_CommName));
            request.configID = pCommunicationManager->GetConfigID(pAIProxy->GetCommunicationConfigName());
            request.contextExpiry = GetPortFloat(pActInfo, EIP_ContextExpiry);
            request.ordering = SCommunicationRequest::Ordered;
            request.skipCommSound = GetPortBool(pActInfo, EIP_SkipSound);
            request.skipCommAnimation = GetPortBool(pActInfo, EIP_SkipAnim);

            request.targetID = FlowEntityId(GetPortEntityId(pActInfo, EIP_TargetId));
            request.target = GetPortVec3(pActInfo, EIP_TargetPos);

            m_playID = pCommunicationManager->PlayCommunication(request);
            if ((bool)m_playID)
            {
                ActivateOutput(pActInfo, EOP_Started, pEntity->GetId());
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
                bFailed = false;
            }
        }

        if (bFailed)
        {
            ActivateOutput(pActInfo, EOP_Fail, pEntity ? pEntity->GetId() : 0);
        }

        ActivateOutput(pActInfo, EOP_Done, true);
    }

    void StopCommunication(SActivationInfo* pActInfo)
    {
        assert(pActInfo);

        ICommunicationManager* pCommunicationManager = gEnv->pAISystem->GetCommunicationManager();
        assert(pCommunicationManager);

        if ((bool)m_playID)
        {
            if (pCommunicationManager->IsPlaying(m_playID))
            {
                pCommunicationManager->StopCommunication(m_playID);

                IEntity* pEntity = pActInfo->pEntity;
                ActivateOutput(pActInfo, EOP_Stopped, pEntity ? pEntity->GetId() : 0);
                ActivateOutput(pActInfo, EOP_Done, true);
            }

            m_playID.id = 0;
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
        }
    }

private:
    CommPlayID m_playID;
};


//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIIsAliveCheck
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_AIIsAliveCheck(SActivationInfo* pActInfo)
    {
    }

    enum
    {
        NUM_ACTORS = 8
    };

    enum EInputs
    {
        IN_Trigger = 0,
        IN_ActorFirst,
        IN_ActorLast = IN_ActorFirst + (NUM_ACTORS - 1)
    };

    enum EOutputs
    {
        OUT_AliveCount = 0,
        OUT_AliveFirst,
        OUT_AliveLast = OUT_AliveFirst + (NUM_ACTORS - 1)
    };


    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Trigger", _HELP("Trigger this port to check for alive status of the Actors")),
            InputPortConfig<FlowEntityId>("Actor_0", 0, _HELP("Entity ID"), _HELP("Actor 0")),
            InputPortConfig<FlowEntityId>("Actor_1", 0, _HELP("Entity ID"), _HELP("Actor 1")),
            InputPortConfig<FlowEntityId>("Actor_2", 0, _HELP("Entity ID"), _HELP("Actor 2")),
            InputPortConfig<FlowEntityId>("Actor_3", 0, _HELP("Entity ID"), _HELP("Actor 3")),
            InputPortConfig<FlowEntityId>("Actor_4", 0, _HELP("Entity ID"), _HELP("Actor 4")),
            InputPortConfig<FlowEntityId>("Actor_5", 0, _HELP("Entity ID"), _HELP("Actor 5")),
            InputPortConfig<FlowEntityId>("Actor_6", 0, _HELP("Entity ID"), _HELP("Actor 6")),
            InputPortConfig<FlowEntityId>("Actor_7", 0, _HELP("Entity ID"), _HELP("Actor 7")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<int>("AliveCount", _HELP("How many of the actors are alive")),
            OutputPortConfig<FlowEntityId>("Alive_0", _HELP("triggered if is alive"), _HELP("Alive Id 0")),
            OutputPortConfig<FlowEntityId>("Alive_1", _HELP("triggered if is alive"), _HELP("Alive Id 1")),
            OutputPortConfig<FlowEntityId>("Alive_2", _HELP("triggered if is alive"), _HELP("Alive Id 2")),
            OutputPortConfig<FlowEntityId>("Alive_3", _HELP("triggered if is alive"), _HELP("Alive Id 3")),
            OutputPortConfig<FlowEntityId>("Alive_4", _HELP("triggered if is alive"), _HELP("Alive Id 4")),
            OutputPortConfig<FlowEntityId>("Alive_5", _HELP("triggered if is alive"), _HELP("Alive Id 5")),
            OutputPortConfig<FlowEntityId>("Alive_6", _HELP("triggered if is alive"), _HELP("Alive Id 6")),
            OutputPortConfig<FlowEntityId>("Alive_7", _HELP("triggered if is alive"), _HELP("Alive Id 7")),
            {0}
        };
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Check which actors of a group are alive");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, IN_Trigger))
            {
                int numAlive = 0;

                for (int i = 0; i < NUM_ACTORS; ++i)
                {
                    FlowEntityId entityId = FlowEntityId(GetPortEntityId(pActInfo, IN_ActorFirst + i));
                    if (entityId != 0)
                    {
                        IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entityId);
                        bool isAlive = pActor && !pActor->IsDead();
                        if (isAlive)
                        {
                            ActivateOutput(pActInfo, OUT_AliveFirst + numAlive, entityId);
                            ++numAlive;
                        }
                    }
                }
                for (int i = numAlive; i < NUM_ACTORS; ++i)
                {
                    ActivateOutput(pActInfo, OUT_AliveFirst + i, 0);
                }
                ActivateOutput(pActInfo, OUT_AliveCount, numAlive);
            }
            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

class CFlowNode_AIGlobalPerceptionScaling
    : public CFlowBaseNode<eNCT_Instanced>
    , public IAIGlobalPerceptionListener
{
public:

    enum EInputs
    {
        IN_Enable = 0,
        IN_Disable,
        IN_AudioScale,
        IN_VisualScale,
        IN_FilterAI,
        IN_Faction,
    };

    enum EOutputs
    {
        OUT_Enabled = 0,
        OUT_Disabled,
    };

    CFlowNode_AIGlobalPerceptionScaling(SActivationInfo* pActInfo)
    {
        gEnv->pAISystem->RegisterGlobalPerceptionListener(this);
    }

    virtual ~CFlowNode_AIGlobalPerceptionScaling()
    {
        gEnv->pAISystem->UnregisterGlobalPerceptionlistener(this);
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Enable", _HELP("Enables the perception scaling.")),
            InputPortConfig_Void("Disable", _HELP("Disables the perception scaling.")),
            InputPortConfig<float>("AudioScale", 0.0f, _HELP("This is the value used to scale down the audio perception. Value is clamped between [0.0 - 1.0]"), _HELP("AudioScale")),
            InputPortConfig<float>("VisualScale", 0.0f, _HELP("This is the value used to scale fown the visual perception. Value is clamped between [0.0 - 1.0]"), _HELP("VisualScale")),
            InputPortConfig<int>("FilterAI", 0, _HELP("Filter which AIs are used for the alertness value."), _HELP("FilterAI"), _UICONFIG("enum_int:All=0,Enemies=1,Friends=2,Faction=3")),
            InputPortConfig<string>("Faction", "", _HELP("Only used when 'FilterAI' input is set to 'Faction'."), nullptr, _UICONFIG("enum_global:Faction")),
            {0},
        };

        static const SOutputPortConfig outputs[] = {
            OutputPortConfig_Void("Enabled", _HELP("Triggered when the node gets enabled.")),
            OutputPortConfig_Void("Disabled", _HELP("Triggered when the node gets disabled.")),
            {0},
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.sDescription = _HELP("Set/Unset a specific global scale for the AI perception.");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowNode_AIGlobalPerceptionScaling(pActInfo);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_actInfo = *pActInfo;
            break;
        }
        case eFE_Activate:
        {
            m_actInfo = *pActInfo;
            if (IsPortActive(pActInfo, IN_Enable))
            {
                float visualScale = ClampPerceptionScale(GetPortFloat(pActInfo, IN_VisualScale));
                float audioScale = ClampPerceptionScale(GetPortFloat(pActInfo, IN_AudioScale));
                EAIFilterType filterAI = GetFilterType(GetPortInt(pActInfo, IN_FilterAI));
                const string& factionName = GetPortString(pActInfo, IN_Faction);
                gEnv->pAISystem->UpdateGlobalPerceptionScale(visualScale, audioScale, filterAI, factionName);
            }

            if (IsPortActive(pActInfo, IN_Disable))
            {
                gEnv->pAISystem->DisableGlobalPerceptionScaling();
            }
            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    // IAIGlobalPerceptionListener implementation
    virtual void OnPerceptionScalingEvent(const IAIGlobalPerceptionListener::EGlobalPerceptionScaleEvent event)
    {
        switch (event)
        {
        case IAIGlobalPerceptionListener::eGPS_Set:
            ActivateOutput(&m_actInfo, OUT_Enabled, true);
            break;
        case IAIGlobalPerceptionListener::eGPS_Disabled:
            ActivateOutput(&m_actInfo, OUT_Disabled, true);
            break;
        default:
            assert(0);
            break;
        }
    }

private:
    float ClampPerceptionScale(float scaleValue)
    {
        return clamp_tpl(scaleValue, 0.0f, 1.0f);
    }

    EAIFilterType GetFilterType(uint8 filterType)
    {
        switch (filterType)
        {
        case 0:
            return eAIFT_All;
        case 1:
            return eAIFT_Enemies;
        case 2:
            return eAIFT_Friends;
        case 3:
            return eAIFT_Faction;
        default:
            return eAIFT_All;
        }
    }

    SActivationInfo m_actInfo; // Activation info instance
};

//////////////////////////////////////////////////////////////////////////
// Node to controls the communication manager variables
//////////////////////////////////////////////////////////////////////////

class CFlowNode_AISetCommunicationVariable
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    enum Inputs
    {
        eIN_Set = 0,
        eIN_VariableName,
        eIN_VariableValue,
    };

    CFlowNode_AISetCommunicationVariable(IFlowNode::SActivationInfo* pActInfo) {}
    virtual ~CFlowNode_AISetCommunicationVariable() {}

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_AISetCommunicationVariable(pActInfo); }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Set"),
            InputPortConfig<string>("communicationVariable_VariableName", _HELP("Variable name")),
            InputPortConfig<bool>("VariableValue", true, _HELP("Value that needs to be set for the specified variable.")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            {0}
        };

        config.sDescription = _HELP("");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
            break;
        case eFE_Activate:
            assert(gEnv->pAISystem);
            ICommunicationManager* pCommunicationManager = gEnv->pAISystem->GetCommunicationManager();
            assert(pCommunicationManager);

            const string variableName = GetPortString(pActInfo, eIN_VariableName);
            const bool variableValue = GetPortBool(pActInfo, eIN_VariableValue);

            pCommunicationManager->SetVariableValue(variableName.c_str(), variableValue);
            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const { s->Add(*this); }
};

//////////////////////////////////////////////////////////////////////////
// Node to notify AI that when the node is triggered, AI can perform
// some actions to express the reinforcement requests
//////////////////////////////////////////////////////////////////////////

class CFlowNode_AIRequestReinforcementReadability
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    enum Inputs
    {
        eIN_Trigger = 0,
        eIN_GroupId,
    };

    enum Outputs
    {
        eOUT_Done,
    };

    CFlowNode_AIRequestReinforcementReadability(IFlowNode::SActivationInfo* pActInfo) {}
    virtual ~CFlowNode_AIRequestReinforcementReadability() {}

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_AISetCommunicationVariable(pActInfo); }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_Void("Trigger"),
            InputPortConfig<int>("GroupId", _HELP("Id of the group that needs to be notified.")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig_Void("Done", _HELP("This value is outputted when the AI is informed."
                    "(This doesn't mean the AI alread performed the readable action of requesting the reinforcement.)")),
            {0}
        };

        config.sDescription = _HELP("");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
            break;
        case eFE_Activate:
            const int groupId = GetPortInt(pActInfo, eIN_GroupId);
            const char* signalName = "OnRequestReinforcementTriggered";

            // Select the first member of the group to use it as a sender
            IAIObject* pFirstGroupMember = gEnv->pAISystem->GetGroupMember(groupId, 0);
            if (pFirstGroupMember)
            {
                gEnv->pAISystem->SendSignal(SIGNALFILTER_GROUPONLY, 1, signalName, pFirstGroupMember);

                ActivateOutput(pActInfo, eOUT_Done, true);
            }

            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const { s->Add(*this); }
};


//////////////////////////////////////////////////////////////////////////
// Node to regenerate a part or everything of the MNM.
//////////////////////////////////////////////////////////////////////////

class CFlowNode_AIRegenerateMNM
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    enum Inputs
    {
        eIN_Trigger,
        eIN_Min,
        eIN_Max,
    };

    CFlowNode_AIRegenerateMNM(SActivationInfo* pActInfo)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Start", _HELP("Trigger this port to regenerate the MNM.")),
            InputPortConfig<Vec3>("Min", Vec3(ZERO), _HELP("Min position of the AABB")),
            InputPortConfig<Vec3>("Max", Vec3(ZERO), _HELP("Min position of the AABB")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.sDescription = _HELP("Triggers recalculation of MNM data for a specified bounding box (leave blank for whole level)");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
            if (IsPortActive(pActInfo, eIN_Trigger) && gEnv->pAISystem->IsEnabled())
            {
                pActInfo->pGraph->RequestFinalActivation(pActInfo->myID);
            }
            break;

        case eFE_FinalActivate:
            AABB aabb(GetPortVec3(pActInfo, eIN_Min), GetPortVec3(pActInfo, eIN_Max));
            gEnv->pAISystem->GetNavigationSystem()->WorldChanged(aabb);
            break;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
// Node to ray cast the MNM.
//////////////////////////////////////////////////////////////////////////

class CFlowNode_AIRayCastMNM
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate,
        Direction,
        MaxLength,
        Position,
        TransformDirection,
    };

    enum OutputPorts
    {
        NoHit,
        Hit,
        RayDirection,
        HitDistance,
        HitPoint,
        MeshId
    };

public:
    CFlowNode_AIRayCastMNM(SActivationInfo* pActInfo)
    {
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("Direction", Vec3(0, 1, 0), _HELP("Direction of Raycast")),
            InputPortConfig<float>("MaxLength", 10.0f, _HELP("Maximum length of Raycast")),
            InputPortConfig<Vec3>("Position", Vec3(0, 0, 0), _HELP("Ray start position, relative to entity")),
            InputPortConfig<bool>("TransformDirection", true, _HELP("Whether direction is transformed by entity orientation.")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<SFlowSystemVoid>("NoHit", _HELP("Activated if NO object was hit by the raycast")),
            OutputPortConfig<SFlowSystemVoid>("Hit", _HELP("Activated if an object was hit by the raycast")),
            OutputPortConfig<Vec3>("RayDirection", _HELP("Actual direction of the cast ray (possibly transformed by entity rotation, assumes [Hit])")),
            OutputPortConfig<float>("HitDistance", _HELP("Distance to the hit object (assumes [Hit])")),
            OutputPortConfig<Vec3>("HitPoint", _HELP("Position of the hit (assumes [Hit])")),
            OutputPortConfig<int>("MeshId", _HELP("The mesh id of the navigation mesh hit (assumes [Hit])")),
            { 0 }
        };
        config.sDescription = _HELP("Perform a raycast to the multilayer nav mesh relative to an entity.");
        config.nFlags |= (EFLN_TARGET_ENTITY | EFLN_ACTIVATION_INPUT);
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, InputPorts::Activate))
        {
            IEntity* pEntity = pActInfo->pEntity;
            if (pEntity)
            {
                const Vec3 LYDirection = GetPortBool(pActInfo, InputPorts::TransformDirection) ?
                    pEntity->GetWorldTM().TransformVector(GetPortVec3(pActInfo, InputPorts::Direction).GetNormalized()) :
                    GetPortVec3(pActInfo, InputPorts::Direction).GetNormalized();

                const AZ::Vector3 AZBegin = LYVec3ToAZVec3(pEntity->GetPos() + GetPortVec3(pActInfo, InputPorts::Position));
                const AZ::Vector3 AZDirection = LYVec3ToAZVec3(LYDirection);
                const float maxDistance = GetPortFloat(pActInfo, InputPorts::MaxLength);

                LmbrCentral::NavRayCastResult rayCastResult;
                EBUS_EVENT_RESULT(rayCastResult, LmbrCentral::NavigationSystemRequestBus, RayCast, AZBegin, AZDirection, maxDistance);

                if (rayCastResult.m_collision)
                {
                    ActivateOutput(pActInfo, OutputPorts::Hit, true);
                    ActivateOutput(pActInfo, OutputPorts::RayDirection, LYDirection);
                    ActivateOutput(pActInfo, OutputPorts::HitDistance, static_cast<float>((AZBegin - rayCastResult.m_position).GetLength()));
                    ActivateOutput(pActInfo, OutputPorts::HitPoint, AZVec3ToLYVec3(rayCastResult.m_position));
                    ActivateOutput(pActInfo, OutputPorts::MeshId, rayCastResult.m_meshId);
                }
                else
                {
                    // Setting this to false gives it the same functionality as the Physics RayCast NoHit output
                    ActivateOutput(pActInfo, OutputPorts::NoHit, false);
                }
            }
        }
    }
};


//////////////////////////////////////////////////////////////////////////
// Simply passes through the faction from the input port to the output port.
// Can be used to prevent typos when dealing with factions in the Flow Graph.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIFaction
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    enum
    {
        InTrigger,
        InFaction,
    };

    enum
    {
        OutFaction,
    };

    CFlowNode_AIFaction(SActivationInfo* pActInfo) {}

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inConfig[] =
        {
            InputPortConfig_Void("Trigger", _HELP("Triggers the 'Faction' output port.")),
            InputPortConfig<string>("Faction", "", _HELP("Faction to pass to the output port."), 0, "enum_global:Faction"),
            {0}
        };

        static const SOutputPortConfig outConfig[] =
        {
            OutputPortConfig<string>("Faction", _HELP("The faction that was picked via the input port.")),
            {0}
        };

        config.sDescription = _HELP("Simply passes the selected faction to the output port. This is more of a helper node to circumvent manual typing.");
        config.pInputPorts = inConfig;
        config.pOutputPorts = outConfig;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InTrigger))
            {
                const string& faction = GetPortString(pActInfo, InFaction);
                ActivateOutput(pActInfo, OutFaction, faction);
            }
            break;
        }
        }
        ;
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


REGISTER_FLOW_NODE("AI:ActiveCount", CFlowNode_AIActiveCount)
REGISTER_FLOW_NODE("AI:ActiveCountInFaction", CFlowNode_AIActiveCountInFaction)
REGISTER_FLOW_NODE("AI:ActiveCountMonitor", CFlowNode_AIActiveCountMonitor)
REGISTER_FLOW_NODE("AI:IgnoreState", CFlowNode_AIIgnore)
REGISTER_FLOW_NODE("AI:FactionReaction", CFlowNode_AIFactionReaction)
REGISTER_FLOW_NODE("AI:Faction", CFlowNode_AIFaction)
REGISTER_FLOW_NODE("AI:SetFaction", CFlowNode_AISetFaction)

REGISTER_FLOW_NODE("AI:ActionStart", CFlowNode_AIActionStart)
REGISTER_FLOW_NODE("AI:ActionEnd", CFlowNode_AIActionEnd)
REGISTER_FLOW_NODE("AI:ActionAbort", CFlowNode_AIActionAbort)
REGISTER_FLOW_NODE("AI:AttentionTarget", CFlowNode_AIAttTarget)
REGISTER_FLOW_NODE("AI:SmartObjectHelper", CFlowNode_AISOHelper)
REGISTER_FLOW_NODE("AI:GroupAlertness", CFlowNode_AIGroupAlertness)
REGISTER_FLOW_NODE("AI:GroupIDSet", CFlowNode_AIMergeGroups)
REGISTER_FLOW_NODE("AI:GroupIDGet", CFlowNode_AIGroupID)
REGISTER_FLOW_NODE("AI:GroupCount", CFlowNode_AIGroupCount)
REGISTER_FLOW_NODE("AI:SmartObjectEvent", CFlowNode_AISmartObjectEvent)
REGISTER_FLOW_NODE("AI:PerceptionScale", CFlowNode_AIPerceptionScale)
REGISTER_FLOW_NODE("AI:NavCostFactor", CFlowNode_AISetNavCostFactor)
REGISTER_FLOW_NODE("AI:AutoDisable", CFlowNode_AIAutoDisable)
REGISTER_FLOW_NODE("AI:ShapeState", CFlowNode_AIEnableShape)
REGISTER_FLOW_NODE("AI:EventListener", CFlowNode_AIEventListener)
REGISTER_FLOW_NODE("AI:Communication", CFlowNode_AICommunication)
REGISTER_FLOW_NODE("AI:IsAliveCheck", CFlowNode_AIIsAliveCheck)
REGISTER_FLOW_NODE("AI:AIGlobalPerceptionScaling", CFlowNode_AIGlobalPerceptionScaling)
REGISTER_FLOW_NODE("AI:SetCommunicationVariable", CFlowNode_AISetCommunicationVariable)
REGISTER_FLOW_NODE("AI:RequestReinforcementReadability", CFlowNode_AIRequestReinforcementReadability)
REGISTER_FLOW_NODE("AI:RegenerateMNM", CFlowNode_AIRegenerateMNM)
REGISTER_FLOW_NODE("AI:RayCastMNM", CFlowNode_AIRayCastMNM)

FLOW_NODE_BLACKLISTED("AI:AlertnessFilter", CFlowNode_AIAlertnessFilter)
FLOW_NODE_BLACKLISTED("AI:AIEnable", CFlowNode_AIEnable)
FLOW_NODE_BLACKLISTED("AI:Interested", CFlowNode_Interested)
FLOW_NODE_BLACKLISTED("AI:InterestingEntity", CFlowNode_InterestingEntity)
FLOW_NODE_BLACKLISTED("AI:AIGroupBeacon", CFlowNode_AIGroupBeacon)
FLOW_NODE_BLACKLISTED("AI:ChangeParameter", CFlowNode_AIChangeParameter)
FLOW_NODE_BLACKLISTED("Crysis:HunterShootTarget", CFlowNode_HunterShootTarget)
FLOW_NODE_BLACKLISTED("Crysis:HunterBeamSweep", CFlowNode_HunterBeamSweep)
