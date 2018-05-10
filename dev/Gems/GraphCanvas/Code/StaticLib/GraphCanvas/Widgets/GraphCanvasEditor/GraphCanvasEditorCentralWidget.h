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
#include <qmimedata.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>

namespace Ui
{
    class GraphCanvasEditorCentralWidget;
}

namespace GraphCanvas
{
    class EditorDockWidget;

    class GraphCanvasEditorCentralWidget
        : public QWidget        
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphCanvasEditorCentralWidget, AZ::SystemAllocator,0);
        GraphCanvasEditorCentralWidget(QWidget* parent);
        ~GraphCanvasEditorCentralWidget();
        
        void SetDragTargetText(const AZStd::string& dragTargetString);

        void RegisterAcceptedMimeType(const QString& mimeType);
        
        void SetEditorId(const EditorId& editorId);
        const EditorId& GetEditorId() const;
        
        GraphCanvas::DockWidgetId CreateNewEditor();
        
        // QWidget
        void childEvent(QChildEvent* event) override;

        void dragEnterEvent(QDragEnterEvent* enterEvent) override;
        void dragMoveEvent(QDragMoveEvent* moveEvent) override;
        void dropEvent(QDropEvent* dropEvent) override;
        ////
        
    protected:
    
        virtual EditorDockWidget* CreateEditorDockWidget(const EditorId& editorId);
        
    private:

        bool AcceptsMimeData(const QMimeData* mimeData) const;

        QMimeData   m_initialDropMimeData;
        int m_emptyChildrenCount;
    
        EditorId m_editorId;
        AZStd::unique_ptr<Ui::GraphCanvasEditorCentralWidget> m_ui;

        bool m_allowDrop;
        AZStd::vector< QString > m_mimeTypes;
    };
}