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

#include "BlendTreeFloatConstantNode.h"
#include <MCore/Source/AttributeSettings.h>

#include <AzFramework/StringFunc/StringFunc.h>


namespace EMotionFX
{
    BlendTreeFloatConstantNode::BlendTreeFloatConstantNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();

        SetNodeInfo("0.0");
    }


    BlendTreeFloatConstantNode::~BlendTreeFloatConstantNode()
    {
    }


    BlendTreeFloatConstantNode* BlendTreeFloatConstantNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeFloatConstantNode(animGraph);
    }


    AnimGraphObjectData* BlendTreeFloatConstantNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    void BlendTreeFloatConstantNode::RegisterPorts()
    {
        InitOutputPorts(1);
        SetupOutputPort("Output", OUTPUTPORT_RESULT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_RESULT);
    }


    void BlendTreeFloatConstantNode::RegisterAttributes()
    {
        MCore::AttributeSettings* param = RegisterAttribute("Constant Value", "constantValue", "The value that the node will output.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        param->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        param->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        param->SetMaxValue(MCore::AttributeFloat::Create(+FLT_MAX));
        param->SetReinitGuiOnValueChange(true);
    }


    const char* BlendTreeFloatConstantNode::GetPaletteName() const
    {
        return "Float Constant";
    }


    AnimGraphObject::ECategory BlendTreeFloatConstantNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    AnimGraphObject* BlendTreeFloatConstantNode::Clone(AnimGraph* animGraph)
    {
        BlendTreeFloatConstantNode* clone = new BlendTreeFloatConstantNode(animGraph);
        CopyBaseObjectTo(clone);
        return clone;
    }


    void BlendTreeFloatConstantNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        const float result = GetAttributeFloatAsFloat(ATTRIB_VALUE);
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(result);
    }


    const char* BlendTreeFloatConstantNode::GetTypeString() const
    {
        return "BlendTreeFloatConstantNode";
    }


    void BlendTreeFloatConstantNode::OnUpdateAttributes()
    {
        const float value = GetAttributeFloatAsFloat(ATTRIB_VALUE);
        AZStd::string valueString = AZStd::string::format("%.6f", value);
        
        // Remove the zero's at the end.
        const size_t dotZeroOffset = valueString.find(".0");
        if (AzFramework::StringFunc::Strip(valueString, "0", false, false, true))   // If something got stripped.
        {
            // If it was say "5.0" and we removed all zeros at the end, we are left with "5.".
            // Turn that into 5.0 again by adding the 0.
            // We detect this by checking if we can still find a ".0" after the stripping of zeros.
            // If there was a ".0" in the string before, and now there isn't, we have to add the 0 back to it.
            const size_t dotZeroOffsetAfterTrim = valueString.find(".0");
            if (dotZeroOffset != AZStd::string::npos && dotZeroOffsetAfterTrim == AZStd::string::npos)
                valueString += "0";
        }

        SetNodeInfo(valueString.c_str());
    }


    uint32 BlendTreeFloatConstantNode::GetVisualColor() const
    {
        return MCore::RGBA(128, 255, 255);
    }


    bool BlendTreeFloatConstantNode::GetSupportsDisable() const
    {
        return false;
    }

}   // namespace EMotionFX

