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
#include "DHQSpinbox.hxx"
#include <AzFramework/Math/MathUtils.h> // for the Close function

#include <QDesktopWidget>
#include <QSignalBlocker>
AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                               // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
#include <QMouseEvent>
AZ_POP_DISABLE_WARNING
#include <QWheelEvent>

namespace AzToolsFramework
{
    namespace DHSpinBox
    {
        // Decimal precision parameters
        static const int DecimalPrecisonDefault = 7;
        static const int DecimalDisplayPrecisionDefault = 3;

        // MouseEvent doesn't need to accept/ignore the event
        // We can check the event and still pass it up to our parent
        static void MouseEvent(QAbstractSpinBox* spinBox, QEvent* event, QPoint& lastMousePos, bool& isMouseCaptured)
        {
            AZ_Assert(spinBox, "spinBox must not be null");
            AZ_Assert(event, "event must not be null");

            switch (event->type())
            {
            case QEvent::MouseButtonPress:
                {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    lastMousePos = mouseEvent->globalPos();
                    isMouseCaptured = true;
                }
                break;
            case QEvent::MouseButtonRelease:
                {
                    spinBox->unsetCursor();
                    isMouseCaptured = false;
                }
                break;
            case QEvent::MouseMove:
                {
                    if (isMouseCaptured)
                    {
                        spinBox->setCursor(Qt::SizeVerCursor);
                        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                        QPoint mousePos = mouseEvent->globalPos();

                        int distance = lastMousePos.y() - mousePos.y();
                        spinBox->stepBy(distance);
                        lastMousePos = mouseEvent->globalPos();

                        int screenId = QApplication::desktop()->screenNumber(mousePos);
                        QRect screenSize = QApplication::desktop()->screen(screenId)->geometry();
                        if (mouseEvent->globalPos().y() >= screenSize.height() - 1)
                        {
                            mousePos.setY(1);
                        }
                        else if (mouseEvent->globalPos().y() <= 0)
                        {
                            mousePos.setY(screenSize.height() - 2);
                        }

                        lastMousePos = mousePos;
                    }
                }
                break;
            }
        }

        static bool WheelEvent(QAbstractSpinBox* spinBox, QWheelEvent* event)
        {
            AZ_Assert(spinBox, "spinBox must not be null");
            AZ_Assert(event, "event must not be null");

            if (!spinBox->hasFocus())
            {
                event->ignore();
                return true;
            }

            return false;
        }

        static QSize MinimumSizeHint(const QAbstractSpinBox* spinBox)
        {
            AZ_Assert(spinBox, "spinBox must not be null");

            // This prevents the range from affecting the size, allowing the use of user-defined size hints.
            QSize size = spinBox->sizeHint();
            size.setWidth(spinBox->minimumWidth());
            return size;
        }

        // Note: use a templated type to facilitate 'duck typing' for common
        // functions between DHQDoubleSpinbox and DHQSpinbox
        template<typename SpinBox>
        void HandleEscape(SpinBox* spinBox)
        {
            AZ_Assert(spinBox, "spinBox must not be null");

            if (!spinBox->keyboardTracking())
            {
                // If we're not keyboard tracking, then changes to the text field
                // aren't 'committed' until the user hits Enter, Return, tabs out of
                // the field, or the spinbox is hidden.
                // We want to stop the commit from happening if the user hits escape.
                if (spinBox->hasFocus())
                {
                    // Big logic jump here; if the current value has been committed already,
                    // then everything listening on the signals knows about it already.
                    // If the current value has NOT been committed, nothing knows about it
                    // but we don't want to trigger any further updates, because we're resetting
                    // it, so we disable signals here
                    {
                        QSignalBlocker blocker(spinBox);
                        spinBox->setValue(spinBox->LastValue());
                    }

                    // If the text was not selected (mostly likely the user was typing a value)
                    // then hitting Escape will first retain focus and select all text in the box
                    if (!spinBox->HasSelectedText())
                    {
                        spinBox->selectAll();
                    }
                    else
                    {
                        // A second press of Escape will clear focus from the box
                        spinBox->clearFocus();
                    }
                }
            }
        }

