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

// Description : Interface for extending frame windows with custom dock panes.


#ifndef CRYINCLUDE_EDITOR_INCLUDE_IFRAMEWINDOWEXTENSIONPANE_H
#define CRYINCLUDE_EDITOR_INCLUDE_IFRAMEWINDOWEXTENSIONPANE_H
#pragma once


//////////////////////////////////////////////////////////////////////////
// Description
//    This interface is used in frame windows to add a new custom
//    docking pane to then from the plugins.
//////////////////////////////////////////////////////////////////////////
struct IFrameWindowExtensionPane
    : public IUnknown
{
    DEFINE_UUID(0x991A2772, 0xB5A1, 0x440d, 0x8D, 0x9C, 0xB1, 0xCE, 0xA5, 0xF8, 0x3A, 0x0A)

    // Return name of the frame window this extension is designed for.
    virtual const char* SupportedFrameWindow() const = 0;

    // Creates a window for the extension docking pane.
    virtual CWnd* CreatePaneWindow(CWnd* pParentWnd) = 0;

    //////////////////////////////////////////////////////////////////////////
    // optional callbacks that pane may want to handle.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnAttachToDockingPane(CXTPDockingPane* pwndDockWindow, CWnd* pPaneWnd) {};
    virtual void OnFileLoad(const char* filepath) {};
    virtual void SaveFile(const char* filepath) {};
    // Called to check if data in the pane needs to be saved.
    virtual bool IsModified() { return false; };
    // Called when view of the frame window is updating.
    virtual void OnViewUpdate() {};
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // IUnknown
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) { return E_NOINTERFACE; };
    virtual ULONG       STDMETHODCALLTYPE AddRef()  { return 0; };
    virtual ULONG       STDMETHODCALLTYPE Release() { return 0; };
    //////////////////////////////////////////////////////////////////////////
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IFRAMEWINDOWEXTENSIONPANE_H
