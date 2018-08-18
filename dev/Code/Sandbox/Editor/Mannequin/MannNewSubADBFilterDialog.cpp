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
#include "MannNewSubADBFilterDialog.h"
#include "MannequinDialog.h"
#include <Mannequin/ui_MannNewSubADBFilterDialog.h>

#include "StringDlg.h"

#include <ICryMannequin.h>
#include <IGameFramework.h>
#include <ICryMannequinEditor.h>

#include <QtUtilWin.h>

#include <QPushButton>
#include <QStringListModel>

namespace
{
    const char* adb_noDB = "<no animation database (adb)>";
    const char* adb_DBext = ".adb";
};

//////////////////////////////////////////////////////////////////////////
CMannNewSubADBFilterDialog::CMannNewSubADBFilterDialog(SMiniSubADB* pData, IAnimationDatabase* animDB, const string& parentName, const EContextDialogModes mode, QWidget* pParent)
    : QDialog(pParent)
    , m_ui(new Ui::MannNewSubADBFilterDialog)
    , m_mode(mode)
    , m_pDataCopy(nullptr)
    , m_animDB(animDB)
    , m_parentName(parentName)
    , m_fragIDList(new QStringListModel)
    , m_fragUsedIDList(new QStringListModel)
{
    m_ui->setupUi(this);
    m_ui->TAGS_PANEL->Setup();

    // List models for fragment IDs
    m_ui->FRAG_ID_LIST->setModel(m_fragIDList.data());
    m_ui->FRAG_USED_ID_LIST->setModel(m_fragUsedIDList.data());

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &CMannNewSubADBFilterDialog::OnOk);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_ui->NAME_EDIT, &QLineEdit::textChanged, this, &CMannNewSubADBFilterDialog::OnFilenameChanged);
    connect(m_ui->FRAG2USED, &QToolButton::clicked, this, &CMannNewSubADBFilterDialog::OnFrag2Used);
    connect(m_ui->USED2FRAG, &QToolButton::clicked, this, &CMannNewSubADBFilterDialog::OnUsed2Frag);
    connect(m_ui->FRAG_ID_LIST->selectionModel(), &QItemSelectionModel::selectionChanged, m_ui->FRAG2USED,
        [&](const QItemSelection& selected, const QItemSelection&)
        {
            m_ui->FRAG2USED->setEnabled(selected.size() > 0);
        });
    connect(m_ui->FRAG_USED_ID_LIST->selectionModel(), &QItemSelectionModel::selectionChanged, m_ui->USED2FRAG,
        [&](const QItemSelection& selected, const QItemSelection&)
        {
            m_ui->USED2FRAG->setEnabled(selected.size() > 0);
        });

    switch (m_mode)
    {
    case eContextDialog_New:
        m_pData = new SMiniSubADB;
        m_pData->filename = adb_noDB;
        break;
    case eContextDialog_Edit:
    {
        assert(pData);
        m_pData = pData;
        CopySubADBData(pData);
        break;
    }
    case eContextDialog_CloneAndEdit:
        assert(pData);
        m_pData = new SMiniSubADB;
        m_pData->vFragIDs.clear();
        CloneSubADBData(pData);
        break;
    }

    OnInitDialog();
}

//////////////////////////////////////////////////////////////////////////
CMannNewSubADBFilterDialog::~CMannNewSubADBFilterDialog()
{
    if (m_mode == eContextDialog_Edit)
    {
        m_pData = nullptr;
    }
    else
    {
        SAFE_DELETE(m_pData);   // ?? not sure if i want to be doing this, revist after adding new contexts to the dialog
    }
    SAFE_DELETE(m_pDataCopy);
}

//////////////////////////////////////////////////////////////////////////
void CMannNewSubADBFilterDialog::OnInitDialog()
{
    m_tagVars.reset(new CVarBlock());

    PopulateFragIDList();
    PopulateControls();

    switch (m_mode)
    {
    case eContextDialog_Edit:
        this->setWindowTitle("Edit Context");
        break;
    case eContextDialog_CloneAndEdit:
        this->setWindowTitle("Clone Context");
        break;
    }

    EnableControls();
}

