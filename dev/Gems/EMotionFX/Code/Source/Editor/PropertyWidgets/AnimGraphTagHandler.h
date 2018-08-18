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

#include <EMotionFX/Source/AnimGraph.h>
#include <AzQtComponents/Components/TagSelector.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QWidget>
#include <QPushButton>


namespace EMotionFX
{
    class AnimGraphTagPicker
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphTagPicker(QWidget* parent);
        void SetAnimGraph(AnimGraph* animGraph);

        void SetTags(const AZStd::vector<AZStd::string>& tags);
        const AZStd::vector<AZStd::string>& GetTags() const;

    signals:
        void TagsChanged();

    private slots:
        void OnSelectedTagsChanged();

    private:
        void GetAvailableTags(QVector<QString>& outTags) const;
        void GetSelectedTags(QVector<QString>& outTags) const;

        AnimGraph*                      m_animGraph;
        AZStd::vector<AZStd::string>    m_tags;
        AzQtComponents::TagSelector*    m_tagSelector;
    };


    class AnimGraphTagHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, AnimGraphTagPicker>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphTagHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(AnimGraphTagPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, AnimGraphTagPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, AnimGraphTagPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    protected:
        AnimGraph* m_animGraph;
    };
} // namespace EMotionFX