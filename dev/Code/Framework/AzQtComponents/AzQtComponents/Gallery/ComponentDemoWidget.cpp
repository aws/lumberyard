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
#include "CardPage.h"

#include <QMap>

const QString g_pageIndexSettingKey = QStringLiteral("ComponentDemoWidgetPage");

ComponentDemoWidget::ComponentDemoWidget(QWidget* parent)
: QMainWindow(parent)
, ui(new Ui::ComponentDemoWidget)
{
    ui->setupUi(this);

    QMap<QString, QWidget*> sortedPages;

    sortedPages.insert("Buttons", new ButtonPage(this));
    sortedPages.insert("ToggleSwitches", new ToggleSwitchPage(this));
    sortedPages.insert("Cards", new CardPage(this));
    sortedPages.insert("CheckBoxes", new CheckBoxPage(this));
    sortedPages.insert("ProgressIndicators", new ProgressIndicatorPage(this));
    sortedPages.insert("RadioButtons", new RadioButtonPage(this));
    sortedPages.insert("Sliders", new SliderPage(this));

    for (const auto& title : sortedPages.keys())
    {
        addPage(sortedPages.value(title), title);
    }

    connect(ui->demoSelector, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), ui->demoWidgetStack, [this](int newIndex) {
        ui->demoWidgetStack->setCurrentIndex(newIndex);

        QString demoSelectorText = ui->demoSelector->currentText();

        QSettings settings;
        settings.setValue(g_pageIndexSettingKey, demoSelectorText);
    });

    QSettings settings;
    QString valueString = settings.value(g_pageIndexSettingKey, 0).toString();
    bool convertedToInt = false;
    int savedIndex = valueString.toInt(&convertedToInt);
    if (!convertedToInt)
    {
        savedIndex = ui->demoSelector->findText(valueString);
    }

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