//////////////////////////////////////////////////////////////////////////
void CMannNewSubADBFilterDialog::CloneSubADBData(const SMiniSubADB* pData)
{
    m_pData->tags = pData->tags;
    m_pData->filename = adb_noDB;   // don't clone the filename, force the user to replace it.
    m_pData->pFragDef = pData->pFragDef;
    m_pData->vFragIDs = pData->vFragIDs;
    m_pData->vSubADBs = pData->vSubADBs;
}

//////////////////////////////////////////////////////////////////////////
void CMannNewSubADBFilterDialog::CopySubADBData(const SMiniSubADB* pData)
{
    assert(nullptr == m_pDataCopy);
    m_pDataCopy = new SMiniSubADB;
    m_pDataCopy->tags = pData->tags;
    m_pDataCopy->filename = pData->filename;
    m_pDataCopy->pFragDef = pData->pFragDef;
    m_pDataCopy->vFragIDs = pData->vFragIDs;
    m_pDataCopy->vSubADBs = pData->vSubADBs;
}

//////////////////////////////////////////////////////////////////////////
void CMannNewSubADBFilterDialog::OnFilenameChanged()
{
    EnableControls();
}

//////////////////////////////////////////////////////////////////////////
void CMannNewSubADBFilterDialog::EnableControls()
{
    const bool isValidFilename = VerifyFilename();
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValidFilename);
}

//////////////////////////////////////////////////////////////////////////
bool CMannNewSubADBFilterDialog::VerifyFilename()
{
    const QString& testString = m_ui->NAME_EDIT->text();
    return (testString != adb_noDB) && testString.endsWith(adb_DBext) && (testString.size() > 4);
}

//////////////////////////////////////////////////////////////////////////
void CMannNewSubADBFilterDialog::PopulateControls()
{
    // Setup the combobox lookup values
    CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();
    SMannequinContexts* pContexts = pMannequinDialog->Contexts();
    const SControllerDef* pControllerDef = pContexts->m_controllerDef;

    // fill in per detail values now
    m_ui->NAME_EDIT->setText(Path::GetFile(m_pData->filename.c_str()));

    // Tags
    m_tagVars->DeleteAllVariables();
    m_tagControls.Init(m_tagVars, pControllerDef->m_tags);
    m_tagControls.Set(m_pData->tags);

    m_ui->TAGS_PANEL->DeleteVars();
    m_ui->TAGS_PANEL->SetVarBlock(m_tagVars.get(), NULL);
}

//////////////////////////////////////////////////////////////////////////
void CMannNewSubADBFilterDialog::PopulateFragIDList()
{
    QStringList fragUsedIds;

    if (m_pData)
    {
        for (SMiniSubADB::TFragIDArray::const_iterator it = m_pData->vFragIDs.begin(); it != m_pData->vFragIDs.end(); ++it)
        {
            QString text(m_pData->pFragDef->GetTagName(*it));
            fragUsedIds.append(text);
        }
    }

    m_fragUsedIDList->setStringList(fragUsedIds);

    QStringList fragIds;

    if (m_animDB)
    {
        const CTagDefinition& fragDefs  = m_animDB->GetFragmentDefs();
        const uint32 numFrags = fragDefs.GetNum();

        for (uint32 i = 0; i < numFrags; i++)
        {
            if (!m_pData || std::find(m_pData->vFragIDs.begin(), m_pData->vFragIDs.end(), i) == m_pData->vFragIDs.end())
            {
                QString fragTagName = fragDefs.GetTagName(i);
                fragIds.append(fragTagName);
            }
        }
    }

    m_fragIDList->setStringList(fragIds);
}

//////////////////////////////////////////////////////////////////////////
void CMannNewSubADBFilterDialog::OnOk()
{
    // retrieve per-detail values now
    m_pData->filename = m_ui->NAME_EDIT->text().toUtf8().constData();

    // verify that the string is ok - don't have to really since button should be disabled if its not
    assert(m_pData->filename != adb_noDB);

    m_pData->tags = m_tagControls.Get();

    // about to rebuild this so delete the old
    m_pData->vFragIDs.clear();
    const int nItems = m_fragUsedIDList->rowCount();
    for (int i = 0; i < nItems; ++i)
    {
        const QString fragmentIdName = m_fragUsedIDList->data(m_fragUsedIDList->index(i), Qt::DisplayRole).toString();
        const FragmentID fragId = m_animDB->GetFragmentID(fragmentIdName.toUtf8());
        m_pData->vFragIDs.push_back(fragId);
    }

    string copyOfFilename = m_pData->filename;
    const string pathToADBs("Animations/Mannequin/ADB/");
    const size_t pos = m_pData->filename.find(pathToADBs);
    if (!VerifyFilename())
    {
        copyOfFilename += adb_DBext;
    }
    const string finalFilename = (pos >= copyOfFilename.size()) ? (pathToADBs + copyOfFilename) : copyOfFilename;

    switch (m_mode)
    {
    case eContextDialog_CloneAndEdit:// fall through
    case eContextDialog_New:
        SaveNewSubADBFile(finalFilename);
        break;
    case eContextDialog_Edit:
        // delete current SubADB
        m_animDB->ClearSubADBFilter(m_pDataCopy->filename);

        // save "new" SubADB as above
        SaveNewSubADBFile(finalFilename);
        break;
    }

    // if everything is ok with the data entered.
    accept();
}

