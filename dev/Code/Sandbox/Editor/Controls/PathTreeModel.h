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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_PATH_TREE_MODEL_H
#define CRYINCLUDE_EDITOR_CONTROLS_PATH_TREE_MODEL_H
#pragma once

#include <QAbstractItemModel>
#include <vector>

class QPixmap;

class PathTreeModel
    : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Role
    {
        LabelRole = Qt::UserRole,
        DataRole,
        NonLeafNodeRole,
        ChildCountRole
    };

    enum PathDetectionMethod
    {
        PathSeparators,
        Periods
    };

public:
    Q_PROPERTY(QPixmap * DirectoryIcon READ DirectoryIcon WRITE SetDirectoryIcon);

    PathTreeModel(PathDetectionMethod pathDetectionMethod, QObject* parent = nullptr);

    virtual ~PathTreeModel();

    QPixmap* DirectoryIcon() const { return m_directoryIcon.data(); }
    void SetDirectoryIcon(QPixmap* directoryIcon) { m_directoryIcon.reset(directoryIcon); }

    void Clear();
    void AddNode(const QString& path, const QString& data);

    int LeafCount() const;

    void BeginIterativeUpdate();
    void UpdateFrom(PathTreeModel& source);
    void EndUpdate();

    QModelIndex IndexOf(const QString& entry) const;

    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    QVariant data(const QModelIndex& index, int role) const override;

    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;

private:
    class Node;

    void Tidy(Node* node);

    QModelIndex IndexOf(QStringList& path, const Node* node) const;

    QScopedPointer<Node> m_root;
    QStringList m_mimeTypes;

    QScopedPointer<QPixmap> m_directoryIcon;

    PathDetectionMethod m_pathDetectionMethod;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_TREECTRLREPORT_H
