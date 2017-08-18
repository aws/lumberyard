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
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace LmbrCentral
{
    /**
     * Base class for in-editor physics components.
     */
    class EditorPhysicsComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorPhysicsComponent, AZ::SystemAllocator, 0);
        AZ_RTTI(EditorPhysicsComponent, "{9B5C5303-BDE6-49EA-87EE-9DA9ADD3ECA5}", EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        ~EditorPhysicsComponent() override = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
    };
} // namespace LmbrCentral