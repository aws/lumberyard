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
#include "EditorDefs.h"
#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <Water/WaterOceanComponentData.h>
#include <AzCore/Component/TransformBus.h>

namespace Water
{
    /**
    * The ocean component editor to author ocean parameter values in the editor
    */
    class WaterOceanEditor
        : public AzToolsFramework::Components::EditorComponentBase
        , private IEditorNotifyListener
        , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(WaterOceanEditor, "{961C77CC-CB98-49B8-83C4-CB7FD6D9AB5B}");

        static void Reflect(AZ::ReflectContext* context);

        WaterOceanEditor();
        ~WaterOceanEditor();

    protected:        

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::Components::EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation.
        void Activate() override;
        void Deactivate() override;

        //////////////////////////////////////////////////////////////////////////
        // IEditorNotifyListener
        void OnEditorNotifyEvent(EEditorNotifyEvent e) override;

        //////////////////////////////////////////////////////////////////////////
        // EditorVisibilityNotificationBus::Handler
        void OnEntityVisibilityChanged(bool visibility) override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

    private:
        WaterOceanComponentData m_data;
    };
}
