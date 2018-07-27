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
#include "BehaviorTreeNodes_Action.h"
#include "BehaviorTree/IBehaviorTree.h"
#include "BehaviorTree/Action.h"
#include "BehaviorTree/Decorator.h"
#include "IActorSystem.h"
#include "Mannequin/AnimActionTriState.h"
#include "XMLAttrReader.h"
#include <LmbrCentral/Animation/MannequinComponentBus.h>
#include <AIHandler.h>

namespace BehaviorTree
{
    // Play an animation fragment directly through Mannequin (start a fragment
    // for a specific FragmentID), and wait until it is done.
    class AnimateFragment
        : public Action
    {
    public:
        struct RuntimeData
        {
            // The action animation action that is running or nullptr if none.
            typedef _smart_ptr<CAnimActionTriState> CAnimActionTriStatePtr;
            CAnimActionTriStatePtr action;

            // The fragment's request id for use with the Mannequin component bus
            LmbrCentral::FragmentRequestId fragmentRequestId;

            // True if the action was queued; false if an error occured.
            bool actionQueuedFlag;

            RuntimeData()
                : actionQueuedFlag(false)
                , fragmentRequestId(0)
            {
            }

            ~RuntimeData()
            {
                ReleaseAction();
            }

            void ReleaseAction()
            {
                if (this->action.get() != nullptr)
                {
                    this->action->ForceFinish();
                    this->action.reset();
                }
            }
        };

        AnimateFragment()
            : m_fragNameCrc32(0)
#ifdef USING_BEHAVIOR_TREE_NODE_CUSTOM_DEBUG_TEXT
            , m_fragName()
#endif
        {
        }

        virtual LoadResult LoadFromXml(const XmlNodeRef& xml, const LoadContext& context) override
        {
            const string fragName = xml->getAttr("name");
            IF_UNLIKELY (fragName.empty())
            {
                ErrorReporter(*this, context).LogError("Missing attribute 'name'.");
                return LoadFailure;
            }
#ifdef USING_BEHAVIOR_TREE_NODE_CUSTOM_DEBUG_TEXT
            m_fragName = fragName;
#endif
            m_fragNameCrc32 = CCrc32::ComputeLowercase(fragName);

            return LoadSuccess;
        }


#ifdef USING_BEHAVIOR_TREE_NODE_CUSTOM_DEBUG_TEXT
        virtual void GetCustomDebugText(const UpdateContext& updateContext, stack_string& debugText) const override
        {
            debugText = m_fragName;
        }
#endif


    protected:

        virtual void OnInitialize(const UpdateContext& context) override
        {
            QueueAction(context);
        }


        virtual void OnTerminate(const UpdateContext& context) override
        {
            RuntimeData& runtimeData = GetRuntimeData<RuntimeData>(context);
            runtimeData.ReleaseAction();
        }


        virtual Status Update(const UpdateContext& context) override
        {
            RuntimeData& runtimeData = GetRuntimeData<RuntimeData>(context);

            IF_UNLIKELY (!runtimeData.actionQueuedFlag)
            {
                return Failure;
            }
            IAction::EStatus currentActionStatus = IAction::None;
            if (runtimeData.action.get())
            {
                currentActionStatus = runtimeData.action->GetStatus();
            }
            else
            {
                EBUS_EVENT_ID_RESULT(currentActionStatus, context.entityId, LmbrCentral::MannequinRequestsBus, GetRequestStatus, runtimeData.fragmentRequestId);
            }
            IF_UNLIKELY ((currentActionStatus == IAction::None) || (currentActionStatus == IAction::Finished))
            {
                return Success;
            }

            return Running;
        }


    private:

        void QueueAction(const UpdateContext& context)
        {
            RuntimeData& runtimeData = GetRuntimeData<RuntimeData>(context);

            if (IActor* actor = CCryAction::GetCryAction()->GetIActorSystem()->GetActor(GetLegacyEntityId(context.entityId)))
            {
                if (IAnimatedCharacter* animChar = actor->GetAnimatedCharacter())
                {
                    IF_LIKELY (runtimeData.action.get() == nullptr)
                    {
                        IActionController* pIActionController = animChar->GetActionController();
                        const FragmentID fragID = pIActionController->GetFragID(m_fragNameCrc32);
                        IF_LIKELY (fragID == FRAGMENT_ID_INVALID)
                        {
        #ifdef USING_BEHAVIOR_TREE_NODE_CUSTOM_DEBUG_TEXT
                            ErrorReporter(*this, context).LogError("Invalid fragment name '%s'", m_fragName);
        #else
                            ErrorReporter(*this, context).LogError("Invalid fragment name!");
        #endif
                            return;
                        }
                        runtimeData.action = new CAnimActionTriState(CAnimActionAIAction::NORMAL_ACTION_PRIORITY, fragID, *animChar, true /*oneshot*/);
                        pIActionController->Queue(*runtimeData.action);
                    }
                }
            }
            else
            {
                IActionController* pIActionController = nullptr;
                EBUS_EVENT_ID_RESULT(pIActionController, context.entityId, LmbrCentral::MannequinRequestsBus, GetActionController);
                if (pIActionController)
                {
                    const FragmentID fragID = pIActionController->GetFragID(m_fragNameCrc32);
                    if (fragID != FRAGMENT_ID_INVALID)
                    {
                        const char* emptyTags = nullptr;
                        EBUS_EVENT_ID_RESULT(runtimeData.fragmentRequestId, context.entityId, LmbrCentral::MannequinRequestsBus, QueueFragmentById, CAnimActionAIAction::NORMAL_ACTION_PRIORITY, fragID, emptyTags, false /*oneshot*/);
                    }
                }
            }
            runtimeData.actionQueuedFlag = true;
        }

    private:

        // The CRC value for the fragment name.
        uint32 m_fragNameCrc32;

#ifdef USING_BEHAVIOR_TREE_NODE_CUSTOM_DEBUG_TEXT
        // The actual fragment name.
        string m_fragName;
#endif
    };
}



void RegisterActionBehaviorTreeNodes()
{
    using namespace BehaviorTree;

    assert(gEnv->pAISystem->GetIBehaviorTreeManager());

    IBehaviorTreeManager& manager = *gEnv->pAISystem->GetIBehaviorTreeManager();

    REGISTER_BEHAVIOR_TREE_NODE(manager, AnimateFragment);
}
