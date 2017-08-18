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

#include "EditorComponentBase.h"
#include "EditorInspectorComponentBus.h"

namespace AzToolsFramework
{
    namespace Components
    {
        /**
        * Contains pending components to be added to the entity we are attached to.
        */
        class EditorInspectorComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorInspectorComponentRequestBus::Handler
        {
        public:
            AZ_COMPONENT(EditorInspectorComponent, "{47DE3DDA-50C5-4F50-B1DB-BA4AE66AB056}", EditorComponentBase);
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            
            ~EditorInspectorComponent();

            ////////////////////////////////////////////////////////////////////
            // EditorInspectorComponentRequestBus
            ComponentOrderArray GetComponentOrderArray() override;
            void SetComponentOrderArray(const ComponentOrderArray& componentOrderArray) override;

        private:
            ////////////////////////////////////////////////////////////////////
            // AZ::Entity
            void Init() override;
            ////////////////////////////////////////////////////////////////////

            ComponentOrderArray m_componentOrderArray;
        };
    } // namespace Components
} // namespace AzToolsFramework