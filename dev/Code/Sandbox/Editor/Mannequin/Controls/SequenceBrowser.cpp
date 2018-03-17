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
#include "SequenceBrowser.h"
#include "PreviewerPage.h"
#include "Controls/FolderTreeCtrl.h"
#include "../MannequinDialog.h"

#include "PreviewerPage.h"

// File name extension for sequence files
static const QString kSequenceFileNameSpec = "*.xml";

// Tree root element name
static const QString kRootElementName = "Sequences";

static QString GetSequenceFolder()
{
    QString sequenceFolder;

    ICVar* pSequencePathCVar = gEnv->pConsole->GetCVar("mn_sequence_path");
    if (pSequencePathCVar)
    {
        sequenceFolder = (Path::GetEditingGameDataFolder() + "/" + pSequencePathCVar->GetString()).c_str();
        sequenceFolder = Path::ToUnixPath(Path::RemoveBackslash(sequenceFolder));
    }

    return sequenceFolder;
}

//////////////////////////////////////////////////////////////////////////
// CSequenceBrowser
//////////////////////////////////////////////////////////////////////////
CSequenceBrowser::CSequenceBrowser(CPreviewerPage& previewerPage, QWidget* parent)
    : QWidget(parent)
    , m_previewerPage(previewerPage)
    , ui(new Ui::CSequenceBrowser)
{
    ui->setupUi(this);

    QStringList folders;
    folders.push_back(GetSequenceFolder());
    ui->TREE->init(folders, kSequenceFileNameSpec, kRootElementName);
    connect(ui->TREE, &QTreeWidget::itemDoubleClicked, this, &CSequenceBrowser::OnTreeDoubleClicked);
    connect(ui->OPEN, &QPushButton::clicked, this, &CSequenceBrowser::OnOpen);
}

void CSequenceBrowser::OnTreeDoubleClicked()
{
    OnOpen();
}

void CSequenceBrowser::OnOpen()
{
    QList<QTreeWidgetItem*> selectedItems = ui->TREE->selectedItems();
    QTreeWidgetItem* selectedItem = selectedItems.empty() ? nullptr : selectedItems.first();

    if (selectedItem == NULL)
    {
        return;
    }

    if (ui->TREE->IsFile(selectedItem))
    {
        QString sequencePath = ui->TREE->GetPath(selectedItem);
        m_previewerPage.LoadSequence(sequencePath.toUtf8());
        CMannequinDialog::GetCurrentInstance()->ShowPane<CPreviewerPage*>();
    }
}

#include <Mannequin/Controls/SequenceBrowser.moc>
