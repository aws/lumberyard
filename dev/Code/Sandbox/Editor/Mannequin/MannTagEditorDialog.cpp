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

#include "stdafx.h"
#include "MannTagEditorDialog.h"
#include "MannequinDialog.h"
#include "MannTagDefEditorDialog.h"
#include <ICryMannequinEditor.h>
#include <IGameFramework.h>

#include <Mannequin/ui_MannTagEditorDialog.h>
#include "QtUtil.h"
#include "QtUtilWin.h"

#include <QMessageBox>
#include <QFileDialog>


//////////////////////////////////////////////////////////////////////////
static QString GetNormalizedFilenameString(const QString& filename)
{
    QString normalizedFilename = Path::FullPathToGamePath(filename);
    Path::ConvertBackSlashToSlash(normalizedFilename);
    return normalizedFilename;
}

//////////////////////////////////////////////////////////////////////////
CMannTagEditorDialog::CMannTagEditorDialog(IAnimationDatabase* animDB, FragmentID fragmentID, QWidget* pParent /*= nullptr*/)
    : QDialog(pParent)
    , m_contexts(NULL)
    , m_animDB(animDB)
    , m_fragmentID(fragmentID)
    , m_fragmentName("")
    , m_nOldADBFileIndex(0)
{
    OnInitDialog();
}

//////////////////////////////////////////////////////////////////////////
CMannTagEditorDialog::~CMannTagEditorDialog()
{
}

//////////////////////////////////////////////////////////////////////////
BOOL CMannTagEditorDialog::OnInitDialog()
{
    setWindowTitle("Mannequin FragmentID Editor");
    m_contexts = CMannequinDialog::GetCurrentInstance()->Contexts();

    ui.reset(new Ui::CMannTagEditorDialog);
    ui->setupUi(this);

    m_nameEdit = ui->FRAGMENT_ID_NAME_EDIT;
    m_btnIsPersistent = ui->IS_PERSISTENT;
    m_btnAutoReinstall = ui->AUTO_REINSTALL_BEST_MATCH;
    m_scopeTagsGroupBox = ui->GROUPBOX_SCOPETAGS;
    m_tagDefComboBox = ui->TAGDEF_COMBO;
    m_FragFileComboBox = ui->FRAGFILE_COMBO;

    QVBoxLayout* l = new QVBoxLayout;
    l->addWidget(&m_scopeTagsControl);
    m_scopeTagsGroupBox->setLayout(l);
    m_scopeTagsGroupBox->show();

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &CMannTagEditorDialog::reject);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CMannTagEditorDialog::OnOK);
    connect(ui->EDIT_TAGS, &QPushButton::clicked, this, &CMannTagEditorDialog::OnEditTagDefs);
    connect(m_FragFileComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CMannTagEditorDialog::OnCbnSelchangeFragfileCombo);
    connect(ui->CREATE_ADB_FILE, &QPushButton::clicked, this, &CMannTagEditorDialog::OnBnClickedCreateAdbFile);

    // Setup the dialog to edit the FragmentID
    const char* fragmentName = m_animDB->GetFragmentDefs().GetTagName(m_fragmentID);
    m_nameEdit->setText(fragmentName);

    const SControllerDef* pControllerDef = m_contexts->m_controllerDef;
    const CTagDefinition& tagDef = pControllerDef->m_scopeIDs;
    m_scopeTagsControl.SetTagDef(&tagDef);

    const SFragmentDef& fragmentDef = pControllerDef->GetFragmentDef(m_fragmentID);
    const CTagDefinition* pFragTagDef = pControllerDef->GetFragmentTagDef(m_fragmentID);

    const ActionScopes scopeMask = fragmentDef.scopeMaskList.GetDefault();
    STagState<sizeof(ActionScopes)> tsScopeMask;
    tsScopeMask.SetFromInteger(scopeMask);
    m_scopeTagsControl.SetTagState(STagStateBase(tsScopeMask));

    m_btnIsPersistent->setChecked((fragmentDef.flags & SFragmentDef::PERSISTENT) != 0);
    m_btnAutoReinstall->setChecked((fragmentDef.flags & SFragmentDef::AUTO_REINSTALL_BEST_MATCH) != 0);

    InitialiseFragmentTags(pFragTagDef);
    InitialiseFragmentADBs();

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::AddFragADBInternal(const QString& ADBDisplayName, const QString& normalizedFilename)
{
    m_vFragADBFiles.push_back(SFragmentTagEntry(ADBDisplayName, normalizedFilename));

    QVariant v;
    v.setValue(m_vFragADBFiles.size() - 1);
    m_FragFileComboBox->addItem(ADBDisplayName, v);
}


