/*
* All or Portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <qevent.h>

#include <AzCore/Component/Entity.h>

#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorDockWidget.h>
#include <StaticLib/GraphCanvas/Widgets/GraphCanvasEditor/ui_GraphCanvasEditorDockWidget.h>

namespace GraphCanvas
{
    /////////////////////
    // EditorDockWidget
    /////////////////////

    static int counter = 0;
    
    EditorDockWidget::EditorDockWidget(const EditorId& editorId, QWidget* parent)
        : QDockWidget(parent)
        , m_editorId(editorId)
        , m_ui(new Ui::GraphCanvasEditorDockWidget())        
    {
        m_ui->setupUi(this);
        m_ui->graphicsView->SetEditorId(editorId);

        setAllowedAreas(Qt::DockWidgetArea::TopDockWidgetArea);
       
        m_dockWidgetId = AZ::Entity::MakeId();

        EditorDockWidgetRequestBus::Handler::BusConnect(m_dockWidgetId);

        setWindowTitle(QString("Window %1").arg(counter));
        windowId = counter;
        ++counter;
    }
    
    EditorDockWidget::~EditorDockWidget()
    {
    }
    
    DockWidgetId EditorDockWidget::GetDockWidgetId() const
    {
        return m_dockWidgetId;
    }

    void EditorDockWidget::ReleaseBus()
    {
        ActiveEditorDockWidgetRequestBus::Handler::BusDisconnect(GetEditorId());
    }
    
    const EditorId& EditorDockWidget::GetEditorId() const
    {
        return m_editorId;
    }

    AZ::EntityId EditorDockWidget::GetViewId() const
    {
        return m_ui->graphicsView->GetViewId();
    }

    GraphId EditorDockWidget::GetGraphId() const
    {
        GraphId graphId;
        ViewRequestBus::EventResult(graphId, GetViewId(), &ViewRequests::GetScene);

        return graphId;
    }

    EditorDockWidget* EditorDockWidget::AsEditorDockWidget()
    {
        return this;
    }

    GraphCanvasGraphicsView* EditorDockWidget::GetGraphicsView() const
    {
        return m_ui->graphicsView;
    }

    void EditorDockWidget::closeEvent(QCloseEvent* closeEvent)
    {
        emit OnEditorClosed(this);

        QDockWidget::closeEvent(closeEvent);        
    }

    void EditorDockWidget::SignalActiveEditor()
    {
        if (!ActiveEditorDockWidgetRequestBus::Handler::BusIsConnected())
        {
            ActiveEditorDockWidgetRequestBus::Event(GetEditorId(), &ActiveEditorDockWidgetRequests::ReleaseBus);
            ActiveEditorDockWidgetRequestBus::Handler::BusConnect(GetEditorId());
        }
    }

#include <StaticLib/GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorDockWidget.moc>
}