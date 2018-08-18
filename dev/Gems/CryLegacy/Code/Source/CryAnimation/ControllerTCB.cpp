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

// Description : TCB controller implementation.


#include "CryLegacy_precompiled.h"
#include "CharacterManager.h"

JointState CControllerTCB::GetOPS(f32 ntime, Quat& rot, Vec3& pos, Diag33& scl) const
{
    f32 key = ntime;
    if (m_active & eJS_Orientation)
    {
        Quat out;
        static_cast<spline::TCBAngleAxisSpline>(m_rotTrack).interpolate(key, out);
        rot = !out;
    }
    if (m_active & eJS_Position)
    {
        Vec3 out;
        static_cast< spline::TCBSpline<Vec3> >(m_posTrack).interpolate(key, out);
        pos = out / 100.0f; // Position controller from Max must be scalled 100 times down.
    }
    if (m_active & eJS_Scale)
    {
        Vec3 out;
        static_cast< spline::TCBSpline<Vec3> >(m_sclTrack).interpolate(key, out);
        scl = Diag33(out);
    }
    return m_active & (eJS_Orientation | eJS_Position | eJS_Scale);
}

JointState CControllerTCB::GetOP(f32 key, Quat& rot, Vec3& pos) const
{
    if (m_active & eJS_Orientation)
    {
        Quat out;
        static_cast<spline::TCBAngleAxisSpline>(m_rotTrack).interpolate(key, out);
        rot = !out;
    }
    if (m_active & eJS_Position)
    {
        Vec3 out;
        static_cast< spline::TCBSpline<Vec3> >(m_posTrack).interpolate(key, out);
        pos = out / 100.0f; // Position controller from Max must be scalled 100 times down.
    }
    return m_active & (eJS_Orientation | eJS_Position);
}

JointState CControllerTCB::GetO(f32 key, Quat& rot) const
{
    if (m_active & eJS_Orientation)
    {
        Quat out;
        static_cast<spline::TCBAngleAxisSpline>(m_rotTrack).interpolate(key, out);
        rot = !out;
    }
    return m_active & eJS_Orientation;
}

JointState CControllerTCB::GetP(f32 key, Vec3& pos) const
{
    if (m_active & eJS_Position)
    {
        Vec3 out;
        static_cast< spline::TCBSpline<Vec3> >(m_posTrack).interpolate(key, out);
        pos = out / 100.0f; // Position controller from Max must be scalled 100 times down.
    }
    return m_active & eJS_Position;
}

JointState CControllerTCB::GetS(f32 key, Diag33& scl) const
{
    if (m_active & eJS_Scale)
    {
        Vec3 out;
        static_cast< spline::TCBSpline<Vec3> >(m_sclTrack).interpolate(key, out);
        scl = Diag33(out);
    }
    return m_active & eJS_Scale;
}

