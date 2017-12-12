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

#include <platform_impl.h>

#include <IGem.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Module/Module.h>

#include "source/Components/WanderMoveController.h"
#include "source/Components/PlayerControllerComponent.h"

namespace NetworkFeatureTest
{
    class Module
        : public CryHooksModule
    {
    public:
        AZ_RTTI(Module, "{10929E5E-AC42-4DAE-ACA1-F52888BAA383}", AZ::Module);

        Module()
            : CryHooksModule()
        {
            m_descriptors.push_back(WanderMoveController::CreateDescriptor());
            m_descriptors.push_back(PlayerControllerComponent::CreateDescriptor());
        }
    };
}

AZ_DECLARE_MODULE_CLASS(NetworkFeatureModule, NetworkFeatureTest::Module)
