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

#ifndef __MYSTICQT_ATTRIBUTEWIDGETS_H
#define __MYSTICQT_ATTRIBUTEWIDGETS_H

// include required headers
#include "MysticQtConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/Attribute.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeInt32.h>
#include <MCore/Source/AttributeFloat.h>
#include <MCore/Source/AttributeString.h>
#include <MCore/Source/AttributeArray.h>
#include <MCore/Source/AttributeColor.h>
#include <MCore/Source/AttributeVector2.h>
#include <MCore/Source/AttributeVector3.h>
#include <MCore/Source/AttributeVector4.h>
#include <MCore/Source/AttributeSettings.h>
#include <MCore/Source/AttributeSet.h>
#include "DoubleSpinbox.h"
#include "IntSpinbox.h"
#include "Slider.h"
#include "ColorLabel.h"
#include "LinkWidget.h"
#include "ComboBox.h"
#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>


namespace MysticQt
{
    class PropertyWidget;

    typedef AZStd::function<void(MCore::Attribute* attribute, MCore::AttributeSettings* settings)>    AttributeChangedFunction;


    class MYSTICQT_API AttributeWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(AttributeWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_ATTRIBUTEWIDGETS);

    public:
        AttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func);
        virtual ~AttributeWidget();

        // called when an attribute inside the widget got changed, this internally calls the callbacks from the attribute widget factory
        void UpdateInterface();

        void CreateStandardLayout(QWidget* widget, MCore::AttributeSettings* attributeSettings);
        void CreateStandardLayout(QWidget* widget, const char* description);
        void CreateStandardLayout(QWidget* widgetA, QWidget* widgetB, MCore::AttributeSettings* attributeSettings);

        void SetCreationMode(bool creationMode)                 { mCreationMode = creationMode; }
        bool GetCreationMode()                                  { return mCreationMode; }
        MCore::AttributeSettings* GetAttributeInfo()            { return mAttributeSettings; }
        virtual QWidget* CreateGizmoWidget()                    { return nullptr; }

        virtual void SetValue(MCore::Attribute* attribute)      { MCORE_UNUSED(attribute); }
        virtual void SetReadOnly(bool readOnly)                 { MCORE_UNUSED(readOnly); }
        virtual void EnableWidgets(bool enabled)                { setDisabled(!enabled); }

    signals:
        void RequestParentReInit();
        void ValueChanged();

    protected:
        void FireValueChangedSignal()                           { emit ValueChanged(); }
        MCore::Array<MCore::Attribute*>         mAttributes;
        MCore::Attribute*                       mFirstAttribute;
        MCore::AttributeSettings*               mAttributeSettings;
        AttributeChangedFunction                mAttribChangedFunc;
        bool                                    mReadOnly;
        bool                                    mCreationMode;
        void*                                   mCustomData;
        bool                                    mNeedsHeavyUpdate;
        bool                                    mNeedsObjectUpdate;
        bool                                    mNeedsAttributeWindowUpdate;
    };


    // checkbox attribute widget
    class CheckBoxAttributeWidget
        : public AttributeWidget
    {
        Q_OBJECT
    public:
        CheckBoxAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func);
        void SetValue(MCore::Attribute* attribute) override;
        void SetReadOnly(bool readOnly) override;

    public slots:
        void OnCheckBox(int state);

    private:
        QCheckBox* mCheckBox;
    };


    // float spinner attribute widget
    class FloatSpinnerAttributeWidget
        : public AttributeWidget
    {
        Q_OBJECT
    public:
        FloatSpinnerAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func);
        void SetValue(MCore::Attribute* attribute) override;
        void SetReadOnly(bool readOnly) override;

    protected slots:
        void OnDoubleSpinner(double value);

    private:
        MysticQt::DoubleSpinBox* mSpinBox;
    };


    // int spinner attribute widget
    class IntSpinnerAttributeWidget
        : public AttributeWidget
    {
        Q_OBJECT
    public:
        IntSpinnerAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func);
        void SetValue(MCore::Attribute* attribute) override;
        void SetReadOnly(bool readOnly) override;

    protected slots:
        void OnIntSpinner(int value);

    private:
        MysticQt::IntSpinBox* mSpinBox;
    };


    // float slider attribute widget
    class FloatSliderAttributeWidget
        : public AttributeWidget
    {
        Q_OBJECT
    public:
        FloatSliderAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func);
        void SetValue(MCore::Attribute* attribute) override;
        void SetReadOnly(bool readOnly) override;

    protected slots:
        void OnFloatSlider(int value);

    private:
        MysticQt::Slider* mSlider;
    };


    // int slider attribute widget
    class IntSliderAttributeWidget
        : public AttributeWidget
    {
        Q_OBJECT
    public:
        IntSliderAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func);
        void SetValue(MCore::Attribute* attribute) override;
        void SetReadOnly(bool readOnly) override;

    protected slots:
        void OnIntSlider(int value);

    private:
        MysticQt::Slider* mSlider;
    };


    // combo box attribute widget
    class ComboBoxAttributeWidget
        : public AttributeWidget
    {
        Q_OBJECT
    public:
        ComboBoxAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func);
        void SetValue(MCore::Attribute* attribute) override;
        void SetReadOnly(bool readOnly) override;

    protected slots:
        void OnComboBox(int value);

    private:
        MysticQt::ComboBox* mComboBox;
    };


    // vector2 attribute widget
    class Vector2AttributeWidget
        : public AttributeWidget
    {
        Q_OBJECT
    public:
        Vector2AttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func);
        void SetValue(MCore::Attribute* attribute) override;
        void SetReadOnly(bool readOnly) override;

    protected slots:
        void OnDoubleSpinnerX(double value);
        void OnDoubleSpinnerY(double value);

    private:
        MysticQt::DoubleSpinBox*    mSpinBoxX;
        MysticQt::DoubleSpinBox*    mSpinBoxY;
    };


    // vector3 attribute widget
    class Vector3AttributeWidget
        : public AttributeWidget
    {
        Q_OBJECT
    public:
        Vector3AttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func);
        void SetValue(MCore::Attribute* attribute) override;
        void SetReadOnly(bool readOnly) override;

    protected slots:
        void OnDoubleSpinnerX(double value);
        void OnDoubleSpinnerY(double value);
        void OnDoubleSpinnerZ(double value);

    private:
        MysticQt::DoubleSpinBox*    mSpinBoxX;
        MysticQt::DoubleSpinBox*    mSpinBoxY;
        MysticQt::DoubleSpinBox*    mSpinBoxZ;
    };


    // vector4 attribute widget
    class Vector4AttributeWidget
        : public AttributeWidget
    {
        Q_OBJECT
    public:
        Vector4AttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func);
        void SetValue(MCore::Attribute* attribute) override;
        void SetReadOnly(bool readOnly) override;

    protected slots:
        void OnDoubleSpinnerX(double value);
        void OnDoubleSpinnerY(double value);
        void OnDoubleSpinnerZ(double value);
        void OnDoubleSpinnerW(double value);

    private:
        MysticQt::DoubleSpinBox*    mSpinBoxX;
        MysticQt::DoubleSpinBox*    mSpinBoxY;
        MysticQt::DoubleSpinBox*    mSpinBoxZ;
        MysticQt::DoubleSpinBox*    mSpinBoxW;
    };


    // string attribute widget
    class StringAttributeWidget
        : public AttributeWidget
    {
        Q_OBJECT
    public:
        StringAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func);
        void SetValue(MCore::Attribute* attribute) override;
        void SetReadOnly(bool readOnly) override;

    protected slots:
        void OnStringChange();

    private:
        QLineEdit*  mLineEdit;
    };


    // color attribute widget
    class ColorAttributeWidget
        : public AttributeWidget
    {
        Q_OBJECT
    public:
        ColorAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func);
        void SetValue(MCore::Attribute* attribute) override;
        void SetReadOnly(bool readOnly) override;

    protected slots:
        void OnColorChanged();

    private:
        ColorLabel* mColorLabel;
    };


    // attribute set widget
    class AttributeSetAttributeWidget
        : public AttributeWidget
    {
        Q_OBJECT
    public:
        AttributeSetAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func);
        void SetValue(MCore::Attribute* attribute) override;
        void SetReadOnly(bool readOnly) override;

    protected slots:

    private:
        PropertyWidget* mPropertyWidget;
    };
} // namespace MysticQt


#endif
