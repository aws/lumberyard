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
#include "StdAfx.h"
#include "PropertyDoubleSliderCtrl.hxx"
#include "DHQSpinbox.hxx"
#include "DHQSlider.hxx"
#include "PropertyQTConstants.h"
#include <QtWidgets/QHBoxLayout>
#include <AzCore/Math/MathUtils.h>
#include <QWheelEvent>

namespace AzToolsFramework
{
    /*
        Truncate qreal to int without flipping on overflow.
    */
    static inline double clampScrollStep(qreal x)
    {
        return double(qBound(qreal(std::numeric_limits<int>::lowest()), x, qreal(std::numeric_limits<int>::max())));
    }

    /*!
    Constructs an int slider.

    The \a parent argument is sent to the QWidget constructor.

    The \l minimum defaults to 0, the \l maximum to 99, with a \l
    singleStep size of 1 and a \l pageStep size of 10, and an initial
    \l value of 0.
    */
    DHPropertyDoubleSlider::DHPropertyDoubleSlider(QWidget* pParent)
        : QWidget(pParent, 0)
        , m_minimum(std::numeric_limits<int>::lowest())
        , m_maximum(std::numeric_limits<int>::max())
        , m_pageStep(0.1)
        , m_value(0.0)
        , m_position(0.0)
        , m_pressValue(-1.0)
        , m_singleStep(0.01)
        , m_softMinimum(0.0)
        , m_softMaximum(0.0)
        , m_offset_accumulated(0.0)
        , m_tracking(true)
        , m_blocktracking(false)
        , m_pressed(false)
        , m_invertedAppearance(false)
        , m_invertedControls(false)
        , m_useSoftMinimum(false)
        , m_useSoftMaximum(false)
        , m_orientation(Qt::Horizontal)
        , m_repeatAction(DHPropertyDoubleSlider::SliderNoAction)
#ifdef QT_KEYPAD_NAVIGATION
        , m_isAutoRepeating(false)
        , m_repeatMultiplier(1)
    {
        firstRepeat.invalidate();
#else
    {
#endif
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        m_pSlider = aznew DHQSlider(Qt::Horizontal, this);
        m_pSpinBox = aznew DHQDoubleSpinbox(this);
        m_pSpinBox->setKeyboardTracking(false);  // do not send valueChanged every time a character is typed
        setRange(0.0, 1.0);

        pLayout->setSpacing(4);
        pLayout->setContentsMargins(1, 0, 1, 0);


        pLayout->addWidget(m_pSpinBox);
        pLayout->addWidget(m_pSlider);

        InitializeSliderPropertyWidgets(m_pSlider, m_pSpinBox);
        setLayout(pLayout);

        setFocusPolicy(Qt::WheelFocus);
        setFocusProxy(m_pSpinBox);

        connect(m_pSlider, SIGNAL(valueChanged(int)), this, SLOT(onChildSliderValueChange(int)));
        connect(m_pSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onChildSpinboxValueChange(double)));
    }

    DHPropertyDoubleSlider::~DHPropertyDoubleSlider()
    {
    }

    double DHPropertyDoubleSlider::bound(double val) const
    {
        return qBound<double>(m_minimum, val, m_maximum);
    }

    double DHPropertyDoubleSlider::softBound(double val) const
    {
        double min = m_minimum;
        double max = m_maximum;

        if (m_useSoftMinimum)
        {
            min = m_softMinimum;
        }
        if (m_useSoftMaximum)
        {
            max = m_softMaximum;
        }
        return qBound<double>(min, val, max);
    }

    void DHPropertyDoubleSlider::setSteps(double single, double page)
    {
        m_singleStep = qAbs(single);
        m_pageStep = qAbs(page);
        sliderChange(DHPropertyDoubleSlider::SliderStepsChange);
    }

