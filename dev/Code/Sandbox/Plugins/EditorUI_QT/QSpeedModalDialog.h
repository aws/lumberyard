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
#ifndef QSpeedModalDialog_h__
#define QSpeedModalDialog_h__
#include "QDialog"
#include "QGridLayout"
#include "QLabel"
#include "QSlider"
#include "QPushButton"
#include "QSpinBox"
class QSpeedModalDialog
    : public QDialog
{
    Q_OBJECT
public:
    QSpeedModalDialog(QWidget* parent = 0, float PlaySpeed = 1.0f);
    ~QSpeedModalDialog();
    float GetSpeed();
protected:
    QGridLayout layout;
    QLabel      msg;
    QPushButton cancelBtn;
    QPushButton acceptBtn;
    QDoubleSpinBox spinBox;
    QSlider* slider;
    float speedControlResult;
protected slots:
    void sliderValueChanged(int);
    void spinBoxValueChanged(double);
};

#endif // QSpeedModalDialog_h__
