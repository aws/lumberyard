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

//  Description: TCB controller implementation.

#ifndef CRYINCLUDE_CRYANIMATION_CONTROLLERTCB_H
#define CRYINCLUDE_CRYANIMATION_CONTROLLERTCB_H
#pragma once

#include "TCBSpline.h"


// CJoint animation.
struct CControllerTCB
    : public IController
{
    size_t SizeOfController() const
    {
        return sizeof(CControllerTCB) + m_posTrack.sizeofThis() +  m_rotTrack.sizeofThis() + m_sclTrack.sizeofThis();
    }

    size_t ApproximateSizeOfThis() const
    {
        return SizeOfController();
    }

    JointState GetOPS(f32 key, Quat& rot, Vec3& pos, Diag33& scl) const;
    JointState GetOP(f32 key, Quat& rot, Vec3& pos) const;
    JointState GetO(f32 key, Quat& rot) const;
    JointState GetP(f32 key, Vec3& pos) const;
    JointState GetS(f32 key, Diag33& scl) const;

    virtual size_t GetRotationKeysNum() const
    {
        return 0;
    }

    virtual size_t GetPositionKeysNum() const
    {
        return 0;
    }

    JointState m_active;
    spline::TCBSpline<Vec3> m_posTrack;
    spline::TCBAngleAxisSpline m_rotTrack;
    spline::TCBSpline<Vec3> m_sclTrack;

    CControllerTCB()
    {
        m_nControllerId = 0xaaaa5555;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_posTrack);
        pSizer->AddObject(m_rotTrack);
        pSizer->AddObject(m_sclTrack);
    }
};


#endif // CRYINCLUDE_CRYANIMATION_CONTROLLERTCB_H