    bool DHPropertyDoubleSlider::scrollByDelta(Qt::Orientation orientation, Qt::KeyboardModifiers modifiers, int delta)
    {
        int stepsToScroll = 0;
        // in Qt scrolling to the right gives negative values.
        if (orientation == Qt::Horizontal)
        {
            delta = -delta;
        }
        qreal offset = qreal(delta) / 120;

        if ((modifiers& Qt::ControlModifier) || (modifiers & Qt::ShiftModifier))
        {
            // Scroll one page regardless of delta:
            stepsToScroll = (int)qBound(-m_pageStep, clampScrollStep(offset * m_pageStep), m_pageStep);
            m_offset_accumulated = 0;
        }
        else
        {
            // Calculate how many lines to scroll. Depending on what delta is (and
            // offset), we might end up with a fraction (e.g. scroll 1.3 lines). We can
            // only scroll whole lines, so we keep the reminder until next event.
            qreal stepsToScrollF =
#ifndef QT_NO_WHEELEVENT
                QApplication::wheelScrollLines() *
#endif
                offset * effectiveSingleStep();
            // Check if wheel changed direction since last event:
            if (m_offset_accumulated != 0 && (offset / m_offset_accumulated) < 0)
            {
                m_offset_accumulated = 0;
            }

            m_offset_accumulated += stepsToScrollF;
#ifndef Q_WS_MAC
            // Don't scroll more than one page in any case:
            stepsToScroll = (int)qBound(-m_pageStep, clampScrollStep(m_offset_accumulated), m_pageStep);
#else
            // Native UI-elements on Mac can scroll hundreds of lines at a time as
            // a result of acceleration. So keep the same behaviour in Qt, and
            // don't restrict stepsToScroll to certain maximum (pageStep):
            stepsToScroll = clampScrollStep(offset_accumulated);
#endif
            m_offset_accumulated -= clampScrollStep(m_offset_accumulated);
            if (stepsToScroll == 0)
            {
                return false;
            }
        }

        if (m_invertedControls)
        {
            stepsToScroll = -stepsToScroll;
        }

        double prevValue = m_value;
        m_position = overflowSafeAdd(stepsToScroll); // value will be updated by triggerAction()
        triggerAction(DHPropertyDoubleSlider::SliderMove);

        if (prevValue == m_value)
        {
            m_offset_accumulated = 0;
            return false;
        }
        return true;
    }

    QWidget* DHPropertyDoubleSlider::GetFirstInTabOrder()
    {
        return m_pSpinBox;
    }
    QWidget* DHPropertyDoubleSlider::GetLastInTabOrder()
    {
        return m_pSlider;
    }

    void DHPropertyDoubleSlider::UpdateTabOrder()
    {
        setTabOrder(m_pSpinBox, m_pSlider);
    }

    void DHPropertyDoubleSlider::focusInEvent(QFocusEvent* e)
    {
        QWidget::focusInEvent(e);
        m_pSpinBox->setFocus();
        m_pSpinBox->selectAll();
    }

    void DHPropertyDoubleSlider::sliderChange(SliderChange change)
    {
        m_pSlider->blockSignals(true);
        m_pSpinBox->blockSignals(true);

        if (change == SliderValueChange)
        {
            m_sliderCurrent = round(m_value / m_singleStep);
            m_pSlider->setValue(m_sliderCurrent);
            m_pSpinBox->setValue(m_value);
        }
        else if (change == SliderRangeChange || change == SliderStepsChange)
        {
            m_sliderMin = static_cast<int>(qMax((m_minimum / m_singleStep), static_cast<double>(std::numeric_limits<int>::lowest())));
            m_sliderMax = static_cast<int>(qMin((m_maximum / m_singleStep), static_cast<double>(std::numeric_limits<int>::max())));

            if (m_useSoftMinimum)
            {
                m_sliderMin = static_cast<int>(qMax((m_softMinimum / m_singleStep), (double)std::numeric_limits<int>::lowest()));
            }
            if (m_useSoftMaximum)
            {
                m_sliderMax = static_cast<int>(qMin((m_softMaximum / m_singleStep), (double)std::numeric_limits<int>::max()));
            }

            m_sliderCurrent = round(m_value / m_singleStep);
            m_pSlider->setRange(m_sliderMin, m_sliderMax);
            m_pSlider->setValue(m_sliderCurrent);
            m_pSpinBox->setRange(m_minimum, m_maximum);
            m_pSpinBox->setSingleStep(m_singleStep);
        }

        m_pSlider->blockSignals(false);
        m_pSpinBox->blockSignals(false);
    }

