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
#include "EntityQScrollArea.hxx"
#include <QMimeData>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsMessaging/EntityHighlightBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>

namespace AzToolsFramework
{
    EntityQScrollArea::EntityQScrollArea(QWidget* parent)
        : QScrollArea(parent)
    {
    }

    EntityQScrollArea::~EntityQScrollArea()
    {
    }

    void EntityQScrollArea::dragEnterEvent(QDragEnterEvent* event)
    {
        const QMimeData* mimeData = event->mimeData();
        EntityIdList selectedEntities;
        EBUS_EVENT_RESULT(selectedEntities, ToolsApplicationRequests::Bus, GetSelectedEntities);
        if (mimeData && selectedEntities.size() > 0)
        {
            if (mimeData->hasFormat(ComponentTypeMimeData::GetMimeType()) ||
                mimeData->hasFormat(ComponentAssetMimeDataContainer::GetMimeType()) ||
                mimeData->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
            {
                event->accept();
            }
        }
    }

    void EntityQScrollArea::dropEvent(QDropEvent* event)
    {
        const QMimeData* mimeData = event->mimeData();

        // Navigation triggered - Drag+Drop from Component Palette/File Browser/Asset Browser to Entity Inspector
        EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(EditorMetricsEventsBusTraits::NavigationTrigger::DragAndDrop);

        if (mimeData->hasFormat(ComponentTypeMimeData::GetMimeType()))
        {
            ComponentTypeMimeData::ClassDataContainer classDataContainer;
            ComponentTypeMimeData::Get(mimeData, classDataContainer);

            AZ::ComponentTypeList componentsToAdd;
            for (auto* classData : classDataContainer)
            {
                if (!classData)
                {
                    continue;
                }

                componentsToAdd.push_back(classData->m_typeId);
            }

            EntityIdList selectedEntities;
            ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);

            EntityCompositionRequests::AddComponentsOutcome addedComponentsResult = AZ::Failure(AZStd::string("Failed to call AddComponentsToEntities on EntityCompositionRequestBus"));
            EntityCompositionRequestBus::BroadcastResult(addedComponentsResult, &EntityCompositionRequests::AddComponentsToEntities, selectedEntities, componentsToAdd);

            if (addedComponentsResult.IsSuccess())
            {
                for (auto componentsAddedToEntity : addedComponentsResult.GetValue())
                {
                    auto entityId = componentsAddedToEntity.first;
                    for (auto componentAddedToEntity : componentsAddedToEntity.second.m_componentsAdded)
                    {
                        // Add Component metrics event (Drag+Drop from Component Palette to Entity Inspector)
                        EditorMetricsEventsBus::Broadcast(&EditorMetricsEventsBusTraits::ComponentAdded, entityId, GetComponentTypeId(componentAddedToEntity));
                    }
                }

                ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::InvalidatePropertyDisplay, Refresh_EntireTree_NewContent);
            }
            event->accept();
        }
        else if (mimeData->hasFormat(ComponentAssetMimeDataContainer::GetMimeType()))
        {
            ScopedUndoBatch undo("Add Component with asset");

            ComponentAssetMimeDataContainer mimeContainer;
            mimeContainer.FromMimeData(event->mimeData());

            //  Add the component
            for (int i = 0; i < mimeContainer.m_assets.size(); i++)
            {
                AZ::Data::AssetId assetId = mimeContainer.m_assets[i].m_assetId;
                AZ::Uuid componentType = mimeContainer.m_assets[i].m_classId;
                AddComponent(assetId, componentType);
            }
        }
        else if (mimeData->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
        {
            ScopedUndoBatch undo("Add Component with asset");

            AZStd::vector<AssetBrowser::AssetBrowserEntry*> entries;
            if (!AssetBrowser::AssetBrowserEntry::FromMimeData(mimeData, entries))
            {
                return;
            }

            // try to add components for each of selected entries
            for (auto entry : entries)
            {
                AZStd::vector<const AssetBrowser::ProductAssetBrowserEntry*> products;
                if (azrtti_istypeof<const AssetBrowser::ProductAssetBrowserEntry*>(entry))
                {
                    products.push_back(azrtti_cast<const AssetBrowser::ProductAssetBrowserEntry*>(entry));
                }
                else
                {
                    entry->GetChildren<AssetBrowser::ProductAssetBrowserEntry>(products);
                }
                for (auto product : products)
                {
                    AZ::Data::AssetId assetId = product->GetAssetId();
                    AZ::Uuid componentTypeId;
                    AZ::AssetTypeInfoBus::EventResult(componentTypeId, product->GetAssetType(), &AZ::AssetTypeInfo::GetComponentTypeId);
                    if (!componentTypeId.IsNull())
                    {
                        AddComponent(assetId, componentTypeId);
                    }
                }
            }
        }
        else
        {
            QScrollArea::dropEvent(event);
        }
    }

    void EntityQScrollArea::AddComponent(AZ::Data::AssetId assetId, AZ::Uuid componentType) const
    {
        EntityIdList selectedEntities;
        ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);

        EntityCompositionRequests::AddComponentsOutcome addComponentsOutcome = AZ::Failure(AZStd::string("AddComponentsToEntities is not handled"));
        EntityCompositionRequestBus::BroadcastResult(addComponentsOutcome, &EntityCompositionRequests::AddComponentsToEntities, selectedEntities, AZ::ComponentTypeList{ componentType });
        if (addComponentsOutcome)
        {
            for (auto entityId : selectedEntities)
            {
                // Add Component metrics event (Drag+Drop from File Browser/Asset Browser to Entity Inspector)
                EBUS_EVENT(EditorMetricsEventsBus, ComponentAdded, entityId, componentType);

                auto addedComponent = addComponentsOutcome.GetValue()[entityId].m_componentsAdded[0];
                AZ_Assert(GetComponentTypeId(addedComponent) == componentType, "Added component returned was not the type requested to add");

                auto editorComponent = GetEditorComponent(addedComponent);
                if (!editorComponent)
                {
                    continue;
                }

                editorComponent->SetPrimaryAsset(assetId);
            }
            EBUS_EVENT(ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, Refresh_EntireTree_NewContent);
        }
    }
}

#include <UI/PropertyEditor/EntityQScrollArea.moc>
