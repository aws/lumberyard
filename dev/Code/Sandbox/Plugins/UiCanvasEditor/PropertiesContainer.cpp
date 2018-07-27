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

#include "EditorCommon.h"
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <LyShine/Bus/UiSystemBus.h>

//-------------------------------------------------------------------------------

#define UICANVASEDITOR_EXTRA_VERTICAL_PADDING_IN_PIXELS (8)

//-------------------------------------------------------------------------------

PropertiesContainer::PropertiesContainer(PropertiesWidget* propertiesWidget,
    EditorWindow* editorWindow)
    : QScrollArea(propertiesWidget)
    , m_propertiesWidget(propertiesWidget)
    , m_editorWindow(editorWindow)
    , m_containerWidget(new QWidget(this))
    , m_rowLayout(new QVBoxLayout(m_containerWidget))
    , m_selectedEntityDisplayNameWidget(nullptr)
    , m_selectionHasChanged(false)
    , m_isCanvasSelected(false)
{
    // Hide the border.
    setStyleSheet("QScrollArea { border: 0px hidden transparent; }");

    m_rowLayout->setContentsMargins(0, 0, 0, 0);
    m_rowLayout->setSpacing(0);

    setWidgetResizable(true);
    setFocusPolicy(Qt::ClickFocus);
    setWidget(m_containerWidget);

    // Get the serialize context.
    EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(m_serializeContext, "We should have a valid context!");
}

void PropertiesContainer::BuildSharedComponentList(ComponentTypeMap& sharedComponentsByType, const AzToolsFramework::EntityIdList& entitiesShown)
{
    // For single selection of a slice-instanced entity, gather the direct slice ancestor
    // so we can visualize per-component differences.
    m_compareToEntity.reset();
    if (1 == entitiesShown.size())
    {
        AZ::SliceComponent::SliceInstanceAddress address;
        EBUS_EVENT_ID_RESULT(address, entitiesShown[0], AzFramework::EntityIdContextQueryBus, GetOwningSlice);
        if (address.first)
        {
            AZ::SliceComponent::EntityAncestorList ancestors;
            address.first->GetInstanceEntityAncestry(entitiesShown[0], ancestors, 1);

            if (!ancestors.empty())
            {
                m_compareToEntity = AzToolsFramework::SliceUtilities::CloneSliceEntityForComparison(*ancestors[0].m_entity, *address.second, *m_serializeContext);
            }
        }
    }

    // Create a SharedComponentInfo for each component
    // that selected entities have in common.
    // See comments on SharedComponentInfo for more details
    for (AZ::EntityId entityId : entitiesShown)
    {
        AZ::Entity* entity = nullptr;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);
        AZ_Assert(entity, "Entity was selected but no such entity exists?");
        if (!entity)
        {
            continue;
        }

        // Track how many of each component-type we've seen on this entity
        AZStd::unordered_map<AZ::Uuid, size_t> entityComponentCounts;

        for (AZ::Component* component : entity->GetComponents())
        {
            const AZ::Uuid& componentType = azrtti_typeid(component);
            const AZ::SerializeContext::ClassData* classData = m_serializeContext->FindClassData(componentType);

            // Skip components without edit data
            if (!classData || !classData->m_editData)
            {
                continue;
            }

            // Skip components that are set to invisible.
            if (const AZ::Edit::ElementData* editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                if (AZ::Edit::Attribute* visibilityAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::Visibility))
                {
                    AzToolsFramework::PropertyAttributeReader reader(component, visibilityAttribute);
                    AZ::u32 visibilityValue;
                    if (reader.Read<AZ::u32>(visibilityValue))
                    {
                        if (visibilityValue == AZ_CRC("PropertyVisibility_Hide", 0x32ab90f7))
                        {
                            continue;
                        }
                    }
                }
            }

            // The sharedComponentList is created based on the first entity.
            if (entityId == *entitiesShown.begin())
            {
                // Add new SharedComponentInfo
                SharedComponentInfo sharedComponent;
                sharedComponent.m_classData = classData;
                sharedComponentsByType[componentType].push_back(sharedComponent);
            }

            // Skip components that don't correspond to a type from the first entity.
            if (sharedComponentsByType.find(componentType) == sharedComponentsByType.end())
            {
                continue;
            }

            // Update entityComponentCounts (may be multiple components of this type)
            auto entityComponentCountsIt = entityComponentCounts.find(componentType);
            size_t componentIndex = (entityComponentCountsIt == entityComponentCounts.end())
                ? 0
                : entityComponentCountsIt->second;
            entityComponentCounts[componentType] = componentIndex + 1;

            // Skip component if the first entity didn't have this many.
            if (componentIndex >= sharedComponentsByType[componentType].size())
            {
                continue;
            }

            // Component accepted! Add it as an instance
            SharedComponentInfo& sharedComponent = sharedComponentsByType[componentType][componentIndex];
            sharedComponent.m_instances.push_back(component);

            // If specified, locate the corresponding component in the comparison entity to
            // visualize differences.
            if (m_compareToEntity && !sharedComponent.m_compareInstance)
            {
                size_t compareComponentIndex = 0;
                for (AZ::Component* compareComponent : m_compareToEntity.get()->GetComponents())
                {
                    const AZ::Uuid& compareComponentType = azrtti_typeid(compareComponent);
                    if (componentType == compareComponentType)
                    {
                        if (componentIndex == compareComponentIndex)
                        {
                            sharedComponent.m_compareInstance = compareComponent;
                            break;
                        }
                        compareComponentIndex++;
                    }
                }
            }
        }
    }

    // Cull any SharedComponentInfo that doesn't fit all our requirements
    ComponentTypeMap::iterator sharedComponentsByTypeIterator = sharedComponentsByType.begin();
    while (sharedComponentsByTypeIterator != sharedComponentsByType.end())
    {
        AZStd::vector<SharedComponentInfo>& sharedComponents = sharedComponentsByTypeIterator->second;

        // Remove component if it doesn't exist on every entity
        AZStd::vector<SharedComponentInfo>::iterator sharedComponentIterator = sharedComponents.begin();
        while (sharedComponentIterator != sharedComponents.end())
        {
            if (sharedComponentIterator->m_instances.size() != entitiesShown.size()
                || sharedComponentIterator->m_instances.empty())
            {
                sharedComponentIterator = sharedComponents.erase(sharedComponentIterator);
            }
            else
            {
                ++sharedComponentIterator;
            }
        }

        // Remove entry if all its components were culled
        if (sharedComponents.size() == 0)
        {
            sharedComponentsByTypeIterator = sharedComponentsByType.erase(sharedComponentsByTypeIterator);
        }
        else
        {
            ++sharedComponentsByTypeIterator;
        }
    }
}

