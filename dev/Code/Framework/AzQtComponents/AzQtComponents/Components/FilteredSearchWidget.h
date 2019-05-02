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

#include <AzCore/std/chrono/chrono.h>
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QWidget>
#include <QScopedPointer>
#include <QFrame>
#include <QMap>
#include <QVariant>
#include <QMenu>
#include <QTimer>

namespace Ui
{
    class FilteredSearchWidget;
}

class FlowLayout;
class QHBoxLayout;
class QLabel;
class QTreeView;
class QSortFilterProxyModel;
class QStandardItem;
class QStandardItemModel;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API FilterCriteriaButton
        : public QFrame
    {
        Q_OBJECT

    public:
        explicit FilterCriteriaButton(QString labelText, QWidget* parent = nullptr);

    protected:
        QHBoxLayout* m_frameLayout;
        QLabel* m_tagLabel;

    signals:
        void RequestClose();
    };

    struct AZ_QT_COMPONENTS_API SearchTypeFilter
    {
        QString category;
        QString displayName;
        QVariant metadata;
        int globalFilterValue;
        bool enabled = false;

        SearchTypeFilter() {}
        SearchTypeFilter(QString category, QString displayName, QVariant metadata = {}, int globalFilterValue = -1)
            : category(category)
            , displayName(displayName)
            , metadata(metadata)
            , globalFilterValue(globalFilterValue)
        {
        }
    };
    using SearchTypeFilterList = QVector<SearchTypeFilter>;


    class AZ_QT_COMPONENTS_API SearchTypeSelector : public QMenu
    {
        Q_OBJECT

    public:
        SearchTypeSelector(QWidget* parent = nullptr);
        QTreeView* GetTree() { return m_tree; }
        void Setup(const SearchTypeFilterList& searchTypes);

        QSize sizeHint() const override;

    signals:
        void TypeToggled(int id, bool enabled);

    private slots:
        void FilterTextChanged(const QString& newFilter);

    protected:
        virtual void RepopulateDataModel();

        QTreeView* m_tree;
        QStandardItemModel* m_model;
        const SearchTypeFilterList* m_unfilteredData;
        QVector<int> m_filteredItemIndices;
        QString m_filterString;
        bool m_settingUp = false;
        int m_numRows;

    private:
        void AddToDataModel(const SearchTypeFilterList* searchList, QMap<QString, QStandardItem*>& categories, int menuStartPosition, const QIcon* icons);

    };


    class AZ_QT_COMPONENTS_API FilteredSearchWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit FilteredSearchWidget(QWidget* parent = nullptr, bool willUseOwnSelector = false);
        ~FilteredSearchWidget() override;

        void SetTypeFilterVisible(bool visible);
        void SetTypeFilters(const SearchTypeFilterList& typeFilters);
        void AddTypeFilter(const SearchTypeFilter& typeFilter);
        void SetupOwnSelector(SearchTypeSelector* selector);

        inline void AddTypeFilter(QString category, QString displayName, QVariant metadata = {}, int globalFilterValue = -1)
        {
            AddTypeFilter(SearchTypeFilter(category, displayName, metadata, globalFilterValue));
        }

        void SetTextFilterVisible(bool visible);
        void SetTextFilter(const QString& textFilter);
        void ClearTextFilter();

        void AddWidgetToSearchWidget(QWidget* w);
        void SetFilteredParentVisible(bool visible);

        void SetFilterState(const QString& category, const QString& displayName, bool enabled);
        void SetFilterInputInterval(AZStd::chrono::milliseconds milliseconds);

    signals:
        void TextFilterChanged(const QString& activeTextFilter);
        void TypeFilterChanged(const SearchTypeFilterList& activeTypeFilters);

    public slots:
        virtual void ClearTypeFilter();

    protected slots:
        virtual void SetFilterStateByIndex(int index, bool enabled);

    private slots:
        void UpdateClearIcon();
        void OnClearFilterContextMenu(const QPoint& pos);

        void OnTextChanged(const QString& activeTextFilter);
        void UpdateTextFilter();

protected:
        SearchTypeFilterList m_typeFilters;
        FlowLayout* m_flowLayout;
        Ui::FilteredSearchWidget* m_ui;
        SearchTypeSelector* m_selector;
        QMap<int, FilterCriteriaButton*> m_typeButtons;

    private:
        int FindFilterIndex(const QString& category, const QString& displayName) const;

        QTimer m_inputTimer;
        QMenu* m_filterMenu;
        static const char* s_filterDataProperty;
    };

}
