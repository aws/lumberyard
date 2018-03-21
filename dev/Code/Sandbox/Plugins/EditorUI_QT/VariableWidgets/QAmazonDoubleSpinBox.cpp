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
#include "QAmazonDoubleSpinBox.h"

#include <QLineEdit>
#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
#include <QtWidgets/QDesktopWidget>


QAmazonDoubleSpinBox::QAmazonDoubleSpinBox(QWidget* parent)
    : QDoubleSpinBox(parent)
{
    setDecimals(m_precision);
    lineEdit()->installEventFilter(this);
    connect(lineEdit(), &QLineEdit::editingFinished, this, [=]() { m_isEditInProgress = false; });
}

QAmazonDoubleSpinBox::~QAmazonDoubleSpinBox()
{
    lineEdit()->removeEventFilter(this);
}

void QAmazonDoubleSpinBox::focusInEvent(QFocusEvent* event)
{
    m_isFocusedIn = true;
    m_isEditInProgress = true;
    QDoubleSpinBox::focusInEvent(event);
    setValue(value());
    selectAll();
}

void QAmazonDoubleSpinBox::MouseEvent(QEvent* event)
{
    // This code is very similiar to MouseEvent() in DHQSpinbox.cpp. We need the mouse input functionality but 
    // the ParticleEditor has a completely separate UX system from the main Editor, so we aren't set up to share 
    // implementation at this time. Consider merging this functionality with DHQSpinbox someday.

    bool& isMouseCaptured = m_mouseCaptured;
    bool& isMouseDragging = m_mouseDragging;
    QAbstractSpinBox* spinBox = this;

    switch (event->type())
    {
    case QEvent::MouseButtonPress:
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        m_lastMousePosition = mouseEvent->globalPos();
        isMouseCaptured = true;
    }
    break;
    case QEvent::MouseButtonRelease:
    {
        if (isMouseCaptured)
        {
            spinBox->unsetCursor();
            isMouseCaptured = false;
        }
        if (isMouseDragging)
        {
            isMouseDragging = false;
            Q_EMIT spinnerDragFinished();
        }
    }
    break;
    case QEvent::MouseMove:
    {
        if (isMouseCaptured)
        {
            Q_EMIT spinnerDragged();

            m_mouseDragging = true;

            spinBox->setCursor(Qt::SizeVerCursor);
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPoint mousePos = mouseEvent->globalPos();

            int distance = m_lastMousePosition.y() - mousePos.y();
            spinBox->stepBy(distance);
            m_lastMousePosition = mouseEvent->globalPos();

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

            m_lastMousePosition = mousePos;
        }
    }
    break;
    }
}

bool QAmazonDoubleSpinBox::event(QEvent* event)
{
    // This code is copied directly from event() in DHQSpinbox.cpp. We need the same mouse input functionality but 
    // the ParticleEditor has a completely separate UX system from the main Editor, so we aren't set up to share 
    // implementation at this time. Consider merging this functionality with DHQSpinbox someday.

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
            MouseEvent(event);
        }
    }

    return QDoubleSpinBox::event(event);
}


bool QAmazonDoubleSpinBox::eventFilter(QObject* obj, QEvent* e)
{
    QObject* _lineEdit = static_cast<QObject*>(lineEdit());
    if (obj == _lineEdit)
    {
        if (e->type() == QMouseEvent::MouseButtonDblClick)
        {
            QMouseEvent* mev = static_cast<QMouseEvent*>(e);
            if (mev->button() == Qt::LeftButton)
            {
                lineEdit()->selectAll();
                return true;
            }
        }
        if (e->type() == QMouseEvent::MouseButtonPress)
        {
            // If the mouse button press event happened when mouse FocusIn
            // the widget, ignored the mousebuttonpress event
            if (m_isFocusedIn)
            {
                m_isFocusedIn = false;
                return true;
            }
        }
    }

    return QDoubleSpinBox::eventFilter(obj, e);
}

void QAmazonDoubleSpinBox::keyPressEvent(QKeyEvent* event)
{
    m_isEditInProgress = true;
    QDoubleSpinBox::keyPressEvent(event);

    //fix the issue with DoubleSpinbox: when enter key pressed after editing the text won't be all selected
    if ((event->key() == Qt::Key_Enter) || (event->key() == Qt::Key_Return))
    {
        lineEdit()->selectAll();
    }
}

void QAmazonDoubleSpinBox::focusOutEvent(QFocusEvent* event)
{
    m_isEditInProgress = false;
    m_isFocusedIn = false;
    return QDoubleSpinBox::focusOutEvent(event);
}

QString QAmazonDoubleSpinBox::textFromValue(double val) const
{
    QString text = "";

    // We show fewer decimal places when not editing for a cleaner visual style.
    text = QString::number(val, m_decimalFormat, (m_isEditInProgress ? m_decimalsLong : m_decimalsShort));

    // Remove trailing 0's
    text.remove(QRegExp("0+$"));

    // Remove trailing decimal point in case all post-decimal digits were 0's
    text.remove(QRegExp("\\.$"));

    return text;
}

// The default valueFromText function can only convert value with precious of 5
double QAmazonDoubleSpinBox::valueFromText(const QString& text) const
{
    return QDoubleSpinBox::valueFromText(text);
}

#include <VariableWidgets/QAmazonDoubleSpinBox.moc>