        template<typename SpinBox>
        bool EventFilter(SpinBox* spinBox, QEvent* event)
        {
            AZ_Assert(spinBox, "spinBox must not be null");
            AZ_Assert(event, "event must not be null");

            // handle 'Escape' key behavior
            if (event->type() == QEvent::ShortcutOverride ||
                event->type() == QEvent::KeyPress)
            {
                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
                if (keyEvent->key() == Qt::Key_Escape && spinBox->hasFocus())
                {
                    if (!event->isAccepted())
                    {
                        event->accept();
                        HandleEscape(spinBox);
                    }

                    return true;
                }
            }

            if (spinBox->isEnabled())
            {
                if (event->type() == QEvent::ShortcutOverride)
                {
                    // This should be handled in the base class, but since that's part of Qt, do it here.
                    // The Up and Down keys have a function while this widget is in focus,
                    // so prevent those shortcuts from firing
                    QKeyEvent* kev = static_cast<QKeyEvent*>(event);
                    switch (kev->key())
                    {
                    case (Qt::Key_Up):
                    case (Qt::Key_Down):
                        event->accept();
                        return true;

                    default:
                        break;
                    }
                }
            }

            return false;
        }

        template void HandleEscape<DHQSpinbox>(DHQSpinbox* spinBox);
        template void HandleEscape<DHQDoubleSpinbox>(DHQDoubleSpinbox* spinBox);
        template bool EventFilter<DHQSpinbox>(DHQSpinbox* spinBox, QEvent* event);
        template bool EventFilter<DHQDoubleSpinbox>(DHQDoubleSpinbox* spinBox, QEvent* event);
    }

    DHQSpinbox::DHQSpinbox(QWidget* parent)
        : QSpinBox(parent)
    {
        setButtonSymbols(QAbstractSpinBox::NoButtons);

        // we want to be able to handle the Escape key so that it reverts
        // what's in the text field if it wasn't committed already
        QObject::connect(
            this, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int newValue)
        {
            m_lastValue = newValue;
        });

