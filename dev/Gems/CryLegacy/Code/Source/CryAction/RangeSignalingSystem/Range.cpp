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


#include "CryLegacy_precompiled.h"
#include "Range.h"
#include "PersonalRangeSignaling.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CRange::CRange(CPersonalRangeSignaling* pPersonal)
    : m_pPersonal(pPersonal)
    , m_fRadius(-1.0f)
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
CRange::~CRange()
{
    if (gEnv->pAISystem)
    {
        gEnv->pAISystem->FreeSignalExtraData(m_pSignalData);
    }
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
void CRange::Init(float fRadius, float fBoundary, const char* sSignal, IAISignalExtraData* pData /*= NULL*/)
{
    CRY_ASSERT(fRadius >= 1.0f);
    CRY_ASSERT(fBoundary >= 0.0f);
    CRY_ASSERT(sSignal != NULL);

    m_sSignal = sSignal;
    m_fRadius = fRadius * fRadius;
    m_fBoundary = fBoundary + fRadius;
    m_fBoundary *= m_fBoundary;

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
//   Destructor
// Arguments:
//
// Return:
//
bool CRange::IsInRange(const Vec3& vPos) const
{
    bool bResult = false;
    IEntity* pEntity = m_pPersonal->GetEntity();
    CRY_ASSERT(pEntity);
    if (pEntity)
    {
        Vec3  vOrigin = pEntity->GetPos();
        bResult = IsInRange((vPos - vOrigin).GetLengthSquared());
    }
    return bResult;
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CRange::IsInRangePlusBoundary(const Vec3& vPos) const
{
    bool bResult = false;
    IEntity* pEntity = m_pPersonal->GetEntity();
    CRY_ASSERT(pEntity);
    if (pEntity)
    {
        Vec3  vOrigin = pEntity->GetPos();
        bResult = IsInRangePlusBoundary((vPos - vOrigin).GetLengthSquared());
    }
    return bResult;
}

