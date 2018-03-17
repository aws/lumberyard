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
#include "MannAnimDBEditorDialog.h"
#include "MannNewSubADBFilterDialog.h"
#include "MannequinDialog.h"

#include <StringUtils.h>

#include "QtUtil.h"

#include <Mannequin/ui_MannAnimDBEditorDialog.h>

#define KTreeItemStrLength 1024
#define KImageForFilter 0
#define KImageSelectedForFilter 0

Q_DECLARE_METATYPE(SMiniSubADB*);

class MannAnimDBEditorModel
    : public QAbstractItemModel
{
public:
    MannAnimDBEditorModel(IAnimationDatabase* pAnimDB, QObject* parent = nullptr)
        : QAbstractItemModel(parent)
        , m_animDB(pAnimDB)
    {
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        if (!parent.isValid())
        {
            return m_vSubADBs.size();
        }
        const SMiniSubADB& sub = *static_cast<SMiniSubADB*>(parent.internalPointer());
        return sub.vSubADBs.size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return 1;
    }

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override
    {
        if (row < 0 || column < 0 || row >= rowCount(parent) || column >= columnCount(parent))
        {
            return QModelIndex();
        }
        const SMiniSubADB::TSubADBArray& arr = parent.isValid() ? static_cast<SMiniSubADB*>(parent.internalPointer())->vSubADBs : m_vSubADBs;
        return createIndex(row, column, const_cast<SMiniSubADB*>(&arr[row]));
    }

    QModelIndex parent(const QModelIndex& index) const override
    {
        if (!index.isValid())
        {
            return QModelIndex();
        }
        const SMiniSubADB& sub = *static_cast<SMiniSubADB*>(index.internalPointer());
        auto p = findParent(sub);
        if (p.first == nullptr || p.second == -1)
        {
            return QModelIndex();
        }
        auto grandParent = findParent(*p.first);
        return createIndex(grandParent.second, 0, const_cast<SMiniSubADB*>(p.first));
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (index.internalPointer() == nullptr)
        {
            return QVariant();
        }

        SMiniSubADB& sub = *static_cast<SMiniSubADB*>(index.internalPointer());
        if (role == Qt::DisplayRole)
        {
            return Path::GetFile(sub.filename.c_str());
        }
        else if (role == Qt::UserRole)
        {
            return QVariant::fromValue(&sub);
        }
        else if (role == Qt::DecorationRole)
        {
            return QPixmap(QStringLiteral(":/FragmentBrowser/Controls/mann_folder.png"));
        }
        return QVariant();
    }

    void refresh()
    {
        beginResetModel();
        // Get the SubADB tree structure from the CAnimationDatabase in CryAction(.dll)
        m_animDB->GetSubADBFragmentFilters(m_vSubADBs);
        endResetModel();
    }

protected:
    QPair<const SMiniSubADB*, int> findParent(const SMiniSubADB& child, const SMiniSubADB* possibleParent = nullptr) const
    {
        const SMiniSubADB::TSubADBArray& parentArray = possibleParent ? possibleParent->vSubADBs : m_vSubADBs;

        for (int i = 0; i < parentArray.size(); ++i)
        {
            if (parentArray[i].filename == child.filename)
            {
                return qMakePair(possibleParent, i);
            }
            auto result = findParent(child, &parentArray[i]);
            if (result.second != -1)
            {
                return result;
            }
        }

        return qMakePair<const SMiniSubADB*, int>(0, -1);
    }

private:
    IAnimationDatabase* m_animDB;
    SMiniSubADB::TSubADBArray m_vSubADBs;
};

//////////////////////////////////////////////////////////////////////////
CMannAnimDBEditorDialog::CMannAnimDBEditorDialog(IAnimationDatabase* pAnimDB, QWidget* pParent)
    : QDialog(pParent)
    , m_animDB(pAnimDB)
    , m_model(new MannAnimDBEditorModel(pAnimDB))
    , m_ui(new Ui::MannAnimDBEditorDialog)
{
    m_ui->setupUi(this);
    connect(m_model, &QAbstractItemModel::modelReset, this, &CMannAnimDBEditorDialog::EnableControls, Qt::QueuedConnection);
    m_ui->m_adbTree->setModel(m_model);

    CRY_ASSERT(m_animDB);

    // flush changes so nothing is lost.
    CMannequinDialog::GetCurrentInstance()->FragmentEditor()->TrackPanel()->FlushChanges();

    connect(m_ui->m_newContext, &QPushButton::clicked, this, &CMannAnimDBEditorDialog::OnNewContext);
    connect(m_ui->m_editContext, &QPushButton::clicked, this, &CMannAnimDBEditorDialog::OnEditContext);
    connect(m_ui->m_cloneContext, &QPushButton::clicked, this, &CMannAnimDBEditorDialog::OnCloneAndEditContext);
    connect(m_ui->m_deleteContext, &QPushButton::clicked, this, &CMannAnimDBEditorDialog::OnDeleteContext);

    connect(m_ui->m_moveUp, &QPushButton::clicked, this, &CMannAnimDBEditorDialog::OnMoveUp);
    connect(m_ui->m_moveDown, &QPushButton::clicked, this, &CMannAnimDBEditorDialog::OnMoveDown);

    connect(m_ui->m_adbTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMannAnimDBEditorDialog::OnSubADBTreeSelChanged);

    OnInitDialog();
}

//////////////////////////////////////////////////////////////////////////
CMannAnimDBEditorDialog::~CMannAnimDBEditorDialog()
{
    CMannequinDialog::GetCurrentInstance()->LoadNewPreviewFile(NULL);
    CMannequinDialog::GetCurrentInstance()->StopEditingFragment();
}

