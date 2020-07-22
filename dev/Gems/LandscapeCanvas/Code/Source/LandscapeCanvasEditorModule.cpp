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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <LandscapeCanvasEditorModule.h>
#include <EditorLandscapeCanvasComponent.h>

namespace LandscapeCanvas
{
    LandscapeCanvasEditorModule::LandscapeCanvasEditorModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            LandscapeCanvasEditorSystemComponent::CreateDescriptor(),
            EditorLandscapeCanvasComponent::CreateDescriptor()
        });
    }

    AZ::ComponentTypeList LandscapeCanvasEditorModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList requiredComponents = LandscapeCanvasModule::GetRequiredSystemComponents();

        requiredComponents.push_back(azrtti_typeid<LandscapeCanvasEditorSystemComponent>());

        return requiredComponents;
    }

    void LandscapeCanvasEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LandscapeCanvasEditorSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<LandscapeCanvasEditorSystemComponent>("LandscapeCanvas", "Graph canvas representation of Dynamic Vegetation")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void LandscapeCanvasEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("LandscapeCanvasEditorService", 0x681491e3));
    }

    void LandscapeCanvasEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LandscapeCanvasEditorService", 0x681491e3));
    }

    void LandscapeCanvasEditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void LandscapeCanvasEditorSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& /*dependent*/)
    {

    }

    void LandscapeCanvasEditorSystemComponent::Activate()
    {
    }

    void LandscapeCanvasEditorSystemComponent::Deactivate()
    {
    }
}

AZ_DECLARE_MODULE_CLASS(LandscapeCanvasEditor, LandscapeCanvas::LandscapeCanvasEditorModule)