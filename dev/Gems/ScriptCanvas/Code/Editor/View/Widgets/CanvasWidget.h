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
    class GraphCanvasGraphicsView;
    class MiniMapGraphicsView;
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
            CanvasWidget(QWidget* parent = nullptr);
            ~CanvasWidget() override;

            void ShowScene(const AZ::EntityId& sceneId);
            const GraphCanvas::ViewId& GetViewId() const;

        protected:

            void resizeEvent(QResizeEvent *ev);

            void OnClicked();

            bool m_attached;

            void SetupGraphicsView();

            AZStd::unique_ptr<Ui::CanvasWidget> ui;

            // ScriptCanvas::Debugger::NotificationBus::Handler
            void OnAttach(const AZ::EntityId& /*graphId*/) override;
            void OnDetach(const AZ::EntityId& /*graphId*/) override;
            //

            void showEvent(QShowEvent *event) override;

        private:

            enum MiniMapPosition
            {
                MM_Not_Visible,
                MM_Upper_Left,
                MM_Upper_Right,
                MM_Lower_Right,
                MM_Lower_Left,

                MM_Position_Count
            };

            void PositionMiniMap();

            GraphCanvas::GraphCanvasGraphicsView* m_graphicsView;
            GraphCanvas::MiniMapGraphicsView* m_miniMapView;
            MiniMapPosition m_miniMapPosition = MM_Upper_Left;
        };
    }
}