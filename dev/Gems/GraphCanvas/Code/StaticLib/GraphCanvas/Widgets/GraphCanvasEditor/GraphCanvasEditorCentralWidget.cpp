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
    /////////////////////////////////////
    // GraphCanvasEditorEmptyDockWidget
    /////////////////////////////////////
    GraphCanvasEditorEmptyDockWidget::GraphCanvasEditorEmptyDockWidget(QWidget* parent)
        : QDockWidget(parent)
        , m_ui(new Ui::GraphCanvasEditorCentralWidget())
    {
        m_ui->setupUi(this);
        setAcceptDrops(true);

        // Because this is the empty visualization. We don't want a title bar.
        setTitleBarWidget(new QWidget(this));
    }
    
    GraphCanvasEditorEmptyDockWidget::~GraphCanvasEditorEmptyDockWidget()
    {        
    }
        
    void GraphCanvasEditorEmptyDockWidget::SetDragTargetText(const AZStd::string& dragTargetString)
    {
        m_ui->dropTarget->setText(dragTargetString.c_str());
    }

    void GraphCanvasEditorEmptyDockWidget::RegisterAcceptedMimeType(const QString& mimeType)
    {
        m_mimeTypes.emplace_back(mimeType);
    }
    
    void GraphCanvasEditorEmptyDockWidget::SetEditorId(const EditorId& editorId)
    {
        AZ_Warning("GraphCanvas", m_editorId == EditorId() || m_editorId == editorId, "Trying to re-use the same Central widget in two different editors.");
        
        m_editorId = editorId;
    }
    
    const EditorId& GraphCanvasEditorEmptyDockWidget::GetEditorId() const
    {
        return m_editorId;
    }

    void GraphCanvasEditorEmptyDockWidget::dragEnterEvent(QDragEnterEvent* enterEvent)
    {
        const QMimeData* mimeData = enterEvent->mimeData();

        m_allowDrop = AcceptsMimeData(mimeData);
        enterEvent->setAccepted(m_allowDrop);
    }

    void GraphCanvasEditorEmptyDockWidget::dragMoveEvent(QDragMoveEvent* moveEvent)
    {
        moveEvent->setAccepted(m_allowDrop);
    }

    void GraphCanvasEditorEmptyDockWidget::dropEvent(QDropEvent* dropEvent)
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

    bool GraphCanvasEditorEmptyDockWidget::AcceptsMimeData(const QMimeData* mimeData) const
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

    /////////////////////////////////
    // AssetEditorCentralDockWindow
    /////////////////////////////////

    AssetEditorCentralDockWindow::AssetEditorCentralDockWindow(const EditorId& editorId)
        : QMainWindow()
        , m_editorId(editorId)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setAutoFillBackground(true);

        setDockNestingEnabled(false);
        setTabPosition(Qt::DockWidgetArea::AllDockWidgetAreas, QTabWidget::TabPosition::North);

        m_emptyDockWidget = aznew GraphCanvasEditorEmptyDockWidget(this);
        addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, m_emptyDockWidget);

        QObject::connect(qApp, &QApplication::focusChanged, this, &AssetEditorCentralDockWindow::OnFocusChanged);
    }

    AssetEditorCentralDockWindow::~AssetEditorCentralDockWindow()
    {
    }

    GraphCanvasEditorEmptyDockWidget* AssetEditorCentralDockWindow::GetEmptyDockWidget() const
    {
        return m_emptyDockWidget;
    }

    void AssetEditorCentralDockWindow::OnEditorOpened(EditorDockWidget* dockWidget)
    {
        QObject::connect(dockWidget, &EditorDockWidget::OnEditorClosed, this, &AssetEditorCentralDockWindow::OnEditorClosed);
        QObject::connect(dockWidget, &QDockWidget::topLevelChanged, this, &AssetEditorCentralDockWindow::OnEditorDockChanged);

        DockWidgetId activeDockWidgetId;
        ActiveEditorDockWidgetRequestBus::EventResult(activeDockWidgetId, m_editorId, &ActiveEditorDockWidgetRequests::GetDockWidgetId);

        EditorDockWidget* activeDockWidget = nullptr;

        if (activeDockWidgetId.IsValid())
        {
            EditorDockWidgetRequestBus::EventResult(activeDockWidget, activeDockWidgetId, &EditorDockWidgetRequests::AsEditorDockWidget);
        }

        if (activeDockWidget && !activeDockWidget->isFloating())
        {
            tabifyDockWidget(activeDockWidget, dockWidget);
        }
        else
        {

            if (m_emptyDockWidget->isVisible())
            {
                addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, dockWidget);
            }
            else
            {
                QDockWidget* leftMostDock = nullptr;
                int minimumPoint = 0;

                for (QDockWidget* testWidget : m_editorDockWidgets)
                {
                    if (!testWidget->isFloating())
                    {
                        QPoint pos = testWidget->pos();

                        if (leftMostDock == nullptr || pos.x() < minimumPoint)
                        {
                            if (pos.x() >= 0)
                            {
                                leftMostDock = testWidget;
                                minimumPoint = pos.x();
                            }
                        }
                    }
                }

                if (leftMostDock)
                {
                    tabifyDockWidget(leftMostDock, dockWidget);
                }
                else
                {
                    addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, dockWidget);
                }
            }
        }

        m_editorDockWidgets.insert(dockWidget);

        dockWidget->show();
        dockWidget->setFocus();
        dockWidget->raise();

        UpdateCentralWidget();
    }

    void AssetEditorCentralDockWindow::OnEditorClosed(EditorDockWidget* dockWidget)
    {
        m_editorDockWidgets.erase(dockWidget);

        UpdateCentralWidget();
    }

    void AssetEditorCentralDockWindow::OnEditorDockChanged(bool isDocked)
    {
        AZ_UNUSED(isDocked);
        UpdateCentralWidget();
    }

    void AssetEditorCentralDockWindow::OnFocusChanged(QWidget* oldFocus, QWidget* newFocus)
    {
        if (newFocus != nullptr)
        {
            QObject* parent = newFocus;
            EditorDockWidget* dockWidget = qobject_cast<EditorDockWidget*>(newFocus);

            while (parent != nullptr && dockWidget == nullptr)
            {
                parent = parent->parent();
                dockWidget = qobject_cast<EditorDockWidget*>(parent);
            }

            if (dockWidget)
            {
                dockWidget->SignalActiveEditor();
            }
        }
    }

    void AssetEditorCentralDockWindow::UpdateCentralWidget()
    {
        bool isMainWindowEmpty = true;

        for (QDockWidget* dockWidget : m_editorDockWidgets)
        {
            if (!dockWidget->isFloating())
            {
                isMainWindowEmpty = false;
                break;
            }
        }

        if (isMainWindowEmpty && !m_emptyDockWidget->isVisible())
        {
            m_emptyDockWidget->show();
            setDockOptions((dockOptions() | ForceTabbedDocks));            
        }
        else if (!isMainWindowEmpty && m_emptyDockWidget->isVisible())
        {
            m_emptyDockWidget->hide();
            setDockOptions((dockOptions() & ~ForceTabbedDocks));
        }
    }
    
#include <StaticLib/GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorCentralWidget.moc>
}