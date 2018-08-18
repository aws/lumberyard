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


#include "EMotionFX_precompiled.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Integration/Assets/ActorAsset.h>
#include <Integration/Editor/Components/EditorSimpleMotionComponent.h>

namespace EMotionFX
{
    namespace Integration
    {
        void EditorSimpleMotionComponent::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<EditorSimpleMotionComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(2)
                    ->Field("Configuration", &EditorSimpleMotionComponent::m_configuration)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorSimpleMotionComponent>(
                        "Simple Motion", "The Simple Motion component assigns a single motion to the associated Actor in lieu of an Anim Graph component")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Animation")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Mannequin.png")
                        ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, azrtti_typeid<MotionAsset>())
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Mannequin.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(0, &EditorSimpleMotionComponent::m_configuration,
                            "Configuration", "Settings for this Simple Motion")
                        ;
                }
            }

        }

        EditorSimpleMotionComponent::EditorSimpleMotionComponent()
            : m_configuration()
        {
        }

        EditorSimpleMotionComponent::~EditorSimpleMotionComponent()
        {
        }

        void EditorSimpleMotionComponent::Activate()
        {
            //check if our motion has changed
            VerifyMotionAssetState();
        }

        void EditorSimpleMotionComponent::Deactivate()
        {
            m_configuration.m_motionAsset.Release();
        }

        void EditorSimpleMotionComponent::VerifyMotionAssetState()
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
            if (m_configuration.m_motionAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::Handler::BusConnect(m_configuration.m_motionAsset.GetId());
                m_configuration.m_motionAsset.QueueLoad();
            }
        }

        void EditorSimpleMotionComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            if (asset != m_configuration.m_motionAsset)
            {
                return;
            }

            m_configuration.m_motionAsset = asset;
        }

        void EditorSimpleMotionComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            OnAssetReady(asset);
        }

        void EditorSimpleMotionComponent::BuildGameEntity(AZ::Entity* gameEntity)
        {
            gameEntity->AddComponent(aznew SimpleMotionComponent(&m_configuration));
        }
    }
}