void PropertiesContainer::BuildSharedComponentUI(ComponentTypeMap& sharedComponentsByType, const AzToolsFramework::EntityIdList& entitiesShown)
{
    (void)entitiesShown;

    // At this point in time:
    // - Each SharedComponentInfo should contain one component instance
    //   from each selected entity.
    // - Any pre-existing m_componentEditor entries should be
    //   cleared of component instances.

    // Add each component instance to its corresponding editor
    // We add them in the order that the component factories were registered in, this provides
    // a consistent order of components. It doesn't appear to be the case that components always
    // stay in the order they were added to the entity in, some of our slices do not have the
    // UiElementComponent first for example.
    const AZStd::vector<AZ::Uuid>* componentTypes;
    EBUS_EVENT_RESULT(componentTypes, UiSystemBus, GetComponentTypesForMenuOrdering);

    // There could be components that were not registered for component ordering. We don't
    // want to hide them. So add them at the end of the list
    AZStd::vector<AZ::Uuid> componentOrdering;
    componentOrdering = *componentTypes;
    for (auto sharedComponentMapEntry : sharedComponentsByType)
    {
        if (AZStd::find(componentOrdering.begin(), componentOrdering.end(), sharedComponentMapEntry.first) == componentOrdering.end())
        {
            componentOrdering.push_back(sharedComponentMapEntry.first);
        }
    }

    for (auto& componentType : componentOrdering)
    {
        if (sharedComponentsByType.count(componentType) <= 0)
        {
            continue; // there are no components of this type in the sharedComponentsByType map
        }

        const auto& sharedComponents = sharedComponentsByType[componentType];

        for (size_t sharedComponentIndex = 0; sharedComponentIndex < sharedComponents.size(); ++sharedComponentIndex)
        {
            const SharedComponentInfo& sharedComponent = sharedComponents[sharedComponentIndex];

            AZ_Assert(sharedComponent.m_instances.size() == entitiesShown.size()
                && !sharedComponent.m_instances.empty(),
                "sharedComponentsByType should only contain valid entries at this point");

            // Create an editor if necessary
            AZStd::vector<AzToolsFramework::ReflectedPropertyEditor*>& componentEditors = m_componentEditorsByType[componentType];
            bool createdEditor = false;
            if (sharedComponentIndex >= componentEditors.size())
            {
                componentEditors.push_back(CreatePropertyEditor());
                createdEditor = true;
            }

            AzToolsFramework::ReflectedPropertyEditor& componentEditor = *componentEditors[sharedComponentIndex];

            // Add instances to componentEditor
            for (AZ::Component* componentInstance : sharedComponent.m_instances)
            {
                // Note that in the case of a GenericComponentWrapper,
                // we give the editor the GenericComponentWrapper
                // rather than the underlying type.
                void* classPtr = componentInstance;
                const AZ::Uuid& classType = azrtti_typeid(componentInstance);

                // non-first instances are aggregated under the first instance
                void* aggregateInstance = nullptr;
                if (componentInstance != sharedComponent.m_instances.front())
                {
                    aggregateInstance = sharedComponent.m_instances.front();
                }

                void* compareInstance = sharedComponent.m_compareInstance;

                componentEditor.AddInstance(classPtr, classType, aggregateInstance, compareInstance);
            }

            // Refresh editor
            componentEditor.InvalidateAll();
            componentEditor.show();
        }
    }
}

