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
#include "AttributeWidgetCreators.h"
#include <MCore/Source/AttributeFactory.h>
#include <MCore/Source/AttributePool.h>


namespace MysticQt
{
    // constructor
    AttributeWidgetCreator::AttributeWidgetCreator()
    {
        mInitialMinValue = nullptr;
        mInitialMaxValue = nullptr;
    }


    // destructor
    AttributeWidgetCreator::~AttributeWidgetCreator()
    {
        if (mInitialMinValue)
        {
            mInitialMinValue->Destroy();
        }
        if (mInitialMaxValue)
        {
            mInitialMaxValue->Destroy();
        }
    }


    // create the attributes
    void AttributeWidgetCreator::CreateAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool initMinMax)
    {
        // get the attribute type
        const uint32 dataType = GetDataType(); // for example an AttributeFloat::TYPE_ID or something else

        // for all attributes
        const uint32 numAttributes = attributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            // if the attribute is not allocated yet or if the type changed, then recreate it
            if (attributes[i] == nullptr || attributes[i]->GetType() != dataType)
            {
                if (attributes[i])
                {
                    attributes[i]->Destroy();
                }
                attributes[i] = MCore::GetAttributeFactory().CreateAttributeByType(dataType);
            }
        }

        // create the min and max values, which are the interface widget min and max values (for example a double spincontrol min and max range)
        if (initMinMax)
        {
            // create the minimum value
            if (attributeSettings->GetMinValue() == nullptr || attributeSettings->GetMinValue()->GetType() != dataType)
            {
                attributeSettings->SetMinValue(MCore::GetAttributeFactory().CreateAttributeByType(dataType));
            }

            // create the maximum value
            if (attributeSettings->GetMaxValue() == nullptr || attributeSettings->GetMaxValue()->GetType() != GetDataType())
            {
                attributeSettings->SetMaxValue(MCore::GetAttributeFactory().CreateAttributeByType(dataType));
            }

            // create the initial values for min and max
            // note: this is not the widget range, but the actual initial value
            if (mInitialMinValue == nullptr)
            {
                mInitialMinValue = MCore::GetAttributeFactory().CreateAttributeByType(dataType);
            }
            if (mInitialMaxValue == nullptr)
            {
                mInitialMaxValue = MCore::GetAttributeFactory().CreateAttributeByType(dataType);
            }
        }
    }


    // initialize float attribs
    void FloatSpinnerAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
        if (forceInitMinMaxAttributes)
        {
            static_cast<MCore::AttributeFloat*>(attributeSettings->GetMinValue())->SetValue(-FLT_MAX);
            static_cast<MCore::AttributeFloat*>(attributeSettings->GetMaxValue())->SetValue(FLT_MAX);

            if (resetMinMaxAttributes)
            {
                static_cast<MCore::AttributeFloat*>(mInitialMinValue)->SetValue(0.0f);
                static_cast<MCore::AttributeFloat*>(mInitialMaxValue)->SetValue(1.0f);
            }
        }
    }


    // initialize checkbox attribs
    void CheckboxAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    // init integer attributes
    void IntSpinnerAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
        if (forceInitMinMaxAttributes)
        {
            static_cast<MCore::AttributeFloat*>(attributeSettings->GetMinValue())->SetValue(-10000000);
            static_cast<MCore::AttributeFloat*>(attributeSettings->GetMaxValue())->SetValue(10000000);

            if (resetMinMaxAttributes)
            {
                static_cast<MCore::AttributeFloat*>(mInitialMinValue)->SetValue(0);
                static_cast<MCore::AttributeFloat*>(mInitialMaxValue)->SetValue(100);
            }
        }
    }


    // init float attributes
    void FloatSliderAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
        if (forceInitMinMaxAttributes)
        {
            static_cast<MCore::AttributeFloat*>(attributeSettings->GetMinValue())->SetValue(-FLT_MAX);
            static_cast<MCore::AttributeFloat*>(attributeSettings->GetMaxValue())->SetValue(FLT_MAX);

            if (resetMinMaxAttributes)
            {
                static_cast<MCore::AttributeFloat*>(mInitialMinValue)->SetValue(0.0f);
                static_cast<MCore::AttributeFloat*>(mInitialMaxValue)->SetValue(1.0f);
            }
        }
    }


    // init int attributes
    void IntSliderAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
        if (forceInitMinMaxAttributes)
        {
            static_cast<MCore::AttributeFloat*>(attributeSettings->GetMinValue())->SetValue(-10000000);
            static_cast<MCore::AttributeFloat*>(attributeSettings->GetMaxValue())->SetValue(10000000);

            if (resetMinMaxAttributes)
            {
                static_cast<MCore::AttributeFloat*>(mInitialMinValue)->SetValue(0);
                static_cast<MCore::AttributeFloat*>(mInitialMaxValue)->SetValue(100);
            }
        }
    }


    // init int attribs
    void ComboBoxAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
        if (forceInitMinMaxAttributes)
        {
            static_cast<MCore::AttributeFloat*>(attributeSettings->GetMinValue())->SetValue(-10000000);
            static_cast<MCore::AttributeFloat*>(attributeSettings->GetMaxValue())->SetValue(10000000);
        }
    }


    // init vector2 attributes
    void Vector2AttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
        if (forceInitMinMaxAttributes)
        {
            static_cast<MCore::AttributeVector2*>(attributeSettings->GetMinValue())->SetValue(AZ::Vector2(-FLT_MAX, -FLT_MAX));
            static_cast<MCore::AttributeVector2*>(attributeSettings->GetMaxValue())->SetValue(AZ::Vector2(FLT_MAX,  FLT_MAX));

            if (resetMinMaxAttributes)
            {
                static_cast<MCore::AttributeVector2*>(mInitialMinValue)->SetValue(AZ::Vector2(-100000.0f, -100000.0f));
                static_cast<MCore::AttributeVector2*>(mInitialMaxValue)->SetValue(AZ::Vector2(100000.0f,  100000.0f));
            }
        }
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


    // init vector4 attributes
    void Vector4AttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
        if (forceInitMinMaxAttributes)
        {
            static_cast<MCore::AttributeVector4*>(attributeSettings->GetMinValue())->SetValue(AZ::Vector4(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX));
            static_cast<MCore::AttributeVector4*>(attributeSettings->GetMaxValue())->SetValue(AZ::Vector4(FLT_MAX,  FLT_MAX,  FLT_MAX,  FLT_MAX));

            if (resetMinMaxAttributes)
            {
                static_cast<MCore::AttributeVector4*>(mInitialMinValue)->SetValue(AZ::Vector4(-100000.0f, -100000.0f, -100000.0f, -100000.0f));
                static_cast<MCore::AttributeVector4*>(mInitialMaxValue)->SetValue(AZ::Vector4(100000.0f,  100000.0f,  100000.0f,  100000.0f));
            }
        }
    }


    // string attribute widget
    void StringAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }


    // init color attribs
    void ColorAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
        if (forceInitMinMaxAttributes)
        {
            static_cast<MCore::AttributeColor*>(attributeSettings->GetMinValue())->SetValue(MCore::RGBAColor(0.0f, 0.0f, 0.0f, 0.0f));
            static_cast<MCore::AttributeColor*>(attributeSettings->GetMaxValue())->SetValue(MCore::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f));
        }
    }


    // attribute set
    void AttributeSetAttributeWidgetCreator::InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes, bool resetMinMaxAttributes)
    {
        MCORE_UNUSED(resetMinMaxAttributes);
        CreateAttributes(attributes, attributeSettings, forceInitMinMaxAttributes);
    }
} // namespace MysticQt

#include <MysticQt/Source/AttributeWidgetCreators.moc>