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
#include "Components/ComponentEntityNode.h"
#include "CryCharAnimationParams.h"
#include "IMaterialEffects.h"
#include "ICryAnimation.h"
#include "Components/IComponentSerialization.h"

DECLARE_DEFAULT_COMPONENT_FACTORY(CComponentEntityNode, CComponentEntityNode)

void CComponentEntityNode::Initialize(const SComponentInitializer& init)
{
    m_pEntity = init.m_pEntity;
    m_pEntity->GetComponent<IComponentSerialization>()->Register<CComponentEntityNode>(SerializationOrder::EntityNode, *this, &CComponentEntityNode::Serialize, &CComponentEntityNode::SerializeXML, &CComponentEntityNode::NeedSerialize, &CComponentEntityNode::GetSignature);
}

void CComponentEntityNode::ProcessEvent(SEntityEvent& event)
{
    switch (event.event)
    {
    case ENTITY_EVENT_ANIM_EVENT:
        const AnimEventInstance* pAnimEvent = reinterpret_cast<const AnimEventInstance*>(event.nParam[0]);
        ICharacterInstance* pCharacter = reinterpret_cast<ICharacterInstance*>(event.nParam[1]);

        if (pAnimEvent)
        {
            if (pAnimEvent->m_EventName && _stricmp(pAnimEvent->m_EventName, "footstep") == 0)
            {
                QuatT jointTransform(IDENTITY);

                if (pAnimEvent->m_BonePathName[0] && pCharacter)
                {
                    uint32 boneID = pCharacter->GetIDefaultSkeleton().GetJointIDByName(pAnimEvent->m_BonePathName);
                    jointTransform = pCharacter->GetISkeletonPose()->GetAbsJointByID(boneID);
                }

                // Setup FX params
                SMFXRunTimeEffectParams params;
                params.audioComponentEntityId = m_pEntity->GetId();
                params.angle = m_pEntity->GetWorldAngles().z + (gf_PI * 0.5f);

                params.pos = params.decalPos = m_pEntity->GetSlotWorldTM(0) * jointTransform.t;

                // Audio: set semantic for audio object
                params.audioComponentOffset = m_pEntity->GetWorldTM().GetInverted().TransformVector(params.pos - m_pEntity->GetWorldPos());

                IMaterialEffects* pMaterialEffects = gEnv->pMaterialEffects;
                TMFXEffectId effectId = InvalidEffectId;

                // note: for some reason material libraries are named "footsteps" when queried by name
                // and "footstep" when queried by ID
                if (pMaterialEffects)
                {
                    if (pAnimEvent->m_CustomParameter[0])
                    {
                        effectId = pMaterialEffects->GetEffectIdByName("footstep", pAnimEvent->m_CustomParameter);
                    }
                    else
                    {
                        effectId = pMaterialEffects->GetEffectIdByName("footstep", "default");
                    }
                }

                if (effectId != InvalidEffectId)
                {
                    pMaterialEffects->ExecuteEffect(effectId, params);
                }
                else
                {
                    gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "Failed to find material for footstep sounds");
                }
            }
        }
    }
}