        installEventFilter(this);
    }

    bool DHQSpinbox::event(QEvent* event)
    {
        DHSpinBox::MouseEvent(this, event, m_lastMousePosition, m_mouseCaptured);
        return QSpinBox::event(event);
    }

    bool DHQSpinbox::eventFilter(QObject* object, QEvent* event)
    {
        if (DHSpinBox::EventFilter(this, event))
        {
            return true;
        }

        return QSpinBox::eventFilter(object, event);
    }

    void DHQSpinbox::focusInEvent(QFocusEvent* event)
    {
        m_lastValue = value();

        QSpinBox::focusInEvent(event);
    }

    QSize DHQSpinbox::minimumSizeHint() const
    {
        return DHSpinBox::MinimumSizeHint(this);
    }

    bool DHQSpinbox::HasSelectedText() const
    {
        return lineEdit()->hasSelectedText();
    }

    DHQDoubleSpinbox::DHQDoubleSpinbox(QWidget* parent)
        : QDoubleSpinBox(parent)
        , m_displayDecimals(DHSpinBox::DecimalDisplayPrecisionDefault)
    {
        setButtonSymbols(QAbstractSpinBox::NoButtons);

        // we want to be able to handle the Escape key so that it reverts
        // what's in the text field if it wasn't committed already
        QObject::connect(
            this, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double newValue)
        {
            m_lastValue = newValue;
        });

        // Our tooltip will be the full decimal value, so keep it updated whenever our value changes
        QObject::connect(
            this, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &DHQDoubleSpinbox::UpdateToolTip);

        // Set the default decimal precision we will store to a large number
        // since we will be truncating the value displayed
        setDecimals(DHSpinBox::DecimalPrecisonDefault);

        UpdateToolTip(value());

        installEventFilter(this);
    }

    bool DHQDoubleSpinbox::event(QEvent* event)
    {
        DHSpinBox::MouseEvent(this, event, m_lastMousePosition, m_mouseCaptured);
        return QDoubleSpinBox::event(event);
    }

    void DHQDoubleSpinbox::wheelEvent(QWheelEvent* event)
    {
        if (!DHSpinBox::WheelEvent(this, event))
        {
            QDoubleSpinBox::wheelEvent(event);
        }
    }

    void DHQSpinbox::wheelEvent(QWheelEvent* event)
    {
        if (!DHSpinBox::WheelEvent(this, event))
        {
            QSpinBox::wheelEvent(event);
        }
    }

    bool DHQDoubleSpinbox::eventFilter(QObject* object, QEvent* event)
    {
        if (DHSpinBox::EventFilter(this, event))
        {
            return true;
        }

        return QDoubleSpinBox::eventFilter(object, event);
    }

    QSize DHQDoubleSpinbox::minimumSizeHint() const
    {
        return DHSpinBox::MinimumSizeHint(this);
    }

    void DHQDoubleSpinbox::SetDisplayDecimals(int precision)
    {
        m_displayDecimals = precision;
    }

    QString DHQDoubleSpinbox::StringValue(double value, bool truncated) const
    {
        // Determine which decimal precision to use for displaying the value
        int numDecimals = decimals();
        if (truncated && m_displayDecimals < numDecimals)
        {
            numDecimals = m_displayDecimals;
        }

        QString stringValue = locale().toString(value, 'f', numDecimals);

        // Handle special cases when we have decimals in our value
        if (numDecimals > 0)
        {
            // Remove trailing zeros, since the locale conversion won't do
            // it for us
            QChar zeroDigit = locale().zeroDigit();
            QString trailingZeros = QString("%1+$").arg(zeroDigit);
            stringValue.remove(QRegExp(trailingZeros));

            // It's possible we could be left with a decimal point on the end
            // if we stripped the trailing zeros, so if that's the case, then
            // add a zero digit on the end so that it is obvious that this is
            // a float value
            QChar decimalPoint = locale().decimalPoint();
            if (stringValue.endsWith(decimalPoint))
            {
                stringValue.append(zeroDigit);
            }
        }

        // Copied from the QDoubleSpinBox sub-class to handle removing the
        // group separator if necessary
        if (!isGroupSeparatorShown() && qAbs(value) >= 1000.0)
        {
            stringValue.remove(locale().groupSeparator());
        }

        return stringValue;
    }

    QString DHQDoubleSpinbox::textFromValue(double value) const
    {
        // If our widget is focused, then show the full decimal value, otherwise
        // show the truncated value
        return StringValue(value, !hasFocus());
    }

    void DHQDoubleSpinbox::UpdateToolTip(double value)
    {
        // Set our tooltip to the full decimal value
        setToolTip(StringValue(value));
    }

    void DHQDoubleSpinbox::focusInEvent(QFocusEvent* event)
    {
        // We need to set the special value text to an empty string, which
        // effectively makes no change, but actually triggers the line edit
        // display value to be updated so that when we receive focus to
        // begin editing, we display the full decimal precision instead of
        // the truncated display value
        setSpecialValueText(QString());

        // Remove the suffix while editing
        m_lastSuffix = suffix();
        setSuffix(QString());

        m_lastValue = value();

        QDoubleSpinBox::focusInEvent(event);
    }

    void DHQDoubleSpinbox::focusOutEvent(QFocusEvent* event)
    {
        QDoubleSpinBox::focusOutEvent(event);

        // restore the suffix now, if needed
        if (!m_lastSuffix.isEmpty())
        {
            setSuffix(m_lastSuffix);
            m_lastSuffix.clear();
        }
    }

    bool DHQDoubleSpinbox::HasSelectedText() const
    {
        return lineEdit()->hasSelectedText();
    }
}

#include <UI/PropertyEditor/DHQSpinbox.moc>
