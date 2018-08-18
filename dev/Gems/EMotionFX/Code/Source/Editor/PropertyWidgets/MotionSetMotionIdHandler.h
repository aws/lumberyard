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

#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <EMotionFX/Source/AnimGraphNodeId.h>
#include <QWidget>
#include <QPushButton>


namespace EMotionFX
{
    class MotionSetMotionIdPicker
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR_DECL

        MotionSetMotionIdPicker(QWidget* parent);

        void SetMotionIds(const AZStd::vector<AZStd::string>& motionIds);
        AZStd::vector<AZStd::string> GetMotionIds() const;

    signals:
        void SelectionChanged();

    private slots:
        void OnPickClicked();

    private:
        void UpdateInterface();

        AZStd::vector<AZStd::string>    m_motionIds;
        QPushButton*                    m_pickButton;
    };


    class MotionSetMultiMotionIdHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, MotionSetMotionIdPicker>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(MotionSetMotionIdPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, MotionSetMotionIdPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, MotionSetMotionIdPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };
} // namespace EMotionFX