    void DHPropertyDoubleSlider::onChildSliderValueChange(int newValue)
    {
        double val = (newValue * m_singleStep);
        
        // eliminate floating point inconsistency
        val = AZ::ClampIfCloseMag(val, double(round(val)));

        setValue(val);
    }

    void DHPropertyDoubleSlider::onChildSpinboxValueChange(double newValue)
    {
        newValue = AZ::ClampIfCloseMag(newValue, double(round(newValue)));

        setValue(newValue);
    }

    void DHPropertyDoubleSlider::revertToDefaultClicked()
    {
        emit revertToDefault();
    }

    /*!
        Sets the slider's minimum to \a min and its maximum to \a max.

        If \a max is smaller than \a min, \a min becomes the only legal
        value.

        \sa minimum maximum
    */
    void DHPropertyDoubleSlider::setRange(double min, double max)
    {
        double oldMin = m_minimum;
        double oldMax = m_maximum;
        m_minimum = min;
        m_maximum = qMax(min, max);
        if (oldMin != m_minimum || oldMax != m_maximum)
        {
            emit rangeChanged(m_minimum, m_maximum);
            refreshUi();
        }
    }

    /*!
    Updates the slider and spin box to the new values.
    */
    void DHPropertyDoubleSlider::refreshUi()
    {
        m_pSlider->blockSignals(true);
        m_pSpinBox->blockSignals(true);
        sliderChange(SliderRangeChange);
        setValue(m_value); // re-bound
        m_pSlider->blockSignals(false);
        m_pSpinBox->blockSignals(false);
    }

    /*!
        \property DHPropertyDoubleSlider::orientation
        \brief the orientation of the slider

        The orientation must be \l Qt::Vertical (the default) or \l
        Qt::Horizontal.
    */
    void DHPropertyDoubleSlider::setOrientation(Qt::Orientation orientation)
    {
        if (m_orientation == orientation)
        {
            return;
        }

        m_orientation = orientation;
        if (!testAttribute(Qt::WA_WState_OwnSizePolicy))
        {
            QSizePolicy sp = sizePolicy();
            sp.transpose();
            setSizePolicy(sp);
            setAttribute(Qt::WA_WState_OwnSizePolicy, false);
        }
        update();
        updateGeometry();
    }

    /*!
        \property DHPropertyDoubleSlider::minimum
        \brief the sliders's minimum value

        When setting this property, the \l maximum is adjusted if
        necessary to ensure that the range remains valid. Also the
        slider's current value is adjusted to be within the new range.

    */
    void DHPropertyDoubleSlider::setMinimum(double min)
    {
        setRange(min, qMax(m_maximum, min));
    }

    /*!
        \property DHPropertyDoubleSlider::maximum
        \brief the slider's maximum value

        When setting this property, the \l minimum is adjusted if
        necessary to ensure that the range remains valid.  Also the
        slider's current value is adjusted to be within the new range.


    */
    void DHPropertyDoubleSlider::setMaximum(double max)
    {
        setRange(qMin(m_minimum, max), max);
    }

    /*!
        \property DHPropertyDoubleSlider::singleStep
        \brief the single step.

        The smaller of two natural steps that an
        abstract sliders provides and typically corresponds to the user
        pressing an arrow key.

        If the property is modified during an auto repeating key event, behavior
        is undefined.

        \sa pageStep
    */
    void DHPropertyDoubleSlider::setStep(double step)
    {
        if (qFuzzyCompare(step, m_singleStep))
        {
            setSteps(step, step * 10.0);
        }
    }

