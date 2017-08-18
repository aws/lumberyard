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

#if defined(OFFLINE_COMPUTATION)

inline const bool NSH::CFullVisCache::IsUpperHemisphereVisible() const
{
    return m_UpperHemisphereIsVisible;
}

inline const bool NSH::CFullVisCache::IsLowerHemisphereVisible() const
{
    return m_LowerHemisphereIsVisible;
}

inline void NSH::CFullVisCache::CSimpleHit::SetupRay(const Vec3& crRayDir, const Vec3& crRayOrigin, const float cRayLen, const float cSign, const Vec3& crNormal, const float cBias)
{
    NSH::CEveryObjectOnlyOnce::SetupRay(crRayDir, crRayOrigin, cRayLen, cBias);
    m_PNormal = crNormal;
    m_D = cSign;
    m_HasHit = false;
};

inline const bool NSH::CFullVisCache::CSimpleHit::IsIntersecting() const
{
    return m_HasHit;
}

inline const bool NSH::CFullVisCache::IsFullyVisible(const TVec& crDir) const
{
    if (crDir.z >= 0)
    {
        return m_UpperHemisphereIsVisible;
    }
    else
    {
        return m_LowerHemisphereIsVisible;
    }
}

#endif