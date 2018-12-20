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

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentExport.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/Components/TransformComponent.h>

#include <AzToolsFramework/Slice/SliceCompilation.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>

namespace AzToolsFramework
{
    namespace Internal
    {
        using ShouldExportResult = AZ::Outcome<bool, AZStd::string>;                            // Outcome describing whether a component should be exported based on
                                                                                                // user EditContext attributes. Error string provided in failure case.
        using ExportedComponentResult = AZ::Outcome<AZ::ExportedComponent, AZStd::string>;      // Outcome describing final resolved component for export.
                                                                                                // Error string provided in error case.

        /**
         * Checks EditContext attributes to determine if the input component should be exported based on the current platform tags.
         */
        ShouldExportResult ShouldExportComponent(AZ::Component* component, const AZ::PlatformTagSet& platformTags, AZ::SerializeContext& serializeContext)
        {
            const AZ::SerializeContext::ClassData* classData = serializeContext.FindClassData(component->RTTI_GetType());
            if (!classData || !classData->m_editData)
            {
                return AZ::Success(true);
            }

            const AZ::Edit::ElementData* editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
            if (!editorDataElement)
            {
                return AZ::Success(true);
            }

            AZ::Edit::Attribute* allTagsAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::ExportIfAllPlatformTags);
            AZ::Edit::Attribute* anyTagsAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::ExportIfAnyPlatformTags);

            AZStd::vector<AZ::Crc32> attributeTags;

            // If the component has declared the 'ExportIfAllPlatforms' attribute, skip export if any of the flags are not present.
            if (allTagsAttribute)
            {
                attributeTags.clear();
                PropertyAttributeReader reader(component, allTagsAttribute);
                if (!reader.Read<decltype(attributeTags)>(attributeTags))
                {
                    return AZ::Failure(AZStd::string("'ExportIfAllPlatforms' attribute is not bound to the correct return type. Expects AZStd::vector<AZ::Crc32>."));
                }

                for (const AZ::Crc32 tag : attributeTags)
                {
                    if (platformTags.find(tag) == platformTags.end())
                    {
                        // Export platform tags does not contain all tags specified in 'ExportIfAllPlatforms' attribute.
                        return AZ::Success(false);
                    }
                }
            }

            // If the component has declared the 'ExportIfAnyPlatforms' attribute, skip export if none of the flags are present.
            if (anyTagsAttribute)
            {
                attributeTags.clear();
                PropertyAttributeReader reader(component, anyTagsAttribute);
                if (!reader.Read<decltype(attributeTags)>(attributeTags))
                {
                    return AZ::Failure(AZStd::string("'ExportIfAnyPlatforms' attribute is not bound to the correct return type. Expects AZStd::vector<AZ::Crc32>."));
                }

                bool anyFlagSet = false;
                for (const AZ::Crc32 tag : attributeTags)
                {
                    if (platformTags.find(tag) != platformTags.end())
                    {
                        anyFlagSet = true;
                        break;
                    }
                }
                if (!anyFlagSet)
                {
                    // None of the flags in 'ExportIfAnyPlatforms' was present in the export platform tags.
                    return AZ::Success(false);
                }
            }

