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
// Description : System "Hardware mouse" cursor with reference counter.
//               This is needed because Menus / HUD / Profiler / or whatever
//               can use the cursor not at the same time be successively
//               => We need to know when to enable/disable the cursor.

#ifndef CRYINCLUDE_CRYSYSTEM_HARDWAREMOUSE_H
#define CRYINCLUDE_CRYSYSTEM_HARDWAREMOUSE_H
#pragma once


//-----------------------------------------------------------------------------------------------------

#include "IRenderer.h"
#include "IHardwareMouse.h"

//-----------------------------------------------------------------------------------------------------
#if defined(AZ_FRAMEWORK_INPUT_ENABLED)
class HardwareMouse : public IHardwareMouse
{
public:
    HardwareMouse(bool visibleByDefault, const char* defaultCursorPath = nullptr);
    ~HardwareMouse() override;
    void Release() override;

    bool SetCursor(const char* cursorPath) override;
    void SetGameMode(bool gameMode) override;

    void IncrementCounter() override;
    void DecrementCounter() override;
    bool IsHidden() override;

    void Update() override;
    void Render() override;

private:
    ITexture* m_cursorTexture;
    float     m_cursorPositionX;
    float     m_cursorPositionY;
    int       m_visibleCounter;
};
#else
class CHardwareMouse :
      public IInputEventListener
    , public IHardwareMouse
    , public ISystemEventListener 
    
{
public:

    // IInputEventListener
    virtual bool OnInputEvent(const SInputEvent& rInputEvent);
    // ~IInputEventListener

    // ISystemEventListener
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
    // ~ISystemEventListener

    // IHardwareMouse
    virtual void Release();
    virtual void OnPreInitRenderer();
    virtual void OnPostInitInput();
    virtual void SetGameMode(bool bGameMode);
    virtual void IncrementCounter();
    virtual void DecrementCounter();
    virtual void GetHardwareMousePosition(float* pfX, float* pfY);
    virtual void SetHardwareMousePosition(float fX, float fY);
    virtual void GetHardwareMouseClientPosition(float* pfX, float* pfY);
    virtual void SetHardwareMouseClientPosition(float fX, float fY);
    virtual void Reset(bool bVisibleByDefault);
    virtual void ConfineCursor(bool confine);
    virtual void Hide(bool hide);
    virtual bool IsHidden();
    virtual void RememberLastCursorPosition(bool remember);
    virtual bool SetCursor(int idc_cursor_id);
    virtual bool SetCursor(const char* path);
#ifdef WIN32
    virtual void UseSystemCursor(bool useWin32Cursor);
#endif
    virtual ISystemEventListener* GetSystemEventListener() { return this; }

    void Update();
    void Render();
    // ~IHardwareMouse

    CHardwareMouse(bool bVisibleByDefault);
    ~CHardwareMouse();

private:
    void ShowHardwareMouse(bool bShow);
    static bool IsFullscreen();
    void DestroyCursor();

    ITexture* m_pCursorTexture;
    int m_iReferenceCounter;
    float m_fCursorX;
    float m_fCursorY;
    float m_fIncX;
    float m_fIncY;
    bool    m_bFocus;
    bool    m_recapture;
    const bool m_allowConfine;

    string m_curCursorPath;

#ifdef WIN32
    bool m_useSystemCursor, m_shouldUseSystemCursor;
    HCURSOR m_hCursor;
    int m_nCurIDCCursorId;
#else
    float m_fVirtualX;
    float m_fVirtualY;
#endif

    bool m_rememberLastPosition;
    bool m_hide;
    bool m_calledShowHWMouse;
    int m_debugHardwareMouse;
};
#endif // defined(AZ_FRAMEWORK_INPUT_ENABLED)
//-----------------------------------------------------------------------------------------------------

#endif // CRYINCLUDE_CRYSYSTEM_HARDWAREMOUSE_H

//-----------------------------------------------------------------------------------------------------
