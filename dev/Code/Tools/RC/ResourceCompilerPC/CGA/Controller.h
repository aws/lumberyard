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

//  Notes:
//    IController interface declaration
//    See the IController comment for more info


#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_CONTROLLER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_CONTROLLER_H
#pragma once



#include "QuatQuantization.h"

#define STATUS_O   0x1
#define STATUS_P   0x2
#define STATUS_S   0x4
#define STATUS_PAD 0x8

// The concept of ticks seems tied to an older TCB format animation representation
// For modern CAF files, ticks and frames are equivalent. The TICK_CONVERT is kept for
// back compatibility with this format.
#define TICKS_PER_FRAME 1
#define TICKS_CONVERT 160

struct Status4
{
    union
    {
        uint32 ops;
        struct
        {
            uint8 o;
            uint8 p;
            uint8 s;
            uint8 pad;
        };
    };
    ILINE Status4()
    {
    }
    ILINE Status4(uint8 _x, uint8 _y, uint8 _z, uint8 _w = 1)
    {
        o = _x;
        p = _y;
        s = _z;
        pad = _w;
    }
};

struct CInfo
{
    uint32 numKeys;
    Quat   quat;
    Vec3   pos;
    int    stime;
    int    etime;
    int    realkeys;

    CInfo()
        : numKeys(-1)
        , stime(-1)
        , etime(-1)
        , realkeys(-1)
    {
    }
};


struct GlobalAnimationHeaderCAF;

//////////////////////////////////////////////////////////////////////////////////////////
// interface IController
// Describes the position and orientation of an object, changing in time.
// Responsible for loading itself from a binary file, calculations
//////////////////////////////////////////////////////////////////////////////////////////
class CController;

class IController
    : public _reference_target_t
{
public:
    virtual const CController* GetCController() const
    {
        return 0;
    }

    // each controller has an ID, by which it is identifiable
    virtual uint32 GetID() const = 0;

    // returns the orientation of the controller at the given time
    virtual Status4 GetOPS(f32 t, Quat& quat, Vec3& pos, Diag33& scale) = 0;
    // returns the orientation and position of the controller at the given time
    virtual Status4 GetOP(f32 t, Quat& quat, Vec3& pos) = 0;
    // returns the orientation of the controller at the given time
    virtual uint32 GetO(f32 t, Quat& quat) = 0;
    // returns position of the controller at the given time
    virtual uint32 GetP(f32 t, Vec3& pos) = 0;
    // returns scale of the controller at the given time
    virtual uint32 GetS(f32 t, Diag33& pos) = 0;

    virtual int32 GetO_numKey() = 0; //only implemented for the PQ controller
    virtual int32 GetP_numKey() = 0; //only implemented for the PQ controller

    virtual CInfo GetControllerInfoRoot() = 0;
    virtual CInfo GetControllerInfo() const = 0;
    virtual size_t SizeOfThis() const = 0;

    void SetFlags(unsigned newFlags)
    {
        m_nFlags = newFlags;
    }

    void AddFlags(unsigned newFlag)
    {
        m_nFlags |= newFlag;
    }

    void RemoveFlags(unsigned removeFlags)
    {
        m_nFlags &= ~removeFlags;
    }

    unsigned GetFlags() const
    {
        return m_nFlags;
    }

    unsigned m_nFlags;

    enum CFlags
    {
        DYNAMIC_CONTROLLER = (1 << 0),
    };

    IController()
        : m_nFlags(0)
    {
    }

    virtual ~IController()
    {
    }
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
    bool operator() (const IController_AutoPtr& a, const IController_AutoPtr& b) const
    {
        assert (a != (IController*)NULL && b != (IController*)NULL);
        return a->GetID() < b->GetID();
    }
    bool operator() (const IController_AutoPtr& a, uint32 nID) const
    {
        assert (a != (IController*)NULL);
        return a->GetID() < nID;
    }
};

class AnimCtrlSortPredInt
{
public:
    bool operator() (const IController_AutoPtr& a, uint32 nID) const
    {
        assert (a != (IController*)NULL);
        return a->GetID() < nID;
    }
};

typedef std::vector<IController_AutoPtr> TControllersVector;

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_CONTROLLER_H
