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

#include <CryExtension/Impl/ClassWeaver.h>
#include <Mannequin/Serialization.h>

#include <ICryMannequin.h>
#include <ICryMannequinEditor.h>

#include <ParticleParams.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <LmbrCentral/Rendering/ParticleComponentBus.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MKS 20140920: Am I reading this right?  the entries in this struct are to match how the order in FragmentTrack.cpp:CProcClipKey::LoadProcDefs() has it...
// but there some of the entries are optional because they're conditionally read in from xml?!  That means as soon as someone omits/adds something extra in an
// xml, engine is broken.  I'm taking about Scripts/Mannequin/ProcDefs.xml
struct SPlayParticleEffectParams
    : public IProceduralParams
{
    SPlayParticleEffectParams()
        : posOffset(ZERO)
        , rotOffset(ZERO)
        , ignoreRotation(false)
        , emitterFollows(true)
        , cloneAttachment(false)
        , killOnExit(false)
        , keepEmitterActive(false)
    {
    }

    void Serialize(Serialization::IArchive& ar) override
    {
        ar(effectName, "EffectName", "Effect Name");
        ar(Serialization::Decorators::JointName<SProcDataCRC>(jointName), "JointName", "Joint Name");
        ar(Serialization::Decorators::AttachmentName<SProcDataCRC>(attachmentName), "AttachmentName", "Attachment Name");
        ar(posOffset, "PosOffset", "Position Offset");
        ar(rotOffset, "RotOffset", "Rotation Offset");
        ar(ignoreRotation, "IgnoreRotation", "Ignore Rotation");
        ar(emitterFollows, "EmitterFollows", "Emitter Follows");
        ar(cloneAttachment, "CloneAttachment", "Clone Attachment");
        ar(killOnExit, "KillOnExit", "Kill on Exit");
        ar(keepEmitterActive, "KeepEmitterActive", "Keep Emitter Active");
    }

    void GetExtraDebugInfo(StringWrapper& extraInfoOut) const override
    {
        extraInfoOut = effectName.c_str();
    }

    SEditorCreateTransformGizmoResult OnEditorCreateTransformGizmo(IEntity& entity) override
    {
        return SEditorCreateTransformGizmoResult(entity, QuatT(posOffset, Quat(rotOffset)), jointName.crc ? jointName.crc : attachmentName.crc);
    }

    void OnEditorMovedTransformGizmo(const QuatT& gizmoLocation) override
    {
        posOffset = gizmoLocation.t;
        rotOffset = Ang3(gizmoLocation.q);
    }

    TProcClipString effectName;
    SProcDataCRC jointName;
    SProcDataCRC attachmentName;
    Vec3 posOffset;
    Ang3 rotOffset;
    bool ignoreRotation;
    bool emitterFollows;
    bool cloneAttachment;   //< Clone an attachment from the specified bone (so as to leave any existing attachment intact)
    bool killOnExit;        //< Kills the emitter and particles when leaving the clip
    bool keepEmitterActive; //< Keeps the emission of new particles when leaving the clip
};

