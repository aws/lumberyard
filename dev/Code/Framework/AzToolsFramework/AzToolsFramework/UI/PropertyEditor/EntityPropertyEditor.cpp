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
#include "stdafx.h"
#include "EntityPropertyEditor.hxx"

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/Slice/SliceDataFlagsCommand.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/UI/ComponentPalette/ComponentPaletteWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorApi.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/Undo/UndoSystem.h>

#include <QtCore/qtimer.h>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollBar>
#include <UI/PropertyEditor/ui_EntityPropertyEditor.h>

namespace AzToolsFramework
{
    EntityPropertyEditor::SharedComponentInfo::SharedComponentInfo(AZ::Component* component, AZ::Component* sliceReferenceComponent)
        : m_sliceReferenceComponent(sliceReferenceComponent)
    {
        m_instances.push_back(component);
    }

    EntityPropertyEditor::EntityPropertyEditor(QWidget* pParent, Qt::WindowFlags flags)
        : QWidget(pParent, flags)
        , m_propertyEditBusy(0)
        , m_componentFilter(AppearsInGameComponentMenu)
        , m_componentPalette(nullptr)
    {
        setObjectName("EntityPropertyEditor");
        m_isAlreadyQueuedRefresh = false;
        m_shouldScrollToNewComponents = false;
        m_shouldScrollToNewComponentsQueued = false;
        m_currentUndoNode = nullptr;
        m_currentUndoOperation = nullptr;
        m_selectionEventAccepted = false;

        m_componentEditorLastSelectedIndex = -1;
        m_componentEditorsUsed = 0;
        m_componentEditors.clear();
        m_componentToEditorMap.clear();

        m_gui = aznew Ui::EntityPropertyEditorUI();
        m_gui->setupUi(this);
        m_gui->m_entityIdEditor->setReadOnly(true);
        m_gui->m_entityNameEditor->setReadOnly(false);
        m_gui->m_entityDetailsLabel->setObjectName("LabelEntityDetails");
        EnableEditor(true);
        m_sceneIsNew = true;

        connect(m_gui->m_entityIcon, &QPushButton::clicked, this, &EntityPropertyEditor::BuildEntityIconMenu);
        connect(m_gui->m_addComponentButton, &QPushButton::clicked, this, &EntityPropertyEditor::OnAddComponent);

        m_componentPalette = aznew ComponentPaletteWidget(this, true);
        connect(m_componentPalette, &ComponentPaletteWidget::OnAddComponentEnd, this, [this]() {
            QueuePropertyRefresh();
            m_shouldScrollToNewComponents = true;
        });

        HideComponentPalette();

        ToolsApplicationEvents::Bus::Handler::BusConnect();
        AZ::EntitySystemBus::Handler::BusConnect();

        m_spacer = nullptr;

        m_serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(m_serializeContext, "Failed to acquire application serialize context.");

        m_isBuildingProperties = false;

        setTabOrder(m_gui->m_entityIdEditor, m_gui->m_entityNameEditor);
        setTabOrder(m_gui->m_entityNameEditor, m_gui->m_addComponentButton);

        // the world editor has a fixed id:

        connect(m_gui->m_entityNameEditor,
            SIGNAL(editingFinished()),
            this,
            SLOT(OnEntityNameChanged()));

        CreateActions();
        UpdateContents();

        EditorEntityContextNotificationBus::Handler::BusConnect();

        //forced to register global event filter with application for selection
        //mousePressEvent and other handlers won't trigger if event is accepted by a child widget like a QLineEdit
        //TODO explore other options to avoid referencing qApp and filtering all events even though research says
        //this is the way to do it without overriding or registering with all child widgets
        qApp->installEventFilter(this);
    }

    EntityPropertyEditor::~EntityPropertyEditor()
    {
        qApp->removeEventFilter(this);

        ToolsApplicationEvents::Bus::Handler::BusDisconnect();
        AZ::EntitySystemBus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();

        delete m_gui;
    }

    void EntityPropertyEditor::BeforeEntitySelectionChanged()
    {
        ClearComponentEditorSelection();
        ClearComponentEditorState();
    }