AzToolsFramework::ReflectedPropertyEditor* PropertiesContainer::CreatePropertyEditor()
{
    AzToolsFramework::ReflectedPropertyEditor* editor = new AzToolsFramework::ReflectedPropertyEditor(nullptr);
    m_rowLayout->addWidget(editor);
    editor->hide();

    const int propertyLabelWidth = 150;
    editor->Setup(m_serializeContext, m_propertiesWidget, true, propertyLabelWidth);
    editor->SetSavedStateKey(AZ_CRC("UiCanvasEditor_PropertyEditor", 0xc402ebcc));
    editor->SetLabelAutoResizeMinimumWidth(propertyLabelWidth);
    editor->SetAutoResizeLabels(true);

    QObject::connect(editor,
        &AzToolsFramework::ReflectedPropertyEditor::OnExpansionContractionDone,
        this,
        &PropertiesContainer::SetHeightOfContentRect);

    return editor;
}

void PropertiesContainer::Update()
{
    size_t selectedEntitiesAmount = m_selectedEntities.size();
    QString displayName;

    if (selectedEntitiesAmount == 0)
    {
        displayName = "No Canvas Loaded";
    }
    else if (selectedEntitiesAmount == 1)
    {
        // Either only one element selected, or none (still is 1 because it selects the canvas instead)

        // If the canvas was selected
        if (m_isCanvasSelected)
        {
            displayName = "Canvas";
        }
        // Else one element was selected
        else
        {
            // Set the name in the properties pane to the name of the element
            AZ::EntityId selectedElement = m_selectedEntities.front();
            AZStd::string selectedEntityName;
            EBUS_EVENT_ID_RESULT(selectedEntityName, selectedElement, UiElementBus, GetName);
            displayName = selectedEntityName.c_str();
        }
    }
    else // more than one entity selected
    {
        displayName = ToString(selectedEntitiesAmount) + " elements selected";
    }

    // Update the selected element display name
    if (m_selectedEntityDisplayNameWidget != nullptr)
    {
        m_selectedEntityDisplayNameWidget->setText(displayName);
    }

    // Clear content.
    {
        for (int j = m_rowLayout->count(); j > 0; --j)
        {
            AzToolsFramework::ReflectedPropertyEditor* editor = static_cast<AzToolsFramework::ReflectedPropertyEditor*>(m_rowLayout->itemAt(j - 1)->widget());

            editor->hide();
            editor->ClearInstances();
        }

        m_compareToEntity.reset();
    }

    if (m_selectedEntities.empty())
    {
        return; // nothing to do
    }

    ComponentTypeMap sharedComponentList;
    BuildSharedComponentList(sharedComponentList, m_selectedEntities);
    BuildSharedComponentUI(sharedComponentList, m_selectedEntities);
    SetHeightOfContentRect();
}

