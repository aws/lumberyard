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
#include "EditorEntityActionComponent.h"

#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Profiler.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzCore/std/containers/map.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include "EditorEntityHelpers.h"

#include <QMimeData>
#include <QMessageBox>

namespace AzToolsFramework
{
    namespace Components
    {
        namespace
        {
            bool DoesComponentProvideService(const AZ::Component* component, AZ::ComponentServiceType service)
            {
                auto componentDescriptor = GetComponentDescriptor(component);

                AZ::ComponentDescriptor::DependencyArrayType providedServices;
                componentDescriptor->GetProvidedServices(providedServices, component);

                return AZStd::find(providedServices.begin(), providedServices.end(), service) != providedServices.end();
            }

            bool IsComponentIncompatibleWithService(const AZ::Component* component, AZ::ComponentServiceType service)
            {
                auto componentDescriptor = GetComponentDescriptor(component);

                AZ::ComponentDescriptor::DependencyArrayType incompatibleServices;
                componentDescriptor->GetIncompatibleServices(incompatibleServices, component);

                return AZStd::find(incompatibleServices.begin(), incompatibleServices.end(), service) != incompatibleServices.end();
            }

            // Check if existing components provide all services required by component
            bool AreComponentRequiredServicesMet(const AZ::Component* component, const AZ::Entity::ComponentArrayType& existingComponents)
            {
                auto componentDescriptor = GetComponentDescriptor(component);

                AZ::ComponentDescriptor::DependencyArrayType requiredServices;
                componentDescriptor->GetRequiredServices(requiredServices, component);

                // Check it's required dependencies are already met
                for (auto& requiredService : requiredServices)
                {
                    bool serviceMet = false;
                    for (auto existingComponent : existingComponents)
                    {
                        if (DoesComponentProvideService(existingComponent, requiredService))
                        {
                            serviceMet = true;
                            break;
                        }
                    }

                    // Once any service isn't met, we're done
                    if (!serviceMet)
                    {
                        return false;
                    }
                }

                return true;
            }

            // Check if existing components are incompatible with services provided by component
            bool AreExistingComponentsIncompatibleWithComponent(const AZ::Component* component, const AZ::Entity::ComponentArrayType& existingComponents)
            {
                auto componentDescriptor = GetComponentDescriptor(component);

                AZ::ComponentDescriptor::DependencyArrayType providedServices;
                componentDescriptor->GetProvidedServices(providedServices, component);

                for (auto& providedService : providedServices)
                {
                    for (auto existingComponent : existingComponents)
                    {
                        if (IsComponentIncompatibleWithService(existingComponent, providedService))
                        {
                            return true;
                        }
                    }
                }

                return false;
            }

            // Check if component is incompatible with any service provided by existing components
            bool IsComponentIncompatibleWithExistingComponents(const AZ::Component* component, const AZ::Entity::ComponentArrayType& existingComponents)
            {
                auto componentDescriptor = GetComponentDescriptor(component);

                AZ::ComponentDescriptor::DependencyArrayType incompatibleServices;
                componentDescriptor->GetIncompatibleServices(incompatibleServices, component);

                for (auto& incompatibleService : incompatibleServices)
                {
                    for (auto existingComponent : existingComponents)
                    {
                        if (DoesComponentProvideService(existingComponent, incompatibleService))
                        {
                            return true;
                        }
                    }
                }

                return false;
            }

            bool AddAnyValidComponentsToList(AZ::Entity::ComponentArrayType& validComponents, AZ::Entity::ComponentArrayType& componentsToAdd, AZ::Entity::ComponentArrayType* componentsAdded = nullptr)
            {
                bool addedAnyComponentsToList = false;

                for (auto componentToAddIterator = componentsToAdd.begin(); componentToAddIterator != componentsToAdd.end();)
                {
                    // Skip components that don't have requirements met
                    if (!AreComponentRequiredServicesMet(*componentToAddIterator, validComponents))
                    {
                        ++componentToAddIterator;
                        continue;
                    }

                    // Skip components that are incompatible with valid ones
                    if (IsComponentIncompatibleWithExistingComponents(*componentToAddIterator, validComponents))
                    {
                        ++componentToAddIterator;
                        continue;
                    }

                    // Skip components that provide services incompatible with valid ones
                    if (AreExistingComponentsIncompatibleWithComponent(*componentToAddIterator, validComponents))
                    {
                        ++componentToAddIterator;
                        continue;
                    }

                    // Component is now valid, push onto the list and remove from add list
                    addedAnyComponentsToList = true;
                    validComponents.push_back(*componentToAddIterator);
                    if (componentsAdded)
                    {
                        componentsAdded->push_back(*componentToAddIterator);
                    }
                    componentToAddIterator = componentsToAdd.erase(componentToAddIterator);
                }

                return addedAnyComponentsToList;
            }