/////////////////////////////////////////////////////////////////////////
void CMannAnimDBEditorDialog::OnInitDialog()
{
    const CTagDefinition& fragDefs = m_animDB->GetFragmentDefs();
    const CTagDefinition& tagDefs = m_animDB->GetTagDefs();

    std::vector<string> TagNames;
    RetrieveTagNames(TagNames, fragDefs);
    RetrieveTagNames(TagNames, tagDefs);

    m_model->refresh();

    EnableControls();
}

//////////////////////////////////////////////////////////////////////////

void CMannAnimDBEditorDialog::RetrieveTagNames(std::vector<string>& tagNames, const CTagDefinition& tagDef)
{
    tagNames.clear();
    const TagID maxTag = tagDef.GetNum();
    tagNames.reserve(maxTag);
    for (TagID it = 0; it < maxTag; ++it)
    {
        const string TagName = tagDef.GetTagName(it);
        tagNames.push_back(TagName);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannAnimDBEditorDialog::EnableControls()
{
    const bool isValidRowSelected = m_ui->m_adbTree->selectionModel()->hasSelection();

    m_ui->m_newContext->setEnabled(true);
    m_ui->m_editContext->setEnabled(isValidRowSelected);
    m_ui->m_cloneContext->setEnabled(isValidRowSelected);
    m_ui->m_deleteContext->setEnabled(isValidRowSelected);
    m_ui->m_moveUp->setEnabled(isValidRowSelected);
    m_ui->m_moveDown->setEnabled(isValidRowSelected);
}


//////////////////////////////////////////////////////////////////////////
void CMannAnimDBEditorDialog::OnSubADBTreeSelChanged()
{
    EnableControls();
}

//////////////////////////////////////////////////////////////////////////
SMiniSubADB* CMannAnimDBEditorDialog::GetSelectedSubADB()
{
    const QModelIndex selItem = m_ui->m_adbTree->currentIndex();
    return selItem.data(Qt::UserRole).value<SMiniSubADB*>();
}

//////////////////////////////////////////////////////////////////////////
void CMannAnimDBEditorDialog::OnNewContext()
{
    const QModelIndex selItem = m_ui->m_adbTree->currentIndex();
    const string parText = (!selItem.isValid()) ? m_animDB->GetFilename() : selItem.data().toByteArray().constData();

    CMannNewSubADBFilterDialog dialog(nullptr, m_animDB, parText, CMannNewSubADBFilterDialog::eContextDialog_New, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        m_model->refresh();
    }
}


//////////////////////////////////////////////////////////////////////////
void CMannAnimDBEditorDialog::OnEditContext()
{
    const QModelIndex selItem = m_ui->m_adbTree->currentIndex();
    const QModelIndex parItem = selItem.parent();
    const string parText = parItem.isValid() ? parItem.data().toByteArray().constData() : m_animDB->GetFilename();

    CMannNewSubADBFilterDialog dialog(GetSelectedSubADB(), m_animDB, parText, CMannNewSubADBFilterDialog::eContextDialog_Edit, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        m_model->refresh();
    }
}


//////////////////////////////////////////////////////////////////////////
void CMannAnimDBEditorDialog::OnCloneAndEditContext()
{
    const QModelIndex selItem = m_ui->m_adbTree->currentIndex();
    const QModelIndex parItem = selItem.parent();
    const string parText = parItem.isValid() ? parItem.data().toByteArray().constData() : m_animDB->GetFilename();

    CMannNewSubADBFilterDialog dialog(GetSelectedSubADB(), m_animDB, parText, CMannNewSubADBFilterDialog::eContextDialog_CloneAndEdit, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        m_model->refresh();
    }
}


//////////////////////////////////////////////////////////////////////////
void CMannAnimDBEditorDialog::OnDeleteContext()
{
    SMiniSubADB* pSelectedSubADB = GetSelectedSubADB();
    if (pSelectedSubADB)
    {
        m_animDB->DeleteSubADBFilter(string(Path::GetFile(pSelectedSubADB->filename.c_str()).toUtf8().data()));
        m_model->refresh();
    }
}



//////////////////////////////////////////////////////////////////////////
void CMannAnimDBEditorDialog::OnMoveUp()
{
    SMiniSubADB* pSelectedSubADB = GetSelectedSubADB();
    if (pSelectedSubADB)
    {
        const string selectedFilename = pSelectedSubADB->filename;
        const bool bMoved = m_animDB->MoveSubADBFilter(selectedFilename, true);
        if (bMoved)
        {
            m_model->refresh();
        }
        // find the item at it's new location and select it, so user can keep moving it if they want too.
        const QModelIndexList index = m_model->match(m_model->index(0, 0), Qt::DisplayRole, Path::GetFile(selectedFilename.c_str()), 1, Qt::MatchFixedString);
        if (!index.isEmpty())
        {
            m_ui->m_adbTree->setCurrentIndex(index.first());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannAnimDBEditorDialog::OnMoveDown()
{
    SMiniSubADB* pSelectedSubADB = GetSelectedSubADB();
    if (pSelectedSubADB)
    {
        const string selectedFilename = pSelectedSubADB->filename;
        const bool bMoved = m_animDB->MoveSubADBFilter(selectedFilename, false);
        if (bMoved)
        {
            m_model->refresh();
        }
        // find the item at it's new location and select it, so user can keep moving it if they want too.
        const QModelIndexList index = m_model->match(m_model->index(0, 0), Qt::DisplayRole, Path::GetFile(selectedFilename.c_str()), 1, Qt::MatchFixedString);
        if (!index.isEmpty())
        {
            m_ui->m_adbTree->setCurrentIndex(index.first());
        }
    }
}

#include <Mannequin/MannAnimDBEditorDialog.moc>
