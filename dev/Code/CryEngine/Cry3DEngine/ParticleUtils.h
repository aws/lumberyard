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

// Description : Splitting out some of the particle specific containers to here.
//               Will be moved to a proper home eventually.


#ifndef CRYINCLUDE_CRY3DENGINE_PARTICLEUTILS_H
#define CRYINCLUDE_CRY3DENGINE_PARTICLEUTILS_H
#pragma once


//////////////////////////////////////////////////////////////////////////
// Ref target that doesn't auto-delete. Handled manually
template <typename Counter>
class _plain_reference_target
{
public:
    _plain_reference_target()
        : m_nRefCounter (0)
    {}

    ~_plain_reference_target()
    {
        assert(m_nRefCounter == 0);
#ifdef _DEBUG
        m_nRefCounter = -1;
#endif
    }

    void AddRef()
    {
        assert(m_nRefCounter >= 0);
        ++m_nRefCounter;
    }

    void Release()
    {
        --m_nRefCounter;
        assert(m_nRefCounter >= 0);
    }

    Counter NumRefs() const
    {
        assert(m_nRefCounter >= 0);
        return m_nRefCounter;
    }

protected:
    Counter m_nRefCounter;
};

extern ITimer* g_pParticleTimer;

ILINE ITimer* GetParticleTimer()
{
    return g_pParticleTimer;
}

//////////////////////////////////////////////////////////////////////////
// 3D helper functions

inline void RotateToUpVector(Quat& qRot, Vec3 const& vNorm)
{
    qRot = Quat::CreateRotationV0V1(qRot.GetColumn2(), vNorm) * qRot;
}

inline void RotateToForwardVector(Quat& qRot, Vec3 const& vNorm)
{
    qRot = Quat::CreateRotationV0V1(qRot.GetColumn1(), vNorm) * qRot;
}

inline bool CheckNormalize(Vec3& vDest, const Vec3& vSource)
{
    float fLenSq = vSource.GetLengthSquared();
    if (fLenSq > FLT_MIN)
    {
        vDest = vSource * isqrt_tpl(fLenSq);
        return true;
    }
    return false;
}
inline bool CheckNormalize(Vec3& v)
{
    return CheckNormalize(v, v);
}

#endif // CRYINCLUDE_CRY3DENGINE_PARTICLEUTILS_H
