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


class CMoveTo_Node
    : public CFlowBaseNode<eNCT_Instanced>
{
    Vec3 m_position;
    Vec3 m_destination;
    float m_lastTime;
    float m_topSpeed;
    float m_easeDistance;

public:
    CMoveTo_Node(SActivationInfo* pActInfo)
        :   m_position(ZERO)
        , m_destination(ZERO)
        , m_lastTime(0.0f)
        , m_topSpeed(0.0f)
        , m_easeDistance(0.0f)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CMoveTo_Node(pActInfo);
    }

    virtual void Serialize(SActivationInfo*, TSerialize ser)
    {
        ser.BeginGroup("Local");
        ser.Value("m_position", m_position);
        ser.Value("m_destination", m_destination);
        ser.Value("m_lastTime", m_lastTime);
        ser.Value("m_topSpeed", m_topSpeed);
        ser.Value("m_easeDistance", m_easeDistance);
        ser.EndGroup();
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<Vec3>("Destination", _HELP("Destination")),
            InputPortConfig<float>("Speed", _HELP("speed (m/sec)")),
            InputPortConfig<float>("EaseDistance", _HELP("distance at which the speed is changed (in meters)")),
            InputPortConfig<bool>("Trigger", _HELP("Starts the motion when triggered")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<Vec3>("Current", _HELP("Current position")),
            OutputPortConfig<bool>("Done", _HELP("Goes to TRUE when the destination is reached")),
            {0}
        };
        config.sDescription = _HELP("Move between two positions at a defined speed");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_OBSOLETE);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        // to make eFE_Update get called:
        //pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
        // to turn it off:
        //pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );

        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, 3))
            {
                // start interpolation
                IEntity* pEnt = pActInfo->pEntity;
                if (pEnt)
                {
                    m_position = pEnt->GetPos();
                }
                m_lastTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
                m_destination = GetPortVec3(pActInfo, 0);
                m_topSpeed = GetPortFloat(pActInfo, 1);
                m_easeDistance = GetPortFloat(pActInfo, 2);
                if (m_topSpeed < 0.0f)
                {
                    m_topSpeed = 0.0f;
                }
                if (m_easeDistance < 0.0f)
                {
                    m_easeDistance = 0.0f;
                }

                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
                ActivateOutput(pActInfo, 1, false);
            }
            break;
        }

        case eFE_Initialize:
        {
            m_position = GetPortVec3(pActInfo, 0);
            m_destination = m_position;
            m_topSpeed = 0.0f;
            m_lastTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
            ActivateOutput(pActInfo, 0, m_position);
            ActivateOutput(pActInfo, 1, false);
            break;
        }

        case eFE_Update:
        {
            float time = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
            float timeDifference = time - m_lastTime;
            m_lastTime = time;

            // let's compute the movement vector now
            if (m_position.IsEquivalent(m_destination))
            {
                m_position = m_destination;
                ActivateOutput(pActInfo, 1, true);
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            }
            else
            {
                Vec3 direction = m_destination - m_position;
                float distance = direction.GetLength();
                Vec3 directionAndSpeed = direction.normalized();
                if (distance < m_easeDistance)      // takes care of m_easeDistance being 0
                {
                    directionAndSpeed *= distance / m_easeDistance;
                }
                directionAndSpeed *= (m_topSpeed * timeDifference) / 1000.0f;

                if (direction.GetLength() < directionAndSpeed.GetLength())
                {
                    m_position = m_destination;
                }
                else
                {
                    m_position += directionAndSpeed;
                }
            }
            ActivateOutput(pActInfo, 0, m_position);
            break;
        }
        }
        ;
    };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

// Todo Ly : This is not as the category suggests a Movement node in the strictest sense
// This is more of a math utility that outputs values that can be used to affect
// Movement , this should be moved to math or a new category should be made
REGISTER_FLOW_NODE("Movement:MoveTo", CMoveTo_Node);
