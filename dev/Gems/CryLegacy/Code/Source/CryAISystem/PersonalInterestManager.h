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

// Description : Interest Manager (tracker) for interested individuals

// Note        : PIMs are owned by the CentralInterestManager - delete there


#ifndef CRYINCLUDE_CRYAISYSTEM_PERSONALINTERESTMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_PERSONALINTERESTMANAGER_H
#pragma once

#include "IInterestSystem.h"
#include "IAIAction.h"


class CCentralInterestManager;
struct SEntityInterest;

// Settings for ai actors
struct SActorInterestSettings
{
    SActorInterestSettings(bool bEnablePIM, float fInterestFilter, float fAngleInDegrees)
        : m_bEnablePIM(bEnablePIM)
        , m_fInterestFilter(fInterestFilter)
    {
        SetAngleInDegrees(fAngleInDegrees);
    }

    SActorInterestSettings()
    {
        Reset();
    }

    // Set to defaults
    void Reset()
    {
        m_bEnablePIM = true;
        m_fInterestFilter = 0.f;
        SetAngleInDegrees(270.f);
    }

    void SetAngleInDegrees(float fAngleInDegrees)
    {
        clamp_tpl(fAngleInDegrees, 0.f, 360.f);
        m_fAngleCos = cos(DEG2RAD(.5f * fAngleInDegrees));
    }

    void Serialize(TSerialize ser)
    {
        ser.Value("fInterestFilter", m_fInterestFilter);
        ser.Value("fAngleCos", m_fAngleCos);
        ser.Value("bEnablePIM", m_bEnablePIM);
    }

    void Set(bool bEnablePIM, float fInterestFilter, float fAngleCos)
    {
        m_bEnablePIM = bEnablePIM;

        if (fInterestFilter >= 0.f)
        {
            m_fInterestFilter = fInterestFilter;
        }

        if (fAngleCos >= -1.f)
        {
            m_fAngleCos = fAngleCos;
        }
    }

    float m_fInterestFilter;    // What minimum interest must something have for this actor?
    float m_fAngleCos;                      // If the angle between our movement direction and the interest Object is beyond this range, it will be ignored
    bool m_bEnablePIM;              // Can this actor use the interest system at all?
};


class CPersonalInterestManager
    : public IAIAction::IAIActionListener
{
public:
    CPersonalInterestManager(CAIActor* pAIActor = NULL);

    // Clear tracking cache, clear assignment
    void Reset();

    bool ForgetInterestingEntity();

    // Is assignment cleared?
    bool IsReset() const { return m_refAIActor.IsReset(); }

    // Required to implement external interface - might use adapter
    void Assign(CAIActor* pAIActor);

    // (Re)assign to an actor (helps in object re-use)
    // NILREF is acceptable, to leave unassigned
    // You must also ensure the PIM pointer in the CAIActor is set to this object
    void Assign(CWeakRef<CAIActor> refAIActor);

    // Get the currently assigned AIActor
    CAIActor* GetAssigned() const { return m_refAIActor.GetAIObject(); }

    // Update
    bool Update(bool bCloseToCamera);

    void Serialize(TSerialize ser);

    // Update interest settings
    // Checks with the CIM for any new settings associated with this actor
    void SetSettings(bool bEnablePIM, float fInterestFilter = -1.f, float fAngleCos = -1.f);

    // Do we have any interest target right now?
    // Allows us to move on as quickly as possible for the common (uninterested) case
    bool IsInterested() const { return m_IdInterestingEntity > 0; }

    // Which entity are we currently interested in?
    // Returns NULL if not currently interested in anything, or that target is not an entity
    IEntity* GetInterestEntity() const;

    // Returns the dummy interest object if currently interested or NILREF
    // Works regardless of the type of the interesting object
    CWeakRef<CAIObject> GetInterestDummyPoint();

    Vec3 GetInterestingPos() const;

    ELookStyle GetLookingStyle() const;

    /// Implement Action Listener Interface
    virtual void OnActionEvent(IAIAction::EActionEvent event);

protected:

    const SEntityInterest* PickMostInteresting() const;

    bool CheckVisibility(const SEntityInterest& interestingRef) const;

    CWeakRef<CAIActor>              m_refAIActor;                               // The actor we are responsible for
    EntityId                                    m_IdInterestingEntity;
    EntityId                                    m_IdLastInterestingEntity;
    CTimeValue                              m_timeLastInterestingEntity;
    Vec3                                            m_vOffsetInterestingEntity;
    CCountedRef<CAIObject>      m_refInterestDummy;                 // Dummy that moves around with any interest target
    CCentralInterestManager*    m_pCIM;                                         // Cached pointer to CIM
    SActorInterestSettings      m_Settings;                                 // Interest settings for this actor
    bool                                            m_bIsPlayingAction;
};


#endif // CRYINCLUDE_CRYAISYSTEM_PERSONALINTERESTMANAGER_H
