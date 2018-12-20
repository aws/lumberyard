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
#include <AzToolsFramework/AzToolsFrameworkModule.h>

 // Component includes
#include <AzFramework/TargetManagement/TargetManagementComponent.h>

#include <AzToolsFramework/Archive/SevenZipComponent.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/Entity/EditorEntityActionComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/Entity/EditorEntityModelComponent.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>
#include <AzToolsFramework/Slice/SliceDependencyBrowserComponent.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextComponent.h>
#include <AzToolsFramework/SourceControl/PerforceComponent.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorSelectionAccentSystemComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/ToolsComponents/ScriptEditorComponent.h>
#include <AzToolsFramework/ToolsComponents/SelectionComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/MaterialBrowser/MaterialBrowserComponent.h>

namespace AzToolsFramework
{
    AzToolsFrameworkModule::AzToolsFrameworkModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            Components::TransformComponent::CreateDescriptor(),
            Components::SelectionComponent::CreateDescriptor(),
            Components::GenericComponentWrapper::CreateDescriptor(),
            Components::GenericComponentUnwrapper::CreateDescriptor(),
            Components::PropertyManagerComponent::CreateDescriptor(),
            Components::ScriptEditorComponent::CreateDescriptor(),
            Components::EditorSelectionAccentSystemComponent::CreateDescriptor(),
            EditorEntityContextComponent::CreateDescriptor(),
            SliceMetadataEntityContextComponent::CreateDescriptor(),
            Components::EditorEntityActionComponent::CreateDescriptor(),
            Components::EditorEntityIconComponent::CreateDescriptor(),
            Components::EditorInspectorComponent::CreateDescriptor(),
            Components::EditorLockComponent::CreateDescriptor(),
            Components::EditorPendingCompositionComponent::CreateDescriptor(),
            Components::EditorVisibilityComponent::CreateDescriptor(),
            Components::EditorDisabledCompositionComponent::CreateDescriptor(),
            Components::EditorOnlyEntityComponent::CreateDescriptor(),
            Components::EditorEntitySortComponent::CreateDescriptor(),
            Components::EditorEntityModelComponent::CreateDescriptor(),
            AssetSystem::AssetSystemComponent::CreateDescriptor(),
            PerforceComponent::CreateDescriptor(),
            AzToolsFramework::SevenZipComponent::CreateDescriptor(),
            AzToolsFramework::SliceDependencyBrowserComponent::CreateDescriptor(),
            AzToolsFramework::Thumbnailer::ThumbnailerComponent::CreateDescriptor(),
            AzToolsFramework::AssetBrowser::AssetBrowserComponent::CreateDescriptor(),
            AzToolsFramework::MaterialBrowser::MaterialBrowserComponent::CreateDescriptor(),
        });
    }
}