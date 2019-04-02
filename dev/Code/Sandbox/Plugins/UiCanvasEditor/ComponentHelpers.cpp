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
#include "UiEditorInternalBus.h"
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <LyShine/Bus/UiSystemBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <LyShine/Bus/UiLayoutManagerBus.h>

namespace ComponentHelpers
{
    AZStd::string GetComponentIconPath(const AZ::SerializeContext::ClassData* componentClass)
    {
        AZStd::string iconPath = "Editor/Icons/Components/Component_Placeholder.png";

        auto editorElementData = componentClass->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
        if (auto iconAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::Icon))
        {
            if (auto iconAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(iconAttribute))
            {
                AZStd::string iconAttributeValue = iconAttributeData->Get(nullptr);
                if (!iconAttributeValue.empty())
                {
                    iconPath = AZStd::move(iconAttributeValue);
                }
            }
        }

        // use absolute path if possible
        AZStd::string iconFullPath;
        bool pathFound = false;
        using AssetSysReqBus = AzToolsFramework::AssetSystemRequestBus;
        AssetSysReqBus::BroadcastResult(pathFound, &AssetSysReqBus::Events::GetFullSourcePathFromRelativeProductPath, iconPath, iconFullPath);

        if (pathFound)
        {
            iconPath = iconFullPath;
        }