static unsigned int ToFlags(const SPlayParticleEffectParams& params)
{
    unsigned int flags = 0;
    if (params.ignoreRotation)
    {
        flags |= ePEF_IgnoreRotation;
    }
    if (!params.emitterFollows)
    {
        flags |= ePEF_NotAttached;
    }
    return flags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct SParticleEffectClipData
{
    SParticleEffectClipData()
        : pEmitter(nullptr)
        , ownedAttachmentNameCRC(0)
        , slot(-1)
    {}

    _smart_ptr<IParticleEmitter> pEmitter;
    uint32                       ownedAttachmentNameCRC;
    int                          slot;
};

class CParticleEffectContext
    : public IProceduralContext
{
private:

    typedef IProceduralContext BaseClass;

    struct SStopRequest
    {
        SStopRequest(SParticleEffectClipData& _data, EntityId _entityId, ICharacterInstance* _pCharInst)
            : pCharInst(_pCharInst)
            , data(_data)
            , entityId(_entityId)
            , isFinished(false)
        {}

        static bool HasFinished(const SStopRequest& input) { return input.isFinished; }

        _smart_ptr<ICharacterInstance> pCharInst;
        SParticleEffectClipData        data;
        EntityId                       entityId;   //< EntityId owning this request.
        bool                           isFinished; //< The request has already been processed.
    };

public:
    PROCEDURAL_CONTEXT(CParticleEffectContext, "ParticleEffectContext", 0x9CED30805F8845A3, 0xF527229C51B542A8);

    void Update(float timePassed) override;

    void StartEffect(SParticleEffectClipData& data, const SPlayParticleEffectParams& params, IScope* scope);
    void StopEffect(SParticleEffectClipData& data, const SPlayParticleEffectParams& params, IScope* scope);

    void StartEffect(SParticleEffectClipData& data, const SPlayParticleEffectParams& params, IScope* scope, const AZ::EntityId& entityId);
    void StopEffect(SParticleEffectClipData& data, const SPlayParticleEffectParams& params, IScope* scope, const AZ::EntityId& entityId);

private:

    bool IsEffectAlive(const SStopRequest& request) const;
    void ShutdownEffect(SStopRequest& request);

private:

    typedef std::vector< SStopRequest > TStopRequestVector;
    TStopRequestVector m_stopRequests;
};

CRYREGISTER_CLASS(CParticleEffectContext);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CProceduralClipParticleEffect
    : public TProceduralContextualClip<CParticleEffectContext, SPlayParticleEffectParams>
{
public:
    CProceduralClipParticleEffect()
    {
    }

    void OnEnter(float blendTime, float duration, const SPlayParticleEffectParams& params) override
    {
        if (gEnv->IsEditor() && gEnv->pGame->GetIGameFramework()->GetMannequinInterface().IsSilentPlaybackMode())
        {
            return;
        }

        if (gEnv->IsDedicated())
        {
            return;
        }

        if (m_scope->GetEntityId())
        {
            m_context->StartEffect(m_data, params, m_scope);
        }
        else
        {
            // In order to play the particle effect, we shall create a new game entity and manually add a ParticleComponent to it.
            // Then we can send bus events to this new entity and have it handle the particle lifetime.
            static AZ::u32 s_procClipIdNext = 0;
            auto name = AZStd::string::format("ProcClip_ParticleEffect_%d", s_procClipIdNext++);

            if (AZ::Entity* childEntity = new AZ::Entity(name.c_str()))
            {
                m_particleEntityId = childEntity->GetId();
                
                childEntity->SetRuntimeActiveByDefault(false);

                EBUS_EVENT(AzFramework::GameEntityContextRequestBus, AddGameEntity, childEntity);

                childEntity->CreateComponent<AzFramework::TransformComponent>();
                childEntity->CreateComponent("{65BC817A-ABF6-440F-AD4F-581C40F92795}");     // HACK!  ParticleComponent Uuid
            }

            EBUS_EVENT(AzFramework::GameEntityContextRequestBus, ActivateGameEntity, m_particleEntityId);
            EBUS_EVENT_ID(m_particleEntityId, AZ::TransformBus, SetParentRelative, m_scope->GetAzEntityId());

            m_context->StartEffect(m_data, params, m_scope, m_particleEntityId);
        }
    }

    void OnExit(float blendTime) override
    {
        const SPlayParticleEffectParams& params = GetParams();
        if (m_data.pEmitter)
        {
            if (params.killOnExit)
            {
                m_data.pEmitter->Kill();
            }
            else if (!params.keepEmitterActive)
            {
                m_data.pEmitter->Activate(false);
            }

            m_context->StopEffect(m_data, params, m_scope);
        }
        else
        {
            m_context->StopEffect(m_data, params, m_scope, m_particleEntityId);

            EBUS_EVENT(AzFramework::GameEntityContextRequestBus, DestroyGameEntity, m_particleEntityId);
            m_particleEntityId = AZ::EntityId();
        }
    }

    void Update(float timePassed) override
    {
    }

private:
    SParticleEffectClipData m_data;
    AZ::EntityId m_particleEntityId;    // AZ::Entity support!
};


REGISTER_PROCEDURAL_CLIP(CProceduralClipParticleEffect, "ParticleEffect");

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Impl
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CParticleEffectContext::CParticleEffectContext()
{
}

CParticleEffectContext::~CParticleEffectContext()
{
    //Destroy all requests as the context is over
    for (uint32 i = 0u, sz = m_stopRequests.size(); i < sz; ++i)
    {
        SStopRequest& request = m_stopRequests[i];
        if (IsEffectAlive(request))
        {
            request.data.pEmitter->Activate(false);
        }
        ShutdownEffect(request);
    }
}

void CParticleEffectContext::Update(float timePassed)
{
    for (uint32 i = 0u, sz = m_stopRequests.size(); i < sz; ++i)
    {
        SStopRequest& request = m_stopRequests[i];
        if (!IsEffectAlive(request))
        {
            ShutdownEffect(request);
        }
    }

    m_stopRequests.erase(std::remove_if(m_stopRequests.begin(), m_stopRequests.end(), SStopRequest::HasFinished), m_stopRequests.end());
}

void CParticleEffectContext::StartEffect(SParticleEffectClipData& data, const SPlayParticleEffectParams& params, IScope* scope)
{
    if (!scope)
    {
        return;
    }

    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(scope->GetEntityId());
    if (!pEntity)
    {
        return;
    }

    const char* const szEffectName = params.effectName.c_str();
    if (IParticleEffect* const pEffect = gEnv->pParticleManager->FindEffect(szEffectName, "Particle.SpawnEffect"))
    {
        IAttachment* pAttachment = nullptr;
        ICharacterInstance* pCharacterInstance = scope->GetCharInst();
        if (pCharacterInstance)
        {
            IAttachmentManager& attachmentManager = *pCharacterInstance->GetIAttachmentManager();
            const char* szNewAttachmentName = nullptr;
            const uint32 jointCrc = params.jointName.crc;
            int32 attachmentJointId = jointCrc ? pCharacterInstance->GetIDefaultSkeleton().GetJointIDByCRC32(jointCrc) : -1;
            if (attachmentJointId >= 0)
            {
                // Found given joint
                szNewAttachmentName = pCharacterInstance->GetIDefaultSkeleton().GetJointNameByID(attachmentJointId);
            }
            else
            {
                // Joint not found (or not defined by the user), try and find an attachment interface
                pAttachment = attachmentManager.GetInterfaceByNameCRC(params.attachmentName.crc);
                if (params.cloneAttachment && pAttachment && pAttachment->GetType() == CA_BONE)
                {
                    // Clone an already existing attachment interface
                    attachmentJointId = pAttachment->GetJointID();
                    szNewAttachmentName = pAttachment->GetName();
                }
            }

            if (szNewAttachmentName && attachmentJointId >= 0)
            {
                // Create new attachment interface with a unique name
                static uint16 s_clonedAttachmentCount = 0;
                ++s_clonedAttachmentCount;
                CryFixedStringT<64> attachmentCloneName;
                attachmentCloneName.Format("%s%s%u", szNewAttachmentName, "FXClone", s_clonedAttachmentCount);

                const char* const pBoneName = pCharacterInstance->GetIDefaultSkeleton().GetJointNameByID(attachmentJointId);
                const IAttachment* const pOriginalAttachment = pAttachment;
                pAttachment = attachmentManager.CreateAttachment(attachmentCloneName.c_str(), CA_BONE, pBoneName);
                data.ownedAttachmentNameCRC = pAttachment->GetNameCRC();

                if (!pOriginalAttachment)
                {
                    // Attachment newly created from a joint: clear the relative transform
                    pAttachment->AlignJointAttachment();
                }
                else
                {
                    // Cloning an attachment: copy original transforms
                    CRY_ASSERT(params.cloneAttachment);
                    pAttachment->SetAttAbsoluteDefault(pOriginalAttachment->GetAttAbsoluteDefault());
                    pAttachment->SetAttRelativeDefault(pOriginalAttachment->GetAttRelativeDefault());
                }
            }

            if (pAttachment && (pAttachment->GetType() == CA_BONE))
            {
                const Matrix34 offsetMatrix(Vec3(1.0f), Quat::CreateRotationXYZ(params.rotOffset), params.posOffset);
                unsigned int flags = ToFlags(params);
                CEffectAttachment* const pEffectAttachment = new CEffectAttachment(pEffect, offsetMatrix, flags);
                pAttachment->AddBinding(pEffectAttachment);
                pAttachment->UpdateAttModelRelative();
                pEffectAttachment->ProcessAttachment(pAttachment);
                data.pEmitter = pEffectAttachment->GetEmitter();

                if (data.pEmitter)
                {
                    // Put the particle emitter in the entity's render proxy so that it can be updated based on visibility.
                    data.slot = pEntity->SetParticleEmitter(-1, data.pEmitter);
                }
            }
        }

        if (!data.pEmitter && !pAttachment)
        {
            if (pEffect->GetParticleParams().eAttachType != GeomType_None)
            {
                data.slot = pEntity->LoadParticleEmitter(-1, pEffect);
                SEntitySlotInfo slotInfo;
                if (pEntity->GetSlotInfo(data.slot, slotInfo))
                {
                    data.pEmitter = slotInfo.pParticleEmitter;
                }
            }
            else
            {
                const Matrix34& transform = pEntity->GetWorldTM();
                const Matrix34 localOffset(Vec3(1.0f), Quat(params.rotOffset), params.posOffset);
                data.pEmitter = pEffect->Spawn(transform * localOffset);
            }
        }

        if (data.pEmitter)
        {
            IMannequin& mannequinInterface = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
            IActionController& actionController = scope->GetActionController();
            const uint32 numListeners = mannequinInterface.GetNumMannequinGameListeners();
            for (uint32 itListeners = 0; itListeners < numListeners; ++itListeners)
            {
                mannequinInterface.GetMannequinGameListener(itListeners)->OnSpawnParticleEmitter(data.pEmitter, actionController);
            }
        }
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "CParticleEffectContext: could not load requested effect %s", szEffectName);
    }
}

