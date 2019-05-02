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
#include <QLineEdit>
#include <QWidget>


QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace EMotionFX
{
    class ActorJointPicker
        : public QWidget
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        ActorJointPicker(bool singleSelection, const char* dialogTitle, const char* dialogDescriptionLabelText, QWidget* parent);

        void AddDefaultFilter(const QString& category, const QString& displayName);

        void SetJointName(const AZStd::string& jointName);
        AZStd::string GetJointName() const;

        void SetJointNames(const AZStd::vector<AZStd::string>& jointNames);
        AZStd::vector<AZStd::string> GetJointNames() const;

        void SetWeightedJointNames(const AZStd::vector<AZStd::pair<AZStd::string, float> >& weightedJointNames);
        AZStd::vector<AZStd::pair<AZStd::string, float> > GetWeightedJointNames() const;

    signals:
        void SelectionChanged();

    private slots:
        void OnPickClicked();
        void OnResetClicked();

    private:
        void UpdateInterface();

        AZStd::vector<AZStd::pair<AZStd::string, float> >   m_weightedJointNames;
        AZStd::vector<AZStd::pair<QString, QString> >       m_defaultFilters;
        AZStd::string                                       m_dialogTitle;
        AZStd::string                                       m_dialogDescriptionLabelText;
        QPushButton*                                        m_pickButton;
        QPushButton*                                        m_resetButton;
        bool                                                m_singleSelection;
    };


    class ActorJointElementWidget
        : public QLineEdit
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        ActorJointElementWidget(QWidget* parent);
    };


    class ActorJointElementHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, ActorJointElementWidget>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(ActorJointElementWidget* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, ActorJointElementWidget* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, ActorJointElementWidget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };


    class ActorSingleJointHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, ActorJointPicker>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(ActorJointPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, ActorJointPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, ActorJointPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };


    class ActorMultiJointHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, ActorJointPicker>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(ActorJointPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, ActorJointPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, ActorJointPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };


    class ActorMultiWeightedJointHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::pair<AZStd::string, float> >, ActorJointPicker>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(ActorJointPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, ActorJointPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, ActorJointPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };
} // namespace EMotionFX