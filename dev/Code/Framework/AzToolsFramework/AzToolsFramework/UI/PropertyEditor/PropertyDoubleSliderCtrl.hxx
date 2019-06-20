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

#ifndef PROPERTY_DOUBLESLIDER_CTRL
#define PROPERTY_DOUBLESLIDER_CTRL

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include "PropertyEditorAPI.h"

#include <QAbstractSlider>
#include <QBasicTimer>
#include <QStyle>
#include <QWidget>

#pragma once

class QSlider;
class QToolButton;

namespace AzToolsFramework
{
    class DHQDoubleSpinbox;

    class DHPropertyDoubleSlider
        : public QWidget
    {
        Q_OBJECT

        Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
        Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
        Q_PROPERTY(double pageStep READ pageStep WRITE setPageStep)
        Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged USER true)
        Q_PROPERTY(double sliderPosition READ sliderPosition WRITE setSliderPosition NOTIFY sliderMoved)
        Q_PROPERTY(bool tracking READ hasTracking WRITE setTracking)
        Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
        Q_PROPERTY(bool invertedAppearance READ invertedAppearance WRITE setInvertedAppearance)
        Q_PROPERTY(bool invertedControls READ invertedControls WRITE setInvertedControls)
        Q_PROPERTY(bool sliderDown READ isSliderDown WRITE setSliderDown DESIGNABLE false)
        Q_PROPERTY(double softMinimum READ softMinimum WRITE setSoftMinimum)
        Q_PROPERTY(double softMaximum READ softMaximum WRITE setSoftMaximum)

    public:
        enum SliderAction
        {
            SliderNoAction,
            SliderSingleStepAdd,
            SliderSingleStepSub,
            SliderPageStepAdd,
            SliderPageStepSub,
            SliderToMinimum,
            SliderToMaximum,
            SliderMove
        };

        enum SliderChange
        {
            SliderRangeChange,
            SliderOrientationChange,
            SliderStepsChange,
            SliderValueChange
        };

    public:
        AZ_CLASS_ALLOCATOR(DHPropertyDoubleSlider, AZ::SystemAllocator, 0);

        DHPropertyDoubleSlider(QWidget* pParent = NULL);
        virtual ~DHPropertyDoubleSlider();

    private:
        QSlider* m_pSlider;
        int m_sliderMin;
        int m_sliderMax;
        int m_sliderCurrent;
        DHQDoubleSpinbox* m_pSpinBox;
        double m_minimum;
        double m_maximum;
        double m_pageStep;
        double m_softMinimum;
        double m_softMaximum;
        double m_value;
        double m_position;
        double m_pressValue;
        double m_singleStep;
        double m_offset_accumulated;
        double m_curveMidpoint;
        bool  m_tracking;
        bool  m_blocktracking;
        bool  m_pressed;
        bool  m_invertedAppearance;
        bool  m_invertedControls;
        bool m_useSoftMinimum;
        bool m_useSoftMaximum;
        Qt::Orientation m_orientation;
        QBasicTimer m_repeatActionTimer;
        int m_repeatActionTime;
        SliderAction m_repeatAction;
#ifdef QT_KEYPAD_NAVIGATION
        int m_origValue;
        bool m_isAutoRepeating;
        qreal m_repeatMultiplier;
        QElapsedTimer m_firstRepeat;
#endif

    public:
        inline double effectiveSingleStep() const
        {
            return m_singleStep
#ifdef QT_KEYPAD_NAVIGATION
                   * m_repeatMultiplier
#endif
            ;
        }

        void setSteps(double single, double page);

        virtual double bound(double val) const;
        virtual double softBound(double val) const;

        inline double overflowSafeAdd(double add) const
        {
            double newValue = m_value + add;
            if (add > 0 && newValue < m_value)
            {
                newValue = m_maximum;
            }
            else if (add < 0 && newValue > m_value)
            {
                newValue = m_minimum;
            }
            return newValue;
        }

        inline void setAdjustedSliderPosition(double position)
        {
            if (style()->styleHint(QStyle::SH_Slider_StopMouseOverSlider, 0, this))
            {
                if ((position > m_pressValue - 2 * m_pageStep) && (position < m_pressValue + 2 * m_pageStep))
                {
                    m_repeatAction = DHPropertyDoubleSlider::SliderNoAction;
                    setSliderPosition(m_pressValue);
                    return;
                }
            }
            triggerAction(m_repeatAction);
        }

        bool scrollByDelta(Qt::Orientation orientation, Qt::KeyboardModifiers modifiers, int delta);

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

        Qt::Orientation orientation() const { return m_orientation; }

        void setMinimum(double);
        double minimum() const { return m_minimum; }

        void setMaximum(double);
        double maximum() const { return m_maximum; }

        void setRange(double min, double max);

