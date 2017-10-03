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

#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Core/Node.h>

#include <ScriptCanvas/Core/GraphBus.h>

#include "PropertyGridBus.h"

class QSpacerItem;
class QScrollArea;

namespace AzToolsFramework
{
    class PropertiesWidget;
    class EntityPropertyEditor;
    namespace UndoSystem
    {
        class URSequencePoint;
    }
    class ComponentEditor;
}

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        class PropertyGrid
            : public AzQtComponents::StyledDockWidget
            , public AzToolsFramework::IPropertyEditorNotify
            , public PropertyGridRequestBus::Handler
            , public ScriptCanvas::EndpointNotificationBus::MultiHandler
        {
            Q_OBJECT

        public:

            PropertyGrid(QWidget* parent = nullptr, const char* name = "Properties");
            ~PropertyGrid() override;

            // AzToolsFramework::IPropertyEditorNotify
            void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
            void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
            void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* pNode) override;
            void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
            void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* node, const QPoint& point) override;
            /////

            // PropertyGridRequestHandler
            void RefreshPropertyGrid() override;
            ////

            void SealUndoStack() override {};

            void ClearSelection();
            void SetSelection(const AZStd::vector<AZ::EntityId>& selectedEntityIds);

            void OnNodeUpdate(const AZ::EntityId&);

            using InstancesToAggregate = AZStd::unordered_map<const AZ::SerializeContext::ClassData*, AZStd::unordered_set<AZ::Component*>>;

        private slots:
            void UpdateContents(const AZStd::vector<AZ::EntityId>& selectedEntityIds);

            // ScriptCanvas::EndpointNotificationBus::MultiHandler
            void OnEndpointConnected(const ScriptCanvas::Endpoint& targetEndpoint) override;
            void OnEndpointDisconnected(const ScriptCanvas::Endpoint& targetEndpoint) override;
            ////////////////////////////

        private:

            void SetVisibility(const AZStd::vector<AZ::EntityId>& selectedEntityIds);
            void DisplayInstances(const InstancesToAggregate& instancesToDisplay);

            AzToolsFramework::ComponentEditor* CreateComponentEditor();

            AZStd::vector<AZStd::unique_ptr<AzToolsFramework::ComponentEditor> > m_componentEditors;

            // the spacer's job is to make sure that its always at the end of the list of components.
            QSpacerItem* m_spacer = nullptr;

            QScrollArea* m_scrollArea = nullptr;
            QWidget* m_host = nullptr;
        };
    }
}
