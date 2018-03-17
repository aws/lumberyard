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

#include "ComponentDemoWidget.h"
#include <Gallery/ui_ComponentDemoWidget.h>

#include <QSettings>

#include "ButtonPage.h"
#include "RadioButtonPage.h"
#include "CheckBoxPage.h"
#include "ToggleSwitchPage.h"
#include "ProgressIndicatorPage.h"
#include "SliderPage.h"

const QString g_pageIndexSettingKey = QStringLiteral("ComponentDemoWidgetPage");

ComponentDemoWidget::ComponentDemoWidget(QWidget* parent)
: QMainWindow(parent)
, ui(new Ui::ComponentDemoWidget)
{
    ui->setupUi(this);

    addPage(new ButtonPage(this), "Buttons");
    addPage(new CheckBoxPage(this), "CheckBoxes");
    addPage(new RadioButtonPage(this), "RadioButtons");
    addPage(new ToggleSwitchPage(this), "ToggleSwitches");
    addPage(new ProgressIndicatorPage(this), "ProgressIndicators");
    addPage(new SliderPage(this), "Sliders");

    connect(ui->demoSelector, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), ui->demoWidgetStack, [this](int newIndex) {
        ui->demoWidgetStack->setCurrentIndex(newIndex);

        QSettings settings;
        settings.setValue(g_pageIndexSettingKey, newIndex);
    });

    QSettings settings;
    int savedIndex = settings.value(g_pageIndexSettingKey, 0).toInt();
    ui->demoSelector->setCurrentIndex(savedIndex);
}

ComponentDemoWidget::~ComponentDemoWidget()
{
}

void ComponentDemoWidget::addPage(QWidget* widget, const QString& title)
{
    ui->demoSelector->addItem(title);
    ui->demoWidgetStack->addWidget(widget);
}

#include <Gallery/ComponentDemoWidget.moc>
