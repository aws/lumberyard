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
#include "InputManagementFramework_precompiled.h"
#include "InputConfigurationComponent.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <InputRequestBus.h>
#include <PlayerProfileRequestBus.h>
#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/Utils.h>

namespace Input
{
    InputConfigurationComponent::~InputConfigurationComponent()
    {
        m_inputEventBindings.Cleanup();
    }

    void InputConfigurationComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("InputConfigurationService"));
    }

    void InputConfigurationComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<InputSubComponent>()
                ->Version(1)
                ;

            serializeContext->Class<InputConfigurationComponent, AZ::Component>()
                ->Version(2)
                ->Field("Input Event Bindings", &InputConfigurationComponent::m_inputEventBindingsAsset)
                ->Field("Input Contexts", &InputConfigurationComponent::m_inputContexts)
                ->Field("Local User Id", &InputConfigurationComponent::m_localUserId)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<InputSubComponent>
                    ("InputSubComponent", "The base class for all input handlers. Implementations will be found in other gems")
                    ;

                editContext->Class<InputConfigurationComponent>("Input",
                    "The Input component allows an entity to bind a set of inputs to an event by referencing a .inputbindings file")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Gameplay")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/InputConfig.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/InputConfig.png")
                        ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<InputEventBindingsAsset>::Uuid())
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-input.html")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &InputConfigurationComponent::m_inputEventBindingsAsset, "Input to event bindings",
                    "Asset containing input to event binding information.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                        ->Attribute("EditButton", "Editor/Icons/Assets/InputBindings")
                        ->Attribute("EditDescription", "Open in Input Bindings Editor")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &InputConfigurationComponent::m_inputContexts, "Input contexts", "These are the contexts valid for this input binding.  The default context is empty string")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &InputConfigurationComponent::m_localUserId, "Local player index",
                            "The player index that this component will receive input from (0 based, -1 means all controllers).\n"
                            "Will only work on platforms such as PC where the local user id corresponds to the local player index.\n"
                            "For other platforms, SetLocalUserId must be called at runtime with the id of a logged in user.")
                        ->Attribute(AZ::Edit::Attributes::Min, -1)
                        ->Attribute(AZ::Edit::Attributes::Max, 3)
                    ;
            }
        }
    }

    void InputConfigurationComponent::Init()
    {
    }

    void InputConfigurationComponent::Activate()
    {

        AZ::PlayerProfileNotificationBus::Handler::BusConnect();
        AZ::InputComponentRequestBus::Handler::BusConnect(GetEntityId());
        AZ::Data::AssetBus::Handler::BusConnect(m_inputEventBindingsAsset.GetId());

        // connect to all of our input contexts or "" if none are specified
        if (m_inputContexts.empty())
        {
            AZ::InputContextNotificationBus::MultiHandler::BusConnect(AZ::Crc32(""));
        }
        else
        {
            for (const AZStd::string& context : m_inputContexts)
            {
                AZ::InputContextNotificationBus::MultiHandler::BusConnect(AZ::Crc32(context.c_str()));
            }
        }
    }

    void InputConfigurationComponent::Deactivate()
    {
        AZ::InputContextNotificationBus::MultiHandler::BusDisconnect();
        AZ::PlayerProfileNotificationBus::Handler::BusDisconnect();
        AZ::InputComponentRequestBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::Handler::BusDisconnect();
        if (m_localUserId != AzFramework::LocalUserIdNone)
        {
            m_inputEventBindings.Deactivate(m_localUserId);
        }
    }

    void InputConfigurationComponent::SetLocalUserId(AzFramework::LocalUserId localUserId)
    {
        if (m_localUserId != localUserId)
        {
            if (m_localUserId != AzFramework::LocalUserIdNone)
            {
                m_inputEventBindings.Deactivate(m_localUserId);
            }
            m_localUserId = localUserId;
            if (m_localUserId != AzFramework::LocalUserIdNone)
            {
                m_inputEventBindings.Activate(m_localUserId);
            }
        }
    }

    // ToDo: Even with the old behavior I don't think this actually works,
    // as all active InputConfigurationComponents will get saved with the
    // same profile using the same asset id so they'll just override each
    // other? If we want to save a custom binding data patch we should do
    // it using something other than the CPlayerProfileManager, or allow
    // PlayerProfileRequests to be called using the id of the profile to
    // associate it with.
    void InputConfigurationComponent::OnProfileSaving()
    {
        if (m_inputEventBindingsAsset.IsReady())
        {
            AZ::DataPatch customBindingsPatch;
            customBindingsPatch.Create(&m_inputEventBindingsAsset.GetAs<InputEventBindingsAsset>()->m_bindings, &m_inputEventBindings);
            if (customBindingsPatch.IsData())
            {
                // store a non empty data patch using the asset id as the key
                AZStd::string assetIdAsString;
                m_inputEventBindingsAsset.GetId().ToString(assetIdAsString);
                bool isStored = false;
                AZ::PlayerProfileRequestBus::BroadcastResult(isStored, &AZ::PlayerProfileRequests::StoreData, assetIdAsString.c_str(), reinterpret_cast<void*>(&customBindingsPatch), AZ::AzTypeInfo<AZ::DataPatch>::Uuid(), nullptr);
            }
        }
    }

    void InputConfigurationComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset.GetId() == m_inputEventBindingsAsset.GetId())
        {
            // before we reload and reapply, disable any existing old ones, or else they'd double up
            // and you'd end up with both being active.
            if (m_localUserId != AzFramework::LocalUserIdNone)
            {
                m_inputEventBindings.Deactivate(m_localUserId);
            }

            m_inputEventBindingsAsset = asset;
            if (asset.IsReady())
            {
                OnAssetReady(asset);
            }
        }
    }

    void InputConfigurationComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (InputEventBindingsAsset* inputAsset = asset.GetAs<InputEventBindingsAsset>())
        {
            // the input asset actually requires us to do additional cloning and copying of the data
            // mainly because we retrieve the player profile data and apply it as a bindings patch on top of the data.

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            if (serializeContext)
            {
                // we swap with a fresh empty one here just to make sure that if this happens repeatedly, we don't have anything left over.
                InputEventBindings freshBindings;
                serializeContext->CloneObjectInplace<InputEventBindings>(freshBindings, &inputAsset->m_bindings);
                m_inputEventBindings.Cleanup();
                m_inputEventBindings.Swap(&freshBindings);
            }

            //patch the data
            void* rawData = nullptr;
            AZStd::string assetIdAsString;
            inputAsset->GetId().ToString(assetIdAsString);
            bool isLoaded = false;
            AZ::PlayerProfileRequestBus::BroadcastResult(isLoaded, &AZ::PlayerProfileRequests::RetrieveData, assetIdAsString.c_str(), &rawData, nullptr);
            AZ::DataPatch* customBindingsPatch = reinterpret_cast<AZ::DataPatch*>(rawData);
            if (customBindingsPatch && customBindingsPatch->IsValid())
            {
                if (InputEventBindings* patchedBindings = customBindingsPatch->Apply(&m_inputEventBindings))
                {
                    m_inputEventBindings = *patchedBindings;
                }
            }

            m_isAssetPrepared = true;
            ActivateBindingsIfAppropriate();
        }
        else
        {
            AZ_Error("Input Configuration", false, "Input bindings asset is not the correct type.");
        }
    }

    void InputConfigurationComponent::ActivateBindingsIfAppropriate()
    {
        if ((m_isContextActive) && (m_isAssetPrepared))
        {
            if (m_localUserId != AzFramework::LocalUserIdNone)
            {
                m_inputEventBindings.Activate(m_localUserId);
            }
        }
    }

    void InputConfigurationComponent::OnInputContextActivated()
    {
        m_isContextActive = true;
        ActivateBindingsIfAppropriate();
    }

    void InputConfigurationComponent::OnInputContextDeactivated()
    {
        m_isContextActive = false;

        if (m_localUserId != AzFramework::LocalUserIdNone)
        {
            m_inputEventBindings.Deactivate(m_localUserId);
        }
    }

    void InputConfigurationComponent::EditorSetPrimaryAsset(const AZ::Data::AssetId& assetId)
    {
        m_inputEventBindingsAsset.Create(assetId);
    }
}