    /*!
    \property DHPropertyDoubleSlider::softMinimum
    \brief update the sliders' minimum soft value

    When setting this property, the \l sliders' range will be 
    limited to a minimum of this soft value. However the user 
    can still go below this number using the spin box.
    */
    void DHPropertyDoubleSlider::setSoftMinimum(double min)
    {
        if (min >= std::numeric_limits<int>::lowest() && (min != m_softMinimum || !m_useSoftMinimum))
        {
            m_useSoftMinimum = true;
            m_softMinimum = min;

            refreshUi();
        }
    }

    /*!
    \property DHPropertyDoubleSlider::softMaximum
    \brief update the sliders' maximum soft value

    When setting this property, the \l sliders' range will be
    limited to a maximum of this soft value. However the user
    can still go above this number using the spin box.
    */
    void DHPropertyDoubleSlider::setSoftMaximum(double max)
    {
        if (max <= std::numeric_limits<int>::max() && (max != m_softMaximum || !m_useSoftMaximum))
        {
            m_useSoftMaximum = true;
            m_softMaximum = max;

            refreshUi();
        }
    }

    /*!
        \property DHPropertyDoubleSlider::pageStep
        \brief the page step.

        The larger of two natural steps that an abstract slider provides
        and typically corresponds to the user pressing PageUp or PageDown.

        \sa singleStep
    */
    void DHPropertyDoubleSlider::setPageStep(double step)
    {
        if (step != m_pageStep)
        {
            setSteps(m_singleStep, step);
        }
    }

    /*!
        \property DHPropertyDoubleSlider::tracking
        \brief whether slider tracking is enabled

        If tracking is enabled (the default), the slider emits the
        valueChanged() signal while the slider is being dragged. If
        tracking is disabled, the slider emits the valueChanged() signal
        only when the user releases the slider.

        \sa sliderDown
    */
    void DHPropertyDoubleSlider::setTracking(bool enable)
    {
        m_tracking = enable;
    }

    /*!
        \property DHPropertyDoubleSlider::setDecimals
        \brief sets decimal precision on spin box control within the slider control.
    */
    void DHPropertyDoubleSlider::setDecimals(int decimals)
    {
        if (m_pSpinBox)
        {
            m_pSpinBox->setDecimals(decimals);
        }
    }

    /*!
    \property DHPropertyDoubleSlider::setDisplayDecimals
    \brief sets display decimal precision on spin box control within the slider control.
    */
    void DHPropertyDoubleSlider::setDisplayDecimals(int displayDecimals)
    {
        if (m_pSpinBox)
        {
            m_pSpinBox->SetDisplayDecimals(displayDecimals);
        }
    }

    /*!
        \property DHPropertyDoubleSlider::sliderDown
        \brief whether the slider is pressed down.

        The property is set by subclasses in order to let the abstract
        slider know whether or not \l tracking has any effect.

        Changing the slider down property emits the sliderPressed() and
        sliderReleased() signals.

    */
    void DHPropertyDoubleSlider::setSliderDown(bool down)
    {
        bool doEmit = m_pressed != down;

        m_pressed = down;

        if (doEmit)
        {
            if (down)
            {
                emit sliderPressed();
            }
            else
            {
                emit sliderReleased();
            }
        }

        if (!down && m_position != m_value)
        {
            triggerAction(SliderMove);
        }
    }

    /*!
        \property DHPropertyDoubleSlider::sliderPosition
        \brief the current slider position

        If \l tracking is enabled (the default), this is identical to \l value.
    */
    void DHPropertyDoubleSlider::setSliderPosition(double position)
    {
        position = softBound(position);
        if (position == m_position)
        {
            return;
        }
        m_position = position;
        if (!m_tracking)
        {
            update();
        }
        if (m_pressed)
        {
            emit sliderMoved(position);
        }
        if (m_tracking && !m_blocktracking)
        {
            triggerAction(SliderMove);
        }
    }

    /*!
        \property DHPropertyDoubleSlider::value
        \brief the slider's current value

        The slider forces the value to be within the legal range: \l
        minimum <= \c value <= \l maximum.

        Changing the value also changes the \l sliderPosition.
    */
    void DHPropertyDoubleSlider::setValue(double value)
    {
        value = bound(value);
        if (m_value == value && m_position == value)
        {
            return;
        }
        m_value = value;
        if (m_position != value)
        {
            m_position = value;
            if (m_pressed)
            {
                emit sliderMoved(value);
            }
        }
        sliderChange(SliderValueChange);
        emit valueChanged(value);
    }

