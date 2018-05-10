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

#include "CardPage.h"
#include <Gallery/ui_CardPage.h>

#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardNotification.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Style.h>
#include <QTextEdit>
#include <QPushButton>
#include <QAction>
#include <QMenu>

#include <QVBoxLayout>
#include <QFrame>

CardPage::CardPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::CardPage)
{
    ui->setupUi(this);

    QString exampleText = R"(

A Card is a container widget that can be expanded and collapsed via an arrow in the title bar. The title bar is represented by a CardHeader object, which consists of an arrow button to expand/collapse, an icon, title text, a help button, a warning icon and a button to trigger the drop down menu.<br/>
<br/>
Example:<br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/Card.h&gt;
#include &lt;AzQtComponents/Components/Widgets/CardHeader.h&gt;
#include &lt;AzQtComponents/Components/Widgets/CardNotification.h&gt;

// To use a Card:
AzQtComponents::Card* card; // assume one is created, either in code or via a .ui file, done in Qt Designer

card->setTitle("Basic Card");

QWidget* mainCardContentWidget = createContentWidget();
card->setContentWidget(mainCardContentWidget);

// Set a secondary content widget, for storing an extra set of options
card->setSecondaryTitle("Advanced Options");

QWidget* secondaryContentWidget = createSecondaryWidget();
card->setSecondaryContentWidget(secondaryContentWidget);

// Put an icon into the header
AzQtComponents::CardHeader* header = ui->functionalCard->header();
header->setIcon(QIcon(":/Cards/img/UI20/Cards/slice_item.png"));

// Set the help url to open in a browser if the user clicks the help button
header->setHelpURL("https://aws.amazon.com/documentation/lumberyard/");

// Clear the help url
header->clearHelpURL();

// Populate the menu that pops up when the header bar is right clicked or when the user clicks on the menu button
connect(card, &AzQtComponents::Card::contextMenuRequested, this, [](const QPoint& pos){
    QMenu menu;

    menu.addAction(new QAction("Example Menu Item"));
    
    menu.exec(point);
});

// Add a warning feature to the bottom of the Card, with a button to resolve it
AzQtComponents::CardNotification* notification = card->addNotification("Warning message!");
QPushButton* button = notification->addButtonFeature("Clear Warning");
connect(button, &QPushButton::clicked, card, &AzQtComponents::Card::clearNotifications);

</pre>

)";

    ui->exampleText->setHtml(exampleText);

    ui->basicCard->setTitle("Example Card");

    QWidget* testWidget = new QWidget(ui->functionalCard);
    testWidget->setMaximumHeight(30);
    testWidget->setMinimumHeight(30);
    ui->basicCard->setContentWidget(testWidget);

    ui->functionalCard->setTitle("Card With Secondary Section");

    testWidget = new QWidget(ui->functionalCard);
    testWidget->setMaximumHeight(60);
    testWidget->setMinimumHeight(60);
    ui->functionalCard->setContentWidget(testWidget);

    // put in an example icon
    AzQtComponents::CardHeader* header = ui->functionalCard->header();
    header->setIcon(QIcon(":/Cards/img/UI20/Cards/slice_item.png"));
    header->setHelpURL("https://aws.amazon.com/documentation/lumberyard/");

    connect(ui->addButton, &QPushButton::clicked, this, &CardPage::addNotification);

    connect(ui->clearButton, &QPushButton::clicked, ui->functionalCard, &AzQtComponents::Card::clearNotifications);

    connect(ui->functionalCard, &AzQtComponents::Card::contextMenuRequested, this, &CardPage::showContextMenu);

    m_addNotification = new QAction("Add Notification", this);
    addAction(m_addNotification);
    connect(m_addNotification, &QAction::triggered, this, &CardPage::addNotification);

    m_clearNotification = new QAction("Clear Notifications", this);
    addAction(m_clearNotification);
    connect(m_clearNotification, &QAction::triggered, ui->functionalCard, &AzQtComponents::Card::clearNotifications);

    m_warningAction = new QAction("Show Warning Icon", this);
    m_warningAction->setCheckable(true);
    addAction(m_warningAction);
    connect(m_warningAction, &QAction::toggled, this, &CardPage::toggleWarning);


    auto card = ui->functionalCard;
    card->setSecondaryTitle("Advanced Options");

    QWidget* testWidget2 = new QWidget(card);
    testWidget2->setMaximumHeight(30);
    testWidget2->setMinimumHeight(30);
    card->setSecondaryContentWidget(testWidget2);
}

CardPage::~CardPage()
{
}

void CardPage::addNotification()
{
    AzQtComponents::CardNotification* notification = ui->functionalCard->addNotification("A warning message indicating a conflict or problem.");
    QPushButton* button = notification->addButtonFeature("Clear Warnings");
    connect(button, &QPushButton::clicked, ui->functionalCard, &AzQtComponents::Card::clearNotifications);
}

void CardPage::toggleWarning(bool checked)
{
    AzQtComponents::CardHeader* header = ui->functionalCard->header();
    header->setWarning(checked);
    m_warningAction->setChecked(checked);
}

void CardPage::showContextMenu(const QPoint& point)
{
    QMenu menu;

    menu.addAction(m_addNotification);
    menu.addAction(m_clearNotification);
    menu.addSeparator();
    menu.addAction(m_warningAction);

    if (!menu.actions().empty())
    {
        menu.exec(point);
    }
}
