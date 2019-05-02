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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Physics/Utils.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyStringLineEditCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>

#include <QBoxLayout>

#include <Editor/CollisionLayersWidget.h>

namespace PhysX
{
    namespace Editor
    {
        const AZStd::string CollisionLayersWidget::s_defaultCollisionLayerName = "Default";

        CollisionLayersWidget::CollisionLayersWidget(QWidget* parent)
            : QWidget(parent)
        {
            CreatePropertyEditor(this);
        }

        void CollisionLayersWidget::SetValue(const Physics::CollisionLayers& layers)
        {
            m_value = layers;

            blockSignals(true);
            m_propertyEditor->ClearInstances();
            m_propertyEditor->AddInstance(&m_value);
            m_propertyEditor->InvalidateAll();
            SetWidgetParameters();
            blockSignals(false);
        }

        const Physics::CollisionLayers& CollisionLayersWidget::GetValue() const
        {
            return m_value;
        }

        void CollisionLayersWidget::CreatePropertyEditor(QWidget* parent)
        {
            QVBoxLayout* verticalLayout = new QVBoxLayout(parent);
            verticalLayout->setContentsMargins(0, 0, 0, 0);
            verticalLayout->setSpacing(0);

            AZ::SerializeContext* m_serializeContext;
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve serialize context.");

            static const int propertyLabelWidth = 250;
            m_propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(parent);
            m_propertyEditor->Setup(m_serializeContext, this, true, propertyLabelWidth);
            m_propertyEditor->show();
            m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            verticalLayout->addWidget(m_propertyEditor);
        }

        void CollisionLayersWidget::BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
        {

        }

        void CollisionLayersWidget::AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
        {

        }

        void CollisionLayersWidget::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/)
        {

        }

        void CollisionLayersWidget::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* node)
        {
            // Find index of modified layer
            AzToolsFramework::InstanceDataNode* nodeParent = node->GetParent();
            AzToolsFramework::InstanceDataNode::NodeContainer nodeSiblings = nodeParent->GetChildren();
            AZ::u64 nodeIndex = 0;
            AzToolsFramework::InstanceDataNode::Address nodeAddress = node->ComputeAddress();
            for (AzToolsFramework::InstanceDataNode& nodeSibling : nodeSiblings)
            {
                if (nodeSibling.ComputeAddress() == nodeAddress)
                {
                    break;
                }
                ++nodeIndex;
            }

            const AZStd::array<AZStd::string, Physics::CollisionLayers::s_maxCollisionLayers>& layerNames = m_value.GetNames();

            if (nodeIndex >= layerNames.size())
            {
                return;
            }
            
            // If corrections to layer name were made so it is unique, refresh UI.
            AZStd::string uniqueLayerName;
            if (ForceUniqueLayerName(nodeIndex, layerNames, uniqueLayerName))
            {
                AZ_Warning("PhysX Collision Layers"
                    , false
                    , "Invalid collision layer name used. Collision layer automatically renamed to: %s"
                    , uniqueLayerName.c_str());
                m_value.SetName(nodeIndex, uniqueLayerName);
                blockSignals(true);
                m_propertyEditor->InvalidateValues();
                blockSignals(false);
            }

            emit onValueChanged(m_value);
        }

        void CollisionLayersWidget::SealUndoStack()
        {

        }

        void CollisionLayersWidget::SetWidgetParameters()
        {
            using namespace AzToolsFramework;
            const ReflectedPropertyEditor::WidgetList& widgets = m_propertyEditor->GetWidgets();
            for (auto& widgetIter : widgets)
            {
                InstanceDataNode* dataNode = widgetIter.first;
                PropertyRowWidget* rowWidget = widgetIter.second;
                QWidget* widget = rowWidget->GetChildWidget();
                if (widget == nullptr)
                {
                    continue;
                }
                PropertyStringLineEditCtrl* lineEditCtrl = static_cast<PropertyStringLineEditCtrl*>(widget); // qobject_cast does not work here
                if (lineEditCtrl == nullptr)
                {
                    continue;
                }
                lineEditCtrl->setMaxLen(s_maxCollisionLayerNameLength);
                if (lineEditCtrl->value() == s_defaultCollisionLayerName)
                {
                    lineEditCtrl->setEnabled(false);
                }
            }
        }

        bool CollisionLayersWidget::ForceUniqueLayerName(AZ::u32 layerIndex
            , const AZStd::array<AZStd::string, Physics::CollisionLayers::s_maxCollisionLayers>& layerNames
            , AZStd::string& uniqueLayerNameOut)
        {
            if (layerIndex >= layerNames.size())
            {
                AZ_Warning("PhysX Collision Layers", false, "Trying to validate layer name of layer with invalid index: %d", layerIndex);
                return false;
            }

            if (IsLayerNameUnique(layerIndex, layerNames))
            {
                return false;
            }

            AZStd::unordered_set<AZStd::string> nameSet(layerNames.begin(), layerNames.end());
            nameSet.erase(AZStd::string("")); // Empty layer names are layers that are not used but remain in the array.
            uniqueLayerNameOut = layerNames[layerIndex];
            Physics::Utils::MakeUniqueString(nameSet
                , uniqueLayerNameOut
                , s_maxCollisionLayerNameLength);
            return true;
        }

        bool CollisionLayersWidget::IsLayerNameUnique(AZ::u32 layerIndex,
            const AZStd::array<AZStd::string, Physics::CollisionLayers::s_maxCollisionLayers>& layerNames)
        {
            for (size_t i = 0; i < layerNames.size(); ++i)
            {
                if (i == layerIndex)
                {
                    continue;
                }

                const AZStd::string& layerName = layerNames[i];
                if (layerName.length() > 0 && layerName == layerNames[layerIndex])
                {
                    return false;
                }
            }
            return true;
        }
    } // namespace Editor
} // namespace PhysX

#include <Editor/CollisionLayersWidget.moc>