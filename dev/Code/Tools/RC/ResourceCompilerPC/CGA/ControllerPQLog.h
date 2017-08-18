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
//
// Notes:
//    CControllerPQLog class declaration
//    CControllerPQLog is implementation of IController which is compatible with
//    the old (before 6/27/02) caf file format that contained only CryBoneKey keys.

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_CONTROLLERPQLOG_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_CONTROLLERPQLOG_H
#pragma once


#include "CGFContent.h"
#include "Controller.h"


// old motion format cry bone controller
class CControllerPQLog
    : public IController
{
public:
    CControllerPQLog()
        : m_nControllerId(0)
    {
    }

    ~CControllerPQLog();

    uint32 numKeys() const
    {
        return m_arrKeys.size();
    }

    uint32 GetID() const
    {
        return m_nControllerId;
    }

    QuatT GetValue3(f32 fTime);

    Status4 GetOPS(f32 realtime, Quat& quat, Vec3& pos, Diag33& scale);
    Status4 GetOP(f32 realtime, Quat& quat, Vec3& pos);
    uint32 GetO(f32 realtime, Quat& quat);
    uint32 GetP(f32 realtime, Vec3& pos);
    uint32 GetS(f32 realtime, Diag33& pos);

    QuatT GetValueByKey(uint32 key) const;

    int32 GetO_numKey()
    {
        uint32 numKey = numKeys();
        return numKey;
    }

    int32 GetP_numKey()
    {
        uint32 numKey = numKeys();
        return numKey;
    }

    CInfo GetControllerInfoRoot()
    {
        CInfo info;
        info.numKeys = m_arrKeys.size();
        info.quat    = !Quat::exp(m_arrKeys[info.numKeys - 1].vRotLog);
        info.pos     = m_arrKeys[info.numKeys - 1].vPos;
        info.etime   = m_arrTimes[info.numKeys - 1];
        return info;
    }

    // returns the start time
    f32 GetTimeStart ()
    {
        return f32(m_arrTimes[0]);
    }

    // returns the end time
    f32 GetTimeEnd()
    {
        assert(numKeys() > 0);
        return f32(m_arrTimes[numKeys() - 1]);
    }


    size_t SizeOfThis() const;


    CInfo GetControllerInfo() const
    {
        CInfo info;
        info.numKeys  = m_arrKeys.size();
        info.quat     = !Quat::exp(m_arrKeys[info.numKeys - 1].vRotLog);
        info.pos      = m_arrKeys[info.numKeys - 1].vPos;
        info.stime    = m_arrTimes[0];
        info.etime    = m_arrTimes[info.numKeys - 1];
        info.realkeys = (info.etime - info.stime) / TICKS_PER_FRAME + 1;
        return info;
    }

    void SetControllerData(const std::vector<PQLog>& arrKeys, const std::vector<int>& arrTimes)
    {
        m_arrKeys = arrKeys;
        m_arrTimes = arrTimes;
    }

    //--------------------------------------------------------------------------------------------------

public:
    std::vector<PQLog> m_arrKeys;
    std::vector<int> m_arrTimes;

    unsigned m_nControllerId;
};


TYPEDEF_AUTOPTR(CControllerPQLog);

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_CONTROLLERPQLOG_H

