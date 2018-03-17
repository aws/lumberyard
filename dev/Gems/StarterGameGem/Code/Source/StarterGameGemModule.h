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

#include "StarterGameGem_precompiled.h"
#include <platform_impl.h>

#include <IGem.h>

namespace StarterGameGem
{
    class StarterGameGemModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(StarterGameGemModule, "{D151EC6D-A70E-4A81-9B43-FE20AC2D1C3A}", CryHooksModule);

        StarterGameGemModule();

		void OnSystemEvent(ESystemEvent e, UINT_PTR wparam, UINT_PTR lparam) override;
		void PostSystemInit();
		void Shutdown();

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(StarterGameGem_5c539192ddda40aaa2e92eb71f8e3170, StarterGameGem::StarterGameGemModule)
