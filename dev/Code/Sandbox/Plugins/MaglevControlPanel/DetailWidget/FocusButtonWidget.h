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

#include <QPushButton>

// Button that signals when it has gained/lost focus
class FocusButtonWidget
    : public QPushButton
{
    Q_OBJECT

public:
    FocusButtonWidget();
    FocusButtonWidget(const QString& title);

private:
    // Events
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

    // Helpers
    void HandleGainFocus();
    void HandleLoseFocus();

    // Track focus since it can come and go from different sources (mouse, keyboard). Note there's only one Qt GUI thread.
    int m_numFocusGains;

signals:
    void FocusGained();
    void FocusLost();
};