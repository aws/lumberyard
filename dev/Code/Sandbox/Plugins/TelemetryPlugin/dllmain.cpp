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

#include "stdafx.h"
#include <afxwin.h>
#include <afxdllx.h>

static AFX_EXTENSION_MODULE TelemetryPluginDLL = { NULL, NULL };

extern "C" int APIENTRY DllMain2(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        if (!AfxInitExtensionModule(TelemetryPluginDLL, hInstance))
        {
            return 0;
        }

        new CDynLinkLibrary(TelemetryPluginDLL);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        AfxTermExtensionModule(TelemetryPluginDLL);
    }
    return 1;   // ok
}
