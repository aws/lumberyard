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


#ifndef CRYINCLUDE_CRYACTION_RANGESIGNALINGSYSTEM_RANGE_H
#define CRYINCLUDE_CRYACTION_RANGESIGNALINGSYSTEM_RANGE_H
#pragma once

class CPersonalRangeSignaling;

class CRange
{
public:
    // Base ----------------------------------------------------------
    CRange(CPersonalRangeSignaling* pPersonal);
    virtual ~CRange();
    void            Init(float fRadius, float fBoundary, const char* sSignal, IAISignalExtraData* pData = NULL);

    // Utils ---------------------------------------------------------
    bool            IsInRange(const Vec3& vPos) const;
    bool            IsInRangePlusBoundary(const Vec3& vPos) const;

    bool operator<(const CRange& Range) const
    {
        return(m_fRadius < Range.m_fRadius);
    }

    bool IsInRange(float fSquaredDIst) const
    {
        return(fSquaredDIst < m_fRadius);
    }

    bool IsInRangePlusBoundary(float fSquaredDIst) const
    {
        return(fSquaredDIst < m_fBoundary);
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

private:
    CPersonalRangeSignaling*  m_pPersonal;
    IAISignalExtraData*       m_pSignalData;
    float                     m_fRadius;
    float                     m_fBoundary;
    string                    m_sSignal;
};
#endif // CRYINCLUDE_CRYACTION_RANGESIGNALINGSYSTEM_RANGE_H
