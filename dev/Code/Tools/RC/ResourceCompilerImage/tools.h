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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_TOOLS_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_TOOLS_H
#pragma once

#include <platform.h>
#if defined(AZ_PLATFORM_WINDOWS)
// FUNCTION: CenterWindow (HWND, HWND)
// PURPOSE:  Center one window over another
// COMMENTS:
//      Dialog boxes take on the screen position that they were designed at,
//      which is not always appropriate. Centering the dialog over a particular
//      window usually results in a better position.
extern void CenterWindow(HWND hwndChild, HWND hwndParent = 0);

// redirect to the parent window
extern INT_PTR CALLBACK WndProcRedirect(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif //AZ_PLATFORM_WINDOWS
#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_TOOLS_H
