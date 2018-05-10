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
#include <qevent.h>
#include <qmimedata.h>
#include <qtimer.h>
#include <qgraphicsview.h>

#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorCentralWidget.h>
#include <StaticLib/GraphCanvas/Widgets/GraphCanvasEditor/ui_GraphCanvasEditorCentralWidget.h>

#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorDockWidget.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

namespace GraphCanvas
{
    ///////////////////////////////////
    // GraphCanvasEditorCentralWidget
    ///////////////////////////////////
    GraphCanvasEditorCentralWidget::GraphCanvasEditorCentralWidget(QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::GraphCanvasEditorCentralWidget())
        , m_emptyChildrenCount(-1)
    {
        m_ui->setupUi(this);

        setAcceptDrops(true);

        m_emptyChildrenCount = children().count();
    }
    
    GraphCanvasEditorCentralWidget::~GraphCanvasEditorCentralWidget()
    {        
    }
        
    void GraphCanvasEditorCentralWidget::SetDragTargetText(const AZStd::string& dragTargetString)
    {
        m_ui->dropTarget->setText(dragTargetString.c_str());
    }

    void GraphCanvasEditorCentralWidget::RegisterAcceptedMimeType(const QString& mimeType)
    {
        m_mimeTypes.emplace_back(mimeType);
    }
    
    void GraphCanvasEditorCentralWidget::SetEditorId(const EditorId& editorId)
    {
        AZ_Warning("GraphCanvas", m_editorId == EditorId() || m_editorId == editorId, "Trying to re-use the same Central widget in two different editors.");
        
        m_editorId = editorId;
    }
    
    const EditorId& GraphCanvasEditorCentralWidget::GetEditorId() const
    {
        return m_editorId;
    }

    GraphCanvas::DockWidgetId GraphCanvasEditorCentralWidget::CreateNewEditor()
    {
        return CreateEditorDockWidget(GetEditorId())->GetDockWidgetId();
    }

    void GraphCanvasEditorCentralWidget::childEvent(QChildEvent* event)
    {
        QWidget::childEvent(event);

        if (m_emptyChildrenCount > 0)
        {
            m_ui->dropTarget->setVisible(children().size() == m_emptyChildrenCount);
        }
    }

    void GraphCanvasEditorCentralWidget::dragEnterEvent(QDragEnterEvent* enterEvent)
    {
        const QMimeData* mimeData = enterEvent->mimeData();

        m_allowDrop = AcceptsMimeData(mimeData);
        enterEvent->setAccepted(m_allowDrop);
    }

    void GraphCanvasEditorCentralWidget::dragMoveEvent(QDragMoveEvent* moveEvent)
    {
        moveEvent->setAccepted(m_allowDrop);
    }

    void GraphCanvasEditorCentralWidget::dropEvent(QDropEvent* dropEvent)
    {
        const QMimeData* mimeData = dropEvent->mimeData();

        if (m_allowDrop)
        {
            m_initialDropMimeData.clear();

            for (const QString& mimeType : mimeData->formats())
            {
                m_initialDropMimeData.setData(mimeType, mimeData->data(mimeType));
            }

            QPoint dropPosition = dropEvent->pos();
            QPoint globalPosition = mapToGlobal(dropEvent->pos());

            AZ::EntityId graphId;
            AssetEditorRequestBus::EventResult(graphId, GetEditorId(), &AssetEditorRequests::CreateNewGraph);

            // Need to delay this by a frame to ensure that the graphics view is actually
            // resized correctly, otherwise the node will move away from it's initial position.
            QTimer::singleShot(0, [graphId, dropPosition, globalPosition, this]
            {
                AZ::EntityId viewId;
                SceneRequestBus::EventResult(viewId, graphId, &GraphCanvas::SceneRequests::GetViewId);

                QPointF nodePoint;

                QGraphicsView* graphicsView = nullptr;
                GraphCanvas::ViewRequestBus::EventResult(graphicsView, viewId, &GraphCanvas::ViewRequests::AsGraphicsView);

                if (graphicsView)
                {
                    // Then we want to remap that into the GraphicsView, so we can map that into the GraphicsScene.
                    QPoint viewPoint = graphicsView->mapFromGlobal(globalPosition);
                    nodePoint = graphicsView->mapToScene(viewPoint);
                }
                else
                {
                    // If the view doesn't exist, this is fairly malformed, so we can just use the drop position directly.
                    nodePoint = dropPosition;
                }

                AZ::Vector2 scenePos = AZ::Vector2(nodePoint.x(), nodePoint.y());
                GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::DispatchSceneDropEvent, scenePos, &this->m_initialDropMimeData);
            });
        }
    }
    
    EditorDockWidget* GraphCanvasEditorCentralWidget::CreateEditorDockWidget(const EditorId& editorId)
    {
        return aznew EditorDockWidget(editorId, this);
    }

    bool GraphCanvasEditorCentralWidget::AcceptsMimeData(const QMimeData* mimeData) const
    {
        bool retVal = false;

        for (const QString& acceptedType : m_mimeTypes)
        {
            if (mimeData->hasFormat(acceptedType))
            {
                retVal = true;
                break;
            }
        }

        return retVal;
    }
    
#include <StaticLib/GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorCentralWidget.moc>
}