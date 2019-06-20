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
#pragma once

#include <QByteArray>
#include <QMainWindow>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Styling/StyleManager.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorCentralWidget.h>

namespace GraphCanvas
{
    class AssetEditorMainWindow
        : public QMainWindow
        , public GraphCanvas::SceneNotificationBus::MultiHandler
        , public GraphCanvas::SceneUIRequestBus::MultiHandler
        , private GraphCanvas::AssetEditorRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(AssetEditorMainWindow, AZ::SystemAllocator, 0);
        
        AssetEditorMainWindow(const EditorId& editorId, AZStd::string_view baseStyleSheet);
        virtual ~AssetEditorMainWindow();
        
        void SetupUI();
        void SetDropAreaText(AZStd::string_view text);        

        DockWidgetId CreateEditorDockWidget();
        
    protected:

        virtual EditorDockWidget* CreateDockWidget(QWidget* parent) const = 0;
        virtual void ConfigureDefaultLayout() = 0;
        
    private:
    
        void SetDefaultLayout();

        void RestoreWindowState();
        void SaveWindowState();

        AssetEditorCentralDockWindow* GetCentralDockWindow() const;
        
        EditorId m_editorId;
        StyleManager m_styleManager;
    };
}
