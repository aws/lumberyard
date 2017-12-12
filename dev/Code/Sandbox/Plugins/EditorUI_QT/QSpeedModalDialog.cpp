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
#include "QSpeedModalDialog.h"

QSpeedModalDialog::QSpeedModalDialog(QWidget* parent /*= 0*/, float PlaySpeed)
    : QDialog(parent)
    , layout(this)
    , cancelBtn(this)
    , acceptBtn(this)
    , msg(this)
{
    speedControlResult = PlaySpeed;
    setWindowTitle("Speed control");

    msg.setText(tr("Speed control"));
    msg.setAlignment(Qt::AlignLeft);
    layout.addWidget(&msg);

    spinBox.setValue(PlaySpeed);
    spinBox.setMaximum(10.0);
    spinBox.setSingleStep(0.01);
    layout.addWidget(&spinBox, 0, 3);

    slider = new QSlider(Qt::Horizontal, this);
    slider->setMinimumWidth(120);
    slider->setMinimumHeight(30);
    float sliderValue = 100.0f * PlaySpeed;
    slider->setValue(sliderValue);
    slider->setMaximum(1000.0);
    slider->setMinimum(0.0);

    QString sliderStyle = "QSlider::handle:horizontal {background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f);"
        "border: 1px solid #5c5c5c;width: 18px;"
        "margin: -2px 0; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */"
        "border-radius: 3px;"
        "}";

    sliderStyle.append("QSlider::add-page:horizontal {background: white;}");
    sliderStyle.append("QSlider::sub-page:horizontal {background: grey;}");
    slider->setStyleSheet(sliderStyle);
    layout.addWidget(slider, 1, 0, 1, 4);

    QString buttonStyleSheet("QPushButton{background: " + palette().background().color().name(QColor::HexArgb) + ";}");
    buttonStyleSheet.append("QPushButton:hover:!pressed{ background-color:grey}");

    acceptBtn.setText(tr("Accept"));
    acceptBtn.setStyleSheet(buttonStyleSheet);
    acceptBtn.setDefault(false);
    layout.addWidget(&acceptBtn, 2, 3);

    cancelBtn.setText(tr("Cancel"));
    cancelBtn.setStyleSheet(buttonStyleSheet);
    acceptBtn.setDefault(false);
    layout.addWidget(&cancelBtn, 2, 2);

    connect(&acceptBtn, &QPushButton::clicked, this, [=](){this->accept(); });
    connect(&cancelBtn, &QPushButton::clicked, this, [=](){this->reject(); });
    connect(&spinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxValueChanged(double)));
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
    spinBoxValueChanged(spinBox.value());
}


QSpeedModalDialog::~QSpeedModalDialog()
{
    delete slider;
}

float QSpeedModalDialog::GetSpeed()
{
    return speedControlResult;
}

void QSpeedModalDialog::sliderValueChanged(int value)
{
    spinBox.blockSignals(true);
    speedControlResult = value / 100.0f;
    spinBox.setValue(value / 100.0f);
    spinBox.blockSignals(false);
}

void QSpeedModalDialog::spinBoxValueChanged(double value)
{
    slider->blockSignals(true);
    speedControlResult = value;
    slider->setValue(value * 100);
    slider->blockSignals(false);
}

#ifdef EDITOR_QT_UI_EXPORTS
#include <QSpeedModalDialog.moc>
#endif

