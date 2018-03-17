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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "PythonScriptsDialog.h"
#include "Controls/FolderTreeCtrl.h"
#include <Dialogs/ui_PythonScriptsDialog.h>
#include <QtUtil.h>
#include "Util/PathUtil.h"
#include "QtViewPaneManager.h"

//////////////////////////////////////////////////////////////////////////
namespace
{
    // File name extension for python files
    const QString s_kSequenceFileNameSpec = "*.py";

    // Tree root element name
    const QString s_kRootElementName = "Python Scripts";
}

//////////////////////////////////////////////////////////////////////////
void CPythonScriptsDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.canHaveMultipleInstances = true;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    AzToolsFramework::RegisterViewPane<CPythonScriptsDialog>("Python Scripts", LyViewPane::CategoryOther, options);
}


//////////////////////////////////////////////////////////////////////////
CPythonScriptsDialog::CPythonScriptsDialog()
    : QWidget()
    , ui(new Ui::CPythonScriptsDialog)
{
    ui->setupUi(this);

    QStringList scriptFolders;

    const auto editorEnvStr = gSettings.strEditorEnv.toLocal8Bit();
    AZStd::string actualPath = AZStd::string::format("@engroot@/%s", editorEnvStr.constData());


    XmlNodeRef envNode = XmlHelpers::LoadXmlFromFile(actualPath.c_str());
    if (envNode)
    {
        QString scriptPath;
        int childrenCount = envNode->getChildCount();
        for (int idx = 0; idx < childrenCount; ++idx)
        {
            XmlNodeRef child = envNode->getChild(idx);
            if (child->haveAttr("scriptPath"))
            {
                scriptPath = child->getAttr("scriptPath");
                scriptFolders.push_back(scriptPath);
            }
        }
    }

    ui->treeWidget->init(scriptFolders, s_kSequenceFileNameSpec, s_kRootElementName, false, false);
    connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, this, &CPythonScriptsDialog::OnExecute);
    connect(ui->executeButton, &QPushButton::clicked, this, &CPythonScriptsDialog::OnExecute);
}

CPythonScriptsDialog::~CPythonScriptsDialog()
{
}

//////////////////////////////////////////////////////////////////////////
void CPythonScriptsDialog::OnExecute()
{
    QList<QTreeWidgetItem*> selectedItems = ui->treeWidget->selectedItems();
    QTreeWidgetItem* selectedItem = selectedItems.empty() ? nullptr : selectedItems.first();

    if (selectedItem == NULL)
    {
        return;
    }

    if (ui->treeWidget->IsFile(selectedItem))
    {
        QString workingDirectory = QDir::currentPath();
        const QString scriptPath = QStringLiteral("%1/%2").arg(workingDirectory).arg(ui->treeWidget->GetPath(selectedItem));
        GetIEditor()->ExecuteCommand(QStringLiteral("general.run_file '%1'").arg(scriptPath));
    }
}

#include <Dialogs/PythonScriptsDialog.moc>
