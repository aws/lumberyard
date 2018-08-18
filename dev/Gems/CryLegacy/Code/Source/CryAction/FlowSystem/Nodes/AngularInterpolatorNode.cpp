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
#include <ISystem.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>

class CRotateTo_Node
    : public CFlowBaseNode<eNCT_Instanced>
{
    Vec3 m_currentRotation;
    Vec3 m_destinationRotation;
    Vec3 m_currentRotationStep; // per ms
    float m_lastTime;
    float m_timeRemaining;          // in ms


public:
    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    CRotateTo_Node(SActivationInfo* pActInfo)
        : m_currentRotation(ZERO)
        , m_destinationRotation(ZERO)
        , m_currentRotationStep(ZERO)
        , m_lastTime(0.0f)
        , m_timeRemaining(0.0f)
    {
    };

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CRotateTo_Node(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser)
    {
        ser.BeginGroup("Local");
        ser.Value("m_currentRotation", m_currentRotation);
        ser.Value("m_destinationRotation", m_destinationRotation);
        ser.Value("m_currentRotationStep", m_currentRotationStep);
        ser.Value("m_lastTime", m_lastTime);
        ser.Value("m_timeRemaining", m_timeRemaining);
        // the regular update is taken care of by the FlowGraph itself
        ser.EndGroup();
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<Vec3>("Destination", _HELP("Destination")),
            InputPortConfig<float>("Time", _HELP("Duration in seconds")),
            InputPortConfig<bool>("Trigger", _HELP("Starts the rotation when triggered")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<Vec3>("Current", _HELP("Current Rotation")),
            OutputPortConfig<bool>("Done", _HELP("Goes to TRUE when the destination is reached")),
            {0}
        };
        config.sDescription = _HELP("Rotates between two angles during a defined period of time");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_OBSOLETE);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (!IsPortActive(pActInfo, 2))
            {
                return;
            }

            m_lastTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
            m_timeRemaining = GetPortFloat(pActInfo, 1) * 1000.0f;
            m_destinationRotation = GetPortVec3(pActInfo, 0);

            // snap angles
            m_destinationRotation.x = Snap_s360(m_destinationRotation.x);
            m_destinationRotation.y = Snap_s360(m_destinationRotation.y);
            m_destinationRotation.z = Snap_s360(m_destinationRotation.z);

            m_currentRotation.x = Snap_s360(m_currentRotation.x);
            m_currentRotation.y = Snap_s360(m_currentRotation.y);
            m_currentRotation.z = Snap_s360(m_currentRotation.z);


            // do we have to do anything.
            if (m_timeRemaining >= 0.0f)
            {
                float x = Snap_s360(m_destinationRotation.x - m_currentRotation.x);
                float y = Snap_s360(m_destinationRotation.y - m_currentRotation.y);
                float z = Snap_s360(m_destinationRotation.z - m_currentRotation.z);

                m_currentRotationStep.x = ((x < 360.0f - x) ? x : x - 360.0f) / m_timeRemaining;
                m_currentRotationStep.y = ((y < 360.0f - y) ? y : y - 360.0f) / m_timeRemaining;
                m_currentRotationStep.z = ((z < 360.0f - z) ? z : z - 360.0f) / m_timeRemaining;
            }
            else
            {
                m_currentRotationStep.x = 0.0f;
                m_currentRotationStep.y = 0.0f;
                m_currentRotationStep.z = 0.0f;
            }
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            break;
        }

        case eFE_Initialize:
        {
            m_lastTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
            m_timeRemaining = GetPortFloat(pActInfo, 1);
            m_currentRotation = GetPortVec3(pActInfo, 0);
            m_destinationRotation = GetPortVec3(pActInfo, 0);
            m_currentRotationStep.x = 0.0f;
            m_currentRotationStep.y = 0.0f;
            m_currentRotationStep.z = 0.0f;
            ActivateOutput(pActInfo, 0, m_currentRotation);
            ActivateOutput(pActInfo, 1, false);
            break;
        }

        case eFE_Update:
        {
            // let's compute the movement now
            float time = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
            float timeDifference = time - m_lastTime;
            m_lastTime = time;
            if (m_timeRemaining - timeDifference <= 0.0f)
            {
                m_timeRemaining = 0.0f;
                m_currentRotation = m_destinationRotation;
                ActivateOutput(pActInfo, 1, true);
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            }
            else
            {
                m_timeRemaining -= timeDifference;
                m_currentRotation += m_currentRotationStep * timeDifference;
            }
            ActivateOutput(pActInfo, 0, m_currentRotation);
            break;
        }
        }
        ;
    };
};

// Todo Ly : This is not as the category suggests a Movement node in the strictest sense
// This is more of a math utility that outputs values that can be used to affect
// Movement , this should be moved to math or a new category should be made
REGISTER_FLOW_NODE("Movement:RotateTo", CRotateTo_Node);