        void setStep(double);
        double step() const { return m_singleStep; }

        void setSoftMinimum(double);
        double softMinimum() const { return m_softMinimum; }

        void setSoftMaximum(double);
        double softMaximum() const { return m_softMaximum; }

        void setPageStep(double);
        double pageStep() const { return m_pageStep; }

        void setTracking(bool enable);
        bool hasTracking() const { return m_tracking; }
        void setDecimals(int decimals);
        void setDisplayDecimals(int displayDecimals);

        void setCurveMidpoint(double midpoint);

        void setSliderDown(bool);
        bool isSliderDown() const { return m_pressed; }

        void setSliderPosition(double);
        double sliderPosition() const { return m_position; }

        void setInvertedAppearance(bool);
        bool invertedAppearance() const { return m_invertedAppearance; }

        void setInvertedControls(bool);
        bool invertedControls() const { return m_invertedControls; }

        double value() const { return m_value;  }

        void triggerAction(SliderAction action);

    signals:
        void revertToDefault();

        void valueChanged(double value);

        void sliderPressed();
        void sliderMoved(double position);
        void sliderReleased();

        void rangeChanged(double min, double max);

        void actionTriggered(int action);

    protected slots:
        void onChildSliderValueChange(int newValue);
        void onChildSpinboxValueChange(double newValue);
        void revertToDefaultClicked();

    public slots:
        void setValue(double);
        void setOrientation(Qt::Orientation);

    protected:
        virtual void focusInEvent(QFocusEvent* e);

        bool event(QEvent* e);

        void setRepeatAction(SliderAction action, int thresholdTime = 500, int repeatTime = 50);
        SliderAction repeatAction() const { return m_repeatAction; }

        virtual void sliderChange(SliderChange change);

        void keyPressEvent(QKeyEvent* ev);
        void timerEvent(QTimerEvent*);
#ifndef QT_NO_WHEELEVENT
        void wheelEvent(QWheelEvent* e);
#endif
        void changeEvent(QEvent* e);

        double ConvertToSliderValue(double value);
        double ConvertFromSliderValue(double value);
        double ConvertPowerCurveValue(double value, bool fromSlider);

#ifdef QT3_SUPPORT
    public:
        inline QT3_SUPPORT double minValue() const { return minimum(); }
        inline QT3_SUPPORT double maxValue() const { return maximum(); }
        inline QT3_SUPPORT double lineStep() const { return singleStep(); }
        inline QT3_SUPPORT void setMinValue(double v) { setMinimum(v); }
        inline QT3_SUPPORT void setMaxValue(double v) { setMaximum(v); }
        inline QT3_SUPPORT void setLineStep(double v) { setSingleStep(v); }
        inline QT3_SUPPORT void setSteps(double single, double page) { setSingleStep(single); setPageStep(page); }
        inline QT3_SUPPORT void addPage() { triggerAction(SliderPageStepAdd); }
        inline QT3_SUPPORT void subtractPage() { triggerAction(SliderPageStepSub); }
        inline QT3_SUPPORT void addLine() { triggerAction(SliderSingleStepAdd); }
        inline QT3_SUPPORT void subtractLine() { triggerAction(SliderSingleStepSub); }
#endif

    private:
        Q_DISABLE_COPY(DHPropertyDoubleSlider)

            void refreshUi();
    };

    template <class ValueType>
    class DoubleSliderHandlerCommon
        : public PropertyHandler<ValueType, DHPropertyDoubleSlider>
    {
        AZ::u32 GetHandlerName(void) const override  { return AZ::Edit::UIHandlers::Slider; }
        QWidget* GetFirstInTabOrder(DHPropertyDoubleSlider* widget) override { return widget->GetFirstInTabOrder(); }
        QWidget* GetLastInTabOrder(DHPropertyDoubleSlider* widget) override { return widget->GetLastInTabOrder(); }
        void UpdateWidgetInternalTabbing(DHPropertyDoubleSlider* widget) override { widget->UpdateTabOrder(); }
    };



    class doublePropertySliderHandler
        : QObject
        , public DoubleSliderHandlerCommon<double>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(doublePropertySliderHandler, AZ::SystemAllocator, 0);

        // common to all double sliders
        static void ConsumeAttributeCommon(DHPropertyDoubleSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName);

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(DHPropertyDoubleSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, DHPropertyDoubleSlider* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, DHPropertyDoubleSlider* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };

    class floatPropertySliderHandler
        : QObject
        , public DoubleSliderHandlerCommon<float>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(floatPropertySliderHandler, AZ::SystemAllocator, 0);

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(DHPropertyDoubleSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, DHPropertyDoubleSlider* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, DHPropertyDoubleSlider* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };


    void RegisterDoubleSliderHandlers();
}

#endif
