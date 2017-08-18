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

#pragma once
#include <QtCore/QAbstractItemModel>

namespace CharacterTool
{
    struct ExplorerEntry;
    struct ExplorerEntryModifyEvent;
    class Explorer;
    class ExplorerModel
        : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        ExplorerModel(Explorer* list, QObject* parent);

        void SetRootByIndex(int index);
        int GetRootIndex() const;
        ExplorerEntry* GetActiveRoot() const;

        static ExplorerEntry* GetEntry(const QModelIndex& index);
        QModelIndex index(int row, int column, const QModelIndex& parent) const override;

        int rowCount(const QModelIndex& parent) const override;
        int columnCount(const QModelIndex& parent) const override;

        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        bool hasChildren(const QModelIndex& parent) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        QModelIndex ModelIndexFromEntry(ExplorerEntry* entry, int column) const;
        QModelIndex NewModelIndexFromEntry(ExplorerEntry* entry, int column) const;
        QModelIndex parent(const QModelIndex& index) const override;

        QMimeData* mimeData(const QModelIndexList& indexes) const override;
        Qt::DropActions supportedDragActions() const override;

    public slots:
        void OnEntryModified(ExplorerEntryModifyEvent& ev);
        void OnBeginAddEntry(ExplorerEntry* entry);
        void OnEndAddEntry();
        void OnBeginRemoveEntry(ExplorerEntry* entry);
        void OnEndRemoveEntry();
    protected:

        bool m_addWithinActiveRoot;
        bool m_removeWithinActiveRoot;
        int m_rootIndex;
        int m_rootSubtree;
        Explorer* m_explorer;
    };
}
