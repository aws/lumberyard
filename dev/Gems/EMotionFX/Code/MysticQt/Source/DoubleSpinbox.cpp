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

// include the required headers
#include "DoubleSpinbox.h"
#include "MysticQtManager.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtGui/QValidator>
#include <QtGui/QDoubleValidator>
#include <QApplication>


namespace MysticQt
{
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Spin button
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    // constructor
    DoubleSpinboxButton::DoubleSpinboxButton(QWidget* parent, DoubleSpinBox* spinbox, bool downButton)
        : QToolButton(parent)
    {
        mSpinbox        = spinbox;
        mPrevMouseY     = 0;
        mPressedMouseY  = 0;
        mDownButton     = downButton;
        mMouseMoveSpinningEnabled = false;

        setFocusPolicy(Qt::StrongFocus);

        if (mDownButton == false)
        {
            setArrowType(Qt::UpArrow);
            setObjectName("SpinBoxButtonUp");
        }
        else
        {
            setArrowType(Qt::DownArrow);
            setObjectName("SpinBoxButtonDown");
        }

        SetButtonEnabled(true);

        setMinimumWidth(13);
        setMaximumWidth(13);
    }


    // destructor
    DoubleSpinboxButton::~DoubleSpinboxButton()
    {
    }


    // make the widget look as it is enabled disabled without using Qt's enabled flag
    void DoubleSpinboxButton::SetButtonEnabled(bool enabled)
    {
        if (mEnabled != enabled)
        {
            /*      if (mEnabled)
                        setStyleSheet("");
                    else
                        setStyleSheet("color: rgb(59, 59, 59); background-color: rgb(34, 34, 34);");*/

            if (mEnabled)
            {
                setStyleSheet("color: rgb(59, 59, 59);");
            }
            else
            {
                setStyleSheet("color: rgb(137, 137, 137); background-color: rgb(34, 34, 34);");
            }
        }

        mEnabled = enabled;
    }


    // called when mouse is moved
    void DoubleSpinboxButton::mouseMoveEvent(QMouseEvent* event)
    {
        // skip this operation in case mouse move spinning is not allowed
        if (mSpinbox->GetMouseMoveSpinningAllowed())
        {
            // calculate the delta mouse movement
            const int32 deltaY = event->globalY() - mPrevMouseY;
            const int32 totalDeltaY = event->globalY() - mPressedMouseY;

            if (mMouseMoveSpinningEnabled || (mMouseMoveSpinningEnabled == false && (totalDeltaY > 2 || totalDeltaY < -2)))
            {
                // set the slider cursor the first time
                if (mMouseMoveSpinningEnabled == false)
                {
                    QApplication::setOverrideCursor(Qt::SizeVerCursor);
                }

                // if we entered here once we use mouse movement spinning
                mMouseMoveSpinningEnabled = true;

                if (deltaY != 0)
                {
                    double newValue = mSpinbox->value() - deltaY * mSpinbox->singleStep() * 0.1;

                    // in case the new value is out of range
                    if (newValue > mSpinbox->maximum())
                    {
                        newValue = mSpinbox->maximum();
                    }

                    if (newValue < mSpinbox->minimum())
                    {
                        newValue = mSpinbox->minimum();
                    }

                    mSpinbox->setValue(newValue);

                    if (mSpinbox->GetValueChangedSignalOnMouseMoveDisabled() == false)
                    {
                        mSpinbox->EmitValueChangedSignal();
                    }
                }
            }

            // store the current value as previous value
            mPrevMouseY = event->globalY();
        }

        QWidget* mainWindow = GetMysticQt()->GetMainWindow();
        MCORE_ASSERT(mainWindow);

        // mouse wrapping when reaching the border
        QPoint localMousePos = mainWindow->mapFromGlobal(event->globalPos());
        if (localMousePos.y() < 0 || localMousePos.y() > mSpinbox->height())
        {
            // wrap the mouse in case we reached the bounds
            if (localMousePos.y() < 0)
            {
                localMousePos.setY(mainWindow->height() - 2);
            }
            else
            if (localMousePos.y() >= mainWindow->height() - 1)
            {
                localMousePos.setY(0);
            }

            // get the mouse position in screen global coordinates
            QPoint newGlobalPos = mainWindow->mapToGlobal(localMousePos);

            // this is needed because when using QCursor::setPos() a mouse move event will be fired by Qt
            int compensateDelta = newGlobalPos.y() - event->globalPos().y();
            double compensatedValue = mSpinbox->value() + compensateDelta * mSpinbox->singleStep() * 0.1;

            mSpinbox->setValue(compensatedValue, false, false);

            // set the new global mouse position
            QCursor::setPos(newGlobalPos.x(), newGlobalPos.y());
        }

        //QToolButton::mouseMoveEvent(event);
        //event->accept();
    }


