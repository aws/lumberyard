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
#include "CryLegacy_precompiled.h"
#include "UiFlow.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <LyShine/ILyShine.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Animation/IUiAnimation.h>

#define FLOW_UI_SEQUENCE_NODE_COMMON_INPUT_PORT_CONFIGS                 \
    InputPortConfig_Void("Start", _HELP("Start playing the sequence")), \
    InputPortConfig_Void("Stop", _HELP("Stop playing the sequence")),   \
    InputPortConfig_Void("Abort", _HELP("Abort the sequence")),         \
    InputPortConfig_Void("Pause", _HELP("Pause the sequence")),         \
    InputPortConfig_Void("Resume", _HELP("Resume the sequence")),       \
    InputPortConfig_Void("Reset", _HELP("Reset the sequence to the start"))

#define FLOW_UI_SEQUENCE_NODE_COMMON_INPUT_OFFSET_PORT_CONFIGS \
    InputPortConfig<string>("SequenceName", _HELP("The name of the sequence to play"))

namespace
{
    const string g_sequencePlayNodePath = "UI:Sequence:Play";
    const string g_entSequencePlayNodePath = "UIe:Sequence:Play";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiSequencePlayBaseNode class
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Flow Graph node for playing a UI animation sequence
class CFlowUiSequencePlayBaseNode
    : public CFlowBaseNode < eNCT_Instanced >
    , public IUiAnimationListener
{
public:
    CFlowUiSequencePlayBaseNode(SActivationInfo* activationInfo, bool hasTargetEntity)
        : CFlowBaseNode()
        , m_sequence(nullptr)
        , m_hasTargetEntity(hasTargetEntity)
    {}

    ~CFlowUiSequencePlayBaseNode() override
    {
        UnregisterListener();
    }

    //-- IFlowNode --

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            FLOW_UI_SEQUENCE_NODE_COMMON_INPUT_PORT_CONFIGS,
            InputPortConfig<int>("CanvasID", _HELP("The canvas ID of canvas containing the sequence")),
            FLOW_UI_SEQUENCE_NODE_COMMON_INPUT_OFFSET_PORT_CONFIGS,
            {0}
        };

        static const SInputPortConfig entInputs[] =
        {
            FLOW_UI_SEQUENCE_NODE_COMMON_INPUT_PORT_CONFIGS,
            FLOW_UI_SEQUENCE_NODE_COMMON_INPUT_OFFSET_PORT_CONFIGS,
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("OnStarted", _HELP("Sends a signal when the sequence sucesfully starts")),
            OutputPortConfig_Void("OnStopped", _HELP("Sends a signal when the sequence stops normally")),
            OutputPortConfig_Void("OnAborted", _HELP("Sends a signal when the sequence stops abnormally")),
            {0}
        };

