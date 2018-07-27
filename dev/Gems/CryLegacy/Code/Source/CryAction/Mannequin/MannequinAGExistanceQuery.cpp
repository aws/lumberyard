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
#include "MannequinAGExistanceQuery.h"


// ============================================================================
// ============================================================================


namespace MannequinAG
{
    // ============================================================================
    // ============================================================================


    //////////////////////////////////////////////////////////////////////////
    CMannequinAGExistanceQuery::CMannequinAGExistanceQuery(IAnimationGraphState* pAnimationGraphState)
        : m_pAnimationGraphState(pAnimationGraphState)
    {
        CRY_ASSERT(m_pAnimationGraphState != NULL);
    }


    //////////////////////////////////////////////////////////////////////////
    CMannequinAGExistanceQuery::~CMannequinAGExistanceQuery()
    {
    }


    //////////////////////////////////////////////////////////////////////////
    IAnimationGraphState* CMannequinAGExistanceQuery::GetState()
    {
        return m_pAnimationGraphState;
    }


    //////////////////////////////////////////////////////////////////////////
    void CMannequinAGExistanceQuery::SetInput(InputID, float)
    {
    }


    //////////////////////////////////////////////////////////////////////////
    void CMannequinAGExistanceQuery::SetInput(InputID, int)
    {
    }


    //////////////////////////////////////////////////////////////////////////
    void CMannequinAGExistanceQuery::SetInput(InputID, const char*)
    {
    }


    //////////////////////////////////////////////////////////////////////////
    bool CMannequinAGExistanceQuery::Complete()
    {
        return true;
    }


    //////////////////////////////////////////////////////////////////////////
    CTimeValue CMannequinAGExistanceQuery::GetAnimationLength() const
    {
        return CTimeValue();
    }


    //////////////////////////////////////////////////////////////////////////
    void CMannequinAGExistanceQuery::Reset()
    {
    }

    // ============================================================================
    // ============================================================================
} // MannequinAG
