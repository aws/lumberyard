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
#include "QCustomSpinbox.h"

#include "qevent.h"
#include "QWidgetAction"
#include "PreviewModelView.h"
#include "PreviewWindowView.h"
#include "ContextMenu.h"
QCustomSpinbox::QCustomSpinbox(CPreviewWindowView* ptr)
    : m_comboDropdownMenu(new ContextMenu((QWidget*)ptr))
{
    m_WindowViewPtr = ptr;
    m_ModelViewPtr = m_WindowViewPtr->m_previewModelView;
    setValue(1.0f);
    setMinimum(0);
    setMaximum(100.0f);
    setSingleStep(0.01);
    setMinimumWidth(50);
    setMaximumWidth(50);
    this->setStyleSheet("QDoubleSpinBox{ color: white }");

    m_comboSlider = new QSlider(Qt::Horizontal, this);
    m_comboSlider->setMinimumWidth(120);
    m_comboSlider->setMinimumHeight(30);
    m_comboSlider->setValue(100.0);
    m_comboSlider->setMaximum(1000.0);
    m_comboSlider->setMinimum(0.0);

    QString comboSliderStyle = "QSlider::handle:horizontal {background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f);"
        "border: 1px solid #5c5c5c;width: 18px;"
        "margin: -2px 0; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */"
        "border-radius: 3px;"
        "}";

    comboSliderStyle.append("QSlider::add-page:horizontal {background: white;}");
    comboSliderStyle.append("QSlider::sub-page:horizontal {background: grey;}");
    m_comboSlider->setStyleSheet(comboSliderStyle);

    m_act = new QWidgetAction(this);
    m_act->setDefaultWidget(m_comboSlider);
    m_comboDropdownMenu->addAction(m_act);

    connect(this, SIGNAL(valueChanged(double)), this, SLOT(onPlaySpeedButtonValueChanged(double)));
    connect(m_comboSlider, SIGNAL(valueChanged(int)), this, SLOT(comboSliderValueChanged(int)));
}


QCustomSpinbox::~QCustomSpinbox()
{
}

void QCustomSpinbox::onPlaySpeedButtonValueChanged(double value)
{
    if (m_ModelViewPtr->m_pEmitter != NULL)
    {
        if (value >= 10)
        {
            this->setValue(10);
        }
        else if (value <= 0)
        {
            this->setValue(0);
        }
        else
        {
            this->setValue(value);
        }
        m_ModelViewPtr->m_timeScale = value;
    }
    else
    {
        this->setValue(1.00);
    }
}

void QCustomSpinbox::comboSliderValueChanged(int value)
{
    if (m_ModelViewPtr->m_pEmitter != NULL)
    {
        this->setValue(value / 100.0f);
        m_ModelViewPtr->m_timeScale = value / 100.0f;
    }
    else
    {
        this->setValue(1.00);
    }
}

#ifdef EDITOR_QT_UI_EXPORTS
#include <QCustomSpinbox.moc>
#endif
