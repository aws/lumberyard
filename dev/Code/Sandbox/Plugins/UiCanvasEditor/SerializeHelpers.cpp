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

#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/std/parallel/thread.h>


namespace SerializeHelpers
{
    bool s_initializedReflection = false;

    //! Simple helper class for serializing a vector of entities, their child entities
    //! and their slice instance information. This is only serialized for the undo system
    //! or the clipboard so it does not require version conversion.
    //! m_entities is the set of entities that were chosen to be serialized (e.g. by a copy
    //! command), m_childEntities are all the descendants of the entities in m_entities.
    class SerializedElementContainer
    {
    public:
        virtual ~SerializedElementContainer() { }
        AZ_CLASS_ALLOCATOR(SerializedElementContainer, AZ::SystemAllocator, 0);
        AZ_RTTI(SerializedElementContainer, "{4A12708F-7EC5-4F56-827A-6E67C3C49B3D}");
        AZStd::vector<AZ::Entity*> m_entities;
        AZStd::vector<AZ::Entity*> m_childEntities;
        EntityRestoreVec m_entityRestoreInfos;
        EntityRestoreVec m_childEntityRestoreInfos;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void InitializeReflection()
    {
        // Reflect the SerializedElementContainer on first use.
        if (!s_initializedReflection)
        {
            AZ::SerializeContext* context = nullptr;
            EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
            AZ_Assert(context, "No serialize context");

            context->Class<SerializedElementContainer>()
                ->Version(1)
                ->Field("Entities", &SerializedElementContainer::m_entities)
                ->Field("ChildEntities", &SerializedElementContainer::m_childEntities)
                ->Field("RestoreInfos", &SerializedElementContainer::m_entityRestoreInfos)
                ->Field("ChildRestoreInfos", &SerializedElementContainer::m_childEntityRestoreInfos);

            s_initializedReflection = true;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void RestoreSerializedElements(
        AZ::EntityId canvasEntityId,
        AZ::Entity*  parent,
        AZ::Entity*  insertBefore,
        UiEditorEntityContext* entityContext,
        const AZStd::string& xml,
        bool isCopyOperation,
        LyShine::EntityArray* cumulativeListOfCreatedEntities)
    {
        LyShine::EntityArray listOfNewlyCreatedTopLevelElements;
        LyShine::EntityArray listOfAllCreatedEntities;
        EntityRestoreVec entityRestoreInfos;

        LoadElementsFromXmlString(
            canvasEntityId,
            xml.c_str(),
            isCopyOperation,
            parent,
            insertBefore,
            listOfNewlyCreatedTopLevelElements,
            listOfAllCreatedEntities,
            entityRestoreInfos);

        if (listOfNewlyCreatedTopLevelElements.empty())
        {
            // This happens when the serialization version numbers DON'T match.
            QMessageBox(QMessageBox::Critical,
                "Error",
                QString("Failed to restore elements. The clipboard serialization format is incompatible."),
                QMessageBox::Ok, QApplication::activeWindow()).exec();

            // Nothing more to do.
            return;
        }

        // This is for error handling only. In the case of an error RestoreSliceEntity will delete the
        // entity. We need to know when this has happened. So we record all the entityIds here and check
        // them afterwards.
        AzToolsFramework::EntityIdList idsOfNewlyCreatedTopLevelElements;
        for (auto entity : listOfNewlyCreatedTopLevelElements)
        {
            idsOfNewlyCreatedTopLevelElements.push_back(entity->GetId());
        }

        // Now we need to restore the slice info for all the created elements
        for (int i=0; i < listOfAllCreatedEntities.size(); ++i)
        {
            AZ::Entity* entity = listOfAllCreatedEntities[i];

            AZ::SliceComponent::EntityRestoreInfo& sliceRestoreInfo = entityRestoreInfos[i];

            if (sliceRestoreInfo)
            {
                if (isCopyOperation)
                {
                    // if a copy we can't use the instanceId of the instance that was copied from so generate
                    // a new 
                    sliceRestoreInfo.m_instanceId = AZ::SliceComponent::SliceInstanceId::CreateRandom();
                }

                EBUS_EVENT(UiEditorEntityContextRequestBus, RestoreSliceEntity, entity, sliceRestoreInfo);
            }
            else
            {
                entityContext->AddUiEntity(entity);
            }
        }

        // If we are restoring slice entities and any of the entities are the first to be using that slice
        // then we have to wait for that slice to be reloaded. We need to wait because we can't create hierarchy
        // items for entities before they are recreated. We have tried trying to solve this by deferring the creation
        // of the hierarchy items on a queue but that gets really complicated because this function is called
        // in several situations. It also seems problematic to return control to the user - they could add or
        // delete more items while we are waiting for the assets to load.
        if (AZ::Data::AssetManager::IsReady())
        {
            bool areRequestsPending = false;
            EBUS_EVENT_RESULT(areRequestsPending, UiEditorEntityContextRequestBus, HasPendingRequests);
            while (areRequestsPending)
            {
                AZ::Data::AssetManager::Instance().DispatchEvents();
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(50));
                EBUS_EVENT_RESULT(areRequestsPending, UiEditorEntityContextRequestBus, HasPendingRequests);
            }
        }

        // Because RestoreSliceEntity can delete the entity we have some recovery code here that will
        // create a new list of top level entities excluding any that have been removed.
        // An error should already have been reported in this case so we don't report it again.
        LyShine::EntityArray validatedListOfNewlyCreatedTopLevelElements;
        for (auto entityId : idsOfNewlyCreatedTopLevelElements)
        {
            AZ::Entity* entity = nullptr;
            EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);

            // Only add it to the validated list if the entity still exists
            if (entity)
            {
                validatedListOfNewlyCreatedTopLevelElements.push_back(entity);
            }
        }

        // Fixup the created entities, we do this before adding the top level element to the parent so that
        // MakeUniqueChileName works correctly
        EBUS_EVENT_ID(canvasEntityId,
            UiCanvasBus,
            FixupCreatedEntities,
            validatedListOfNewlyCreatedTopLevelElements,
            isCopyOperation,
            parent);

        // Now add the top-level created elements as children of the parent
        for (auto entity : validatedListOfNewlyCreatedTopLevelElements)
        {
            // add this new entity as a child of the parent (insertionPoint or root)
            EBUS_EVENT_ID(canvasEntityId,
                UiCanvasBus,
                AddElement,
                entity,
                parent,
                insertBefore);
        }

        // if a list of entities was passed then add all the entities that we added 
        // to the list
        if (cumulativeListOfCreatedEntities)
        {
            cumulativeListOfCreatedEntities->push_back(validatedListOfNewlyCreatedTopLevelElements);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string SaveElementsToXmlString(const LyShine::EntityArray& elements, AZ::SliceComponent* rootSlice, AZStd::unordered_set<AZ::Data::AssetId>& referencedSliceAssets)
    {
        InitializeReflection();

        // The easiest way to write multiple elements to a stream is to create class that contains them
        // that has an allocator. SerializedElementContainer exists for this purpose.
        // It saves/loads two lists. One is a list of top-level elements, the second is a list of all of
        // the children of those elements.
        SerializedElementContainer entitiesToSerialize;
        for (auto element : elements)
        {
            entitiesToSerialize.m_entities.push_back(element);

            // add the slice restore info for this top level element
            AZ::SliceComponent::EntityRestoreInfo sliceRestoreInfo;
            rootSlice->GetEntityRestoreInfo(element->GetId(), sliceRestoreInfo);
            entitiesToSerialize.m_entityRestoreInfos.push_back(sliceRestoreInfo);

            referencedSliceAssets.insert(sliceRestoreInfo.m_assetId);

            LyShine::EntityArray childElements;
            EBUS_EVENT_ID(element->GetId(), UiElementBus, FindDescendantElements,
                [](const AZ::Entity* entity) { return true; },
                childElements);

            for (auto child : childElements)
            {
                entitiesToSerialize.m_childEntities.push_back(child);

                // add the slice restore info for this child element
                rootSlice->GetEntityRestoreInfo(child->GetId(), sliceRestoreInfo);
                entitiesToSerialize.m_childEntityRestoreInfos.push_back(sliceRestoreInfo);

                referencedSliceAssets.insert(sliceRestoreInfo.m_assetId);
            }
        }

        AZStd::string charBuffer;
        AZ::IO::ByteContainerStream<AZStd::string> charStream(&charBuffer);
        bool success = AZ::Utils::SaveObjectToStream(charStream, AZ::ObjectStream::ST_XML, &entitiesToSerialize);
        AZ_Assert(success, "Failed to serialize elements to XML");

        return charBuffer;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LoadElementsFromXmlString(
        AZ::EntityId canvasEntityId,
        const AZStd::string& string,
        bool makeNewIDs,
        AZ::Entity* insertionPoint,
        AZ::Entity* insertBefore,
        LyShine::EntityArray& listOfCreatedTopLevelElements,
        LyShine::EntityArray& listOfAllCreatedElements,
        EntityRestoreVec& entityRestoreInfos)
    {
        InitializeReflection();

        AZ::IO::ByteContainerStream<const AZStd::string> charStream(&string);
        SerializedElementContainer* unserializedEntities =
            AZ::Utils::LoadObjectFromStream<SerializedElementContainer>(charStream);

        // If we want new IDs then generate them and fixup all references within the list of entities
        if (makeNewIDs)
        {
            AZ::SerializeContext* context = nullptr;
            EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
            AZ_Assert(context, "No serialization context found");

            AZ::SliceComponent::EntityIdToEntityIdMap entityIdMap;
            AZ::EntityUtils::GenerateNewIdsAndFixRefs(unserializedEntities, entityIdMap, context);
        }

        // copy unserializedEntities into the return output list of top-level entities
        for (auto newEntity : unserializedEntities->m_entities)
        {
            listOfCreatedTopLevelElements.push_back(newEntity);
        }

        // we also return a list of all of the created entities (not just top level ones)
        listOfAllCreatedElements.insert(listOfAllCreatedElements.end(),
            unserializedEntities->m_entities.begin(), unserializedEntities->m_entities.end());
        listOfAllCreatedElements.insert(listOfAllCreatedElements.end(),
            unserializedEntities->m_childEntities.begin(), unserializedEntities->m_childEntities.end());

        // return a list of the EntityRestoreInfos in the same order
        entityRestoreInfos.insert(entityRestoreInfos.end(),
            unserializedEntities->m_entityRestoreInfos.begin(), unserializedEntities->m_entityRestoreInfos.end());
        entityRestoreInfos.insert(entityRestoreInfos.end(),
            unserializedEntities->m_childEntityRestoreInfos.begin(), unserializedEntities->m_childEntityRestoreInfos.end()); 
    }

}   // namespace EntityHelpers
