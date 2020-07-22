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
#include <EditorDefs.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>

//////////////////////////////////////////////////////////////////////////
namespace
{
    // File name extension for python files
    const QString s_kPythonFileNameSpec = "*.py";

    // Tree root element name
    const QString s_kRootElementName = "Python Scripts";
}

//////////////////////////////////////////////////////////////////////////
void CPythonScriptsDialog::RegisterViewClass()
{
    if (AzToolsFramework::EditorPythonRunnerRequestBus::HasHandlers())
    {
        AzToolsFramework::ViewPaneOptions options;
        options.canHaveMultipleInstances = true;
        options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
        AzToolsFramework::RegisterViewPane<CPythonScriptsDialog>("Python Scripts", LyViewPane::CategoryOther, options);
    }
}

//////////////////////////////////////////////////////////////////////////
CPythonScriptsDialog::CPythonScriptsDialog(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::CPythonScriptsDialog)
{
    ui->setupUi(this);

    QStringList scriptFolders;

    const auto editorEnvStr = gSettings.strEditorEnv.toLocal8Bit();
    AZStd::string editorScriptsPath = AZStd::string::format("@engroot@/%s", editorEnvStr.constData());
    XmlNodeRef envNode = XmlHelpers::LoadXmlFromFile(editorScriptsPath.c_str());
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

    ScanFolderForScripts(QString("@devroot@/%1/Editor/Scripts").arg(GetIEditor()->GetProjectName()), scriptFolders);

    auto moduleCallback = [this, &scriptFolders](const AZ::ModuleData& moduleData) -> bool
    {
        if (moduleData.GetDynamicModuleHandle())
        {
            const AZ::OSString& modulePath = moduleData.GetDynamicModuleHandle()->GetFilename();
            AZStd::string fileName;
            AzFramework::StringFunc::Path::GetFileName(modulePath.c_str(), fileName);

            AZStd::vector<AZStd::string> tokens;
            AzFramework::StringFunc::Tokenize(fileName.c_str(), tokens, '.');
            if (tokens.size() > 2 && tokens[0] == "Gem")
            {
                ScanFolderForScripts(QString("@engroot@/Gems/%1/Editor/Scripts").arg(tokens[1].c_str()), scriptFolders);
            }
        }
        return true;
    };
    AZ::ModuleManagerRequestBus::Broadcast(&AZ::ModuleManagerRequestBus::Events::EnumerateModules, moduleCallback);

    ui->treeWidget->init(scriptFolders, s_kPythonFileNameSpec, s_kRootElementName, false, false);
    connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, this, &CPythonScriptsDialog::OnExecute);
    connect(ui->executeButton, &QPushButton::clicked, this, &CPythonScriptsDialog::OnExecute);
}

//////////////////////////////////////////////////////////////////////////
void CPythonScriptsDialog::ScanFolderForScripts(QString path, QStringList& scriptFolders) const
{
    char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
    if (AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(path.toLocal8Bit().constData(), resolvedPath, AZ_MAX_PATH_LEN))
    {
        if (AZ::IO::SystemFile::Exists(resolvedPath))
        {
            scriptFolders.push_back(path);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
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
        using namespace AzToolsFramework;
        EditorPythonRunnerRequestBus::Broadcast(&EditorPythonRunnerRequestBus::Events::ExecuteByFilename, scriptPath.toUtf8().constData());
    }
}

#include <Dialogs/PythonScriptsDialog.moc>
