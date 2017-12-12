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

#ifndef CRYINCLUDE_CRYANIMATION_CONTROLLER_H
#define CRYINCLUDE_CRYANIMATION_CONTROLLER_H
#pragma once


#define TICKS_CONVERT       (160)           //what is this?

#include "JointState.h"

//////////////////////////////////////////////////////////////////////////////////////////
// interface IController
// Describes the position and orientation of an object, changing in time.
// Responsible for loading itself from a binary file, calculations
//////////////////////////////////////////////////////////////////////////////////////////
class IController
    : public _i_reference_target_t
{
public:
    // each controller has an ID, by which it is identifiable
    uint32 m_nControllerId;

    // each controller has flags which setup special handling
    uint32 m_nFlags;

    enum e_ControllerFlags
    {
        DYNAMIC_CONTROLLER = (1 << 0),
    };

    uint32 GetID() const { return m_nControllerId; }
    uint32 GetFlags() const { return m_nFlags; }

    // returns the orientation,position and scaling of the controller at the given time
    virtual JointState GetOPS(f32 key, Quat& quat, Vec3& pos, Diag33& scale) const = 0;
    // returns the orientation and position of the controller at the given time
    virtual JointState GetOP(f32 key, Quat& quat, Vec3& pos) const = 0;

    // returns the orientation of the controller at the given time
    virtual JointState GetO(f32 key, Quat& quat) const = 0;
    // returns position of the controller at the given time
    virtual JointState GetP(f32 key, Vec3& pos) const = 0;
    // returns scale of the controller at the given time
    virtual JointState GetS(f32 key, Diag33& pos) const = 0;

    virtual int32 GetO_numKey() const { return -1; } //only implemented for the PQ controller
    virtual int32 GetP_numKey() const { return -1; } //only implemented for the PQ controller

    virtual size_t GetRotationKeysNum() const = 0;
    virtual size_t GetPositionKeysNum() const = 0;

    virtual size_t SizeOfController() const = 0;
    virtual size_t ApproximateSizeOfThis() const = 0;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
};



TYPEDEF_AUTOPTR(IController);




// adjusts the rotation of these PQs: if necessary, flips them or one of them (effectively NOT changing the whole rotation,but
// changing the rotation angle to Pi-X and flipping the rotation axis simultaneously)
// this is needed for blending between animations represented by quaternions rotated by ~PI in quaternion space
// (and thus ~2*PI in real space)
extern void AdjustLogRotations (Vec3& vRotLog1, Vec3& vRotLog2);


//////////////////////////////////////////////////////////////////////////
// This class is a non-trivial predicate used for sorting an
// ARRAY OF smart POINTERS  to IController's. That's why I need a separate
// predicate class for that. Also, to avoid multiplying predicate classes,
// there are a couple of functions that are also used to find a IController
// in a sorted by ID array of IController* pointers, passing only ID to the
// lower_bound function instead of creating and passing a dummy IController*
class AnimCtrlSortPred
{
public:
    bool operator() (const IController_AutoPtr& a, const IController_AutoPtr& b)
    {
        CRY_ASSERT(a && b);
        return a->GetID() < b->GetID();
    }
};


class AnimCtrlSortPredInt
{
public:
    bool operator() (const IController* a, uint32 nID)
    {
        CRY_ASSERT(a);
        return a->GetID() < nID;
    }
    bool operator() (uint32 nID, const IController* b)
    {
        CRY_ASSERT(b);
        return nID < b->GetID();
    }
};


#endif // CRYINCLUDE_CRYANIMATION_CONTROLLER_H
