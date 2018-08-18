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
#include "Components/IComponentRender.h"

class CComputeStaticShadows_Node
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CComputeStaticShadows_Node(SActivationInfo* pActInfo)
    {
    }

    enum EInputs
    {
        eI_Trigger = 0,
        eI_Min,
        eI_Max,
        eI_NextCascadeScale
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_Void("Trigger", _HELP("Trigger this to recompute all cached shadows")),
            InputPortConfig<Vec3>("Min", Vec3(ZERO), _HELP("Minimum position of shadowed area")),
            InputPortConfig<Vec3>("Max", Vec3(ZERO), _HELP("Maximum position of shadowed area")),
            InputPortConfig<float>("NextCascadesScale", 2.f, _HELP("Scale factor for subsequent cached shadow cascades")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            {0}
        };
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Triggers recalculation of cached shadow maps");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        EFlowEvent eventType = event;

        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, eI_Trigger))
            {
                pActInfo->pGraph->RequestFinalActivation(pActInfo->myID);
            }

            break;
        }

        case eFE_FinalActivate:
        {
            Vec3 minCorner = GetPortVec3(pActInfo, eI_Min);
            Vec3 maxCorner = GetPortVec3(pActInfo, eI_Max);
            float nextCascadeScale = GetPortFloat(pActInfo, eI_NextCascadeScale);

            gEnv->p3DEngine->SetCachedShadowBounds(AABB(minCorner, maxCorner), nextCascadeScale);
            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


class CPerEntityShadow_Node
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    CPerEntityShadow_Node(SActivationInfo* pActInfo)
    {
    }

    enum EInputs
    {
        eI_Enable = 0,
        eI_Trigger,
        eI_ConstBias,
        eI_SlopeBias,
        eI_Jittering,
        eI_BBoxScale,
        eI_ShadowMapSize,
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig<bool> ("Enabled", true, "Enables node", "Enabled"),
            InputPortConfig_Void("Trigger", _HELP("Update the engine")),
            InputPortConfig<float>("ConstBias", 0.001f, _HELP("Constant Bias")),
            InputPortConfig<float>("SlopeBias", 0.01f, _HELP("Slope Bias")),
            InputPortConfig<float>("Jittering", 0.01f, _HELP("Jittering")),
            InputPortConfig<Vec3>("BBoxScale", Vec3(1, 1, 1), _HELP("Object Bounding box scale")),
            InputPortConfig<int>("ShadowMapSize", 1024, _HELP("Shadow Map size")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            {0}
        };
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Per Entity Shadows");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        EFlowEvent eventType = event;

        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, eI_Trigger) && gEnv->pAISystem->IsEnabled())
            {
                pActInfo->pGraph->RequestFinalActivation(pActInfo->myID);
            }

            break;
        }

        case eFE_FinalActivate:
        {
            if (!pActInfo->pEntity)
            {
                break;
            }

            if (pActInfo->pEntity->GetId() != INVALID_ENTITYID)
            {
                IEntity* pEntity = gEnv->pEntitySystem->GetEntity(pActInfo->pEntity->GetId());
                if (pEntity && pEntity->GetComponent<IComponentRender>())
                {
                    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
                    if (IRenderNode* pRenderNode = pRenderComponent->GetRenderNode())
                    {
                        const bool bActivate = GetPortBool(pActInfo, eI_Enable);
                        if (bActivate)
                        {
                            float fConstBias = GetPortFloat(pActInfo, eI_ConstBias);
                            float fSlopeBias = GetPortFloat(pActInfo, eI_SlopeBias);
                            float fJittering = GetPortFloat(pActInfo, eI_Jittering);
                            Vec3 vBBoxScale  = GetPortVec3(pActInfo, eI_BBoxScale);
                            uint nShadowMapSize = GetPortInt(pActInfo, eI_ShadowMapSize);

                            gEnv->p3DEngine->AddPerObjectShadow(pRenderNode, fConstBias, fSlopeBias, fJittering, vBBoxScale, nShadowMapSize);
                        }
                        else
                        {
                            gEnv->p3DEngine->RemovePerObjectShadow(pRenderNode);
                        }
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

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CPerEntityShadow_Node(pActInfo); }
};

REGISTER_FLOW_NODE("Environment:RecomputeStaticShadows", CComputeStaticShadows_Node);
REGISTER_FLOW_NODE("Environment:PerEntityShadows", CPerEntityShadow_Node);
