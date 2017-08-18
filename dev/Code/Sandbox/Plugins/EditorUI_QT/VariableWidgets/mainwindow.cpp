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
#include "mainwindow.h"
#include <VariableWidgets/ui_mainwindow.h>
#include "QBitmapPreviewDialog.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_previewDialog(nullptr)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_previewDialog = new QBitmapPreviewDialog(this);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (m_previewDialog)
    {
        delete m_previewDialog;
    }
}

void MainWindow::on_pushButton_clicked()
{
    if (m_previewDialog->isVisible())
    {
        m_previewDialog->hide();
    }
    else
    {
        m_previewDialog->show();
    }
}
