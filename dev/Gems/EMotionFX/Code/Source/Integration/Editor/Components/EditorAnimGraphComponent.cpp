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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Script/ScriptProperty.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Integration/Editor/Components/EditorAnimGraphComponent.h>
#include <Integration/ActorComponentBus.h>

#include <MCore/Source/Attribute.h>
#include <MCore/Source/AttributeSettings.h>
#include <MCore/Source/AttributeFloat.h>

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MainWindow.h>


//#pragma optimize("",off)

namespace EMotionFX
{
    namespace Integration
    {
        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<EditorAnimGraphComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(1)
                    ->Field("AnimGraphAsset", &EditorAnimGraphComponent::m_animGraphAsset)
                    ->Field("MotionSetAsset", &EditorAnimGraphComponent::m_motionSetAsset)
                    ->Field("ParameterDefaults", &EditorAnimGraphComponent::m_parameterDefaults)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<AnimGraphComponent::ParameterDefaults>(
                        "Parameter Defaults", "Default values for anim graph parameters.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Button, &AnimGraphComponent::ParameterDefaults::m_parameters, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ;

                    editContext->Class<EditorAnimGraphComponent>(
                        "Anim Graph", "The Anim Graph component manages a set of assets that are built in the Animation Editor, including the animation graph, default parameter settings, and assigned motion set for the associated Actor")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Animation")
                        ->Attribute(AZ::Edit::Attributes::Icon, ":/EMotionFX/AnimGraphComponent.png")
                        ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<AnimGraphAsset>::Uuid())
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, ":/EMotionFX/AnimGraphComponent.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(0, &EditorAnimGraphComponent::m_motionSetAsset,
                            "Motion set", "EMotion FX motion set to be loaded for this actor.")
                        ->Attribute("EditButton", "Gems/EMotionFX/Assets/Editor/Images/Icons/EMFX_icon_32x32")
                        ->Attribute("EditDescription", "Open in Animation Editor")
                        ->Attribute("EditCallback", &EditorAnimGraphComponent::LaunchAnimationEditor)
                        ->DataElement(0, &EditorAnimGraphComponent::m_animGraphAsset,
                            "Anim graph", "EMotion FX anim graph to be assigned to this actor.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAnimGraphComponent::OnAnimGraphAssetSelected)
                        ->Attribute("EditButton", "Gems/EMotionFX/Assets/Editor/Images/Icons/EMFX_icon_32x32")
                        ->Attribute("EditDescription", "Open in Animation Editor")
                        ->Attribute("EditCallback", &EditorAnimGraphComponent::LaunchAnimationEditor)
                        ->DataElement(0, &EditorAnimGraphComponent::m_parameterDefaults,
                            "Parameters", "Anim graph default parameter values.")
                        ;
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        EditorAnimGraphComponent::EditorAnimGraphComponent()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        EditorAnimGraphComponent::~EditorAnimGraphComponent()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::Activate()
        {
            // Refresh parameters in case anim graph asset changed since last session.
            OnAnimGraphAssetSelected();
            EditorAnimGraphComponentRequestBus::Handler::BusConnect(GetEntityId());
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::Deactivate()
        {
            EditorAnimGraphComponentRequestBus::Handler::BusDisconnect();
            m_animGraphAsset.Release();
            m_motionSetAsset.Release();
        }

        void EditorAnimGraphComponent::LaunchAnimationEditor(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType)
        {
            if (assetId.IsValid())
            {
                AZ::Data::AssetId actorAssetId;
                actorAssetId.SetInvalid();
                EditorActorComponentRequestBus::EventResult(actorAssetId, GetEntityId(), &EditorActorComponentRequestBus::Events::GetActorAssetId);

                // call to open must be done before LoadCharacter
                const char* panelName = EMStudio::MainWindow::GetEMotionFXPaneName();
                EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, OpenViewPane, panelName);

                EMStudio::MainWindow* mainWindow = EMStudio::GetMainWindow();
                if (mainWindow)
                {
                    mainWindow->LoadCharacter(actorAssetId, m_animGraphAsset.GetId(), m_motionSetAsset.GetId());
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::OnAnimGraphAssetSelected()
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();

            if (m_animGraphAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::Handler::BusConnect(m_animGraphAsset.GetId());
                m_animGraphAsset.QueueLoad();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            // Re-process anim graph asset.
            OnAssetReady(asset);
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
        {
            AZ::Data::Asset<AnimGraphAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<AnimGraphAsset>(assetId, false);
            if (asset)
            {
                m_animGraphAsset = asset;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            if (asset != m_animGraphAsset)
            {
                return;
            }

            m_animGraphAsset = asset;

            AnimGraphAsset* data = m_animGraphAsset.GetAs<AnimGraphAsset>();
            if (!data)
            {
                return;
            }

            EMotionFX::AnimGraph* animGraph = data->GetAnimGraph().get();

            // Remove any parameters we have values for that are no longer in the anim graph.
            for (auto iter = m_parameterDefaults.m_parameters.begin(); iter != m_parameterDefaults.m_parameters.end(); )
            {
                const char* name = (*iter)->m_name.c_str();
                if (animGraph->FindParameterIndex(name) == MCORE_INVALIDINDEX32)
                {
                    delete *iter;
                    iter = m_parameterDefaults.m_parameters.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }

            // Populate property array based on parameters found in the anim graph.
            for (AZ::u32 paramIndex = 0; paramIndex < animGraph->GetNumParameters(); ++paramIndex)
            {
                MCore::AttributeSettings* param = animGraph->GetParameter(paramIndex);

                const char* paramName = param->GetName();

                // If we already have a value for this property, skip it.
                bool found = false;
                for (AZ::ScriptProperty* prop : m_parameterDefaults.m_parameters)
                {
                    if (0 == azstricmp(paramName, prop->m_name.c_str()))
                    {
                        found = true;
                        break;
                    }
                }

                if (found)
                {
                    continue;
                }

                // Based on the anim graph param type, create an appropriate script property for serialization and editing.
                MCore::Attribute* paramValue = param->GetDefaultValue();
                switch (paramValue->GetType())
                {
                case MCore::AttributeFloat::TYPE_ID:
                {
                    MCore::AttributeFloat* paramData = static_cast<MCore::AttributeFloat*>(paramValue);

                    switch (param->GetInterfaceType())
                    {
                    case MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER:
                    case MCore::ATTRIBUTE_INTERFACETYPE_FLOATSLIDER:
                    case MCore::ATTRIBUTE_INTERFACETYPE_INTSLIDER:
                    case MCore::ATTRIBUTE_INTERFACETYPE_INTSPINNER:
                    {
                        m_parameterDefaults.m_parameters.emplace_back(aznew AZ::ScriptPropertyNumber(paramName, paramData->GetValue()));
                    }
                    break;
                    case MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX:
                    {
                        m_parameterDefaults.m_parameters.emplace_back(aznew AZ::ScriptPropertyBoolean(paramName, paramData->GetValue() > 0.f));
                    }
                    break;
                    }
                }
                break;
                }
            }

            // Force-refresh the property grid.
            using namespace AzToolsFramework;
            EBUS_EVENT(ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, Refresh_EntireTree);
        }


        const AZ::Data::AssetId& EditorAnimGraphComponent::GetAnimGraphAssetId()
        {
            return m_animGraphAsset.GetId();
        }

        const AZ::Data::AssetId& EditorAnimGraphComponent::GetMotionSetAssetId()
        {
            return m_motionSetAsset.GetId();
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::BuildGameEntity(AZ::Entity* gameEntity)
        {
            AnimGraphComponent::Configuration cfg;
            cfg.m_animGraphAsset = m_animGraphAsset;
            cfg.m_motionSetAsset = m_motionSetAsset;
            cfg.m_parameterDefaults = m_parameterDefaults;

            gameEntity->AddComponent(aznew AnimGraphComponent(&cfg));
        }

       
    } //namespace Integration
} // namespace EMotionFX

