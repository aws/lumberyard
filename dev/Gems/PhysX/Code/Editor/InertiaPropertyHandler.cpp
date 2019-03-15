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
#include <Editor/InertiaPropertyHandler.h>
#include <AzCore/Math/Matrix3x3.h>

namespace PhysX
{
    namespace Editor
    {
        AZ::u32 InertiaPropertyHandler::GetHandlerName(void) const
        {
            return InertiaHandler;
        }

        QWidget* InertiaPropertyHandler::CreateGUI(QWidget* parent)
        {
            AzToolsFramework::PropertyVectorCtrl* newCtrl = aznew AzToolsFramework::PropertyVectorCtrl(parent, 3, -1, "");
            connect(newCtrl, &AzToolsFramework::PropertyVectorCtrl::valueChanged, newCtrl, [newCtrl]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
            });

            newCtrl->setMinimum(0.0f);
            newCtrl->setMaximum(std::numeric_limits<float>::max());

            return newCtrl;
        }

        void InertiaPropertyHandler::ConsumeAttribute(AzToolsFramework::PropertyVectorCtrl* GUI, AZ::u32 attrib,
            AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
        {
        }

        void InertiaPropertyHandler::WriteGUIValuesIntoProperty(size_t index, AzToolsFramework::PropertyVectorCtrl* GUI,
            AZ::Matrix3x3& instance, AzToolsFramework::InstanceDataNode* node)
        {
            AzToolsFramework::VectorElement** elements = GUI->getElements();

            AZ::Vector3 diagonalElements(elements[0]->GetValue(), elements[1]->GetValue(), elements[2]->GetValue());
            instance = AZ::Matrix3x3::CreateDiagonal(diagonalElements);
        }

        bool InertiaPropertyHandler::ReadValuesIntoGUI(size_t index, AzToolsFramework::PropertyVectorCtrl* GUI,
            const AZ::Matrix3x3& instance, AzToolsFramework::InstanceDataNode* node)
        {
            QSignalBlocker signalBlocker(GUI);
            AZ::Vector3 diagonalElements = instance.GetDiagonal();
            for (int idx = 0; idx < 3; ++idx)
            {
                float value = diagonalElements.GetElement(idx);
                GUI->setValuebyIndex(diagonalElements.GetElement(idx), idx);
            }
            return true;
        }
    } // namespace Editor
} // namespace PhysX
