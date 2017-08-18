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
#pragma once
#include <QDoubleSpinBox>

/*used to get access to line edit so we can select all text on double click*/
class QAmazonDoubleSpinBox
    : public QDoubleSpinBox
{
    Q_OBJECT

public:
    QAmazonDoubleSpinBox(QWidget* parent = nullptr);
    virtual ~QAmazonDoubleSpinBox();
           
    bool event(QEvent* event) override;

protected:
    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual void focusInEvent(QFocusEvent* event) override;
    virtual void focusOutEvent(QFocusEvent* event) override;
    virtual bool eventFilter(QObject*, QEvent*) override;
    virtual QString textFromValue(double val) const override;
    virtual double valueFromText(const QString& text) const override;

signals:
    void spinnerDragged();
    void spinnerDragFinished();

private:
    void MouseEvent(QEvent* event);

    // Even though this is a "Double" Spin Box, we currently only use it with Vec4 which stores floats.
    // The max precision is most 7 since float will not be accurate after 7 decimal.
    const int m_precision = 7;

    // The max number of decimals places should be at most 7 since float will not be accurate after 7 decimal. 
    // Using 6 prevents numbers like 3.4 from showing up as 3.4000001
    const int m_decimalsLong = 6;

    // In some cases we reduce to fewer decimal place for a cleaner visual style.
    const int m_decimalsShort = 4;

    // 'f' Prevents scientific notation
    const char m_decimalFormat = 'f';

    bool m_isEditInProgress = false;
    bool m_isFocusedIn = false;
    double m_value = 0.0;
    QPoint m_lastMousePosition;
    bool m_mouseCaptured = false;
    bool m_mouseDragging = false;
};