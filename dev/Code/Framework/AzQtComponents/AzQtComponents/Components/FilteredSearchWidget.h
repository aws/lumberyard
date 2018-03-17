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

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QWidget>
#include <QScopedPointer>
#include <QFrame>
#include <QMap>
#include <QVariant>
#include <QMenu>

namespace Ui
{
    class FilteredSearchWidget;
}

class FlowLayout;
class QTreeView;
class QSortFilterProxyModel;
class QStandardItemModel;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API FilterCriteriaButton
        : public QFrame
    {
        Q_OBJECT

    public:
        explicit FilterCriteriaButton(QString labelText, QWidget* parent = nullptr);

    signals:
        void RequestClose();
    };

    struct AZ_QT_COMPONENTS_API SearchTypeFilter
    {
        QString category;
        QString displayName;
        QVariant metadata;
        bool enabled = false;

        SearchTypeFilter() {}
        SearchTypeFilter(QString category, QString displayName, QVariant metadata = {})
            : category(category)
            , displayName(displayName)
            , metadata(metadata)
        {
        }
    };
    using SearchTypeFilterList = QVector<SearchTypeFilter>;

    class AZ_QT_COMPONENTS_API SearchTypeSelector : public QMenu
    {
        Q_OBJECT

    public:
        SearchTypeSelector(QWidget* parent = nullptr);
        void Setup(const SearchTypeFilterList& searchTypes);

        QSize sizeHint() const override;

    signals:
        void TypeToggled(int id, bool enabled);

    private:
        QTreeView* m_tree;
        QStandardItemModel* m_model;
        bool m_settingUp = false;
    };

    class AZ_QT_COMPONENTS_API FilteredSearchWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit FilteredSearchWidget(QWidget* parent = nullptr);
        ~FilteredSearchWidget() override;

        void SetTypeFilterVisible(bool visible);
        void SetTypeFilters(const SearchTypeFilterList& typeFilters);
        void AddTypeFilter(const SearchTypeFilter& typeFilter);

        inline void AddTypeFilter(QString category, QString displayName, QVariant metadata = {})
        {
            AddTypeFilter(SearchTypeFilter(category, displayName, metadata)); 
        }

        void SetTextFilterVisible(bool visible);
        void ClearTextFilter();

    signals:
        void TextFilterChanged(const QString& activeTextFilter);
        void TypeFilterChanged(const SearchTypeFilterList& activeTypeFilters);

    public slots:
        void ClearTypeFilter();

    private slots:
        void UpdateClearIcon();
        void SetFilterState(int index, bool enabled);

    private:
        SearchTypeFilterList m_typeFilters;
        QMap<int, FilterCriteriaButton*> m_typeButtons;
        Ui::FilteredSearchWidget* m_ui;
        FlowLayout* m_flowLayout;
        QMenu* m_filterMenu;
        SearchTypeSelector* m_selector;
        static const char* s_filterDataProperty;
    };
}
