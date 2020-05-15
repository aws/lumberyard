/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <QAbstractItemModel>
#include <QString>

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace GraphCanvas
{
    class ComboBoxModelInterface
    {
    public:
        ComboBoxModelInterface() = default;
        ~ComboBoxModelInterface() = default;

        virtual void SetFontScale(qreal fontScale) = 0;

        virtual QString GetNameForIndex(const QModelIndex& modelIndex) const = 0;
        virtual QModelIndex FindIndexForName(const QString& name) const = 0;

        virtual QModelIndex GetDefaultIndex() const = 0;

        // Methods for the display list
        virtual QAbstractItemModel* GetDropDownItemModel() = 0;
        virtual int GetSortColumn() const = 0;
        virtual int GetFilterColumn() const = 0;

        virtual QModelIndex GetNextIndex(const QModelIndex& modelIndex) const = 0;
        virtual QModelIndex GetPreviousIndex(const QModelIndex& modelIndex) const = 0;
        ////

        // Methods for the Autocompleter
        virtual QAbstractListModel* GetCompleterItemModel() = 0;
        virtual int GetCompleterColumn() const = 0;
        ////
    };

    class GraphCanvasListComboBoxModel
        : public QAbstractListModel
        , public ComboBoxModelInterface
    {
        Q_OBJECT
    public:

        enum ColumnIndex
        {
            Name = 0
        };
    
        GraphCanvasListComboBoxModel() = default;
        ~GraphCanvasListComboBoxModel() = default;

        void SetFontScale(qreal fontScale) override;
        
        QModelIndex AddElement(const AZStd::string& element);        
        void RemoveElement(const AZStd::string& element);
        void RemoveElement(const QModelIndex& index);
        
        // QAbstractTableModel
        int rowCount(const QModelIndex& parent = QModelIndex()) const override final;
        
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override final;
        ////
        
        // ComboBoxModelInterface
        QString GetNameForIndex(const QModelIndex& index) const override;
        QModelIndex FindIndexForName(const QString& name) const override;

        QModelIndex GetDefaultIndex() const override;

        QAbstractItemModel* GetDropDownItemModel() override;
        int GetSortColumn() const override;
        int GetFilterColumn() const override;

        QModelIndex GetNextIndex(const QModelIndex& modelIndex) const override;
        QModelIndex GetPreviousIndex(const QModelIndex& modelIndex) const override;

        QAbstractListModel* GetCompleterItemModel() override;
        int GetCompleterColumn() const override;
        ////

    private:

        virtual QVariant GetRoleData(const QModelIndex& index, int role) const;

        qreal m_fontScale = 1.0;

        AZStd::vector< QString > m_elements;        
    };
}