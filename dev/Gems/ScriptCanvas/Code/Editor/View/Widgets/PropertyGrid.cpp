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
#include "PropertyGrid.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>

#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>

#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditorHeader.hxx>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponentBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <Editor/View/Widgets/PropertyGridContextMenu.h>
#include <ScriptCanvas/Bus/RequestBus.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/SceneBus.h>

#include <ScriptCanvas/Core/Endpoint.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

namespace
{
    bool ShouldDisplayComponent(AZ::Component* component)
    {
        auto classData = AzToolsFramework::GetComponentClassData(component);

        // Skip components without edit data
        if (!classData || !classData->m_editData)
        {
            return false;
        }

        // Skip components that are set to invisible.
        if (auto editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
        {
            if (auto visibilityAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::Visibility))
            {
                AzToolsFramework::PropertyAttributeReader reader(component, visibilityAttribute);
                AZ::u32 visibilityValue;
                if (reader.Read<AZ::u32>(visibilityValue))
                {
                    if (visibilityValue == AZ::Edit::PropertyVisibility::Hide)
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    // Returns a set of unique display component instances
    AZStd::unordered_set<AZ::Component*> GetVisibleGcInstances(const AZ::EntityId& entityId)
    {
        AZStd::unordered_set<AZ::Component*> result;
        GraphCanvas::GraphCanvasPropertyBus::EnumerateHandlersId(entityId,
            [&result](GraphCanvas::GraphCanvasPropertyInterface* propertyInterface) -> bool
            {
                AZ::Component* component = propertyInterface->GetPropertyComponent();
                if (ShouldDisplayComponent(component))
                {
                    result.insert(component);
                }

                // Continue enumeration.
                return true;
            });
        return result;
    }

    // Returns a set of unique display component instances
    AZStd::unordered_set<AZ::Component*> GetVisibleScInstances(const AZ::EntityId& entityId)
    {
        // GraphCanvas entityId -> scriptCanvasEntity
        AZStd::any* userData {};
        GraphCanvas::NodeRequestBus::EventResult(userData, entityId, &GraphCanvas::NodeRequests::GetUserData);
        AZ::EntityId scriptCanvasId = userData && userData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
        if (!scriptCanvasId.IsValid())
        {
            return {};
        }

        AZ::Entity* scriptCanvasEntity = AzToolsFramework::GetEntityById(scriptCanvasId);
        if (!scriptCanvasEntity)
        {
            return {};
        }

        // scriptCanvasEntity -> ScriptCanvas::Node
        AZStd::unordered_set<AZ::Component*> result;
        auto components = AZ::EntityUtils::FindDerivedComponents<ScriptCanvas::Node>(scriptCanvasEntity);
        for (auto component : components)
        {
            if (ShouldDisplayComponent(component))
            {
                result.insert(component);
            }
        }

        return result;
    }

    void AppendInstances(ScriptCanvasEditor::Widget::PropertyGrid::InstancesToAggregate& instancesToDisplay,
        const AZStd::unordered_set<AZ::Component*>& components)
    {
        if (components.empty())
        {
            return;
        }

        for (auto component : components)
        {
            const AZ::SerializeContext::ClassData* positionInMap = AzToolsFramework::GetComponentClassData(component);
            instancesToDisplay[positionInMap].insert(component);
        }
    }

    void CollectInstancesToDisplay(ScriptCanvasEditor::Widget::PropertyGrid::InstancesToAggregate& instancesToDisplay,
        const AZStd::vector<AZ::EntityId>& selectedEntityIds)
    {
        for (auto& entityId : selectedEntityIds)
        {
            AppendInstances(instancesToDisplay, GetVisibleScInstances(entityId));
            AppendInstances(instancesToDisplay, GetVisibleGcInstances(entityId));
        }
    }
}

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        PropertyGrid::PropertyGrid(QWidget* parent /*= nullptr*/, const char* name /*= "Properties"*/)
            : AzQtComponents::StyledDockWidget(parent)
        {
            // This is used for styling.
            setObjectName("PropertyGrid");

            m_componentEditors.clear();

            m_spacer = new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding);

            setWindowTitle(name);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            m_scrollArea = new QScrollArea(this);
            m_scrollArea->setWidgetResizable(true);
            m_scrollArea->setSizePolicy(QSizePolicy::Policy::Ignored, QSizePolicy::Policy::Ignored);

            m_host = new QWidget;
            m_host->setLayout(new QVBoxLayout());

            m_scrollArea->setWidget(m_host);
            setWidget(m_scrollArea);

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to acquire application serialize context.");

            UpdateContents(AZStd::vector<AZ::EntityId>());

            PropertyGridRequestBus::Handler::BusConnect();
        }

        PropertyGrid::~PropertyGrid()
        {
        }

        void PropertyGrid::ClearSelection()
        {
            m_componentEditors.clear();
            ScriptCanvas::EndpointNotificationBus::MultiHandler::BusDisconnect();
        }

        void PropertyGrid::DisplayInstances(const InstancesToAggregate& instancesToDisplay)
        {
            for (auto& pair : instancesToDisplay)
            {
                AzToolsFramework::ComponentEditor* componentEditor = CreateComponentEditor();

                // Display all instances.
                // Add instances to componentEditor
                AZ::Component* firstInstance = !pair.second.empty() ? *pair.second.begin() : nullptr;
                for (AZ::Component* componentInstance : pair.second)
                {
                    // non-first instances are aggregated under the first instance
                    AZ::Component* aggregateInstance = componentInstance != firstInstance ? firstInstance : nullptr;
                    componentEditor->AddInstance(componentInstance, aggregateInstance, nullptr);
                }

                // Refresh editor
                componentEditor->AddNotifications();
                componentEditor->SetExpanded(true);
                componentEditor->InvalidateAll();

                // hiding the icon on the header for Preview
                componentEditor->GetHeader()->SetIcon(QIcon());

                componentEditor->show();
            }
        }

        AzToolsFramework::ComponentEditor* PropertyGrid::CreateComponentEditor()
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to acquire application serialize context.");

            m_componentEditors.push_back(AZStd::make_unique<AzToolsFramework::ComponentEditor>(serializeContext, this, this));
            AzToolsFramework::ComponentEditor* componentEditor = m_componentEditors.back().get();

            componentEditor->GetHeader()->SetHasContextMenu(false);
            componentEditor->GetPropertyEditor()->SetHideRootProperties(false);
            componentEditor->GetPropertyEditor()->SetAutoResizeLabels(true);

            connect(componentEditor, &AzToolsFramework::ComponentEditor::OnExpansionContractionDone, this, [this]()
                {
                    m_host->layout()->update();
                    m_host->layout()->activate();
                });

            //move spacer to bottom of editors
            m_host->layout()->removeItem(m_spacer);
            m_host->layout()->addWidget(componentEditor);
            m_host->layout()->addItem(m_spacer);
            m_host->layout()->update();

            componentEditor->GetPropertyEditor()->SetDynamicEditDataProvider(
                [](const void* objectPtr, const AZ::SerializeContext::ClassData* classData) -> const AZ::Edit::ElementData*
            {
                if (classData->m_typeId == azrtti_typeid<ScriptCanvas::Datum>())
                {
                    auto datum = reinterpret_cast<const ScriptCanvas::Datum*>(objectPtr);
                    return datum->GetEditElementData();
                }
                return nullptr;

                });

            return componentEditor;
        }

        void PropertyGrid::SetSelection(const AZStd::vector<AZ::EntityId>& selectedEntityIds)
        {
            ClearSelection();
            UpdateContents(selectedEntityIds);
            RefreshPropertyGrid();
        }

        void PropertyGrid::OnNodeUpdate(const AZ::EntityId&)
        {
            RefreshPropertyGrid();
        }

        void PropertyGrid::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
        {
        }

        void PropertyGrid::AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode)
        {
            AzToolsFramework::InstanceDataNode* componentNode = pNode;
            do
            {
                auto* componentClassData = componentNode->GetClassMetadata();
                if (componentClassData && componentClassData->m_azRtti && componentClassData->m_azRtti->IsTypeOf(azrtti_typeid<AZ::Component>()))
                {
                    break;
                }
            } while (componentNode = componentNode->GetParent());

            if (!componentNode)
            {
                AZ_Warning("Script Canvas", false, "Failed to locate component data associated with the script canvas property. Unable to mark parent Entity as dirty.");
                return;
            }

            // Only need one instance to lookup the SceneId in-order to record the undo state
            const size_t firstInstanceIdx = 0;
            if (componentNode->GetNumInstances())
            {
                AZ::SerializeContext* context = componentNode->GetSerializeContext();
                AZ::Component* componentInstance = context->Cast<AZ::Component*>(componentNode->GetInstance(firstInstanceIdx), componentNode->GetClassMetadata()->m_typeId);
                if (componentInstance && componentInstance->GetEntity())
                {
                    AZ::EntityId sceneId;
                    if (AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(componentInstance->GetEntity()))
                    {
                        // ScriptCanvas Node
                        ScriptCanvas::EditorNodeRequestBus::EventResult(sceneId, componentInstance->GetEntityId(), &ScriptCanvas::EditorNodeRequests::GetGraphEntityId);
                    }
                    else
                    {
                        // GraphCanvas Node
                        GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, componentInstance->GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);
                    }
                    GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, sceneId);
                }
            }
        }

        void PropertyGrid::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* pNode)
        {
        }

        void PropertyGrid::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
        {
        }

        void PropertyGrid::RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* node, const QPoint& point)
        {
            PropertyGridContextMenu contextMenu(node);
            if (!contextMenu.actions().empty())
            {
                contextMenu.exec(point);
            }
        }

        void PropertyGrid::RefreshPropertyGrid()
        {
            for (auto& componentEditor : m_componentEditors)
            {
                if (componentEditor->isVisible())
                {
                    componentEditor->QueuePropertyEditorInvalidation(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values);
                }
                else
                {
                    break;
                }
            }
        }

        void PropertyGrid::SetVisibility(const AZStd::vector<AZ::EntityId>& selectedEntityIds)
        {
            // Set the visibility and connect for changes.
            for (auto& gcNodeEntityId : selectedEntityIds)
            {
                // GC node -> SC node.
                AZStd::any* nodeUserData = nullptr;
                GraphCanvas::NodeRequestBus::EventResult(nodeUserData, gcNodeEntityId, &GraphCanvas::NodeRequests::GetUserData);
                AZ::EntityId scNodeEntityId = nodeUserData && nodeUserData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(nodeUserData) : AZ::EntityId();

                ScriptCanvas::Node* node = ScriptCanvas::Node::FindNode(scNodeEntityId);
                if (!node)
                {
                    continue;
                }

                AZStd::vector<AZ::EntityId> gcSlotEntityIds;
                GraphCanvas::NodeRequestBus::EventResult(gcSlotEntityIds, gcNodeEntityId, &GraphCanvas::NodeRequests::GetSlotIds);

                for (auto& gcSlotEntityId : gcSlotEntityIds)
                {
                    // GC slot -> SC slot.
                    AZStd::any* slotUserData = nullptr;
                    GraphCanvas::SlotRequestBus::EventResult(slotUserData, gcSlotEntityId, &GraphCanvas::SlotRequests::GetUserData);
                    ScriptCanvas::SlotId scSlotId = slotUserData && slotUserData->is<ScriptCanvas::SlotId>() ? *AZStd::any_cast<ScriptCanvas::SlotId>(slotUserData) : ScriptCanvas::SlotId();

                    const ScriptCanvas::Slot* slot = node->GetSlot(scSlotId);

                    if (!slot || slot->GetType() != ScriptCanvas::SlotType::DataIn)
                    {
                        continue;
                    }

                    // Set the visibility based on the connection status.
                    {
                        bool isConnected = false;
                        GraphCanvas::SlotRequestBus::EventResult(isConnected, gcSlotEntityId, &GraphCanvas::SlotRequests::HasConnections);

                        ScriptCanvas::Datum* datum = nullptr;
                        ScriptCanvas::EditorNodeRequestBus::EventResult(datum, scNodeEntityId, &ScriptCanvas::EditorNodeRequests::ModInput, scSlotId);
                        if(datum)
                        {
                            datum->SetVisibility(isConnected ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show);
                        }
                    }

                    // Connect to get notified of changes.
                    ScriptCanvas::EndpointNotificationBus::MultiHandler::BusConnect(ScriptCanvas::Endpoint(scNodeEntityId, scSlotId));
                }
            }
        }

        void PropertyGrid::UpdateContents(const AZStd::vector<AZ::EntityId>& selectedEntityIds)
        {
            if (!selectedEntityIds.empty())
            {
                // Build up components to display
                InstancesToAggregate instancesToDisplay;
                CollectInstancesToDisplay(instancesToDisplay, selectedEntityIds);

                SetVisibility(selectedEntityIds);
                DisplayInstances(instancesToDisplay);
            }
        }

        void PropertyGrid::OnEndpointConnected(const ScriptCanvas::Endpoint& targetEndpoint)
        {
            const ScriptCanvas::Endpoint* sourceEndpoint = ScriptCanvas::EndpointNotificationBus::GetCurrentBusId();

            ScriptCanvas::Datum* datum = nullptr;
            ScriptCanvas::EditorNodeRequestBus::EventResult(datum, sourceEndpoint->GetNodeId(), &ScriptCanvas::EditorNodeRequests::ModInput, sourceEndpoint->GetSlotId());

            if (datum)
            {
                datum->SetVisibility(AZ::Edit::PropertyVisibility::Hide);
            }

            RefreshPropertyGrid();
        }

        void PropertyGrid::OnEndpointDisconnected(const ScriptCanvas::Endpoint& targetEndpoint)
        {
            const ScriptCanvas::Endpoint* sourceEndpoint = ScriptCanvas::EndpointNotificationBus::GetCurrentBusId();

            ScriptCanvas::Datum* datum = nullptr;
            ScriptCanvas::EditorNodeRequestBus::EventResult(datum, sourceEndpoint->GetNodeId(), &ScriptCanvas::EditorNodeRequests::ModInput, sourceEndpoint->GetSlotId());

            if (datum)
            {
                datum->SetVisibility(AZ::Edit::PropertyVisibility::Show);
            }

            RefreshPropertyGrid();
        }

#include <Editor/View/Widgets/PropertyGrid.moc>
    }
}
