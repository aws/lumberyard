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

#include "ComboBoxPage.h"
#include <Gallery/ui_ComboBoxPage.h>

#include <AzQtComponents/Components/Widgets/CheckBox.h>

#include <QMenu>
#include <QCheckBox>
#include <QLineEdit>

ComboBoxPage::ComboBoxPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::ComboBoxPage)
{
    ui->setupUi(this);

    // Add some sample items to the combo box
    ui->m_data->addItem("Selected value", 0);
    for (int i = 1; i < 5; i++)
    {
        ui->m_data->addItem(QString("Option %1").arg(i), i);
    }
    ui->m_data->setCurrentIndex(0);
    ui->m_flat->setModel(ui->m_data->model());

    // Make the control with data be editable too
    ui->m_data->setEditable(true);
    ui->m_data->lineEdit()->setPlaceholderText("Hint Text"); // visible only 

    ui->m_disabled->setDisabled(true);
    ui->m_disabled->addItem("Disabled dropdown");
    ui->m_disabled->setCurrentIndex(0);

    QString exampleText = R"(

QComboBox docs: <a href="http://doc.qt.io/qt-5/qcombobox.html">http://doc.qt.io/qt-5/qcombobox.html</a><br/>

<pre>
#include &lt;QComboBox&gt;
#include &lt;QDebug&gt;

QCombBox* comboBox;

// Assuming you've created a comboBox already (either in code or via .ui file):

// Add items to a combo box
comboBox->addItem("Selected value", 0);
for (int i = 1; i &lt; 5; i++)
{
    comboBox->addItem(QString("Option %1").arg(i), i);
}

// set placeholder text, visible when the selected item is empty or when the user is editing and has deleted all text
comboBox->setEditable(true); // must make it editable first!
comboBox->lineEdit()->setPlaceholderText("Hint Text");

// NOTE: if a QComboBox is set to allow editing (setEditable(true)) then the user can enter new items that will be added to the drop down
// list each time the user hits the enter key.
// Only use this feature if you intend to allow the user to add new items to the list.

// disable the combo box
comboBox->setDisabled(true);

// set the current index
comboBox->setCurrentIndex(0);

// listen for changes (need to static cast the currentIndexChanged signal, because there are both int and QString versions)
connect(comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int newIndex) {
    qDebug() &lt;&lt; QString("New selection: %1").arg(newIndex);
});

// listen for text changes
connect(comboBox, &QComboBox::currentTextChanged), this, [this, combo](const QString& newText) {
    qDebug() &lt;&lt; QString("New text in combo box: %1").arg(newText);
});

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

ComboBoxPage::~ComboBoxPage()
{
}

#include <Gallery/ComboBoxPage.moc>
