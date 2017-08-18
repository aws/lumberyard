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

#include "StdAfx.h"
#include "CryAction.h"
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/alignment_of.h>

#define CRYACTION_API DLL_EXPORT

extern "C"
{
CRYACTION_API IGameFramework* CreateGameFramework()
{
    // at this point... we have no dynamic memory allocation, and we cannot
    // rely on atexit() doing the right thing; the only recourse is to
    // have a static buffer that we use for this object
    static AZStd::aligned_storage<sizeof(CCryAction), AZStd::alignment_of<CCryAction>::value>::type cryAction_buffer;
    return new (&cryAction_buffer)CCryAction();
}   
}
