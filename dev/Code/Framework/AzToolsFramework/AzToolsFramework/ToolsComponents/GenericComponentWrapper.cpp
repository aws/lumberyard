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
#include "StdAfx.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzFramework/Components/EditorEntityEvents.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AzToolsFramework
{
    namespace Components
    {
        ////////////////////////////////////////////////////////////////////////
        // GenericComponentWrapper
        ////////////////////////////////////////////////////////////////////////

        void GenericComponentWrapper::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<GenericComponentWrapper, EditorComponentBase>()
                    ->Field("m_template", &GenericComponentWrapper::m_template);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<GenericComponentWrapper>("GenericComponentWrapper", "This should be hidden!")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &GenericComponentWrapper::GetDisplayName)
                        ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &GenericComponentWrapper::GetDisplayDescription)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement("", &GenericComponentWrapper::m_template, "m_template", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20));
                }
            }
        }

        GenericComponentWrapper::GenericComponentWrapper()
        {
        }

        GenericComponentWrapper::GenericComponentWrapper(const AZ::SerializeContext::ClassData* templateClassData)
        {
            EBUS_EVENT_ID_RESULT(m_template, templateClassData->m_typeId, AZ::ComponentDescriptorBus, CreateComponent);
        }

        GenericComponentWrapper::GenericComponentWrapper(AZ::Component* templateClassInstance)
        {
            if (templateClassInstance)
            {
                if (templateClassInstance->GetEntity())
                {
                    AZ_Error("GenericComponentWrapper", false, "Component must be detached from entity before placing inside GenericComponentWrapper.");
                }
                else
                {
                    m_id = templateClassInstance->GetId();
                    m_template = templateClassInstance;
                }
            }
        }

        GenericComponentWrapper::~GenericComponentWrapper()
        {
            delete m_template;
        }

        const char* GenericComponentWrapper::GetDisplayName() const
        {
            return m_displayName.c_str();
        }

        const char* GenericComponentWrapper::GetDisplayDescription() const
        {
            return m_displayDescription.c_str();
        }

        void GenericComponentWrapper::Init()
        {
            EditorComponentBase::Init();

            if (m_template)
            {
                m_displayName = GetFriendlyComponentName(m_template);
                m_displayDescription = GetFriendlyComponentDescription(m_template);
                if (m_displayDescription.empty())
                {
                    m_displayDescription = m_displayName;
                }

                m_templateEvents = azrtti_cast<AzFramework::EditorEntityEvents*>(m_template);
                if (m_templateEvents)
                {
                    m_templateEvents->EditorInit(GetEntityId());
                }
            }
        }

        void GenericComponentWrapper::Activate()
        {
            EditorComponentBase::Activate();

            if (m_templateEvents)
            {
                m_templateEvents->EditorActivate(GetEntityId());
                AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
            }
        }

        void GenericComponentWrapper::Deactivate()
        {
            EditorComponentBase::Deactivate();

            if (m_templateEvents)
            {
                AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
                m_templateEvents->EditorDeactivate(GetEntityId());
            }
        }

        void GenericComponentWrapper::BuildGameEntity(AZ::Entity* gameEntity)
        {
            if (m_template)
            {
                gameEntity->AddComponent(m_template);
            }
        }

        void GenericComponentWrapper::FinishedBuildingGameEntity(AZ::Entity* gameEntity)
        {
            if (m_template)
            {
                gameEntity->RemoveComponent(m_template);
            }
        }

        void GenericComponentWrapper::DisplayEntity(bool& handled)
        {
            if (m_templateEvents)
            {
                auto* displayInterface = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
                if (displayInterface)
                {
                    m_templateEvents->EditorDisplay(GetEntityId(), *displayInterface, GetWorldTM(), handled);
                }
            }
        }

        void GenericComponentWrapper::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
        {
            if (m_templateEvents)
            {
                m_templateEvents->EditorSetPrimaryAsset(assetId);
            }
        }

        AZ::Component* GenericComponentWrapper::ReleaseTemplate()
        {
            AZ::Component* component = m_template;
            m_template = nullptr;
            return component;
        }

        class GenericComponentWrapperDescriptor
            : public AZ::ComponentDescriptorHelper<GenericComponentWrapper>
        {
        public:
            AZ_CLASS_ALLOCATOR(GenericComponentWrapperDescriptor, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(GenericComponentWrapperDescriptor, "{3326B218-282B-4985-AEDE-39A77D48AD57}");

            AZ::ComponentDescriptor* GetTemplateDescriptor(const AZ::Component* instance) const
            {
                AZ::ComponentDescriptor* templateDescriptor = nullptr;

                const GenericComponentWrapper* wrapper = azrtti_cast<const GenericComponentWrapper*>(instance);
                if (wrapper && wrapper->GetTemplate())
                {
                    EBUS_EVENT_ID_RESULT(
                        templateDescriptor, wrapper->GetTemplate()->RTTI_GetType(),
                        AZ::ComponentDescriptorBus, GetDescriptor);
                }

                return templateDescriptor;
            }

            void Reflect(AZ::ReflectContext* reflection) const override
            {
                GenericComponentWrapper::Reflect(reflection);
            }

            void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided, const AZ::Component* instance) const override
            {
                const AZ::ComponentDescriptor* templateDescriptor = GetTemplateDescriptor(instance);
                if (templateDescriptor)
                {
                    templateDescriptor->GetProvidedServices(provided, instance);
                }
            }

            void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent, const AZ::Component* instance) const override
            {
                const AZ::ComponentDescriptor* templateDescriptor = GetTemplateDescriptor(instance);
                if (templateDescriptor)
                {
                    templateDescriptor->GetDependentServices(dependent, instance);
                }
            }

            void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required, const AZ::Component* instance) const override
            {
                const AZ::ComponentDescriptor* templateDescriptor = GetTemplateDescriptor(instance);
                if (templateDescriptor)
                {
                    templateDescriptor->GetRequiredServices(required, instance);
                }
            }

            void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible, const AZ::Component* instance) const override
            {
                const AZ::ComponentDescriptor* templateDescriptor = GetTemplateDescriptor(instance);
                if (templateDescriptor)
                {
                    templateDescriptor->GetIncompatibleServices(incompatible, instance);
                }
            }
        };

        AZ::ComponentDescriptor* GenericComponentWrapper::CreateDescriptor()
        {
            AZ::ComponentDescriptor* descriptor = nullptr;
            EBUS_EVENT_ID_RESULT(descriptor, GenericComponentWrapper::RTTI_Type(), AZ::ComponentDescriptorBus, GetDescriptor);

            return descriptor ? descriptor : aznew GenericComponentWrapperDescriptor();
        }

        ////////////////////////////////////////////////////////////////////////
        // GenericComponentUnwrapper
        ////////////////////////////////////////////////////////////////////////

        void GenericComponentUnwrapper::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<GenericComponentUnwrapper, AZ::Component>();
            }
        }

        void GenericComponentUnwrapper::Activate()
        {
            AZ::SliceAssetSerializationNotificationBus::Handler::BusConnect();
        }

        void GenericComponentUnwrapper::Deactivate()
        {
            AZ::SliceAssetSerializationNotificationBus::Handler::BusDisconnect();
        }

        // Why do the swap after a SliceAsset loads?
        // We can't swap during version-conversion of the editor-component
        // because converters don't have access to parent data.
        // We can't swap after writing GenericComponentWrapper from data
        // because we don't have access to the parent Entity.
        // We could have swapped after writing the AZ::Entity from data,
        // but Entities are written with high frequency for many reasons (ex: undo).
        // Therefore, do the swap after slice entities finish loading.
        // Any editor-entity that's saved out to disk will come in via a
        // SliceAsset, so this is a safe place for the check.
        void GenericComponentUnwrapper::OnSliceEntitiesLoaded(const AZStd::vector<AZ::Entity*>& entities)
        {
            for (AZ::Entity* entity : entities)
            {
                for (AZ::Component* component : entity->GetComponents())
                {
                    if (auto genericComponentWrapper = azrtti_cast<GenericComponentWrapper*>(component))
                    {
                        if (auto wrappedComponent = azrtti_cast<EditorComponentBase*>(genericComponentWrapper->GetTemplate()))
                        {
                            entity->SwapComponents(genericComponentWrapper, wrappedComponent);

                            genericComponentWrapper->ReleaseTemplate();
                            delete genericComponentWrapper;
                        }
                    }
                }
            }
        }

    }   // namespace Components

    const AZ::Uuid& GetUnderlyingComponentType(const AZ::Component& component)
    {
        if (const auto* componentWrapper = azdynamic_cast<const Components::GenericComponentWrapper*>(&component))
        {
            if (AZ::Component* underlyingComponent = componentWrapper->GetTemplate())
            {
                return azrtti_typeid(underlyingComponent);
            }
        }
        return azrtti_typeid(&component);
    }
} // namespace AzToolsFramework
