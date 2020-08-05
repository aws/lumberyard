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

#include <NvCloth_precompiled.h>

#include <platform_impl.h> // Needed because of Cry_Vector3.h in ClothComponent. Added here for the whole module to be able to link its content.
#include <CrySystemBus.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <System/SystemComponent.h>
#include <Components/ClothComponent.h>

#ifdef NVCLOTH_EDITOR
#include <Components/EditorClothComponent.h>
#include <Pipeline/SceneAPIExt/ClothRuleBehavior.h>
#include <Pipeline/RCExt/CgfClothExporter.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Editor/PropertyTypes.h>
#endif //NVCLOTH_EDITOR

namespace NvCloth
{
    class Module
        : public AZ::Module
        , protected CrySystemEventBus::Handler
    {
    public:
        AZ_RTTI(Module, "{34C529D4-688F-4B51-BF60-75425754A7E6}", AZ::Module);
        AZ_CLASS_ALLOCATOR(Module, AZ::SystemAllocator, 0);

        Module()
            : AZ::Module()
        {
            CrySystemEventBus::Handler::BusConnect();

            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                SystemComponent::CreateDescriptor(),
                ClothComponent::CreateDescriptor(),

#ifdef NVCLOTH_EDITOR
                EditorClothComponent::CreateDescriptor(),
                Pipeline::ClothRuleBehavior::CreateDescriptor(),
                Pipeline::CgfClothExporter::CreateDescriptor(),
#endif //NVCLOTH_EDITOR
            });
        }

        ~Module()
        {
            CrySystemEventBus::Handler::BusDisconnect();
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<SystemComponent>(),
            };
        }

    protected:
        // CrySystemEventBus ...
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemShutdown(ISystem& system) override;

    private:
#ifdef NVCLOTH_EDITOR
        AZStd::vector<AzToolsFramework::PropertyHandlerBase*> m_propertyHandlers;
#endif //NVCLOTH_EDITOR
    };

    void Module::OnCrySystemInitialized(ISystem& system,
        [[maybe_unused]] const SSystemInitParams& systemInitParams)
    {
#ifdef NVCLOTH_EDITOR
        m_propertyHandlers = NvCloth::Editor::RegisterPropertyTypes();
#endif //NVCLOTH_EDITOR
    }

    void Module::OnCrySystemShutdown([[maybe_unused]] ISystem& system)
    {
#ifdef NVCLOTH_EDITOR
        NvCloth::Editor::UnregisterPropertyTypes(m_propertyHandlers);
#endif //NVCLOTH_EDITOR
    }
} // namespace NvCloth

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(NvCloth_6ab53783d9f54c9e97a15ad729e7c182, NvCloth::Module)
