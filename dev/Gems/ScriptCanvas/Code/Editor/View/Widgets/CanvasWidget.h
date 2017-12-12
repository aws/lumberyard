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
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Component/EntityId.h>
#include <Debugger/Bus.h>
#include <AzCore/Asset/AssetCommon.h>

#include <GraphCanvas/Components/ViewBus.h>

class QVBoxLayout;

namespace Ui
{
    class CanvasWidget;
}

namespace GraphCanvas
{
    class CanvasGraphicsView;
}

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        class CanvasWidget 
            : public QWidget
            , ScriptCanvas::Debugger::NotificationBus::Handler
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(CanvasWidget, AZ::SystemAllocator, 0);
            CanvasWidget(const AZ::Data::AssetId& assetId, const AZ::EntityId& graphId, QWidget* parent = nullptr);
            ~CanvasWidget() override;

            void ShowScene(const AZ::EntityId& sceneId);
            const GraphCanvas::ViewId& GetViewId() const;
            void SetGraphId(const AZ::EntityId& graphId) { m_graphId = graphId; }

        protected:

            void OnClicked();

            bool m_attached;

            void SetupGraphicsView();
            
            AZ::Data::AssetId m_assetId;
            AZ::EntityId m_graphId;

            AZStd::unique_ptr<Ui::CanvasWidget> ui;

            // ScriptCanvas::Debugger::NotificationBus::Handler
            void OnAttach(const AZ::EntityId& /*graphId*/) override;
            void OnDetach(const AZ::EntityId& /*graphId*/) override;
            //

            void showEvent(QShowEvent *event) override;

        private:
            GraphCanvas::CanvasGraphicsView* m_graphicsView;
        };
    }
}