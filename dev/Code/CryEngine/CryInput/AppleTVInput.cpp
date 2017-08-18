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
#include "StdAfx.h"

#ifdef USE_APPLETVINPUT

#include "AppleTVInput.h"

#include <ILog.h>
#include <ISystem.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// CAppleTVInput Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
CAppleTVInput::CAppleTVInput(ISystem* pSystem)
    : CBaseInput()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CAppleTVInput::~CAppleTVInput()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAppleTVInput::Init()
{
    gEnv->pLog->Log("Initializing AppleTVInput");

    if (!CBaseInput::Init())
    {
        gEnv->pLog->Log("Error: CBaseInput::Init failed");
        return false;
    }

    // ToDo: AppleTV input needs to handle input from the remote and gamepads: https://issues.labcollab.net/browse/LMBR-24877

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CAppleTVInput::ShutDown()
{
    CBaseInput::ShutDown();
}

#endif // USE_APPLETVINPUT
