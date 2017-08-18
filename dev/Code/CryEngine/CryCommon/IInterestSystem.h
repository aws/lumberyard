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

#ifndef CRYINCLUDE_CRYCOMMON_IINTERESTSYSTEM_H
#define CRYINCLUDE_CRYCOMMON_IINTERESTSYSTEM_H
#pragma once

#include <IEntity.h> // <> required for Interfuscator


// AI event listener
struct IInterestListener
{
    // Event types corresponding to state changes in an interest entity,
    // or any underlying AI action associated with the interest entity.
    enum EInterestEvent
    {
        eIE_InterestStart,
        eIE_InterestStop,
        eIE_InterestActionComplete,
        eIE_InterestActionAbort,
        eIE_InterestActionCancel,
    };

    // <interfuscator:shuffle>
    virtual ~IInterestListener(){}

    virtual void OnInterestEvent(EInterestEvent eInterestEvent, EntityId idInterestedActor, EntityId idInterestingEntity) = 0;

    virtual void GetMemoryUsage(ICrySizer* pCrySizer) const = 0;
    // </interfuscator:shuffle>
};


//---------------------------------------------------------------------//

class ICentralInterestManager
{
public:
    // <interfuscator:shuffle>
    virtual ~ICentralInterestManager(){}
    virtual void Reset() = 0;
    virtual bool Enable(bool bEnable) = 0;
    virtual bool IsEnabled() = 0;
    virtual void Update(float fDelta) = 0;

    // pEntity == 0, fRadius == -1.f etc. means "Don't change these properties"
    virtual void ChangeInterestingEntityProperties(IEntity* pEntity, float fRadius = -1.f, float fBaseInterest = -1.f, const char* szActionName = NULL, const Vec3& vOffset = Vec3_Zero, float fPause = -1.f, int nbShared = -1) = 0;
    virtual void DeregisterInterestingEntity(IEntity* pEntity) = 0;

    // pEntity == 0, fInterestFilter == -1.f etc. means "Don't change these properties"
    virtual void ChangeInterestedAIActorProperties(IEntity* pEntity, float fInterestFilter = -1.f, float fAngleCos = -1.f) = 0;
    virtual bool DeregisterInterestedAIActor(IEntity* pEntity) = 0;

    virtual void RegisterListener(IInterestListener* pInterestListener, EntityId idInterestingEntity) = 0;
    virtual void UnRegisterListener(IInterestListener* pInterestListener, EntityId idInterestingEntity) = 0;
    // </interfuscator:shuffle>
};


#endif // CRYINCLUDE_CRYCOMMON_IINTERESTSYSTEM_H
