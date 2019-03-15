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

#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/Parameter/TagParameter.h>
#include <Editor/AnimGraphEditorBus.h>
#include <Editor/PropertyWidgets/AnimGraphTagHandler.h>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSignalBlocker>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphTagPicker, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphTagHandler, AZ::SystemAllocator, 0)

    AnimGraphTagPicker::AnimGraphTagPicker(QWidget* parent)
        : QWidget(parent)
        , m_animGraph(nullptr)
        , m_tagSelector(nullptr)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        // Create the tag selector widget.
        m_tagSelector = new AzQtComponents::TagSelector(this);

        connect(m_tagSelector, &AzQtComponents::TagSelector::TagsChanged, this, &AnimGraphTagPicker::OnSelectedTagsChanged);

        hLayout->addWidget(m_tagSelector);
        setLayout(hLayout);
    }


    void AnimGraphTagPicker::Reinit()
    {
        SetTags(m_tags);
    }


    void AnimGraphTagPicker::SetAnimGraph(AnimGraph* animGraph)
    {
        m_animGraph = animGraph;
    }


    void AnimGraphTagPicker::SetTags(const AZStd::vector<AZStd::string>& tags)
    {
        m_tags = tags;

        // Get the list of available tags and update the tag selector.
        QVector<QString> availableTags;
        GetAvailableTags(availableTags);
        m_tagSelector->Reinit(availableTags);

        // Get the tag strings from the array of string attributes for selection.
        QVector<QString> tagStrings;
        GetSelectedTags(tagStrings);
        
        QSignalBlocker tagSelectorSignalBlocker(m_tagSelector);
        m_tagSelector->SelectTags(tagStrings);
    }


    const AZStd::vector<AZStd::string>& AnimGraphTagPicker::GetTags() const
    {
        return m_tags;
    }


    void AnimGraphTagPicker::OnSelectedTagsChanged()
    {
        // Get the currently selected tag strings from the widget.
        QVector<QString> tagStrings;
        m_tagSelector->GetSelectedTagStrings(tagStrings);
        const int numTags = tagStrings.count();

        AZStd::vector<AZStd::string> newTags;
        newTags.reserve(numTags);
        for (const QString& tagString : tagStrings)
        {
            newTags.emplace_back(tagString.toUtf8().data());
        }

        m_tags = newTags;
        emit TagsChanged();
    }


    void AnimGraphTagPicker::GetAvailableTags(QVector<QString>& outTags) const
    {
        outTags.clear();

        if (!m_animGraph)
        {
            AZ_Error("EMotionFX", false, "Cannot open anim graph node selection window. No valid anim graph.");
            return;
        }

        const ValueParameterVector& valueParameters = m_animGraph->RecursivelyGetValueParameters();
        for (const ValueParameter* valueParameter : valueParameters)
        {
            if (azrtti_typeid(valueParameter) == azrtti_typeid<TagParameter>())
            {
                outTags.push_back(valueParameter->GetName().c_str());
            }
        }
    }


    void AnimGraphTagPicker::GetSelectedTags(QVector<QString>& outTags) const
    {
        outTags.clear();
        outTags.reserve(static_cast<int>(m_tags.size()));

        for (const AZStd::string& tag : m_tags)
        {
            outTags.push_back(tag.c_str());
        }
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AnimGraphTagHandler::AnimGraphTagHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, AnimGraphTagPicker>()
        , m_animGraph(nullptr)
    {
    }

    AZ::u32 AnimGraphTagHandler::GetHandlerName() const
    {
        return AZ_CRC("AnimGraphTags", 0x05dc9a94);
    }


    QWidget* AnimGraphTagHandler::CreateGUI(QWidget* parent)
    {
        AnimGraphTagPicker* picker = aznew AnimGraphTagPicker(parent);

        connect(picker, &AnimGraphTagPicker::TagsChanged, this, [picker]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
        });

        return picker;
    }


    void AnimGraphTagHandler::ConsumeAttribute(AnimGraphTagPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }

        if (attrib == AZ_CRC("AnimGraph", 0x0d53d4b3))
        {
            attrValue->Read<AnimGraph*>(m_animGraph);
            GUI->SetAnimGraph(m_animGraph);
        }
    }


    void AnimGraphTagHandler::WriteGUIValuesIntoProperty(size_t index, AnimGraphTagPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetTags();
    }


    bool AnimGraphTagHandler::ReadValuesIntoGUI(size_t index, AnimGraphTagPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetTags(instance);
        return true;
    }
} // namespace EMotionFX