    void EntityPropertyEditor::AfterEntitySelectionChanged()
    {
        EntityIdList selectedEntityIds;
        ToolsApplicationRequests::Bus::BroadcastResult(selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        if (selectedEntityIds.empty())
        {
            // Ensure a prompt refresh when all entities have been removed/deselected.
            UpdateContents();
            return;
        }

        // when entity selection changed, we need to repopulate our GUI
        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::OnEntityActivated(const AZ::EntityId& entityId)
    {
        if (IsEntitySelected(entityId))
        {
            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::OnEntityDeactivated(const AZ::EntityId& entityId)
    {
        if (IsEntitySelected(entityId))
        {
            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::OnEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name)
    {
        if (IsSingleEntitySelected(entityId))
        {
            m_gui->m_entityNameEditor->setText(QString(name.c_str()));
        }
    }

    bool EntityPropertyEditor::IsEntitySelected(const AZ::EntityId& id) const
    {
        return AZStd::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(), id) != m_selectedEntityIds.end();
    }

    bool EntityPropertyEditor::IsSingleEntitySelected(const AZ::EntityId& id) const
    {
        return m_selectedEntityIds.size() == 1 && IsEntitySelected(id);
    }

    void EntityPropertyEditor::OnStartPlayInEditor()
    {
        setEnabled(false);
    }

    void EntityPropertyEditor::OnStopPlayInEditor()
    {
        setEnabled(true);
    }

    void EntityPropertyEditor::UpdateEntityIcon()
    {
        AZStd::string iconSourcePath;
        AzToolsFramework::EditorRequestBus::BroadcastResult(iconSourcePath, &AzToolsFramework::EditorRequestBus::Events::GetDefaultEntityIcon);

        if (!m_selectedEntityIds.empty())
        {
            //Test if every entity has the same icon, if so use that icon, otherwise use the default one.
            AZStd::string firstIconPath;
            EditorEntityIconComponentRequestBus::EventResult(firstIconPath, m_selectedEntityIds.front(), &EditorEntityIconComponentRequestBus::Events::GetEntityIconPath);

            bool haveSameIcon = true;
            for (const auto entityId : m_selectedEntityIds)
            {
                AZStd::string iconPath;
                EditorEntityIconComponentRequestBus::EventResult(iconPath, entityId, &EditorEntityIconComponentRequestBus::Events::GetEntityIconPath);
                if (iconPath != firstIconPath)
                {
                    haveSameIcon = false;
                    break;
                }
            }

            if (haveSameIcon)
            {
                iconSourcePath = AZStd::move(firstIconPath);
            }
        }

        m_gui->m_entityIcon->setIcon(QIcon(iconSourcePath.c_str()));
        m_gui->m_entityIcon->repaint();
    }

    void EntityPropertyEditor::UpdateEntityDisplay()
    {
        // Generic text for multiple entities selected
        if (m_selectedEntityIds.size() > 1)
        {
            m_gui->m_entityDetailsLabel->setVisible(true);
            m_gui->m_entityDetailsLabel->setText(tr("Only common components shown"));
            m_gui->m_entityNameEditor->setText(tr("%n entities selected", "", static_cast<int>(m_selectedEntityIds.size())));
            m_gui->m_entityIdEditor->setText(tr("%n entities selected", "", static_cast<int>(m_selectedEntityIds.size())));
            m_gui->m_entityNameEditor->setReadOnly(true);
        }
        else if (!m_selectedEntityIds.empty())
        {
            auto entityId = m_selectedEntityIds.front();
            m_gui->m_entityIdEditor->setText(QString("%1").arg(static_cast<AZ::u64>(entityId)));

            // No entity details for one entity
            m_gui->m_entityDetailsLabel->setVisible(false);

            // If we're in edit mode, make the name field editable.
            m_gui->m_entityNameEditor->setReadOnly(!m_gui->m_componentListContents->isEnabled());

            // get the name of the entity.
            auto entity = GetEntityById(entityId);
            m_gui->m_entityNameEditor->setText(entity ? entity->GetName().data() : "Entity Not Found");
        }
    }

    void EntityPropertyEditor::UpdateContents()
    {
        setUpdatesEnabled(false);

        m_isAlreadyQueuedRefresh = false;
        m_isBuildingProperties = true;
        m_sliceCompareToEntity.reset();

        HideComponentPalette();

        ClearInstances(false);

        m_selectedEntityIds.clear();
        ToolsApplicationRequests::Bus::BroadcastResult(m_selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        SourceControlFileInfo scFileInfo;
        ToolsApplicationRequests::Bus::BroadcastResult(scFileInfo, &ToolsApplicationRequests::GetSceneSourceControlInfo);

        if (!m_spacer)
        {
            m_spacer = aznew QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding);
        }

        // Hide the entity stuff and add component button if no entities are displayed
        bool hasEntitiesDisplayed = !m_selectedEntityIds.empty();
        m_gui->m_entityDetailsLabel->setText(tr(hasEntitiesDisplayed ? "" : "No Entity Selected"));
        m_gui->m_entityDetailsLabel->setVisible(!hasEntitiesDisplayed);
        m_gui->m_addComponentButton->setEnabled(hasEntitiesDisplayed);
        m_gui->m_addComponentButton->setVisible(hasEntitiesDisplayed);
        m_gui->m_entityNameEditor->setVisible(hasEntitiesDisplayed);
        m_gui->m_entityNameLabel->setVisible(hasEntitiesDisplayed);
        m_gui->m_entityIdEditor->setVisible(hasEntitiesDisplayed);
        m_gui->m_entityIdLabel->setVisible(hasEntitiesDisplayed);
        m_gui->m_entityIcon->setVisible(hasEntitiesDisplayed);

        if (hasEntitiesDisplayed)
        {
            // Build up components to display
            SharedComponentArray sharedComponentArray;
            BuildSharedComponentArray(sharedComponentArray);
            BuildSharedComponentUI(sharedComponentArray);

            UpdateEntityIcon();
            UpdateEntityDisplay();
        }

        UpdateActions();
        QueueScrollToNewComponent();
        LoadComponentEditorState();

        layout()->update();
        layout()->activate();

        m_isBuildingProperties = false;

        setUpdatesEnabled(true);
    }

    // This returns a sorted component list AND updates the component order list (removes outdated entries, adds new ones)
    void EntityPropertyEditor::SortComponentsByOrder(AZ::Entity::ComponentArrayType& componentsToSort, ComponentOrderArray& componentOrderList)
    {
        AZ::Entity::ComponentArrayType sortedComponentArray;

        using ComponentIdList = AZStd::list<AZ::Component*>;
        // Build up a map of componentId to iterators in the component list
        AZStd::unordered_map<AZ::ComponentId, ComponentIdList::iterator> componentIdToComponentIterator;
        // List to keep track of who we have not yet placed in the proper order
        ComponentIdList listOfComponents;
        for (auto component : componentsToSort)
        {
            // Insert each component into the list and store the resulting iterator in the map keyed by component id
            componentIdToComponentIterator[component->GetId()] = listOfComponents.insert(listOfComponents.end(), component);
        }

        for (auto componentOrderIterator = componentOrderList.begin(); componentOrderIterator != componentOrderList.end();)
        {
            auto componentId = *componentOrderIterator;

            // Look into the map to find the iterator
            auto mapIterator = componentIdToComponentIterator.find(componentId);
            if (mapIterator == componentIdToComponentIterator.end())
            {
                // Order list knows about a component id that we no longer have, remove it from the order list and continue
                componentOrderIterator = componentOrderList.erase(componentOrderIterator);
                continue;
            }

            // This gives us the iterator into the list
            auto listIterator = mapIterator->second;

            // This gives us the component pointer
            auto component = *listIterator;

            // Push component onto final vector
            sortedComponentArray.push_back(component);

            // Remove the component from the list (will not invalidate any other iterators)
            listOfComponents.erase(listIterator);

            ++componentOrderIterator;
        }

        // Run through the list to add any component for which we had no order information
        for (auto component : listOfComponents)
        {
            sortedComponentArray.push_back(component);
            componentOrderList.push_back(component->GetId());
        }

        componentsToSort.swap(sortedComponentArray);
    }

    AZStd::vector<AZ::Component*> EntityPropertyEditor::GetAllComponentsForEntityInOrder(AZ::Entity* entity)
    {
        auto componentsToSort = AzToolsFramework::GetAllComponentsForEntity(entity);
        componentsToSort.erase(
            AZStd::remove_if(componentsToSort.begin(), componentsToSort.end(), [this](AZ::Component* component) {return !ShouldDisplayComponent(component) || !IsComponentRemovable(component);}),
            componentsToSort.end());

        auto componentsToKeep = AzToolsFramework::GetAllComponentsForEntity(entity);
        componentsToKeep.erase(
            AZStd::remove_if(componentsToKeep.begin(), componentsToKeep.end(), [this](AZ::Component* component) {return !ShouldDisplayComponent(component) || IsComponentRemovable(component);}),
            componentsToKeep.end());

        // Query the current known component order
        ComponentOrderArray componentOrderArray;
        EditorInspectorComponentRequestBus::EventResult(componentOrderArray, entity->GetId(), &EditorInspectorComponentRequests::GetComponentOrderArray);

        // Sort the components (this also gives us an updated order array
        SortComponentsByOrder(componentsToSort, componentOrderArray);

        // Set the updated order array (this is only going to be different for entities that don't currently have an order list or were otherwise updated outside of the editor, which should be rare)
        EditorInspectorComponentRequestBus::Event(entity->GetId(), &EditorInspectorComponentRequests::SetComponentOrderArray, componentOrderArray);

        componentsToKeep.insert(componentsToKeep.end(), componentsToSort.begin(), componentsToSort.end());

        return componentsToKeep;
    }

    bool EntityPropertyEditor::ShouldDisplayComponent(AZ::Component* component) const
    {
        auto classData = GetComponentClassData(component);

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
                PropertyAttributeReader reader(component, visibilityAttribute);
                AZ::u32 visibilityValue;
                if (reader.Read<AZ::u32>(visibilityValue))
                {
                    if (visibilityValue == AZ_CRC("PropertyVisibility_Hide", 0x32ab90f7))
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool EntityPropertyEditor::IsComponentRemovable(const AZ::Component* component) const
    {
        auto componentClassData = GetComponentClassData(component);

        // Determine if this component can be removed.
        return m_componentFilter(*componentClassData);
    }

    bool EntityPropertyEditor::AreComponentsRemovable(const AZ::Entity::ComponentArrayType& components) const
    {
        for (auto component : components)
        {
            if (!IsComponentRemovable(component))
            {
                return false;
            }
        }
        return true;
    }

    AZ::Component* EntityPropertyEditor::ExtractMatchingComponent(AZ::Component* referenceComponent, AZ::Entity::ComponentArrayType& availableComponents)
    {
        for (auto availableComponentIterator = availableComponents.begin(); availableComponentIterator != availableComponents.end(); ++availableComponentIterator)
        {
            auto component = *availableComponentIterator;
            auto editorComponent = GetEditorComponent(component);
            auto referenceEditorComponent = GetEditorComponent(referenceComponent);

            // Early out on class data not matching, we require components to be the same class
            if (GetComponentClassData(referenceComponent) != GetComponentClassData(component))
            {
                continue;
            }

            auto referenceEditorComponentDescriptor = GetEditorComponentDescriptor(referenceComponent);
            // If not AZ_EDITOR_COMPONENT, then as long as they match type (checked above) it is considered matching
            // Otherwise, use custom component matching available via AZ_EDITOR_COMPONENT
            if (!referenceEditorComponentDescriptor || referenceEditorComponentDescriptor->DoComponentsMatch(referenceEditorComponent, editorComponent))
            {
                // Remove the component we found from the available set
                availableComponents.erase(availableComponentIterator);
                return component;
            }
        }

        return nullptr;
    }

    void EntityPropertyEditor::BuildSharedComponentArray(SharedComponentArray& sharedComponentArray)
    {
        AZ_Assert(!m_selectedEntityIds.empty(), "BuildSharedComponentArray should only be called if there are entities being displayed");

        auto entityId = m_selectedEntityIds.front();

        // For single selection of a slice-instanced entity, gather the direct slice ancestor
        // so we can visualize per-component differences.
        m_sliceCompareToEntity.reset();
        if (1 == m_selectedEntityIds.size())
        {
            AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;

            AzFramework::EntityIdContextQueryBus::EventResult(sliceInstanceAddress, entityId, &AzFramework::EntityIdContextQueries::GetOwningSlice);
            auto sliceReference = sliceInstanceAddress.first;
            auto sliceInstance = sliceInstanceAddress.second;
            if (sliceReference)
            {
                AZ::SliceComponent::EntityAncestorList ancestors;
                sliceReference->GetInstanceEntityAncestry(entityId, ancestors, 1);

                if (!ancestors.empty())
                {
                    m_sliceCompareToEntity = SliceUtilities::CloneSliceEntityForComparison(*ancestors[0].m_entity, *sliceInstance, *m_serializeContext);
                }
            }
        }

        // Gather initial list of eligible display components from the first entity
        auto entity = GetEntityById(entityId);
        AZ::Entity::ComponentArrayType entityComponents = GetAllComponentsForEntityInOrder(entity);
        AZ::Entity::ComponentArrayType sliceEntityComponents;
        if (m_sliceCompareToEntity)
        {
            sliceEntityComponents = GetAllComponentsForEntityInOrder(m_sliceCompareToEntity.get());
        }
        for (auto component : entityComponents)
        {
            // Skip components that we should not display
            // Only need to do this for the initial set, components without edit data on other entities should not match these
            if (!ShouldDisplayComponent(component))
            {
                continue;
            }

            // Grab the slice reference component, if we have a slice compare entity
            auto sliceReferenceComponent = m_sliceCompareToEntity ? m_sliceCompareToEntity->FindComponent(component->GetId()) : nullptr;

            sharedComponentArray.push_back(SharedComponentInfo(component, sliceReferenceComponent));
        }

        // Now loop over the other entities
        for (size_t entityIndex = 1; entityIndex < m_selectedEntityIds.size(); ++entityIndex)
        {
            entity = GetEntityById(m_selectedEntityIds[entityIndex]);
            AZ_Assert(entity, "Entity id selected for display but no such entity exists");
            if (!entity)
            {
                continue;
            }

            entityComponents = GetAllComponentsForEntityInOrder(entity);

            // Loop over the known components on all entities (so far)
            // Check to see if they are also on this entity
            // If not, remove them from the list
            for (auto sharedComponentInfoIterator = sharedComponentArray.begin(); sharedComponentInfoIterator != sharedComponentArray.end();)
            {
                auto firstComponent = sharedComponentInfoIterator->m_instances[0];

                auto matchingComponent = ExtractMatchingComponent(firstComponent, entityComponents);
                if (!matchingComponent)
                {
                    // Remove this as it isn't on all entities so don't bother continuing with it
                    sharedComponentInfoIterator = sharedComponentArray.erase(sharedComponentInfoIterator);
                    continue;
                }

                // Add the matching component and continue
                sharedComponentInfoIterator->m_instances.push_back(matchingComponent);
                ++sharedComponentInfoIterator;
            }
        }
    }

    void EntityPropertyEditor::BuildSharedComponentUI(SharedComponentArray& sharedComponentArray)
    {
        // last-known widget, for the sake of tab-ordering
        QWidget* lastTabWidget = m_gui->m_addComponentButton;

        for (auto& sharedComponentInfo : sharedComponentArray)
        {
            auto componentEditor = CreateComponentEditor();

            // Add instances to componentEditor
            auto& componentInstances = sharedComponentInfo.m_instances;
            for (AZ::Component* componentInstance : componentInstances)
            {
                // Map the component to the editor
                m_componentToEditorMap[componentInstance] = componentEditor;

                // non-first instances are aggregated under the first instance
                AZ::Component* aggregateInstance = componentInstance != componentInstances.front() ? componentInstances.front() : nullptr;

                // Reference the slice entity if we are a slice so we can indicate differences from base
                AZ::Component* referenceComponentInstance = sharedComponentInfo.m_sliceReferenceComponent;
                componentEditor->AddInstance(componentInstance, aggregateInstance, referenceComponentInstance);
            }

            // Set tab order for editor
            setTabOrder(lastTabWidget, componentEditor);
            lastTabWidget = componentEditor;

            // Refresh editor
            componentEditor->AddNotifications();
            componentEditor->UpdateExpandability();
            componentEditor->InvalidateAll();
            componentEditor->show();
        }
    }

    ComponentEditor* EntityPropertyEditor::CreateComponentEditor()
    {
        //caching allocated component editors for reuse and to preserve order
        if (m_componentEditorsUsed >= m_componentEditors.size())
        {
            //create a new component editor since cache has been exceeded 
            auto componentEditor = aznew ComponentEditor(m_serializeContext, this, this);

            connect(componentEditor, &ComponentEditor::OnExpansionContractionDone, this, [this]()
            {
                setUpdatesEnabled(false);
                layout()->update();
                layout()->activate();
                setUpdatesEnabled(true);
            });
            connect(componentEditor, &ComponentEditor::OnDisplayComponentEditorMenu, this, &EntityPropertyEditor::OnDisplayComponentEditorMenu);
            connect(componentEditor, &ComponentEditor::OnRequestRequiredComponents, this, &EntityPropertyEditor::OnRequestRequiredComponents);
            connect(componentEditor, &ComponentEditor::OnRequestRemoveComponents, this, [this](const AZStd::vector<AZ::Component*>& components) {RemoveComponents(components);});
            connect(componentEditor, &ComponentEditor::OnRequestDisableComponents, this, [this](const AZStd::vector<AZ::Component*>& components) {DisableComponents(components);});
            componentEditor->GetPropertyEditor()->SetValueComparisonFunction([this](const InstanceDataNode* source, const InstanceDataNode* target) { return CompareInstanceDataNodeValues(source, target); });

            //move spacer to bottom of editors
            m_gui->m_componentListContents->layout()->removeItem(m_spacer);
            m_gui->m_componentListContents->layout()->addWidget(componentEditor);
            m_gui->m_componentListContents->layout()->addItem(m_spacer);
            m_gui->m_componentListContents->layout()->update();

            //add new editor to cache
            m_componentEditors.push_back(componentEditor);
        }

        //return editor at current index from cache
        return m_componentEditors[m_componentEditorsUsed++];
    }

    void EntityPropertyEditor::InvalidatePropertyDisplay(PropertyModificationRefreshLevel level)
    {
        if (level == Refresh_None)
        {
            return;
        }

        if (level == Refresh_EntireTree)
        {
            QueuePropertyRefresh();
            return;
        }

        if (level == Refresh_EntireTree_NewContent)
        {
            QueuePropertyRefresh();
            m_shouldScrollToNewComponents = true;
            return;
        }

        for (auto componentEditor : m_componentEditors)
        {
            if (componentEditor->isVisible())
            {
                componentEditor->QueuePropertyEditorInvalidation(level);
            }
        }
    }

    void EntityPropertyEditor::BeforeUndoRedo()
    {
        m_currentUndoOperation = nullptr;
    }

    void EntityPropertyEditor::AfterUndoRedo()
    {
        m_currentUndoOperation = nullptr;
        QueuePropertyRefresh();
    }

    // implementation of IPropertyEditorNotify:
    void EntityPropertyEditor::BeforePropertyModified(InstanceDataNode* pNode)
    {
        if (m_isBuildingProperties)
        {
            AZ_Error("PropertyEditor", !m_isBuildingProperties, "A property is being modified while we're building properties - make sure you do not call RequestWrite except in response to user action!");
            return;
        }

        EntityIdList selectedEntityIds;
        ToolsApplicationRequests::Bus::BroadcastResult(selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);
        if (selectedEntityIds.empty())
        {
            return;
        }

        bool areEntitiesEditable = true;
        ToolsApplicationRequests::Bus::BroadcastResult(areEntitiesEditable, &ToolsApplicationRequests::AreEntitiesEditable, selectedEntityIds);
        if (!areEntitiesEditable)
        {
            return;
        }

        for (AZ::EntityId entityId : selectedEntityIds)
        {
            bool editable = true;
            ToolsApplicationRequests::Bus::BroadcastResult(editable, &ToolsApplicationRequests::IsEntityEditable, entityId);
            if (!editable)
            {
                return;
            }
        }

        if ((m_currentUndoNode == pNode) && (m_currentUndoOperation))
        {
            // attempt to resume the last undo operation:
            ToolsApplicationRequests::Bus::BroadcastResult(m_currentUndoOperation, &ToolsApplicationRequests::ResumeUndoBatch, m_currentUndoOperation, "Modify Entity Property");
        }
        else
        {
            ToolsApplicationRequests::Bus::BroadcastResult(m_currentUndoOperation, &ToolsApplicationRequests::BeginUndoBatch, "Modify Entity Property");
            m_currentUndoNode = pNode;
        }

        // mark the ones' we're editing as dirty, so the user does not have to.
        for (auto entityShownId : selectedEntityIds)
        {
            ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::AddDirtyEntity, entityShownId);
        }
    }

    void EntityPropertyEditor::AfterPropertyModified(InstanceDataNode* pNode)
    {
        AZ_Assert(m_currentUndoOperation, "Invalid undo operation in AfterPropertyModified");
        AZ_Assert(m_currentUndoNode == pNode, "Invalid undo operation in AfterPropertyModified - node doesn't match!");

        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::EndUndoBatch);

        InstanceDataNode* componentNode = pNode;
        while (componentNode->GetParent())
        {
            componentNode = componentNode->GetParent();
        }

        if (!componentNode)
        {
            AZ_Warning("Property Editor", false, "Failed to locate component data associated with the affected field. External change event cannot be sent.");
            return;
        }

        // Send EBus notification for the affected entities, along with information about the affected component.
        for (size_t instanceIdx = 0; instanceIdx < componentNode->GetNumInstances(); ++instanceIdx)
        {
            AZ::Component* componentInstance = m_serializeContext->Cast<AZ::Component*>(
                componentNode->GetInstance(instanceIdx), componentNode->GetClassMetadata()->m_typeId);
            if (componentInstance)
            {
                PropertyEditorEntityChangeNotificationBus::Event(
                    componentInstance->GetEntityId(),
                    &PropertyEditorEntityChangeNotifications::OnEntityComponentPropertyChanged,
                    componentInstance->GetId());
            }
        }
    }

    void EntityPropertyEditor::SetPropertyEditingActive(InstanceDataNode* /*pNode*/)
    {
        MarkPropertyEditorBusyStart();
    }

    void EntityPropertyEditor::SetPropertyEditingComplete(InstanceDataNode* /*pNode*/)
    {
        MarkPropertyEditorBusyEnd();
    }

    void EntityPropertyEditor::OnPropertyRefreshRequired()
    {
        MarkPropertyEditorBusyEnd();
    }

    void EntityPropertyEditor::Reflect(AZ::ReflectContext* /*context*/)
    {
    }

    void EntityPropertyEditor::SetAllowRename(bool allowRename)
    {
        m_gui->m_entityNameEditor->setReadOnly(!allowRename);
    }

    void EntityPropertyEditor::ClearInstances(bool invalidateImmediately)
    {
        for (auto componentEditor : m_componentEditors)
        {
            componentEditor->hide();
            componentEditor->ClearInstances(invalidateImmediately);
        }

        m_componentEditorsUsed = 0;
        m_currentUndoNode = nullptr;
        m_currentUndoOperation = nullptr;
        m_selectedEntityIds.clear();
        m_componentToEditorMap.clear();
    }

    void EntityPropertyEditor::MarkPropertyEditorBusyStart()
    {
        m_currentUndoNode = nullptr;
        m_currentUndoOperation = nullptr;
        m_propertyEditBusy++;
    }

    void EntityPropertyEditor::MarkPropertyEditorBusyEnd()
    {
        m_propertyEditBusy--;
        if (m_propertyEditBusy <= 0)
        {
            m_propertyEditBusy = 0;
            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::SealUndoStack()
    {
        m_currentUndoNode = nullptr;
        m_currentUndoOperation = nullptr;
    }

    void EntityPropertyEditor::QueuePropertyRefresh()
    {
        if (!m_isAlreadyQueuedRefresh)
        {
            m_isAlreadyQueuedRefresh = true;

            // Refresh the properties using a singleShot (with a time of 10ms)
            // this makes sure that the properties aren't refreshed while processing
            // other events, and instead runs after the current events are processed.
            QTimer::singleShot(0, this, &EntityPropertyEditor::UpdateContents);
        }

        SaveComponentEditorState();
    }

    void EntityPropertyEditor::OnAddComponent()
    {
        AZStd::vector<AZ::ComponentServiceType> serviceFilter;
        QPoint position = mapToGlobal(m_gui->m_addComponentButton->pos());
        QSize size = QSize(m_gui->m_addComponentButton->width(), m_gui->m_addComponentButton->height() + m_gui->m_componentList->height());
        ShowComponentPalette(m_componentPalette, position, size, serviceFilter);
    }

    void EntityPropertyEditor::GotSceneSourceControlStatus(SourceControlFileInfo& fileInfo)
    {
        m_sceneIsNew = false;
        EnableEditor(!fileInfo.IsReadOnly());
    }

    void EntityPropertyEditor::PerformActionsBasedOnSceneStatus(bool sceneIsNew, bool readOnly)
    {
        m_sceneIsNew = sceneIsNew;
        EnableEditor(!readOnly);
    }

    void EntityPropertyEditor::EnableEditor(bool enabled)
    {
        m_gui->m_entityNameEditor->setReadOnly(!enabled);
        m_gui->m_addComponentButton->setVisible(enabled);
        m_gui->m_componentListContents->setEnabled(enabled);
        m_gui->m_componentList->setAcceptDrops(true);
    }

    void EntityPropertyEditor::OnEntityNameChanged()
    {
        if (m_gui->m_entityNameEditor->isReadOnly())
        {
            return;
        }

        EntityIdList selectedEntityIds;
        ToolsApplicationRequests::Bus::BroadcastResult(selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        if (selectedEntityIds.size() == 1)
        {
            QByteArray entityNameByteArray = m_gui->m_entityNameEditor->text().toUtf8();

            const AZStd::string entityName(entityNameByteArray.data());

            AZStd::vector<AZ::Entity*> entityList;

            for (AZ::EntityId entityId : selectedEntityIds)
            {
                AZ::Entity* entity = GetEntityById(entityId);

                if (entityName != entity->GetName())
                {
                    entityList.push_back(entity);
                }
            }

            if (entityList.size())
            {
                ScopedUndoBatch undoBatch("ModifyEntityName");

                for (AZ::Entity* entity : entityList)
                {
                    entity->SetName(entityName);
                    ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::AddDirtyEntity, entity->GetId());
                }
            }
        }
    }

    void EntityPropertyEditor::RequestPropertyContextMenu(InstanceDataNode* node, const QPoint& position)
    {
        AZ_Assert(node, "Invalid node passed to context menu callback.");

        if (!node)
        {
            return;
        }

        // Locate the owning component and class data corresponding to the clicked node.
        InstanceDataNode* componentNode = node;
        while (componentNode->GetParent())
        {
            componentNode = componentNode->GetParent();
        }

        // Get inner class data for the clicked component.
        AZ::Component* component = m_serializeContext->Cast<AZ::Component*>(
            componentNode->FirstInstance(), componentNode->GetClassMetadata()->m_typeId);
        if (component)
        {
            auto componentClassData = GetComponentClassData(component);
            if (componentClassData)
            {
                QMenu menu;

                // Populate and show the context menu.
                AddMenuOptionsForComponents(menu, position);

                if (!menu.actions().empty())
                {
                    menu.addSeparator();
                }

                AddMenuOptionsForFields(node, componentNode, componentClassData, menu);

                AddMenuOptionForSliceReset(menu);

                if (!menu.actions().empty())
                {
                    menu.exec(position);
                }
            }
        }
    }

    void EntityPropertyEditor::AddMenuOptionsForComponents(QMenu& menu, const QPoint& /*position*/)
    {
        UpdateActions();
        menu.addActions(actions());
    }

    void EntityPropertyEditor::AddMenuOptionsForFields(
        InstanceDataNode* fieldNode,
        InstanceDataNode* componentNode,
        const AZ::SerializeContext::ClassData* componentClassData,
        QMenu& menu)
    {
        if (!fieldNode || !fieldNode->GetComparisonNode())
        {
            return;
        }

        // For items marked as new (container elements), we'll synchronize the parent node.
        if (fieldNode->IsNewVersusComparison())
        {
            fieldNode = fieldNode->GetParent();
            AZ_Assert(fieldNode && fieldNode->GetClassMetadata() && fieldNode->GetClassMetadata()->m_container, "New element should be a child of a container.");
        }

        // Generate prefab data push/pull options.
        AZ::Component* componentInstance = m_serializeContext->Cast<AZ::Component*>(
            componentNode->FirstInstance(), componentClassData->m_typeId);
        AZ_Assert(componentInstance, "Failed to cast component instance.");

        // With the entity the property ultimately belongs to, we can look up prefab ancestry for push/pull options.
        AZ::Entity* entity = componentInstance->GetEntity();

        AZ::SliceComponent::SliceInstanceAddress address;
        AzFramework::EntityIdContextQueryBus::EventResult(address, entity->GetId(), &AzFramework::EntityIdContextQueries::GetOwningSlice);
        if (!address.first)
        {
            // This entity does not belong to a prefab, so data push/pull is not relevant.
            return;
        }

        AZ::SliceComponent::EntityAncestorList ancestors;
        address.first->GetInstanceEntityAncestry(entity->GetId(), ancestors);

        if (ancestors.empty())
        {
            AZ_Error("PropertyEditor", false, "Entity \"%s\" belongs to a prefab, but its source entity could not be located.");
            return;
        }

        AZStd::string sliceAssetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, ancestors.front().m_sliceAddress.first->GetSliceAsset().GetId());

        bool hasChanges = fieldNode->HasChangesVersusComparison(false);
        bool isLeafNode = !fieldNode->GetClassMetadata()->m_container && fieldNode->GetClassMetadata()->m_serializer;

        AZ::EntityId entityId = entity->GetId();
        QAction* pushAction = menu.addAction(tr("Push to slice..."));
        pushAction->setEnabled(hasChanges);
        connect(pushAction, &QAction::triggered, this, [this, entityId, fieldNode]()
        {
            SliceUtilities::PushEntitiesModal({ entityId }, nullptr);
        }
        );

        menu.addSeparator();

        if (!hasChanges && isLeafNode)
        {
            // Add an option to set the ForceOverride flag for this field
            QAction* forceOverrideAction = menu.addAction(tr("Force property override"));
            connect(forceOverrideAction, &QAction::triggered, this, [this, fieldNode]()
            {
                ContextMenuActionSetDataFlag(fieldNode, AZ::DataPatch::Flag::ForceOverride, true);
            }
            );
        }
        else
        {
            // Add an option to pull data from the first level of the slice (clear the override).
            QAction* pullAction = menu.addAction(tr("Revert property override"));
            pullAction->setEnabled(isLeafNode);
            connect(pullAction, &QAction::triggered, this, [this, componentInstance, fieldNode]()
            {
                ContextMenuActionPullFieldData(componentInstance, fieldNode);
            }
            );
        }
    }

    void EntityPropertyEditor::AddMenuOptionForSliceReset(QMenu& menu)
    {
        bool hasSliceChanges = false;
        bool isPartOfSlice = false;

        const auto& componentsToEdit = GetSelectedComponents();

        // Determine if there are changes for any selected components
        if (!componentsToEdit.empty())
        {
            for (auto component : componentsToEdit)
            {
                AZ_Assert(component, "Parent component is invalid.");
                auto componentEditorIterator = m_componentToEditorMap.find(component);
                AZ_Assert(componentEditorIterator != m_componentToEditorMap.end(), "Unable to find a component editor for the given component");
                if (componentEditorIterator != m_componentToEditorMap.end())
                {
                    auto componentEditor = componentEditorIterator->second;
                    componentEditor->GetPropertyEditor()->EnumerateInstances(
                        [&hasSliceChanges, &isPartOfSlice](InstanceDataHierarchy& hierarchy)
                    {
                        InstanceDataNode* root = hierarchy.GetRootNode();
                        if (root && root->GetComparisonNode())
                        {
                            isPartOfSlice = true;
                            if (root->HasChangesVersusComparison(true))
                            {
                                hasSliceChanges = true;
                            }
                        }
                    }
                    );
                }
            }
        }

        if (isPartOfSlice)
        {
            QAction* resetToSliceAction = menu.addAction(tr("Revert component overrides"));
            resetToSliceAction->setEnabled(hasSliceChanges);
            connect(resetToSliceAction, &QAction::triggered, this, [this]()
            {
                ResetToSlice();
            }
            );
        }
    }


    void EntityPropertyEditor::ContextMenuActionPullFieldData(AZ::Component* parentComponent, InstanceDataNode* fieldNode)
    {
        AZ_Assert(fieldNode && fieldNode->GetComparisonNode(), "Invalid node for prefab data pull.");
        if (!fieldNode->GetComparisonNode())
        {
            return;
        }

        BeforePropertyModified(fieldNode);

        // Clear any slice data flags for this field
        if (GetEntityDataPatchAddress(fieldNode, m_dataPatchAddressBuffer))
        {
            auto clearDataFlagsCommand = aznew ClearSliceDataFlagsBelowAddressCommand(parentComponent->GetEntityId(), m_dataPatchAddressBuffer, "Clear data flags");
            clearDataFlagsCommand->SetParent(m_currentUndoOperation);
        }

        AZStd::unordered_set<InstanceDataNode*> targetContainerNodesWithNewElements;
        InstanceDataHierarchy::CopyInstanceData(fieldNode->GetComparisonNode(), fieldNode, m_serializeContext,
            // Target container child node being removed callback
            nullptr,

            // Source container child node being created callback
            [&targetContainerNodesWithNewElements](const InstanceDataNode* sourceNode, InstanceDataNode* targetNodeParent)
        {
            (void)sourceNode;

            targetContainerNodesWithNewElements.insert(targetNodeParent);
        }
        );

        // Invoke Add notifiers
        for (InstanceDataNode* targetContainerNode : targetContainerNodesWithNewElements)
        {
            AZ_Assert(targetContainerNode->GetClassMetadata()->m_container, "Target container node isn't a container!");
            for (const AZ::Edit::AttributePair& attribute : targetContainerNode->GetElementEditMetadata()->m_attributes)
            {
                if (attribute.first == AZ::Edit::Attributes::AddNotify)
                {
                    AZ::Edit::AttributeFunction<void()>* func = azdynamic_cast<AZ::Edit::AttributeFunction<void()>*>(attribute.second);
                    if (func)
                    {
                        InstanceDataNode* parentNode = targetContainerNode->GetParent();
                        if (parentNode)
                        {
                            for (size_t idx = 0; idx < parentNode->GetNumInstances(); ++idx)
                            {
                                func->Invoke(parentNode->GetInstance(idx));
                            }
                        }
                    }
                }
            }
        }

        // Make sure change notifiers are invoked for the change across any changed nodes in sub-hierarchy
        // under the affected node.
        AZ_Assert(parentComponent, "Parent component is invalid.");
        auto componentEditorIterator = m_componentToEditorMap.find(parentComponent);
        AZ_Assert(componentEditorIterator != m_componentToEditorMap.end(), "Unable to find a component editor for the given component");
        if (componentEditorIterator != m_componentToEditorMap.end())
        {
            AZStd::vector<PropertyRowWidget*> widgetStack;
            AZStd::vector<PropertyRowWidget*> widgetsToNotify;
            widgetStack.reserve(64);
            widgetsToNotify.reserve(64);

            auto componentEditor = componentEditorIterator->second;
            PropertyRowWidget* widget = componentEditor->GetPropertyEditor()->GetWidgetFromNode(fieldNode);
            if (widget)
            {
                widgetStack.push_back(widget);

                while (!widgetStack.empty())
                {
                    PropertyRowWidget* top = widgetStack.back();
                    widgetStack.pop_back();

                    InstanceDataNode* topWidgetNode = top->GetNode();
                    if (topWidgetNode && (topWidgetNode->IsDifferentVersusComparison() || topWidgetNode->IsNewVersusComparison() || topWidgetNode->IsRemovedVersusComparison()))
                    {
                        widgetsToNotify.push_back(top);
                    }

                    for (auto* childWidget : top->GetChildrenRows())
                    {
                        widgetStack.push_back(childWidget);
                    }
                }

                // Issue property notifications, starting with leaf widgets first.
                for (auto iter = widgetsToNotify.rbegin(); iter != widgetsToNotify.rend(); ++iter)
                {
                    (*iter)->DoPropertyNotify();
                }
            }
        }

        AfterPropertyModified(fieldNode);

        // We must refresh the entire tree, because restoring previous entity state may've had major structural effects.
        InvalidatePropertyDisplay(Refresh_EntireTree);
    }

    void EntityPropertyEditor::ContextMenuActionSetDataFlag(InstanceDataNode* node, AZ::DataPatch::Flag flag, bool additive)
    {
        // Attempt to get Entity and DataPatch address from this node.
        AZ::EntityId entityId;
        if (GetEntityDataPatchAddress(node, m_dataPatchAddressBuffer, &entityId))
        {
            BeforePropertyModified(node);

            auto command = aznew SliceDataFlagsCommand(entityId, m_dataPatchAddressBuffer, flag, additive, "Set slice data flag");
            command->SetParent(m_currentUndoOperation);

            AfterPropertyModified(node);
            InvalidatePropertyDisplay(Refresh_AttributesAndValues);
        }
    }

    bool EntityPropertyEditor::GetEntityDataPatchAddress(const InstanceDataNode* node, AZ::DataPatch::AddressType& dataPatchAddressOut, AZ::EntityId* entityIdOut) const
    {
        if (InstanceDataNode* componentNode = node ? node->GetRoot() : nullptr)
        {
            AZ::Component* component = m_serializeContext->Cast<AZ::Component*>(
                componentNode->FirstInstance(), componentNode->GetClassMetadata()->m_typeId);
            if (component)
            {
                AZ::EntityId entityId = component->GetEntityId();
                if (entityId.IsValid())
                {
                    InstanceDataNode::Address nodeAddress = node->ComputeAddress();

                    // Translate InstanceDataNode address into a DataPatch address that's relative to an entity.
                    // Differences:
                    // 1) They're stored in reverse order. nodeAddress is leaf-first. dataPatchAddress is root-first.
                    // 2) nodeAddress stores a CRC of the component's class name, dataPatchAddress stores the ComponentId instead.
                    // 3) dataPatchAddress is relative to the entity, nodeAddress is relative to the component.

                    AZ_Assert(nodeAddress.back() == AZ::Crc32(componentNode->GetClassMetadata()->m_name),
                        "GetEntityDataPatchAddress has bad assumptions about InstanceDataNode addressing");
                    nodeAddress.pop_back(); // throw out CRC of component name

                    // start the dataPatchAddress with the path to this particular Component in the Entity
                    dataPatchAddressOut.clear();
                    dataPatchAddressOut.reserve(nodeAddress.size() + 2);
                    dataPatchAddressOut.push_back(AZ_CRC("Components", 0xee48f5fd));
                    dataPatchAddressOut.push_back(component->GetId());

                    // the rest of the dataPatchAddress is identical to the reversed nodeAddress
                    for (auto reverseIterator = nodeAddress.rbegin(); reverseIterator != nodeAddress.rend(); ++reverseIterator)
                    {
                        dataPatchAddressOut.push_back(*reverseIterator);
                    }

                    if (entityIdOut)
                    {
                        *entityIdOut = entityId;
                    }

                    return true;
                }
            }
        }

        return false;
    }

    bool EntityPropertyEditor::CompareInstanceDataNodeValues(const InstanceDataNode* sourceNode, const InstanceDataNode* targetNode)
    {
        // If target node has ForceOverride flag set, consider it different from source.
        AZ::EntityId entityId;
        if (GetEntityDataPatchAddress(targetNode, m_dataPatchAddressBuffer, &entityId))
        {
            AZ::SliceComponent::SliceInstanceAddress instanceAddress;
            AzFramework::EntityIdContextQueryBus::EventResult(instanceAddress, entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningSlice);
            if (const AZ::SliceComponent::SliceInstance* instance = instanceAddress.second)
            {
                AZ::DataPatch::Flags flags = instance->GetDataFlags().GetEntityDataFlagsAtAddress(entityId, m_dataPatchAddressBuffer);
                if (flags & AZ::DataPatch::Flag::ForceOverride)
                {
                    return false;
                }
            }
        }

        // Otherwise, do the default value comparison.
        return InstanceDataHierarchy::DefaultValueComparisonFunction(sourceNode, targetNode);
    }

    void EntityPropertyEditor::OnDisplayComponentEditorMenu(const QPoint& position)
    {
        QMenu menu;

        AddMenuOptionsForComponents(menu, position);

        if (!menu.actions().empty())
        {
            menu.addSeparator();
        }

        AddMenuOptionForSliceReset(menu);

        if (!menu.actions().empty())
        {
            menu.exec(position);
        }
    }

    void EntityPropertyEditor::OnRequestRequiredComponents(const QPoint& position, const QSize& size, const AZStd::vector<AZ::ComponentServiceType>& services)
    {
        ShowComponentPalette(m_componentPalette, position, size, services);
    }

    void EntityPropertyEditor::HideComponentPalette()
    {
        m_componentPalette->hide();
        m_componentPalette->setGeometry(QRect(QPoint(0, 0), QSize(0, 0)));
    }

    void EntityPropertyEditor::ShowComponentPalette(
        ComponentPaletteWidget* componentPalette,
        const QPoint& position,
        const QSize& size,
        const AZStd::vector<AZ::ComponentServiceType>& serviceFilter)
    {
        HideComponentPalette();

        bool areEntitiesEditable = true;
        ToolsApplicationRequests::Bus::BroadcastResult(areEntitiesEditable, &ToolsApplicationRequests::AreEntitiesEditable, m_selectedEntityIds);
        if (areEntitiesEditable)
        {
            componentPalette->Populate(m_serializeContext, m_selectedEntityIds, m_componentFilter, serviceFilter);
            componentPalette->Present();
            componentPalette->setGeometry(QRect(position, size));
        }
    }

    void EntityPropertyEditor::SetAddComponentMenuFilter(ComponentFilter componentFilter)
    {
        m_componentFilter = AZStd::move(componentFilter);
    }

    void EntityPropertyEditor::CreateActions()
    {
        m_addComponentAction = aznew QAction(tr("Add component"), this);
        m_addComponentAction->setShortcut(QKeySequence::New);
        m_addComponentAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_addComponentAction, &QAction::triggered, this, &EntityPropertyEditor::OnAddComponent);
        addAction(m_addComponentAction);

        m_removeAction = aznew QAction(tr("Remove component"), this);
        m_removeAction->setShortcut(QKeySequence::Delete);
        m_removeAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_removeAction, &QAction::triggered, this, [this]() { RemoveComponents(); });
        addAction(m_removeAction);

        m_cutAction = aznew QAction(tr("Cut component"), this);
        m_cutAction->setShortcut(QKeySequence::Cut);
        m_cutAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_cutAction, &QAction::triggered, this, &EntityPropertyEditor::CutComponents);
        addAction(m_cutAction);

        m_copyAction = aznew QAction(tr("Copy component"), this);
        m_copyAction->setShortcut(QKeySequence::Copy);
        m_copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_copyAction, &QAction::triggered, this, &EntityPropertyEditor::CopyComponents);
        addAction(m_copyAction);

        m_pasteAction = aznew QAction(tr("Paste component"), this);
        m_pasteAction->setShortcut(QKeySequence::Paste);
        m_pasteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_pasteAction, &QAction::triggered, this, &EntityPropertyEditor::PasteComponents);
        addAction(m_pasteAction);

        m_enableAction = aznew QAction(tr("Enable component"), this);
        m_enableAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_enableAction, &QAction::triggered, this, [this]() { EnableComponents(); });
        addAction(m_enableAction);

