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
#include "MannTagDefEditorDialog.h"
#include <ICryMannequin.h>
#include <IGameFramework.h>
#include <ISourceControl.h>
#include "MannequinDialog.h"

#include <QAbstractListModel>
#include <QMessageBox>
#include <QMimeData>
#include <QInputDialog>
#include <QtUtilWin.h>

#include <Mannequin/ui_MannTagDefEditorDialog.h>

class TagDefModel
    : public QAbstractListModel
{
public:
    TagDefModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        CTagDefinition* item = m_items[index.row()];
        if (role == Qt::DisplayRole)
        {
            return QString(item->GetFilename()).mid(MANNEQUIN_FOLDER.length());
        }
        else if (role == Qt::UserRole)
        {
            return QVariant::fromValue(item);
        }
        return QVariant();
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return m_items.count();
    }

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override
    {
        if (parent.isValid())
        {
            return false;
        }
        beginRemoveRows(QModelIndex(), row, row + count - 1);
        m_items.remove(row, count);
        endRemoveRows();
        return true;
    }

    void addItem(CTagDefinition* item)
    {
        const int row = m_items.count();
        beginInsertRows(QModelIndex(), row, row);
        m_items.push_back(item);
        endInsertRows();
    }

private:
    QVector<CTagDefinition*> m_items;
};

