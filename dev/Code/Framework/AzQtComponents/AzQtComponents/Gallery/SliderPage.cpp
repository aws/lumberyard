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

#include "SliderPage.h"
#include <Gallery/ui_SliderPage.h>

#include <AzQtComponents/Components/Widgets/Slider.h>

SliderPage::SliderPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::SliderPage)
{
    ui->setupUi(this);

    QString exampleText = R"(

A Slider is a wrapper around a QSlider.<br/>
There are two variants: SliderInt and SliderDouble, for working with signed integers and doubles, respectively.<br/>
They add tooltip functionality, as well as conversion to the proper data types (int and double).
<br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/Slider.h&gt;
#include &lt;QDebug&gt;

// Here's an example that creates a slider and sets the hover/tooltip indicator to display as a percentage
SliderInt* sliderInt = new SliderInt();
sliderInt->setRange(0, 100);
sliderInt->setToolTipFormatting("", "%");

// Assuming you've created a slider already (either in code or via .ui file), give it the mid point style like this:
Slider::applyMidPointStyle(sliderInt);

// Disable it like this:
sliderInt->setEnabled(false);

// Here's an example of creating a SliderDouble and setting it up:
SliderDouble* sliderDouble = new SliderDouble();
double min = -10.0;
double max = 10.0;
int numSteps = 21;
sliderDouble->setRange(min, max, numSteps);
sliderDouble->setValue(0.0);

// Listen for changes; same format for SliderInt as SliderDouble
connect(sliderDouble, &SliderDouble::valueChanged, sliderDouble, [sliderDouble](double newValue){
    qDebug() << "Slider value changed to " << newValue;
});

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

SliderPage::~SliderPage()
{
}

#include <Gallery/SliderPage.moc>