    // called when a mouse click got detected
    void DoubleSpinboxButton::mousePressEvent(QMouseEvent* event)
    {
        QToolButton::mousePressEvent(event);

        mMouseMoveSpinningEnabled = false;

        // store the mouse position at the time when we pressed the mouse button
        //mPressedMouseX = event->globalX();
        mPressedMouseY = event->globalY();

        // store the current value as previous value
        //mPrevMouseX = event->globalX();
        mPrevMouseY = event->globalY();

        //event->accept();
    }


    // called when the mouse got released after pressing it
    void DoubleSpinboxButton::mouseReleaseEvent(QMouseEvent* event)
    {
        QToolButton::mouseReleaseEvent(event);

        if (mSpinbox->GetMouseMoveSpinningAllowed() && mMouseMoveSpinningEnabled)
        {
            mSpinbox->EmitValueChangedMouseReleased();

            // restore the override cursor
            QApplication::restoreOverrideCursor();
        }
        else
        {
            if (mDownButton)
            {
                OnDownButtonClicked();
            }
            else
            {
                OnUpButtonClicked();
            }
        }

        mMouseMoveSpinningEnabled = false;
        //event->accept();
    }


    // called when a keyboard button got pressed
    void DoubleSpinboxButton::keyPressEvent(QKeyEvent* event)
    {
        int32 key = event->key();

        // handle decreasing the value
        if (key == Qt::Key_Down)
        {
            OnDownButtonClicked();
        }

        if (key == Qt::Key_PageDown)
        {
            OnPageDownPressed();
        }

        // handle increasing the value
        if (key == Qt::Key_Up)
        {
            OnUpButtonClicked();
        }

        if (key == Qt::Key_PageUp)
        {
            OnPageUpPressed();
        }

        event->accept();
    }


    // just for completelyness and accepting the key events
    void DoubleSpinboxButton::keyReleaseEvent(QKeyEvent* event)
    {
        event->accept();
    }


    // called when the spin up button has been clicked
    void DoubleSpinboxButton::OnUpButtonClicked()
    {
        // add a single step to the value
        double newValue = mSpinbox->value() + mSpinbox->singleStep();

        // check if the new value is out of range
        if (newValue > mSpinbox->maximum())
        {
            newValue = mSpinbox->maximum();
        }

        // fire value changed events
        mSpinbox->setValue(newValue);
        mSpinbox->EmitValueChangedSignal();
    }


    // called when the spin down button has been clicked
    void DoubleSpinboxButton::OnDownButtonClicked()
    {
        // remove a single step from the value
        double newValue = mSpinbox->value() - mSpinbox->singleStep();

        // check if the new value is out of range, don't emit any changes in this case
        if (newValue < mSpinbox->minimum())
        {
            newValue = mSpinbox->minimum();
        }

        // fire value changed events
        mSpinbox->setValue(newValue);
        mSpinbox->EmitValueChangedSignal();
    }


    // called when page up gets pressed
    void DoubleSpinboxButton::OnPageUpPressed()
    {
        // add a single step to the value
        double newValue = mSpinbox->value() + mSpinbox->singleStep() * 5;

        // check if the new value is out of range
        if (newValue > mSpinbox->maximum())
        {
            newValue = mSpinbox->maximum();
        }

        // fire value changed events
        mSpinbox->setValue(newValue);
        mSpinbox->EmitValueChangedSignal();
    }


    // called when page down gets pressed
    void DoubleSpinboxButton::OnPageDownPressed()
    {
        // remove a single step from the value
        double newValue = mSpinbox->value() - mSpinbox->singleStep() * 5;

        // check if the new value is out of range, don't emit any changes in this case
        if (newValue < mSpinbox->minimum())
        {
            newValue = mSpinbox->minimum();
        }

        // fire value changed events
        mSpinbox->setValue(newValue);
        mSpinbox->EmitValueChangedSignal();
    }


    // click the button
    void DoubleSpinboxButton::ClickButton()
    {
        if (mDownButton)
        {
            OnDownButtonClicked();
        }
        else
        {
            OnUpButtonClicked();
        }
    }


    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Specialized spinbox edit field
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    // constructor
    DoubleSpinboxLineEdit::DoubleSpinboxLineEdit(QWidget* parent, DoubleSpinBox* spinbox)
        : QLineEdit(parent)
    {
        //setFocusPolicy(Qt::WheelFocus);
        setFocusPolicy(Qt::StrongFocus);

        mSpinbox = spinbox;
    }


