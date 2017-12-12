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

// Description : implementation file


#include "StdAfx.h"
#include "ViewPane.h"
#include "ViewManager.h"

#include "LayoutWnd.h"
#include "Viewport.h"
#include "LayoutConfigDialog.h"
#include "TopRendererWnd.h"

#include "Util/BoostPythonHelpers.h"
#include "UserMessageDefines.h"

#include "MainWindow.h"
#include "QtViewPaneManager.h"

#include <QMouseEvent>
#include <QDebug>
#include <QToolBar>
#include <QLayout>
#include <QLabel>

const int MAX_CLASSVIEWS = 100;
const int MIN_VIEWPORT_RES = 64;

enum TitleMenuCommonCommands
{
    ID_MAXIMIZED = 50000,
    ID_LAYOUT_CONFIG,

    FIRST_ID_CLASSVIEW,
    LAST_ID_CLASSVIEW = FIRST_ID_CLASSVIEW + MAX_CLASSVIEWS - 1
};

#define VIEW_BORDER 0

/////////////////////////////////////////////////////////////////////////////
// CLayoutViewPane

//////////////////////////////////////////////////////////////////////////
CLayoutViewPane::CLayoutViewPane(QWidget* parent)
    : AzQtComponents::ToolBarArea(parent)
    , m_viewportTitleDlg(this)
{
    m_viewport = 0;
    m_active = 0;
    m_nBorder = VIEW_BORDER;

    m_bFullscreen = false;
    m_viewportTitleDlg.SetViewPane(this);

    QWidget* viewportContainer = m_viewportTitleDlg.findChild<QWidget*>(QStringLiteral("ViewportTitleDlgContainer"));
    QToolBar* toolbar = CreateToolBarFromWidget(viewportContainer,
                                                Qt::TopToolBarArea,
                                                QStringLiteral("Viewport Settings"));
    toolbar->setMovable(false);
    toolbar->installEventFilter(&m_viewportTitleDlg);
    toolbar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(toolbar, &QWidget::customContextMenuRequested, &m_viewportTitleDlg, &QWidget::customContextMenuRequested);

    setContextMenuPolicy(Qt::NoContextMenu);

    m_id = -1;
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::SetViewClass(const QString& sClass)
{
    if (m_viewport && m_viewPaneClass == sClass)
    {
        return;
    }
    m_viewPaneClass = sClass;

    ReleaseViewport();

    QWidget* newPane = QtViewPaneManager::instance()->CreateWidget(sClass);
    if (newPane)
    {
        newPane->setProperty("IsViewportWidget", true);
        connect(newPane, &QWidget::windowTitleChanged, &m_viewportTitleDlg, &CViewportTitleDlg::SetTitle, Qt::UniqueConnection);
        AttachViewport(newPane);
    }
}

//////////////////////////////////////////////////////////////////////////
QString CLayoutViewPane::GetViewClass() const
{
    return m_viewPaneClass;
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::OnDestroy()
{
    ReleaseViewport();
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::SwapViewports(CLayoutViewPane* pView)
{
    QWidget* pViewport = pView->GetViewport();
    QWidget* pViewportOld = m_viewport;

    std::swap(m_viewPaneClass, pView->m_viewPaneClass);

    AttachViewport(pViewport);
    pView->AttachViewport(pViewportOld);
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::AttachViewport(QWidget* pViewport)
{
    if (pViewport == m_viewport)
    {
        return;
    }

    DisconnectRenderViewportInteractionRequestBus();
    m_viewport = pViewport;
    if (pViewport)
    {
        m_viewport->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        SetMainWidget(m_viewport);

        if (QtViewport* vp = qobject_cast<QtViewport*>(pViewport))
        {
            vp->SetViewportId(GetId());
            vp->SetViewPane(this);
            if (CRenderViewport* renderViewport = viewport_cast<CRenderViewport*>(vp))
            {
                renderViewport->ConnectViewportInteractionRequestBus();
            }
        }

        m_viewport->setVisible(true);

        setWindowTitle(m_viewPaneClass);
        m_viewportTitleDlg.SetTitle(pViewport->windowTitle());

        if (QtViewport* vp = qobject_cast<QtViewport*>(pViewport))
        {
            OnFOVChanged(vp->GetFOV());
        }
        else
        {
            OnFOVChanged(gSettings.viewports.fDefaultFov);
        }

        m_viewportTitleDlg.OnViewportSizeChanged(pViewport->width(), pViewport->height());
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::DetachViewport()
{
    DisconnectRenderViewportInteractionRequestBus();
    OnFOVChanged(gSettings.viewports.fDefaultFov);
    m_viewport = 0;
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::ReleaseViewport()
{
    if (m_viewport)
    {
        DisconnectRenderViewportInteractionRequestBus();
        m_viewport->deleteLater();
        m_viewport = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::resizeEvent(QResizeEvent* event)
{
    AzQtComponents::ToolBarArea::resizeEvent(event);

    int toolHeight = 0;

    if (m_viewport)
    {
        m_viewportTitleDlg.OnViewportSizeChanged(m_viewport->width(), m_viewport->height());
    }
}

void CLayoutViewPane::DisconnectRenderViewportInteractionRequestBus()
{
    if (QtViewport* vp = qobject_cast<QtViewport*>(m_viewport))
    {
        if (CRenderViewport* renderViewport = viewport_cast<CRenderViewport*>(vp))
        {
            renderViewport->DisconnectViewportInteractionRequestBus();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::ResizeViewport(int width, int height)
{
    assert(width >= MIN_VIEWPORT_RES && height >= MIN_VIEWPORT_RES);
    if (!m_viewport)
    {
        return;
    }

    QWidget* mainWindow = MainWindow::instance();
    if (mainWindow->isMaximized())
    {
        mainWindow->showNormal();
    }

    const QSize deltaSize = QSize(width, height) - m_viewport->size();
    mainWindow->move(0, 0);
    mainWindow->resize(mainWindow->size() + deltaSize);
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::SetAspectRatio(unsigned int x, unsigned int y)
{
    if (x == 0 || y == 0)
    {
        return;
    }

    const QRect viewportRect = m_viewport->rect();

    // Round height to nearest multiple of y aspect, then scale width according to ratio
    const unsigned int height = ((viewportRect.height() + y - 1) / y) * y;
    const unsigned int width = height / y * x;

    ResizeViewport(width, height);
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::SetViewportFOV(float fov)
{
    if (CRenderViewport* pRenderViewport = qobject_cast<CRenderViewport*>(m_viewport))
    {
        pRenderViewport->SetFOV(DEG2RAD(fov));

        // if viewport camera is active, make selected fov new default
        if (pRenderViewport->GetViewManager()->GetCameraObjectId() == GUID_NULL)
        {
            gSettings.viewports.fDefaultFov = DEG2RAD(fov);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::ToggleMaximize()
{
    ////////////////////////////////////////////////////////////////////////
    // Switch in and out of fullscreen mode for a edit view
    ////////////////////////////////////////////////////////////////////////
    CLayoutWnd* wnd = GetIEditor()->GetViewManager()->GetLayout();
    if (wnd)
    {
        wnd->MaximizeViewport(GetId());
    }
    setFocus();
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::OnMenuLayoutConfig()
{
    if (GetIEditor()->IsInGameMode())
    {
        // you may not change your viewports while game mode is running.
        CryLog("You may not change viewport configuration while in game mode.");
        return;
    }

    CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
    if (layout)
    {
        CLayoutConfigDialog dlg;
        dlg.SetLayout(layout->GetLayout());
        if (dlg.exec() == QDialog::Accepted)
        {
            // Will kill this Pane. so must be last line in this function.
            layout->CreateLayout(dlg.GetLayout());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::OnMenuViewSelected(const QString& paneName)
{
    if (GetIEditor()->IsInGameMode())
    {
        CryLog("You may not change viewport configuration while in game mode.");
        // you may not change your viewports while game mode is running.
        return;
    }

    CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
    if (layout)
    {
        layout->BindViewport(this, paneName);
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::OnMenuMaximized()
{
    CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
    if (m_viewport && layout)
    {
        layout->MaximizeViewport(GetId());
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::ShowTitleMenu()
{
    ////////////////////////////////////////////////////////////////////////
    // Process clicks on the view buttons and the menu button
    ////////////////////////////////////////////////////////////////////////
    // Only continue when we have a viewport.

    // Create pop up menu.
    QMenu root(this);
    if (QtViewport* vp = qobject_cast<QtViewport*>(m_viewport))
    {
        vp->OnTitleMenu(&root);
    }

    if (!root.isEmpty())
    {
        root.addSeparator();
    }

    CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
    QAction* action = root.addAction(tr("Maximized"));
    if (layout)
    {
        connect(action, &QAction::triggered, layout, &CLayoutWnd::MaximizeViewport);
    }
    action->setChecked(IsFullscreen());
    QMenu* viewsMenu = root.addMenu(tr("Viewport Type"));
    action = root.addAction(tr("Configure Layout..."));

    // NOTE: this must be a QueuedConnection, so that it executes after the menu is deleted.
    // Changing the layout can cause the current "this" pointer to be deleted
    // and since we've made the "this" pointer the menu's parent,
    // it gets deleted when the "this" pointer is deleted. Since it's not a heap object,
    // that causes a crash. Using a QueuedConnection forces the layout config to happen
    // after the QMenu is cleaned up on the stack.
    connect(action, &QAction::triggered, this, &CLayoutViewPane::OnMenuLayoutConfig, Qt::QueuedConnection);

    QtViewPanes viewports = QtViewPaneManager::instance()->GetRegisteredViewportPanes();

    for (auto it = viewports.cbegin(), end = viewports.cend(); it != end; ++it)
    {
        const QtViewPane& pane = *it;
        action = viewsMenu->addAction(pane.m_name);
        action->setCheckable(true);
        action->setChecked(m_viewPaneClass == pane.m_name);
        connect(action, &QAction::triggered, [pane, this] { OnMenuViewSelected(pane.m_name); });
    }

    root.exec(QCursor::pos());
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        ToggleMaximize();
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::focusInEvent(QFocusEvent* event)
{
    // Forward SetFocus call to child viewport.
    if (m_viewport)
    {
        m_viewport->setFocus();
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::SetFullscren(bool f)
{
    m_bFullscreen = f;
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::SetFullscreenViewport(bool b)
{
    if (!m_viewport)
    {
        return;
    }

    if (b)
    {
        m_viewport->setParent(0);

        GetIEditor()->GetRenderer()->ChangeResolution(800, 600, 32, 80, true, false);
    }
    else
    {
        m_viewport->setParent(this);
        GetIEditor()->GetRenderer()->ChangeResolution(800, 600, 32, 80, false, false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::SetFocusToViewportSearch()
{
    m_viewportTitleDlg.SetFocusToSearchField();
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::OnFOVChanged(float fov)
{
    m_viewportTitleDlg.OnViewportFOVChanged(fov);
}

//////////////////////////////////////////////////////////////////////////
namespace
{
    boost::python::tuple PyGetViewPortSize()
    {
        CLayoutViewPane* viewPane = MainWindow::instance()->GetActiveView();
        if (viewPane)
        {
            const QRect rcViewport = viewPane->rect();
            return boost::python::make_tuple(rcViewport.width(), rcViewport.height());
        }
        else
        {
            return boost::python::make_tuple(0, 0);
        }
    }

    static void PySetViewPortSize(int width, int height)
    {
        CLayoutViewPane* viewPane = MainWindow::instance()->GetActiveView();
        if (viewPane)
        {
            viewPane->ResizeViewport(width, height);
        }
    }

    static void PyUpdateViewPort()
    {
        GetIEditor()->UpdateViews(eRedrawViewports);
    }

    void PyResizeViewport(int width, int height)
    {
        CLayoutViewPane* viewPane = MainWindow::instance()->GetActiveView();
        ;
        if (viewPane)
        {
            viewPane->ResizeViewport(width, height);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void PyBindViewport(const char* viewportName)
    {
        CLayoutViewPane* viewPane = MainWindow::instance()->GetActiveView();
        if (viewPane)
        {
            GetIEditor()->GetViewManager()->GetLayout()->BindViewport(viewPane, viewportName);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
REGISTER_PYTHON_COMMAND(PyGetViewPortSize, general, get_viewport_size, "Get the width and height of the active viewport.");
REGISTER_PYTHON_COMMAND(PySetViewPortSize, general, set_viewport_size, "Set the width and height of the active viewport.");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyUpdateViewPort, general, update_viewport,
    "Update all visible SDK viewports.",
    "general.update_viewport()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyResizeViewport, general, resize_viewport,
    "Resizes the viewport resolution to a given width & hegiht.",
    "general.resize_viewport(int width, int height)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyBindViewport, general, bind_viewport,
    "Binds the viewport to a specific view like 'Top', 'Front', 'Perspective'.",
    "general.bind_viewport(str viewportName)");

#include <ViewPane.moc>
