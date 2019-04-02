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

#include <PhysX_precompiled.h>
#include <Editor/CollisionGroupWidget.h>
#include <AzFramework/Physics/PropertyTypes.h>
#include <PhysX/ConfigurationBus.h>

#include <QEvent>

namespace PhysX
{
    namespace Editor
    {
        CollisionGroupWidget::CollisionGroupWidget()
        {
        }

        AZ::u32 CollisionGroupWidget::GetHandlerName() const
        {
            return Physics::Edit::CollisionGroupSelector;
        }

        QWidget* CollisionGroupWidget::CreateGUI(QWidget* parent)
        {
            widget_t* picker = new widget_t(parent);
            picker->installEventFilter(this);

            connect(picker, &widget_t::currentTextChanged, this, [picker]()
            {
                EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
            });

            return picker;
        }

        bool CollisionGroupWidget::eventFilter(QObject* /*object*/, QEvent* event)
        {
            if (event->type() == QEvent::Wheel)
            {
                return true;
            }
            return false;
        }

        bool CollisionGroupWidget::IsDefaultHandler() const
        {
            return true;
        }

        void CollisionGroupWidget::ConsumeAttribute(widget_t* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
        {
            if (attrib == AZ::Edit::Attributes::ReadOnly)
            {
                bool value = false;
                if (attrValue->Read<bool>(value))
                {
                    GUI->setEnabled(!value);
                }
            }
        }

        void CollisionGroupWidget::WriteGUIValuesIntoProperty(size_t index, widget_t* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
        {
            instance = GetGroupFromName(GUI->currentText().toUtf8().data());
        }

        bool CollisionGroupWidget::ReadValuesIntoGUI(size_t index, widget_t* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
        {
            QSignalBlocker signalBlocker(GUI);
            GUI->clear();

            auto groupNames = GetGroupNames();
            for (auto& layerName : groupNames)
            {
                GUI->addItem(layerName.c_str());
            }

            auto groupName = GetNameFromGroup(instance);
            GUI->setCurrentText(groupName.c_str());
            return true;
        }

        Physics::CollisionGroups::Id CollisionGroupWidget::GetGroupFromName(const AZStd::string& groupName)
        {
            PhysX::Configuration configuration;
            PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &PhysX::ConfigurationRequests::GetConfiguration);
            return configuration.m_collisionGroups.FindGroupIdByName(groupName);
        }

        AZStd::string CollisionGroupWidget::GetNameFromGroup(const Physics::CollisionGroups::Id& collisionGroup)
        {
            PhysX::Configuration configuration;
            PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &PhysX::ConfigurationRequests::GetConfiguration);
            return configuration.m_collisionGroups.FindGroupNameById(collisionGroup);
        }

        AZStd::vector<AZStd::string> CollisionGroupWidget::GetGroupNames()
        {
            PhysX::Configuration configuration;
            PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &PhysX::ConfigurationRequests::GetConfiguration);
            AZStd::vector<AZStd::string> groupNames;
            for (auto& preset : configuration.m_collisionGroups.GetPresets())
            {
                groupNames.push_back(preset.m_name);
            }
            return groupNames;
        }
    } // namespace Editor
} // namespace PhysX

#include <Editor/CollisionGroupWidget.moc>