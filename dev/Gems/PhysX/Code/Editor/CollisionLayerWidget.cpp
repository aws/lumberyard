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
#include <AzFramework/Physics/PropertyTypes.h>
#include <PhysX/ConfigurationBus.h>
#include <Editor/CollisionLayerWidget.h>

#include <QEvent>

namespace PhysX
{
    namespace Editor
    {
        AZ::u32 CollisionLayerWidget::GetHandlerName() const
        {
            return Physics::Edit::CollisionLayerSelector;
        }

        QWidget* CollisionLayerWidget::CreateGUI(QWidget* parent)
        {
            QComboBox* picker = new QComboBox(parent);
            picker->installEventFilter(this);

            connect(picker, &QComboBox::currentTextChanged, this, [picker]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, picker);
            });

            return picker;
        }

        bool CollisionLayerWidget::eventFilter(QObject* /*object*/, QEvent* event)
        {
            if (event->type() == QEvent::Wheel)
            {
                return true;
            }
            return false;
        }

        bool CollisionLayerWidget::IsDefaultHandler() const
        {
            return true;
        }

        void CollisionLayerWidget::ConsumeAttribute(widget_t* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
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

        void CollisionLayerWidget::WriteGUIValuesIntoProperty(size_t index, widget_t* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
        {
            instance = GetLayerFromName(GUI->currentText().toUtf8().data());
        }

        bool CollisionLayerWidget::ReadValuesIntoGUI(size_t index, widget_t* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
        {
            QSignalBlocker signalBlocker(GUI);
            GUI->clear();

            auto layerNames = GetLayerNames();
            for (auto& layerName : layerNames)
            {
                GUI->addItem(layerName.c_str());
            }
            
            auto layerName = GetNameFromLayer(instance);
            GUI->setCurrentText(layerName.c_str());
            
            return true;
        }

        Physics::CollisionLayer CollisionLayerWidget::GetLayerFromName(const AZStd::string& layerName)
        {
            PhysX::Configuration configuration;
            PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &PhysX::ConfigurationRequests::GetConfiguration);
            return configuration.m_collisionLayers.GetLayer(layerName);
        }

        AZStd::string CollisionLayerWidget::GetNameFromLayer(const Physics::CollisionLayer& layer)
        {
            PhysX::Configuration configuration;
            PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &PhysX::ConfigurationRequests::GetConfiguration);
            return configuration.m_collisionLayers.GetName(layer);
        }

        AZStd::vector<AZStd::string> CollisionLayerWidget::GetLayerNames()
        {
            AZStd::vector<AZStd::string> layerNames;
            PhysX::Configuration configuration;
            PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &PhysX::ConfigurationRequests::GetConfiguration);
            for (AZ::u8 layer = 0; layer < Physics::CollisionLayers::s_maxCollisionLayers; ++layer)
            {
                auto layerName = configuration.m_collisionLayers.GetName(layer);
                if (!layerName.empty())
                {
                    layerNames.push_back(layerName);
                }
            }
            return layerNames;
        }
    }
    
} // namespace PhysX

#include <Editor/CollisionLayerWidget.moc>