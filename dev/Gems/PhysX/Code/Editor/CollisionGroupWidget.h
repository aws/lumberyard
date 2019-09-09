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

#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzFramework/Physics/Collision.h>
#include <Editor/ComboBoxEditButtonPair.h>

namespace PhysX
{
    namespace Editor
    {
        class CollisionGroupWidget
            : public QObject
            , public AzToolsFramework::PropertyHandler<Physics::CollisionGroups::Id, ComboBoxEditButtonPair>
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(CollisionGroupWidget, AZ::SystemAllocator, 0);

            CollisionGroupWidget();
            
            AZ::u32 GetHandlerName() const override;
            QWidget* CreateGUI(QWidget* parent) override;
            bool IsDefaultHandler() const override;

            void ConsumeAttribute(widget_t* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

            void WriteGUIValuesIntoProperty(size_t index, widget_t* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
            bool ReadValuesIntoGUI(size_t index, widget_t* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

        private:
            Physics::CollisionGroups::Id GetGroupFromName(const AZStd::string& groupName);
            AZStd::string GetNameFromGroup(const Physics::CollisionGroups::Id& groupId);
            AZStd::vector<AZStd::string> GetGroupNames();
            void OnEditButtonClicked();
        };
    } // namespace Editor
} // namespace PhysX
