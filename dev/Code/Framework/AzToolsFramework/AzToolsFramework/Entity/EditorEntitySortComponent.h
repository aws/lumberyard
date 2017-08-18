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

#include "EditorEntitySortBus.h"
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace AzToolsFramework
{
    namespace Components
    {
        class EditorEntitySortComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorEntitySortRequestBus::Handler
        {
        public:
            AZ_COMPONENT(EditorEntitySortComponent, "{6EA1E03D-68B2-466D-97F7-83998C8C27F0}", EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);

            ~EditorEntitySortComponent();

            ////////////////////////////////////////
            // EditorEntitySortRequestBus::Handler
            ////////////////////////////////////////
            EntityOrderArray GetChildEntityOrderArray() override;
            void SetChildEntityOrderArray(const EntityOrderArray& entityOrderArray) override;
        private:
            ///////////////
            // AZ::Entity
            ///////////////
            void Init() override;

            EntityOrderArray m_childEntityOrderArray;
        };
    }
} // namespace AzToolsFramework
