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

#include "NativeUISystemComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#pragma message("NativeUI: This Gem has been deprecated and is no longer needed by your project. Make sure to remove it from your game's gems settings.")

void NativeUI::SystemComponent::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serialize->Class<SystemComponent, AZ::Component>()
            ->Version(0)
            ;

        if (AZ::EditContext* ec = serialize->GetEditContext())
        {
            ec->Class<SystemComponent>("NativeUI", "Adds basic support for native (platform specific) UI dialog boxes (Deprecated! Use AZCore NativeUI Ebus)")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
        }
    }
}

void NativeUI::SystemComponent::Init()
{
}

void NativeUI::SystemComponent::Activate()
{
}

void NativeUI::SystemComponent::Deactivate()
{
}