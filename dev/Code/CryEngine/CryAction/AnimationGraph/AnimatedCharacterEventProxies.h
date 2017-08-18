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

#ifndef CRYINCLUDE_CRYACTION_ANIMATIONGRAPH_ANIMATEDCHARACTEREVENTPROXIES_H
#define CRYINCLUDE_CRYACTION_ANIMATIONGRAPH_ANIMATEDCHARACTEREVENTPROXIES_H
#pragma once

#include <IComponent.h>

class CAnimatedCharacter;
class CAnimatedCharacterComponent_Base
    : public IComponent
{
public:
    struct SComponentInitializerAnimChar
        : public SComponentInitializer
    {
        SComponentInitializerAnimChar(IEntity* piEntity, CAnimatedCharacter* pAnimChar)
            : SComponentInitializer(piEntity)
            ,   m_pAnimCharacter(pAnimChar)
        {
        }

        CAnimatedCharacter* m_pAnimCharacter;
    };

    virtual void Initialize(const SComponentInitializer& init);

protected:

    typedef CAnimatedCharacterComponent_Base SuperClass;

    explicit CAnimatedCharacterComponent_Base();

    virtual void ProcessEvent(SEntityEvent& event);
    virtual void OnPrePhysicsUpdate(float elapseTime) = 0;

    CAnimatedCharacter* m_pAnimCharacter;
    IEntity* m_piEntity;
};

class CAnimatedCharacterComponent_PrepareAnimatedCharacterForUpdate
    : public CAnimatedCharacterComponent_Base
{
public:
    DECLARE_COMPONENT_TYPE("AnimatedCharacterComponent_PrepareAnimatedCharacterForUpdate", 0x7AD4F41B71E4416E, 0xB1D121F27D34A1A5)

    CAnimatedCharacterComponent_PrepareAnimatedCharacterForUpdate();

    ILINE void QueueRotation(const Quat& rotation) { m_queuedRotation = rotation; m_hasQueuedRotation = true; }
    ILINE void ClearQueuedRotation() { m_hasQueuedRotation = false; }

protected:

    virtual ComponentEventPriority GetEventPriority(const int eventID) const;

private:
    Quat m_queuedRotation;
    bool m_hasQueuedRotation;
    virtual void OnPrePhysicsUpdate(float elapsedTime);
};

DECLARE_COMPONENT_POINTERS(CAnimatedCharacterComponent_PrepareAnimatedCharacterForUpdate);

class CAnimatedCharacterComponent_StartAnimProc
    : public CAnimatedCharacterComponent_Base
{
public:
    DECLARE_COMPONENT_TYPE("AnimatedCharacterComponent_StartAnimProc", 0xD6102AF31C0E4F45, 0x97BF0B388F112955)

    CAnimatedCharacterComponent_StartAnimProc() {}

protected:

    virtual ComponentEventPriority GetEventPriority(const int eventID) const;
    virtual void OnPrePhysicsUpdate(float elapsedTime);
};

class CAnimatedCharacterComponent_GenerateMoveRequest
    : public CAnimatedCharacterComponent_Base
{
public:
    DECLARE_COMPONENT_TYPE("AnimatedCharacterComponent_GenerateMoveRequest", 0x5E79058469294C15, 0x9A25E264D8691960)

    CAnimatedCharacterComponent_GenerateMoveRequest() {}

protected:

    virtual ComponentEventPriority GetEventPriority(const int eventID) const;
    virtual void OnPrePhysicsUpdate(float elapsedTime);
};

#endif // CRYINCLUDE_CRYACTION_ANIMATIONGRAPH_ANIMATEDCHARACTEREVENTPROXIES_H
