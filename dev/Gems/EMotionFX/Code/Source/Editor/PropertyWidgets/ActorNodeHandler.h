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
#include <QWidget>
#include <QPushButton>


namespace EMotionFX
{
    class ActorNodePicker
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR_DECL

        ActorNodePicker(QWidget* parent, bool singleSelection=false);

        void SetNodeName(const AZStd::string& nodeName);
        AZStd::string GetNodeName() const;

        void SetNodeNames(const AZStd::vector<AZStd::string>& nodeNames);
        AZStd::vector<AZStd::string> GetNodeNames() const;

        void SetWeightedNodeNames(const AZStd::vector<AZStd::pair<AZStd::string, float>>& weightedNodeNames);
        AZStd::vector<AZStd::pair<AZStd::string, float>> GetWeightedNodeNames() const;

    signals:
        void SelectionChanged();

    private slots:
        void OnPickClicked();
        void OnResetClicked();

    private:
        void UpdateInterface();

        AZStd::vector<AZStd::pair<AZStd::string, float>>    m_weightedNodeNames;
        QPushButton*                                        m_pickButton;
        QPushButton*                                        m_resetButton;
        bool                                                m_singleSelection;
    };


    class ActorSingleNodeHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, ActorNodePicker>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(ActorNodePicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, ActorNodePicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, ActorNodePicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };


    class ActorMultiNodeHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, ActorNodePicker>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(ActorNodePicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, ActorNodePicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, ActorNodePicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };


    class ActorMultiWeightedNodeHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::pair<AZStd::string, float>>, ActorNodePicker>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(ActorNodePicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, ActorNodePicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, ActorNodePicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };
} // namespace EMotionFX