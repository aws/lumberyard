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

#ifndef CRYINCLUDE_CRYANIMATION_FACIALANIMATION_FACESTATE_H
#define CRYINCLUDE_CRYANIMATION_FACIALANIMATION_FACESTATE_H
#pragma once

#include <IFacialAnimation.h>
#include "FaceEffector.h"

class CFacialModel;

//////////////////////////////////////////////////////////////////////////
// FacialState represents full state of all end effectors.
//////////////////////////////////////////////////////////////////////////
class CFaceState
    : public IFaceState
    , public _i_reference_target_t
{
public:
    CFaceState(CFacialModel* pFacialModel) { m_pFacialModel = pFacialModel; };

    //////////////////////////////////////////////////////////////////////////
    // IFaceState
    //////////////////////////////////////////////////////////////////////////
    virtual float GetEffectorWeight(int nIndex)
    {
        if (nIndex >= 0 && nIndex < (int)m_weights.size())
        {
            return GetWeight(nIndex);
        }
        return 0;
    }
    virtual void SetEffectorWeight(int nIndex, float fWeight)
    {
        if (nIndex >= 0 && nIndex < (int)m_weights.size())
        {
            SetWeight(nIndex, fWeight);
        }
    }
    //////////////////////////////////////////////////////////////////////////

    CFacialModel* GetModel() const { return m_pFacialModel; };

    float GetWeight(int nIndex) const
    {
        CRY_ASSERT(nIndex >= 0 && nIndex < (int)m_weights.size());
        return m_weights[nIndex];
    }
    void SetWeight(int nIndex, float fWeight)
    {
        CRY_ASSERT(nIndex >= 0 && nIndex < (int)m_weights.size());
        m_weights[nIndex] = fWeight;
    }
    void SetWeight(IFacialEffector* pEffector, float fWeight)
    {
        int nIndex = ((CFacialEffector*)pEffector)->m_nIndexInState;
        if (nIndex >= 0 && nIndex < (int)m_weights.size())
        {
            m_weights[nIndex] = fWeight;
        }
    }
    float GetBalance(int nIndex) const
    {
        CRY_ASSERT(nIndex >= 0 && nIndex < (int)m_balance.size());
        return m_balance[nIndex];
    }
    void SetBalance(int nIndex, float fBalance)
    {
        CRY_ASSERT(nIndex >= 0 && nIndex < (int)m_balance.size());
        m_balance[nIndex] = fBalance;
    }
    void Reset();

    void SetNumWeights(int n);
    int  GetNumWeights() { return m_weights.size(); };

    size_t SizeOfThis()
    {
        return sizeofVector(m_weights) * sizeof(float) + sizeofVector(m_balance);
    }

    void GetMemoryUsage (ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_weights);
        pSizer->AddObject(m_balance);
    }

public:
    // Weights of the effectors in state.
    std::vector<float> m_weights;
    // Balances of the effectors in state.
    std::vector<float> m_balance;
    CFacialModel* m_pFacialModel;
};

#endif // CRYINCLUDE_CRYANIMATION_FACIALANIMATION_FACESTATE_H
