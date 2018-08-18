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

#include "BrowseEditPage.h"
#include <Gallery/ui_BrowseEditPage.h>

#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <QIntValidator>
#include <QInputDialog>
#include <QMessageBox>

BrowseEditPage::BrowseEditPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::BrowseEditPage)
{
    ui->setupUi(this);

    QIcon icon(":/stylesheet/img/question.png");

    ui->browseEditEnabled->setClearButtonEnabled(true);
    ui->browseEditEnabled->setAttachedButtonIcon(icon);
    ui->browseEditEnabled->setLineEditReadOnly(true);
    ui->browseEditEnabled->setPlaceholderText("Some placeholder");
    connect(ui->browseEditEnabled, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, [this]() {
        QString text = QInputDialog::getText(this, "Text Entry", "Enter a number, or some invalid text");
        ui->browseEditEnabled->setText(text);
    });

    ui->browseEditDisabled->setClearButtonEnabled(true);
    ui->browseEditDisabled->setLineEditReadOnly(true);
    ui->browseEditDisabled->setEnabled(false);
    ui->browseEditDisabled->setAttachedButtonIcon(icon);
    ui->browseEditDisabled->setText("Text");
    connect(ui->browseEditDisabled, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, [this]() {
        QString text = QInputDialog::getText(this, "Text Entry", "Enter a number, or some invalid text");
        ui->browseEditDisabled->setText(text);
    });

    ui->browseEditNumberReadOnly->setLineEditReadOnly(true);
    ui->browseEditNumberReadOnly->setValidator(new QIntValidator(this));
    ui->browseEditNumberReadOnly->setAttachedButtonIcon(icon);
    ui->browseEditNumberReadOnly->setToolTip("Click the attached button to enter a number. Put random characters in to put the control into error state.");
    ui->browseEditNumberReadOnly->setErrorToolTip("The control only accepts numbers!");
    ui->browseEditNumberReadOnly->setText("Nan");
    connect(ui->browseEditNumberReadOnly, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, [this]() {
        // pop up a dialog box and set the line edit based on the user's input
        QString text = QInputDialog::getText(this, "Text Entry", "Enter a number, or some invalid text");
        ui->browseEditNumberReadOnly->setText(text);
    });

    ui->browseEditNumberNonReadOnly->setValidator(new QIntValidator(this));
    ui->browseEditNumberNonReadOnly->setClearButtonEnabled(true);
    ui->browseEditNumberNonReadOnly->setAttachedButtonIcon(icon);
    ui->browseEditNumberNonReadOnly->setToolTip("Click the attached button to enter a number. Put random characters in to put the control into error state.");
    ui->browseEditNumberNonReadOnly->setErrorToolTip("The control only accepts numbers!");
    connect(ui->browseEditNumberNonReadOnly, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, [this]() {
        // pop up a dialog box and set the line edit based on the user's input
        QMessageBox::critical(this, "title", "Button attach worked!");
    });

    QString exampleText =
R"(


)";

    ui->exampleText->setHtml(exampleText);
}

BrowseEditPage::~BrowseEditPage()
{
}

#include <Gallery/BrowseEditPage.moc>
