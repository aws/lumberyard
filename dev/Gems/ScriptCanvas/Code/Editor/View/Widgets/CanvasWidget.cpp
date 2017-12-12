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

#include <GraphCanvas/Widgets/CanvasGraphicsView/CanvasGraphicsView.h>
#include <GraphCanvas/GraphCanvasBus.h>

#include <Debugger/Bus.h>
#include <Editor/View/Dialogs/Settings.h>

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        CanvasWidget::CanvasWidget(const AZ::Data::AssetId& assetId, const AZ::EntityId& graphId, QWidget* parent)
            : QWidget(parent)
            , ui(new Ui::CanvasWidget())
            , m_assetId(assetId)
            , m_graphId(graphId)
            , m_attached(false)
            , m_graphicsView(nullptr)
        {
            ui->setupUi(this);

            SetupGraphicsView();

            connect(ui->m_debugAttach, &QPushButton::clicked, this, &CanvasWidget::OnClicked);
            ui->m_debuggingControls->hide();
        }

        CanvasWidget::~CanvasWidget()
        {
            hide();
        }

        void CanvasWidget::ShowScene(const AZ::EntityId& sceneId)
        {
            m_graphicsView->SetScene(sceneId);
        }

        const GraphCanvas::ViewId& CanvasWidget::GetViewId() const
        {
            return m_graphicsView->GetId();
        }

        void CanvasWidget::SetupGraphicsView()
        {
            m_graphicsView = aznew GraphCanvas::CanvasGraphicsView();

            AZ_Assert(m_graphicsView, "Could Canvas Widget unable to create CanvasGraphicsView object.");
            if (m_graphicsView)
            {
                ui->verticalLayout->addWidget(m_graphicsView);
                m_graphicsView->show();
            }
        }

        void CanvasWidget::showEvent(QShowEvent* /*event*/)
        {
            ui->m_debugAttach->setText(m_attached ? "Debugging: On" : "Debugging: Off");
        }

        void CanvasWidget::OnClicked()
        {
            if (!m_attached)
            {
                ScriptCanvas::Debugger::NotificationBus::Handler::BusConnect();

                ScriptCanvas::Debugger::ConnectionRequestBus::Broadcast(&ScriptCanvas::Debugger::ConnectionRequests::Attach, m_graphId);
            }
            else
            {
                ScriptCanvas::Debugger::ConnectionRequestBus::Broadcast(&ScriptCanvas::Debugger::ConnectionRequests::Detach, m_graphId);
            }
        }

        void CanvasWidget::OnAttach(const AZ::EntityId& graphId)
        {
            if (m_graphId != graphId)
            {
                return;
            }

            m_attached = true;
            ui->m_debugAttach->setText("Debugging: On");
        }

        void CanvasWidget::OnDetach(const AZ::EntityId& graphId)
        {
            if (m_graphId != graphId)
            {
                return;
            }

            m_attached = false;
            ui->m_debugAttach->setText("Debugging: Off");

            ScriptCanvas::Debugger::NotificationBus::Handler::BusDisconnect();
        }

        #include <Editor/View/Widgets/CanvasWidget.moc>

    }
}
