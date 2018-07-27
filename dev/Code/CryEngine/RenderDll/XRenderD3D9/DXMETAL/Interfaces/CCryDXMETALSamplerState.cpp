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

// Description : Definition of the DXGL wrapper for ID3D11SamplerState

#include "StdAfx.h"
#include "CCryDXMETALSamplerState.hpp"
#include "CCryDXMETALDevice.hpp"
#include "../Implementation/GLState.hpp"
#include "../Implementation/MetalDevice.hpp"


CCryDXGLSamplerState::CCryDXGLSamplerState(const D3D11_SAMPLER_DESC& kDesc, CCryDXGLDevice* pDevice)
    : CCryDXGLDeviceChild(pDevice)
    , m_kDesc(kDesc)
    //  Confetti BEGIN: Igor Lobanchikov
    , m_MetalSamplerState(0)
    //  Confetti End: Igor Lobanchikov
{
    DXGL_INITIALIZE_INTERFACE(D3D11SamplerState)
}

CCryDXGLSamplerState::~CCryDXGLSamplerState()
{
    //  Confetti BEGIN: Igor Lobanchikov
    if (m_MetalSamplerState)
    {
        [m_MetalSamplerState release];
    }
    //  Confetti End: Igor Lobanchikov
}

bool CCryDXGLSamplerState::Initialize(CCryDXGLDevice* pDevice)
{
    //  Confetti BEGIN: Igor Lobanchikov
    return NCryMetal::InitializeSamplerState(m_kDesc, m_MetalSamplerState, pDevice->GetGLDevice());
    //  Confetti End: Igor Lobanchikov
}

void CCryDXGLSamplerState::Apply(uint32 uStage, uint32 uSlot, NCryMetal::CContext* pContext)
{
    //  Confetti BEGIN: Igor Lobanchikov
    pContext->SetSampler(m_MetalSamplerState, uStage, uSlot);
    //  Confetti End: Igor Lobanchikov
}


////////////////////////////////////////////////////////////////
// Implementation of ID3D11SamplerState
////////////////////////////////////////////////////////////////

void CCryDXGLSamplerState::GetDesc(D3D11_SAMPLER_DESC* pDesc)
{
    (*pDesc) = m_kDesc;
}