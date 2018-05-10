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

#ifndef __MYSTICQT_INTSPINBOX_H
#define __MYSTICQT_INTSPINBOX_H

//
#include <MCore/Source/StandardHeaders.h>
#include "MysticQtConfig.h"
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QToolButton>


namespace MysticQt
{
    // forward declaration
    class IntSpinBox;

    /**
     * Custom spin up and down button implementation
     */
    class MYSTICQT_API IntSpinboxButton
        : public QToolButton
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(IntSpinboxButton, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS)

    public:
        IntSpinboxButton(QWidget* parent, IntSpinBox* spinbox, bool downButton);
        virtual ~IntSpinboxButton();

        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void SetButtonEnabled(bool enabled);
        bool GetButtonEnabled() const                   { return mEnabled; }

        void ClickButton();
        void OnPageDownPressed();
        void OnPageUpPressed();

    private:
        void OnUpButtonClicked();
        void OnDownButtonClicked();

        IntSpinBox* mSpinbox;

        double  mMouseMoveDeltaValue;
        int32   mPrevMouseY;
        int32   mPressedMouseY;
        bool    mMouseMoveSpinningEnabled;
        bool    mDownButton;
        bool    mEnabled;
    };


    /**
     * Specialized spinbox edit field
     */
    class MYSTICQT_API IntSpinboxLineEdit
        : public QLineEdit
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(IntSpinboxLineEdit, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS)

    public:
        IntSpinboxLineEdit(QWidget* parent, IntSpinBox* spinbox);
        virtual ~IntSpinboxLineEdit();

        void focusOutEvent(QFocusEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;

    private:
        IntSpinBox*     mSpinbox;
        AZStd::string   mTemp;
    };


    /**
     * Spinbox widget
     */
    class MYSTICQT_API IntSpinBox
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(IntSpinBox, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS)

    public:
        IntSpinBox(QWidget* parent = nullptr);
        ~IntSpinBox();

        void SetMouseMoveSpinningAllowed(bool flag)                         { mMouseMoveSpinningAllowed = flag; }
        bool GetMouseMoveSpinningAllowed() const                            { return mMouseMoveSpinningAllowed; }

        void SetValueChangedSignalOnMouseMoveDisabled(bool flag)            { mValueChangedSignalOnMouseMoveDisabled = flag; }
        bool GetValueChangedSignalOnMouseMoveDisabled() const               { return mValueChangedSignalOnMouseMoveDisabled; }

        void setSingleStep(int deltaValue)                                  { mSingleStep = deltaValue; }
        void setValue(int value, bool rangeCheck = true, bool update = true);
        void setRange(int min, int max)                                     { mMinimum = min; mMaximum = max; Update(); }
        void setSuffix(const char* suffix)                                  { MCORE_UNUSED(suffix); }

        int value() const                                                   { return mValue; }
        int minimum() const                                                 { return mMinimum; }
        int maximum() const                                                 { return mMaximum; }
        int singleStep() const                                              { return mSingleStep; }

        void Update();

        void EmitValueChangedSignal()                                       { emit valueChanged(mValue); }
        void EmitValueChangedMouseReleased()                                { emit ValueChangedMouseReleased(mValue); }

        void resizeEvent(QResizeEvent* event) override;

    signals:
        void ValueChangedByTextFinished(int value);
        void ValueChangedMouseReleased(int value);
        void valueChanged(int d);
        void valueChanged(const QString& text);

    private slots:
        void OnEditingFinished();
        void OnTextEdited(const QString& newText);

    protected:
        //void wheelEvent(QWheelEvent* event);

        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        int                     mValue;
        int                     mMinimum;
        int                     mMaximum;
        int                     mSingleStep;
        AZStd::string           mTemp;

        QLineEdit*              mLineEdit;
        QWidget*                mSpinButtonWidget;
        IntSpinboxButton*       mUpButton;
        IntSpinboxButton*       mDownButton;

        bool                    mMouseMoveSpinningAllowed;
        bool                    mValueChangedSignalOnMouseMoveDisabled;
    };
} // namespace MysticQt


#endif