void PropertiesContainer::SetHeightOfContentRect()
{
    int sumContentsRect = 0;

    for (auto& componentEditorsPair : m_componentEditorsByType)
    {
        for (AzToolsFramework::ReflectedPropertyEditor* editor : componentEditorsPair.second)
        {
            if (editor)
            {
                // We DON'T want to scroll thru individual editors.
                // We ONLY want to scroll thru the ENTIRE layout of editors.
                // We achieve this by setting each editor's minimum height
                // to its full layout and setting the container's height
                // (this class) to the full span of all its sub-editors.
                int minimumHeight = editor->GetContentHeight() + UICANVASEDITOR_EXTRA_VERTICAL_PADDING_IN_PIXELS;

                editor->setMinimumHeight(minimumHeight);

                sumContentsRect += minimumHeight;
            }
        }
    }

    // Set the container's maximum height.
    m_containerWidget->setMaximumHeight(sumContentsRect);
}

void PropertiesContainer::Refresh(AzToolsFramework::PropertyModificationRefreshLevel refreshLevel, const AZ::Uuid* componentType)
{
    if (m_selectionHasChanged)
    {
        Update();
        m_selectionHasChanged = false;
    }
    else
    {
        for (auto& componentEditorsPair : m_componentEditorsByType)
        {
            if (!componentType || (*componentType == componentEditorsPair.first))
            {
                for (AzToolsFramework::ReflectedPropertyEditor* editor : componentEditorsPair.second)
                {
                    if (editor)
                    {
                        editor->QueueInvalidation(refreshLevel);
                    }
                }
            }
        }

        // If the selection has not changed, but a refresh was prompted then the name of the currently selected entity might
        // have changed.
        size_t selectedEntitiesAmount = m_selectedEntities.size();
        // Check if only one entity is selected and that it is an element
        if (selectedEntitiesAmount == 1 && !m_isCanvasSelected)
        {
            // Set the name in the properties pane to the name of the element
            AZ::EntityId selectedElement = m_selectedEntities.front();
            AZStd::string selectedEntityName;
            EBUS_EVENT_ID_RESULT(selectedEntityName, selectedElement, UiElementBus, GetName);

            // Update the selected element display name
            if (m_selectedEntityDisplayNameWidget != nullptr)
            {
                m_selectedEntityDisplayNameWidget->setText(selectedEntityName.c_str());
            }
        }
    }
}

void PropertiesContainer::SelectionChanged(HierarchyItemRawPtrList* items)
{
    m_selectedEntities.clear();
    if (items)
    {
        for (auto i : *items)
        {
            m_selectedEntities.push_back(i->GetEntityId());
        }
    }

    m_isCanvasSelected = false;

    if (m_selectedEntities.empty())
    {
        // Add the canvas
        AZ::EntityId canvasId = m_editorWindow->GetCanvas();
        if (canvasId.IsValid())
        {
            m_selectedEntities.push_back(canvasId);
            m_isCanvasSelected = true;
        }
    }

    m_selectionHasChanged = true;
}

void PropertiesContainer::SelectedEntityPointersChanged()
{
    m_selectionHasChanged = true;
    Refresh();
}

void PropertiesContainer::RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* node, const QPoint& globalPos)
{
    AZ::Component* componentToRemove = nullptr;
    {
        while (node)
        {
            if ((node->GetClassMetadata()) && (node->GetClassMetadata()->m_azRtti))
            {
                if (node->GetClassMetadata()->m_azRtti->IsTypeOf(AZ::Component::RTTI_Type()))
                {
                    AZ::Component* component = static_cast<AZ::Component*>(node->FirstInstance());
                    // Only break if the component we got was a component on an entity, not just a member variable component
                    if (component->GetEntity() != nullptr)
                    {
                        componentToRemove = component;
                        break;
                    }
                }
            }

            node = node->GetParent();
        }
    }

    HierarchyMenu contextMenu(m_editorWindow->GetHierarchy(),
        HierarchyMenu::Show::kRemoveComponents | HierarchyMenu::Show::kPushToSlice,
        false,
        componentToRemove);

    if (!contextMenu.isEmpty())
    {
        contextMenu.exec(globalPos);
    }
}

void PropertiesContainer::SetSelectedEntityDisplayNameWidget(QLabel* selectedEntityDisplayNameWidget)
{
    m_selectedEntityDisplayNameWidget = selectedEntityDisplayNameWidget;
}

#include <PropertiesContainer.moc>
