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

#pragma once

#include "DetailWidget.h"

#include <IAWSResourceManager.h>
#include <FilePathLabel.h>

#include <QAction>
#include <QMessageBox>
#include <QPushButton>
#include <QDir>
#include <QApplication>
#include <QClipboard>

class CodeDirectoryDetailWidget
    : public DetailWidget
{
public:

    CodeDirectoryDetailWidget(ResourceManagementView* view, QSharedPointer<ICodeDirectoryModel> codeDirectoryModel, QWidget* parent = nullptr)
        : DetailWidget{view, parent}
        , m_codeDirectoryModel{codeDirectoryModel}
    {
    }

    void show() override
    {
        DetailWidget::show();

        m_view->m_menuNewDirectory->setEnabled(true);
        connectUntilHidden(m_view->m_menuNewDirectory, &QAction::triggered, this, &CodeDirectoryDetailWidget::OnNewDirectory);

        m_view->m_menuNewFile->setEnabled(true);
        connectUntilHidden(m_view->m_menuNewFile, &QAction::triggered, this, &CodeDirectoryDetailWidget::OnNewFile);
    }

    QMenu* GetTreeContextMenu() override
    {
        auto menu = new ToolTipMenu {};

        auto newFile = menu->addAction("New file");
        newFile->setToolTip(m_view->m_menuNewFile->toolTip());
        connectUntilDeleted(newFile, &QAction::triggered, this, &CodeDirectoryDetailWidget::OnNewFile);

        auto newDirectory = menu->addAction("New directory");
        newDirectory->setToolTip(m_view->m_menuNewDirectory->toolTip());
        connectUntilDeleted(newDirectory, &QAction::triggered, this, &CodeDirectoryDetailWidget::OnNewDirectory);

        menu->addSeparator();

        if (!m_codeDirectoryModel->DoNotDelete())
        {
            auto deleteDirectory = menu->addAction("Delete directory");
            deleteDirectory->setToolTip("Delete the directory from disk.");
            connectUntilDeleted(deleteDirectory, &QAction::triggered, this, &CodeDirectoryDetailWidget::OnDeleteRequested);
        }

        menu->addSeparator(); 
        
        //Add right click menu option to code directory node in tree
        auto openPathInExplorer = menu->addAction("View in Explorer");
        openPathInExplorer->setToolTip(tr("View the directory in Windows Explorer."));
        connectUntilDeleted(openPathInExplorer, &QAction::triggered, this, &CodeDirectoryDetailWidget::OnOpenPathInExplorer);

        menu->addSeparator();

        auto copyPathToClipboard = menu->addAction("Copy path to clipboard");
        copyPathToClipboard->setToolTip(tr("Copy the directory's path to the clipboard."));
        connectUntilDeleted(copyPathToClipboard, &QAction::triggered, this, &CodeDirectoryDetailWidget::OnCopyPathToClipboard);

        return menu;
    }

private:
    void OnUploadCode()
    {
        //Get the resource group name
        QString path = m_codeDirectoryModel->Path();
        QStringList tempList = path.split("\\");
        QString resourceGroupName = tempList[tempList.count() - 2];

        //Upload the function code
        QSharedPointer<IResourceGroupStatusModel> resourceGroupStatusModel = GetIEditor()->GetAWSResourceManager()->GetProjectModel()->ResourceGroupStatusModel(resourceGroupName);
        m_view->UploadLambdaCode(resourceGroupStatusModel, "");
    }

    QSharedPointer<ICodeDirectoryModel> m_codeDirectoryModel;

    void OnNewDirectory()
    {
        QString directoryName = QInputDialog::getText(this, "New Directory", "Directory Name");
        if (directoryName.isEmpty())
        {
            return;
        }

        QDir dir {
            m_codeDirectoryModel->Path()
        };

        if (dir.exists(directoryName))
        {
            QMessageBox box(QMessageBox::NoIcon,
                "New Directory",
                "The directory already contains a sub-directory named \"" + directoryName + "\".",
                QMessageBox::Ok,
                Q_NULLPTR,
                Qt::Popup);
            box.exec();
            return;
        }

        auto directoryPath = QDir::toNativeSeparators(dir.absoluteFilePath(directoryName));

        m_view->m_waitingForPath = directoryPath;

        if (!dir.mkpath(directoryPath))
        {
            m_view->m_waitingForPath.clear();
            QMessageBox box(QMessageBox::NoIcon,
                "New Directory",
                "The directory could not be created.",
                QMessageBox::Ok,
                Q_NULLPTR,
                Qt::Popup);
            box.exec();
            return;
        }
    }

    void OnNewFile()
    {
        QString fileName = QInputDialog::getText(this, "New File", "File Name");
        if (fileName.isEmpty())
        {
            return;
        }

        QDir dir {
            m_codeDirectoryModel->Path()
        };

        if (dir.exists(fileName))
        {
            QMessageBox box(QMessageBox::NoIcon,
                "New File",
                "The directory already contains a " + fileName + " file.",
                QMessageBox::Ok,
                Q_NULLPTR,
                Qt::Popup);
            box.exec();
            return;
        }

        auto filePath = QDir::toNativeSeparators(dir.absoluteFilePath(fileName));

        m_view->m_waitingForPath = filePath;

        QFile f(filePath);
        if (f.open(QFile::WriteOnly | QFile::Text))
        {
            GetIEditor()->GetAWSResourceManager()->RequestAddFile(filePath);
            f.close();
        }
        else
        {
            m_view->m_waitingForPath.clear();
            QMessageBox box(QMessageBox::NoIcon,
                "New File",
                "The file could not be created. " + f.errorString(),
                QMessageBox::Ok,
                Q_NULLPTR,
                Qt::Popup);
            box.exec();
        }
    }

    void OnCopyPathToClipboard()
    {
        auto clipboard = QApplication::clipboard();
        clipboard->setText(m_codeDirectoryModel->Path());
    }

    void OnOpenPathInExplorer()
    {
        // GetIEditor()->GetFileUtil()->ShowInExplorer doesn't handle full paths. Using openUrl instead.
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_codeDirectoryModel->Path()));
    }

    void OnDeleteRequested()
    {
        auto reply = QMessageBox::question(
                this,
                "Delete Directory",
                "Delete the following directory and all of its contents?<br><br>" + m_codeDirectoryModel->Path(),
                QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            if (!GetIEditor()->GetFileUtil()->Deltree(m_codeDirectoryModel->Path().toStdString().c_str(), true))
            {
                QMessageBox box(QMessageBox::NoIcon,
                    "Delete Directory",
                    "Directory could not be deleted.",
                    QMessageBox::Ok);
                box.exec();
            }
        }
    }
};