    // destructor
    DoubleSpinboxLineEdit::~DoubleSpinboxLineEdit()
    {
    }


    // called when the line edit lost focus
    void DoubleSpinboxLineEdit::focusOutEvent(QFocusEvent* event)
    {
        FromQtString(text(), &mTemp);
        AzFramework::StringFunc::TrimWhiteSpace(mTemp, true, true);
        AzFramework::StringFunc::Replace(mTemp, MCore::CharacterConstants::comma, MCore::CharacterConstants::dot);

        if (mTemp.empty())
        {
            mSpinbox->Update();
        }

        QLineEdit::focusOutEvent(event);
    }


    // called when the edit field got double clicked
    void DoubleSpinboxLineEdit::mouseDoubleClickEvent(QMouseEvent* event)
    {
        QLineEdit::mouseDoubleClickEvent(event);
        selectAll();
    }




    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Finally the spinbox itself
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    // constructor
    DoubleSpinBox::DoubleSpinBox(QWidget* parent)
        : QWidget(parent)
    {
        mValue                                  = 0.0;
        mSingleStep                             = 1.0;
        mMinimum                                = 0.0;
        mMaximum                                = 99.99;
        mNumDecimals                            = 2;
        mMouseMoveSpinningAllowed               = true;
        mValueChangedSignalOnMouseMoveDisabled  = false;

        // create the edit field
        mLineEdit = new DoubleSpinboxLineEdit(this, this);
        mLineEdit->setValidator(new QRegExpValidator(QRegExp("-?[0-9]*[.,]?[0-9]{,}"), this));
        mLineEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

        connect(mLineEdit, &QLineEdit::editingFinished, this, &MysticQt::DoubleSpinBox::OnEditingFinished);
        connect(mLineEdit, &QLineEdit::returnPressed, this, &MysticQt::DoubleSpinBox::OnEditingFinished);
        connect(mLineEdit, &QLineEdit::textEdited, this, &MysticQt::DoubleSpinBox::OnTextEdited);

        // create the up and down spin buttons
        mUpButton   = new DoubleSpinboxButton(this, this, false);
        mDownButton = new DoubleSpinboxButton(this, this, true);

        // the vertical layout where we put the spin buttons in
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        vLayout->setSpacing(0);

        vLayout->addWidget(mUpButton);
        vLayout->addWidget(mDownButton);

        mSpinButtonWidget = new QWidget();
        mSpinButtonWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        mSpinButtonWidget->setLayout(vLayout);

        // the main horizontal layout
        QHBoxLayout* layout = new QHBoxLayout();

        layout->setMargin(0);
        layout->setSpacing(0);
        layout->addWidget(mLineEdit);
        layout->addWidget(mSpinButtonWidget);

        setLayout(layout);

        setMaximumHeight(20);

        //setFocusPolicy(Qt::WheelFocus);
        setFocusPolicy(Qt::StrongFocus);

        // get the visuals up to date
        Update();
    }


