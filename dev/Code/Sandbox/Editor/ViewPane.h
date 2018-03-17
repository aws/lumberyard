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

#ifndef CRYINCLUDE_EDITOR_VIEWPANE_H
#define CRYINCLUDE_EDITOR_VIEWPANE_H

#pragma once
// ViewPane.h : header file
//

#include "ViewportTitleDlg.h"

#include <AzQtComponents/Components/ToolBarArea.h>

class CViewport;
class QToolBar;

/////////////////////////////////////////////////////////////////////////////
// CViewPane view

class CLayoutViewPane
    : public AzQtComponents::ToolBarArea
{
    Q_OBJECT
public:
    CLayoutViewPane(QWidget* parent = nullptr);
    virtual ~CLayoutViewPane() { OnDestroy(); }

    // Operations
public:
    // Set get this pane id.
    void SetId(int id) { m_id = id; }
    int GetId() { return m_id; }

    void SetViewClass(const QString& sClass);
    QString GetViewClass() const;

    //! Assign viewport to this pane.
    void SwapViewports(CLayoutViewPane* pView);

    void AttachViewport(QWidget* pViewport);
    //! Detach viewport from this pane.
    void DetachViewport();
    // Releases and destroy attached viewport.
    void ReleaseViewport();

    void SetFullscren(bool f);
    bool IsFullscreen() const { return m_bFullscreen; }

    void SetFullscreenViewport(bool b);

    QWidget* GetViewport() { return m_viewport; }

    //////////////////////////////////////////////////////////////////////////
    bool IsActiveView() const { return m_active; }

    void ShowTitleMenu();
    void ToggleMaximize();

    void SetFocusToViewportSearch();

    void ResizeViewport(int width, int height);
    void SetAspectRatio(unsigned int x, unsigned int y);
    void SetViewportFOV(float fov);
    void OnFOVChanged(float fov);

protected:
    void OnMenuLayoutConfig();
    void OnMenuViewSelected(const QString& paneName);
    void OnMenuMaximized();

    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void OnDestroy();
    void focusInEvent(QFocusEvent* event) override;

    void DisconnectRenderViewportInteractionRequestBus();

private:
    QString m_viewPaneClass;
    bool m_bFullscreen;
    CViewportTitleDlg m_viewportTitleDlg;

    int m_id;
    int m_nBorder;
    int m_titleHeight;

    QWidget* m_viewport;
    bool m_active;
};

/////////////////////////////////////////////////////////////////////////////

#endif // CRYINCLUDE_EDITOR_VIEWPANE_H
