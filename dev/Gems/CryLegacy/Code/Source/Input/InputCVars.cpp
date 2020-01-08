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
#include "InputCVars.h"
#include <IConsole.h>
#include <ISystem.h>

CInputCVars* g_pInputCVars = 0;

CInputCVars::CInputCVars()
{
    REGISTER_CVAR(i_debug, 0, 0,
        "Toggles input event debugging.\n"
        "Usage: i_debug [0/1]\n"
        "Default is 0 (off). Set to 1 to spam console with key events (only press and release).");

    REGISTER_CVAR(i_debugDigitalButtons, 0, 0,
        "render controller's digital button pressed info\n"
        "Usage: 0 (off), 1(on)"
        "Default is 0. Value must be >=0");

    i_synergyServer = REGISTER_STRING("i_synergyServer", "", VF_NULL,
            "Usage: Set the host ip or hostname for the synergy client to connect to\n"
            "Default is empty ");
    i_synergyScreenName = REGISTER_STRING("i_synergyScreenName", "Console", VF_NULL,
            "Usage: Set a screen name for this target\n"
            "Default is \"Console\"");
}

CInputCVars::~CInputCVars()
{
    gEnv->pConsole->UnregisterVariable("i_debug");
    gEnv->pConsole->UnregisterVariable("i_debugDigitalButtons");
}
