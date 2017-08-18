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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_MANNTAGDEFEDITORDIALOG_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_MANNTAGDEFEDITORDIALOG_H
#pragma once


#include <ICryMannequinEditor.h>
#include <ICryMannequinDefs.h>

#include <QDialog>
#include <QModelIndex>

namespace Ui
{
    class MannTagDefEditorDialog;
}

class CTagDefinition;
class CTagDefTreeCtrl;

class TagDefModel;
class TagDefTreeModel;

class CMannTagDefEditorDialog
    : public QDialog
{
    Q_OBJECT
public:
    CMannTagDefEditorDialog(const QString& tagFilename = QString(), QWidget* pParent = NULL);
    virtual ~CMannTagDefEditorDialog();

signals:
    void TagRenamed(const CTagDefinition* tagDef, TagID tagID, const QString& name);
    void TagRemoved(const CTagDefinition* tagDef, TagID tagID);
    void TagAdded(const CTagDefinition* tagDef, const QString& name);

protected:
    friend class TagDefTreeModel;

    struct STagDefPair
    {
        STagDefPair(const CTagDefinition* pOriginal, CTagDefinition* pCopy)
            : m_pOriginal(pOriginal)
            , m_pCopy(pCopy)
            , m_modified(false) {}

        const CTagDefinition* m_pOriginal;
        CTagDefinition* m_pCopy;
        bool m_modified;

        struct SRenameInfo
        {
            QString                     m_newName;
            int32                           m_originalCRC;
            bool                            m_isGroup;
        };
        DynArray<SRenameInfo> m_renameInfo;
    };

    void OnInitDialog();

    void PopulateTagDefList();
    void AddTagDefToList(CTagDefinition* pTagDef, bool select = false);
    void EnableControls();
    bool ConfirmDelete(const QString& text);
    TagID GetSelectedTagID();
    TagGroupID GetSelectedTagGroupID();
    bool IsTagTreeItem(const QModelIndex& hItem, TagID& tagID);

    bool ValidateTagName(const QString& name);
    bool ValidateGroupName(const QString& name);
    bool ValidateTagDefName(QString& name);

    void OnTagDefListItemChanged();
    void OnTagTreeSelChanged();
    bool OnTagTreeEndLabelEdit(const QModelIndex& index, const QString& text);
    void OnPriorityChanged();
    void OnPriorityEditChanged();
    void accept();

private:
    void OnNewDefButton();
    void OnNewGroupButton();
    void OnEditGroupButton();
    void OnDeleteGroupButton();
    void OnNewTagButton();
    void OnEditTagButton();
    void OnDeleteTagButton();
    void OnFilterTagsButton();

    void PopulateTagDefListRec(const QString& baseDir);
    void RemoveUnmodifiedTagDefs(void);


    DynArray<STagDefPair> m_tagDefLocalCopy;

    SSnapshotCollection m_snapshotCollection;

    QString                         m_initialTagFilename;

    TagDefModel* m_tagDefModel;
    TagDefTreeModel* m_tagDefTreeModel;

    QScopedPointer<Ui::MannTagDefEditorDialog> m_ui;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_MANNTAGDEFEDITORDIALOG_H
