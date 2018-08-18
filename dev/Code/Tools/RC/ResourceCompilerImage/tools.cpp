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
#include <platform.h>
#if defined(AZ_PLATFORM_WINDOWS)

#include "stdafx.h"
#include "tools.h"


void CenterWindow(HWND hwndChild, HWND hwndParent)
{
    RECT rChild, rParent;
    int wChild, hChild, wParent, hParent;
    int wScreen, hScreen, xNew, yNew;
    HDC hdc;

    // Get the Height and Width of the child window
    GetWindowRect(hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    // Get the display limits
    hdc = GetDC(hwndChild);
    wScreen = GetDeviceCaps(hdc, HORZRES);
    hScreen = GetDeviceCaps(hdc, VERTRES);
    ReleaseDC(hwndChild, hdc);

    // Get the Height and Width of the parent window
    if (hwndParent)
    {
        GetWindowRect(hwndParent, &rParent);
        wParent = rParent.right - rParent.left;
        hParent = rParent.bottom - rParent.top;
    }
    else // 0 means it's the screen
    {
        wParent = wScreen;
        hParent = hScreen;
        rParent.left = 0;
        rParent.top = 0;
    }

    // Calculate new X position, then adjust for screen
    xNew = rParent.left + ((wParent - wChild) / 2);

    if (xNew < 0)
    {
        xNew = 0;
    }
    else if (xNew + wChild > wScreen)
    {
        xNew = wScreen - wChild;
    }

    // Calculate new Y position, then adjust for screen
    yNew = rParent.top + ((hParent - hChild) / 2);
    if (yNew < 0)
    {
        yNew = 0;
    }
    else if (yNew + hChild > hScreen)
    {
        yNew = hScreen - hChild;
    }

    // Set it, and return
    //  SetWindowPos(hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    MoveWindow(hwndChild, xNew, yNew, wChild, hChild, TRUE);
}


INT_PTR CALLBACK WndProcRedirect(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hParent = GetParent(hWnd);

    if (hParent)
    {
        return SendMessage(hParent, uMsg, wParam, lParam) != 0 ? TRUE : FALSE;
    }

    //  return DefWindowProc( hWnd, uMsg, wParam, lParam );
    return FALSE;
}

#endif //AZ_PLATFORM_WINDOWS
