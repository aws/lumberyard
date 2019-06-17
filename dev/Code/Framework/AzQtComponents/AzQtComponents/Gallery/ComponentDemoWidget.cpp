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

#include "AssetBrowserFolderPage.h"
#include "BreadCrumbsPage.h"
#include "BrowseEditPage.h"
#include "ButtonPage.h"
#include "CardPage.h"
#include "CheckBoxPage.h"
#include "ColorPickerPage.h"
#include "ComboBoxPage.h"
#include "GradientSliderPage.h"
#include "LineEditPage.h"
#include "ProgressIndicatorPage.h"
#include "RadioButtonPage.h"
#include "SegmentControlPage.h"
#include "SliderPage.h"
#include "SvgLabelPage.h"
#include "TabWidgetPage.h"
#include "ToggleSwitchPage.h"
#include "TypographyPage.h"
#include "SpinBoxPage.h"
#include "StyleSheetPage.h"
#include "FilteredSearchWidgetPage.h"
#include "TableViewPage.h"
#include "StyledDockWidgetPage.h"
#include "TitleBarPage.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMap>

const QString g_pageIndexSettingKey = QStringLiteral("ComponentDemoWidgetPage");

ComponentDemoWidget::ComponentDemoWidget(bool legacyUISetting, QWidget* parent)
: QMainWindow(parent)
, ui(new Ui::ComponentDemoWidget)
{
    ui->setupUi(this);

    setupMenuBar(legacyUISetting);

    QMap<QString, QWidget*> sortedPages;

    SpinBoxPage* spinBoxPage = new SpinBoxPage(this);

    sortedPages.insert("Buttons", new ButtonPage(this));
    sortedPages.insert("ToggleSwitches", new ToggleSwitchPage(this));
    sortedPages.insert("Cards", new CardPage(this));
    sortedPages.insert("CheckBoxes", new CheckBoxPage(this));
    sortedPages.insert("ProgressIndicators", new ProgressIndicatorPage(this));
    sortedPages.insert("RadioButtons", new RadioButtonPage(this));
    sortedPages.insert("Sliders", new SliderPage(this));
    sortedPages.insert("GradientSliders", new GradientSliderPage(this));
    sortedPages.insert("Color Picker", new ColorPickerPage(this));
    sortedPages.insert("BrowseEdits", new BrowseEditPage(this));
    sortedPages.insert("Typography", new TypographyPage(this));
    sortedPages.insert("BreadCrumbs", new BreadCrumbsPage(this));
    sortedPages.insert("LineEdits", new LineEditPage(this));
    sortedPages.insert("SegmentControls", new SegmentControlPage(this));
    sortedPages.insert("ComboBoxes", new ComboBoxPage(this));
    sortedPages.insert("SpinBox", spinBoxPage);
    sortedPages.insert("SVGLabel", new SvgLabelPage(this));
    sortedPages.insert("Qt Style Sheets", new StyleSheetPage(this));
    sortedPages.insert("TabWidget", new TabWidgetPage(this));
    sortedPages.insert("AssetBrowserFolder", new AssetBrowserFolderPage(this));
    sortedPages.insert("FilteredSearchWidget", new FilteredSearchWidgetPage(this));
    sortedPages.insert("TableView", new TableViewPage(this));
    sortedPages.insert("StyledDockWidget", new StyledDockWidgetPage(this));
    sortedPages.insert("TitleBar", new TitleBarPage(this));

    for (const auto& title : sortedPages.keys())
    {
        addPage(sortedPages.value(title), title);
    }

    connect(ui->demoSelector, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), ui->demoWidgetStack, [this, spinBoxPage](int newIndex) {
        ui->demoWidgetStack->setCurrentIndex(newIndex);

        QString demoSelectorText = ui->demoSelector->currentText();

        QSettings settings;
        settings.setValue(g_pageIndexSettingKey, demoSelectorText);

        m_editMenu->clear();
        if (ui->demoWidgetStack->currentWidget() == spinBoxPage)
        {
            QAction* undo = spinBoxPage->getUndoStack().createUndoAction(m_editMenu);
            undo->setShortcut(QKeySequence::Undo);
            m_editMenu->addAction(undo);

            QAction* redo = spinBoxPage->getUndoStack().createRedoAction(m_editMenu);
            redo->setShortcut(QKeySequence::Redo);
            m_editMenu->addAction(redo);
        }
        else
        {
            createEditMenuPlaceholders();
        }
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

void ComponentDemoWidget::setupMenuBar(bool legacyUISetting)
{
    auto fileMenu = menuBar()->addMenu("&File");

    auto styleToggle = fileMenu->addAction("Enable UI 1.0");
    styleToggle->setShortcut(QKeySequence("Ctrl+T"));
    styleToggle->setCheckable(true);
    styleToggle->setChecked(legacyUISetting);
    QObject::connect(styleToggle, &QAction::toggled, this, &ComponentDemoWidget::styleChanged);

    QAction* refreshAction = fileMenu->addAction("Refresh Stylesheet");
    QObject::connect(refreshAction, &QAction::triggered, this, &ComponentDemoWidget::refreshStyle);
    fileMenu->addSeparator();

    m_editMenu = menuBar()->addMenu("&Edit");
    createEditMenuPlaceholders();

#if defined(Q_OS_MACOS)
    QAction* quitAction = fileMenu->addAction("&Quit");
#else
    QAction* quitAction = fileMenu->addAction("E&xit");
#endif
    quitAction->setShortcut(QKeySequence::Quit);
    QObject::connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
}

void ComponentDemoWidget::createEditMenuPlaceholders()
{
    QAction* undo = m_editMenu->addAction("&Undo");
    undo->setDisabled(true);
    undo->setShortcut(QKeySequence::Undo);

    QAction* redo = m_editMenu->addAction("&Redo");
    redo->setDisabled(true);
    redo->setShortcut(QKeySequence::Redo);
}

#include <Gallery/ComponentDemoWidget.moc>