    /*!
        \property DHPropertyDoubleSlider::invertedAppearance
        \brief whether or not a slider shows its values inverted.

        If this property is false (the default), the minimum and maximum will
        be shown in its classic position for the inherited widget. If the
        value is true, the minimum and maximum appear at their opposite location.

        Note: This property makes most sense for sliders and dials. For
        scroll bars, the visual effect of the scroll bar subcontrols depends on
        whether or not the styles understand inverted appearance; most styles
        ignore this property for scroll bars.
    */
    void DHPropertyDoubleSlider::setInvertedAppearance(bool invert)
    {
        m_invertedAppearance = invert;
        update();
    }

    /*!
        \property DHPropertyDoubleSlider::invertedControls
        \brief whether or not the slider inverts its wheel and key events.

        If this property is false, scrolling the mouse wheel "up" and using keys
        like page up will increase the slider's value towards its maximum. Otherwise
        pressing page up will move value towards the slider's minimum.
    */
    void DHPropertyDoubleSlider::setInvertedControls(bool invert)
    {
        m_invertedControls = invert;
    }

    /*!  Triggers a slider \a action.  Possible actions are \l
      SliderSingleStepAdd, \l SliderSingleStepSub, \l SliderPageStepAdd,
      \l SliderPageStepSub, \l SliderToMinimum, \l SliderToMaximum, and \l
      SliderMove.

      \sa actionTriggered()
     */
    void DHPropertyDoubleSlider::triggerAction(SliderAction action)
    {
        m_blocktracking = true;
        switch (action)
        {
        case SliderSingleStepAdd:
            setSliderPosition(overflowSafeAdd(effectiveSingleStep()));
            break;
        case SliderSingleStepSub:
            setSliderPosition(overflowSafeAdd(-effectiveSingleStep()));
            break;
        case SliderPageStepAdd:
            setSliderPosition(overflowSafeAdd(m_pageStep));
            break;
        case SliderPageStepSub:
            setSliderPosition(overflowSafeAdd(-m_pageStep));
            break;
        case SliderToMinimum:
            setSliderPosition(m_minimum);
            break;
        case SliderToMaximum:
            setSliderPosition(m_maximum);
            break;
        case SliderMove:
        case SliderNoAction:
            break;
        }
        ;
        emit actionTriggered(action);
        m_blocktracking = false;
        setValue(m_position);
    }

    /*!  Sets action \a action to be triggered repetitively in intervals
    of \a repeatTime, after an initial delay of \a thresholdTime.

    \sa triggerAction() repeatAction()
     */
    void DHPropertyDoubleSlider::setRepeatAction(SliderAction action, int thresholdTime, int repeatTime)
    {
        if ((m_repeatAction = action) == SliderNoAction)
        {
            m_repeatActionTimer.stop();
        }
        else
        {
            m_repeatActionTime = repeatTime;
            m_repeatActionTimer.start(thresholdTime, this);
        }
    }

    /*!
      Returns the current repeat action.
      \sa setRepeatAction()
     */
    void DHPropertyDoubleSlider::timerEvent(QTimerEvent* e)
    {
        if (e->timerId() == m_repeatActionTimer.timerId())
        {
            if (m_repeatActionTime)   // was threshold time, use repeat time next time
            {
                m_repeatActionTimer.start(m_repeatActionTime, this);
                m_repeatActionTime = 0;
            }
            if (m_repeatAction == SliderPageStepAdd)
            {
                setAdjustedSliderPosition(overflowSafeAdd(m_pageStep));
            }
            else if (m_repeatAction == SliderPageStepSub)
            {
                setAdjustedSliderPosition(overflowSafeAdd(-m_pageStep));
            }
            else
            {
                triggerAction(m_repeatAction);
            }
        }
    }

#ifndef QT_NO_WHEELEVENT
    void DHPropertyDoubleSlider::wheelEvent(QWheelEvent* e)
    {
        e->ignore();
        int delta = e->delta();
        if (scrollByDelta(e->orientation(), e->modifiers(), delta))
        {
            e->accept();
        }
    }
#endif

