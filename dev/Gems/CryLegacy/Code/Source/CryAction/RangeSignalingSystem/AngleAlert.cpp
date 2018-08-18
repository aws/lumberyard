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

// Description : Single angle slice


#include "CryLegacy_precompiled.h"
#include "AngleAlert.h"
#include "PersonalRangeSignaling.h"
#include "IAIObject.h"
// Description:
//   Constructor
// Arguments:
//
// Return:
//
CAngleAlert::CAngleAlert(CPersonalRangeSignaling* pPersonal)
    : m_pPersonal(pPersonal)
    , m_fAngle(-1.0f)
    , m_fBoundary(-1.0f)
    , m_pSignalData(NULL)
{
    CRY_ASSERT(pPersonal != NULL);
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CAngleAlert::~CAngleAlert()
{
    if (gEnv->pAISystem)
    {
        gEnv->pAISystem->FreeSignalExtraData(m_pSignalData);
    }
}

// Description:
//   Init
// Arguments:
//
// Return:
//
void CAngleAlert::Init(float fAngle, float fBoundary, const char* sSignal, IAISignalExtraData* pData /*= NULL*/)
{
    CRY_ASSERT(fAngle >= 1.0f);
    CRY_ASSERT(fBoundary >= 0.0f);
    CRY_ASSERT(sSignal != NULL);

    m_sSignal = sSignal;
    m_fAngle = DEG2RAD(fAngle);
    m_fBoundary = DEG2RAD(fBoundary) + m_fAngle;

    // Clone extra data
    if (pData && gEnv->pAISystem)
    {
        gEnv->pAISystem->FreeSignalExtraData(m_pSignalData);
        m_pSignalData = gEnv->pAISystem->CreateSignalExtraData();
        CRY_ASSERT(m_pSignalData);
        *m_pSignalData = *pData;
    }
}

// Description:
//
// Arguments:
//
// Return:
//
bool CAngleAlert::Check(const Vec3& vPos) const
{
    return(GetAngleTo(vPos) < m_fAngle);
}

// Description:
//
// Arguments:
//
// Return:
//
bool CAngleAlert::CheckPlusBoundary(const Vec3& vPos) const
{
    return(GetAngleTo(vPos) < m_fBoundary);
}

// Description:
//
// Arguments:
//
// Return:
//
float CAngleAlert::GetAngleTo(const Vec3& vPos) const
{
    float fResult = 0.0f;
    IEntity* pEntity = m_pPersonal->GetEntity();
    CRY_ASSERT(pEntity);
    if (pEntity)
    {
        const Vec3&  vEntityPos = pEntity->GetPos();
        const Vec3&  vEntityDir = pEntity->GetAI()->GetViewDir();
        Vec3  vDiff = vPos - vEntityPos;
        vDiff.NormalizeSafe();
        fResult = acosf(vEntityDir.Dot(vDiff));
    }
    return fResult;
}
