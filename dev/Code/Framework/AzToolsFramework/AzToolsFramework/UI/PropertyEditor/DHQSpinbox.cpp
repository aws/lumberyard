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
#include "stdafx.h"
#include "DHQSpinbox.hxx"
#include <AzFramework/Math/MathUtils.h> // for the Close function

#include <QDesktopWidget>
#include <QSignalBlocker>

namespace AzToolsFramework
{
    void MouseEvent(QEvent* event, QAbstractSpinBox* spinBox, QPoint& lastMousePos, bool& isMouseCaptured)
    {
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
                    spinBox->cursor().setPos(mousePos);
                }
                else if (mouseEvent->globalPos().y() <= 0)
                {
                    mousePos.setY(screenSize.height() - 2);
                    spinBox->cursor().setPos(mousePos);
                }

                lastMousePos = mousePos;
            }
        }
        break;
        }
    }

    DHQSpinbox::DHQSpinbox(QWidget* parent) 
        : QSpinBox(parent)
    {
        setButtonSymbols(QAbstractSpinBox::NoButtons);

        // we want to be able to handle the Escape key so that it reverts
        // what's in the text field if it wasn't committed already
        using OnValueChanged = void(QSpinBox::*)(int);
        auto valueChanged = static_cast<OnValueChanged>(&QSpinBox::valueChanged);
        connect(this, valueChanged, this, [this](int newValue){
            m_lastValue = newValue;
        });

        EditorEvents::Bus::Handler::BusConnect();
    }

    bool DHQSpinbox::event(QEvent* event)
    {
        if (isEnabled())
        {
            if (event->type() == QEvent::Wheel)
            {
                if (hasFocus())
                {
                    QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
                    QSpinBox::wheelEvent(wheelEvent);
                }
                else
                {
                    event->ignore();
                    return true;
                }
            }
            else
            {
                //  MouseEvent doesn't need to accept/ignore the event.
                //  We can check the event and still pass it up to our parent.
                MouseEvent(event, this, lastMousePosition, mouseCaptured);
            }
        }

        return QSpinBox::event(event);
    }

    void DHQSpinbox::setValue(int value)
    {
        m_lastValue = value;
        QSpinBox::setValue(value);
    }

    void DHQSpinbox::OnEscape()
    {
        if (!keyboardTracking())
        {
            // If we're not keyboard tracking, then changes to the text field
            // aren't 'committed' until the user hits Enter, Return, tabs out of
            // the field, or the spinbox is hidden.
            // We want to stop the commit from happening if the user hits escape.
            if (hasFocus())
            {
                // Big logic jump here; if the current value has been committed already,
                // then everything listening on the signals knows about it already.
                // If the current value has NOT been committed, nothing knows about it
                // but we don't want to trigger any further updates, because we're resetting
                // it.
                // So we disable signals here
                {
                    QSignalBlocker blocker(this);
                    setValue(m_lastValue);
                }

                selectAll();
            }
        }
    }

    QSize DHQSpinbox::minimumSizeHint() const
    {
        //  This prevents the range from affecting the size, allowing the use of user-defined size hints.
        QSize size = QSpinBox::sizeHint();
        size.setWidth(minimumWidth());
        return size;
    }

    DHQDoubleSpinbox::DHQDoubleSpinbox(QWidget* parent)
        : QDoubleSpinBox(parent)
    {
        setButtonSymbols(QAbstractSpinBox::NoButtons);

        // we want to be able to handle the Escape key so that it reverts
        // what's in the text field if it wasn't committed already
        using OnValueChanged = void(QDoubleSpinBox::*)(double);
        auto valueChanged = static_cast<OnValueChanged>(&QDoubleSpinBox::valueChanged);
        connect(this, valueChanged, this, [this](double newValue){
            m_lastValue = newValue;
        });

        EditorEvents::Bus::Handler::BusConnect();
    }

    bool DHQDoubleSpinbox::event(QEvent* event)
    {
        if (isEnabled())
        {
            if (event->type() == QEvent::Wheel)
            {
                if (hasFocus())
                {
                    QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
                    QDoubleSpinBox::wheelEvent(wheelEvent);
                }
                else
                {
                    event->ignore();
                    return true;
                }
            }
            else
            {
                //  MouseEvent doesn't need to accept/ignore the event.
                //  We can check the event and still pass it up to our parent.
                MouseEvent(event, this, lastMousePosition, mouseCaptured);
            }
        }

        return QDoubleSpinBox::event(event);
    }

    void DHQDoubleSpinbox::setValue(double value)
    {
        m_lastValue = value;
        QDoubleSpinBox::setValue(value);
    }

    void DHQDoubleSpinbox::OnEscape()
    {
        if (!keyboardTracking())
        {
            // If we're not keyboard tracking, then changes to the text field
            // aren't 'committed' until the user hits Enter, Return, tabs out of
            // the field, or the spinbox is hidden.
            // We want to stop the commit from happening if the user hits escape.
            if (hasFocus())
            {
                // Big logic jump here; if the current value has been committed already,
                // then everything listening on the signals knows about it already.
                // If the current value has NOT been committed, nothing knows about it
                // but we don't want to trigger any further updates, because we're resetting
                // it.
                // So we disable signals here
                {
                    QSignalBlocker blocker(this);
                    setValue(m_lastValue);
                }

                selectAll();
            }
        }
    }

    QSize DHQDoubleSpinbox::minimumSizeHint() const
    {
        //  This prevents the range from affecting the size, allowing the use of user-defined size hints.
        QSize size = QDoubleSpinBox::sizeHint();
        size.setWidth(minimumWidth());
        return size;
    }
}