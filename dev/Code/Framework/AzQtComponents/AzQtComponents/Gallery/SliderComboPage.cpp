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

#include "SliderComboPage.h"
#include <Gallery/ui_SliderComboPage.h>

#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzQtComponents/Components/Widgets/SliderCombo.h>

SliderComboPage::SliderComboPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::SliderComboPage)
{
    ui->setupUi(this);

    ui->verticalSliderComboWithoutValue->setRange(0, 100);

    ui->verticalSliderCombo->setRange(0, 100);
    ui->verticalSliderCombo->setValue(50);

    ui->verticalSliderDoubleComboWithoutValue->setRange(0.5, 250.5);

    ui->verticalSliderDoubleCombo->setRange(0.5, 250.5);
    ui->verticalSliderDoubleCombo->setValue(100.0);


    QString exampleText = R"(

A SliderCombo is a widget which combined a Slider and a spinbox.<br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/SliderCombo.h&gt;
#include &lt;QDebug&gt;

// Here's an example that creates a sliderCombo
SliderCombo* sliderCombo = new SliderCombo();
SliderCombo->setRange(0, 100); 


</pre>

)";


    ui->exampleText->setHtml(exampleText);
}

SliderComboPage::~SliderComboPage()
{
}

#include <Gallery/SliderComboPage.moc>
