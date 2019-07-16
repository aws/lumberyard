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

#include <QWidget>
#include <QMainWindow>
#include <qdockwidget.h>
#include <qmimedata.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>

#include <GraphCanvas/Styling/StyleManager.h>

namespace Ui
{
    class GraphCanvasEditorCentralWidget;
}

namespace GraphCanvas
{
    class EditorDockWidget;

    class GraphCanvasEditorEmptyDockWidget
        : public QDockWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphCanvasEditorEmptyDockWidget, AZ::SystemAllocator,0);
        GraphCanvasEditorEmptyDockWidget(QWidget* parent);
        ~GraphCanvasEditorEmptyDockWidget();
        
        void SetDragTargetText(const AZStd::string& dragTargetString);

        void RegisterAcceptedMimeType(const QString& mimeType);
        
        void SetEditorId(const EditorId& editorId);
        const EditorId& GetEditorId() const;
        
        // QWidget
        void dragEnterEvent(QDragEnterEvent* enterEvent) override;
        void dragMoveEvent(QDragMoveEvent* moveEvent) override;
        void dropEvent(QDropEvent* dropEvent) override;
        ////
        
    protected:        
    private:

        bool AcceptsMimeData(const QMimeData* mimeData) const;

        QMimeData   m_initialDropMimeData;
    
        EditorId m_editorId;
        AZStd::unique_ptr<Ui::GraphCanvasEditorCentralWidget> m_ui;

        bool m_allowDrop;
        AZStd::vector< QString > m_mimeTypes;
    };

    class AssetEditorCentralDockWindow
        : public QMainWindow
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AssetEditorCentralDockWindow, AZ::SystemAllocator, 0);

        AssetEditorCentralDockWindow(const EditorId& editorId);
        ~AssetEditorCentralDockWindow();

        GraphCanvasEditorEmptyDockWidget* GetEmptyDockWidget() const;

        void OnEditorOpened(EditorDockWidget* dockWidget);
        void OnEditorClosed(EditorDockWidget* dockWidget);

        void OnEditorDockChanged(bool isDocked);

        void OnFocusChanged(QWidget* oldWidget, QWidget* nowFocus);

    protected:

        void UpdateCentralWidget();

    private:

        EditorId m_editorId;

        GraphCanvasEditorEmptyDockWidget*    m_emptyDockWidget;
        AZStd::unordered_set< QDockWidget* > m_editorDockWidgets;
    };
}