void CParticleEffectContext::StartEffect(SParticleEffectClipData& data, const SPlayParticleEffectParams& params, IScope* scope, const AZ::EntityId& entityId)
{
    // Todo: hook up to bones and attachments
    (void)data;
    (void)scope;

    // Set the emitter settings...
    LmbrCentral::ParticleEmitterSettings emitterSettings;
    emitterSettings.m_notAttached = !params.emitterFollows;
    emitterSettings.m_ignoreRotation = params.ignoreRotation;

    EBUS_EVENT_ID(entityId, LmbrCentral::ParticleComponentRequestBus, SetupEmitter, AZStd::string(params.effectName.c_str()), emitterSettings);
    EBUS_EVENT_ID(entityId, LmbrCentral::ParticleComponentRequestBus, Show);
}

void CParticleEffectContext::StopEffect(SParticleEffectClipData& data, const SPlayParticleEffectParams& params, IScope* scope)
{
    if (!scope)
    {
        return;
    }

    SStopRequest newRequest(data, scope->GetEntityId(), scope->GetCharInst());

    //overwrite the attachmentNameCRC if needed
    newRequest.data.ownedAttachmentNameCRC = newRequest.data.ownedAttachmentNameCRC ? newRequest.data.ownedAttachmentNameCRC : params.attachmentName.crc;

    //try immediate stop
    if (!IsEffectAlive(newRequest))
    {
        ShutdownEffect(newRequest);
    }

    //we need to defer the stop request for later.
    if (!newRequest.isFinished)
    {
        m_stopRequests.push_back(newRequest);
    }
}

