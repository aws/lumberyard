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

// Description: System "Hardware mouse" cursor with reference counter.
//              This is needed because Menus / HUD / Profiler / or whatever
//              can use the cursor not at the same time be successively
//              => We need to know when to enable/disable the cursor.

#ifndef CRYINCLUDE_CRYCOMMON_IHARDWAREMOUSE_H
#define CRYINCLUDE_CRYCOMMON_IHARDWAREMOUSE_H
#pragma once

#include <AzFramework/Input/System/InputSystemComponent.h>

//-----------------------------------------------------------------------------------------------------
#if defined(AZ_FRAMEWORK_INPUT_ENABLED)
struct IHardwareMouse
{
    virtual ~IHardwareMouse() {}
    virtual void Release() = 0;

    virtual bool SetCursor(const char* cursorPath) = 0;
    virtual void SetGameMode(bool gameMode) = 0; // Should only be called from the editor

    virtual void IncrementCounter() = 0;
    virtual void DecrementCounter() = 0;
    virtual bool IsHidden() = 0;

    virtual void Update() = 0;
    virtual void Render() = 0;
};
#else
struct IHardwareMouse
{
    // <interfuscator:shuffle>
    virtual ~IHardwareMouse(){}

    virtual void Release() = 0;

    // We need to register after the creation of the device but before its init
    virtual void OnPreInitRenderer() = 0;

    // We need to register after the creation of input to emulate mouse
    virtual void OnPostInitInput() = 0;

    // Called only in Editor when switching from editing to game mode
    virtual void SetGameMode(bool bGameMode) = 0;

    // Increment when you want to show the cursor, decrement otherwise
    virtual void IncrementCounter() = 0;
    virtual void DecrementCounter() = 0;

    // Standard get/set functions, mainly for Gamepad emulation purpose
    virtual void GetHardwareMousePosition(float* pfX, float* pfY) = 0;
    virtual void SetHardwareMousePosition(float fX, float fY) = 0;

    // Same as above, but relative to upper-left corner to the client area of our app
    virtual void GetHardwareMouseClientPosition(float* pfX, float* pfY) = 0;
    virtual void SetHardwareMouseClientPosition(float fX, float fY) = 0;

    // Load and/or set cursor
    virtual bool SetCursor(int idc_cursor_id) = 0;
    virtual bool SetCursor(const char* path) = 0;

    virtual void Reset(bool bVisibleByDefault) = 0;
    virtual void ConfineCursor(bool confine) = 0;
    virtual void Hide(bool hide) = 0;
    virtual bool IsHidden() = 0;

    // When set, HardwareMouse will remember the last cursor position before hiding.
    // On show, HardwareMouse will restore the position of the mouse.
    virtual void RememberLastCursorPosition(bool remember) = 0;
    virtual void Update() = 0;
    virtual void Render() = 0;
    // </interfuscator:shuffle>

#ifdef WIN32
    virtual void UseSystemCursor(bool useSystemCursor) = 0;
#endif

    virtual ISystemEventListener* GetSystemEventListener() = 0;
};
#endif // defined(AZ_FRAMEWORK_INPUT_ENABLED)
//-----------------------------------------------------------------------------------------------------

#endif // CRYINCLUDE_CRYCOMMON_IHARDWAREMOUSE_H

//-----------------------------------------------------------------------------------------------------