        m_disableAction = aznew QAction(tr("Disable component"), this);
        m_disableAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_disableAction, &QAction::triggered, this, [this]() { DisableComponents(); });
        addAction(m_disableAction);

        m_moveUpAction = aznew QAction(tr("Move component up"), this);
        m_moveUpAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_moveUpAction, &QAction::triggered, this, &EntityPropertyEditor::MoveComponentsUp);
        addAction(m_moveUpAction);

        m_moveDownAction = aznew QAction(tr("Move component down"), this);
        m_moveDownAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_moveDownAction, &QAction::triggered, this, &EntityPropertyEditor::MoveComponentsDown);
        addAction(m_moveDownAction);

        UpdateActions();
    }

    void EntityPropertyEditor::UpdateActions()
    {
        const auto& componentsToEdit = GetSelectedComponents();

        // Check and see if we are not already at the top
        AzToolsFramework::ComponentOrderArray componentOrder;
        if (!m_selectedEntityIds.empty())
        {
            EditorInspectorComponentRequestBus::EventResult(componentOrder, m_selectedEntityIds.front(), &EditorInspectorComponentRequests::GetComponentOrderArray);
        }

        bool allowRemove = !m_selectedEntityIds.empty() && !componentsToEdit.empty() && AreComponentsRemovable(componentsToEdit);

        m_addComponentAction->setEnabled(!m_selectedEntityIds.empty());
        m_removeAction->setEnabled(allowRemove);
        m_cutAction->setEnabled(allowRemove);
        m_copyAction->setEnabled(allowRemove);
        m_pasteAction->setEnabled(!m_selectedEntityIds.empty() && ComponentMimeData::GetComponentMimeDataFromClipboard());
        m_moveUpAction->setEnabled(allowRemove && componentsToEdit.size() == 1 && !componentOrder.empty() && componentOrder.front() != componentsToEdit.front()->GetId());
        m_moveDownAction->setEnabled(allowRemove && componentsToEdit.size() == 1 && !componentOrder.empty() && componentOrder.back() != componentsToEdit.front()->GetId());

        bool allowEnable = false;
        bool allowDisable = false;

        //build a working set of all components selected for edit
        AZStd::unordered_set<AZ::Component*> enabledComponents(componentsToEdit.begin(), componentsToEdit.end());
        for (const auto& entityId : m_selectedEntityIds)
        {
            AZ::Entity::ComponentArrayType disabledComponents;
            EditorDisabledCompositionRequestBus::EventResult(disabledComponents, entityId, &EditorDisabledCompositionRequests::GetDisabledComponents);

            //remove all disabled components from the set of enabled components
            for (auto disabledComponent : disabledComponents)
            {
                if (enabledComponents.erase(disabledComponent) > 0)
                {
                    //if any disabled components were found/removed from the selected set, assume they can be enabled
                    allowEnable = true;
                }
            }
        }

        //if any components remain in the selected set, assume they can be disabled
        allowDisable = !enabledComponents.empty();

        //disable actions when not allowed
        m_enableAction->setEnabled(allowRemove && allowEnable);
        m_disableAction->setEnabled(allowRemove && allowDisable);

        //additional request to hide actions when not allowed so enable and disable aren't shown at the same time
        m_enableAction->setVisible(allowRemove && allowEnable);
        m_disableAction->setVisible(allowRemove && allowDisable);
    }

    AZStd::vector<AZ::Component*> EntityPropertyEditor::GetCopyableComponents() const
    {
        const auto& selectedComponents = GetSelectedComponents();
        AZStd::vector<AZ::Component*> copyableComponents;
        copyableComponents.reserve(selectedComponents.size());
        for (auto component : selectedComponents)
        {
            const AZ::Entity* entity = component ? component->GetEntity() : nullptr;
            const AZ::EntityId entityId = entity ? entity->GetId() : AZ::EntityId();
            if (entityId == m_selectedEntityIds.front())
            {
                copyableComponents.push_back(component);
            }
        }
        return copyableComponents;
    }

    void EntityPropertyEditor::RemoveComponents(const AZStd::vector<AZ::Component*>& components)
    {
        if (!components.empty() && AreComponentsRemovable(components))
        {
            ScopedUndoBatch undoBatch("Removing components.");

            // Navigation triggered - Right Click in the Entity Inspector on the removed component
            EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu);

            for (auto component : components)
            {
                // Remove Component metrics event (Right Click in the Entity Inspector on the removed component)
                auto componentClassData = GetComponentClassData(component);
                EditorMetricsEventsBus::Broadcast(&EditorMetricsEventsBusTraits::ComponentRemoved, component->GetEntityId(), componentClassData->m_typeId);
            }

            AzToolsFramework::RemoveComponents(components);

            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::RemoveComponents()
    {
        RemoveComponents(GetSelectedComponents());
    }

    void EntityPropertyEditor::CutComponents()
    {
        const auto& componentsToEdit = GetSelectedComponents();
        if (!componentsToEdit.empty() && AreComponentsRemovable(componentsToEdit))
        {
            //intentionally not using EntityCompositionRequests::CutComponents because we only want to copy components from first selected entity
            ScopedUndoBatch undoBatch("Cut components.");
            CopyComponents();
            RemoveComponents();
            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::CopyComponents()
    {
        const auto& componentsToEdit = GetCopyableComponents();
        if (!componentsToEdit.empty() && AreComponentsRemovable(componentsToEdit))
        {
            EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::CopyComponents, componentsToEdit);
            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::PasteComponents()
    {
        if (!m_selectedEntityIds.empty() && ComponentMimeData::GetComponentMimeDataFromClipboard())
        {
            ScopedUndoBatch undoBatch("Paste components.");

            AZStd::vector<AZ::Component*> selectedComponents = GetSelectedComponents();

            //paste to all selected entities
            for (auto entityId : m_selectedEntityIds)
            {
                //get components and order before pasting so we can insert new components above the first selected one
                AZStd::vector<AZ::Component*> componentsInOrder = GetAllComponentsForEntityInOrder(GetEntity(entityId));

                //perform the paste operation which should add new components to the entity or pending list
                EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::PasteComponentsToEntity, entityId);

                //get the post-paste set of components, which should include all prior components plus new ones
                AZStd::vector<AZ::Component*> componentsAfterPaste = GetAllComponentsForEntity(entityId);

                //remove pre-existing components from the post-paste set
                for (auto component : componentsInOrder)
                {
                    componentsAfterPaste.erase(AZStd::remove(componentsAfterPaste.begin(), componentsAfterPaste.end(), component), componentsAfterPaste.end());
                }

                for (auto component : componentsAfterPaste)
                {
                    //add the remaining new components before the first selected one
                    auto insertItr = AZStd::find_first_of(componentsInOrder.begin(), componentsInOrder.end(), selectedComponents.begin(), selectedComponents.end());
                    componentsInOrder.insert(insertItr, component);

                    //only scroll to the bottom if added to the back of the first entity
                    if (insertItr == componentsInOrder.end() && entityId == m_selectedEntityIds.front())
                    {
                        m_shouldScrollToNewComponents = true;
                    }
                }

                //update component order to the new layout
                ComponentOrderArray componentOrderArray;
                componentOrderArray.reserve(componentsInOrder.size());
                for (auto component : componentsInOrder)
                {
                    componentOrderArray.push_back(component->GetId());
                }
                EditorInspectorComponentRequestBus::Event(entityId, &EditorInspectorComponentRequests::SetComponentOrderArray, componentOrderArray);
            }

            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::EnableComponents(const AZStd::vector<AZ::Component*>& components)
    {
        EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::EnableComponents, components);
        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::EnableComponents()
    {
        EnableComponents(GetSelectedComponents());
    }

    void EntityPropertyEditor::DisableComponents(const AZStd::vector<AZ::Component*>& components)
    {
        EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::DisableComponents, components);
        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::DisableComponents()
    {
        DisableComponents(GetSelectedComponents());
    }

    void EntityPropertyEditor::MoveComponentsUp()
    {
        const auto& componentsToEdit = GetSelectedComponents();
        if (m_selectedEntityIds.size() != 1 || componentsToEdit.size() != 1 || !IsComponentRemovable(componentsToEdit.front()))
        {
            return;
        }

        auto firstEntityId = m_selectedEntityIds.front();

        // Check and see if we are not already at the top
        AzToolsFramework::ComponentOrderArray componentOrder;
        EditorInspectorComponentRequestBus::EventResult(componentOrder, firstEntityId, &EditorInspectorComponentRequests::GetComponentOrderArray);

        // Can't move up if already 0
        auto componentItr = AZStd::find(componentOrder.begin(), componentOrder.end(), componentsToEdit.front()->GetId());
        if (componentItr == componentOrder.end() || (*componentItr) == componentOrder.front())
        {
            return;
        }

        AZStd::swap(*(componentItr), *(componentItr - 1));
        EditorInspectorComponentRequestBus::Event(firstEntityId, &EditorInspectorComponentRequests::SetComponentOrderArray, componentOrder);
        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::MoveComponentsDown()
    {
        const auto& componentsToEdit = GetSelectedComponents();
        if (m_selectedEntityIds.size() != 1 || componentsToEdit.size() != 1 || !IsComponentRemovable(componentsToEdit.front()))
        {
            return;
        }

        auto firstEntityId = m_selectedEntityIds.front();

        // Check and see if we are not already at the bottom
        AzToolsFramework::ComponentOrderArray componentOrder;
        EditorInspectorComponentRequestBus::EventResult(componentOrder, firstEntityId, &EditorInspectorComponentRequests::GetComponentOrderArray);

        // Can't move down if already at end
        auto componentItr = AZStd::find(componentOrder.begin(), componentOrder.end(), componentsToEdit.front()->GetId());
        if (componentItr == componentOrder.end() || (*componentItr) == componentOrder.back())
        {
            return;
        }

        AZStd::swap(*(componentItr), *(componentItr + 1));
        EditorInspectorComponentRequestBus::Event(firstEntityId, &EditorInspectorComponentRequests::SetComponentOrderArray, componentOrder);
        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::ResetToSlice()
    {
        // Reset selected components to slice values
        const auto& componentsToEdit = GetSelectedComponents();

        if (!componentsToEdit.empty())
        {
            ScopedUndoBatch undoBatch("Resetting components to slice defaults.");

            for (auto component : componentsToEdit)
            {
                AZ_Assert(component, "Component is invalid.");
                auto componentEditorIterator = m_componentToEditorMap.find(component);
                AZ_Assert(componentEditorIterator != m_componentToEditorMap.end(), "Unable to find a component editor for the given component");
                if (componentEditorIterator != m_componentToEditorMap.end())
                {
                    auto componentEditor = componentEditorIterator->second;
                    componentEditor->GetPropertyEditor()->EnumerateInstances(
                        [this, &component](InstanceDataHierarchy& hierarchy)
                    {
                        InstanceDataNode* root = hierarchy.GetRootNode();
                        ContextMenuActionPullFieldData(component, root);
                    }
                    );
                }
            }

            QueuePropertyRefresh();
        }
    }

    bool EntityPropertyEditor::DoesOwnFocus() const
    {
        for (QWidget* widget = QApplication::focusWidget(); widget; widget = widget->parentWidget())
        {
            if (widget == this)
            {
                return true;
            }
        }
        return false;
    }

    bool EntityPropertyEditor::DoesIntersectWidget(const QRect& rectGlobal, const QWidget* widget) const
    {
        QRect widgetBoundsLocal(widget->rect());
        QRect widgetBoundsGlobal(
            widget->mapToGlobal(widgetBoundsLocal.topLeft()),
            widget->mapToGlobal(widgetBoundsLocal.bottomRight()));
        bool doesWidgetContainPositiont = widgetBoundsGlobal.contains(rectGlobal);
        bool isWidgetEligible = widget->isVisible() && widget->isEnabled();
        return isWidgetEligible && doesWidgetContainPositiont;
    }

    bool EntityPropertyEditor::DoesIntersectSelectedComponentEditor(const QRect& rectGlobal) const
    {
        for (auto componentEditor : GetIntersectingComponentEditors(rectGlobal))
        {
            if (componentEditor->IsSelected())
            {
                return true;
            }
        }
        return false;
    }

    bool EntityPropertyEditor::DoesIntersectNonSelectedComponentEditor(const QRect& rectGlobal) const
    {
        for (auto componentEditor : GetIntersectingComponentEditors(rectGlobal))
        {
            if (!componentEditor->IsSelected())
            {
                return true;
            }
        }
        return false;
    }

    void EntityPropertyEditor::ClearComponentEditorSelection()
    {
        for (auto componentEditor : m_componentEditors)
        {
            componentEditor->SetSelected(false);
        }
        UpdateActions();
    }

    void EntityPropertyEditor::SelectRangeOfComponentEditors(const AZ::s32 index1, const AZ::s32 index2, bool selected)
    {
        if (index1 >= 0 && index2 >= 0)
        {
            const AZ::s32 min = AZStd::min(index1, index2);
            const AZ::s32 max = AZStd::max(index1, index2);
            for (AZ::s32 index = min; index <= max; ++index)
            {
                m_componentEditors[index]->SetSelected(selected);
            }
            m_componentEditorLastSelectedIndex = index2;
        }
    }

    void EntityPropertyEditor::SelectIntersectingComponentEditors(const QRect& rectGlobal, bool selected)
    {
        for (auto componentEditor : GetIntersectingComponentEditors(rectGlobal))
        {
            componentEditor->SetSelected(selected);
            m_componentEditorLastSelectedIndex = GetComponentEditorIndex(componentEditor);
        }
        UpdateActions();
    }

    void EntityPropertyEditor::ToggleIntersectingComponentEditors(const QRect& rectGlobal)
    {
        for (auto componentEditor : GetIntersectingComponentEditors(rectGlobal))
        {
            componentEditor->SetSelected(!componentEditor->IsSelected());
            m_componentEditorLastSelectedIndex = GetComponentEditorIndex(componentEditor);
        }
        UpdateActions();
    }

    AZ::s32 EntityPropertyEditor::GetComponentEditorIndex(const ComponentEditor* componentEditor) const
    {
        AZ::s32 index = 0;
        for (auto componentEditorToCompare : m_componentEditors)
        {
            if (componentEditorToCompare == componentEditor)
            {
                return index;
            }
            ++index;
        }
        return -1;
    }

    ComponentEditorVector EntityPropertyEditor::GetIntersectingComponentEditors(const QRect& rectGlobal) const
    {
        ComponentEditorVector intersectingComponentEditors;
        intersectingComponentEditors.reserve(m_componentEditors.size());
        for (auto componentEditor : m_componentEditors)
        {
            if (DoesIntersectWidget(rectGlobal, componentEditor))
            {
                intersectingComponentEditors.push_back(componentEditor);
            }
        }
        return intersectingComponentEditors;
    }

    ComponentEditorVector EntityPropertyEditor::GetSelectedComponentEditors() const
    {
        //TODO cache selected component editors as optimization if needed
        ComponentEditorVector selectedComponentEditors;
        selectedComponentEditors.reserve(m_componentEditors.size());
        for (auto componentEditor : m_componentEditors)
        {
            if (componentEditor->IsSelected())
            {
                selectedComponentEditors.push_back(componentEditor);
            }
        }
        return selectedComponentEditors;
    }

    AZStd::vector<AZ::Component*> EntityPropertyEditor::GetSelectedComponents() const
    {
        //TODO cache selected components as optimization if needed
        AZStd::vector<AZ::Component*> selectedComponents;
        for (auto componentEditor : m_componentEditors)
        {
            if (componentEditor->IsSelected())
            {
                const auto& components = componentEditor->GetComponents();
                selectedComponents.insert(selectedComponents.end(), components.begin(), components.end());
            }
        }
        return selectedComponents;
    }

    void EntityPropertyEditor::SaveComponentEditorState()
    {
        m_componentEditorSaveStateTable.clear();
        for (auto componentEditor : m_componentEditors)
        {
            for (auto component : componentEditor->GetComponents())
            {
                AZ::ComponentId componentId = component->GetId();
                ComponentEditorSaveState& state = m_componentEditorSaveStateTable[componentId];
                state.m_expanded = componentEditor->IsExpanded();
                state.m_selected = componentEditor->IsSelected();
            }
        }
    }

    void EntityPropertyEditor::LoadComponentEditorState()
    {
        for (auto componentEditor : m_componentEditors)
        {
            ComponentEditorSaveState state;
            if (!componentEditor->GetComponents().empty())
            {
                AZ::ComponentId componentId = componentEditor->GetComponents().front()->GetId();
                state = m_componentEditorSaveStateTable[componentId];
            }

            componentEditor->SetExpanded(state.m_expanded);
            componentEditor->SetSelected(state.m_selected);
        }
        UpdateActions();
    }

    void EntityPropertyEditor::ClearComponentEditorState()
    {
        m_componentEditorSaveStateTable.clear();
    }

    void EntityPropertyEditor::ScrollToNewComponent()
    {
        //force new components to be visible, assuming they are added to the end of the list and layout
        if (m_componentEditorsUsed > 0 &&
            m_componentEditorsUsed <= m_componentEditors.size())
        {
            m_gui->m_componentList->ensureWidgetVisible(
                m_componentEditors[m_componentEditorsUsed - 1]);
        }
        m_shouldScrollToNewComponents = false;
        m_shouldScrollToNewComponentsQueued = false;
    }

    void EntityPropertyEditor::QueueScrollToNewComponent()
    {
        if (m_shouldScrollToNewComponents && !m_shouldScrollToNewComponentsQueued)
        {
            QTimer::singleShot(100, this, &EntityPropertyEditor::ScrollToNewComponent);
            m_shouldScrollToNewComponentsQueued = true;
        }
    }

    void EntityPropertyEditor::BuildEntityIconMenu()
    {
        QMenu menu(this);
        menu.setToolTipsVisible(true);

        QAction* action = nullptr;
        action = menu.addAction(QObject::tr("Set default icon"));
        action->setToolTip(tr("Sets the icon to the first component icon after the transform or uses the transform icon"));
        QObject::connect(action, &QAction::triggered, [this] { SetEntityIconToDefault(); });

        action = menu.addAction(QObject::tr("Set custom icon"));
        action->setToolTip(tr("Choose a custom icon for this entity"));
        QObject::connect(action, &QAction::triggered, [this] { PopupAssetBrowserForEntityIcon(); });

        QPoint pos = m_gui->m_entityIcon->geometry().bottomLeft();
        QPoint globalPos = m_gui->m_entityIcon->parentWidget()->mapToGlobal(pos);
        menu.exec(globalPos);
    }

    void EntityPropertyEditor::SetEntityIconToDefault()
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("Change Entity Icon");

        AZ::Data::AssetId invalidAssetId;
        for (AZ::EntityId entityId : m_selectedEntityIds)
        {
            EditorEntityIconComponentRequestBus::Event(entityId, &EditorEntityIconComponentRequestBus::Events::SetEntityIconAsset, invalidAssetId);
        }

        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::PopupAssetBrowserForEntityIcon()
    {
        /*
        * Request the AssetBrowser Dialog and set a type filter
        */

        if (m_selectedEntityIds.empty())
        {
            return;
        }

        AZ::Data::AssetType entityIconAssetType("{3436C30E-E2C5-4C3B-A7B9-66C94A28701B}");
        AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection(entityIconAssetType);

        AZ::Data::AssetId currentEntityIconAssetId;
        EditorEntityIconComponentRequestBus::EventResult(currentEntityIconAssetId, m_selectedEntityIds.front(), &EditorEntityIconComponentRequestBus::Events::GetEntityIconAssetId);
        selection.SetSelectedAssetId(currentEntityIconAssetId);
        EditorRequests::Bus::Broadcast(&EditorRequests::BrowseForAssets, selection);

        if (selection.IsValid())
        {
            auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
            if (product)
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Change Entity Icon");

                AZ::Data::AssetId iconAssetId = product->GetAssetId();
                for (AZ::EntityId entityId : m_selectedEntityIds)
                {
                    EditorEntityIconComponentRequestBus::Event(entityId, &EditorEntityIconComponentRequestBus::Events::SetEntityIconAsset, iconAssetId);
                }

                QueuePropertyRefresh();
            }
            else
            {
                AZ_Warning("Property Editor", false, "Incorrect asset selected. Expected entity icon asset.");
            }
        }
    }

    void EntityPropertyEditor::contextMenuEvent(QContextMenuEvent* event)
    {
        OnDisplayComponentEditorMenu(event->globalPos());
        event->accept();
    }

    //overridden to intercept application level mouse events for component editor selection
    bool EntityPropertyEditor::eventFilter(QObject* object, QEvent* event)
    {
        (void)object;

        if (m_selectedEntityIds.empty())
        {
            return false;
        }

        if (event->type() != QEvent::MouseButtonPress &&
            event->type() != QEvent::MouseButtonDblClick &&
            event->type() != QEvent::MouseButtonRelease)
        {
            return false;
        }

        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        //reset flag when mouse is released to allow additional selection changes
        if (event->type() == QEvent::MouseButtonRelease)
        {
            m_selectionEventAccepted = false;
            return false;
        }

        //reject input if selection already occurred for this click
        if (m_selectionEventAccepted)
        {
            return false;
        }

        //reject input if a popup or modal window is active
        if (QApplication::activeModalWidget() != nullptr ||
            QApplication::activePopupWidget() != nullptr ||
            QApplication::focusWidget() == m_componentPalette)
        {
            return false;
        }

        QRect rectGlobal(mouseEvent->globalPos(), mouseEvent->globalPos());

        //reject input outside of the inspector's component list
        if (!DoesOwnFocus() ||
            !DoesIntersectWidget(rectGlobal, m_gui->m_componentList))
        {
            return false;
        }

        //reject input from other buttons
        if (mouseEvent->button() != Qt::LeftButton &&
            mouseEvent->button() != Qt::RightButton)
        {
            return false;
        }

        //right click is allowed if the component editor under the mouse is not selected
        if (mouseEvent->button() == Qt::RightButton)
        {
            if (DoesIntersectSelectedComponentEditor(rectGlobal))
            {
                return false;
            }

            ClearComponentEditorSelection();
            SelectIntersectingComponentEditors(rectGlobal, true);
        }
        else if (mouseEvent->button() == Qt::LeftButton)
        {
            //if shift or control is pressed this is a multi=-select operation, otherwise reset the selection
            if (mouseEvent->modifiers() & Qt::ControlModifier)
            {
                ToggleIntersectingComponentEditors(rectGlobal);
            }
            else if (mouseEvent->modifiers() & Qt::ShiftModifier)
            {
                ComponentEditorVector intersections = GetIntersectingComponentEditors(rectGlobal);
                if (!intersections.empty())
                {
                    SelectRangeOfComponentEditors(m_componentEditorLastSelectedIndex, GetComponentEditorIndex(intersections.front()), true);
                }
            }
            else
            {
                ClearComponentEditorSelection();
                SelectIntersectingComponentEditors(rectGlobal, true);
            }
        }

        UpdateActions();

        //ensure selection logic executes only once per click since eventFilter may execute multiple times for a single click
        m_selectionEventAccepted = true;
        return false;
    }

}

#include <UI/PropertyEditor/EntityPropertyEditor.moc>