    void DoubleSpinBox::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);
        mSpinButtonWidget->setMinimumHeight(mLineEdit->height());
        mSpinButtonWidget->setMaximumHeight(mLineEdit->height());
    }


    // destructor
    DoubleSpinBox::~DoubleSpinBox()
    {
    }


    // update the interface
    void DoubleSpinBox::Update()
    {
        // convert the value to string
        QString valueString = QString::number(mValue, 'f', mNumDecimals);

        // remove trailing zeros
        if (valueString.contains('.'))
        {
            valueString.remove(QRegExp("0+$"));
        }
        else
        {
            valueString += '.';
        }

        // add one zero if the text ends with one dot
        if (valueString.endsWith('.'))
        {
            valueString += "0";
        }

        // update the line edit
        mLineEdit->setText(valueString);
        mLineEdit->setStyleSheet("");
        mLineEdit->setCursorPosition(0);

        // enable or disable the up and down buttons based on the value and its valid range
        mUpButton->SetButtonEnabled(mValue < mMaximum);
        mDownButton->SetButtonEnabled(mValue > mMinimum);
    }


    // called each time when a new character has been entered or removed, this doesn't update the value yet
    void DoubleSpinBox::OnTextEdited(const QString& newText)
    {
        // get the value string from the line edit
        FromQtString(newText, &mTemp);
        AzFramework::StringFunc::TrimWhiteSpace(mTemp, true, true);
        AzFramework::StringFunc::Replace(mTemp, MCore::CharacterConstants::comma, MCore::CharacterConstants::dot);

        // check if the text is a valid value
        float newValue;
        int newValueInt;
        if (AzFramework::StringFunc::LooksLikeFloat(mTemp.c_str(), &newValue))
        {
            UpdateStyleForRangeCheck(newValue);
        }
        else if (AzFramework::StringFunc::LooksLikeInt(mTemp.c_str(), &newValueInt))
        {
            newValue = static_cast<float>(newValueInt);
            UpdateStyleForRangeCheck(newValue);
        }
        else
        {
            SetStyleToError();
        }
    }

    void DoubleSpinBox::SetStyleToError()
    {
        mLineEdit->setStyleSheet("color: red;");
    }

    void DoubleSpinBox::UpdateStyleForRangeCheck(float newValue)
    {
        // check if the value is in range
        if (newValue >= mMinimum && newValue <= mMaximum)
        {
            SetStyleToOk();
        }
        else
        {
            SetStyleToError();
        }
    }

    void DoubleSpinBox::SetStyleToOk()
    {
        mLineEdit->setStyleSheet("");
    }


    // called when the user pressed enter, tab or we lost focus
    void DoubleSpinBox::OnEditingFinished()
    {
        // get the value string from the line edit
        FromQtString(mLineEdit->text(), &mTemp);
        AzFramework::StringFunc::TrimWhiteSpace(mTemp, true, true);
        AzFramework::StringFunc::Replace(mTemp, MCore::CharacterConstants::comma, MCore::CharacterConstants::dot);

        bool inputFormatError = false;
        // check if the text is an invalid value
        float newValue;
        int newValueInt;
        if ( !AzFramework::StringFunc::LooksLikeFloat(mTemp.c_str(), &newValue) )
        {
            if (!AzFramework::StringFunc::LooksLikeInt(mTemp.c_str(), &newValueInt))
            {
                inputFormatError = true;
            }
            else
            {
                newValue = static_cast<float>(newValueInt);
            }
        }

        if (inputFormatError)
        {
            // reset the value to the last valid and used one
            setValue(mValue);
            emit valueChanged(mValue);
            return;
        }

        // in case the new value is out of range, use the last valid value
        if (newValue > mMaximum || newValue < mMinimum)
        {
            newValue = mValue;
        }

        // fire value changed events
        if (newValue != mValue)
        {
            setValue(newValue);
            emit valueChanged(newValue);
            emit ValueChangedByTextFinished(newValue);
        }
    }


    // set a new value to the widget, this doesn't fire value changed events
    void DoubleSpinBox::setValue(double value, bool rangeCheck, bool update)
    {
        // check if we need to change if the given value is in range
        if (rangeCheck)
        {
            // if the value is too big, set it to the maximum
            if (value > mMaximum)
            {
                value = mMaximum;
            }

            // if the value is too small, set it to the minimum
            if (value < mMinimum)
            {
                value = mMinimum;
            }
        }

        // adjust the value
        mValue = value;

        // show the new value in the interface
        if (update)
        {
            Update();
        }
    }


    // called when the wheel rolled up or down
    /*void DoubleSpinBox::wheelEvent(QWheelEvent* event)
    {
        // only handle the mouse wheel when the spinbox has focus
        if (mLineEdit->hasFocus() == false)
        {
            event->ignore();
            return;
        }

        // convert the rotated mouse wheel to a delta value and add a single step to the value
        const int delta = event->delta() / 120;
        double newValue = mValue + mSingleStep * delta;

        // if the value is too big, set it to the maximum
        if (newValue > mMaximum)
            newValue = mMaximum;

        // if the value is too small, set it to the minimum
        if (newValue < mMinimum)
            newValue = mMinimum;

        // fire value changed events
        setValue(newValue);
        emit valueChanged(newValue);
        event->accept();
    }*/


    // called when a keyboard button got pressed
    void DoubleSpinBox::keyPressEvent(QKeyEvent* event)
    {
        int32 key = event->key();

        // handle decreasing the value
        if (key == Qt::Key_Down)
        {
            mDownButton->ClickButton();
        }

        if (key == Qt::Key_PageDown)
        {
            mDownButton->OnPageDownPressed();
        }

        // handle increasing the value
        if (key == Qt::Key_Up)
        {
            mUpButton->ClickButton();
        }

        if (key == Qt::Key_PageUp)
        {
            mUpButton->OnPageUpPressed();
        }

        event->accept();
    }


    // just for completelyness and accepting the key events
    void DoubleSpinBox::keyReleaseEvent(QKeyEvent* event)
    {
        event->accept();
    }
} // namespace MysticQt

#include <MysticQt/Source/DoubleSpinbox.moc>