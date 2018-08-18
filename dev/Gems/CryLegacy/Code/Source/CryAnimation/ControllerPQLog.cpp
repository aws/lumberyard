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
#include "CharacterManager.h"
#include <float.h>


CControllerPQLog::CControllerPQLog()//: m_nControllerId(0)
{
    m_lastTime = -FLT_MAX;
    m_lastTimeLM = -FLT_MAX;
}

CControllerPQLog::~CControllerPQLog() { }






inline f64 DLength(Vec3& v)
{
    f64 dx = v.x * v.x;
    f64 dy = v.y * v.y;
    f64 dz = v.z * v.z;
    return sqrt(dx + dy + dz);
}


// blends the pqFrom and pqTo with weights 1-fBlend and fBlend into pqResult
void PQLog::blendPQ (const PQLog& pqFrom, const PQLog& pqTo, f32 fBlend)
{
    CRY_ASSERT(fBlend >= 0 && fBlend <= 1);
    vPos = pqFrom.vPos * (1 - fBlend) + pqTo.vPos * fBlend;
    Quat qFrom = Quat::exp(pqFrom.vRotLog);
    Quat qTo   = Quat::exp(pqTo.vRotLog);
    qFrom = Quat::CreateNlerp(qFrom, qTo, fBlend);
    vRotLog = Quat::log(qFrom);
}




// adjusts the rotation of these PQs: if necessary, flips them or one of them (effectively NOT changing the whole rotation,but
// changing the rotation angle to Pi-X and flipping the rotation axis simultaneously)
// this is needed for blending between animations represented by quaternions rotated by ~PI in quaternion space
// (and thus ~2*PI in real space)
void AdjustLogRotations (Vec3& vRotLog1, Vec3& vRotLog2)
{
    f64 dLen1 = DLength(vRotLog1);
    if (dLen1 > gf_PI / 2)
    {
        vRotLog1 *= (f32)(1.0f - gf_PI / dLen1);
        // now the length of the first vector is actually gf_PI - dLen1,
        // and it's closer to the origin than its complementary
        // but we won't need the dLen1 any more
    }

    f64 dLen2 = DLength(vRotLog2);
    // if the flipped 2nd rotation vector is closer to the first rotation vector,
    // then flip the second vector
    if ((vRotLog1 | vRotLog2) < (dLen2 - gf_PI / 2) * dLen2 && dLen2 > 0.000001f)
    {
        // flip the second rotation also
        vRotLog2 *= (f32)(1.0f - gf_PI / dLen2);
    }
}

JointState CControllerPQLog::GetOPS(f32 key, Quat& quat, Vec3& pos, Diag33& scale) const
{
    QuatT pq = DecodeKey(key);
    quat = pq.q;
    pos = pq.t;
    return eJS_Position | eJS_Orientation;
}

JointState CControllerPQLog::GetOP(f32 key, Quat& quat, Vec3& pos) const
{
    QuatT pq = DecodeKey(key);
    quat = pq.q;
    pos = pq.t;
    return eJS_Position | eJS_Orientation;
}

JointState CControllerPQLog::GetO(f32 key, Quat& quat) const
{
    QuatT pq = DecodeKey(key);
    quat = pq.q;
    return eJS_Orientation;
}
JointState CControllerPQLog::GetP(f32 key, Vec3& pos) const
{
    QuatT pq = DecodeKey(key);
    pos = pq.t;
    return eJS_Position;
}
JointState CControllerPQLog::GetS(f32 key, Diag33& pos) const
{
    return 0;
}
