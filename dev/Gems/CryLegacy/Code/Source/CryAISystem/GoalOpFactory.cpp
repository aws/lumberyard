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

// Description : Goal Op Factory interface and management classes


#include "CryLegacy_precompiled.h"
#include "GoalOpFactory.h"


IGoalOp* CGoalOpFactoryOrdering::GetGoalOp(const char* sGoalOpName, IFunctionHandler* pH, int nFirstParam, GoalParameters& params) const
{
    IGoalOp* pResult = NULL;
    const TFactoryVector::const_iterator itEnd = m_Factories.end();
    for (TFactoryVector::const_iterator it = m_Factories.begin(); !pResult && it != itEnd; ++it)
    {
        pResult = (*it)->GetGoalOp(sGoalOpName, pH, nFirstParam, params);
    }
    return pResult;
}


IGoalOp* CGoalOpFactoryOrdering::GetGoalOp(EGoalOperations op, GoalParameters& params) const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

    IGoalOp* pResult = NULL;
    const TFactoryVector::const_iterator itEnd = m_Factories.end();
    for (TFactoryVector::const_iterator it = m_Factories.begin(); !pResult && it != itEnd; ++it)
    {
        pResult = (*it)->GetGoalOp(op, params);
    }
    return pResult;
}


void CGoalOpFactoryOrdering::AddFactory(IGoalOpFactory* pFactory)
{
    m_Factories.push_back(pFactory);
}

void CGoalOpFactoryOrdering::PrepareForFactories(size_t count)
{
    m_Factories.reserve(count);
}

void CGoalOpFactoryOrdering::DestroyAll(void)
{
    const TFactoryVector::const_iterator itEnd = m_Factories.end();
    for (TFactoryVector::const_iterator it = m_Factories.begin(); it != itEnd; ++it)
    {
        delete (*it);
    }
    m_Factories.clear();
}
