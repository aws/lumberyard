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

#include <precompiled.h>
#include "Debugging.h"

#include <AzCore/Component/ComponentApplicationBus.h>

#include "Editor/View/Widgets/ui_Debugging.h"

#include <QToolBar>
#include <QVBoxLayout>

#include <Debugger/Bus.h>
#include <Core/GraphBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        Debugging::Debugging(QWidget* parent /*= nullptr*/)
            : AzQtComponents::StyledDockWidget(parent)
            , ui(new Ui::Debugging())
            , m_state(State::Detached)
        {
            ui->setupUi(this);
            setWindowTitle(tr("Debugging"));
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            setMinimumWidth(200);
            setMinimumHeight(40);

            ui->m_connect->setText(tr("Start Debugging"));
            ui->m_status->setText(tr("Not Debugging"));

            connect(ui->m_connect, &QPushButton::released, this, &Debugging::onConnectReleased);
        }

        void Debugging::onConnectReleased()
        {
            // TODO: Send the ID of the graph to attach
            AZ::EntityId scriptCanvasSceneId;
            ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(scriptCanvasSceneId, &ScriptCanvasEditor::GeneralRequests::GetActiveScriptCanvasGraphId);

            if (!scriptCanvasSceneId.IsValid())
            {
                // Do nothing
                return;
            }

            if (m_state == State::Detached)
            {
                AzFramework::TargetContainer targets;
                AzFramework::TargetManager::Bus::Broadcast(&AzFramework::TargetManager::EnumTargetInfos, targets);

                // Currently nullNetworkId is used to identify the "self" connection
                const AZ::u32 nullNetworkId = static_cast<AZ::u32>(-1);
                for (auto& target : targets)
                {
                    // Connecting to self
                    if (target.first == nullNetworkId)
                    {
                        const AzFramework::TargetInfo& info = target.second;
                        AZ_TracePrintf("Debug", "Script Canvas Debugger: connecting to target: %s", info.GetDisplayName());
                        AzFramework::TargetManager::Bus::Broadcast(&AzFramework::TargetManager::SetDesiredTarget, nullNetworkId);
                    }
                }
            
                ScriptCanvas::Debugger::NotificationBus::Handler::BusConnect();

                ScriptCanvas::Debugger::ConnectionRequestBus::Broadcast(&ScriptCanvas::Debugger::ConnectionRequests::Attach, scriptCanvasSceneId);
            }
            else
            {
                // Request the debugger to detach
                ScriptCanvas::Debugger::ConnectionRequestBus::Broadcast(&ScriptCanvas::Debugger::ConnectionRequests::Detach, scriptCanvasSceneId);
            }


        }

        void Debugging::OnAttach(const AZ::EntityId& graphId)
        {
            m_state = State::Attached;

            AZ::Entity* entity = nullptr;
            ScriptCanvas::GraphRequestBus::EventResult(entity, graphId, &ScriptCanvas::GraphRequests::GetGraphEntity);
            if (entity)
            {
                AZStd::string name = entity->GetName();
                auto* scriptCanvasComponent = entity->FindComponent<EditorScriptCanvasComponent>();
                if (scriptCanvasComponent)
                {
                    name = scriptCanvasComponent->GetName();
                }
                ui->m_status->setText(QString("Debugging: %1").arg(name.c_str()));
                ui->m_connect->setText(tr("Stop Debugging"));
            }
        }

        void Debugging::OnDetach(const AZ::EntityId& /*graphId*/)
        {
            m_state = State::Detached;


            ScriptCanvas::Debugger::NotificationBus::Handler::BusDisconnect();

            ui->m_status->setText(tr("Not Debugging"));
            ui->m_connect->setText(tr("Start Debugging"));

        }

        #include <Editor/View/Widgets/Debugging.moc>

    }
}

