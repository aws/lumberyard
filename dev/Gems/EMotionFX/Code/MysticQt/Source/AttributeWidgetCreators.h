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

#ifndef __MYSTICQT_ATTRIBUTEWIDGETCREATORS_H
#define __MYSTICQT_ATTRIBUTEWIDGETCREATORS_H

// include required headers
#include <MCore/Source/StandardHeaders.h>
#include "MysticQtConfig.h"
#include "AttributeWidgets.h"


namespace MysticQt
{
    class MYSTICQT_API AttributeWidgetCreator
    {
        MCORE_MEMORYOBJECTCATEGORY(AttributeWidgetCreator, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_ATTRIBUTEWIDGETS);

    public:
        AttributeWidgetCreator();
        virtual ~AttributeWidgetCreator();

        virtual uint32 GetType() const = 0;
        virtual uint32 GetDefaultType() const = 0;
        virtual uint32 GetMinMaxType() const = 0;
        virtual uint32 GetDataType() const = 0;
        virtual bool CanBeParameter() const = 0;
        virtual const char* GetTypeString() const = 0;
        virtual bool GetHasMinMaxValues() const = 0;
        virtual void InitAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes = false, bool resetMinMaxAttributes = true) = 0;

        virtual AttributeWidget* Clone(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func) = 0;

        void CreateAttributes(MCore::Array<MCore::Attribute*>& attributes, MCore::AttributeSettings* attributeSettings, bool initMinMax);
        MCore::Attribute* GetInitialMinValue() const    { return mInitialMinValue; }
        MCore::Attribute* GetInitialMaxValue() const    { return mInitialMaxValue; }

    protected:
        MCore::Attribute*   mInitialMinValue;
        MCore::Attribute*   mInitialMaxValue;
    };


    // easy creator class definition
