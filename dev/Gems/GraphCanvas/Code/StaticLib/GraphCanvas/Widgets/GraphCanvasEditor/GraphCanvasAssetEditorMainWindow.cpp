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
#include <qlayout.h>
#include <qtimer.h>

#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasAssetEditorMainWindow.h>

#include <GraphCanvas/Styling/StyleManager.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorCentralWidget.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorDockWidget.h>

namespace GraphCanvas
{
    //////////////////////////
    // AssetEditorMainWindow
    //////////////////////////   
    
    AssetEditorMainWindow::AssetEditorMainWindow(const EditorId& editorId, AZStd::string_view baseStyle)
        : QMainWindow(nullptr, Qt::Widget | Qt::WindowMinMaxButtonsHint)
        , m_styleManager(editorId, baseStyle)
        , m_editorId(editorId)
    {        
        QTimer::singleShot(0, [this]() {
            SetDefaultLayout();

            RestoreWindowState();
            SaveWindowState();
        });
    }
    
    AssetEditorMainWindow::~AssetEditorMainWindow()
    {
        SaveWindowState();
    }

    void AssetEditorMainWindow::SetupUI()
    {
        setDockNestingEnabled(false);
        setDockOptions(dockOptions());
        setTabPosition(Qt::DockWidgetArea::AllDockWidgetAreas, QTabWidget::TabPosition::North);

        AssetEditorCentralDockWindow* centralWidget = aznew GraphCanvas::AssetEditorCentralDockWindow(m_editorId);
        setCentralWidget(centralWidget);
    }
    
    void AssetEditorMainWindow::SetDropAreaText(AZStd::string_view text)
    {
        static_cast<AssetEditorCentralDockWindow*>(centralWidget())->GetEmptyDockWidget()->SetDragTargetText(text.data());
    }

    DockWidgetId AssetEditorMainWindow::CreateEditorDockWidget()
    {
        EditorDockWidget* dockWidget = CreateDockWidget(this);

        AssetEditorCentralDockWindow* centralDockWindow = GetCentralDockWindow();
        centralDockWindow->OnEditorOpened(dockWidget);

        return dockWidget->GetDockWidgetId();
    }

    void AssetEditorMainWindow::SetDefaultLayout()
    {
        ConfigureDefaultLayout();
    }

    void AssetEditorMainWindow::RestoreWindowState()
    {

    }

    void AssetEditorMainWindow::SaveWindowState()
    {

    }

    AssetEditorCentralDockWindow* AssetEditorMainWindow::GetCentralDockWindow() const
    {
        return static_cast<AssetEditorCentralDockWindow*>(centralWidget());
    }
}