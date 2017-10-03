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

#ifndef __MYSTICQT_INTSLIDER_H
#define __MYSTICQT_INTSLIDER_H

//
#include <MCore/Source/StandardHeaders.h>
#include "MysticQtConfig.h"
#include <QtWidgets/QWidget>
#include "IntSpinbox.h"
#include "DoubleSpinbox.h"
#include "Slider.h"


namespace MysticQt
{
    class MYSTICQT_API IntSlider
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(IntSlider, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS)

    public:
        /**
         * Constructor.
         */
        IntSlider(QWidget* parent = nullptr);

        /**
         * Destructor.
         */
        ~IntSlider();

        void SetEnabled(bool isEnabled);
        void SetValue(int value);
        void SetRange(int min, int max);
        void BlockSignals(bool flag);

        MCORE_INLINE int GetValue() const                           { return mSlider->value(); }
        MCORE_INLINE Slider* GetSlider() const                      { return mSlider; }
        MCORE_INLINE IntSpinBox* GetSpinBox() const                 { return mSpinBox; }

    signals:
        void ValueChanged(int value);
        void ValueChanged();
        void FinishedValueChange(int value);

    private slots:
        void OnSpinBoxChanged(int value);
        void OnSpinBoxEditingFinished(int value);
        void OnSliderChanged(int value);
        void OnSliderReleased();

    private:
        Slider*     mSlider;
        IntSpinBox* mSpinBox;
    };



    class MYSTICQT_API FloatSlider
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(FloatSlider, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS)

    public:
        /**
         * Constructor.
         */
        FloatSlider(QWidget* parent = nullptr);

        /**
         * Destructor.
         */
        ~FloatSlider();

        void SetEnabled(bool isEnabled);
        void SetValue(float value);
        void SetNormalizedValue(float value);
        void SetRange(float min, float max);
        void SetSingleStep(float delta)                             { mSpinBox->setSingleStep(delta); }
        void BlockSignals(bool flag);

        MCORE_INLINE float GetValue() const                         { return mSpinBox->value(); }
        MCORE_INLINE Slider* GetSlider() const                      { return mSlider; }
        MCORE_INLINE DoubleSpinBox* GetSpinBox() const              { return mSpinBox; }
        MCORE_INLINE bool GetIsPressed()                            { return mSlider->isSliderDown(); }

    signals:
        void ValueChanged(float value);
        void ValueChanged();
        void FinishedValueChange(float value);

    private slots:
        void OnSpinBoxChanged(double value);
        void OnSpinBoxEditingFinished(double value);
        void OnSliderChanged(int value);
        void OnSliderReleased();

    private:
        Slider*         mSlider;
        DoubleSpinBox*  mSpinBox;
    };
} // namespace MysticQt


#endif
