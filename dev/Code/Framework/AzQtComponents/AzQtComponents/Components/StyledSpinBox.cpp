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

#include <AzQtComponents/Components/StyledSpinBox.h>

#include <math.h>

#include <QApplication>
#include <QIntValidator>
#include <QLineEdit>
#include <QSignalBlocker>

namespace AzQtComponents
{

    const int sliderMarginToSpinBox = 4;
    const int sliderDefaultHeight = 35;
    const int sliderDefaultWidth = 192;

    class FocusInEventFilterPrivate
        : public QObject
    {
    public:
        FocusInEventFilterPrivate(StyledDoubleSpinBox* spinBox)
            : QObject(spinBox)
            , m_spinBox(spinBox){}

    protected:
        bool eventFilter(QObject* obj, QEvent* event)
        {
            if (event->type() == QEvent::FocusIn)
            {
                m_spinBox->displaySlider();
            }
            else if (event->type() == QEvent::FocusOut)
            {
                // Don't allow the focus out event to propogate if the mouse was
                // clicked on the slider
                if (m_spinBox->isMouseOnSlider())
                {
                    return true;
                }
                // Otherwise, hide the slider once we have lost focus
                else
                {
                    m_spinBox->hideSlider();
                }
            }

            return QObject::eventFilter(obj, event);
        }
    private:
        StyledDoubleSpinBox* m_spinBox;
    };

    class ClickEventFilterPrivate
        : public QObject
    {
    public:
        ClickEventFilterPrivate(StyledDoubleSpinBox* spinBox)
            : QObject(spinBox)
            , m_spinBox(spinBox){}
        ~ClickEventFilterPrivate(){}
    signals:
        void clickOnApplication(const QPoint& pos);
    protected:
        bool eventFilter(QObject* obj, QEvent* event)
        {
            if (event->type() == QEvent::MouseButtonRelease)
            {
                m_spinBox->handleClickOnApp(QCursor::pos());
            }

            return QObject::eventFilter(obj, event);
        }
    private:
        StyledDoubleSpinBox* m_spinBox;
    };

    StyledDoubleSpinBox::StyledDoubleSpinBox(QWidget* parent)
        : QDoubleSpinBox(parent)
        , m_restrictToInt(false)
        , m_customSliderMinValue(0.0f)
        , m_customSliderMaxValue(0.0f)
        , m_hasCustomSliderRange(false)
        , m_slider(nullptr)
        , m_ignoreNextUpdateFromSlider(false)
        , m_ignoreNextUpdateFromSpinBox(false)
    {
        setProperty("class", "SliderSpinBox");
        setButtonSymbols(QAbstractSpinBox::NoButtons);
    }

    StyledDoubleSpinBox::~StyledDoubleSpinBox()
    {
        delete m_slider;
    }

    void StyledDoubleSpinBox::showEvent(QShowEvent* ev)
    {
        if (!m_slider)
        {
            initSlider();
        }

        prepareSlider();
        QWidget::showEvent(ev);
    }

    void StyledDoubleSpinBox::resizeEvent(QResizeEvent* ev)
    {
        if (!m_slider)
        {
            initSlider();
        }

        prepareSlider();
        QDoubleSpinBox::resizeEvent(ev);
    }

    void StyledDoubleSpinBox::displaySlider()
    {
        if (!m_slider)
        {
            initSlider();
        }

        prepareSlider();
        m_slider->setFocus();
        setProperty("SliderSpinBoxFocused", true);
        m_justPassFocusSlider = true;
        m_slider->show();
        m_slider->raise();
    }

    void StyledDoubleSpinBox::hideSlider()
    {
        // prevent slider to hide when the
        // spinbox is passing focus along
        if (m_justPassFocusSlider)
        {
            m_justPassFocusSlider = false;
        }
        else
        {
            m_slider->hide();
            setProperty("SliderSpinBoxFocused", false);
            update();
        }
    }

    void StyledDoubleSpinBox::handleClickOnApp(const QPoint& pos)
    {
        if (isClickOnSlider(pos) || isClickOnSpinBox(pos))
        {
            if (m_slider->isVisible())
            {
                displaySlider();
            }
        }
        else
        {
            hideSlider();
            clearFocus();
        }
    }

    bool StyledDoubleSpinBox::isMouseOnSlider()
    {
        const QPoint& globalPos = QCursor::pos();
        return isClickOnSlider(globalPos);
    }

    bool StyledDoubleSpinBox::isClickOnSpinBox(const QPoint& globalPos)
    {
        const auto pos = mapFromGlobal(globalPos);
        const auto spaceRect = QRect(0, 0, width(), height());
        return spaceRect.contains(pos);
    }

    bool StyledDoubleSpinBox::isClickOnSlider(const QPoint& globalPos)
    {
        if (!m_slider || m_slider->isHidden())
        {
            return false;
        }

        const auto pos = m_slider->mapFromGlobal(globalPos);
        auto spaceRect = m_slider->rect();
        return spaceRect.contains(pos);
    }

