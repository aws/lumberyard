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
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace AZ
{
    namespace VR
    {
        /*!
        * The VR Preview Component is responsible for setting up elements for the VR Preview functionality
        */
        class EditorVRPreviewComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
        {
        public:
            AZ_EDITOR_COMPONENT(EditorVRPreviewComponent, "{02C83699-3C4F-40A3-B135-F7BDD394D830}", AzToolsFramework::Components::EditorComponentBase);

            EditorVRPreviewComponent() = default;
            ~EditorVRPreviewComponent() override = default;

        protected:
            static void Reflect(ReflectContext* context);

        private:
            //////////////////////////////////////////////////////////////////////////
            // Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // AzToolsFramework::EditorEntityContextNotificationBus interface implementation
            void OnSliceInstantiated(const Data::AssetId& sliceAssetId, SliceComponent::SliceInstanceAddress& sliceAddress, const AzFramework::SliceInstantiationTicket& ticket) override;
            //////////////////////////////////////////////////////////////////////////

            /// Helper to generate the Editor navigation area
            void GenerateEditorNavigationArea();

        private:
            bool m_generateNavMesh = true;
            bool m_firstActivate = true;
            Vector3 m_bounds = Vector3(50.0f, 50.0f, 50.0f);
        };
    } // namespace VR
} // namespace AZ
