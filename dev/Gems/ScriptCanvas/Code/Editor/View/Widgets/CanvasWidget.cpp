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

#include "precompiled.h"

#include "CanvasWidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QGraphicsView>
#include <QPushButton>

#include "Editor/View/Widgets/ui_CanvasWidget.h"

#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/MiniMapGraphicsView/MiniMapGraphicsView.h>
#include <GraphCanvas/GraphCanvasBus.h>

#include <Debugger/Bus.h>
#include <Core/Graph.h>
#include <Editor/View/Dialogs/Settings.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        CanvasWidget::CanvasWidget(QWidget* parent)
            : QWidget(parent)
            , ui(new Ui::CanvasWidget())
            , m_attached(false)
            , m_graphicsView(nullptr)
            , m_miniMapView(nullptr)
        {
            ui->setupUi(this);
            ui->m_debuggingControls->hide();

            SetupGraphicsView();

            connect(ui->m_debugAttach, &QPushButton::clicked, this, &CanvasWidget::OnClicked);
        }

        CanvasWidget::~CanvasWidget()
        {
            hide();
        }

        void CanvasWidget::ShowScene(const AZ::EntityId& scriptCanvasGraphId)
        {
            EditorGraphRequestBus::Event(scriptCanvasGraphId, &EditorGraphRequests::CreateGraphCanvasScene);

            AZ::EntityId graphCanvasSceneId;
            EditorGraphRequestBus::EventResult(graphCanvasSceneId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

            m_graphicsView->SetScene(graphCanvasSceneId);
        }

        const GraphCanvas::ViewId& CanvasWidget::GetViewId() const
        {
            return m_graphicsView->GetViewId();
        }

        void CanvasWidget::SetupGraphicsView()
        {
            m_graphicsView = aznew GraphCanvas::GraphCanvasGraphicsView();

            AZ_Assert(m_graphicsView, "Could Canvas Widget unable to create CanvasGraphicsView object.");
            if (m_graphicsView)
            {
                ui->verticalLayout->addWidget(m_graphicsView);
                m_graphicsView->show();                
                m_graphicsView->SetEditorId(ScriptCanvasEditor::AssetEditorId);

                // Temporary shortcut for docking the MiniMap. Removed until we fix up the MiniMap
                /*
                {
                    QAction* action = new QAction(m_graphicsView);
                    action->setShortcut(QKeySequence(Qt::Key_M));
                    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);

                    connect(action, &QAction::triggered,
                        [this]()
                        {
                            if (!m_graphicsView->rubberBandRect().isNull() || QApplication::mouseButtons() || m_graphicsView->GetIsEditing())
                            {
                                // Nothing to do.
                                return;
                            }

                            if (m_miniMapView)
                            {
                                // Cycle the position.
                                m_miniMapPosition = static_cast<MiniMapPosition>((m_miniMapPosition + 1) % MM_Position_Count);
                            }
                            else
                            {
                                m_miniMapView = aznew GraphCanvas::MiniMapGraphicsView(0 , false, m_graphicsView->GetScene(), m_graphicsView);
                            }

                            // Apply position.
                            PositionMiniMap();
                        });

                    m_graphicsView->addAction(action);
                }*/
            }
        }

        void CanvasWidget::showEvent(QShowEvent* /*event*/)
        {
            ui->m_debugAttach->setText(m_attached ? "Debugging: On" : "Debugging: Off");
        }

        void CanvasWidget::PositionMiniMap()
        {
            if (!(m_miniMapView && m_graphicsView))
            {
                // Nothing to do.
                return;
            }

            const QRect& parentRect = m_graphicsView->frameGeometry();

            if (m_miniMapPosition == MM_Upper_Left)
            {
                m_miniMapView->move(0, 0);
            }
            else if (m_miniMapPosition == MM_Upper_Right)
            {
                m_miniMapView->move(parentRect.width() - m_miniMapView->size().width(), 0);
            }
            else if (m_miniMapPosition == MM_Lower_Right)
            {
                m_miniMapView->move(parentRect.width() - m_miniMapView->size().width(), parentRect.height() - m_miniMapView->size().height());
            }
            else if (m_miniMapPosition == MM_Lower_Left)
            {
                m_miniMapView->move(0, parentRect.height() - m_miniMapView->size().height());
            }

            m_miniMapView->setVisible(m_miniMapPosition != MM_Not_Visible);
        }

        void CanvasWidget::resizeEvent(QResizeEvent *ev)
        {
            QWidget::resizeEvent(ev);

            PositionMiniMap();
        }

        void CanvasWidget::OnClicked()
        {
            ScriptCanvas::Graph* graph{};
            ScriptCanvas::GraphRequestBus::EventResult(graph, m_graphicsView->GetScene(), &ScriptCanvas::GraphRequests::GetGraph);
            if (!graph)
            {
                return;
            }

        }

#include <Editor/View/Widgets/CanvasWidget.moc>

    }
}
