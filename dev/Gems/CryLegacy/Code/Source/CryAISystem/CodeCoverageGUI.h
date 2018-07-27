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

#ifndef CRYINCLUDE_CRYAISYSTEM_CODECOVERAGEGUI_H
#define CRYINCLUDE_CRYAISYSTEM_CODECOVERAGEGUI_H
#pragma once

#if !defined(_RELEASE)

const int CCLAST_ELEM = 3;


class CCodeCoverageGUI
{
    struct SStrAndTime
    {
        SStrAndTime(const char* str = NULL, float time = 0.f)
            : pStr(str)
            , fTime(time) {}

        const char* pStr;
        float               fTime;
    };

public:     // Construction & destruction
    CCodeCoverageGUI(void);
    ~CCodeCoverageGUI(void);

public:     // Operations
    void Reset(IAISystem::EResetReason reason);
    void Update(CTimeValue frameStartTime, float frameDeltaTime);
    void DebugDraw(int nMode);

private:    // Member data
    float m_fPercentageDone, m_fNewHit, m_fUnexpectedHit;
    ITexture* m_pTex, * m_pTexHit, * m_pTexUnexpected;
    SStrAndTime m_arrLast3[CCLAST_ELEM];
    std::vector<const char*> m_vecRemaining;
    int m_nListUnexpectedSize;
};

#endif //_RELEASE

#endif // CRYINCLUDE_CRYAISYSTEM_CODECOVERAGEGUI_H
