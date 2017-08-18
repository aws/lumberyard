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
#include "QSpinBox"
#include "QMenu"
#include "QSlider"
#include "QWidgetAction"
class CPreviewModelView;
class CPreviewWindowView;
class QCustomSpinbox
    : public QDoubleSpinBox
{
    Q_OBJECT
public:
    QCustomSpinbox(CPreviewWindowView* ptr);
    ~QCustomSpinbox();

    void onPlaySpeedButtonClicked(bool hasFocus);
    QMenu* m_comboDropdownMenu;
    QSlider* m_comboSlider;
    QWidgetAction* m_act;
    CPreviewModelView* m_ModelViewPtr;
    CPreviewWindowView* m_WindowViewPtr;
private Q_SLOTS:
    void onPlaySpeedButtonValueChanged(double value);
    void comboSliderValueChanged(int value);
};

