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

#include <QScopedPointer>
#include <QFrame>
#include <QMap>
#include <QVariant>
#include <QMenu>
#include <QTimer>

#include <AzCore/std/chrono/chrono.h>

namespace Ui
{
    class FilteredSearchWidget;
}

class FlowLayout;
class QTreeView;
class QSortFilterProxyModel;
class QStandardItemModel;
class QStandardItem;
class QSettings;
class QLineEdit;
class QPushButton;
class QLabel;
class QHBoxLayout;
class QIcon;
class QBoxLayout;


namespace AzQtComponents
{
    class Style;

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

    class SearchTypeSelectorTreeView;

    class AZ_QT_COMPONENTS_API SearchTypeSelector : public QMenu
    {
        Q_OBJECT

        // SearchTypeSelector popup menus are fixed width. Set the fixed width via this if you want something non-default.
        // In a stylesheet, set it this way:
        //
        // qproperty-fixedWidth: yourIntegerVirtualPixelValueHere;
        //
        Q_PROPERTY(int fixedWidth READ fixedWidth WRITE setFixedWidth)

        // SearchTypeSelector popup menus make a decent guess of how big the contents are in order to properly
        // position the menu above or below the parent button. But even with the guess, there is configurable
        // padding not taken into account. That extra padding along the bottom can be tweaked by setting
        // the heightEstimatePadding value.
        //
        // In a stylesheet, set it this way:
        //
        // qproperty-heightEstimatePadding: yourIntegerVirtualPixelValueHere;
        //
        Q_PROPERTY(int heightEstimatePadding READ heightEstimatePadding WRITE setHeightEstimatePadding)

        // Set this to false in order to hide the text filter, usually on small numbers of items.
        // Defaults to true.
        //
        // In a stylesheet, set it this way:
        //
        // qproperty-lineEditSearchVisible: 0;
        //
        Q_PROPERTY(bool lineEditSearchVisible READ lineEditSearchVisible WRITE setLineEditSearchVisible);

        // The margin to apply around the line edit search field's layout.
        // Defaults to 4.
        //
        // In a stylesheet, set it this way:
        //
        // qproperty-searchLayoutMargin: 0;
        //
        Q_PROPERTY(int searchLayoutMargin READ searchLayoutMargin WRITE setSearchLayoutMargin);

    public:
        SearchTypeSelector(QPushButton* parent = nullptr);
        QTreeView* GetTree();
        void Setup(const SearchTypeFilterList& searchTypes);

        int fixedWidth() const { return m_fixedWidth; }
        void setFixedWidth(int newFixedWidth);

        int heightEstimatePadding() const { return m_heightEstimatePadding; }
        void setHeightEstimatePadding(int newHeightEstimatePadding);

        bool lineEditSearchVisible() const;
        void setLineEditSearchVisible(bool visible);

        int searchLayoutMargin() const;
        void setSearchLayoutMargin(int newMargin);

    signals:
        void TypeToggled(int id, bool enabled);

    private slots:
        void FilterTextChanged(const QString& newFilter);

    protected:
        void estimateTableHeight(QStandardItem* firstCategory, int numCategories, QStandardItem* firstItem, int numItems);
        void resetData();

        // can be used to override the logic when adding items in RepopulateDataModel
        virtual bool filterItemOut(int index, bool itemMatchesFilter, bool categoryMatchesFilter);
        virtual void initItem(QStandardItem* item, const SearchTypeFilter& filter, int unfilteredDataIndex);

        void showEvent(QShowEvent* e) override;

        virtual void RepopulateDataModel();
        void maximizeGeometryToFitScreen();

        SearchTypeSelectorTreeView* m_tree;
        QStandardItemModel* m_model;
        const SearchTypeFilterList* m_unfilteredData;
        QVector<int> m_filteredItemIndices;
        QString m_filterString;
        bool m_settingUp = false;
        int m_fixedWidth = 256;
        QLineEdit* m_searchField = nullptr;
        QBoxLayout* m_searchLayout = nullptr;
        int m_estimatedTableHeight = 0;
        int m_heightEstimatePadding = 10;
        int m_searchLayoutMargin = 4;
        bool m_lineEditSearchVisible = true;
    };

    class AZ_QT_COMPONENTS_API FilteredSearchWidget
        : public QFrame
    {
        Q_OBJECT
        Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY placeholderTextChanged)
        Q_PROPERTY(QString textFilter READ textFilter WRITE SetTextFilter NOTIFY TextFilterChanged)
        Q_PROPERTY(bool textFilterFillsWidth READ textFilterFillsWidth WRITE setTextFilterFillsWidth NOTIFY textFilterFillsWidthChanged)

    public:
        struct Config
        {
        };

        /*!
         * Loads the button config data from a settings object.
         */
        static Config loadConfig(QSettings& settings);

        /*!
         * Returns default button config data.
         */
        static Config defaultConfig();

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

        QString placeholderText() const;
        void setPlaceholderText(const QString& placeholderText);

        QString textFilter() const;
        bool hasStringFilter() const;

        bool textFilterFillsWidth() const;
        void setTextFilterFillsWidth(bool fillsWidth);

        void clearLabelText();
        void setLabelText(const QString& newLabelText);
        QString labelText() const;

    signals:
        void TextFilterChanged(const QString& activeTextFilter);
        void TypeFilterChanged(const SearchTypeFilterList& activeTypeFilters);

        void placeholderTextChanged(const QString& placeholderText);
        void textFilterFillsWidthChanged(bool fillsWidth);

    public slots:
        virtual void ClearTypeFilter();

        virtual void SetFilterStateByIndex(int index, bool enabled);

        void SetFilterState(int index, bool enabled) { SetFilterStateByIndex(index, enabled); }

        void readSettings(QSettings& settings, const QString& widgetName);
        void writeSettings(QSettings& settings, const QString& widgetName);

    protected:
        void emitTypeFilterChanged();
        QLineEdit* filterLineEdit() const;
        QPushButton* filterTypePushButton() const;
        SearchTypeSelector* filterTypeSelector() const;
        const SearchTypeFilterList& typeFilters() const;

        virtual FilterCriteriaButton* createCriteriaButton(const SearchTypeFilter& filter, int filterIndex);

        QPushButton* assetTypeSelectorButton() const;

    private slots:
        void UpdateTextFilterWidth();
        void OnClearFilterContextMenu(const QPoint& pos);

        void OnTextChanged(const QString& activeTextFilter);
        void UpdateTextFilter();

    protected:
        SearchTypeFilterList m_typeFilters;
        FlowLayout* m_flowLayout;
        Ui::FilteredSearchWidget* m_ui;
        SearchTypeSelector* m_selector;
        QMap<int, FilterCriteriaButton*> m_typeButtons;
        bool m_textFilterFillsWidth;

    private:
        int FindFilterIndex(const QString& category, const QString& displayName) const;

        QTimer m_inputTimer;
        QMenu* m_filterMenu;

        static const char* s_filterDataProperty;

        friend class Style;

        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const Config& config);
    };
}