//////////////////////////////////////////////////////////////////////////
void CMannNewSubADBFilterDialog::SaveNewSubADBFile(const string& filename)
{
    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    IMannequinEditorManager* pMannequinEditorManager = mannequinSys.GetMannequinEditorManager();
    CRY_ASSERT(pMannequinEditorManager);

    m_animDB->AddSubADBTagFilter(m_parentName, filename, TAG_STATE_EMPTY); // TODO: Create SubADB file

    pMannequinEditorManager->SetSubADBTagFilter(m_animDB, filename.c_str(), m_pData->tags);

    // now add the fragment filters
    for (SMiniSubADB::TFragIDArray::const_iterator it = m_pData->vFragIDs.begin(); it != m_pData->vFragIDs.end(); ++it)
    {
        pMannequinEditorManager->AddSubADBFragmentFilter(m_animDB, filename.c_str(), *it);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannNewSubADBFilterDialog::FindFragmentIDsMissing(SMiniSubADB::TFragIDArray& outFrags, const SMiniSubADB::TFragIDArray& AFrags, const SMiniSubADB::TFragIDArray& BFrags)
{
    // iterate through the "copy"
    for (SMiniSubADB::TFragIDArray::const_iterator Aiter = AFrags.begin(); Aiter != AFrags.end(); ++Aiter)
    {
        bool bFound = false;
        // iterate through the "original"
        for (SMiniSubADB::TFragIDArray::const_iterator Biter = BFrags.begin(); Biter != BFrags.end(); ++Biter)
        {
            if ((*Aiter) == (*Biter))
            {
                bFound = true;
                break;
            }
        }
        // if we didn't find it then it must have been removed
        if (!bFound)
        {
            outFrags.push_back(*Aiter);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannNewSubADBFilterDialog::OnFrag2Used()
{
    QModelIndexList selected = m_ui->FRAG_ID_LIST->selectionModel()->selectedIndexes();

    int rowCount = m_fragUsedIDList->rowCount();
    m_fragUsedIDList->insertRows(rowCount, selected.count());
    for (auto& index : selected)
    {
        QModelIndex target = m_fragUsedIDList->index(rowCount++);
        m_fragUsedIDList->setData(target, index.data(Qt::DisplayRole), Qt::DisplayRole);
    }

    qSort(selected.begin(), selected.end(), qGreater<QModelIndex>());
    for (auto removal = selected.cbegin(); removal != selected.cend(); ++removal)
    {
        m_fragIDList->removeRow(removal->row());
    }

    m_fragIDList->sort(0);
    m_fragUsedIDList->sort(0);

    EnableControls();
}

//////////////////////////////////////////////////////////////////////////
void CMannNewSubADBFilterDialog::OnUsed2Frag()
{
    QModelIndexList selected = m_ui->FRAG_USED_ID_LIST->selectionModel()->selectedIndexes();

    int rowCount = m_fragIDList->rowCount();
    m_fragIDList->insertRows(rowCount, selected.count());
    for (auto& index : selected)
    {
        QModelIndex target = m_fragIDList->index(rowCount++);
        m_fragIDList->setData(target, index.data(Qt::DisplayRole), Qt::DisplayRole);
    }

    qSort(selected.begin(), selected.end(), qGreater<QModelIndex>());
    for (auto removal : selected)
    {
        m_fragUsedIDList->removeRow(removal.row());
    }

    m_fragIDList->sort(0);
    m_fragUsedIDList->sort(0);

    EnableControls();
}

#include <Mannequin/MannNewSubADBFilterDialog.moc>