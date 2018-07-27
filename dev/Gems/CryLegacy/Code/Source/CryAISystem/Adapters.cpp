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

// Description : Implements adapters for AI objects from external interfaces to internal
//               This is purely a translation layer without concrete instances
//               They can have no state and must remain abstract


#include "CryLegacy_precompiled.h"

#include "Adapters.h"

CWeakRef<CAIObject> GetWeakRefSafe(IAIObject* pObj) { return (pObj ? GetWeakRef((CAIObject*)pObj) : NILREF); }

IAIObject*  CAIGroupAdapter::GetAttentionTarget(bool bHostileOnly, bool bLiveOnly) const
{
    CWeakRef<CAIObject> refTarget = GetAttentionTarget(bHostileOnly, bLiveOnly, NILREF);
    return refTarget.GetIAIObject();
}
