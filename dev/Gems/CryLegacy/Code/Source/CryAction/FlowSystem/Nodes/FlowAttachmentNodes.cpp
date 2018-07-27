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

class CFlowNode_Attachment
    : public CFlowEntityNodeBase
{
protected:
    IAttachment* m_pNewAttachment;
public:
    CFlowNode_Attachment(SActivationInfo* pActInfo)
    {
        m_event = ENTITY_EVENT_RESET;
    }

    enum EInPorts
    {
        eIP_EntityId,
        eIP_BoneName,
        eIP_CharacterSlot,
        eIP_Attach,
        eIP_Detach,
        eIP_Hide,
        eIP_UnHide,
        eIP_RotationOffset,
        eIP_TranslationOffset
    };

    enum EOutPorts
    {
        eOP_Attached,
        eOP_Detached
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<FlowEntityId>("Item", _HELP("Entity to be linked")),
            InputPortConfig<string>("BoneName", _HELP("Attachment bone")),
            InputPortConfig<int>("CharacterSlot", 0, _HELP("Host character slot")),
            InputPortConfig<SFlowSystemVoid>("Attach", _HELP("Attach")),
            InputPortConfig<SFlowSystemVoid>("Detach", _HELP("Detach any entity currently attached to the given bone")),
            InputPortConfig_Void("Hide", _HELP("Hides attachment")),
            InputPortConfig_Void("UnHide", _HELP("Unhides attachment")),
            InputPortConfig<Vec3>("RotationOffset", _HELP("Rotation offset")),
            InputPortConfig<Vec3>("TranslationOffset", _HELP("Translation offset")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<SFlowSystemVoid>("Attached"),
            OutputPortConfig<SFlowSystemVoid>("Detached"),
            {0}
        };
        config.sDescription = _HELP("Attach/Detach an entity to another one.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED | EFLN_TARGET_ENTITY);
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_Attachment(pActInfo); }

    virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        CFlowEntityNodeBase::ProcessEvent(event, pActInfo);
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, eIP_Attach))
            {
                AttachObject(pActInfo);
            }
            if (IsPortActive(pActInfo, eIP_Detach))
            {
                DetachObject(pActInfo);
            }

            if (IsPortActive(pActInfo, eIP_Hide))
            {
                HideAttachment(pActInfo);
            }
            else if (IsPortActive(pActInfo, eIP_UnHide))
            {
                UnHideAttachment(pActInfo);
            }

            if (IsPortActive(pActInfo, eIP_RotationOffset) || IsPortActive(pActInfo, eIP_TranslationOffset))
            {
                UpdateOffset(pActInfo);
            }
            break;
        }
        case eFE_Initialize:
            if (gEnv->IsEditor() && m_entityId)
            {
                UnregisterEvent(m_event);
                RegisterEvent(m_event);
                m_pNewAttachment = NULL;
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    IAttachment* GetAttachment(SActivationInfo* pActInfo)
    {
        IAttachment* pAttachment = NULL;
        if (pActInfo->pEntity)
        {
            int slot = GetPortInt(pActInfo, eIP_CharacterSlot);
            ICharacterInstance* pCharacter = pActInfo->pEntity->GetCharacter(slot);
            if (pCharacter)
            {
                const string& boneName = GetPortString(pActInfo, eIP_BoneName);
                IAttachmentManager* pAttachmentManager = pCharacter->GetIAttachmentManager();
                pAttachment = pAttachmentManager->GetInterfaceByName(boneName);
            }
        }
        return pAttachment;
    }

    void AttachObject(SActivationInfo* pActInfo)
    {
        if (pActInfo->pEntity)
        {
            FlowEntityId entityId = FlowEntityId(GetPortEntityId(pActInfo, eIP_EntityId));
            IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityId);
            if (pEntity)
            {
                //Keep track of the last attachment performed in order to properly reset the state in the end
                m_pNewAttachment = GetAttachment(pActInfo);
                if (m_pNewAttachment)
                {
                    CEntityAttachment* pEntityAttachment = new CEntityAttachment;
                    pEntityAttachment->SetEntityId(entityId);
                    m_pNewAttachment->AddBinding(pEntityAttachment);

                    UpdateOffset(pActInfo);
                    ActivateOutput(pActInfo, eOP_Attached, 0);
                }
            }
        }
    }

    void DetachObject(SActivationInfo* pActInfo)
    {
        IAttachment* pAttachment = GetAttachment(pActInfo);
        if (pAttachment)
        {
            pAttachment->ClearBinding();
            ActivateOutput(pActInfo, eOP_Detached, 0);
        }
    }

    void HideAttachment(SActivationInfo* pActInfo)
    {
        IAttachment* pAttachment = GetAttachment(pActInfo);
        if (pAttachment)
        {
            pAttachment->HideAttachment(1);

            IAttachmentObject* pAttachmentObject = pAttachment->GetIAttachmentObject();
            if ((pAttachmentObject != NULL) && (pAttachmentObject->GetAttachmentType() == IAttachmentObject::eAttachment_Entity))
            {
                IEntity* pAttachedEntity = gEnv->pEntitySystem->GetEntity(static_cast<CEntityAttachment*>(pAttachmentObject)->GetEntityId());
                if (pAttachedEntity)
                {
                    pAttachedEntity->Hide(true);
                }
            }
        }
    }

    void UnHideAttachment(SActivationInfo* pActInfo)
    {
        IAttachment* pAttachment = GetAttachment(pActInfo);
        if (pAttachment)
        {
            pAttachment->HideAttachment(0);

            IAttachmentObject* pAttachmentObject = pAttachment->GetIAttachmentObject();
            if ((pAttachmentObject != NULL) && (pAttachmentObject->GetAttachmentType() == IAttachmentObject::eAttachment_Entity))
            {
                IEntity* pAttachedEntity = gEnv->pEntitySystem->GetEntity(static_cast<CEntityAttachment*>(pAttachmentObject)->GetEntityId());
                if (pAttachedEntity)
                {
                    pAttachedEntity->Hide(false);
                }
            }
        }
    }

    void UpdateOffset(SActivationInfo* pActInfo)
    {
        IAttachment* pAttachment = GetAttachment(pActInfo);
        if (pAttachment)
        {
            const Vec3& rotationOffsetEuler = GetPortVec3(pActInfo, eIP_RotationOffset);
            QuatT offset = QuatT::CreateRotationXYZ(Ang3(DEG2RAD(rotationOffsetEuler)));
            offset.t = GetPortVec3(pActInfo, eIP_TranslationOffset);
            pAttachment->SetAttRelativeDefault(offset);
        }
    }

    void OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
    {
        if (event.event == m_event)
        {
            if (m_pNewAttachment)
            {
                m_pNewAttachment->ClearBinding();
            }
        }
    }
};

REGISTER_FLOW_NODE("Entity:Attachment", CFlowNode_Attachment);