            AZ::ComponentDescriptor::DependencyArrayType GetUnsatisfiedRequiredDependencies(const AZ::Component* component, AZ::Entity::ComponentArrayType providerComponents)
            {
                AZ::ComponentDescriptor* componentDescriptor = GetComponentDescriptor(component);

                AZ::ComponentDescriptor::DependencyArrayType requiredServices;
                componentDescriptor->GetRequiredServices(requiredServices, component);

                AZ::ComponentDescriptor::DependencyArrayType unsatisfiedRequiredDependencies;
                for (auto requiredService : requiredServices)
                {
                    bool serviceIsSatisfied = false;
                    for (auto providerComponent : providerComponents)
                    {
                        if (DoesComponentProvideService(providerComponent, requiredService))
                        {
                            serviceIsSatisfied = true;
                            break;
                        }
                    }

                    if (!serviceIsSatisfied)
                    {
                        unsatisfiedRequiredDependencies.push_back(requiredService);
                    }
                }
                return unsatisfiedRequiredDependencies;
            }

            bool ComponentsAreIncompatible(const AZ::Component* componentA, AZ::Component* componentB)
            {
                return IsComponentIncompatibleWithExistingComponents(componentA, { componentB }) || AreExistingComponentsIncompatibleWithComponent(componentA, { componentB });
            }
        } // namespace

        void EditorEntityActionComponent::Init()
        {
        }

        void EditorEntityActionComponent::Activate()
        {
            EntityCompositionRequestBus::Handler::BusConnect();
        }

        void EditorEntityActionComponent::Deactivate()
        {
            EntityCompositionRequestBus::Handler::BusDisconnect();
        }


        void EditorEntityActionComponent::CutComponents(const AZStd::vector<AZ::Component*>& components)
        {
            // Create the mime data object which will have a serialized copy of the components
            AZStd::unique_ptr<QMimeData> mimeData = ComponentMimeData::Create(components);
            if (!mimeData)
            {
                return;
            }

            // Try to remove the components
            auto removalOutcome = RemoveComponents(components);

            // Put it on the clipboard if succeeded
            if (removalOutcome)
            {
                ComponentMimeData::PutComponentMimeDataOnClipboard(AZStd::move(mimeData));
            }
        }

        void EditorEntityActionComponent::CopyComponents(const AZStd::vector<AZ::Component*>& components)
        {
            // Create the mime data object
            AZStd::unique_ptr<QMimeData> mimeData = ComponentMimeData::Create(components);

            // Put it on the clipboard
            ComponentMimeData::PutComponentMimeDataOnClipboard(AZStd::move(mimeData));
        }

        void EditorEntityActionComponent::PasteComponentsToEntity(AZ::EntityId entityId)
        {
            auto entity = GetEntityById(entityId);
            if (!entity)
            {
                return;
            }

            // Grab component data from clipboard, if exists
            const QMimeData* mimeData = ComponentMimeData::GetComponentMimeDataFromClipboard();

            if (!mimeData)
            {
                AZ_Error("Editor", false, "No component data was found on the clipboard to paste.");
                return;
            }

            // Create components from mime data
            AZStd::vector<AZ::Component*> components;
            ComponentMimeData::GetComponentDataFromMimeData(mimeData, components);

            auto addComponentsOutcome = AddExistingComponentsToEntity(entity, components);
            if (!addComponentsOutcome)
            {
                AZ_Error("Editor", false, "Pasting components to entity failed to add");
                return;
            }

            auto componentsAdded = addComponentsOutcome.GetValue().m_componentsAdded;
            AZ_Assert(components.size() == componentsAdded.size(), "Amount of components added is not the same amount of components requested");
            for (int componentIndex = 0; componentIndex < components.size(); ++componentIndex)
            {
                auto component = components[componentIndex];
                auto componentAdded = componentsAdded[componentIndex];
                AZ_Assert(GetComponentClassData(componentAdded) == GetComponentClassData(component), "Component added is not the same type as requested");
                componentAdded = AZStd::move(component);
            }
        }

