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
#include <QRegExpValidator>


namespace EMotionFX
{
    class AnimGraphNode;

    class AnimGraphNodeNameLineEdit
        : public QLineEdit
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphNodeNameLineEdit(QWidget* parent);
        ~AnimGraphNodeNameLineEdit() = default;

        void SetNode(AnimGraphNode* node);
        void SetName(const QString& newName);
        void SetPreviousName(const QString& previousName);
        QString GetName() const;
        QString GetPreviousName() const;

    signals:
        void NameChanged();

    private slots:
        void OnTextChanged();
        void OnEditingFinished();

    private:
        bool IsValid();

        AnimGraphNode* m_node;
        QString m_previousName;

        QRegExp m_validationExpr;
        QRegExpValidator m_lineValidator;
    };


    /**
    * The AnimGraphNodeNameHandler is a custom property handler for the name property of AnimGraphNodes.
    * It validates if the currently entered name candidate is unique and blocks a name change in case it isn't.
    */
    class AnimGraphNodeNameHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, AnimGraphNodeNameLineEdit>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphNodeNameHandler();
        ~AnimGraphNodeNameHandler() = default;

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(AnimGraphNodeNameLineEdit* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, AnimGraphNodeNameLineEdit* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, AnimGraphNodeNameLineEdit* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    protected:
        AnimGraphNode* m_node;
    };
} // namespace EMotionFX