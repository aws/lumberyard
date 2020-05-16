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
#include "EditorUI_QT_Precompiled.h"
#include "QFileSelectWidget.h"
#include <VariableWidgets/ui_QFileSelectWidget.h>
#include <QFileDialog>

#include "QAmazonLineEdit.h"
#include <QEvent>
#include <QKeyEvent>

QFileSelectWidget::QFileSelectWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::QFileSelectWidget)
    , m_dialogCaption("Select File")
    , m_fileFilter(tr("Any file (*.*)"))
{
    ui->setupUi(this);
    ui->gridLayout->removeWidget(ui->buttonOpenFileDialog);
    ui->gridLayout->removeWidget(ui->path);
    ui->gridLayout->addWidget(ui->buttonOpenFileDialog, 0, 5, 1, 1);
    ui->gridLayout->addWidget(ui->path, 0, 0, 1, 5);
    ui->path->setObjectName("FileSelectLineEdit");
    ui->path->installEventFilter(this);
    ui->path->setEditable(true);
    ui->path->setInsertPolicy(QComboBox::InsertAtTop);
    ui->path->setDuplicatesEnabled(false);
    ui->path->lineEdit()->setPlaceholderText(tr("path"));
    ui->path->setContentsMargins(0, 0, 0, 0);
    connect(ui->path->lineEdit(), &QLineEdit::editingFinished, this, [=]()
        {
            onReturnPressed();
        });
    connect(ui->path, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIdxChanged(int)));
    
    ui->path->installEventFilter(this);
}

QFileSelectWidget::~QFileSelectWidget()
{
    delete ui;
}

bool QFileSelectWidget::eventFilter(QObject * watched, QEvent * event)
{
    // I set the "path" combo box's focusPolicy to StrongFocus instead of Wheel, but it is still acting as if it's 
    // using Wheel. Not sure why. Anyway, even if we get the focus behavior right, and clear focus when the user
    // makes a change, the wheel behavior still is odd (you can only scroll one space up/down and then focus is
    // cleared). So we just prevent all wheel events.
    return (watched == ui->path && event->type() == QEvent::Wheel);
}

void QFileSelectWidget::setPath(const QString& path)
{
    ui->path->lineEdit()->setText(path);
}

QString QFileSelectWidget::getDisplayPath() const
{
    return ui->path->lineEdit()->text();
}

QString QFileSelectWidget::getFullPath() const
{
    return ui->path->currentData().toString();
}

bool QFileSelectWidget::addUniqueItem(const QString& text, const QVariant& userData)
{
    for (int index = 0; index < ui->path->count(); index++)
    {
        if (text == ui->path->itemText(index))
        {
            ui->path->setCurrentIndex(index);
            return false;
        }
    }
    ui->path->addItem(text, userData);
    ui->path->setCurrentIndex(ui->path->count()-1);
    return true;
}

void QFileSelectWidget::onOpenSelectDialog()
{
    QString path = QFileDialog::getOpenFileName(this, m_dialogCaption, m_lookInFolder, m_fileFilter);
    setPath(path);
}

void QFileSelectWidget::onReturnPressed()
{
}

void QFileSelectWidget::onSelectedIndexChanged(int index)
{
    setPath(ui->path->itemData(index).toString());
}

void QFileSelectWidget::onCurrentIdxChanged(int index)
{
    onSelectedIndexChanged(index);
}

QString QFileSelectWidget::pathFilter(const QString& path)
{
    return path;
}

void QFileSelectWidget::addWidget(QWidget* widget, int row, int col, int rowspan, int colspan)
{
    ui->gridLayout->addWidget(widget, row, col, rowspan, colspan);
}

void QFileSelectWidget::setMainButton(const QString& caption, const QString& tooltip, const QIcon* icon, int row, int col, int rowspan, int colspan)
{
    if (icon)
    {
        ui->buttonOpenFileDialog->setIcon(*icon);
    }

    ui->buttonOpenFileDialog->setText(caption);
    ui->buttonOpenFileDialog->setToolTip(tooltip);
}

QPushButton* QFileSelectWidget::addButton(const QString& caption, const QString& tooltip, int row, int col, int rowspan, int colspan, const QIcon* icon)
{
    QPushButton* btn = new QPushButton("Unamed Button", this);

    if (icon)
    {
        btn->setIcon(*icon);
    }

    btn->setText(caption);
    btn->setToolTip(tooltip);

    addWidget(btn, row, col, rowspan, colspan);

    return btn;
}

void QFileSelectWidget::on_buttonOpenFileDialog_clicked()
{
    onOpenSelectDialog();
}

#ifdef EDITOR_QT_UI_EXPORTS
#include <VariableWidgets/QFileSelectWidget.moc>
#endif