//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::AddFragADB(const QString& filename)
{
    const QString ADBDisplayName = Path::GetFile(filename);

    for (TEntryContainer::const_iterator it = m_vFragADBFiles.begin(); it != m_vFragADBFiles.end(); ++it)
    {
        if (QString::compare(ADBDisplayName, it->displayName, Qt::CaseInsensitive) == 0)
        {
            return;
        }
    }

    const QString normalizedFilename = GetNormalizedFilenameString(filename);

    AddFragADBInternal(ADBDisplayName, normalizedFilename);
}

//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::InitialiseFragmentADBsRec(const QString& baseDir)
{
    ICryPak* pCryPak = gEnv->pCryPak;
    _finddata_t fd;

    intptr_t handle = pCryPak->FindFirst((baseDir + QStringLiteral("*")).toUtf8().data(), &fd);
    if (handle != -1)
    {
        do
        {
            if (fd.name[0] == '.')
            {
                continue;
            }

            QString filename(baseDir + fd.name);
            if (fd.attrib & _A_SUBDIR)
            {
                InitialiseFragmentTagsRec(filename + "/");
            }
            else
            {
                if (filename.right(4) == ".adb")
                {
                    AddFragADB(filename);
                }
            }
        } while (pCryPak->FindNext(handle, &fd) >= 0);

        pCryPak->FindClose(handle);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::InitialiseFragmentADBs()
{
    InitialiseFragmentADBsRec (MANNEQUIN_FOLDER);

    const char* filename = m_animDB->FindSubADBFilenameForID(m_fragmentID);
    const QString cleanName = Path::GetFile(filename);

    TEntryContainer::const_iterator it = m_vFragADBFiles.begin();
    for (; it != m_vFragADBFiles.end(); ++it)
    {
        if (QString::compare(cleanName, it->displayName, Qt::CaseInsensitive) == 0)
        {
            break;
        }
    }

    if (it != m_vFragADBFiles.end())
    {
        m_nOldADBFileIndex = m_FragFileComboBox->findText(it->displayName);
        if (m_nOldADBFileIndex != -1)
        {
            m_FragFileComboBox->setCurrentIndex(m_nOldADBFileIndex);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::ResetFragmentADBs()
{
    m_FragFileComboBox->clear();
    m_vFragADBFiles.clear();

    InitialiseFragmentADBs();
}


//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::InitialiseFragmentTagsRec(const QString& baseDir)
{
    ICryPak* pCryPak = gEnv->pCryPak;
    _finddata_t fd;

    intptr_t handle = pCryPak->FindFirst((baseDir + QStringLiteral("*.xml")).toUtf8().data(), &fd);
    if (handle != -1)
    {
        do
        {
            if (fd.name[0] == '.')
            {
                continue;
            }

            QString filename(baseDir + fd.name);
            if (fd.attrib & _A_SUBDIR)
            {
                InitialiseFragmentTagsRec(filename + "/");
            }
            else
            {
                if (XmlNodeRef root = GetISystem()->LoadXmlFromFile(filename.toUtf8().data()))
                {
                    // Ignore non-tagdef XML files
                    QString tag = root->getTag();
                    if (tag == "TagDefinition")
                    {
                        IAnimationDatabaseManager& db = gEnv->pGame->GetIGameFramework()->GetMannequinInterface().GetAnimationDatabaseManager();
                        const CTagDefinition* pTagDef = db.LoadTagDefs(filename.toUtf8().data(), true);
                        if (pTagDef)
                        {
                            AddTagDef(pTagDef->GetFilename());
                        }
                    }
                }
            }
        } while (pCryPak->FindNext(handle, &fd) >= 0);

        pCryPak->FindClose(handle);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::InitialiseFragmentTags(const CTagDefinition* pTagDef)
{
    m_entries.clear();
    AddTagDefInternal("", "");

    InitialiseFragmentTagsRec(MANNEQUIN_FOLDER);

    SelectTagDefByTagDef(pTagDef);
}

//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::OnOK()
{
    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    IMannequinEditorManager* pManEditMan = mannequinSys.GetMannequinEditorManager();
    assert(pManEditMan);

    m_fragmentName = m_nameEdit->text();

    const QString commonErrorMessage = tr("Failed to create FragmentID with name '%1'.\n  Reason:\n\n  %2").arg(m_fragmentName);

    EModifyFragmentIdResult result = pManEditMan->RenameFragmentID(m_animDB->GetFragmentDefs(), m_fragmentID, m_fragmentName.toUtf8().data());

    if (result != eMFIR_Success)
    {
        switch (result)
        {
        case eMFIR_DuplicateName:
            QMessageBox::critical(this, QString(), commonErrorMessage.arg(tr("There is already a FragmentID with this name.")));
            break;

        case eMFIR_InvalidNameIdentifier:
            QMessageBox::critical(this, QString(), commonErrorMessage.arg(tr("Invalid name identifier for a FragmentID.\n  A valid name can only use a-Z, A-Z, 0-9 and _ characters and cannot start with a digit.")));
            break;

        case eMFIR_UnknownInputTagDefinition:
            QMessageBox::critical(this, QString(), commonErrorMessage.arg(tr("Unknown input tag definition. Your changes cannot be saved.")));
            break;

        case eMFIR_InvalidFragmentId:
            QMessageBox::critical(this, QString(), commonErrorMessage.arg(tr("Invalid FragmentID. Your changes cannot be saved.")));
            break;

        default:
            assert(false);
            break;
        }
        return;
    }

    ProcessFragmentTagChanges();
    ProcessFlagChanges();
    ProcessScopeTagChanges();

    // All validation has passed so let the dialog close

    accept();
}

//////////////////////////////////////////////////////////////////////////
const QString CMannTagEditorDialog::GetCurrentADBFileSel() const
{
    int nIndex = m_FragFileComboBox->currentIndex();
    if (nIndex == -1)
    {
        return QString();
    }

    if (m_nOldADBFileIndex == nIndex)
    {
        return QString();
    }

    return m_vFragADBFiles[m_FragFileComboBox->itemData(nIndex).toInt()].filename;
}

//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::ProcessFragmentADBChanges()
{
    const QString sADBFileName = GetCurrentADBFileSel();

    if (sADBFileName.isEmpty())
    {
        return;
    }

    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    IMannequinEditorManager* pMannequinEditorManager = mannequinSys.GetMannequinEditorManager();
    CRY_ASSERT(pMannequinEditorManager);

    const char* const currentSubADBFragmentFilterFilename = m_animDB->FindSubADBFilenameForID(m_fragmentID);
    if (currentSubADBFragmentFilterFilename && currentSubADBFragmentFilterFilename[0])
    {
        pMannequinEditorManager->RemoveSubADBFragmentFilter(m_animDB, currentSubADBFragmentFilterFilename, m_fragmentID);
    }
    pMannequinEditorManager->AddSubADBFragmentFilter(m_animDB, sADBFileName.toUtf8().data(), m_fragmentID);

    CMannequinDialog::GetCurrentInstance()->FragmentBrowser()->SetADBFileNameTextToCurrent();
}


//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::ProcessFragmentTagChanges()
{
    const SControllerDef* pControllerDef = m_contexts->m_controllerDef;
    assert(pControllerDef);

    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();

    const QString tagsFilename = GetSelectedTagDefFilename();
    const bool hasFilename = !tagsFilename.isEmpty();
    const CTagDefinition* pTagDef = hasFilename ? mannequinSys.GetAnimationDatabaseManager().LoadTagDefs(tagsFilename.toUtf8().data(), true) : NULL;
    const CTagDefinition* pCurFragFragTagDef = pControllerDef->GetFragmentTagDef(m_fragmentID);

    if (pTagDef == pCurFragFragTagDef)
    {
        return;
    }

    IMannequinEditorManager* pManEditMan = mannequinSys.GetMannequinEditorManager();
    assert(pManEditMan);

    pManEditMan->SetFragmentTagDef(pControllerDef->m_fragmentIDs, m_fragmentID, pTagDef);
}


//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::ProcessFlagChanges()
{
    const SControllerDef* pControllerDef = m_contexts->m_controllerDef;
    assert(pControllerDef);
    SFragmentDef fragmentDef = pControllerDef->GetFragmentDef(m_fragmentID);

    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();

    uint8 flags = 0;
    if (m_btnIsPersistent->isChecked())
    {
        flags |= SFragmentDef::PERSISTENT;
    }
    if (m_btnAutoReinstall->isChecked())
    {
        flags |= SFragmentDef::AUTO_REINSTALL_BEST_MATCH;
    }

    if (flags == fragmentDef.flags)
    {
        return;
    }

    fragmentDef.flags = flags;

    IMannequinEditorManager* pManEditMan = mannequinSys.GetMannequinEditorManager();
    assert(pManEditMan);

    pManEditMan->SetFragmentDef(*pControllerDef, m_fragmentID, fragmentDef);
}

//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::ProcessScopeTagChanges()
{
    const ActionScopes scopeMask = m_contexts->m_controllerDef->GetFragmentDef(m_fragmentID).scopeMaskList.GetDefault();
    auto tagState = m_scopeTagsControl.GetTagState();
    const ActionScopes scopeTags = *((ActionScopes*)(&tagState));

    if (scopeMask == scopeTags)
    {
        return;
    }

    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    IMannequinEditorManager* pManEditMan = mannequinSys.GetMannequinEditorManager();

    assert(pManEditMan);
    assert(m_contexts->m_controllerDef);

    SFragmentDef fragmentDef = m_contexts->m_controllerDef->GetFragmentDef(m_fragmentID);
    fragmentDef.scopeMaskList.Insert(SFragTagState(), scopeTags);

    pManEditMan->SetFragmentDef(*m_contexts->m_controllerDef, m_fragmentID, fragmentDef);

    CMannequinDialog::GetCurrentInstance()->FragmentEditor()->TrackPanel()->SetCurrentFragment();
}

//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::OnEditTagDefs()
{
    const int selIndex = m_tagDefComboBox->currentIndex();
    QString displayName;
    if (selIndex >= 0 && selIndex < m_tagDefComboBox->count())
    {
        displayName = m_tagDefComboBox->currentText();
    }
    CMannTagDefEditorDialog dialog(displayName);
    dialog.exec();

    // Update the list of tag defs in the combo box in case new ones have been created
    m_tagDefComboBox->clear();
    m_entries.clear();
    AddTagDefInternal("", "");

    IMannequinEditorManager* pManEditMan = gEnv->pGame->GetIGameFramework()->GetMannequinInterface().GetMannequinEditorManager();
    DynArray<const CTagDefinition*> tagDefs;
    pManEditMan->GetLoadedTagDefs(tagDefs);

    for (DynArray<const CTagDefinition*>::const_iterator it = tagDefs.begin(), itEnd = tagDefs.end(); it != itEnd; ++it)
    {
        AddTagDef((*it)->GetFilename());
    }

    const CTagDefinition* pFragTagDef = m_contexts->m_controllerDef->GetFragmentTagDef(m_fragmentID);
    SelectTagDefByTagDef(pFragTagDef);
}

//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::AddTagDef(const QString& filename)
{
    const QString tagDefDisplayName = Path::GetFile(filename);

    if (ContainsTagDefDisplayName(tagDefDisplayName))
    {
        return;
    }

    // Only add filenames with "tag" in it
    const bool isFragTagDefinitionFile = filename.contains(QStringLiteral("tag"), Qt::CaseInsensitive);
    if (!isFragTagDefinitionFile)
    {
        return;
    }

    const QString normalizedFilename = GetNormalizedFilenameString(filename);

    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    if (!mannequinSys.GetAnimationDatabaseManager().LoadTagDefs(filename.toUtf8().data(), true))
    {
        return;
    }

    AddTagDefInternal(tagDefDisplayName, normalizedFilename);
}

//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::AddTagDefInternal(const QString& tagDefDisplayName, const QString& normalizedFilename)
{
    m_entries.push_back(SFragmentTagEntry(tagDefDisplayName, normalizedFilename));

    QVariant v;
    v.setValue(m_entries.size() - 1);
    m_tagDefComboBox->addItem(tagDefDisplayName, v);
}


//////////////////////////////////////////////////////////////////////////
bool CMannTagEditorDialog::ContainsTagDefDisplayName(const QString& tagDefDisplayNameToSearch) const
{
    const size_t entriesCount = m_entries.size();
    for (size_t i = 0; i < entriesCount; ++i)
    {
        const SFragmentTagEntry& entry = m_entries[i];
        if (QString::compare(tagDefDisplayNameToSearch, entry.displayName, Qt::CaseInsensitive) == 0)
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
const QString CMannTagEditorDialog::GetSelectedTagDefFilename() const
{
    int nIndex = m_tagDefComboBox->currentIndex();

    if (nIndex != -1)
    {
        return m_entries[m_tagDefComboBox->itemData(nIndex).toInt()].filename;
    }

    return "";
}

//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::SelectTagDefByFilename(const QString& filename)
{
    const QString normalizedFilename = GetNormalizedFilenameString(filename);

    const size_t entriesCount = m_entries.size();
    for (size_t i = 0; i < entriesCount; ++i)
    {
        const SFragmentTagEntry& entry = m_entries[i];
        if (QString::compare(normalizedFilename, entry.filename, Qt::CaseInsensitive) == 0)
        {
            int nIndex = m_tagDefComboBox->findText(entry.displayName);
            if (nIndex != -1)
            {
                m_tagDefComboBox->setCurrentIndex(nIndex);
            }
            return;
        }
    }

    m_tagDefComboBox->setCurrentIndex(0);
}

//////////////////////////////////////////////////////////////////////////
void CMannTagEditorDialog::SelectTagDefByTagDef(const CTagDefinition* pTagDef)
{
    if (!pTagDef)
    {
        m_tagDefComboBox->setCurrentIndex(0);
        return;
    }

    SelectTagDefByFilename(pTagDef->GetFilename());
}


void CMannTagEditorDialog::OnCbnSelchangeFragfileCombo()
{
    ProcessFragmentADBChanges();
}



void CMannTagEditorDialog::OnBnClickedCreateAdbFile()
{
    const char* fragName = (m_fragmentID != FRAGMENT_ID_INVALID) ? m_animDB->GetFragmentDefs().GetTagName(m_fragmentID) : "NoFragment";
    AZStd::string fragNameFullPath = Path::GamePathToFullPath(MANNEQUIN_FOLDER + fragName).toUtf8().data();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), fragNameFullPath.c_str(), tr("ADB Files (*.adb);;All Files (*.*)"));

    if (!fileName.isEmpty())
    {
        const QFileInfo fileInfo(fileName);
        const QString sADBFileName = fileInfo.fileName().toLower();
        const QString sADBFolder = fileInfo.absolutePath();
        if (sADBFileName.isEmpty())
        {
            return;
        }

        const QString sRealFileName = sADBFolder + "/" + sADBFileName;

        if (!fileInfo.exists())
        {
            QFile file(sRealFileName);
            file.open(QIODevice::WriteOnly);
            file.close();
        }

        const QString normalizedFilename = GetNormalizedFilenameString(MANNEQUIN_FOLDER + sADBFileName);
        m_animDB->RemoveSubADBFragmentFilter(m_fragmentID);
        m_animDB->AddSubADBFragmentFilter(normalizedFilename.toUtf8().data(), m_fragmentID);

        ResetFragmentADBs();
    }
}

#include <Mannequin/MannTagEditorDialog.moc>