            return AZ::Success(true);
        }

        /**
         * Recursively resolves to the component that should be exported to the runtime slice per the current platform flags
         * and any custom user export callbacks.
         * This is recursive to allow deep exports, such an editor component exporting a runtime component, which in turn
         * exports a custom version of itself depending on platform.
         */
        ExportedComponentResult ResolveExportedComponent(AZ::ExportedComponent component, 
            const AZ::PlatformTagSet& platformTags, 
            AZ::SerializeContext& serializeContext)
        {
            AZ::Component* inputComponent = component.m_component;

            if (!inputComponent)
            {
                return AZ::Success(AZ::ExportedComponent(component));
            }

            // Don't export the component if it has unmet platform tag requirements.
            ShouldExportResult shouldExportResult = ShouldExportComponent(inputComponent, platformTags, serializeContext);
            if (!shouldExportResult)
            {
                return AZ::Failure(shouldExportResult.TakeError());
            }

            if (!shouldExportResult.GetValue())
            {
                return AZ::Success(AZ::ExportedComponent());
            }

            // Determine if the component has a custom export callback, and invoke it if so.
            const AZ::SerializeContext::ClassData* classData = serializeContext.FindClassData(inputComponent->RTTI_GetType());
            if (classData && classData->m_editData)
            {
                const AZ::Edit::ElementData* editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                if (editorDataElement)
                {
                    AZ::Edit::Attribute* exportCallbackAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::RuntimeExportCallback);
                    if (exportCallbackAttribute)
                    {
                        PropertyAttributeReader reader(inputComponent, exportCallbackAttribute);
                        AZ::ExportedComponent exportedComponent;

                        if (reader.Read<AZ::ExportedComponent>(exportedComponent, inputComponent, platformTags))
                        {
                            // If the callback provided a different component instance, continue to resolve recursively.
                            if (exportedComponent.m_component != inputComponent)
                            {
                                return ResolveExportedComponent(exportedComponent, platformTags, serializeContext);
                            }
                        }
                        else
                        {
                            return AZ::Failure(AZStd::string("Bound 'CustomExportCallback' does not have the required return type/signature."));
                        }
                    }
                }
            }

            auto* asEditorComponent = azrtti_cast<Components::EditorComponentBase*>(component.m_component);
            if (asEditorComponent)
            {
                // Only export runtime components.
                return AZ::Success(AZ::ExportedComponent());
            }

            return AZ::Success(component);
        }

        /**
         * Identify and remove any entities marked as editor-only.
         * If any are discovered, adjust descendants' transforms to retain spatial relationships.
         * Note we cannot use EBuses for this purpose, since we're crunching data, and can't assume any entities are active.
         */
        EditorOnlyEntityHandler::Result 
        AdjustForEditorOnlyEntities(AZ::SliceComponent* slice, const AZStd::unordered_set<AZ::EntityId>& editorOnlyEntities, AZ::SerializeContext& serializeContext, EditorOnlyEntityHandler* customHandler)
        {
            AzToolsFramework::EntityList entities;
            slice->GetEntities(entities);

            // Invoke custom handler if provided, so callers can process the slice to account for editor-only entities
            // that are about to be removed.
            if (customHandler)
            {
                const EditorOnlyEntityHandler::Result handlerResult = customHandler->HandleEditorOnlyEntities(entities, editorOnlyEntities, serializeContext);
                if (!handlerResult)
                {
                    return handlerResult;
                }
            }

            // Remove editor-only entities from the slice's final entity list.
            for (auto entityIter = entities.begin(); entityIter != entities.end(); )
            {
                AZ::Entity* entity = (*entityIter);

                if (editorOnlyEntities.find(entity->GetId()) != editorOnlyEntities.end())
                {
                    entityIter = entities.erase(entityIter);
                    slice->RemoveEntity(entity);
                }
                else
                {
                    ++entityIter;
                }
            }

            return AZ::Success();
        }

    } // namespace Internal

    /**
     * Compiles the provided source slice into a runtime slice.
     * Components are validated and exported considering platform tags and EditContext-driven user validation and export customizations.
     */
    SliceCompilationResult CompileEditorSlice(const AZ::Data::Asset<AZ::SliceAsset>& sourceSliceAsset, const AZ::PlatformTagSet& platformTags, AZ::SerializeContext& serializeContext, EditorOnlyEntityHandler* editorOnlyEntityHandler)
    {
        if (!sourceSliceAsset)
        {
            return AZ::Failure(AZStd::string("Source slice is invalid."));
        }

        AZ::SliceComponent::EntityList sourceEntities;
        sourceSliceAsset.Get()->GetComponent()->GetEntities(sourceEntities);

        // Create a new target slice asset to which we'll export entities & components.
        AZ::Entity* exportSliceEntity = aznew AZ::Entity(sourceSliceAsset.Get()->GetEntity()->GetId());
        AZ::SliceComponent* exportSliceData = exportSliceEntity->CreateComponent<AZ::SliceComponent>();
        AZ::Data::Asset<AZ::SliceAsset> exportSliceAsset(aznew AZ::SliceAsset(sourceSliceAsset.GetId()));
        exportSliceAsset.Get()->SetData(exportSliceEntity, exportSliceData);

        // For export, components can assume they're initialized, but not activated.
        for (AZ::Entity* sourceEntity : sourceEntities)
        {
            if (sourceEntity->GetState() == AZ::Entity::ES_CONSTRUCTED)
            {
                sourceEntity->Init();
            }
        }

        // Prepare source entity container for validation callbacks.
        AZ::ImmutableEntityVector immutableSourceEntities;
        immutableSourceEntities.reserve(sourceEntities.size());
        for (AZ::Entity* entity : sourceEntities)
        {
            immutableSourceEntities.push_back(entity);
        }

        AZStd::unordered_set<AZ::EntityId> editorOnlyEntities;

        // Prepare entities for export. This involves invoking BuildGameEntity on source
        // entity's components, targeting a separate entity for export.
        for (AZ::Entity* sourceEntity : sourceEntities)
        {
            AZ::Entity* exportEntity = aznew AZ::Entity(sourceEntity->GetId(), sourceEntity->GetName().c_str());
            exportEntity->SetRuntimeActiveByDefault(sourceEntity->IsRuntimeActiveByDefault());

            bool isEditorOnly = false;
            EditorOnlyEntityComponentRequestBus::EventResult(isEditorOnly, sourceEntity->GetId(), &EditorOnlyEntityComponentRequests::IsEditorOnlyEntity);
            if (isEditorOnly)
            {
                editorOnlyEntities.insert(sourceEntity->GetId());
            }

            const AZ::Entity::ComponentArrayType& editorComponents = sourceEntity->GetComponents();
            for (AZ::Component* component : editorComponents)
            {
                auto* asEditorComponent =
                    azrtti_cast<Components::EditorComponentBase*>(component);

                if (asEditorComponent)
                {
                    // Call validation callback on source editor component.
                    AZ::ComponentValidationResult result = asEditorComponent->ValidateComponentRequirements(immutableSourceEntities);
                    if (!result.IsSuccess())
                    {
                        // Try to cast to GenericComponentWrapper, and if we can, get the internal template.
                        const char* componentName = asEditorComponent->RTTI_GetTypeName();
                        Components::GenericComponentWrapper* wrapper = azrtti_cast<Components::GenericComponentWrapper*>(asEditorComponent);
                        if (wrapper && wrapper->GetTemplate())
                        {
                            componentName = wrapper->GetTemplate()->RTTI_GetTypeName();
                        }

                        return AZ::Failure(AZStd::string::format("[Entity] \"%s\" [Entity Id] 0x%llu, [Editor Component] \"%s\" could not pass validation for [Slice] \"%s\" [Error] %s",
                            sourceEntity->GetName().c_str(),
                            sourceEntity->GetId(),
                            componentName,
                            sourceSliceAsset.GetHint().c_str(),
                            result.GetError().c_str()));
                    }

                    // Resolve the runtime component to export.
                    const Internal::ExportedComponentResult exportResult = Internal::ResolveExportedComponent(
                        AZ::ExportedComponent(asEditorComponent, false), 
                        platformTags, 
                        serializeContext);

                    if (!exportResult)
                    {
                        return AZ::Failure(AZStd::string::format("Source component \"%s\" could not be exported for Entity \"%s\" [0x%llu] due to export attributes: %s.",
                            component->RTTI_GetTypeName(),
                            exportEntity->GetName().c_str(),
                            static_cast<AZ::u64>(exportEntity->GetId()),
                            exportResult.GetError().c_str()));
                    }

                    AZ::ExportedComponent exportedComponent = exportResult.GetValue();

                    if (exportedComponent.m_component)
                    {
                        AZ::Component* runtimeComponent = exportedComponent.m_component;

                        // If the final component is not owned by us, make our own copy.
                        if (!exportedComponent.m_deleteAfterExport)
                        {
                            runtimeComponent = serializeContext.CloneObject(runtimeComponent);
                        }

                        // Synchronize to source component Id, and add to the export entity.
                        runtimeComponent->SetId(asEditorComponent->GetId());
                        if (!exportEntity->AddComponent(runtimeComponent))
                        {
                            return AZ::Failure(AZStd::string::format("Component \"%s\" could not be added to Entity \"%s\" [0x%llu].",
                                runtimeComponent->RTTI_GetTypeName(),
                                exportEntity->GetName().c_str(),
                                static_cast<AZ::u64>(exportEntity->GetId())));
                        }
                    }
                    else // BEGIN BuildGameEntity compatibility path for editor components not using the newer RuntimeExportCallback functionality.
                    {
                        const size_t oldComponentCount = exportEntity->GetComponents().size();
                        asEditorComponent->BuildGameEntity(exportEntity);
                        if (exportEntity->GetComponents().size() > oldComponentCount)
                        {
                            AZ::Component* exportComponent = exportEntity->GetComponents().back();

                            if (asEditorComponent->GetId() == AZ::InvalidComponentId)
                            {
                                return AZ::Failure(AZStd::string::format("Entity \"%s\" [0x%llu], component \"%s\" doesn't have a valid component Id.",
                                    sourceEntity->GetName().c_str(), 
                                    static_cast<AZ::u64>(sourceEntity->GetId()),
                                    asEditorComponent->RTTI_GetType().ToString<AZStd::string>().c_str()));
                            }

                            exportComponent->SetId(asEditorComponent->GetId());
                        }
                    } // END BuildGameEntity compatibility path for editor components not using the newer RuntimeExportCallback functionality.
                }
                else // Source component is not an editor component. Simply clone and add.
                {
                    AZ::Component* exportComponent = serializeContext.CloneObject(component);
                    exportEntity->AddComponent(exportComponent);
                }
            }

            // Pre-sort prior to exporting so it isn't required at instantiation time.
            const AZ::Entity::DependencySortResult sortResult = exportEntity->EvaluateDependencies();
            if (AZ::Entity::DependencySortResult::Success != sortResult)
            {
                const char* sortResultError = "";
                switch (sortResult)
                {
                case AZ::Entity::DependencySortResult::HasCyclicDependency:
                    sortResultError = "Cyclic dependency found";
                    break;
                case AZ::Entity::DependencySortResult::MissingRequiredService:
                    sortResultError = "Required services missing";
                    break;
                }

                return AZ::Failure(AZStd::string::format("Entity \"%s\" [0x%llu] dependency evaluation failed: %s.",
                    exportEntity->GetName().c_str(), 
                    static_cast<AZ::u64>(exportEntity->GetId()),
                    sortResultError));
            }

            exportSliceData->AddEntity(exportEntity);
        }

        AZ::SliceComponent::EntityList exportEntities;
        exportSliceData->GetEntities(exportEntities);

        if (exportEntities.size() != sourceEntities.size())
        {
            return AZ::Failure(AZStd::string("Entity export list size must match that of the import list."));
        }

        // Notify user callback, and then strip out any editor-only entities.
        // This operation can generate a failure if a the callback failed validation.
        if (!editorOnlyEntities.empty())
        {
            EditorOnlyEntityHandler::Result result = Internal::AdjustForEditorOnlyEntities(exportSliceData, editorOnlyEntities, serializeContext, editorOnlyEntityHandler);
            if (!result)
            {
                return AZ::Failure(result.TakeError());
            }
        }

        // Call validation callbacks on final runtime components.
        AZ::ImmutableEntityVector immutableExportEntities;
        immutableExportEntities.reserve(exportEntities.size());
        for (AZ::Entity* entity : exportEntities)
        {
            immutableExportEntities.push_back(entity);
        }

        for (AZ::Entity* exportEntity : exportEntities)
        {
            const AZ::Entity::ComponentArrayType& gameComponents = exportEntity->GetComponents();
            for (AZ::Component* component : gameComponents)
            {
                AZ::ComponentValidationResult result = component->ValidateComponentRequirements(immutableExportEntities);
                if (!result.IsSuccess())
                {
                    // Try to cast to GenericComponentWrapper, and if we can, get the internal template.
                    const char* componentName = component->RTTI_GetTypeName();
                    return AZ::Failure(AZStd::string::format("[Entity] \"%s\" [Entity Id] 0x%llu, [Exported Component] \"%s\" could not pass validation for [Slice] \"%s\" [Error] %s",
                        exportEntity->GetName().c_str(), 
                        exportEntity->GetId(),
                        componentName,
                        sourceSliceAsset.GetHint().c_str(),
                        result.GetError().c_str()));
                }
            }
        }
    
        return AZ::Success(exportSliceAsset);
    }
    
    EditorOnlyEntityHandler::Result WorldEditorOnlyEntityHandler::HandleEditorOnlyEntities(const AzToolsFramework::EntityList& entities, const AzToolsFramework::EntityIdSet& editorOnlyEntityIds, AZ::SerializeContext& serializeContext)
    {
        FixTransformRelationships(entities, editorOnlyEntityIds);

        return ValidateReferences(entities, editorOnlyEntityIds, serializeContext);
    }

    void WorldEditorOnlyEntityHandler::FixTransformRelationships(const AzToolsFramework::EntityList& entities, const AzToolsFramework::EntityIdSet& editorOnlyEntityIds)
    {
        AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZ::Entity*>> parentToChildren;

        // Build a map of entity Ids to their parent Ids, for faster lookup during processing.
        for (AZ::Entity* entity : entities)
        {
            auto* transformComponent = entity->FindComponent<AzFramework::TransformComponent>();
            if (transformComponent)
            {
                const AZ::EntityId parentId = transformComponent->GetParentId();
                if (parentId.IsValid())
                {
                    parentToChildren[parentId].push_back(entity);
                }
            }
        }

        // Identify any editor-only entities. If we encounter one, adjust transform relationships
        // for all of its children to ensure relative transforms are maintained and respected at
        // runtime.
        // This works regardless of entity ordering in the slice because we add reassigned children to 
        // parentToChildren cache during the operation.
        for (AZ::Entity* entity : entities)
        {
            if (editorOnlyEntityIds.end() == editorOnlyEntityIds.find(entity->GetId()))
            {
                continue; // This is not an editor-only entity.
            }

            auto* transformComponent = entity->FindComponent<AzFramework::TransformComponent>();
            if (transformComponent)
            {
                const AZ::Transform& parentLocalTm = transformComponent->GetLocalTM();

                // Identify all transform children and adjust them to be children of the removed entity's parent.
                for (AZ::Entity* childEntity : parentToChildren[entity->GetId()])
                {
                    auto* childTransformComponent = childEntity->FindComponent<AzFramework::TransformComponent>();

                    if (childTransformComponent && childTransformComponent->GetParentId() == entity->GetId())
                    {
                        const AZ::Transform localTm = childTransformComponent->GetLocalTM();
                        childTransformComponent->SetParent(transformComponent->GetParentId());
                        childTransformComponent->SetLocalTM(parentLocalTm * localTm);

                        parentToChildren[transformComponent->GetParentId()].push_back(childEntity);
                    }
                }
            }
        }
    }

    EditorOnlyEntityHandler::Result WorldEditorOnlyEntityHandler::ValidateReferences(const AzToolsFramework::EntityList& entities, const AzToolsFramework::EntityIdSet& editorOnlyEntityIds, AZ::SerializeContext& serializeContext)
    {
        EditorOnlyEntityHandler::Result result = AZ::Success();

        // Inspect all runtime entities via the serialize context and identify any references to editor-only entity Ids.
        for (AZ::Entity* runtimeEntity : entities)
        {
            if (editorOnlyEntityIds.end() != editorOnlyEntityIds.find(runtimeEntity->GetId()))
            {
                continue; // This is not a runtime entity, so no need to validate its references as it's going away.
            }

            AZ::EntityUtils::EnumerateEntityIds<AZ::Entity>(runtimeEntity, 
                [&editorOnlyEntityIds, &result, runtimeEntity](const AZ::EntityId& id, bool /*isEntityId*/, const AZ::SerializeContext::ClassElement* /*elementData*/)
                {
                    if (editorOnlyEntityIds.end() != editorOnlyEntityIds.find(id))
                    {
                        result = AZ::Failure(AZStd::string::format("A runtime entity (%s) contains references to an entity marked as editor-only.", runtimeEntity->GetName().c_str()));
                        return false;
                    }

                    return true;
                }
                , &serializeContext);

            if (!result)
            {
                break;
            }
        }

        return result;
    }

} // AzToolsFramework