        return iconPath;
    }

    const char* GetFriendlyComponentName(const AZ::SerializeContext::ClassData& classData)
    {
        return classData.m_editData
               ? classData.m_editData->m_name
               : classData.m_name;
    }

    bool AppearsInUIComponentMenu(const AZ::SerializeContext::ClassData& classData, bool isCanvasSelected)
    {
        return AzToolsFramework::AppearsInAddComponentMenu(classData, isCanvasSelected ? AZ_CRC("CanvasUI", 0xe1e05605) : AZ_CRC("UI", 0x27ff46b0));
    }

    bool IsAddableByUser(const AZ::SerializeContext::ClassData* classData)
    {
        if (classData->m_editData)
        {
            if (auto editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                if (auto addableAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::AddableByUser))
                {
                    // skip this component (return early) if user is not allowed to add it directly
                    if (auto addableData = azdynamic_cast<AZ::Edit::AttributeData<bool>*>(addableAttribute))
                    {
                        if (!addableData->Get(nullptr))
                        {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    bool CanCreateComponentOnEntity(const AZ::SerializeContext::ClassData& componentClassData, AZ::Entity* entity)
    {
        // Get the componentDescriptor from the componentClassData
        AZ::ComponentDescriptor* componentDescriptor = nullptr;
        EBUS_EVENT_ID_RESULT(componentDescriptor, componentClassData.m_typeId, AZ::ComponentDescriptorBus, GetDescriptor);
        if (!componentDescriptor)
        {
            AZStd::string message = AZStd::string::format("ComponentDescriptor not found for %s", GetFriendlyComponentName(componentClassData));
            AZ_Assert(false, message.c_str());
            return false;
        }

        // Get the incompatible, provided and required services from the descriptor
        AZ::ComponentDescriptor::DependencyArrayType incompatibleServices;
        componentDescriptor->GetIncompatibleServices(incompatibleServices, nullptr);

        AZ::ComponentDescriptor::DependencyArrayType providedServices;
        componentDescriptor->GetProvidedServices(providedServices, nullptr);

        AZ::ComponentDescriptor::DependencyArrayType requiredServices;
        componentDescriptor->GetRequiredServices(requiredServices, nullptr);

        // Check if component can be added to this entity
        AZ::ComponentDescriptor::DependencyArrayType services;
        for (const AZ::Component* component : entity->GetComponents())
        {
            const AZ::Uuid& existingComponentTypeId = AzToolsFramework::GetUnderlyingComponentType(*component);

            AZ::ComponentDescriptor* existingComponentDescriptor = nullptr;
            EBUS_EVENT_ID_RESULT(existingComponentDescriptor, existingComponentTypeId, AZ::ComponentDescriptorBus, GetDescriptor);
            if (!componentDescriptor)
            {
                return false;
            }

            services.clear();
            existingComponentDescriptor->GetProvidedServices(services, nullptr);

            // Check that none of the services currently provided by the entity are incompatible services
            // for the new component.
            // Also check that all of the required services of the new component are provided by the
            // existing components
            for (const AZ::ComponentServiceType& service : services)
            {
                for (const AZ::ComponentServiceType& incompatibleService : incompatibleServices)
                {
                    if (service == incompatibleService)
                    {
                        return false;
                    }
                }

                for (auto iter = requiredServices.begin(); iter != requiredServices.end(); ++iter)
                {
                    const AZ::ComponentServiceType& requiredService = *iter;
                    if (service == requiredService)
                    {
                        // this required service has been matched - remove from list
                        requiredServices.erase(iter);
                        break;
                    }
                }
            }

            services.clear();
            existingComponentDescriptor->GetIncompatibleServices(services, nullptr);

            // Check that none of the services provided by the component are incompatible with any
            // of the services currently provided by the entity
            for (const AZ::ComponentServiceType& service : services)
            {
                for (const AZ::ComponentServiceType& providedService : providedServices)
                {
                    if (service == providedService)
                    {
                        return false;
                    }
                }
            }
        }

        if (requiredServices.size() > 0)
        {
            // some required services were not provided by the components on the entity
            return false;
        }

        return true;
    }

    bool CanRemoveComponentFromEntity(AZ::SerializeContext* serializeContext, const AZ::Component* componentToRemove, AZ::Entity* entity)
    {
        const AZ::Uuid& componentToRemoveTypeId = AzToolsFramework::GetUnderlyingComponentType(*componentToRemove);
        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(componentToRemoveTypeId);

        if (!IsAddableByUser(classData))
        {
            return false;
        }

        // Go through all the components on the entity (except this one) and collect all the required services
        // and all the provided services
        AZ::ComponentDescriptor::DependencyArrayType allRequiredServices;
        AZ::ComponentDescriptor::DependencyArrayType allProvidedServices;
        AZ::ComponentDescriptor::DependencyArrayType services;
        for (const AZ::Component* component : entity->GetComponents())
        {
            if (component == componentToRemove)
            {
                continue;
            }

            const AZ::Uuid& componentTypeId = AzToolsFramework::GetUnderlyingComponentType(*component);

            AZ::ComponentDescriptor* componentDescriptor = nullptr;
            EBUS_EVENT_ID_RESULT(componentDescriptor, componentTypeId, AZ::ComponentDescriptorBus, GetDescriptor);
            if (!componentDescriptor)
            {
                return false;
            }

            services.clear();
            componentDescriptor->GetRequiredServices(services, nullptr);

            for (auto requiredService : services)
            {
                allRequiredServices.push_back(requiredService);
            }

            services.clear();
            componentDescriptor->GetProvidedServices(services, nullptr);

            for (auto providedService : services)
            {
                allProvidedServices.push_back(providedService);
            }
        }

        // remove all the satisfied services from the required services lits
        for (auto providedService : allProvidedServices)
        {
            for (auto iter = allRequiredServices.begin(); iter != allRequiredServices.end(); ++iter)
            {
                const AZ::ComponentServiceType& requiredService = *iter;
                if (providedService == requiredService)
                {
                    // this required service has been matched - remove from list
                    allRequiredServices.erase(iter);
                    break;
                }
            }
        }

        // if there is anything left in required services then check if the component we are removing
        // provides any of those services, if so we cannot remove it
        if (allRequiredServices.size() > 0)
        {
            AZ::ComponentDescriptor* componentDescriptor = nullptr;
            EBUS_EVENT_ID_RESULT(componentDescriptor, componentToRemoveTypeId, AZ::ComponentDescriptorBus, GetDescriptor);
            if (!componentDescriptor)
            {
                return false;
            }

            // Get the services provided by the component to be deleted
            AZ::ComponentDescriptor::DependencyArrayType providedServices;
            componentDescriptor->GetProvidedServices(providedServices, nullptr);

            for (auto requiredService: allRequiredServices)
            {
                // Check that none of the services currently still by the entity are the any of the ones we want to remove
                for (auto providedService : providedServices)
                {
                    if (requiredService == providedService)
                    {
                        // this required service is being provided by the component we want to remove
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool CanComponentsBeRemoved(const AZ::Entity::ComponentArrayType& components)
    {
        // Get the serialize context.
        AZ::SerializeContext* serializeContext;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "We should have a valid context!");

        bool canRemove = true;
        for (auto component : components)
        {
            AZ::Entity* entity = component->GetEntity();

            if (!entity || !CanRemoveComponentFromEntity(serializeContext, component, entity))
            {
                canRemove = false;
                break;
            }
        }

        return canRemove;
    }

    bool AreComponentsAddableByUser(const AZ::Entity::ComponentArrayType& components)
    {
        // Get the serialize context.
        AZ::SerializeContext* serializeContext;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "We should have a valid context!");

        bool canAdd = true;
        for (auto component : components)
        {
            const AZ::Uuid& componentToAddTypeId = AzToolsFramework::GetUnderlyingComponentType(*component);
            const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(componentToAddTypeId);

            if (!IsAddableByUser(classData))
            {
                canAdd = false;
                break;
            }
        }

        return canAdd;
    }

    bool CanPasteComponents(const AzToolsFramework::EntityIdList& entities, bool isCanvasSelected)
    {
        const QMimeData* mimeData = AzToolsFramework::ComponentMimeData::GetComponentMimeDataFromClipboard();

        // Check that there are components on the clipboard and that they can all be pasted onto the selected elements
        bool canPasteAll = true;

        if (entities.empty() || !mimeData)
        {
            canPasteAll = false;
        }
        else
        {
            AzToolsFramework::ComponentTypeMimeData::ClassDataContainer classDataForComponentsToAdd;
            AzToolsFramework::ComponentTypeMimeData::Get(mimeData, classDataForComponentsToAdd);

            // Create class data from mime data
            for (const AZ::EntityId& entityId : entities)
            {
                for (auto classData : classDataForComponentsToAdd)
                {
                    auto entity = AzToolsFramework::GetEntityById(entityId);

                    // Check if this component can be added
                    if (!entity
                        || !AppearsInUIComponentMenu(*classData, isCanvasSelected)
                        || !IsAddableByUser(classData)
                        || !CanCreateComponentOnEntity(*classData, entity))
                    {
                        canPasteAll = false;
                        break;
                    }
                }
            }
        }

        return canPasteAll;
    }

    AzToolsFramework::EntityIdList GetSelectedEntities(bool* isCanvasSelectedOut = nullptr)
    {
        AzToolsFramework::EntityIdList selectedEntities;
        EBUS_EVENT_RESULT(selectedEntities, UiEditorInternalRequestBus, GetSelectedEntityIds);

        if (isCanvasSelectedOut)
        {
            *isCanvasSelectedOut = false;
        }
        if (selectedEntities.empty())
        {
            AZ::EntityId canvasEntityId;
            EBUS_EVENT_RESULT(canvasEntityId, UiEditorInternalRequestBus, GetActiveCanvasEntityId);
            selectedEntities.push_back(canvasEntityId);
            if (isCanvasSelectedOut)
            {
                *isCanvasSelectedOut = true;
            }
        }

        return selectedEntities;
    }

    AZ::Entity::ComponentArrayType GetCopyableComponents(const AZ::Entity::ComponentArrayType componentsToCopy)
    {
        AzToolsFramework::EntityIdList selectedEntities = GetSelectedEntities();

        // Copyable components are the components that belong to the first selected entity
        AZ::Entity::ComponentArrayType copyableComponents;
        copyableComponents.reserve(componentsToCopy.size());
        for (auto component : componentsToCopy)
        {
            const AZ::Entity* entity = component ? component->GetEntity() : nullptr;
            const AZ::EntityId entityId = entity ? entity->GetId() : AZ::EntityId();
            if (entityId == selectedEntities.front())
            {
                copyableComponents.push_back(component);
            }
        }

        return copyableComponents;
    }

    void HandleSelectedEntitiesPropertiesChanged()
    {
        EBUS_EVENT(UiEditorInternalNotificationBus, OnSelectedEntitiesPropertyChanged);
    }

    void RemoveComponents(const AZ::Entity::ComponentArrayType& componentsToRemove)
    {
        // Since the undo commands use the selected entities, make sure that the components being removed belong to selected entities
        AzToolsFramework::EntityIdList selectedEntities = GetSelectedEntities();
        bool foundUnselectedEntities = false;
        for (auto componentToRemove : componentsToRemove)
        {
            AZ::Entity* entity = componentToRemove->GetEntity();
            if (AZStd::find(selectedEntities.begin(), selectedEntities.end(), entity->GetId()) == selectedEntities.end())
            {
                foundUnselectedEntities = true;
                break;
            }
        }
        AZ_Assert(!foundUnselectedEntities, "Attempting to remove components from an unselected element");
        if (foundUnselectedEntities)
        {
            return;
        }

        EBUS_EVENT(UiEditorInternalNotificationBus, OnBeginUndoableEntitiesChange);

        // Remove all the components requested
        for (auto componentToRemove : componentsToRemove)
        {
            AZ::Entity* entity = componentToRemove->GetEntity();

            bool reactivate = false;
            // We must deactivate entities to remove components
            if (entity->GetState() == AZ::Entity::ES_ACTIVE)
            {
                reactivate = true;
                entity->Deactivate();
            }

            // See if the component is on the entity
            const auto& entityComponents = entity->GetComponents();
            if (AZStd::find(entityComponents.begin(), entityComponents.end(), componentToRemove) != entityComponents.end())
            {
                entity->RemoveComponent(componentToRemove);

                delete componentToRemove;
            }

            // Attempt to re-activate if we were previously active
            if (reactivate)
            {
                entity->Activate();
            }
        }

        EBUS_EVENT(UiEditorInternalNotificationBus, OnEndUndoableEntitiesChange, "delete component");
    }

    void CopyComponents(const AZ::Entity::ComponentArrayType& copyableComponents)
    {
        // Create the mime data object
        AZStd::unique_ptr<QMimeData> mimeData = AzToolsFramework::ComponentMimeData::Create(copyableComponents);

        // Put it on the clipboard
        AzToolsFramework::ComponentMimeData::PutComponentMimeDataOnClipboard(AZStd::move(mimeData));
    }

    QList<QAction*> CreateAddComponentActions(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems,
        QWidget* parent)
    {
        HierarchyItemRawPtrList items = SelectionHelpers::GetSelectedHierarchyItems(hierarchy,
                selectedItems);

        AzToolsFramework::EntityIdList entitiesSelected;

        bool isCanvasSelected = false;
        if (selectedItems.empty())
        {
            isCanvasSelected = true;
            entitiesSelected.push_back(hierarchy->GetEditorWindow()->GetCanvas());
        }
        else
        {
            for (auto item : items)
            {
                entitiesSelected.push_back(item->GetEntityId());
            }
        }

        using ComponentsList = AZStd::vector < const AZ::SerializeContext::ClassData* >;
        ComponentsList componentsList;

        // Get the serialize context.
        AZ::SerializeContext* serializeContext;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "We should have a valid context!");

        // Gather all components that match our filter and group by category.
        serializeContext->EnumerateDerived<AZ::Component>(
            [ &componentsList, isCanvasSelected ](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid& knownType) -> bool
            {
                (void)knownType;

                if (AppearsInUIComponentMenu(*classData, isCanvasSelected))
                {
                    if (!IsAddableByUser(classData))
                    {
                        return true;
                    }

                    componentsList.push_back(classData);
                }

                return true;
            }
            );

        // Create a component list that is in the same order that the components were registered in
        const AZStd::vector<AZ::Uuid>* componentOrderList;
        ComponentsList orderedComponentsList;
        EBUS_EVENT_RESULT(componentOrderList, UiSystemBus, GetComponentTypesForMenuOrdering);
        for (auto& componentType : * componentOrderList)
        {
            auto iter = AZStd::find_if(componentsList.begin(), componentsList.end(),
                    [componentType](const AZ::SerializeContext::ClassData* classData) -> bool
                    {
                        return (classData->m_typeId == componentType);
                    }
                    );

            if (iter != componentsList.end())
            {
                orderedComponentsList.push_back(*iter);
                componentsList.erase(iter);
            }
        }
        // add any remaining component desciptiors to the end of the ordered list
        // (just to catch any component types that were not registered for ordering)
        for (auto componentClass : componentsList)
        {
            orderedComponentsList.push_back(componentClass);
        }

        QList<QAction*> result;
        {
            // Add an action for each factory.
            for (auto componentClass : orderedComponentsList)
            {
                const char* typeName = componentClass->m_editData->m_name;

                AZStd::string iconPath = GetComponentIconPath(componentClass);

                QString iconUrl = QString(iconPath.c_str());

                bool isEnabled = false;
                if (items.empty())
                {
                    auto canvasEntity = AzToolsFramework::GetEntityById(hierarchy->GetEditorWindow()->GetCanvas());
                    isEnabled = CanCreateComponentOnEntity(*componentClass, canvasEntity);
                }
                else
                {
                    for (auto i : items)
                    {
                        AZ::Entity* entity = i->GetElement();
                        if (CanCreateComponentOnEntity(*componentClass, entity))
                        {
                            isEnabled = true;

                            // Stop iterating thru the items
                            // and go to the next factory.
                            break;
                        }
                    }
                }
                QAction* action = new QAction(QIcon(iconUrl), typeName, parent);
                action->setEnabled(isEnabled);
                QObject::connect(action,
                    &QAction::triggered,
                    hierarchy,
                    [ serializeContext, hierarchy, componentClass, items ](bool checked)
                    {
                        EBUS_EVENT(UiEditorInternalNotificationBus, OnBeginUndoableEntitiesChange);

                        AzToolsFramework::EntityIdList entitiesSelected;
                        if (items.empty())
                        {
                            entitiesSelected.push_back(hierarchy->GetEditorWindow()->GetCanvas());
                        }
                        else
                        {
                            for (auto item : items)
                            {
                                entitiesSelected.push_back(item->GetEntityId());
                            }
                        }

                        for (auto i : entitiesSelected)
                        {
                            AZ::Entity* entity = AzToolsFramework::GetEntityById(i);
                            if (CanCreateComponentOnEntity(*componentClass, entity))
                            {
                                entity->Deactivate();
                                AZ::Component* component;
                                EBUS_EVENT_ID_RESULT(component, componentClass->m_typeId, AZ::ComponentDescriptorBus, CreateComponent);
                                entity->AddComponent(component);
                                entity->Activate();
                            }
                        }

                        EBUS_EVENT(UiEditorInternalNotificationBus, OnEndUndoableEntitiesChange, "add component");

                        HandleSelectedEntitiesPropertiesChanged();
                    });

                result.push_back(action);
            }
        }

        return result;
    }

    QAction* CreateRemoveComponentsAction(QWidget* parent)
    {
        QAction* action = new QAction("Delete component", parent);
        action->setShortcut(QKeySequence::Delete);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            []()
        {
            AZ::Entity::ComponentArrayType componentsToRemove;
            EBUS_EVENT_RESULT(componentsToRemove, UiEditorInternalRequestBus, GetSelectedComponents);

            RemoveComponents(componentsToRemove);

            HandleSelectedEntitiesPropertiesChanged();
        });

        return action;
    }

    void UpdateRemoveComponentsAction(QAction* action)
    {
        AZ::Entity::ComponentArrayType componentsToRemove;
        EBUS_EVENT_RESULT(componentsToRemove, UiEditorInternalRequestBus, GetSelectedComponents);

        action->setText(componentsToRemove.size() > 1 ? "Delete components" : "Delete component");

        // Check if we can remove every component from every element
        bool canRemove = true;
        if (componentsToRemove.empty() || !CanComponentsBeRemoved(componentsToRemove))
        {
            canRemove = false;
        }

        // Disable the action if not every element can remove the component
        action->setEnabled(canRemove);
    }

    QAction* CreateCutComponentsAction(QWidget* parent)
    {
        QAction* action = new QAction("Cut component", parent);
        action->setShortcut(QKeySequence::Cut);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            []()
        {
            AZ::Entity::ComponentArrayType componentsToCut;
            EBUS_EVENT_RESULT(componentsToCut, UiEditorInternalRequestBus, GetSelectedComponents);

            AZ::Entity::ComponentArrayType copyableComponents = GetCopyableComponents(componentsToCut);

            // Copy components
            CopyComponents(copyableComponents);
            // Delete components
            RemoveComponents(componentsToCut);

            HandleSelectedEntitiesPropertiesChanged();
        });

        return action;
    }

    void UpdateCutComponentsAction(QAction* action)
    {
        AZ::Entity::ComponentArrayType componentsToCut;
        EBUS_EVENT_RESULT(componentsToCut, UiEditorInternalRequestBus, GetSelectedComponents);

        AZ::Entity::ComponentArrayType copyableComponents = GetCopyableComponents(componentsToCut);

        action->setText(componentsToCut.size() > 1 ? "Cut components" : "Cut component");

        // Check that all components can be deleted and that all copyable components can be pasted
        bool canCut = true;
        if (copyableComponents.empty() || componentsToCut.empty() || !AreComponentsAddableByUser(copyableComponents) || !CanComponentsBeRemoved(componentsToCut))
        {
            canCut = false;
        }

        // Disable the action if not every component can be deleted or every copyable components pasted
        action->setEnabled(canCut);
    }

    QAction* CreateCopyComponentsAction(QWidget* parent)
    {
        QAction* action = new QAction("Copy component", parent);
        action->setShortcut(QKeySequence::Copy);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            []()
        {
            AZ::Entity::ComponentArrayType componentsToCopy;
            EBUS_EVENT_RESULT(componentsToCopy, UiEditorInternalRequestBus, GetSelectedComponents);

            // Get the components of the first selected elements to copy onto the clipboard
            AZ::Entity::ComponentArrayType copyableComponents = GetCopyableComponents(componentsToCopy);

            CopyComponents(copyableComponents);
        });

        return action;
    }

    void UpdateCopyComponentsAction(QAction* action)
    {
        AZ::Entity::ComponentArrayType componentsToCopy;
        EBUS_EVENT_RESULT(componentsToCopy, UiEditorInternalRequestBus, GetSelectedComponents);

        // Get the components of the first selected elements to copy onto the clipboard
        AZ::Entity::ComponentArrayType copyableComponents = GetCopyableComponents(componentsToCopy);

        action->setText(copyableComponents.size() > 1 ? "Copy components" : "Copy component");

        // Check that all copyable components can be added by the user
        bool canCopy = true;
        if (copyableComponents.empty() || !AreComponentsAddableByUser(copyableComponents))
        {
            canCopy = false;
        }

        // Disable the action if not all copyable components can be added to all elements
        action->setEnabled(canCopy);
    }

    QAction* CreatePasteComponentsAction(QWidget* parent)
    {
        QAction* action = new QAction("Paste component", parent);
        action->setShortcut(QKeySequence::Paste);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            []()
        {
            bool isCanvasSelected = false;
            AzToolsFramework::EntityIdList selectedEntities = GetSelectedEntities(&isCanvasSelected);

            if (CanPasteComponents(selectedEntities, isCanvasSelected))
            {
                EBUS_EVENT(UiEditorInternalNotificationBus, OnBeginUndoableEntitiesChange);

                // Paste to all selected entities
                for (const AZ::EntityId& entityId : selectedEntities)
                {
                    auto entity = AzToolsFramework::GetEntityById(entityId);
                    if (!entity)
                    {
                        continue;
                    }

                    // Create components from mime data
                    const QMimeData* mimeData = AzToolsFramework::ComponentMimeData::GetComponentMimeDataFromClipboard();
                    AzToolsFramework::ComponentMimeData::ComponentDataContainer componentsToAdd;
                    AzToolsFramework::ComponentMimeData::GetComponentDataFromMimeData(mimeData, componentsToAdd);

                    // Create class data from mime data
                    AzToolsFramework::ComponentTypeMimeData::ClassDataContainer classDataForComponentsToAdd;
                    AzToolsFramework::ComponentTypeMimeData::Get(mimeData, classDataForComponentsToAdd);

                    AZ_Assert(componentsToAdd.size() == classDataForComponentsToAdd.size(), "Component mime data's components list size is different from class data list size");
                    if (componentsToAdd.size() == classDataForComponentsToAdd.size())
                    {
                        // Add components
                        for (int componentIndex = 0; componentIndex < componentsToAdd.size(); ++componentIndex)
                        {
                            AZ::Component* component = componentsToAdd[componentIndex];
                            AZ_Assert(component, "Null component provided");
                            if (!component)
                            {
                                // Just skip null components
                                continue;
                            }

                            // Check if this component can be added
                            const AZ::SerializeContext::ClassData& classData = *classDataForComponentsToAdd[componentIndex];
                            bool isCanvas = (UiCanvasBus::FindFirstHandler(entityId));
                            if (AppearsInUIComponentMenu(classData, isCanvas)
                                && IsAddableByUser(&classData)
                                && CanCreateComponentOnEntity(classData, entity))
                            {
                                // Add the component

                                bool reactivate = false;
                                // We must deactivate entities to add components
                                if (entity->GetState() == AZ::Entity::ES_ACTIVE)
                                {
                                    reactivate = true;
                                    entity->Deactivate();
                                }

                                entity->AddComponent(component);

                                // Attempt to re-activate if we were previously active
                                if (reactivate)
                                {
                                    entity->Activate();
                                }
                            }
                            else
                            {
                                // Delete the component
                                delete component;
                            }
                        }
                    }
                }

                EBUS_EVENT(UiEditorInternalNotificationBus, OnEndUndoableEntitiesChange, "paste component");

                HandleSelectedEntitiesPropertiesChanged();
            }
        });

        return action;
    }

    void UpdatePasteComponentsAction(QAction* action)
    {
        const QMimeData* mimeData = AzToolsFramework::ComponentMimeData::GetComponentMimeDataFromClipboard();
        AzToolsFramework::ComponentTypeMimeData::ClassDataContainer classDataForComponentsToAdd;
        AzToolsFramework::ComponentTypeMimeData::Get(mimeData, classDataForComponentsToAdd);

        action->setText(classDataForComponentsToAdd.size() > 1 ? "Paste components" : "Paste component");

        bool isCanvasSelected = false;
        AzToolsFramework::EntityIdList selectedEntities = GetSelectedEntities(&isCanvasSelected);

        // Check that there are components on the clipboard and that they can all be pasted onto the selected elements
        bool canPasteAll = CanPasteComponents(selectedEntities, isCanvasSelected);

        // Disable the action if not every component can be pasted onto every element
        action->setEnabled(canPasteAll);
    }

    AZStd::vector<ComponentTypeData> GetAllComponentTypesThatCanAppearInAddComponentMenu()
    {
        using ComponentsList = AZStd::vector<ComponentTypeData>;
        ComponentsList componentsList;

        // Get the serialize context.
        AZ::SerializeContext* serializeContext;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "We should have a valid context!");

        const AZStd::list<AZ::ComponentDescriptor*>* lyShineComponentDescriptors;
        EBUS_EVENT_RESULT(lyShineComponentDescriptors, UiSystemBus, GetLyShineComponentDescriptors);

        // Gather all components that match our filter and group by category.
        serializeContext->EnumerateDerived<AZ::Component>(
            [ &componentsList, lyShineComponentDescriptors ](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid& knownType) -> bool
            {
                (void)knownType;

                if (AppearsInUIComponentMenu(*classData, false))
                {
                    if (!IsAddableByUser(classData))
                    {
                        // skip this component (return early) if user is not allowed to add it directly
                        return true;
                    }

                    bool isLyShineComponent = false;
                    for (auto descriptor : * lyShineComponentDescriptors)
                    {
                        if (descriptor->GetUuid() == classData->m_typeId)
                        {
                            isLyShineComponent = true;
                            break;
                        }
                    }
                    ComponentTypeData data;
                    data.classData = classData;
                    data.isLyShineComponent = isLyShineComponent;
                    componentsList.push_back(data);
                }

                return true;
            }
            );

        return componentsList;
    }
}   // namespace ComponentHelpers
