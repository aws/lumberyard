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

// Description : Single Range donut


#ifndef CRYINCLUDE_CRYACTION_RANGESIGNALINGSYSTEM_ANGLEALERT_H
#define CRYINCLUDE_CRYACTION_RANGESIGNALINGSYSTEM_ANGLEALERT_H
#pragma once

class CPersonalRangeSignaling;

class CAngleAlert
{
public:
    // Base ----------------------------------------------------------
    CAngleAlert(CPersonalRangeSignaling* pPersonal);
    virtual         ~CAngleAlert();
    void            Init(float fAngle, float fBoundary, const char* sSignal, IAISignalExtraData* pData = NULL);

    // Utils ---------------------------------------------------------
    bool            Check(const Vec3& vPos) const;
    bool            CheckPlusBoundary(const Vec3& vPos) const;
    float           GetAngleTo(const Vec3& vPos) const;

    bool operator<(const CAngleAlert& AngleAlert) const
    {
        return(m_fAngle < AngleAlert.m_fAngle);
    }

    bool Check(float fAngle) const
    {
        return(fAngle < m_fAngle);
    }

    bool CheckPlusBoundary(float fAngle) const
    {
        return(fAngle < m_fBoundary);
    }

    const string& GetSignal() const
    {
        return(m_sSignal);
    }

    IAISignalExtraData* GetSignalData() const
    {
        return(m_pSignalData);
    }

private:
    CPersonalRangeSignaling*  m_pPersonal;
    IAISignalExtraData*       m_pSignalData;
    float                     m_fAngle;
    float                     m_fBoundary;
    string                    m_sSignal;
};
#endif // CRYINCLUDE_CRYACTION_RANGESIGNALINGSYSTEM_ANGLEALERT_H
