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

// Description : Base class for effects managed by the effect system


#include "CryLegacy_precompiled.h"
#include "Effect.h"


bool CEffect::Activating(float delta)
{
    return true;
}
bool CEffect::Update(float delta)
{
    return true;
}
bool CEffect::Deactivating(float delta)
{
    return true;
}

bool CEffect::OnActivate()
{
    return true;
}

bool CEffect::OnDeactivate()
{
    return true;
}

void CEffect::SetState(EEffectState state)
{
    m_state = state;
}

EEffectState CEffect::GetState()
{
    return m_state;
}