    void StyledDoubleSpinBox::SetCustomSliderRange(double min, double max)
    {
        m_customSliderMinValue = min;
        m_customSliderMaxValue = max;
        m_hasCustomSliderRange = true;
    }

    double StyledDoubleSpinBox::GetSliderMinimum()
    {
        if (m_hasCustomSliderRange)
        {
            return m_customSliderMinValue;
        }

        return minimum();
    }

    double StyledDoubleSpinBox::GetSliderRange()
    {
        if (m_hasCustomSliderRange)
        {
            return m_customSliderMaxValue - m_customSliderMinValue;
        }

        return maximum() - minimum();
    }

    void StyledDoubleSpinBox::prepareSlider()
    {
        // If we are treating this as integer only (like QSpinBox), then we can
        // set our min/max and value directly to the slider since it uses integers
        if (m_restrictToInt)
        {
            // If we have a custom slider range, then set that, otherwise
            // use the same range for the slider as our spinbox
            if (m_hasCustomSliderRange)
            {
                m_slider->setMinimum(m_customSliderMinValue);
                m_slider->setMaximum(m_customSliderMaxValue);
            }
            else
            {
                m_slider->setMinimum(minimum());
                m_slider->setMaximum(maximum());
            }
        }
        // Otherwise, we need to set a custom scale for our slider using 0 as the
        // minimum and a power of 10 based on our decimal precision as the maximum
        else
        {
            int scaledMax = pow(10, (int)log10(GetSliderRange()) + decimals());
            m_slider->setMinimum(0);
            m_slider->setMaximum(scaledMax);
        }

        // Set our slider value
        updateSliderValue(value());

        // detect the required background depending on how close to border is the spinbox
        auto globalWindow = QApplication::activeWindow();
        if (!globalWindow)
        {
            return;
        }

        auto spinBoxTopLeftGlobal = mapToGlobal(QPoint(0, 0));
        int globalWidth = globalWindow->x() + globalWindow->width();

        if (globalWidth - spinBoxTopLeftGlobal.x() < sliderDefaultWidth)
        {
            int offset = sliderDefaultWidth - width();
            m_slider->setStyleSheet("background: transparent; border-image: url(:/stylesheet/img/styledspinbox-bg-right.png);");
            m_slider->setGeometry(spinBoxTopLeftGlobal.x() - offset, spinBoxTopLeftGlobal.y() + height(), sliderDefaultWidth, sliderDefaultHeight);
        }
        else
        {
            m_slider->setStyleSheet("background: transparent; border-image: url(:/stylesheet/img/styledspinbox-bg-left.png);");
            m_slider->setGeometry(spinBoxTopLeftGlobal.x(), spinBoxTopLeftGlobal.y() + height(), sliderDefaultWidth, sliderDefaultHeight);
        }
    }

    void StyledDoubleSpinBox::initSlider()
    {
        m_slider = new StyledSliderPrivate;
        m_slider->setWindowFlags(Qt::WindowFlags(Qt::Window) | Qt::WindowFlags(Qt::FramelessWindowHint) | Qt::WindowFlags(Qt::ToolTip));

        QObject::connect(this, static_cast<void(StyledDoubleSpinBox::*)(double)>(&StyledDoubleSpinBox::valueChanged),
            this, &StyledDoubleSpinBox::updateSliderValue);

        QObject::connect(m_slider, &QSlider::valueChanged,
            this, &StyledDoubleSpinBox::updateValueFromSlider);

        // These event filters will be automatically removed when our spin box is deleted
        // since they are parented to it
        qApp->installEventFilter(new ClickEventFilterPrivate(this));
        installEventFilter(new FocusInEventFilterPrivate(this));
    }

    void StyledDoubleSpinBox::updateSliderValue(double newVal)
    {
        if (!m_slider)
        {
            return;
        }

        // Ignore this update if it was triggered by the user changing the slider,
        // which updated our spin box value
        if (m_ignoreNextUpdateFromSpinBox)
        {
            m_ignoreNextUpdateFromSpinBox = false;
            return;
        }

        // No need to continue if the slider value didn't change
        int currentSliderValue = m_slider->value();
        int sliderValue = ConvertToSliderValue(newVal);
        if (sliderValue == currentSliderValue)
        {
            return;
        }

        // Since we are about to set the slider value, flag the next update
        // to be ignored the slider so that it doesn't cause an extra loop,
        // but only if the value wouldn't be out of bounds in the case where
        // our slider range is different than our text input range, because
        // if the value would be out of range for the slider, the value won't
        // actually change if the slider value is already at the min or max
        // value
        bool outOfBounds = false;
        if (m_hasCustomSliderRange)
        {
            if (m_restrictToInt)
            {
                if (currentSliderValue == m_customSliderMaxValue && sliderValue > m_customSliderMaxValue)
                {
                    outOfBounds = true;
                }
                else if (currentSliderValue == m_customSliderMinValue && sliderValue < m_customSliderMinValue)
                {
                    outOfBounds = true;
                }
            }
            else
            {
                if (currentSliderValue == m_slider->maximum() && sliderValue > m_slider->maximum())
                {
                    outOfBounds = true;
                }
                else if (currentSliderValue == 0 && sliderValue < 0)
                {
                    outOfBounds = true;
                }
            }
        }
        if (!outOfBounds)
        {
            m_ignoreNextUpdateFromSlider = true;
        }

        // Update the slider value
        m_slider->setValue(sliderValue);
    }

