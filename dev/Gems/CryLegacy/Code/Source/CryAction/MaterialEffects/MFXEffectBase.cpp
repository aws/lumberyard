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
#include "MFXEffectBase.h"

#include <IMaterialEffects.h>

CMFXEffectBase::CMFXEffectBase(const uint16 _typeFilterFlag)
{
    m_runtimeExecutionFilter.AddFlags(_typeFilterFlag);
}

void CMFXEffectBase::SetCustomParameter(const char* customParameter, const SMFXCustomParamValue& customParameterValue)
{
}

void CMFXEffectBase::PreLoadAssets()
{
}

void CMFXEffectBase::ReleasePreLoadAssets()
{
}

bool CMFXEffectBase::CanExecute(const SMFXRunTimeEffectParams& params) const
{
    return ((params.playflags & m_runtimeExecutionFilter.GetRawFlags()) != 0);
}