    void DHPropertyDoubleSlider::keyPressEvent(QKeyEvent* ev)
    {
        SliderAction action = SliderNoAction;
    #ifdef QT_KEYPAD_NAVIGATION
        if (ev->isAutoRepeat())
        {
            if (!m_firstRepeat.isValid())
            {
                m_firstRepeat.start();
            }
            else if (1 == m_repeatMultiplier)
            {
                // This is the interval in milli seconds which one key repetition
                // takes.
                const int repeatMSecs = m_firstRepeat.elapsed();

                /**
                 * The time it takes to currently navigate the whole slider.
                 */
                const qreal currentTimeElapse = (qreal(maximum()) / singleStep()) * repeatMSecs;

                /**
                 * This is an arbitrarily determined constant in msecs that
                 * specifies how long time it should take to navigate from the
                 * start to the end(excluding starting key auto repeat).
                 */
                const int SliderRepeatElapse = 2500;

                m_repeatMultiplier = currentTimeElapse / SliderRepeatElapse;
            }
        }
        else if (m_firstRepeat.isValid())
        {
            m_firstRepeat.invalidate();
            m_repeatMultiplier = 1;
        }

    #endif

        switch (ev->key())
        {
    #ifdef QT_KEYPAD_NAVIGATION
        case Qt::Key_Select:
            if (QApplication::keypadNavigationEnabled())
            {
                setEditFocus(!hasEditFocus());
            }
            else
            {
                ev->ignore();
            }
            break;
        case Qt::Key_Back:
            if (QApplication::keypadNavigationEnabled() && hasEditFocus())
            {
                setValue(m_origValue);
                setEditFocus(false);
            }
            else
            {
                ev->ignore();
            }
            break;
    #endif

        // It seems we need to use invertedAppearance for Left and right, otherwise, things look weird.
        case Qt::Key_Left:
    #ifdef QT_KEYPAD_NAVIGATION
            // In QApplication::KeypadNavigationDirectional, we want to change the slider
            // value if there is no left/right navigation possible and if this slider is not
            // inside a tab widget.
            if (QApplication::keypadNavigationEnabled()
                && (!hasEditFocus() && QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder
                    || d->orientation == Qt::Vertical
                    || !hasEditFocus()
                    && (QWidgetPrivate::canKeypadNavigate(Qt::Horizontal) || QWidgetPrivate::inTabWidget(this))))
            {
                ev->ignore();
                return;
            }
            if (QApplication::keypadNavigationEnabled() && m_orientation == Qt::Vertical)
            {
                action = m_invertedControls ? SliderSingleStepSub : SliderSingleStepAdd;
            }
            else
    #endif
            if (isRightToLeft())
            {
                action = m_invertedAppearance ? SliderSingleStepSub : SliderSingleStepAdd;
            }
            else
            {
                action = !m_invertedAppearance ? SliderSingleStepSub : SliderSingleStepAdd;
            }
            break;
        case Qt::Key_Right:
    #ifdef QT_KEYPAD_NAVIGATION
            // Same logic as in Qt::Key_Left
            if (QApplication::keypadNavigationEnabled()
                && (!hasEditFocus() && QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder
                    || m_orientation == Qt::Vertical
                    || !hasEditFocus()
                    && (QWidgetPrivate::canKeypadNavigate(Qt::Horizontal) || QWidgetPrivate::inTabWidget(this))))
            {
                ev->ignore();
                return;
            }
            if (QApplication::keypadNavigationEnabled() && m_orientation == Qt::Vertical)
            {
                action = m_invertedControls ? SliderSingleStepAdd : SliderSingleStepSub;
            }
            else
    #endif
            if (isRightToLeft())
            {
                action = m_invertedAppearance ? SliderSingleStepAdd : SliderSingleStepSub;
            }
            else
            {
                action = !m_invertedAppearance ? SliderSingleStepAdd : SliderSingleStepSub;
            }
            break;
        case Qt::Key_Up:
    #ifdef QT_KEYPAD_NAVIGATION
            // In QApplication::KeypadNavigationDirectional, we want to change the slider
            // value if there is no up/down navigation possible.
            if (QApplication::keypadNavigationEnabled()
                && (QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder
                    || m_orientation == Qt::Horizontal
                    || !hasEditFocus() && QWidgetPrivate::canKeypadNavigate(Qt::Vertical)))
            {
                ev->ignore();
                break;
            }
    #endif
            action = m_invertedControls ? SliderSingleStepSub : SliderSingleStepAdd;
            break;
        case Qt::Key_Down:
    #ifdef QT_KEYPAD_NAVIGATION
            // Same logic as in Qt::Key_Up
            if (QApplication::keypadNavigationEnabled()
                && (QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder
                    || d->orientation == Qt::Horizontal
                    || !hasEditFocus() && QWidgetPrivate::canKeypadNavigate(Qt::Vertical)))
            {
                ev->ignore();
                break;
            }
    #endif
            action = m_invertedControls ? SliderSingleStepAdd : SliderSingleStepSub;
            break;
        case Qt::Key_PageUp:
            action = m_invertedControls ? SliderPageStepSub : SliderPageStepAdd;
            break;
        case Qt::Key_PageDown:
            action = m_invertedControls ? SliderPageStepAdd : SliderPageStepSub;
            break;
        case Qt::Key_Home:
            action = SliderToMinimum;
            break;
        case Qt::Key_End:
            action = SliderToMaximum;
            break;
        default:
            ev->ignore();
            break;
        }
        if (action)
        {
            triggerAction(action);
        }
    }

