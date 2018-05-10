/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#include "CryEntityRemoval_precompiled.h"
#include <platform_impl.h>

#include "CryEntityRemovalSystemComponent.h"
#ifdef CRY_ENTITY_REMOVAL_EDITOR
#include <IEditor.h>
#include "EditorCoreAPI.h"
#endif

#include <IGem.h>

namespace CryEntityRemoval
{
    class CryEntityRemovalModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CryEntityRemovalModule, "{0A7C1550-A8F0-4C9E-A242-5403DACD93DB}", CryHooksModule);

        CryEntityRemovalModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CryEntityRemovalSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CryEntityRemovalSystemComponent>(),
            };
        }

        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override
        {
            CryHooksModule::OnCrySystemInitialized(system, systemInitParams);

            // Disable the legacy UI once the system has initialized
#ifdef CRY_ENTITY_REMOVAL_EDITOR
            GetIEditor()->SetLegacyUIEnabled(false);
#endif
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CryEntityRemoval_f3ae24a1635e4f849535f37ef9f4b4da, CryEntityRemoval::CryEntityRemovalModule)
