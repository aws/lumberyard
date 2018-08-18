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
#include "MannequinUtils.h"

#include <ICryMannequin.h>

const char* mannequin::FindProcClipTypeName(const IProceduralClipFactory::THash& typeNameHash)
{
    if (gEnv && gEnv->pGame && gEnv->pGame->GetIGameFramework())
    {
        const IProceduralClipFactory& proceduralClipFactory = gEnv->pGame->GetIGameFramework()->GetMannequinInterface().GetProceduralClipFactory();
        const char* const typeName = proceduralClipFactory.FindTypeName(typeNameHash);
        return typeName;
    }
    return NULL;
}