        bool EditorEntityActionComponent::HasComponentsToPaste()
        {
            return ComponentMimeData::GetComponentMimeDataFromClipboard() != nullptr;
        }

        void EditorEntityActionComponent::EnableComponents(const AZStd::vector<AZ::Component*>& components)
        {
            ScopedUndoBatch undoBatch("Enabling components.");

            // Enable all the components requested
            for (auto component : components)
            {
                AZ::Entity* entity = component->GetEntity();

                bool isEntityEditable = false;
                EBUS_EVENT_RESULT(isEntityEditable, AzToolsFramework::ToolsApplicationRequests::Bus, IsEntityEditable, entity->GetId());
                if (!isEntityEditable)
                {
                    continue;
                }

                undoBatch.MarkEntityDirty(entity->GetId());

                bool reactivate = false;
                // We must deactivate entities to remove components
                if (entity->GetState() == AZ::Entity::ES_ACTIVE)
                {
                    reactivate = true;
                    entity->Deactivate();
                }

                //calling SetEntity changes the component id so we save id to restore when component is moved
                auto componentId = component->GetId();

                //force component to be removed from entity or related containers
                RemoveComponentFromEntityAndContainers(entity, component);

                GetEditorComponent(component)->SetEntity(entity);

                component->SetId(componentId);

                //move the component to the pending container
                AzToolsFramework::EditorPendingCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::AddPendingComponent, component);

                //update entity state after component shuffling
                ScrubEntity(entity);

                // Attempt to re-activate if we were previously active
                if (reactivate)
                {
                    // If previously active, attempt to re-activate entity
                    entity->Activate();

                    // If entity is not active now, something failed hardcore
                    AZ_Assert(entity->GetState() == AZ::Entity::ES_ACTIVE, "Failed to reactivate entity even after scrubbing on component removal");
                }
            }
        }