    void DHPropertyDoubleSlider::changeEvent(QEvent* ev)
    {
        switch (ev->type())
        {
        case QEvent::EnabledChange:
            if (!isEnabled())
            {
                m_repeatActionTimer.stop();
                setSliderDown(false);
            }
        // fall through...
        default:
            QWidget::changeEvent(ev);
        }
    }

    bool DHPropertyDoubleSlider::event(QEvent* e)
    {
#ifdef QT_KEYPAD_NAVIGATION
        switch (e->type())
        {
        case QEvent::FocusIn:
            m_origValue = m_value;
            break;
        default:
            break;
        }
#endif
        return QWidget::event(e);
    }


    // a common function to eat attribs, for all int handlers:
    void doublePropertySliderHandler::ConsumeAttributeCommon(DHPropertyDoubleSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        GUI->blockSignals(true);
        (void)debugName;
        double value;
        if (attrib == AZ::Edit::Attributes::Min)
        {
            if (attrValue->Read<double>(value))
            {
                GUI->setMinimum(value);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Min' attribute from property '%s' into Slider", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Max)
        {
            if (attrValue->Read<double>(value))
            {
                GUI->setMaximum(value);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Max' attribute from property '%s' into Slider", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Step)
        {
            if (attrValue->Read<double>(value))
            {
                GUI->setStep(value);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Step' attribute from property '%s' into Slider", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::SoftMin)
        {
            if (attrValue->Read<double>(value))
            {
                GUI->setSoftMinimum(value);
            }
        }
        else if (attrib == AZ::Edit::Attributes::SoftMax)
        {
            if (attrValue->Read<double>(value))
            {
                GUI->setSoftMaximum(value);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Decimals)
        {
            int intValue = 0;
            if (attrValue->Read<int>(intValue))
            {
                GUI->setDecimals(intValue);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Decimals' attribute from property '%s' into Slider", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::DisplayDecimals)
        {
            int intValue = 0;
            if (attrValue->Read<int>(intValue))
            {
                GUI->setDisplayDecimals(intValue);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'DisplayDecimals' attribute from property '%s' into Slider", debugName);
            }
        }

        // Verify that the bounds are within the acceptable range of the int slider.
        const double max = GUI->maximum() / GUI->effectiveSingleStep();
        const double min = GUI->minimum() / GUI->effectiveSingleStep();
        if (max > std::numeric_limits<int>::max() || max < std::numeric_limits<int>::lowest() || min > std::numeric_limits<int>::max() || min < std::numeric_limits<int>::lowest())
        {
            AZ_WarningOnce("AzToolsFramework", false, "Property '%s' : 0x%08x  in double/float Slider exceeds Min/Max values", debugName, attrib);
        }
        GUI->blockSignals(false);
    }


    QWidget* doublePropertySliderHandler::CreateGUI(QWidget* pParent)
    {
        DHPropertyDoubleSlider* newCtrl = aznew DHPropertyDoubleSlider(pParent);
        connect(newCtrl, &DHPropertyDoubleSlider::valueChanged, this, [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        // set defaults:
        newCtrl->setRange(0, 1);

        return newCtrl;
    }

    QWidget* floatPropertySliderHandler::CreateGUI(QWidget* pParent)
    {
        DHPropertyDoubleSlider* newCtrl = aznew DHPropertyDoubleSlider(pParent);
        connect(newCtrl, &DHPropertyDoubleSlider::valueChanged, this, [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        // set defaults:
        newCtrl->setRange(0, 1);

        return newCtrl;
    }

    void doublePropertySliderHandler::ConsumeAttribute(DHPropertyDoubleSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        doublePropertySliderHandler::ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);
    }

    void floatPropertySliderHandler::ConsumeAttribute(DHPropertyDoubleSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        doublePropertySliderHandler::ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);
    }

    void doublePropertySliderHandler::WriteGUIValuesIntoProperty(size_t index, DHPropertyDoubleSlider* GUI, property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        double val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    void floatPropertySliderHandler::WriteGUIValuesIntoProperty(size_t index, DHPropertyDoubleSlider* GUI, property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        double val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    bool doublePropertySliderHandler::ReadValuesIntoGUI(size_t index, DHPropertyDoubleSlider* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        GUI->blockSignals(true);
        GUI->setValue(instance);
        GUI->blockSignals(false);
        return false;
    }

    bool floatPropertySliderHandler::ReadValuesIntoGUI(size_t index, DHPropertyDoubleSlider* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        GUI->blockSignals(true);
        GUI->setValue(instance);
        GUI->blockSignals(false);
        return false;
    }

    bool doublePropertySliderHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        DHPropertyDoubleSlider* propertyControl = qobject_cast<DHPropertyDoubleSlider*>(widget);
        AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
        if (propertyControl)
        {
            if (!toolTipString.isEmpty())
            {
                toolTipString += "\n";
            }
            toolTipString += "[";
            if (propertyControl->minimum() <= std::numeric_limits<int>::lowest())
            {
                toolTipString += "-" + tr(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->minimum());
            }
            toolTipString += ", ";
            if (propertyControl->maximum() >= std::numeric_limits<int>::max())
            {
                toolTipString += tr(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->maximum());
            }
            toolTipString += "]";
            return true;
        }
        return false;
    }

    bool floatPropertySliderHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        DHPropertyDoubleSlider* propertyControl = qobject_cast<DHPropertyDoubleSlider*>(widget);
        AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
        if (propertyControl)
        {
            if (!toolTipString.isEmpty())
            {
                toolTipString += "\n";
            }
            toolTipString += "[";
            if (propertyControl->minimum() <= std::numeric_limits<int>::lowest())
            {
                toolTipString += "-" + tr(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->minimum());
            }
            toolTipString += ", ";
            if (propertyControl->maximum() >= std::numeric_limits<int>::max())
            {
                toolTipString += tr(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->maximum());
            }
            toolTipString += "]";
            return true;
        }
        return false;
    }

    void RegisterDoubleSliderHandlers()
    {
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew doublePropertySliderHandler());
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew floatPropertySliderHandler());
    }
}

#include <UI/PropertyEditor/PropertyDoubleSliderCtrl.moc>