        config.pInputPorts = m_hasTargetEntity ? entInputs : inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Play a sequence in the given canvas");
        config.SetCategory(EFLN_APPROVED);
        if (m_hasTargetEntity)
        {
            config.nFlags |= EFLN_TARGET_ENTITY;
        }
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        if (eFE_Activate == event && gEnv->pLyShine)
        {
            // If this node was already triggered, remove it as a listener
            // so it doesn't get other random callbacks
            UnregisterListener();

            const string& nodePath = m_hasTargetEntity ? g_entSequencePlayNodePath : g_sequencePlayNodePath;

            // Get canvas entity Id
            AZ::EntityId canvasEntityId = UiFlow::GetCanvasEntityId(activationInfo,
                    (m_hasTargetEntity ? -1 : InputPortCanvasId), nodePath);
            if (canvasEntityId.IsValid())
            {
                int inputPortOffsetStart = (m_hasTargetEntity ? InputPortCanvasId : InputPortCanvasId + 1);

                // Play the sequence
                string sequenceName = GetPortString(activationInfo, inputPortOffsetStart + InputPortOffsetSequenceName);

                IUiAnimationSystem* animSystem = nullptr;
                EBUS_EVENT_ID_RESULT(animSystem, canvasEntityId, UiCanvasBus, GetAnimationSystem);
                if (animSystem)
                {
                    IUiAnimSequence* pSequence = animSystem->FindSequence(sequenceName);
                    if (pSequence)
                    {
                        // add listener before calling functions like PlaySequence since that can
                        // trigger immediate events on the listener
                        if (!m_canvasEntityId.IsValid() || !m_sequence)
                        {
                            m_canvasEntityId = canvasEntityId;
                            m_actInfo = *activationInfo;
                            m_sequence = pSequence;
                            animSystem->AddUiAnimationListener(pSequence, this);
                        }

                        if (IsPortActive(activationInfo, InputPortStart))
                        {
                            animSystem->PlaySequence(pSequence, nullptr, false, false);
                        }
                        else if (IsPortActive(activationInfo, InputPortStop))
                        {
                            animSystem->StopSequence(pSequence);
                        }
                        else if (IsPortActive(activationInfo, InputPortAbort))
                        {
                            animSystem->AbortSequence(pSequence);
                        }
                        else if (IsPortActive(activationInfo, InputPortPause))
                        {
                            pSequence->Pause();
                        }
                        else if (IsPortActive(activationInfo, InputPortResume))
                        {
                            pSequence->Resume();
                        }
                        else if (IsPortActive(activationInfo, InputPortReset))
                        {
                            pSequence->Reset(true);
                        }
                    }
                    else
                    {
                        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                            "FlowGraph: %s Node: couldn't find UI animation sequence with name: %s\n",
                            nodePath.c_str(), sequenceName.c_str());
                    }
                }
            }
        }
    }

    void OnUiAnimationEvent(EUiAnimationEvent uiAnimationEvent, IUiAnimSequence* pAnimSequence) override
    {
        switch (uiAnimationEvent)
        {
        case eUiAnimationEvent_Started:
            ActivateOutput(&m_actInfo, OutputPortOnStarted, 0);
            break;

        case eUiAnimationEvent_Stopped:
            ActivateOutput(&m_actInfo, OutputPortOnStopped, 0);
            break;

        case eUiAnimationEvent_Aborted:
            ActivateOutput(&m_actInfo, OutputPortOnAborted, 0);
            break;
        }
    }

    //-- ~IFlowNode --

protected:
    //! Unregister UiAnimationListener
    void UnregisterListener()
    {
        if (m_canvasEntityId.IsValid() && m_sequence)
        {
            IUiAnimationSystem* animSystem = nullptr;
            EBUS_EVENT_ID_RESULT(animSystem, m_canvasEntityId, UiCanvasBus, GetAnimationSystem);
            if (animSystem)
            {
                // Stop listening for actions
                animSystem->RemoveUiAnimationListener(m_sequence, this);
            }
        }

        m_canvasEntityId.SetInvalid();
        m_sequence = nullptr;
    }

    enum InputPorts
    {
        InputPortStart = 0,
        InputPortStop,
        InputPortAbort,
        InputPortPause,
        InputPortResume,
        InputPortReset,
        InputPortCanvasId
    };

    enum InputOffsetPorts
    {
        InputPortOffsetSequenceName = 0
    };

    enum OutputPorts
    {
        OutputPortOnStarted = 0,
        OutputPortOnStopped,
        OutputPortOnAborted,
    };

    //! The canvas this node is registered with, if any
    AZ::EntityId m_canvasEntityId;

    //! Store the activation info for activating outputs
    SActivationInfo m_actInfo;

    //! The sequence being controled
    IUiAnimSequence* m_sequence;

    //! Specifies whether the node contains a canvas ID port or a target entity port
    bool m_hasTargetEntity;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiSequencePlayNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiSequencePlayNode
    : public CFlowUiSequencePlayBaseNode
{
public:
    CFlowUiSequencePlayNode(SActivationInfo* activationInfo)
        : CFlowUiSequencePlayBaseNode(activationInfo, false)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiSequencePlayNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlowUiEntSequencePlayNode class
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowUiEntSequencePlayNode
    : public CFlowUiSequencePlayBaseNode
{
public:
    CFlowUiEntSequencePlayNode(SActivationInfo* activationInfo)
        : CFlowUiSequencePlayBaseNode(activationInfo, true)
    {
    }

    //-- IFlowNode --

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new CFlowUiEntSequencePlayNode(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    //-- ~IFlowNode --
};

REGISTER_FLOW_NODE(g_sequencePlayNodePath, CFlowUiSequencePlayNode);
REGISTER_FLOW_NODE(g_entSequencePlayNodePath, CFlowUiEntSequencePlayNode);
