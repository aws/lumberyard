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

#include "ButtonPage.h"
#include <Gallery/ui_ButtonPage.h>

#include <AzQtComponents/Components/Widgets/PushButton.h>

#include <QMenu>
#include <QPushButton>

ButtonPage::ButtonPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::ButtonPage)
{
    ui->setupUi(this);

    QIcon icon(":/stylesheet/img/question.png");

    ui->buttonWithIconEnabled->setIcon(icon);
    ui->buttonWithIconDisabled->setIcon(icon);
    AzQtComponents::PushButton::applySmallIconStyle(ui->buttonWithIconEnabled);
    AzQtComponents::PushButton::applySmallIconStyle(ui->buttonWithIconDisabled);

    QMenu* menu = new QMenu(this);
    menu->addAction("Option 1");
    menu->addAction("Option 2");
    menu->addSeparator();
    menu->addAction("Option 3");
    menu->addAction("Option 4");
    menu->addAction("Option 5");
    menu->addSeparator();
    menu->addAction("Option 6");
    menu->addAction("Option 7");
    menu->addAction("Option 8");
    menu->addAction("Option 9");
    menu->addSeparator();
    menu->addAction("A long string of characters");

    ui->buttonWithDropDownEnabled->setMenu(menu);
    ui->buttonWithDropDownDisabled->setMenu(menu);

    ui->buttonWithDropDownAndIconEnabled->setMenu(menu);
    ui->buttonWithDropDownAndIconEnabled->setIcon(icon);
    AzQtComponents::PushButton::applySmallIconStyle(ui->buttonWithDropDownAndIconEnabled);

    ui->buttonWithDropDownAndIconDisabled->setMenu(menu);
    ui->buttonWithDropDownAndIconDisabled->setIcon(icon);
    AzQtComponents::PushButton::applySmallIconStyle(ui->buttonWithDropDownAndIconDisabled);

    QString exampleText = R"(

QPushButton docs: <a href="http://doc.qt.io/qt-5/qpushbutton.html">http://doc.qt.io/qt-5/qpushbutton.html</a><br/>
QToolButton docs: <a href="http://doc.qt.io/qt-5/qpushbutton.html">http://doc.qt.io/qt-5/qtoolbutton.html</a><br/>

<pre>
#include &lt;QPushButton&gt;
#include &lt;QToolButton&gt;
#include &lt;AzQtComponents/Components/Widgets/PushButton.h&gt;

QPushButton* pushButton;

// Assuming you've created a QPushButton already (either in code or via .ui file):

// To make a button into a "Primary" button:
pushButton->setAutoDefault(true);

// To set a drop down menu, just do it the normal Qt way:
QMenu* menu;
pushButton->setMenu(menu);

// To disable the button
pushButton->setEnabled(false);

// Assuming you've created a QToolButton already (either in code or via .ui file):
QIcon icon(":/stylesheet/img/question.png");
QToolButton* smallIconButton;
smallIconButton->setIcon(icon);

// To make the button into a small icon button:
PushButton::applySmallIconStyle(smallIconButton);

// disabling a button and adding a menu work exactly the same way between QToolButton and QPushButton

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

ButtonPage::~ButtonPage()
{
}

#include <Gallery/ButtonPage.moc>