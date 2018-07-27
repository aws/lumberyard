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

#include "LineEditPage.h"
#include <Gallery/ui_LineEditPage.h>

#include <QValidator>

#include <AzQtComponents/Components/Widgets/LineEdit.h>

class ErrorValidator : public QValidator
{
public:
    ErrorValidator(QObject* parent) : QValidator(parent) {}

    QValidator::State validate(QString& input, int& pos) const override
    {
        return QValidator::Invalid;
    }
};

LineEditPage::LineEditPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::LineEditPage)
{
    ui->setupUi(this);

    ui->defaultWithHintText->setPlaceholderText("Hint text");

    ui->withData->setText("Filled in data");
    ui->withData->setToolTip("<b></b>This will be a very very long sentence with an end but not for a long time, spanning an entire screen. I said, spanning an entire screen. No really. An entire screen. A very long, very wide screen.");
    ui->withData->setClearButtonEnabled(true);

    ui->error->setText("Error");
    ui->error->setValidator(new ErrorValidator(ui->error));
    ui->error->setClearButtonEnabled(true);

    ui->disabled->setDisabled(true);
    ui->disabled->setText("Disabled");

    AzQtComponents::LineEdit::applySearchStyle(ui->searchBox);
    ui->searchBox->setPlaceholderText("Search...");
    ui->searchBox->setClearButtonEnabled(true);

    ui->longSentence->setText("This will be a very very long sentence without end");
    ui->longSentence->setClearButtonEnabled(true);

    QString exampleText = R"(

QLineEdit docs: <a href="http://doc.qt.io/qt-5/qcombobox.html">http://doc.qt.io/qt-5/qlineedit.html</a><br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/LineEdit.h&gt;
#include &lt;QLineEdit&gt;
#include &lt;QDebug&gt;

QLineEdit* lineEdit;

// Assuming you've created a QLineEdit already (either in code or via .ui file):

// to set the text:
lineEdit->setText("Filled in data");

// to set hint text:
lineEdit->setPlaceholderText("Hint text");

// to disable line edits
lineEdit->setDisabled(true);

// to style a line edit with the search icon on the left:
AzQtComponents::LineEdit::applySearchStyle(ui->searchBox);

// to indicate errors, set a QValidator subclass on the QLineEdit object:
lineEdit->setValidator(new CustomValidator(lineEdit));

// You can also use pre-made validators for QDoubleValidator, QIntValidator, QRegularExpressionValidator

// To listen for text changes:
connect(lineEdit, QLineEdit::textChanged, this, [](const QString& newText){
    qDebug() &lt;&lt; QString("Text changed: %1").arg(newText);
});

connect(lineEdit, QLineEdit::textEdit, this, [](const QString& newText){
    // only happens when the user changed the text, not when lineEdit->setText("some text"); is called
    qDebug() &lt;&lt; QString("Text edited: %1").arg(newText);
});

connect(lineEdit, QLineEdit::editingFinished, this, [lineEdit](){
    // this happens when the focus leaves the line edit or the user hits the enter key
    qDebug() &lt;&lt; QString("Editing finished. New text: %1").arg(lineEdit->text());
});

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

LineEditPage::~LineEditPage()
{
}

#include <Gallery/LineEditPage.moc>