void CParticleEffectContext::StopEffect(SParticleEffectClipData& data, const SPlayParticleEffectParams& params, IScope* scope, const AZ::EntityId& entityId)
{
    (void)data;
    (void)params;
    (void)scope;
    EBUS_EVENT_ID(entityId, LmbrCentral::ParticleComponentRequestBus, Hide);
}

bool CParticleEffectContext::IsEffectAlive(const SStopRequest& request) const
{
    return (request.data.pEmitter && request.data.pEmitter->IsAlive());
}

void CParticleEffectContext::ShutdownEffect(SStopRequest& request)
{
    //Kill the attachment,emitter and request
    if (request.data.pEmitter)
    {
        if (request.pCharInst)
        {
            IAttachmentManager& attachmentManager = *request.pCharInst->GetIAttachmentManager();
            if (IAttachment* pAttachment = attachmentManager.GetInterfaceByNameCRC(request.data.ownedAttachmentNameCRC))
            {
                IAttachmentObject* pAttachmentObject = pAttachment->GetIAttachmentObject();
                if (pAttachmentObject && (pAttachmentObject->GetAttachmentType() == IAttachmentObject::eAttachment_Effect))
                {
                    CEffectAttachment* pEffectAttachment = static_cast<CEffectAttachment*>(pAttachmentObject);
                    if (pEffectAttachment->GetEmitter() == request.data.pEmitter)
                    {
                        pAttachment->ClearBinding();
                    }
                }
            }
        }

        //need to clear emitters from slot(if slotted)
        IEntity* pEntity = gEnv->pEntitySystem->GetEntity(request.entityId);
        if (request.data.slot != -1 && pEntity)
        {
            pEntity->FreeSlot(request.data.slot);
            request.data.slot = -1;
        }

        request.data.pEmitter.reset();
    }

    request.isFinished = true;
}