class TagDefTreeModel
    : public QAbstractItemModel
{
public:
    TagDefTreeModel(CMannTagDefEditorDialog* dialog)
        : QAbstractItemModel(dialog)
        , m_selectedTagDef(nullptr)
        , m_dialog(dialog)
    {
    }

    void setSelectedTagDefinition(CTagDefinition* tagDef)
    {
        if (m_selectedTagDef == tagDef)
        {
            return;
        }
        beginResetModel();
        m_selectedTagDef = tagDef;
        endResetModel();
    }

    CTagDefinition* selectedTagDefinition() const
    {
        return m_selectedTagDef;
    }

    QModelIndex addGroup(const QString& name)
    {
        if (m_selectedTagDef == nullptr)
        {
            return QModelIndex();
        }
        const int row = m_selectedTagDef->GetNumGroups();
        beginInsertRows(QModelIndex(), row, row);
        TagGroupID groupID = m_selectedTagDef->AddGroup(name.toUtf8().data());
        m_selectedTagDef->AssignBits();
        endInsertRows();
        return index(row, 0);
    }

    QModelIndex findGroup(const QString& group) const
    {
        const QModelIndexList result = match(index(0, 0), Qt::DisplayRole, group, 1, Qt::MatchExactly);
        if (result.isEmpty())
        {
            return QModelIndex();
        }
        if (result.first().row() < m_selectedTagDef->GetNumGroups())
        {
            return result.first();
        }
        return QModelIndex();
    }

    QModelIndex addTag(const QString& name, const QString& group)
    {
        if (m_selectedTagDef == nullptr)
        {
            return QModelIndex();
        }

        // First ensure that there's enough space to create the new tag
        CTagDefinition tempDefinition = *m_selectedTagDef;
        const TagID tagID = tempDefinition.AddTag(name.toUtf8().data(), group.isEmpty() ? nullptr : group.toUtf8().data());
        const bool success = tempDefinition.AssignBits();
        if (!success)
        {
            QMessageBox::critical(0, tr("Invalid Tag Definition"), tr("Cannot add new tag: please check with a programmer to increase the tag definition size if necessary."));
            return QModelIndex();
        }

        QModelIndex parent = findGroup(group);
        beginInsertRows(parent, rowCount(parent), rowCount(parent));
        *m_selectedTagDef = tempDefinition;
        endInsertRows();
        return index(rowCount(parent) - 1, 0, parent);
    }

    bool isGroup(const QModelIndex& parent) const
    {
        if (!parent.isValid())
        {
            return true;
        }
        // internal id is the parent row - cannot be a group in that case
        if (parent.internalId() != -1)
        {
            return false;
        }
        return parent.row() < m_selectedTagDef->GetNumGroups();
    }

    bool setData(const QModelIndex& index, const QVariant& data, int role = Qt::EditRole) override
    {
        if (role != Qt::EditRole)
        {
            return false;
        }

        if (!m_dialog->OnTagTreeEndLabelEdit(index, data.toString()))
        {
            return true;
        }

        Q_EMIT dataChanged(index, index);
        return true;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::DecorationRole)
        {
            if (isGroup(index))
            {
                return QPixmap(QStringLiteral(":/FragmentBrowser/Controls/mann_folder.png"));
            }
            else
            {
                return QPixmap(QStringLiteral(":/FragmentBrowser/Controls/mann_tag.png"));
            }
        }
        else if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::UserRole)
        {
            int row = index.row();
            // internal id == -1 -> no parent - this is the return path for group names
            if (row < m_selectedTagDef->GetNumGroups() && index.internalId() == -1)
            {
                return role == Qt::UserRole ? QVariant(row) : QVariant(QString::fromLatin1(m_selectedTagDef->GetGroupName(row)));
            }
            // this will be the return path for tag names
            if (index.internalId() == -1)
            {
                row -= m_selectedTagDef->GetNumGroups();
            }
            // search for the row'th tag with parent internalId (-1 for no parent, otherwise that's the gorup id)
            for (int i = 0; i < m_selectedTagDef->GetNum(); ++i)
            {
                if (m_selectedTagDef->GetGroupID(i) == index.internalId())
                {
                    --row;
                }
                if (row == -1)
                {
                    return role == Qt::UserRole ? QVariant(i) : QVariant(m_selectedTagDef->GetTagName(i));
                }
            }
        }
        return QVariant();
    }

    QModelIndex parent(const QModelIndex& index) const override
    {
        // valid index might have an internal id: The parent's row
        // If it does not, the resulting parent index will be invalid
        if (index.isValid())
        {
            return createIndex(index.internalId(), 0, -1);
        }
        return QModelIndex();
    }

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override
    {
        // internal id is set to the parent's row, if any.
        if (row < rowCount(parent) && column < columnCount(parent))
        {
            return createIndex(row, column, parent.row());
        }
        return QModelIndex();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return 1;
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        if (m_selectedTagDef == nullptr)
        {
            return 0;
        }
        if (!parent.isValid())
        {
            // parent not valid: count groups and tags without group
            int rootTags = 0;
            for (int i = 0; i < m_selectedTagDef->GetNum(); ++i)
            {
                if (m_selectedTagDef->GetGroupID(i) == GROUP_ID_NONE)
                {
                    ++rootTags;
                }
            }
            return m_selectedTagDef->GetNumGroups() + rootTags;
        }
        else if (!isGroup(parent))
        {
            // parent is not a group? Certainly doesn't have sub-tags, then...
            return 0;
        }
        else
        {
            // parent valid: count tags with have group id == parent's row
            int groupTags = 0;
            for (int i = 0; i < m_selectedTagDef->GetNum(); ++i)
            {
                if (m_selectedTagDef->GetGroupID(i) == parent.row())
                {
                    ++groupTags;
                }
            }
            return groupTags;
        }
    }

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override
    {
        beginRemoveRows(parent, row, row + count - 1);
        for (int r = row + count - 1; r >= row; --r)
        {
            if (!parent.isValid() && r < m_selectedTagDef->GetNumGroups())
            {
                // remove a group, first remove the tags inside of it
                for (int i = 0; i < m_selectedTagDef->GetNum(); ++i)
                {
                    if (m_selectedTagDef->GetGroupID(i) == r)
                    {
                        m_selectedTagDef->RemoveTag(i);
                    }
                }
                m_selectedTagDef->RemoveGroup(r);
            }
            else if (!parent.isValid())
            {
                // remove a tag not in a group
                int currentRow = r - m_selectedTagDef->GetNumGroups();
                for (int i = 0; i < m_selectedTagDef->GetNum(); ++i)
                {
                    if (m_selectedTagDef->GetGroupID(i) == parent.row() && currentRow-- == 0)
                    {
                        m_selectedTagDef->RemoveTag(i);
                    }
                }
            }
            else
            {
                // remove a tag in a group
                int currentRow = r;
                for (int i = 0; i < m_selectedTagDef->GetNum(); ++i)
                {
                    if (m_selectedTagDef->GetGroupID(i) == parent.row() && currentRow-- == 0)
                    {
                        m_selectedTagDef->RemoveTag(i);
                    }
                }
            }
        }
        m_selectedTagDef->AssignBits();
        endRemoveRows();
        return true;
    }

    QStringList mimeTypes() const override
    {
        return QStringList() << QStringLiteral("x-tagdeftreemodel/x-items");
    }

    Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override
    {
        if (!QAbstractItemModel::canDropMimeData(data, action, row, column, parent))
        {
            return false;
        }
        return row == -1 && column == -1;
    }

    bool dropMimeData(const QMimeData* data, Qt::DropAction, int, int, const QModelIndex& parent) override
    {
        QByteArray encoded = data->data(QStringLiteral("x-tagdeftreemodel/x-items"));
        QDataStream stream(&encoded, QIODevice::ReadOnly);

        while (!stream.atEnd())
        {
            int sourceGroupID;
            stream >> sourceGroupID;
            int tagID;
            stream >> tagID;
            if (sourceGroupID == -1)
            {
                tagID -= m_selectedTagDef->GetNumGroups();
            }

            int sourceRow = sourceGroupID == -1 ? m_selectedTagDef->GetNumGroups() : 0;
            for (int i = 0; i < m_selectedTagDef->GetNum() && i < tagID; ++i)
            {
                if (m_selectedTagDef->GetGroupID(i) == sourceGroupID)
                {
                    ++sourceRow;
                }
            }
            int targetRow = rowCount(parent);

            beginRemoveRows(index(sourceGroupID, 0), sourceRow, sourceRow);
            beginInsertRows(parent, targetRow, targetRow);
            TagGroupID targetGroupID = parent.isValid() ? parent.row() : GROUP_ID_NONE;
            m_selectedTagDef->SetTagGroup(tagID, targetGroupID);
            endInsertRows();
            endRemoveRows();

            //m_tagsTree->SortChildren(m_tagsTree->GetParentItem(hNewItem));
        }
        Q_EMIT dataChanged(index(0, 0, parent), index(rowCount(parent) - 1, 0, parent));

        m_selectedTagDef->AssignBits();
        return true;
    }

    QMimeData* mimeData(const QModelIndexList& indexes) const override
    {
        QMimeData* data = new QMimeData;
        QByteArray encoded;
        QDataStream stream(&encoded, QIODevice::WriteOnly);
        for (const QModelIndex& index : indexes)
        {
            stream << static_cast<int>(index.internalId()) << index.row();
        }
        data->setData(QStringLiteral("x-tagdeftreemodel/x-items"), encoded);
        return data;
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        Qt::ItemFlags f = QAbstractItemModel::flags(index);
        f |= Qt::ItemIsEditable;
        if (isGroup(index) || !index.isValid())
        {
            f |= Qt::ItemIsDropEnabled;
        }
        else
        {
            f |= Qt::ItemIsDragEnabled;
        }
        return f;
    }