#define DEFINE_ATTRIBUTEWIDGETCREATOR(CLASSNAME, ATTRIBUTEWIDGETCLASSNAME, TYPEID, DEFAULTYPEID, MINMAXTYPEID, DATATYPEID, HASMINMAXVALUES, CANBEPARAMETER, TYPESTRING)                                          \
    class MYSTICQT_API CLASSNAME                                                                                                                                                                                 \
        : public AttributeWidgetCreator                                                                                                                                                                          \
    {                                                                                                                                                                                                            \
    public:                                                                                                                                                                                                      \
        enum { TYPE_ID = TYPEID };                                                                                                                                                                               \
        CLASSNAME()                                                                                                                                                                                              \
            : AttributeWidgetCreator()      { mInitialMinValue = nullptr; mInitialMaxValue = nullptr; }                                                                                                          \
        ~CLASSNAME() {}                                                                                                                                                                                          \
        uint32 GetType() const                      { return TYPEID; };                                                                                                                                          \
        uint32 GetDefaultType() const               { return DEFAULTYPEID; };                                                                                                                                    \
        uint32 GetMinMaxType() const                { return MINMAXTYPEID; };                                                                                                                                    \
        uint32 GetDataType() const                  { return DATATYPEID; };                                                                                                                                      \
        bool CanBeParameter() const                 { return CANBEPARAMETER; }                                                                                                                                   \
        const char* GetTypeString() const           { return TYPESTRING; };                                                                                                                                      \
        bool GetHasMinMaxValues() const             { return HASMINMAXVALUES; }                                                                                                                                  \
        void InitAttributes(MCore::Array<MCore::Attribute*>&attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes = false, bool resetMinMaxAttributes = true);                 \
        AttributeWidget* Clone(MCore::Array<MCore::Attribute*>&attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction&func) \
        {                                                                                                                                                                                                        \
            return new ATTRIBUTEWIDGETCLASSNAME(attributes, attributeSettings, customData, readOnly, creationMode, func);                                                                                        \
        }                                                                                                                                                                                                        \
    };

    DEFINE_ATTRIBUTEWIDGETCREATOR(CheckboxAttributeWidgetCreator,      CheckBoxAttributeWidget,        MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX,        MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX,        MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX,        MCore::AttributeFloat::TYPE_ID,     false,  true,   "Checkbox");
    DEFINE_ATTRIBUTEWIDGETCREATOR(FloatSpinnerAttributeWidgetCreator,  FloatSpinnerAttributeWidget,    MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER,    MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER,    MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER,    MCore::AttributeFloat::TYPE_ID,     true,   true,   "FloatSpinner");
    DEFINE_ATTRIBUTEWIDGETCREATOR(IntSpinnerAttributeWidgetCreator,    IntSpinnerAttributeWidget,      MCore::ATTRIBUTE_INTERFACETYPE_INTSPINNER,      MCore::ATTRIBUTE_INTERFACETYPE_INTSPINNER,      MCore::ATTRIBUTE_INTERFACETYPE_INTSPINNER,      MCore::AttributeFloat::TYPE_ID,     true,   true,   "IntSpinner");
    DEFINE_ATTRIBUTEWIDGETCREATOR(FloatSliderAttributeWidgetCreator,   FloatSliderAttributeWidget,     MCore::ATTRIBUTE_INTERFACETYPE_FLOATSLIDER,     MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER,    MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER,    MCore::AttributeFloat::TYPE_ID,     true,   true,   "FloatSlider");
    DEFINE_ATTRIBUTEWIDGETCREATOR(IntSliderAttributeWidgetCreator,     IntSliderAttributeWidget,       MCore::ATTRIBUTE_INTERFACETYPE_INTSLIDER,       MCore::ATTRIBUTE_INTERFACETYPE_INTSPINNER,      MCore::ATTRIBUTE_INTERFACETYPE_INTSPINNER,      MCore::AttributeFloat::TYPE_ID,     true,   true,   "IntSlider");
    DEFINE_ATTRIBUTEWIDGETCREATOR(ComboBoxAttributeWidgetCreator,      ComboBoxAttributeWidget,        MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX,        MCore::ATTRIBUTE_INTERFACETYPE_INTSPINNER,      MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX,        MCore::AttributeFloat::TYPE_ID,     false,  false,  "ComboBox");
    DEFINE_ATTRIBUTEWIDGETCREATOR(Vector2AttributeWidgetCreator,       Vector2AttributeWidget,         MCore::ATTRIBUTE_INTERFACETYPE_VECTOR2,         MCore::ATTRIBUTE_INTERFACETYPE_VECTOR2,         MCore::ATTRIBUTE_INTERFACETYPE_VECTOR2,         MCore::AttributeVector2::TYPE_ID,   true,   true,   "Vector2");
    DEFINE_ATTRIBUTEWIDGETCREATOR(Vector3AttributeWidgetCreator,       Vector3AttributeWidget,         MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3,         MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3,         MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3,         MCore::AttributeVector3::TYPE_ID,   true,   true,   "Vector3NoGizmo");
    DEFINE_ATTRIBUTEWIDGETCREATOR(Vector4AttributeWidgetCreator,       Vector4AttributeWidget,         MCore::ATTRIBUTE_INTERFACETYPE_VECTOR4,         MCore::ATTRIBUTE_INTERFACETYPE_VECTOR4,         MCore::ATTRIBUTE_INTERFACETYPE_VECTOR4,         MCore::AttributeVector4::TYPE_ID,   true,   true,   "Vector4");
    DEFINE_ATTRIBUTEWIDGETCREATOR(StringAttributeWidgetCreator,        StringAttributeWidget,          MCore::ATTRIBUTE_INTERFACETYPE_STRING,          MCore::ATTRIBUTE_INTERFACETYPE_STRING,          MCore::ATTRIBUTE_INTERFACETYPE_STRING,          MCore::AttributeString::TYPE_ID,    false,  true,   "String");
    DEFINE_ATTRIBUTEWIDGETCREATOR(ColorAttributeWidgetCreator,         ColorAttributeWidget,           MCore::ATTRIBUTE_INTERFACETYPE_COLOR,           MCore::ATTRIBUTE_INTERFACETYPE_COLOR,           MCore::ATTRIBUTE_INTERFACETYPE_COLOR,           MCore::AttributeColor::TYPE_ID,     true,   true,   "Color");
    DEFINE_ATTRIBUTEWIDGETCREATOR(AttributeSetAttributeWidgetCreator,  AttributeSetAttributeWidget,    MCore::ATTRIBUTE_INTERFACETYPE_PROPERTYSET,     MCore::ATTRIBUTE_INTERFACETYPE_PROPERTYSET,     MCore::ATTRIBUTE_INTERFACETYPE_PROPERTYSET,     MCore::AttributeSet::TYPE_ID,       false,  false,  "AttributeSet");
} // namespace MysticQt


#endif
