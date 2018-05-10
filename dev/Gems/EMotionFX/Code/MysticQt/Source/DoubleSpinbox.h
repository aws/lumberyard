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

#ifndef __MYSTICQT_DOUBLESPINBOX_H
#define __MYSTICQT_DOUBLESPINBOX_H

#include <MCore/Source/StandardHeaders.h>
#include "MysticQtConfig.h"
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QToolButton>


namespace MysticQt
{
    // forward declaration
    class DoubleSpinBox;

    /**
     * Custom spin up and down button implementation
     */
    class MYSTICQT_API DoubleSpinboxButton
        : public QToolButton
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(DoubleSpinboxButton, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS)

    public:
        DoubleSpinboxButton(QWidget* parent, DoubleSpinBox* spinbox, bool downButton);
        virtual ~DoubleSpinboxButton();

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

        DoubleSpinBox* mSpinbox;

        int32   mPrevMouseY = 0;
        int32   mPressedMouseY = 0;
        bool    mMouseMoveSpinningEnabled = false;
        bool    mDownButton = false;
        bool    mEnabled = false;
    };


    /**
     * Specialized spinbox edit field
     */
    class MYSTICQT_API DoubleSpinboxLineEdit
        : public QLineEdit
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(DoubleSpinboxLineEdit, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS)

    public:
        DoubleSpinboxLineEdit(QWidget* parent, DoubleSpinBox* spinbox);
        virtual ~DoubleSpinboxLineEdit();

        void focusOutEvent(QFocusEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;

    private:
        DoubleSpinBox*  mSpinbox;
        AZStd::string   mTemp;
    };


    /**
     * Spinbox widget
     */
    class MYSTICQT_API DoubleSpinBox
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(DoubleSpinBox, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS)

    public:
        DoubleSpinBox(QWidget* parent = nullptr);
        ~DoubleSpinBox();

        void SetMouseMoveSpinningAllowed(bool flag)                             { mMouseMoveSpinningAllowed = flag; }
        bool GetMouseMoveSpinningAllowed() const                                { return mMouseMoveSpinningAllowed; }

        void SetValueChangedSignalOnMouseMoveDisabled(bool flag)                { mValueChangedSignalOnMouseMoveDisabled = flag; }
        bool GetValueChangedSignalOnMouseMoveDisabled() const                   { return mValueChangedSignalOnMouseMoveDisabled; }

        void setDecimals(int numDecimals)                                       { mNumDecimals = numDecimals; Update(); }
        void setSingleStep(double deltaValue)                                   { mSingleStep = deltaValue; }
        void setValue(double value, bool rangeCheck = true, bool update = true);
        void setRange(double min, double max)                                   { mMinimum = min; mMaximum = max; Update(); }
        void setSuffix(const char* suffix)                                      { MCORE_UNUSED(suffix); }

        QLineEdit* GetLineEdit()                                                { return mLineEdit; }

        double value() const                                                    { return mValue; }
        double minimum() const                                                  { return mMinimum; }
        double maximum() const                                                  { return mMaximum; }
        double singleStep() const                                               { return mSingleStep; }
        int decimals() const                                                    { return mNumDecimals; }

        void Update();

        void EmitValueChangedSignal()                                           { emit valueChanged(mValue); }
        void EmitValueChangedMouseReleased()                                    { emit ValueChangedMouseReleased(mValue); }

        void resizeEvent(QResizeEvent* event) override;

    signals:
        void ValueChangedByTextFinished(double value);
        void ValueChangedMouseReleased(double value);
        void valueChanged(double d);
        void valueChanged(const QString& text);

    private slots:
        void OnEditingFinished();
        void OnTextEdited(const QString& newText);

    protected:
        //void wheelEvent(QWheelEvent* event);

        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        double                  mValue;
        double                  mMinimum;
        double                  mMaximum;
        double                  mSingleStep;
        int                     mNumDecimals;
        AZStd::string           mTemp;

        QLineEdit*              mLineEdit;
        QWidget*                mSpinButtonWidget;
        DoubleSpinboxButton*    mUpButton;
        DoubleSpinboxButton*    mDownButton;

        bool                    mMouseMoveSpinningAllowed;
        bool                    mValueChangedSignalOnMouseMoveDisabled;
    };
} // namespace MysticQt


#endif