private:
    CTagDefinition* m_selectedTagDef;
    CMannTagDefEditorDialog* m_dialog;
};

Q_DECLARE_METATYPE(CTagDefinition*)

//////////////////////////////////////////////////////////////////////////
CMannTagDefEditorDialog::CMannTagDefEditorDialog(const QString& tagFilename, QWidget* pParent)
    : QDialog(pParent)
    , m_initialTagFilename(tagFilename)
    , m_tagDefModel(new TagDefModel(this))
    , m_tagDefTreeModel(new TagDefTreeModel(this))
    , m_ui(new Ui::MannTagDefEditorDialog)
{
    m_ui->setupUi(this);

    CMannequinDialog::GetCurrentInstance()->FragmentEditor()->TrackPanel()->FlushChanges();

    m_ui->m_tagDefList->setModel(m_tagDefModel);
    m_ui->m_tagsTree->setModel(m_tagDefTreeModel);

    OnInitDialog();
}

//////////////////////////////////////////////////////////////////////////
CMannTagDefEditorDialog::~CMannTagDefEditorDialog()
{
    while (m_tagDefLocalCopy.size() > 0)
    {
        CTagDefinition* ptagDef = m_tagDefLocalCopy.back().m_pCopy;
        m_tagDefLocalCopy.pop_back();
        delete ptagDef;
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::OnInitDialog()
{
    connect(m_ui->m_newDefButton, &QToolButton::clicked, this, &CMannTagDefEditorDialog::OnNewDefButton);
    connect(m_ui->m_filterTagsButton, &QToolButton::clicked, this, &CMannTagDefEditorDialog::OnFilterTagsButton);
    m_ui->m_filterTagsButton->setChecked(true);

    connect(m_ui->m_newGroupButton, &QToolButton::clicked, this, &CMannTagDefEditorDialog::OnNewGroupButton);
    connect(m_ui->m_editGroupButton, &QToolButton::clicked, this, &CMannTagDefEditorDialog::OnEditGroupButton);
    connect(m_ui->m_deleteGroupButton, &QToolButton::clicked, this, &CMannTagDefEditorDialog::OnDeleteGroupButton);
    connect(m_ui->m_newTagButton, &QToolButton::clicked, this, &CMannTagDefEditorDialog::OnNewTagButton);
    connect(m_ui->m_editTagButton, &QToolButton::clicked, this, &CMannTagDefEditorDialog::OnEditTagButton);
    connect(m_ui->m_deleteTagButton, &QToolButton::clicked, this, &CMannTagDefEditorDialog::OnDeleteTagButton);

    connect(m_ui->m_tagDefList->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMannTagDefEditorDialog::OnTagDefListItemChanged);
    connect(m_ui->m_tagsTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMannTagDefEditorDialog::OnTagTreeSelChanged);

    void(QSpinBox::* valueChanged)(int) = &QSpinBox::valueChanged;
    connect(m_ui->m_priorityEdit, valueChanged, this, &CMannTagDefEditorDialog::OnPriorityEditChanged);

    PopulateTagDefList();
    EnableControls();
    OnTagTreeSelChanged();

    // now select an entry if we were passed one initially
    if (!m_initialTagFilename.isEmpty())
    {
        const QModelIndexList indexes = m_ui->m_tagDefList->model()->match(m_ui->m_tagDefList->model()->index(0, 0), Qt::DisplayRole, m_initialTagFilename, 1, Qt::MatchExactly);
        if (!indexes.isEmpty())
        {
            m_ui->m_tagDefList->setCurrentIndex(indexes.first());
            m_ui->m_pCurrentTagFile->setText(QString(m_tagDefTreeModel->selectedTagDefinition()->GetFilename()).mid(MANNEQUIN_FOLDER.length()));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::accept()
{
    IMannequinEditorManager* pMgr = gEnv->pGame->GetIGameFramework()->GetMannequinInterface().GetMannequinEditorManager();

    // Do renaming here
    for (uint32 index = 0; index < m_tagDefLocalCopy.size(); ++index)
    {
        const CTagDefinition* pOriginal = m_tagDefLocalCopy[index].m_pOriginal;

        for (uint32 renameIndex = 0; renameIndex < m_tagDefLocalCopy[index].m_renameInfo.size(); ++renameIndex)
        {
            STagDefPair::SRenameInfo& renameInfo = m_tagDefLocalCopy[index].m_renameInfo[renameIndex];

            if (renameInfo.m_isGroup)
            {
                TagGroupID id = pOriginal->FindGroup(renameInfo.m_originalCRC);
                if (id != GROUP_ID_NONE)
                {
                    //CryLog("[TAGDEF]: OnOKButton() (%s): renaming tag group [%s] to [%s]", pOriginal->GetFilename(), pOriginal->GetGroupName(id), renameInfo.m_newName);
                    pMgr->RenameTagGroup(pOriginal, renameInfo.m_originalCRC, renameInfo.m_newName.toUtf8().data());
                }
            }
            else
            {
                TagID id = pOriginal->Find(renameInfo.m_originalCRC);
                if (id != TAG_ID_INVALID)
                {
                    //CryLog("[TAGDEF]: OnOKButton() (%s): renaming tag [%s] to [%s]", pOriginal->GetFilename(), pOriginal->GetGroupName(id), renameInfo.m_newName);
                    pMgr->RenameTag(pOriginal, renameInfo.m_originalCRC, renameInfo.m_newName.toUtf8().data());
                    emit TagRenamed(pOriginal, id, renameInfo.m_newName);
                }
            }
        }
    }

    // Look for removed and added tags

    for (uint32 index = 0; index < m_tagDefLocalCopy.size(); ++index)
    {
        if (m_tagDefLocalCopy[index].m_modified)
        {
            auto pOriginal = m_tagDefLocalCopy[index].m_pOriginal;
            auto pCopy = m_tagDefLocalCopy[index].m_pCopy;

            FragmentID idOriginal = 0;
            FragmentID idCopy = 0;

            // look for removed tags

            while (idOriginal != pOriginal->GetNum())
            {
                if (idCopy != pCopy->GetNum() && pOriginal->GetTagCRC(idOriginal) == pCopy->GetTagCRC(idCopy))
                {
                    ++idCopy;
                }
                else
                {
                    emit TagRemoved(pOriginal, idOriginal);
                }

                ++idOriginal;
            }

            // new tags are always added to the end of the list

            while (idCopy != pCopy->GetNum())
            {
                emit TagAdded(pOriginal, pCopy->GetTagName(idCopy));
                ++idCopy;
            }
        }
    }

    // Make a snapshot
    pMgr->SaveDatabasesSnapshot(m_snapshotCollection);

    // Apply changes
    for (uint32 index = 0; index < m_tagDefLocalCopy.size(); ++index)
    {
        if (m_tagDefLocalCopy[index].m_modified == true)
        {
            pMgr->ApplyTagDefChanges(m_tagDefLocalCopy[index].m_pOriginal, m_tagDefLocalCopy[index].m_pCopy);
        }
    }

    // Reload the snapshot
    pMgr->LoadDatabasesSnapshot(m_snapshotCollection);

    QDialog::accept();
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::OnNewDefButton()
{
    QInputDialog inputDlg(this);
    inputDlg.setLabelText(tr("New Tag Definition Filename (*tags.xml)"));
    inputDlg.setWindowTitle(tr("New Tag Definition File"));
    inputDlg.setWindowFlags(inputDlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    if (inputDlg.exec() == QDialog::Accepted)
    {
        QString text = inputDlg.textValue().trimmed();

        if (!ValidateTagDefName(text))
        {
            return;
        }

        IAnimationDatabaseManager& db = gEnv->pGame->GetIGameFramework()->GetMannequinInterface().GetAnimationDatabaseManager();
        // TODO: This is actually changing the live database - cancelling the tagdef editor will not remove this!
        CTagDefinition* pTagDef = db.CreateTagDefinition(QString(MANNEQUIN_FOLDER + text).toUtf8().data());
        CTagDefinition* pTagDefCopy = new CTagDefinition(*pTagDef);
        STagDefPair tagDefPair(pTagDef, pTagDefCopy);
        m_tagDefLocalCopy.push_back(tagDefPair);

        AddTagDefToList(pTagDefCopy, true);
        m_ui->m_tagDefList->setFocus();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::OnNewGroupButton()
{
    QInputDialog inputDlg(this);
    inputDlg.setLabelText(tr("New Group Name"));
    inputDlg.setWindowTitle(tr("New Group"));
    inputDlg.setWindowFlags(inputDlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    if (inputDlg.exec() == QDialog::Accepted)
    {
        const QString text = inputDlg.textValue().trimmed();

        if (!ValidateGroupName(text))
        {
            return;
        }

        const QModelIndex index = m_tagDefTreeModel->addGroup(text);
        m_ui->m_tagsTree->setCurrentIndex(index);
        m_ui->m_tagsTree->setFocus();

        STagDefPair* pTagDefPair = NULL;
        for (uint32 index = 0; index < m_tagDefLocalCopy.size(); ++index)
        {
            if (m_tagDefLocalCopy[index].m_pCopy == m_tagDefTreeModel->selectedTagDefinition())
            {
                pTagDefPair = &m_tagDefLocalCopy[index];
                pTagDefPair->m_modified = true;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::OnEditGroupButton()
{
    m_ui->m_tagsTree->edit(m_ui->m_tagsTree->currentIndex());
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::OnDeleteGroupButton()
{
    const QModelIndex item = m_ui->m_tagsTree->currentIndex();
    const QString itemText = item.data().toString();

    const QString message = tr("Delete the group '%1' and all contained tags?").arg(itemText);

    if (ConfirmDelete(message))
    {
        m_ui->m_tagsTree->model()->removeRow(item.row(), item.parent());

        STagDefPair* pTagDefPair = NULL;
        for (uint32 index = 0; index < m_tagDefLocalCopy.size(); ++index)
        {
            if (m_tagDefLocalCopy[index].m_pCopy == m_tagDefTreeModel->selectedTagDefinition())
            {
                pTagDefPair = &m_tagDefLocalCopy[index];
                pTagDefPair->m_modified = true;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::OnNewTagButton()
{
    QInputDialog inputDlg(this);
    inputDlg.setLabelText(tr("New Tag Name"));
    inputDlg.setWindowTitle(tr("New Tag Definition"));
    inputDlg.setWindowFlags(inputDlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    if (inputDlg.exec() == QDialog::Accepted)
    {
        const QString text = inputDlg.textValue().trimmed();
        if (!ValidateTagName(text))
        {
            return;
        }

        const TagGroupID groupID = GetSelectedTagGroupID();
        const QString group = groupID != TAG_ID_INVALID ? QString(m_tagDefTreeModel->selectedTagDefinition()->GetGroupName(groupID)) : QString();
        const QModelIndex index = m_tagDefTreeModel->addTag(text, group);
        m_ui->m_tagsTree->expand(index.parent());
        m_ui->m_tagsTree->setFocus();

        STagDefPair* pTagDefPair = NULL;
        for (uint32 index = 0; index < m_tagDefLocalCopy.size(); ++index)
        {
            if (m_tagDefLocalCopy[index].m_pCopy == m_tagDefTreeModel->selectedTagDefinition())
            {
                pTagDefPair = &m_tagDefLocalCopy[index];
                pTagDefPair->m_modified = true;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::OnEditTagButton()
{
    m_ui->m_tagsTree->edit(m_ui->m_tagsTree->currentIndex());
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::OnDeleteTagButton()
{
    const QModelIndex item = m_ui->m_tagsTree->currentIndex();
    const QString itemText = item.data().toString();
    TagID tagID = GetSelectedTagID();

    STagDefPair* pTagDefPair = NULL;
    for (uint32 index = 0; index < m_tagDefLocalCopy.size(); ++index)
    {
        if (m_tagDefLocalCopy[index].m_pCopy == m_tagDefTreeModel->selectedTagDefinition())
        {
            pTagDefPair = &m_tagDefLocalCopy[index];
            break;
        }
    }

    char buffer[4096];
    IMannequinEditorManager* pMgr = gEnv->pGame->GetIGameFramework()->GetMannequinInterface().GetMannequinEditorManager();
    pMgr->GetAffectedFragmentsString(pTagDefPair->m_pOriginal, tagID, buffer, sizeof(buffer));
    const QString message = tr("Deleting tag '%1' will affect:%2\nAre you sure?").arg(itemText).arg(buffer);

    if (ConfirmDelete(message))
    {
        m_ui->m_tagsTree->model()->removeRow(item.row(), item.parent());
        pTagDefPair->m_modified = true;
        RemoveUnmodifiedTagDefs();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CMannTagDefEditorDialog::ConfirmDelete(const QString& text)
{
    return QMessageBox::question(this, tr("Confirm Delete"), text) == QMessageBox::Yes;
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::PopulateTagDefListRec(const QString& baseDir)
{
    const bool filterTags = m_ui->m_filterTagsButton->isChecked();
    const QString filter = filterTags ? TAGS_FILENAME_ENDING + TAGS_FILENAME_EXTENSION : TAGS_FILENAME_EXTENSION;

    ICryPak* pCryPak = gEnv->pCryPak;
    _finddata_t fd;

    intptr_t handle = pCryPak->FindFirst(QString(baseDir + "*").toUtf8().data(), &fd);
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
                PopulateTagDefListRec(filename + "/");
            }
            else
            {
                if (filename.right(filter.length()).compare(filter, Qt::CaseInsensitive) == 0)
                {
                    if (XmlNodeRef root = GetISystem()->LoadXmlFromFile(filename.toUtf8().data()))
                    {
                        // Ignore non-tagdef XML files
                        QString tag = root->getTag();
                        if (tag == "TagDefinition")
                        {
                            IAnimationDatabaseManager& db = gEnv->pGame->GetIGameFramework()->GetMannequinInterface().GetAnimationDatabaseManager();
                            const CTagDefinition* pTagDef = db.LoadTagDefs(filename.toUtf8().data(), true);
                            CTagDefinition* pTagDefCopy = new CTagDefinition(*pTagDef);
                            STagDefPair tagDefPair(pTagDef, pTagDefCopy);
                            m_tagDefLocalCopy.push_back(tagDefPair);

                            AddTagDefToList(pTagDefCopy);
                        }
                    }
                }
            }
        } while (pCryPak->FindNext(handle, &fd) >= 0);

        pCryPak->FindClose(handle);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::RemoveUnmodifiedTagDefs(void)
{
    for (int index = m_tagDefModel->rowCount() - 1; index >= 0; --index)
    {
        CTagDefinition* pItem = m_tagDefModel->index(index).data(Qt::UserRole).value<CTagDefinition*>();
        if (pItem != m_tagDefTreeModel->selectedTagDefinition())
        {
            m_tagDefModel->removeRow(index);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::PopulateTagDefList()
{
    if (m_tagDefModel->rowCount() > 0)
    {
        m_tagDefModel->removeRows(0, m_tagDefModel->rowCount());
    }

    PopulateTagDefListRec(MANNEQUIN_FOLDER);
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::AddTagDefToList(CTagDefinition* pTagDef, bool select)
{
    m_tagDefModel->addItem(pTagDef);

    if (select)
    {
        m_ui->m_tagDefList->setCurrentIndex(m_tagDefModel->index(m_tagDefModel->rowCount() - 1));
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::EnableControls()
{
    bool bTagDefSelected = m_ui->m_tagDefList->selectionModel()->hasSelection();
    m_ui->m_tagsTree->setEnabled(bTagDefSelected);

    bool bTagSelected = false;
    bool bGroupSelected = false;
    const QModelIndex item = m_ui->m_tagsTree->selectionModel()->hasSelection() ? m_ui->m_tagsTree->currentIndex() : QModelIndex();
    if (item.isValid())
    {
        TagID id;
        bTagSelected = IsTagTreeItem(item, id);
        bGroupSelected = !bTagSelected;
    }
    m_ui->m_priorityLabel->setEnabled(bTagSelected);
    m_ui->m_priorityEdit->setEnabled(bTagSelected);

    m_ui->m_newGroupButton->setEnabled(bTagDefSelected);
    m_ui->m_editGroupButton->setEnabled(bGroupSelected);
    m_ui->m_deleteGroupButton->setEnabled(bGroupSelected);
    m_ui->m_newTagButton->setEnabled(bTagDefSelected);
    m_ui->m_editTagButton->setEnabled(bTagSelected);
    m_ui->m_deleteTagButton->setEnabled(bTagSelected);
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::OnTagDefListItemChanged()
{
    if (m_ui->m_tagDefList->selectionModel()->hasSelection())
    {
        m_tagDefTreeModel->setSelectedTagDefinition(m_ui->m_tagDefList->currentIndex().data(Qt::UserRole).value<CTagDefinition*>());
        m_ui->m_pCurrentTagFile->setText(QString(m_tagDefTreeModel->selectedTagDefinition()->GetFilename()).mid(MANNEQUIN_FOLDER.length()));
    }
    else
    {
        m_tagDefTreeModel->setSelectedTagDefinition(nullptr);
        m_ui->m_pCurrentTagFile->clear();
    }

    EnableControls();
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::OnTagTreeSelChanged()
{
    const QModelIndex item = m_ui->m_tagsTree->selectionModel()->hasSelection() ? m_ui->m_tagsTree->currentIndex() : QModelIndex();

    TagID id;
    if (IsTagTreeItem(item, id))
    {
        uint32 priority = m_tagDefTreeModel->selectedTagDefinition()->GetPriority(id);
        priority = std::min(priority, 10u);
        m_ui->m_priorityEdit->setSpecialValueText(QString());
        m_ui->m_priorityEdit->setValue(priority);
    }
    else
    {
        m_ui->m_priorityEdit->setSpecialValueText(QStringLiteral(" "));
        m_ui->m_priorityEdit->setValue(0);
    }

    EnableControls();
}

//////////////////////////////////////////////////////////////////////////
bool CMannTagDefEditorDialog::OnTagTreeEndLabelEdit(const QModelIndex& index, const QString& text)
{
    TagID id;
    if (IsTagTreeItem(index, id))
    {
        // Tag
        if (!ValidateTagName(text))
        {
            return false;
        }

        for (uint32 index = 0; index < m_tagDefLocalCopy.size(); ++index)
        {
            if (m_tagDefLocalCopy[index].m_pCopy == m_tagDefTreeModel->selectedTagDefinition())
            {
                STagDefPair::SRenameInfo renameInfo;
                renameInfo.m_originalCRC = m_tagDefTreeModel->selectedTagDefinition()->GetTagCRC(id);
                renameInfo.m_newName = text;
                renameInfo.m_isGroup = false;

                m_tagDefLocalCopy[index].m_renameInfo.push_back(renameInfo);
                m_tagDefLocalCopy[index].m_modified = true;
                RemoveUnmodifiedTagDefs();
                break;
            }
        }

        m_tagDefTreeModel->selectedTagDefinition()->SetTagName(id, text.toUtf8().data());
    }
    else
    {
        // Group
        if (!ValidateGroupName(text))
        {
            return false;
        }

        const TagGroupID groupID = index.row();

        for (uint32 index = 0; index < m_tagDefLocalCopy.size(); ++index)
        {
            if (m_tagDefLocalCopy[index].m_pCopy == m_tagDefTreeModel->selectedTagDefinition())
            {
                STagDefPair::SRenameInfo renameInfo;
                renameInfo.m_originalCRC = m_tagDefTreeModel->selectedTagDefinition()->GetGroupCRC(groupID);
                renameInfo.m_newName = text;
                renameInfo.m_isGroup = true;

                m_tagDefLocalCopy[index].m_renameInfo.push_back(renameInfo);
                m_tagDefLocalCopy[index].m_modified = true;
                RemoveUnmodifiedTagDefs();
                break;
            }
        }

        m_tagDefTreeModel->selectedTagDefinition()->SetGroupName(groupID, text.toUtf8().data());
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::OnPriorityEditChanged()
{
    if (!m_tagDefTreeModel->selectedTagDefinition())
    {
        return;
    }

    TagID tagID = GetSelectedTagID();
    if (tagID != TAG_ID_INVALID)
    {
        m_tagDefTreeModel->selectedTagDefinition()->SetPriority(tagID, m_ui->m_priorityEdit->value());
        for (uint32 index = 0; index < m_tagDefLocalCopy.size(); ++index)
        {
            if (m_tagDefTreeModel->selectedTagDefinition() == m_tagDefLocalCopy[index].m_pCopy)
            {
                m_tagDefLocalCopy[index].m_modified = true;
                RemoveUnmodifiedTagDefs();
                break;
            }
        }
    }
}
//////////////////////////////////////////////////////////////////////////
TagID CMannTagDefEditorDialog::GetSelectedTagID()
{
    const QModelIndex item = m_ui->m_tagsTree->currentIndex();
    if (item.isValid())
    {
        TagID id;
        if (IsTagTreeItem(item, id))
        {
            return id;
        }
    }
    return TAG_ID_INVALID;
}

//////////////////////////////////////////////////////////////////////////
TagGroupID CMannTagDefEditorDialog::GetSelectedTagGroupID()
{
    const QModelIndex item = m_ui->m_tagsTree->selectionModel()->hasSelection() ? m_ui->m_tagsTree->currentIndex() : QModelIndex();
    if (item.isValid())
    {
        TagID id;
        if (!IsTagTreeItem(item, id))
        {
            return id;
        }
    }
    return TAG_ID_INVALID;
}

//////////////////////////////////////////////////////////////////////////
bool CMannTagDefEditorDialog::IsTagTreeItem(const QModelIndex& hItem, TagID& tagID)
{
    if (!hItem.isValid())
    {
        return false;
    }
    tagID = hItem.data(Qt::UserRole).toInt();
    return !m_tagDefTreeModel->isGroup(hItem);
}

//////////////////////////////////////////////////////////////////////////
bool CMannTagDefEditorDialog::ValidateTagName(const QString& name)
{
    // Check string format
    if (name.isEmpty())
    {
        QMessageBox::critical(this, tr("Invalid Name"), tr("Invalid tag name"));
        return false;
    }

    // Check for duplicates
    if (m_tagDefTreeModel->selectedTagDefinition()->Find(name.toUtf8().data()) != TAG_ID_INVALID)
    {
        QMessageBox::critical(this, tr("Invalid Name"), tr("The tag name '%1' already exists").arg(name));
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CMannTagDefEditorDialog::ValidateGroupName(const QString& name)
{
    // Check string format
    if (name.isEmpty())
    {
        QMessageBox::critical(this, tr("Invalid Name"), tr("Invalid group name"));
        return false;
    }

    // Check for duplicates
    if (m_tagDefTreeModel->selectedTagDefinition()->FindGroup(name.toUtf8().data()) != GROUP_ID_NONE)
    {
        QMessageBox::critical(this, tr("Invalid Name"), tr("The group name '%1' already exists").arg(name));
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CMannTagDefEditorDialog::ValidateTagDefName(QString& name)
{
    // Check string format
    if (name.isEmpty())
    {
        QMessageBox::critical(this, tr("Invalid Name"), tr("Invalid tag definition name"));
        return false;
    }

    if (name.contains('/') || name.contains('\\'))
    {
        QMessageBox::critical(this, tr("Invalid Name"), tr("Invalid tag definition name - cannot contain path information"));
        return false;
    }

    // Ensure the name has the correct ending and extension
    QString filename(name);
    int dot = filename.lastIndexOf('.');
    if (dot > 0)
    {
        filename.remove(dot, filename.length() - dot);
    }

    if (filename.right(TAGS_FILENAME_ENDING.length()).compare(TAGS_FILENAME_ENDING, Qt::CaseInsensitive) != 0)
    {
        filename += TAGS_FILENAME_ENDING;
    }

    filename += TAGS_FILENAME_EXTENSION;

    // Check for duplicates
    IAnimationDatabaseManager& db = gEnv->pGame->GetIGameFramework()->GetMannequinInterface().GetAnimationDatabaseManager();
    if (db.FindTagDef(QString(MANNEQUIN_FOLDER + filename).toUtf8().data()))
    {
        QMessageBox::critical(this, tr("Invalid Name"), tr("The tag definition name '%1' already exists").arg(filename));
        return false;
    }

    name = filename;
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CMannTagDefEditorDialog::OnFilterTagsButton()
{
    PopulateTagDefList();
    EnableControls();
}

#include <Mannequin/MannTagDefEditorDialog.moc>
