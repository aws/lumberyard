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

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/sort.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/UI/Slice/SlicePushWidget.hxx>

#include <QtWidgets/QWidget>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QErrorMessage>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QThread>

#include <LyShine/Bus/UiElementBus.h>

#include "UiSlicePushWidget.h"
#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

//////////////////////////////////////////////////////////////////////////
UiSliceManager::UiSliceManager(AzFramework::EntityContextId entityContextId)
    : m_entityContextId(entityContextId)
{
    UiEditorEntityContextNotificationBus::Handler::BusConnect();
}

//////////////////////////////////////////////////////////////////////////
UiSliceManager::~UiSliceManager()
{
    UiEditorEntityContextNotificationBus::Handler::BusDisconnect();
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::OnSliceInstantiationFailed(const AZ::Data::AssetId&, const AzFramework::SliceInstantiationTicket&)
{
    QMessageBox::warning(QApplication::activeWindow(),
        QStringLiteral("Cannot Instantiate UI Slice"),
        QString("Slice cannot be instantiated. Check that it is a slice containing UI elements."),
        QMessageBox::Ok);
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::InstantiateSlice(const AZ::Data::AssetId& assetId, AZ::Vector2 viewportPosition)
{
    AZ::Data::Asset<AZ::SliceAsset> sliceAsset;
    sliceAsset.Create(assetId, true);

    EBUS_EVENT(UiEditorEntityContextRequestBus, InstantiateEditorSlice, sliceAsset, viewportPosition);
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::InstantiateSliceUsingBrowser(HierarchyWidget* hierarchy, AZ::Vector2 viewportPosition)
{
    AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection("Slice");
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (!selection.IsValid())
    {
        return;
    }

    auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
    AZ_Assert(product, "Selection is invalid.");

    InstantiateSlice(product->GetAssetId(), viewportPosition);
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::MakeSliceFromSelectedItems(HierarchyWidget* hierarchy, bool inheritSlices)
{
    QTreeWidgetItemRawPtrQList selectedItems(hierarchy->selectedItems());

    HierarchyItemRawPtrList items = SelectionHelpers::GetSelectedHierarchyItems(hierarchy,
            selectedItems);

    AzToolsFramework::EntityIdList selectedEntities;
    for (auto item : items)
    {
        selectedEntities.push_back(item->GetEntityId());
    }

    MakeSliceFromEntities(selectedEntities, inheritSlices);
}

bool UiSliceManager::IsRootEntity(const AZ::Entity& entity) const
{
    AZ::Entity* parent = nullptr;
    EBUS_EVENT_ID_RESULT(parent, entity.GetId(), UiElementBus, GetParent);
    return (parent == nullptr);
}

AZ::SliceComponent* UiSliceManager::GetRootSlice() const
{
    AZ::SliceComponent* rootSlice = nullptr;
    EBUS_EVENT_ID_RESULT(rootSlice, m_entityContextId, UiEditorEntityContextRequestBus, GetUiRootSlice);
    return rootSlice;
}

//////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::MakeSliceFromEntities(AzToolsFramework::EntityIdList& entities, bool inheritSlices)
{
    // expand the list of entities to include all child entities
    for (int i = 0; i < entities.size(); ++i)
    {
        LyShine::EntityArray children;
        EBUS_EVENT_ID_RESULT(children, entities[i], UiElementBus, GetChildElements);
        for (auto child : children)
        {
            AZ::EntityId childId = child->GetId();
            if (AZStd::find(entities.begin(), entities.end(), childId) == entities.end())
            {
                entities.push_back(childId);
            }
        }
    }

    const AZStd::string slicesAssetsPath = "@devassets@/UI/Slices";

    if (!gEnv->pFileIO->Exists(slicesAssetsPath.c_str()))
    {
        gEnv->pFileIO->CreatePath(slicesAssetsPath.c_str());
    }

    char path[AZ_MAX_PATH_LEN] = { 0 };
    gEnv->pFileIO->ResolvePath(slicesAssetsPath.c_str(), path, AZ_MAX_PATH_LEN);

    MakeNewSlice(entities, path, inheritSlices);
}

//////////////////////////////////////////////////////////////////////////
bool UiSliceManager::MakeNewSlice(
    const AzToolsFramework::EntityIdList& entities, 
    const char* targetDirectory, 
    bool inheritSlices, 
    AZ::SerializeContext* serializeContext)
{
    // Get the root slice of the canvas
    AZ::SliceComponent* rootSlice = GetRootSlice();
    if (!rootSlice)
    {
        AZ_Error("Slices", false, "Failed to retrieve root slice.");
        return false;
    }

    AzToolsFramework::EntityIdList orderedEntityList;
    AZ::Entity* insertBefore = nullptr;
    AZ::Entity* commonParent = ValidateSingleRoot(entities, orderedEntityList, insertBefore);
    if (!commonParent)
    {
        QMessageBox::warning(QApplication::activeWindow(),
            QStringLiteral("Cannot Create UI Slice"),
            QString("The slice cannot be created because there is no single element in the selection that is parent "
                    "to all other elements in the selection."
                    "Please make sure your slice contains only one root entity.\r\n\r\n"
                    "You may want to create a new entity, and assign it as the parent of your existing root entities."),
            QMessageBox::Ok);
        return false;
    }

    // Save a reference to our currently active window since it will be
    // temporarily null after the QFileDialog closes, which we need to
    // be able to parent our message dialogs properly
    QWidget* activeWindow = QApplication::activeWindow();
    const QString saveAs = QFileDialog::getSaveFileName(nullptr, QString("Save As..."), targetDirectory, QString("Slices (*.slice)"));
    if (saveAs.isEmpty())
    {
        return false;
    }

    const AZStd::string targetPath = saveAs.toUtf8().constData();

    if (!serializeContext)
    {
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to retrieve application serialize context.");
    }

    // Expand the list to include children of the selected entities
    AzToolsFramework::EntityIdSet selectedHierarchyEntities = GatherEntitiesAndAllDescendents(entities);

    // Check for references that don't fall within the selected entity set.
    // Give the user the option to pull in all referenced entities, or to
    // stick with the current selection.
    AZStd::unordered_set<AZ::EntityId> allReferencedEntities = selectedHierarchyEntities;
    GatherAllReferencedEntities(allReferencedEntities, *serializeContext);
            
    // NOTE: that AZStd::unordered_set equality operator only returns true if they are in the same order
    // (which appears to deviate from the standard). So we have to do the comparison ourselves.
    bool referencedEntitiesNotInSelection = (allReferencedEntities.size() > selectedHierarchyEntities.size());
    if (!referencedEntitiesNotInSelection)
    {
        for (const AZ::EntityId& id : allReferencedEntities)
        {
            if (selectedHierarchyEntities.find(id) == selectedHierarchyEntities.end())
            {
                referencedEntitiesNotInSelection = true;
                break;
            }
        }
    }

    if (referencedEntitiesNotInSelection)
    {
        const AZStd::string message = AZStd::string::format(
                "Some of the selected entities reference entities not contained in the selection and its children.\r\n"
                "UI slices cannot contain references to outside of the slice.\r\n");

        QMessageBox::warning(activeWindow, QStringLiteral("Create Slice"),
                QString(message.c_str()), QMessageBox::Ok);

        return false;
    }

    // If the slice already exists, we instead want to *push* the entities to the existing
    // asset, as opposed to creating a new one.
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    if (fileIO && fileIO->Exists(targetPath.c_str()))
    {
        const AZStd::string message = AZStd::string::format(
                "You are attempting to overwrite an existing slice: \"%s\".\r\n\r\n"
                "This will damage instances or cascades of this slice. \r\n\r\n"
                "Instead, either push entities/fields to the slice, or save to a different location.",
                targetPath.c_str());

        QMessageBox::warning(activeWindow, QStringLiteral("Unable to Overwrite Slice"),
            QString(message.c_str()), QMessageBox::Ok, QMessageBox::Ok);

        return false;
    }

    // Create a new slice.
    AZ::Entity sliceEntity;
    AZ::SliceComponent* newSlice = sliceEntity.CreateComponent<AZ::SliceComponent>();

    AZStd::vector<AZ::Entity*> reclaimFromSlice;
    AZStd::vector<AZ::SliceComponent::SliceInstanceAddress> sliceInstances;

    AZStd::unordered_map<AZ::EntityId, AZ::EntityId> liveEntityToSliceAssetIdMap;
    AZ::SliceComponent::EntityAncestorList ancestorTemp;

    // Add all included entities to the new slice, either by inheriting their source slices
    // or as loose entities.
    for (const AZ::EntityId& id : orderedEntityList)
    {
        AZ::Entity* entity = nullptr;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, id);
        if (entity)
        {
            AZ::SliceComponent::SliceInstanceAddress sliceAddress(nullptr, nullptr);

            if (inheritSlices)
            {
                EBUS_EVENT_ID_RESULT(sliceAddress, entity->GetId(), AzFramework::EntityIdContextQueryBus, GetOwningSlice);
            }

            if (sliceAddress.first)
            {
                ancestorTemp.clear();
                sliceAddress.first->GetInstanceEntityAncestry(id, ancestorTemp, 1);
                AZ_Error("Slices", !ancestorTemp.empty(), "Failed to retrieve ancestry for slice-owned entity during cascaded slice creation.");
                if (!ancestorTemp.empty())
                {
                    liveEntityToSliceAssetIdMap[id] = ancestorTemp.front().m_entity->GetId();
                }

                // This entity already belongs to a slice instance, so inherit that instance (the whole thing for now).
                if (sliceInstances.end() == AZStd::find(sliceInstances.begin(), sliceInstances.end(), sliceAddress))
                {
                    sliceInstances.push_back(sliceAddress);
                }
            }
            else
            {
                // Otherwise add loose.
                liveEntityToSliceAssetIdMap[entity->GetId()] = entity->GetId();
                newSlice->AddEntity(entity);
                reclaimFromSlice.push_back(entity);
            }
        }
    }

    for (AZ::SliceComponent::SliceInstanceAddress& info : sliceInstances)
    {
        info = newSlice->AddSliceInstance(info.first, info.second);
    }

    // Clone the entities prior to postprocessing/writing, since we need to manipulate target transforms.
    AZ::Entity* finalSliceEntity = serializeContext->CloneObject(&sliceEntity);
    AZ::Data::Asset<AZ::SliceAsset> finalAsset = AZ::Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
    finalAsset.Get()->SetData(finalSliceEntity, finalSliceEntity->FindComponent<AZ::SliceComponent>());

    // Ensure single-root rule is enforced.
    bool result = RootEntityTransforms(finalAsset, liveEntityToSliceAssetIdMap);
            
    if (result)
    {
        // Save the new asset.
        result = SaveSlice(finalAsset, targetPath.c_str(), serializeContext);
        if (result)
        {
            // Once the asset is processed and ready, we can replace the source entities with an instance of the new slice.
            EBUS_EVENT_ID(m_entityContextId, UiEditorEntityContextRequestBus, QueueSliceReplacement, targetPath.c_str(),
                liveEntityToSliceAssetIdMap, allReferencedEntities, commonParent, insertBefore);

            // Bump the slice asset up in the asset processor's queue.
            AzFramework::AssetSystemRequestBus::Broadcast(
                &AzFramework::AssetSystem::AssetSystemRequests::GetAssetStatus, targetPath.c_str());
        }
        else
        {
            QMessageBox::warning(activeWindow, QStringLiteral("Cannot Save Slice"),
                QString("Failed to write slice because asset file \"%1\" could not be written to. Slice creation canceled.").arg(targetPath.c_str()), 
                QMessageBox::Ok);
        }
    }
    else
    {
        QMessageBox::warning(activeWindow, QStringLiteral("Cannot Save Slice"),
            QString("Failed to write slice \"%1\" because transforms could not be rooted.").arg(targetPath.c_str()), 
            QMessageBox::Ok);
    }

    // Reclaim entities and slices from the output slice. We borrowed them for serialization,
    // but they belong to the editor entity context.
    for (AZ::Entity* entity : reclaimFromSlice)
    {
        newSlice->RemoveEntity(entity, false);
    }

    if (rootSlice)
    {
        for (const AZ::SliceComponent::SliceInstanceAddress& info : sliceInstances)
        {
            rootSlice->AddSliceInstance(info.first, info.second);
        }
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::GetTopLevelEntities(const AZ::SliceComponent::EntityList& entities, AZ::SliceComponent::EntityList& topLevelEntities)
{
    AZStd::unordered_set<AZ::Entity*> allEntities;
    allEntities.insert(entities.begin(), entities.end());

    for (auto entity : entities)
    {
        // if this entities parent is not in the set then it is a top-level
        AZ::Entity* parentElement = nullptr;
        EBUS_EVENT_ID_RESULT(parentElement, entity->GetId(), UiElementBus, GetParent);

        if (parentElement)
        {
            if (allEntities.count(parentElement) == 0)
            {
                topLevelEntities.push_back(entity);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// This is similar to ToolsApplicationRequests::GatherEntitiesAndAllDescendents
// except that function assumes that the entities are supporting the the AZ::TransformBus
// for hierarchy. This UI-specific version uses the UiElementBus
AzToolsFramework::EntityIdSet UiSliceManager::GatherEntitiesAndAllDescendents(const AzToolsFramework::EntityIdList& inputEntities)
{
    AzToolsFramework::EntityIdSet output;
    AzToolsFramework::EntityIdList tempList;
    for (const AZ::EntityId& id : inputEntities)
    {
        output.insert(id);

        LyShine::EntityArray descendants;
        EBUS_EVENT_ID(id, UiElementBus, FindDescendantElements, [](const AZ::Entity*){ return true; }, descendants);

        for (auto descendant : descendants)
        {
            output.insert(descendant->GetId());
        }
    }

    return output;
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::GatherAllReferencedEntities(AZStd::unordered_set<AZ::EntityId>& entitiesWithReferences,
                                    AZ::SerializeContext& serializeContext)
{
    AZStd::vector<AZ::EntityId> floodQueue;
    floodQueue.reserve(entitiesWithReferences.size());

    auto visitChild = [&floodQueue, &entitiesWithReferences](const AZ::EntityId& childId) -> void
    {
        if (entitiesWithReferences.insert(childId).second)
        {
            floodQueue.push_back(childId);
        }
    };

    // Seed with all provided entity Ids.
    // UI_CHANGE: Do not use the TransformComponent.
    // We do not need to use the UiElementComponent to add the children since those are outgoing
    // references, not incoming as they are with the TransformComponent
    for (const AZ::EntityId& entityId : entitiesWithReferences)
    {
        floodQueue.push_back(entityId);
    }

    // Flood-fill via outgoing entity references and gather all unique visited entities.
    while (!floodQueue.empty())
    {
        const AZ::EntityId id = floodQueue.back();
        floodQueue.pop_back();

        AZ::Entity* entity = nullptr;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, id);

        if (entity)
        {
            AZStd::vector<const AZ::SerializeContext::ClassData*> parentStack;
            parentStack.reserve(30);
            auto beginCB = [&](void* ptr, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* elementData) -> bool
            {
                parentStack.push_back(classData);
                    if (elementData && elementData->m_editData)
                    {
                        AZ::Edit::Attribute* attribute = elementData->m_editData->FindAttribute(AZ::Edit::Attributes::SliceFlags);
                        if (attribute)
                        {
                            AZ::u32 flags = 0;
                            AzToolsFramework::PropertyAttributeReader reader(nullptr, attribute);
                            reader.Read<AZ::u32>(flags);
                            if (0 != (flags & AZ::Edit::SliceFlags::DontGatherReference))
                            {
                            return false;
                            }
                        }
                    }

                if (classData->m_typeId == AZ::SerializeTypeInfo<AZ::EntityId>::GetUuid())
                {
                    if (!parentStack.empty() && parentStack.back()->m_typeId == AZ::SerializeTypeInfo<AZ::Entity>::GetUuid())
                    {
                        // Ignore the entity's actual Id field. We're only looking for references.
                    }
                    else
                    {
                        AZ::EntityId* entityIdPtr = (elementData->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER) ?
                            *reinterpret_cast<AZ::EntityId**>(ptr) : reinterpret_cast<AZ::EntityId*>(ptr);
                        if (entityIdPtr)
                        {
                            const AZ::EntityId id = *entityIdPtr;
                            if (id.IsValid())
                            {
                    visitChild(id);
                }
                        }
                    }
                }

                // Keep recursing.
                return true;
            };

            auto endCB = [&]() -> bool
            {
                parentStack.pop_back();
                return true;
            };

            serializeContext.EnumerateInstanceConst(
                entity,
                azrtti_typeid<AZ::Entity>(),
                beginCB,
                endCB,
                AZ::SerializeContext::ENUM_ACCESS_FOR_READ,
                nullptr,
                nullptr,
                nullptr
            );
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::PushEntitiesModal(const AzToolsFramework::EntityIdList& entities, 
                        AZ::SerializeContext* serializeContext)
{
    AzToolsFramework::EntityIdList adjustedEntities(entities);

    // First we check that the selection is valid for a push
    if (!ValidatePushSelection(adjustedEntities, serializeContext))
    {
        return;
    }

    using UiCanvasEditor::UiSlicePushWidget;

    QDialog* dialog = new QDialog();
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(5, 5, 5, 5);
    UiSlicePushWidget* widget = new UiSlicePushWidget(this, adjustedEntities, serializeContext);
    mainLayout->addWidget(widget);
    dialog->setWindowTitle(widget->tr("Push to Slice(es)"));
    dialog->setMinimumSize(QSize(600, 200));
    dialog->resize(QSize(1000, 600));
    dialog->setLayout(mainLayout);

    QWidget::connect(widget, &UiSlicePushWidget::OnFinished, dialog,
        [dialog] ()
        {
            dialog->accept();
        }
    );

    QWidget::connect(widget, &UiSlicePushWidget::OnCanceled, dialog,
        [dialog] ()
        {
            dialog->reject();
        }
    );

    dialog->exec();
    delete dialog;
}
        
//////////////////////////////////////////////////////////////////////////
void UiSliceManager::DetachSliceEntities(const AzToolsFramework::EntityIdList& entities)
{
    if (!entities.empty())
    {
        QString title;
        QString body;
        if (entities.size() == 1)
        {
            title = QObject::tr("Detach Slice Entity");
            body = QObject::tr("A detached entity will no longer receive pushes from its slice. The entity will be converted into a non-slice entity. This action cannot be undone.\n\n"
                "Are you sure you want to detach the selected entity?");
        }
        else
        {
            title = QObject::tr("Detach Slice Entities");
            body = QObject::tr("Detached entities no longer receive pushes from their slices. The entities will be converted into non-slice entities. This action cannot be undone.\n\n"
                "Are you sure you want to detach the selected entities and their descendants?");
        }

        if (ConfirmDialog_Detach(title, body))
        {
            EBUS_EVENT(UiEditorEntityContextRequestBus, DetachSliceEntities, entities);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::DetachSliceInstances(const AzToolsFramework::EntityIdList& entities)
{
    if (!entities.empty())
    {
        // Get all slice instances for given entities
        AZStd::vector<AZ::SliceComponent::SliceInstanceAddress> sliceInstances;
        for (const AZ::EntityId& entityId : entities)
        {
            AZ::SliceComponent::SliceInstanceAddress sliceAddress(nullptr, nullptr);
            EBUS_EVENT_ID_RESULT(sliceAddress, entityId, AzFramework::EntityIdContextQueryBus, GetOwningSlice);

            if (sliceAddress.first)
            {
                if (sliceInstances.end() == AZStd::find(sliceInstances.begin(), sliceInstances.end(), sliceAddress))
                {
                    sliceInstances.push_back(sliceAddress);
                }
            }
        }

        QString title;
        QString body;
        if (sliceInstances.size() == 1)
        {
            title = QObject::tr("Detach Slice Instance");
            body = QObject::tr("A detached instance will no longer receive pushes from its slice. All entities in the slice instance will be converted into non-slice entities. This action cannot be undone.\n\n"
                "Are you sure you want to detach the selected instance?");
        }
        else
        {
            title = QObject::tr("Detach Slice Instances");
            body = QObject::tr("Detached instances no longer receive pushes from their slices. All entities in the slice instances will be converted into non-slice entities. This action cannot be undone.\n\n"
                "Are you sure you want to detach the selected instances?");
        }

        if (ConfirmDialog_Detach(title, body))
        {
            // Get all instantiated entities for the slice instances
            AzToolsFramework::EntityIdList entitiesToDetach = entities;
            for (const AZ::SliceComponent::SliceInstanceAddress& sliceInstance : sliceInstances)
            {
                const AZ::SliceComponent::InstantiatedContainer* instantiated = sliceInstance.second->GetInstantiated();
                if (instantiated)
                {
                    for (AZ::Entity* entityInSlice : instantiated->m_entities)
                    {
                        entitiesToDetach.push_back(entityInSlice->GetId());
                    }
                }
            }

            // Detach the entities
            EBUS_EVENT(UiEditorEntityContextRequestBus, DetachSliceEntities, entitiesToDetach);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
AZ::Entity* UiSliceManager::ValidateSingleRoot(const AzToolsFramework::EntityIdList& liveEntities, AzToolsFramework::EntityIdList& orderedEntityList, AZ::Entity*& insertBefore)
{
    // The low-level slice component code has no limit on there being a single root element
    // in a slice. It does make it simpler to do so though. Also this is the same limitation
    // that we had with the old Prefabs in the UI Editor.
    AZStd::unordered_set<AZ::EntityId> allEntities;
    allEntities.insert(liveEntities.begin(), liveEntities.end());

    AZStd::unordered_set<AZ::EntityId> childrenOfCommonParent;
    AZ::Entity* commonParent = nullptr;
    for (auto entity : allEntities)
    {
        AZ::Entity* parentElement = nullptr;
        EBUS_EVENT_ID_RESULT(parentElement, entity, UiElementBus, GetParent);

        if (parentElement)
        {
            // if this entities parent is not in the set then it is a top-level
            if (allEntities.count(parentElement->GetId()) == 0)
            {
                // this is a top level element
                if (commonParent)
                {
                    if (commonParent != parentElement)
                    {
                        // we have already found a parent
                        return nullptr;
                    }
                    else
                    {
                        childrenOfCommonParent.insert(entity);
                    }
                }
                else
                {
                    commonParent = parentElement;
                    childrenOfCommonParent.insert(entity);
                }
            }
        }
    }

    // At present there must be a single UI element that is the root element of the slice
    // This means that there should only be one child of the commonParent (the commonParent is always outside
    // of the slice)
    if (childrenOfCommonParent.size() != 1)
    {
        return nullptr;
    }

    // ensure that the top level entities are in the order that they are children of the common parent
    // without this check they would be in the order that they were selected
    orderedEntityList.clear();

    LyShine::EntityArray allChildrenOfCommonParent;
    EBUS_EVENT_ID_RESULT(allChildrenOfCommonParent, commonParent->GetId(), UiElementBus, GetChildElements);

    bool justFound = false;
    for (auto entity : allChildrenOfCommonParent)
    {
        // if this child is in the set of top level elements to go in the prefab
        // then add it to the vectors so that we have an ordered list in child order
        if (childrenOfCommonParent.count(entity->GetId()) > 0)
        {
            orderedEntityList.push_back(entity->GetId());

            // we are actually only supporting one child of the common parent
            // If this is it, set a flag so we can record the child immediately after it.
            // This is used later to insert the slice instance before this child
            justFound = true;
        }
        else
        {
            if (justFound)
            {
                insertBefore = entity;
                justFound = false;
            }
        }
    }

    // now add the rest of the entities (that are not top-level) to the list in any order
    for (auto entity : allEntities)
    {
        if (childrenOfCommonParent.count(entity) == 0)
        {
            orderedEntityList.push_back(entity);
        }
    }

    return commonParent;
}

//////////////////////////////////////////////////////////////////////////
bool UiSliceManager::ValidatePushSelection(AzToolsFramework::EntityIdList& entities, AZ::SerializeContext* serializeContext)
{
    // 1. All selected elements (and their descendants) should be part of the same prefab instance (or not in a slice)
    // 2. None of the elements in the prefab (or their children) can contain references to elements
    //    that will not become part of the prefab

    // First check that the selected entities are all either part of the same slice or are
    // not in a slice

    AZ::SliceComponent::SliceInstance* sliceInstance = nullptr;
    AZ::EntityId firstEntityInSliceInstance;

    AzToolsFramework::EntityIdList entitiesNotInSlice;
    for (auto entityId : entities)
    {
        AZ::SliceComponent::SliceInstanceAddress sliceAddress(nullptr, nullptr);
        EBUS_EVENT_ID_RESULT(sliceAddress, entityId, AzFramework::EntityIdContextQueryBus, GetOwningSlice);
        if (sliceAddress.second)    // the SliceInstance pointer
        {
            if (sliceInstance)
            {
                if (sliceAddress.second != sliceInstance)
                {
                    // We have already found an entity that is in a different slice instance
                    AZStd::string firstName = EntityHelpers::GetHierarchicalElementName(firstEntityInSliceInstance);
                    AZStd::string secondName = EntityHelpers::GetHierarchicalElementName(entityId);

                    AZStd::string message = AZStd::string::format(
                            "The selected entites cannot be pushed to a slice because entities from "
                            "multiple slice instances are selected.\r\n"
                            "Conflicting entity names are '%s' and '%s'.\r\n"
                            "Please make sure your selection contains only elements from one slice.\r\n",
                            firstName.c_str(), secondName.c_str());

                    QMessageBox::warning(QApplication::activeWindow(),
                        QStringLiteral("Cannot Push Selection to Slice"),
                        message.c_str(),
                        QMessageBox::Ok);
                    return false;
                }
            }
            else
            {
                // This is the first entity that is in a slice. Store a pointer to its SliceInstance.
                sliceInstance = sliceAddress.second;
                firstEntityInSliceInstance = entityId;
            }
        }
        else
        {
            // This entity is not yet in a slice, add it to the list to track entities that we are adding
            entitiesNotInSlice.push_back(entityId);
        }
    }

    if (!sliceInstance)
    {
        // None of the entities were in a slice. This is not valid as there is no slice to push to.
        // This should never happen because the menu item should be greyed out
        QMessageBox::warning(QApplication::activeWindow(),
            QStringLiteral("Cannot Push Selection to Slice"),
            QString("None of the selected entities are in a slice instance."),
            QMessageBox::Ok);
        return false;
    }

    // Expand the list to include children of the selected entities
    AzToolsFramework::EntityIdSet selectedHierarchyEntities = GatherEntitiesAndAllDescendents(entities);

    //HiearchyMenu::SliceMenuItems explicity passes in nullptr for the SerializeContext* to UiSliceManager::PushEntitiesModal, which then invokes this function
    if (!serializeContext)
    {
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to retrieve application serialize context.");
    }

    // Check for references that don't fall within the selected entity set.
    // Give the user the option to pull in all referenced entities, or to
    // stick with the current selection.
    AZStd::unordered_set<AZ::EntityId> allReferencedEntities = selectedHierarchyEntities;
    GatherAllReferencedEntities(allReferencedEntities, *serializeContext);
            
    // Check that all referenced entities are in the selection, in the UI system it is
    // generally true that entities can only reference other entities that are children
    for (const AZ::EntityId& id : allReferencedEntities)
    {
        if (selectedHierarchyEntities.find(id) == selectedHierarchyEntities.end())
        {
            // We found a referenced entity that is not in the selection
            AZ::Entity* referencedEntity = nullptr;
            EBUS_EVENT_RESULT(referencedEntity, AZ::ComponentApplicationBus, FindEntity, id);
            AZStd::string message;
            if (referencedEntity)
            {
                AZStd::string name = EntityHelpers::GetHierarchicalElementName(id);
                message = AZStd::string::format(
                        "Some of the selected entities reference entities not contained in the selection "
                        "and its children. UI slices cannot contain references to outside of the slice.\r\n\r\n"
                        "For example, entity '%s' is referenced but is not in the selection or its children.",
                        name.c_str());
            }
            else
            {
                message = AZStd::string::format(
                        "Some of the selected entities reference entities which no longer exist. "
                        "UI slices cannot contain references to outside of the slice.");
            }

            QMessageBox::warning(QApplication::activeWindow(), QStringLiteral("Cannot Push to Slice"),
                    QString(message.c_str()), QMessageBox::Ok);

            return false;
        }
    }

    // do another check, now that we have a list of the selected entities and all descendants,
    // make sure that all of these entities are either in the same slice or not in a slice
    // This is fairly restrictive but, since we do not yet support pushing a new slice instance into an
    // existing slice instance, it seems reasonable. Better to be too restrictive than to allow
    // invalid slices with dangling references.
    for (auto entityId : selectedHierarchyEntities)
    {
        AZ::SliceComponent::SliceInstanceAddress sliceAddress(nullptr, nullptr);
        EBUS_EVENT_ID_RESULT(sliceAddress, entityId, AzFramework::EntityIdContextQueryBus, GetOwningSlice);
        if (sliceAddress.second)
        {
            if (sliceAddress.second != sliceInstance)
            {
                AZStd::string firstName = EntityHelpers::GetHierarchicalElementName(firstEntityInSliceInstance);
                AZStd::string secondName = EntityHelpers::GetHierarchicalElementName(entityId);

                const AZStd::string message = AZStd::string::format(
                        "The selected entities cannot be pushed to a slice because entities from "
                        "multiple slice instances are selected or are the children of selected entities.\r\n"
                        "Conflicting entity names are '%s' and '%s'.\r\n"
                        "Please make sure your selection and its children contain only elements from one slice.\r\n",
                        firstName.c_str(), secondName.c_str());
                QMessageBox::warning(QApplication::activeWindow(),
                    QStringLiteral("Cannot Push Selection to Slice"),
                    message.c_str(),
                    QMessageBox::Ok);
                return false;
            }
        }
    }

    // if there are selected entities that are not in the slice then they will be added to the slice
    // but this means that their parent must be in the slice and their parent must be in the selection
    // so that the m_children array gets pushed
    AzToolsFramework::EntityIdList parentsToAddToSelection;
    for (auto entityId : entitiesNotInSlice)
    {
        AZ::Entity* parentElement = nullptr;
        EBUS_EVENT_ID_RESULT(parentElement, entityId, UiElementBus, GetParent);
        while (parentElement)
        {
            AZ::SliceComponent::SliceInstanceAddress sliceAddress(nullptr, nullptr);
            EBUS_EVENT_ID_RESULT(sliceAddress, parentElement->GetId(), AzFramework::EntityIdContextQueryBus, GetOwningSlice);
            if (sliceAddress.second)
            {
                if (sliceAddress.second == sliceInstance)
                {
                    // parentElement is in the same slice as the one we are pushing to
                    // this is a good thing
                    break;
                }
                else
                {
                    AZStd::string name = EntityHelpers::GetHierarchicalElementName(entityId);

                    AZStd::string message = AZStd::string::format(
                        "Selected entity '%s' is not in the slice and does not have an ancestor in the slice.\r\n"
                        "An Entity cannot be pushed to a slice if it doesn't have a parent in the slice.\r\n",
                        name.c_str());

                    QMessageBox::warning(QApplication::activeWindow(), QStringLiteral("Cannot Push to Slice"),
                            QString(message.c_str()), QMessageBox::Ok);

                    return false;
                }
            }

            // If the other tests pass then this parent should be added to the selection so that its m_children 
            // array is pushed.
            parentsToAddToSelection.push_back(parentElement->GetId());

            EBUS_EVENT_ID_RESULT(parentElement, parentElement->GetId(), UiElementBus, GetParent);
        }

        if (!parentElement)
        {
            AZStd::string name = EntityHelpers::GetHierarchicalElementName(entityId);

            AZStd::string message = AZStd::string::format(
                "Selected entity '%s' is not in the slice and does not have an ancestor in the slice.\r\n"
                "An Entity cannot be pushed to a slice if it doesn't have a parent in the slice.\r\n",
                name.c_str());

            QMessageBox::warning(QApplication::activeWindow(), QStringLiteral("Cannot Push to Slice"),
                    QString(message.c_str()), QMessageBox::Ok);

            return false;
        }

        // add this parent (the one we broke out of the loop on)
        parentsToAddToSelection.push_back(parentElement->GetId());
    }

    // now add any entities in parentsToAddToSelection that are not already in 'entities' to the list
    for (auto parentToAdd : parentsToAddToSelection)
    {
        if (AZStd::find(entities.begin(), entities.end(), parentToAdd) == entities.end())
        {
            entities.push_back(parentToAdd);
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool UiSliceManager::RootEntityTransforms(AZ::Data::Asset<AZ::SliceAsset>& targetSlice, const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& liveToAssetIdMap)
{
    AZ::SliceComponent::EntityList sliceEntities;
    targetSlice.Get()->GetComponent()->GetEntities(sliceEntities);

    // here we need to set the parent of the top-level elements to be null
    AZ::SliceComponent::EntityList topLevelEntities;
    GetTopLevelEntities(sliceEntities, topLevelEntities);

    AZ::u32 rootEntityCount = 0;
    for (AZ::Entity* entity : topLevelEntities)
    {
        // do something with offsets?
    }

    return true;
}
//////////////////////////////////////////////////////////////////////////
AZ::Entity* UiSliceManager::FindAncestorInTargetSlice(const AZ::Data::Asset<AZ::SliceAsset>& targetSlice,
    const AZ::SliceComponent::EntityAncestorList& ancestors)
{
    //  Locate target ancestor entity in slice asset.
    for (const AZ::SliceComponent::Ancestor& ancestor : ancestors)
    {
        if (ancestor.m_sliceAddress.first->GetSliceAsset().GetId() == targetSlice.GetId())
        {
            AZ::SliceComponent::EntityList entitiesInSlice;
            targetSlice.Get()->GetComponent()->GetEntities(entitiesInSlice);
            for (AZ::Entity* entityInSlice : entitiesInSlice)
            {
                if (entityInSlice->GetId() == ancestor.m_entity->GetId())
                {
                    return entityInSlice;
                }
            }
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
AZStd::string UiSliceManager::MakeTemporaryFilePathForSave(const char* targetFilename)
{
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    AZ_Assert(fileIO, "File IO is not initialized.");

    AZStd::string devAssetPath = fileIO->GetAlias("@devassets@");
    AZStd::string userPath = fileIO->GetAlias("@user@");
    AZStd::string tempPath = targetFilename;
    EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, devAssetPath);
    EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, userPath);
    EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, tempPath);
    AzFramework::StringFunc::Replace(tempPath, "@devassets@", devAssetPath.c_str());
    AzFramework::StringFunc::Replace(tempPath, devAssetPath.c_str(), userPath.c_str());
    tempPath.append(".slicetemp");

    return tempPath;
}

//////////////////////////////////////////////////////////////////////////
bool UiSliceManager::SaveSlice(const AZ::Data::Asset<AZ::SliceAsset>& slice,
    const char* fullPath,
    AZ::SerializeContext* serializeContext)
{
    AZ_Assert(slice.Get(), "Invalid asset provided, or asset is not created.");

    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    AZ_Assert(fileIO, "File IO is not initialized.");

    if (!serializeContext)
    {
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to retrieve application serialize context.");
    }

    // Write to a temporary location, and later move to the target location.
    const AZStd::string tempFilePath = MakeTemporaryFilePathForSave(fullPath);

    AZ::IO::FileIOStream fileStream(tempFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary);
    if (fileStream.IsOpen())
    {
        bool saved = AZ::Utils::SaveObjectToStream(fileStream, AZ::DataStream::ST_XML, slice.Get()->GetEntity(), serializeContext);
        fileStream.Close();
        if (saved)
        {
            // Copy scratch file to target location.
            const bool targetFileExists = fileIO->Exists(fullPath);
            const bool removedTargetFile = fileIO->Remove(fullPath);

            if (targetFileExists && !removedTargetFile)
            {
                QMessageBox::warning(AzToolsFramework::GetActiveWindow(),
                    QApplication::tr("Failed to Save Slice"),
                    QApplication::tr("Unable to modify existing target slice file. Please make the slice writeable and try again: \"%1\".").arg(fullPath));
                return false;
            }

            AZ::IO::Result renameResult = fileIO->Rename(tempFilePath.c_str(), fullPath);
            if (!renameResult)
            {
                QMessageBox::warning(AzToolsFramework::GetActiveWindow(),
                    QApplication::tr("Failed to Save Slice"),
                    QApplication::tr("Unable to move temporary slice file to target location: \"%1\".").arg(fullPath));
                fileIO->Remove(tempFilePath.c_str());
                return false;
            }

            // Bump the slice asset up in the asset processor's queue.
            EBUS_EVENT(AzFramework::AssetSystemRequestBus, GetAssetStatus, fullPath);
            return true;
        }
        else
        {
            QMessageBox::warning(AzToolsFramework::GetActiveWindow(),
                QApplication::tr("Failed to Save Slice"),
                QApplication::tr("Unable to save slice to a temporary file at location: \"%1\".").arg(tempFilePath.c_str()));
            return false;
        }

    }
    else
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(),
            QApplication::tr("Failed to Save Slice"),
            QApplication::tr("Unable to create temporary slice file at location: \"%1\".").arg(tempFilePath.c_str()));
        return false;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool UiSliceManager::ConfirmDialog_Detach(const QString& title, const QString& text)
{
    QMessageBox questionBox(QApplication::activeWindow());
    questionBox.setIcon(QMessageBox::Question);
    questionBox.setWindowTitle(title);
    questionBox.setText(text);
    QAbstractButton* detachButton = questionBox.addButton(QObject::tr("Detach"), QMessageBox::YesRole);
    questionBox.addButton(QObject::tr("Cancel"), QMessageBox::NoRole);
    questionBox.exec();
    return questionBox.clickedButton() == detachButton;
}

