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
#include "AnimatedCharacterEventProxies.h"
#include "AnimatedCharacter.h"

#include "CryActionCVars.h"

#include <IComponent.h>

#include <CryExtension/Impl/ClassWeaver.h>
#include <CryExtension/CryCreateClassInstance.h>

DECLARE_DEFAULT_COMPONENT_FACTORY(CAnimatedCharacterComponent_PrepareAnimatedCharacterForUpdate, CAnimatedCharacterComponent_PrepareAnimatedCharacterForUpdate)
DECLARE_DEFAULT_COMPONENT_FACTORY(CAnimatedCharacterComponent_StartAnimProc, CAnimatedCharacterComponent_StartAnimProc)
DECLARE_DEFAULT_COMPONENT_FACTORY(CAnimatedCharacterComponent_GenerateMoveRequest, CAnimatedCharacterComponent_GenerateMoveRequest)

//////////////////////////////////////////////////////////////////////////

CAnimatedCharacterComponent_Base::CAnimatedCharacterComponent_Base()
    : m_pAnimCharacter(NULL)
{
}

void CAnimatedCharacterComponent_Base::Initialize(const SComponentInitializer& init)
{
    m_piEntity = init.m_pEntity;
    m_pAnimCharacter = static_cast<const SComponentInitializerAnimChar&> (init).m_pAnimCharacter;

    RegisterEvent(ENTITY_EVENT_PREPHYSICSUPDATE);
}

void CAnimatedCharacterComponent_Base::ProcessEvent(SEntityEvent& event)
{
    switch (event.event)
    {
    case ENTITY_EVENT_PREPHYSICSUPDATE:
        OnPrePhysicsUpdate(event.fParam[0]);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////

CAnimatedCharacterComponent_PrepareAnimatedCharacterForUpdate::CAnimatedCharacterComponent_PrepareAnimatedCharacterForUpdate()
    :  m_queuedRotation(IDENTITY)
    , m_hasQueuedRotation(false)
{
}

void CAnimatedCharacterComponent_PrepareAnimatedCharacterForUpdate::OnPrePhysicsUpdate(float)
{
    CRY_ASSERT(m_pAnimCharacter);

    if (CAnimationGraphCVars::Get().m_useQueuedRotation && m_hasQueuedRotation)
    {
        m_piEntity->SetRotation(m_queuedRotation, ENTITY_XFORM_USER | ENTITY_XFORM_NOT_REREGISTER);
        ClearQueuedRotation();
    }

    m_pAnimCharacter->PrepareAnimatedCharacterForUpdate();
}

IComponent::ComponentEventPriority CAnimatedCharacterComponent_PrepareAnimatedCharacterForUpdate::GetEventPriority(const int eventID) const
{
    CRY_ASSERT(m_pAnimCharacter);

    switch (eventID)
    {
    case ENTITY_EVENT_PREPHYSICSUPDATE:
    {
        int priority = EntityEventPriority::User + EEntityEventPriority_PrepareAnimatedCharacterForUpdate;

        if (m_pAnimCharacter->IsClient())
        {
            // we want the client StartAnimProc to happen after both CActor and GenMoveRequest.
            priority += EEntityEventPriority_Client;
        }

        return priority;
    }
    }
    return EntityEventPriority::User;
}

//////////////////////////////////////////////////////////////////////////

void CAnimatedCharacterComponent_StartAnimProc::OnPrePhysicsUpdate(float elapsedTime)
{
    CRY_ASSERT(m_pAnimCharacter);

    m_pAnimCharacter->PrepareAndStartAnimProc();
}

IComponent::ComponentEventPriority CAnimatedCharacterComponent_StartAnimProc::GetEventPriority(const int eventID) const
{
    CRY_ASSERT(m_pAnimCharacter);

    switch (eventID)
    {
    case ENTITY_EVENT_PREPHYSICSUPDATE:
    {
        int priority = EntityEventPriority::User + EEntityEventPriority_StartAnimProc;

        if (m_pAnimCharacter->IsClient())
        {
            // we want the client StartAnimProc to happen after both CActor and GenMoveRequest.
            priority += EEntityEventPriority_Client;
        }
        return priority;
    }
    }

    return EntityEventPriority::User;
}

//////////////////////////////////////////////////////////////////////////

void CAnimatedCharacterComponent_GenerateMoveRequest::OnPrePhysicsUpdate(float elapsedTime)
{
    CRY_ASSERT(m_pAnimCharacter);

    m_pAnimCharacter->GenerateMovementRequest();
}

IComponent::ComponentEventPriority CAnimatedCharacterComponent_GenerateMoveRequest::GetEventPriority(const int eventID) const
{
    CRY_ASSERT(m_pAnimCharacter);

    switch (eventID)
    {
    case ENTITY_EVENT_PREPHYSICSUPDATE:
    {
        int priority = EntityEventPriority::User + EEntityEventPriority_AnimatedCharacter;

        if (m_pAnimCharacter->IsClient())
        {
            priority += EEntityEventPriority_Client;
        }
        return priority;
    }
    }

    return EntityEventPriority::User;
}
