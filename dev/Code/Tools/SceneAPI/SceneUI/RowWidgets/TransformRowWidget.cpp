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

#include <QLabel>
#include <QStyle.h>
#include <QGridLayout>
#include <AzFramework/Math/MathUtils.h>
#include <SceneAPI/SceneUI/RowWidgets/TransformRowWidget.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyVectorCtrl.hxx>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            AZ_CLASS_ALLOCATOR_IMPL(ExpandedTransform, SystemAllocator, 0);
            ExpandedTransform::ExpandedTransform(const Transform& transform)
            {
                SetTransform(transform);
            }

            void ExpandedTransform::SetTransform(const AZ::Transform& transform)
            {
                m_translation = transform.GetTranslation();
                m_rotation = AzFramework::ConvertTransformToEulerDegrees(transform);
                m_scale = transform.RetrieveScaleExact();
            }

            void ExpandedTransform::GetTransform(AZ::Transform& transform) const
            {
                transform = Transform::CreateTranslation(m_translation);
                transform *= AzFramework::ConvertEulerDegreesToTransform(m_rotation);
                transform.MultiplyByScale(m_scale);
            }

            const AZ::Vector3& ExpandedTransform::GetTranslation() const
            {
                return m_translation;
            }

            const AZ::Vector3& ExpandedTransform::GetRotation() const
            {
                return m_rotation;
            }

            const AZ::Vector3& ExpandedTransform::GetScale() const
            {
                return m_scale;
            }
            
            
            AZ_CLASS_ALLOCATOR_IMPL(TransformRowWidget, SystemAllocator, 0);

            TransformRowWidget::TransformRowWidget(QWidget* parent)
                : QWidget(parent)
            {
                QGridLayout* layout = new QGridLayout();
                setLayout(layout);

                m_translationWidget = aznew AzToolsFramework::PropertyVectorCtrl(this, 3);
                m_rotationWidget = aznew AzToolsFramework::PropertyVectorCtrl(this, 3);
                m_scaleWidget = aznew AzToolsFramework::PropertyVectorCtrl(this, 3);

                m_rotationWidget->setLabel(0, "P");
                m_rotationWidget->setLabel(1, "R");
                m_rotationWidget->setLabel(2, "Y");
                m_rotationWidget->setLabelStyle(0, "font: bold; color: rgb(184,51,51);");
                m_rotationWidget->setLabelStyle(1, "font: bold; color: rgb(48,208,120);");
                m_rotationWidget->setLabelStyle(2, "font: bold; color: rgb(66,133,244);");

                m_translationWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                m_rotationWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                m_scaleWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                
                layout->addWidget(new QLabel("Position"), 0, 0);
                layout->addWidget(m_translationWidget, 0, 1);
                layout->addWidget(new QLabel("Rotation"), 1, 0);
                layout->addWidget(m_rotationWidget, 1, 1);
                layout->addWidget(new QLabel("Scale"), 2, 0);
                layout->addWidget(m_scaleWidget, 2, 1);
            }

            void TransformRowWidget::SetTransform(const AZ::Transform& transform)
            {
                blockSignals(true);
                
                m_transform.SetTransform(transform);
                
                m_translationWidget->setValuebyIndex(m_transform.GetTranslation().GetX(), 0);
                m_translationWidget->setValuebyIndex(m_transform.GetTranslation().GetY(), 1);
                m_translationWidget->setValuebyIndex(m_transform.GetTranslation().GetZ(), 2);

                m_rotationWidget->setValuebyIndex(m_transform.GetRotation().GetX(), 0);
                m_rotationWidget->setValuebyIndex(m_transform.GetRotation().GetY(), 1);
                m_rotationWidget->setValuebyIndex(m_transform.GetRotation().GetZ(), 2);

                m_scaleWidget->setValuebyIndex(m_transform.GetScale().GetX(), 0);
                m_scaleWidget->setValuebyIndex(m_transform.GetScale().GetY(), 1);
                m_scaleWidget->setValuebyIndex(m_transform.GetScale().GetZ(), 2);

                blockSignals(false);
            }

            void TransformRowWidget::GetTransform(AZ::Transform& transform) const
            {
                return m_transform.GetTransform(transform);
            }

            const ExpandedTransform& TransformRowWidget::GetExpandedTransform() const
            {
                return m_transform;
            }

            AzToolsFramework::PropertyVectorCtrl* TransformRowWidget::GetTranslationWidget()
            {
                return m_translationWidget;
            }

            AzToolsFramework::PropertyVectorCtrl* TransformRowWidget::GetRotationWidget()
            {
                return m_rotationWidget;
            }

            AzToolsFramework::PropertyVectorCtrl* TransformRowWidget::GetScaleWidget()
            {
                return m_scaleWidget;
            }
        } // namespace SceneUI
    } // namespace SceneAPI
} // namespace AZ

#include <RowWidgets/TransformRowWidget.moc>
