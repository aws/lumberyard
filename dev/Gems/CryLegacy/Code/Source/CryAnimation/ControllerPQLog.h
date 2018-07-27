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

// Notes:
//    CControllerPQLog class declaration
//    CControllerPQLog is implementation of IController which is compatible with
//    the old (before 6/27/02) caf file format that contained only CryBoneKey keys.

#ifndef CRYINCLUDE_CRYANIMATION_CONTROLLERPQLOG_H
#define CRYINCLUDE_CRYANIMATION_CONTROLLERPQLOG_H
#pragma once

#include "CGFContent.h"

// old motion format cry bone controller
class CControllerPQLog
    : public IController
{
public:

    CControllerPQLog();
    ~CControllerPQLog();

    uint32 numKeysPQ() const
    {
        return m_arrKeys.size();
    }


    //////////////////////////////////////////////////////////////////////////
    // retrieves the position and orientation (in the logarithmic space,
    // i.e. instead of quaternion, its logarithm is returned)
    // may be optimal for motion interpolation
    ILINE QuatT DecodeKey(f32 key) const
    {
#ifdef DEFINE_PROFILER_FUNCTION
        DEFINE_PROFILER_FUNCTION();
#endif
        f32 realtime = key;

        int nArrTimesSize = m_arrTimes.size();
        int nNumKeysPQ = numKeysPQ();

        // use local variables to spare checking the this pointereverytime
        const PQLog* __restrict arrKeys  = &m_arrKeys[0];
        const int*      __restrict arrTimes = &m_arrTimes[0];

        if (realtime == m_lastTime)
        {
            return m_lastValue;
        }
        m_lastTime = realtime;

        PQLog pq;
        CRY_ASSERT(numKeysPQ());

        uint32 numKey = numKeysPQ() - 1;
        f32 keytime_start = (f32)arrTimes[0];
        f32 keytime_end = (f32)arrTimes[numKey];

        f32 test_end = keytime_end;
        if (realtime < keytime_start)
        {
            test_end += realtime;
        }

        if (realtime < keytime_start)
        {
            pq = arrKeys[0];
            m_lastValue = QuatT(!Quat::exp(pq.vRotLog), pq.vPos);
            return m_lastValue;
        }

        if (realtime >= keytime_end)
        {
            pq = arrKeys[numKey];
            m_lastValue = QuatT(!Quat::exp(pq.vRotLog), pq.vPos);
            return m_lastValue;
        }

        uint32 nK = nNumKeysPQ;
        CRY_ASSERT(numKeysPQ() > 1);

        int nPos  = nNumKeysPQ >> 1;
        int nStep = nNumKeysPQ >> 2;

        // use binary search
        while (nStep)
        {
            const int time = arrTimes[nPos];
            if (realtime < time)
            {
                nPos = nPos - nStep;
            }
            else
            if (realtime > time)
            {
                nPos = nPos + nStep;
            }
            else
            {
                break;
            }

            nStep = nStep >> 1;
        }

        // fine-tuning needed since time is not linear
        while (realtime > arrTimes[nPos])
        {
            nPos++;
        }

        while (realtime < arrTimes[nPos - 1])
        {
            nPos--;
        }

        CRY_ASSERT(nPos > 0 && nPos < (int)nNumKeysPQ);
        int32 k0 = arrTimes[nPos];
        int32 k1 = arrTimes[nPos - 1];
        if (k0 == k1)
        {
            m_lastValue = QuatT(!Quat::exp(arrKeys[nPos].vRotLog), arrKeys[nPos].vPos);
            return m_lastValue;
        }

        f32 t = (f32(realtime - arrTimes[nPos - 1])) / (arrTimes[nPos] - arrTimes[nPos - 1]);
        PQLog pKeys[2] = { arrKeys[nPos - 1], arrKeys[nPos] };
        AdjustLogRotations (pKeys[0].vRotLog, pKeys[1].vRotLog);
        pq.blendPQ (pKeys[0], pKeys[1], t);

        m_lastValue = QuatT(!Quat::exp(pq.vRotLog), pq.vPos);
        return m_lastValue;
    }


    JointState GetOPS(f32 key, Quat& quat, Vec3& pos, Diag33& scale) const;
    JointState GetOP(f32 key, Quat& quat, Vec3& pos) const;
    JointState GetO(f32 key, Quat& quat) const;
    JointState GetP(f32 key, Vec3& pos) const;
    JointState GetS(f32 key, Diag33& pos) const;

    QuatT GetKey0()
    {
        uint32 numKey = numKeysPQ();
        CRY_ASSERT(numKey);
        PQLog pq = m_arrKeys[0];
        return QuatT(!Quat::exp(pq.vRotLog), pq.vPos);
    }

    int32 GetO_numKey() const
    {
        uint32 numKey = numKeysPQ();
        return numKey;
    }
    int32 GetP_numKey() const
    {
        uint32 numKey = numKeysPQ();
        return numKey;
    }

    size_t SizeOfController() const
    {
        size_t s0 = sizeof(*this);
        size_t s1 = m_arrKeys.get_alloc_size();
        size_t s2 = m_arrTimes.get_alloc_size();
        return s0 + s1 + s2;
    }


    size_t ApproximateSizeOfThis() const { return SizeOfController(); }


    void SetControllerData(const DynArray<PQLog>& arrKeys, const DynArray<int>& arrTimes)
    {
        m_arrKeys   = arrKeys;
        m_arrTimes = arrTimes;
    }

    size_t GetRotationKeysNum() const
    {
        return 0;
    }

    size_t GetPositionKeysNum() const
    {
        return 0;
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_arrKeys);
        pSizer->AddObject(m_arrTimes);
    }
    //--------------------------------------------------------------------------------------------------

    //protected:
    DynArray<PQLog> m_arrKeys;
    DynArray<int> m_arrTimes;

    mutable float m_lastTime;
    mutable QuatT m_lastValue;

    float m_lastTimeLM;
    QuatT m_lastValueLM;
};


TYPEDEF_AUTOPTR(CControllerPQLog);

#endif // CRYINCLUDE_CRYANIMATION_CONTROLLERPQLOG_H