    void StyledDoubleSpinBox::updateValueFromSlider(int newVal)
    {
        if (!m_slider)
        {
            return;
        }

        // Ignore this updated if it was triggered by the user changing the spin box,
        // which updated our slider value
        if (m_ignoreNextUpdateFromSlider)
        {
            m_ignoreNextUpdateFromSlider = false;
            return;
        }

        // No need to continue of the spinbox value didn't change
        double currentSpinBoxValue = value();
        double spinBoxValue = ConvertFromSliderValue(newVal);
        if (spinBoxValue == currentSpinBoxValue)
        {
            return;
        }

        // Since we are about to set the spin box value, flag the next update
        // to be ignored the spin box so that it doesn't cause an extra loop
        m_ignoreNextUpdateFromSpinBox = true;

        // Update the spin box value
        setValue(spinBoxValue);
    }

    int StyledDoubleSpinBox::ConvertToSliderValue(double spinBoxValue)
    {
        // If we are in integer only mode, we can cast the value directly
        int newVal;
        if (m_restrictToInt)
        {
            newVal = (int)spinBoxValue;
        }
        // Otherwise we need to convert our spin box double value to an
        // appropriate integer value for our custom slider scale
        else
        {
            newVal = ((spinBoxValue - GetSliderMinimum()) / GetSliderRange()) * m_slider->maximum();
        }

        return newVal;
    }

    double StyledDoubleSpinBox::ConvertFromSliderValue(int sliderValue)
    {
        // If we are in integer only mode, we can cast the value directly
        double newVal;
        if (m_restrictToInt)
        {
            newVal = (double)sliderValue;
        }
        // Otherwise we need to convert our slider int value from its custom
        // scale to an appropriate double value for our spin box
        else
        {
            double sliderScale = (double)sliderValue / (double)m_slider->maximum();
            newVal = (sliderScale * GetSliderRange()) + GetSliderMinimum();
        }

        return newVal;
    }

    StyledSliderPrivate::StyledSliderPrivate()
        : QSlider()
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setOrientation(Qt::Horizontal);
        hide();
    }

    StyledSpinBox::StyledSpinBox(QWidget* parent)
        : StyledDoubleSpinBox(parent)
        , m_validator(new QIntValidator(minimum(), maximum(), this))
    {
        // This StyledSpinBox mirrors the same functionality of the QSpinBox, so
        // we need to set this flag so our StyledDoubleSpinBox knows to behave
        // assuming only integer values
        m_restrictToInt = true;

        // To enforce integer only input, we set our decimal precision to 0 and
        // change the validator of the QLineEdit to only accept integers
        setDecimals(0);
        QLineEdit* lineEdit = findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly);
        if (lineEdit)
        {
            lineEdit->setValidator(m_validator);
        }

        // Added a valueChanged signal with an int parameter to mirror the behavior
        // of the QSpinBox
        QObject::connect(this, static_cast<void(StyledDoubleSpinBox::*)(double)>(&StyledDoubleSpinBox::valueChanged), [this](double val) {
            emit valueChanged((int)val);
        });
    }

    int StyledSpinBox::maximum() const
    {
        return (int)StyledDoubleSpinBox::maximum();
    }

    int StyledSpinBox::minimum() const
    {
        return (int)StyledDoubleSpinBox::minimum();
    }

    void StyledSpinBox::setMaximum(int max)
    {
        StyledDoubleSpinBox::setMaximum((double)max);

        // Update our validator maximum
        m_validator->setTop(max);
    }

    void StyledSpinBox::setMinimum(int min)
    {
        StyledDoubleSpinBox::setMinimum((double)min);

        // Update our validator minimum
        m_validator->setBottom(min);
    }

    void StyledSpinBox::setRange(int min, int max)
    {
        StyledDoubleSpinBox::setRange((double)min, (double)max);

        // Update our validator range
        m_validator->setRange(min, max);
    }

    void StyledSpinBox::setSingleStep(int val)
    {
        StyledDoubleSpinBox::setSingleStep((double)val);
    }

    int StyledSpinBox::singleStep() const
    {
        return (int)StyledDoubleSpinBox::singleStep();
    }

    int StyledSpinBox::value() const
    {
        return (int)StyledDoubleSpinBox::value();
    }

    void StyledSpinBox::setValue(int val)
    {
        StyledDoubleSpinBox::setValue((double)val);
    }

#include <Components/StyledSpinBox.moc>
} // namespace AzQtComponents
