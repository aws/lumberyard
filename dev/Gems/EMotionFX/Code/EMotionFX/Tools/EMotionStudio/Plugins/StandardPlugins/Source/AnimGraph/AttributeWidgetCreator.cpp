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

// include required headers
#include "AttributeWidgetCreator.h"
#include <MCore/Source/AttributeFactory.h>


namespace EMStudio
{
    // init string attributes
    void FileBrowserAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    // init vector3 attributes
    void Vector3AttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
        if (forceInitMinMaxAttributes)
        {
            static_cast<MCore::AttributeVector3*>(attributeSettings->GetMinValue())->SetValue(MCore::Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
            static_cast<MCore::AttributeVector3*>(attributeSettings->GetMaxValue())->SetValue(MCore::Vector3(FLT_MAX,  FLT_MAX,  FLT_MAX));

            if (resetMinMaxAttributes)
            {
                static_cast<MCore::AttributeVector3*>(mInitialMinValue)->SetValue(MCore::Vector3(-100000.0f, -100000.0f, -100000.0f));
                static_cast<MCore::AttributeVector3*>(mInitialMaxValue)->SetValue(MCore::Vector3(100000.0f,  100000.0f,  100000.0f));
            }
        }
    }


    // init rotation attributes
    void RotationAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    // init node names attributes
    void NodeNamesAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    // init node selection attributes
    void NodeSelectionAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    // init node selection attributes
    void GoalNodeSelectionAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    // init state selection attributes
    void AnimGraphStateAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    // init string attribute
    void MotionPickerAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    // init array attribute
    void MultipleMotionPickerAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }

    void BlendSpaceMotionsAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }

    void BlendSpaceMotionPickerAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }

    // init int attribs
    void ParameterPickerAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    // init anim graph node picker attribute
    void AnimGraphNodeAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    // init blend tree motion attribute
    void BlendTreeMotionAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    // init string attribs
    void MotionEventTrackAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }
    /*
    // init int attribs
    void MotionExtractionComponentWidgetCreator::InitAttributes(Array<Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes( attributes, attributeSettings, forceInitMinMaxAttributes );
    }
    */

    // init parameter names attributes
    void ParameterNamesAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    // init node groups attributes
    void StateFilterLocalAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    void TagAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    void TagPickerAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AttributeWidgetCreator.moc>
