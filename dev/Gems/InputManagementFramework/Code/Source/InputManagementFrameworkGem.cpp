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
#include <platform_impl.h>
#include <IGem.h>

#include <AzCore/std/containers/vector.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Module/Module.h>

#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/Input/System/InputSystemComponent.h>
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

#include <InputManagementFramework/InputEventBindings.h>
#include <InputManagementFramework/InputSubComponent.h>
#include <InputManagementFramework/InputEventGroup.h>

#include <InputNotificationBus.h>
#include <InputRequestBus.h>

#include "InputConfigurationComponent.h"

namespace Input
{
    class InputManagementFrameworkSystemComponent
        : public AZ::Component
        , public AZ::InputRequestBus::Handler
    {
    public:
        AZ_COMPONENT(InputManagementFrameworkSystemComponent, "{B52457FC-2DEA-4F0E-B0D0-F17786B40F02}");

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("InputManagementFrameworkService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("InputManagementFrameworkService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("AssetDatabaseService"));
            required.push_back(AZ_CRC("AssetCatalogService"));
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<InputManagementFrameworkSystemComponent, AZ::Component>()
                    ->Version(1)
                    ->SerializerForEmptyClass()
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<InputManagementFrameworkSystemComponent>(
                        "Input management framework", "Manages input bindings and events")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ;
                }
            }

            Input::InputEventBindings::Reflect(context);
            Input::InputEventGroup::Reflect(context);
            Input::InputEventBindingsAsset::Reflect(context);
        }

        void Activate() override
        {
            // Register asset handlers. Requires "AssetDatabaseService"
            AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");

            m_inputEventBindingsAssetHandler = aznew AzFramework::GenericAssetHandler<Input::InputEventBindingsAsset>("Input Bindings", "Other", "inputbindings", AZ::AzTypeInfo<InputConfigurationComponent>::Uuid());
            m_inputEventBindingsAssetHandler->Register();

            AZ::InputRequestBus::Handler::BusConnect();
            AZ::InputContextNotificationBus::Event(AZ::Crc32(GetCurrentContext().c_str()), &AZ::InputContextNotifications::OnInputContextActivated);
        }

        void Deactivate() override
        {
            AZ::InputRequestBus::Handler::BusDisconnect();

            delete m_inputEventBindingsAssetHandler;
            m_inputEventBindingsAssetHandler = nullptr;
        }

    protected:
        AZ::u8 RequestDeviceIndexMapping(const Input::ProfileId& profileId) override
        {
            if (m_profileIdToDeviceIndex.find(profileId) == m_profileIdToDeviceIndex.end())
            {
                AZ_Error("Input", m_nextValidDeviceIndex < std::numeric_limits<AZ::u8>::max(), "Attempting to request more devices than are supported");
                AZ_WarningOnce("Input", m_nextValidDeviceIndex < std::numeric_limits<AZ::u8>::digits, "You are requesting a high volume of devices, is this intended?");
                m_profileIdToDeviceIndex[profileId] = m_nextValidDeviceIndex++;
            }
            return m_profileIdToDeviceIndex[profileId];
        }

        AZ::u8 GetMappedDeviceIndex(const Input::ProfileId& profileId) override
        {
            if (m_profileIdToDeviceIndex.find(profileId) == m_profileIdToDeviceIndex.end())
            {
                AZ_Warning("Input", false, "Trying to get the device index for a profile that hasn't been mapped.");
                return 0;
            }
            return m_profileIdToDeviceIndex[profileId];
        }

        void ClearAllDeviceMappings()
        {
            m_nextValidDeviceIndex = 0;
            m_profileIdToDeviceIndex.clear();
        }

        void PushContext(const AZStd::string& context) override
        {
            AZ::InputContextNotificationBus::Event(AZ::Crc32(GetCurrentContext().c_str()), &AZ::InputContextNotifications::OnInputContextDeactivated);
            m_contexts.push_back(context);
            AZ::InputContextNotificationBus::Event(AZ::Crc32(context.c_str()), &AZ::InputContextNotifications::OnInputContextActivated);
        }

        void PopContext() override
        {
            if (m_contexts.size())
            {
                AZ::InputContextNotificationBus::Event(AZ::Crc32(GetCurrentContext().c_str()), &AZ::InputContextNotifications::OnInputContextDeactivated);
                m_contexts.pop_back();
                AZ::InputContextNotificationBus::Event(AZ::Crc32(GetCurrentContext().c_str()), &AZ::InputContextNotifications::OnInputContextActivated);
            }
        }

        void PopAllContexts() override
        {
            if (m_contexts.size())
            {
                AZ::InputContextNotificationBus::Event(AZ::Crc32(GetCurrentContext().c_str()), &AZ::InputContextNotifications::OnInputContextDeactivated);
                m_contexts.clear();
                AZ::InputContextNotificationBus::Event(AZ::Crc32(GetCurrentContext().c_str()), &AZ::InputContextNotifications::OnInputContextActivated);
            }
        }

        AZStd::string GetCurrentContext() override
        {
            if (m_contexts.size())
            {
                return m_contexts.back();
            }
            return AZStd::string("");
        }

        AZStd::vector<AZStd::string> GetContextStack() override
        {
            return m_contexts;
        }

    private:
        AzFramework::GenericAssetHandler<Input::InputEventBindingsAsset>* m_inputEventBindingsAssetHandler = nullptr;
        AZStd::unordered_map<Input::ProfileId, AZ::u8> m_profileIdToDeviceIndex;
        AZStd::vector<AZStd::string> m_contexts;
        AZ::u8 m_nextValidDeviceIndex = 0;
    };

    class InputManagementFrameworkModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(InputManagementFrameworkModule, "{F42A5E0C-2EA5-46C1-BA0F-D4A665653B0B}", AZ::Module);

        InputManagementFrameworkModule()
            : CryHooksModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                InputConfigurationComponent::CreateDescriptor(),
                InputManagementFrameworkSystemComponent::CreateDescriptor()
            });





            // This is an internal Amazon gem, so register it's components for metrics tracking, otherwise the name of the component won't get sent back.
            // IF YOU ARE A THIRDPARTY WRITING A GEM, DO NOT REGISTER YOUR COMPONENTS WITH EditorMetricsComponentRegistrationBus
            AZStd::vector<AZ::Uuid> typeIds;
            typeIds.reserve(m_descriptors.size());
            for (AZ::ComponentDescriptor* descriptor : m_descriptors)
            {
                typeIds.emplace_back(descriptor->GetUuid());
            }
            EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, typeIds);
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return{
                azrtti_typeid<InputManagementFrameworkSystemComponent>()
            };
        }

        void OnSystemEvent(ESystemEvent event, UINT_PTR, UINT_PTR) override
        {
            switch (event)
            {
                case ESYSTEM_EVENT_LEVEL_LOAD_START:
                case ESYSTEM_EVENT_GAME_MODE_SWITCH_START:
                {
                    AZ::InputRequestBus::Broadcast(&AZ::InputRequests::ClearAllDeviceMappings);
                    AZ::InputRequestBus::Broadcast(&AZ::InputRequests::PopAllContexts);
                }
                break;
                default:
                break;
            }
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(InputManagementFramework_59b1b2acc1974aae9f18faddcaddac5b, Input::InputManagementFrameworkModule)
