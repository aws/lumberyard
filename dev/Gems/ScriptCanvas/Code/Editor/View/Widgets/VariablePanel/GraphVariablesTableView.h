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
#pragma once

#include <QAbstractItemModel>
#include <QRegExp>
#include <QString>
#include <QSortFilterProxyModel>
#include <QTableView>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

#include <ScriptCanvas/Variable/VariableBus.h>

#include <GraphCanvas/Widgets/StyledItemDelegates/IconDecoratedNameDelegate.h>

namespace ScriptCanvasEditor
{
    class GraphVariablesModel
        : public QAbstractTableModel
        , public ScriptCanvas::GraphVariableManagerNotificationBus::Handler
        , public ScriptCanvas::VariableNotificationBus::MultiHandler
    {
        Q_OBJECT
    public:

        enum ColumnIndex
        {
            Name,
            Type,
            DefaultValue,
            Count
        };

        enum CustomRole
        {
            VarIdRole = Qt::UserRole
        };

        static const char* GetMimeType() { return "lumberyard/x-scriptcanvas-varpanel"; }

        AZ_CLASS_ALLOCATOR(GraphVariablesModel, AZ::SystemAllocator, 0);
        GraphVariablesModel(QObject* parent = nullptr);
        ~GraphVariablesModel();

        // QAbstractTableModel
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role) override;

        Qt::ItemFlags flags(const QModelIndex &index) const override;

        QStringList mimeTypes() const override;
        QMimeData* mimeData(const QModelIndexList &indexes) const override;
        ////

        void SetActiveScene(const AZ::EntityId& scriptCanvasGraphId);
        AZ::EntityId GetScriptCanvasGraphId() const;

        // ScriptCanvas::GraphVariableManagerNotificationBus
        void OnVariableAddedToGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override;
        void OnVariableRemovedFromGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override;
        void OnVariableNameChangedInGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override;
        ////

        // ScriptCanvas::VariableNotificationBus
        void OnVariableValueChanged() override;
        ////

        ScriptCanvas::VariableId FindVariableIdForIndex(const QModelIndex& index) const;
        int FindRowForVariableId(const ScriptCanvas::VariableId& variableId) const;

    signals:
        void VariableAdded(QModelIndex modelIndex);

    private:

        bool IsEditabletype(ScriptCanvas::Data::Type scriptCanvasDataType) const;

        void PopulateSceneVariables();

        AZStd::vector<ScriptCanvas::VariableId> m_variableIds;
        AZ::EntityId m_scriptCanvasGraphId;
    };

    class GraphVariablesModelSortFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphVariablesModelSortFilterProxyModel, AZ::SystemAllocator, 0);
        GraphVariablesModelSortFilterProxyModel(QObject* parent);

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

        void SetFilter(const QString& filter);

    private:
        QString m_filter;
        QRegExp m_filterRegex;
    };

    class GraphVariablesTableView
        : public QTableView
        , public GraphCanvas::SceneNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphVariablesTableView, AZ::SystemAllocator, 0);
        GraphVariablesTableView(QWidget* parent);

        void SetActiveScene(const AZ::EntityId& scriptCanvasGraphId);
        void SetFilter(const QString& filterString);

        // QObject
        void hideEvent(QHideEvent* event) override;
        ////

        // QTableView
        void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
        ////

        // GraphCanvas::SceneNotifications
        void OnSelectionChanged();
        ////

    public slots:
        void OnVariableAdded(QModelIndex modelIndex);
        void OnDeleteSelected();

    signals:
        void SelectionChanged(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds);
        void DeleteVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds);

    private:

        AZ::EntityId                             m_graphCanvasGraphId;
        GraphVariablesModelSortFilterProxyModel* m_proxyModel;
        GraphVariablesModel*                     m_model;
    };
}