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

#include <QCompleter>
#include <QAbstractItemModel>
#include <QRegExp>
#include <QString>
#include <QSortFilterProxyModel>
#include <QTableView>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <ScriptCanvas/Data/Data.h>

#include <GraphCanvas/Widgets/StyledItemDelegates/IconDecoratedNameDelegate.h>

namespace ScriptCanvasEditor
{
    class VariablePaletteModel
        : public QAbstractTableModel
    {
        Q_OBJECT
    public:

        enum ColumnIndex
        {
            Pinned,
            Type,
            Count
        };

        enum CustomRole
        {
            VarTypeRole = Qt::UserRole
        };

        AZ_CLASS_ALLOCATOR(VariablePaletteModel, AZ::SystemAllocator, 0);

        VariablePaletteModel(QObject* parent = nullptr);

        // QAbstractTableModel
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;
        ////

        QItemSelectionRange GetSelectionRangeForRow(int row);

        void PopulateVariablePalette(const AZStd::unordered_set< AZ::Uuid >& objectTypes);

        ScriptCanvas::Data::Type FindDataForIndex(const QModelIndex& index);
        ScriptCanvas::Data::Type FindDataForTypeName(const AZStd::string& typeName);

        void TogglePendingPinChange(const AZ::Uuid& azVarType);
        const AZStd::unordered_set< AZ::Uuid >& GetPendingPinChanges() const;
        void SubmitPendingPinChanges();

    private:

        QIcon   m_pinIcon;

        AZStd::unordered_set< AZ::Uuid >     m_pinningChanges;

        AZStd::vector<ScriptCanvas::Data::Type> m_variableTypes;
        AZStd::unordered_map<AZStd::string, ScriptCanvas::Data::Type> m_typeNameMapping;
    };
    
    class VariablePaletteSortFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(VariablePaletteSortFilterProxyModel, AZ::SystemAllocator, 0);

        VariablePaletteSortFilterProxyModel(QObject* parent = nullptr);
        
        // QSortFilterProxyModel
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
        ////
        
        void SetFilter(const QString& filter);

    private:
        QString m_filter;
        QRegExp m_testRegex;
    };
    
    class VariablePaletteTableView
        : public QTableView
    {
        Q_OBJECT
    public:
        VariablePaletteTableView(QWidget* parent);
        ~VariablePaletteTableView();
        
        void PopulateVariablePalette(const AZStd::unordered_set< AZ::Uuid >& objectTypes);
        void SetFilter(const QString& filter);

        QCompleter* GetVariableCompleter();
        void TryCreateVariableByTypeName(const AZStd::string& typeName);

        // QObject
        void hideEvent(QHideEvent* hideEvent) override;
        void showEvent(QShowEvent* showEvent) override;
        ////

    public slots:
        void OnClicked(const QModelIndex& modelIndex);

    signals:
        void CreateVariable(const ScriptCanvas::Data::Type& variableType);

    private:

        VariablePaletteSortFilterProxyModel*    m_proxyModel;
        VariablePaletteModel*                   m_model;

        QCompleter*                          m_completer;
    };
}