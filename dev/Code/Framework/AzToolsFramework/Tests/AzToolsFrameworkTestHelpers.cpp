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

#include "AzToolsFrameworkTestHelpers.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/ToString.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>

#include <QAction>

// make gtest/gmock aware of these types so when a failure occurs we get more useful output
namespace AZ
{
    std::ostream& operator<<(std::ostream& os, const AZ::Vector3& vec)
    {
        return os << AZ::ToString(vec).c_str();
    }

    std::ostream& operator<<(std::ostream& os, const AZ::Quaternion& quat)
    {
        return os << AZ::ToString(quat).c_str();
    }
} // namespace AZ

using namespace AzToolsFramework;

namespace UnitTest
{
    bool TestWidget::eventFilter(QObject* watched, QEvent* event)
    {
        switch (event->type())
        {
        case QEvent::ShortcutOverride:
        {
            const auto& cachedActions = actions();

            // perform cast after type check (now safe to do so)
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            // do our best to build a key sequence
            // note: this will not handle more complex multi-key shortcuts at this time
            const auto keySequence = QKeySequence(keyEvent->modifiers() + keyEvent->key());
            // lookup the action that corresponds to this key event
            const auto actionIt = AZStd::find_if(
                cachedActions.begin(), cachedActions.end(), [keySequence](QAction* action)
            {
                return action->shortcut() == keySequence;
            });

            // if we get a match, generate the shortcut for this key combination
            if (actionIt != cachedActions.end())
            {
                // trigger shortcut
                QShortcutEvent shortcutEvent(keySequence, 0);
                QApplication::sendEvent(*actionIt, &shortcutEvent);
                keyEvent->accept();
                return true;
            }

            return false;
        }
        default:
            return false;
        }
    }

    void TestEditorActions::AddActionViaBus(int id, QAction* action)
    {
        AZ_Assert(action, "Attempting to add a null action");

        if (action)
        {
            action->setData(id);
            action->setShortcutContext(Qt::ApplicationShortcut);
            m_testWidget.addAction(action);
        }
    }

    void TestEditorActions::RemoveActionViaBus(QAction* action)
    {
        AZ_Assert(action, "Attempting to remove a null action");

        if (action)
        {
            m_testWidget.removeAction(action);
        }
    }

    EditorEntityComponentChangeDetector::EditorEntityComponentChangeDetector(const AZ::EntityId entityId)
    {
        PropertyEditorEntityChangeNotificationBus::Handler::BusConnect(entityId);
        EditorTransformChangeNotificationBus::Handler::BusConnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
    }

    EditorEntityComponentChangeDetector::~EditorEntityComponentChangeDetector()
    {
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
        EditorTransformChangeNotificationBus::Handler::BusDisconnect();
        PropertyEditorEntityChangeNotificationBus::Handler::BusDisconnect();
    }

    void EditorEntityComponentChangeDetector::OnEntityComponentPropertyChanged(const AZ::ComponentId componentId)
    {
        m_componentIds.push_back(componentId);
    }

    void EditorEntityComponentChangeDetector::OnEntityTransformChanged(
        const AzToolsFramework::EntityIdList& entityIds)
    {
        for (const AZ::EntityId& entityId : entityIds)
        {
            if (const auto * entity = GetEntityById(entityId))
            {
                if (AZ::Component * transformComponent = entity->FindComponent<Components::TransformComponent>())
                {
                    OnEntityComponentPropertyChanged(transformComponent->GetId());
                }
            }
        }
    }

    void EditorEntityComponentChangeDetector::InvalidatePropertyDisplay(
        AzToolsFramework::PropertyModificationRefreshLevel /*level*/)
    {
        m_propertyDisplayInvalidated = true;
    }

    AZ::EntityId CreateDefaultEditorEntity(const char* name, AZ::Entity** outEntity /*= nullptr*/)
    {
        AZ::Entity* entity = nullptr;
        AzFramework::EntityContextRequestBus::BroadcastResult(
            entity, &AzFramework::EntityContextRequests::CreateEntity, name);

        entity->Init();

        // add required components for the Editor entity
        entity->CreateComponent<Components::TransformComponent>();
        entity->CreateComponent<Components::EditorLockComponent>();
        entity->CreateComponent<Components::EditorVisibilityComponent>();

        entity->Activate();

        if (outEntity)
        {
            *outEntity = entity;
        }

        return entity->GetId();
    }

    AZ::Data::AssetId SaveAsSlice(
        AZStd::vector<AZ::Entity*> entities, AzToolsFramework::ToolsApplication* toolsApplication,
        SliceAssets& inoutSliceAssets)
    {
        AZ::Entity* sliceEntity = aznew AZ::Entity();
        AZ::SliceComponent* sliceComponent = aznew AZ::SliceComponent();
        sliceComponent->SetSerializeContext(toolsApplication->GetSerializeContext());
        for (const auto& entity : entities)
        {
            sliceComponent->AddEntity(entity);
        }

        // don't activate `sliceEntity`, whose purpose is to be attached by `sliceComponent`
        sliceEntity->AddComponent(sliceComponent);

        AZ::Data::AssetId assetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
        AZ::Data::Asset<AZ::SliceAsset> sliceAssetHolder =
            AZ::Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(assetId);
        sliceAssetHolder.GetAs<AZ::SliceAsset>()->SetData(sliceEntity, sliceComponent);

        // hold on to sliceAssetHolder so it's not ref-counted away
        inoutSliceAssets.emplace(assetId, sliceAssetHolder);

        return assetId;
    }

    AZ::SliceComponent::EntityList InstantiateSlice(
        AZ::Data::AssetId sliceAssetId, const SliceAssets& sliceAssets)
    {
        auto foundItr = sliceAssets.find(sliceAssetId);
        AZ_Assert(foundItr != sliceAssets.end(), "sliceAssetId not in sliceAssets");

        AZ::SliceComponent* rootSlice;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            rootSlice, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorRootSlice);
        AZ::SliceComponent::SliceInstanceAddress sliceInstAddress = rootSlice->AddSlice(foundItr->second);
        rootSlice->Instantiate();

        const AZ::SliceComponent::InstantiatedContainer* instanceContainer =
            sliceInstAddress.GetInstance()->GetInstantiated();

        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::AddEditorSliceEntities,
            instanceContainer->m_entities);

        return instanceContainer->m_entities;
    }

    void DestroySlices(SliceAssets& sliceAssets)
    {
        AZ::SliceComponent* rootSlice = nullptr;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            rootSlice, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorRootSlice);

        for (const auto& sliceAssetPair : sliceAssets)
        {
            rootSlice->RemoveSlice(sliceAssetPair.second);
        }

        sliceAssets.clear();
    }
} // namespace UnitTest