        void EditorEntityActionComponent::DisableComponents(const AZStd::vector<AZ::Component*>& components)
        {
            ScopedUndoBatch undoBatch("Disabling components.");

            // Disable all the components requested
            for (auto component : components)
            {
                AZ::Entity* entity = component->GetEntity();

                bool isEntityEditable = false;
                EBUS_EVENT_RESULT(isEntityEditable, AzToolsFramework::ToolsApplicationRequests::Bus, IsEntityEditable, entity->GetId());
                if (!isEntityEditable)
                {
                    continue;
                }

                undoBatch.MarkEntityDirty(entity->GetId());

                bool reactivate = false;
                // We must deactivate entities to remove components
                if (entity->GetState() == AZ::Entity::ES_ACTIVE)
                {
                    reactivate = true;
                    entity->Deactivate();
                }

                //calling SetEntity changes the component id so we save id to restore when component is moved
                auto componentId = component->GetId();

                //force component to be removed from entity or related containers
                RemoveComponentFromEntityAndContainers(entity, component);

                GetEditorComponent(component)->SetEntity(entity);

                component->SetId(componentId);

                //move the component to the disabled container
                AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::AddDisabledComponent, component);

                //update entity state after component shuffling
                ScrubEntity(entity);

                // Attempt to re-activate if we were previously active
                if (reactivate)
                {
                    // If previously active, attempt to re-activate entity
                    entity->Activate();

                    // If entity is not active now, something failed hardcore
                    AZ_Assert(entity->GetState() == AZ::Entity::ES_ACTIVE, "Failed to reactivate entity even after scrubbing on component removal");
                }
            }
        }

        EditorEntityActionComponent::RemoveComponentsOutcome EditorEntityActionComponent::RemoveComponents(const AZStd::vector<AZ::Component*>& componentsToRemove)
        {
            EntityToRemoveComponentsResultMap resultMap;
            {
                ScopedUndoBatch undoBatch("Removing components.");

                // Only remove, do not delete components until we know it was successful
                AZStd::vector<AZ::Component*> removedComponents;

                // Remove all the components requested
                for (auto componentToRemove : componentsToRemove)
                {
                    AZ::Entity* entity = componentToRemove->GetEntity();

                    bool isEntityEditable = false;
                    EBUS_EVENT_RESULT(isEntityEditable, AzToolsFramework::ToolsApplicationRequests::Bus, IsEntityEditable, entity->GetId());
                    if (!isEntityEditable)
                    {
                        continue;
                    }

                    undoBatch.MarkEntityDirty(entity->GetId());

                    bool reactivate = false;
                    // We must deactivate entities to remove components
                    if (entity->GetState() == AZ::Entity::ES_ACTIVE)
                    {
                        reactivate = true;
                        entity->Deactivate();
                    }

                    if (RemoveComponentFromEntityAndContainers(entity, componentToRemove))
                    {
                        removedComponents.push_back(componentToRemove);
                    }

                    // Run the scrubber and store the result
                    resultMap.emplace(entity->GetId(), AZStd::move(ScrubEntity(entity)));

                    // Attempt to re-activate if we were previously active
                    if (reactivate)
                    {
                        // If previously active, attempt to re-activate entity
                        entity->Activate();

                        // If entity is not active now, something failed hardcore
                        AZ_Assert(entity->GetState() == AZ::Entity::ES_ACTIVE, "Failed to reactivate entity even after scrubbing on component removal");
                    }
                }

                for (auto removedComponent : removedComponents)
                {
                    delete removedComponent;
                }
            }

            return AZ::Success(AZStd::move(resultMap));
        }


        bool EditorEntityActionComponent::RemoveComponentFromEntityAndContainers(AZ::Entity* entity, AZ::Component* componentToRemove)
        {
            // See if the component is on the entity proper
            const auto& entityComponents = entity->GetComponents();
            if (AZStd::find(entityComponents.begin(), entityComponents.end(), componentToRemove) != entityComponents.end())
            {
                entity->RemoveComponent(componentToRemove);
                return true;
            }

            // Check for pending component
            AZStd::vector<AZ::Component*> pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
            if (AZStd::find(pendingComponents.begin(), pendingComponents.end(), componentToRemove) != pendingComponents.end())
            {
                AzToolsFramework::EditorPendingCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::RemovePendingComponent, componentToRemove);

                // Remove the entity* in this case because it'll try to call RemoveComponent if it isn't null
                GetEditorComponent(componentToRemove)->SetEntity(nullptr);
                return true;
            }

            // Check for disabled component
            AZStd::vector<AZ::Component*> disabledComponents;
            AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
            if (AZStd::find(disabledComponents.begin(), disabledComponents.end(), componentToRemove) != disabledComponents.end())
            {
                AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::RemoveDisabledComponent, componentToRemove);

                // Remove the entity* in this case because it'll try to call RemoveComponent if it isn't null
                GetEditorComponent(componentToRemove)->SetEntity(nullptr);
                return true;
            }

            AZ_Assert(false, "Component being removed was not on the entity, pending, or disabled lists.");
            return false;
        }

        EditorEntityActionComponent::AddComponentsOutcome EditorEntityActionComponent::AddComponentsToEntities(const EntityIdList& entityIds, const AZ::ComponentTypeList& componentsToAdd)
        {
            if (entityIds.empty() || componentsToAdd.empty())
            {
                return AZ::Success(AddComponentsOutcome::ValueType());
            }

            // Validate componentsToAdd uuids and convert into class data
            AzToolsFramework::ClassDataList componentsToAddClassData;
            for (auto componentToAddUuid : componentsToAdd)
            {
                if (componentToAddUuid.IsNull())
                {
                    return AZ::Failure(AZStd::string::format("Invalid component uuid (%s) provided to AddComponentsToEntities, no components have been added", componentToAddUuid.template ToString<AZStd::string>()));
                }
                auto componentClassData = GetComponentClassDataForType(componentToAddUuid);
                if (!componentClassData)
                {
                    return AZ::Failure(AZStd::string::format("Invalid class data from uuid (%s) provided to AddComponentsToEntities, no components have been added", componentToAddUuid.template ToString<AZStd::string>()));
                }
                componentsToAddClassData.push_back(componentClassData);
            }

            ScopedUndoBatch undo("Adding components to entity");
            EntityToAddedComponentsMap entityToAddedComponentsMap;
            {
                for (auto& entityId : entityIds)
                {
                    auto entity = GetEntityById(entityId);
                    if (!entity)
                    {
                        continue;
                    }

                    auto& entityComponentsResult = entityToAddedComponentsMap[entityId];

                    AZStd::vector<AZ::Component*> componentsToAddToEntity;
                    for (auto& componentClassData : componentsToAddClassData)
                    {
                        AZ_Assert(componentClassData, "null component class data provided to AddComponentsToEntities");
                        if (!componentClassData)
                        {
                            continue;
                        }

                        // Create component.
                        // If it's not an "editor component" then wrap it in a GenericComponentWrapper.
                        AZ::Component* component = nullptr;
                        bool isEditorComponent = componentClassData->m_azRtti && componentClassData->m_azRtti->IsTypeOf(Components::EditorComponentBase::RTTI_Type());
                        if (isEditorComponent)
                        {
                            EBUS_EVENT_ID_RESULT(component, componentClassData->m_typeId, AZ::ComponentDescriptorBus, CreateComponent);
                        }
                        else
                        {
                            component = aznew Components::GenericComponentWrapper(componentClassData);
                        }

                        componentsToAddToEntity.push_back(component);
                    }

                    auto addExistingComponentsResult = AddExistingComponentsToEntity(entity, componentsToAddToEntity);
                    // This should never fail since we check the preconditions already (entity is non-null and it ignores null components)
                    AZ_Assert(addExistingComponentsResult, "Adding the components created to an entity failed");
                    if (addExistingComponentsResult)
                    {
                        // Repackage the single-entity result into the overall result
                        entityComponentsResult = addExistingComponentsResult.GetValue();
                    }
                    
                }
            }

            return AZ::Success(AZStd::move(entityToAddedComponentsMap));
        }

        EditorEntityActionComponent::AddExistingComponentsOutcome EditorEntityActionComponent::AddExistingComponentsToEntity(AZ::Entity* entity, const AZStd::vector<AZ::Component*>& componentsToAdd)
        {
            if (!entity)
            {
                return AZ::Failure(AZStd::string("Null entity provided to AddExistingComponentsToEntity"));
            }

            ScopedUndoBatch undo("Add existing components to entity");

            AddComponentsResults addComponentsResults;

            // Add all components to the pending list
            for (auto component : componentsToAdd)
            {
                AZ_Assert(component, "Null component provided to AddExistingComponentsToEntity");
                if (!component)
                {
                    // Just skip null components
                    continue;
                }

                // Obliterate any existing component id to allow the entity to set the id
                component->SetId(AZ::InvalidComponentId);

                // Set the entity on the component (but do not add yet) so that existing systems such as UI will work properly and understand who this component belongs to.
                GetEditorComponent(component)->SetEntity(entity);

                // Add component to pending for entity
                AzToolsFramework::EditorPendingCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::AddPendingComponent, component);

                addComponentsResults.m_componentsAdded.push_back(component);

                undo.MarkEntityDirty(entity->GetId());
            }

            // Run the scrubber!
            auto scrubEntityResult = ScrubEntity(entity);
            // Adding components to pending should NEVER invalidate existing components
            AZ_Assert(!scrubEntityResult.m_invalidatedComponents.size(), "Adding pending components invalidated existing components!");
            // Loop over all the components we added, if they were validated during the scrub, put them in the valid result
            // Otherwise, they are still pending
            for (auto componentAddedToEntity : addComponentsResults.m_componentsAdded)
            {
                auto iterValidComponent = AZStd::find(scrubEntityResult.m_validatedComponents.begin(), scrubEntityResult.m_validatedComponents.end(), componentAddedToEntity);
                if (iterValidComponent != scrubEntityResult.m_validatedComponents.end())
                {
                    addComponentsResults.m_addedValidComponents.push_back(componentAddedToEntity);
                    scrubEntityResult.m_validatedComponents.erase(iterValidComponent);
                }
                else
                {
                    addComponentsResults.m_addedPendingComponents.push_back(componentAddedToEntity);
                }
            }

            // Any left over validated components are other components that happened to get validated because of our change and return those, but separately
            addComponentsResults.m_additionalValidatedComponents.swap(scrubEntityResult.m_validatedComponents);

            return AZ::Success(AZStd::move(addComponentsResults));
        }

        EditorEntityActionComponent::ScrubEntityResult EditorEntityActionComponent::ScrubEntity(AZ::Entity* entity)
        {
            ScrubEntityResult result;
            ScopedUndoBatch undo("Scrub entity");

            bool entityWasActive = entity->GetState() == AZ::Entity::State::ES_ACTIVE;
            if (entityWasActive)
            {
                entity->Deactivate();
            }

            // Components are assumed invalid until proven valid
            AZ::Entity::ComponentArrayType validComponents;
            AZ::Entity::ComponentArrayType invalidComponents = entity->GetComponents();

            // Keep looping to add any component that has it's dependencies met
            // This will keep checking invalid components against more and more valid components
            // Until we cannot add any further components, which terminates the loop
            while (AddAnyValidComponentsToList(validComponents, invalidComponents)); // <- Intential semicolon

            // Move invalid components from the entity to the pending queue
            for (auto invalidComponent : invalidComponents)
            {
                result.m_invalidatedComponents.push_back(invalidComponent);

                // Save the component ID since RemoveComponent will reset it
                auto componentId = invalidComponent->GetId();

                entity->RemoveComponent(invalidComponent);
                undo.MarkEntityDirty(entity->GetId());

                // Restore the component ID and entity*
                invalidComponent->SetId(componentId);

                GetEditorComponent(invalidComponent)->SetEntity(entity);

                // Add the component to the pending list
                EBUS_EVENT_ID(entity->GetId(), AzToolsFramework::EditorPendingCompositionRequestBus, AddPendingComponent, invalidComponent);
            }

            // Attempt to add pending components
            auto addPendingOutcome = AddPendingComponentsToEntity(entity);
            if (addPendingOutcome)
            {
                result.m_validatedComponents.swap(addPendingOutcome.GetValue());
            }

            if (entityWasActive)
            {
                entity->Activate();
                // This should never occur as AddPendingComponentsToEntity guarantees that the operation will succeed.
                AZ_Assert(entity->GetState() == AZ::Entity::State::ES_ACTIVE, "Failed to re-activate entity during ScrubEntity.", entity->GetName().c_str());
            }
            return result;
        }

        EditorEntityActionComponent::AddPendingComponentsOutcome EditorEntityActionComponent::AddPendingComponentsToEntity(AZ::Entity* entity)
        {
            // We ensure our operations should succeed, we do not take care of entity maintenance
            AZ_Assert(entity->GetState() != AZ::Entity::State::ES_ACTIVE, "AddPendingComponentsToEntity assumes that the calling function is handling entity deactivation/reactivation");
            ScopedUndoBatch undo("Added pending components to entity");

            // Same looping algorithm as the scrubber, but we'll also get the list of added components so we can clean up the pending list if we were successful
            AZStd::vector<AZ::Component*> pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);

            AZ::Entity::ComponentArrayType addedPendingComponents;
            AZ::Entity::ComponentArrayType currentComponents = entity->GetComponents();
            while (AddAnyValidComponentsToList(currentComponents, pendingComponents, &addedPendingComponents)); // <- Intential semicolon

            if (addedPendingComponents.size())
            {
                for (auto addedPendingComponent : addedPendingComponents)
                {
                    // Save off the component id, reset the entity pointer since it will be checked in AddComponent otherwise
                    auto componentId = addedPendingComponent->GetId();
                    GetEditorComponent(addedPendingComponent)->SetEntity(nullptr);
                    // Restore the component id, in case it got changed
                    addedPendingComponent->SetId(componentId);

                    if (entity->AddComponent(addedPendingComponent))
                    {
                        AzToolsFramework::EditorPendingCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::RemovePendingComponent, addedPendingComponent);
                    }
                }
            }

            return AZ::Success(addedPendingComponents);
        }

        EditorEntityActionComponent::PendingComponentInfo EditorEntityActionComponent::GetPendingComponentInfo(const AZ::Component* component)
        {
            EditorEntityActionComponent::PendingComponentInfo pendingComponentInfo;

            if (!component || !component->GetEntity())
            {
                return pendingComponentInfo;
            }

            AZStd::vector<AZ::Component*> pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(component->GetEntityId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
            auto iterPendingComponent = AZStd::find(pendingComponents.begin(), pendingComponents.end(), component);

            //removing assert so we can query pending info for non-pending components withut calling
            //GetPendingComponents twice and just returning
            //AZ_Assert(iterPendingComponent != pendingComponents.end(), "Component being queried was not in the pending list");
            if (iterPendingComponent == pendingComponents.end())
            {
                return pendingComponentInfo;
            }

            const auto& entityComponents = component->GetEntity()->GetComponents();
            // See what is not already provided by valid components
            auto servicesNotSatisfiedByValidComponents = GetUnsatisfiedRequiredDependencies(component, entityComponents);

            for (auto serviceNotSatisfiedByValidComponent : servicesNotSatisfiedByValidComponents)
            {
                // Check if pending provides and then list them under waiting on list
                bool serviceSatisfiedByPendingComponent = false;
                for (auto pendingComponent : pendingComponents)
                {
                    if (DoesComponentProvideService(pendingComponent, serviceNotSatisfiedByValidComponent))
                    {
                        // Ensure if pending component provides a service it isn't incompatible with the component
                        if (ComponentsAreIncompatible(component, pendingComponent))
                        {
                            continue;
                        }
                        serviceSatisfiedByPendingComponent = true;
                        pendingComponentInfo.m_pendingComponentsWithRequiredServices.push_back(pendingComponent);
                        break;
                    }
                }

                // If pending does not provide, add to required services list
                if (!serviceSatisfiedByPendingComponent)
                {
                    pendingComponentInfo.m_missingRequiredServices.push_back(serviceNotSatisfiedByValidComponent);
                }
            }

            // Check for incompatibility with only the entity components, we don't care about other pending components
            for (auto entityComponent : entityComponents)
            {
                // Either we are incompatible with them or they would be incompatible with us
                if (ComponentsAreIncompatible(component, entityComponent))
                {
                    pendingComponentInfo.m_validComponentsThatAreIncompatible.push_back(entityComponent);
                }
            }

            return pendingComponentInfo;
        }

        AZStd::string EditorEntityActionComponent::GetComponentName(const AZ::Component* component)
        {
            return GetFriendlyComponentName(component);
        }

        void EditorEntityActionComponent::OnEntityStreamLoadSuccess()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            AZ::SliceComponent* rootSlice = nullptr;
            EBUS_EVENT_RESULT(rootSlice, EditorEntityContextRequestBus, GetEditorRootSlice);

            AZ::SliceComponent::EntityList entitiesLoaded;
            rootSlice->GetEntities(entitiesLoaded);

            // Validate all loaded entities
            for (auto& entity : entitiesLoaded)
            {
                // Run the scrubber!
                ScrubEntity(entity);
            }
        }

        void EditorEntityActionComponent::Reflect(AZ::ReflectContext* context)
        {
            ComponentMimeData::Reflect(context);
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<EditorEntityActionComponent, AZ::Component>();
            }
        }

        void EditorEntityActionComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("EntityCompositionRequests", 0x29838b44));
        }

        void EditorEntityActionComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("EntityCompositionRequests", 0x29838b44));
        }

        void EditorEntityActionComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& /*required*/)
        {
        }

    } // namespace Components
} // namespace AzToolsFramework
