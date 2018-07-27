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

#include <AzQtComponents/Components/FilteredSearchWidget.h>

#include <Components/ui_FilteredSearchWidget.h>

#include <AzToolsFramework/UI/Qt/FlowLayout.h>

#include <QMenu>
#include <QLabel>
#include <QVBoxLayout>
#include <QTreeView>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QScopedValueRollback>

namespace AzQtComponents
{
    const char* FilteredSearchWidget::s_filterDataProperty = "filterData";

    FilterCriteriaButton::FilterCriteriaButton(QString labelText, QWidget* parent)
        : QFrame(parent)
    {
        setStyleSheet(styleSheet() + "padding: 0px; border-radius: 2px; border: 1px solid #808080; background-color: #404040");
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumSize(60, 24);

        QHBoxLayout* frameLayout = new QHBoxLayout(this);
        frameLayout->setMargin(0);
        frameLayout->setSpacing(4);
        frameLayout->setContentsMargins(4, 1, 4, 1);

        QLabel* tagLabel = new QLabel(this);
        tagLabel->setStyleSheet(tagLabel->styleSheet() + "border: 0px; background-color: transparent;");
        tagLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        tagLabel->setMinimumSize(24, 22);
        tagLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        tagLabel->setText(labelText);

        QIcon closeIcon(":/stylesheet/img/titlebar/titlebar_close.png");

        QPushButton* button = new QPushButton(this);
        button->setStyleSheet(button->styleSheet() + "border: 0px;");
        button->setFlat(true);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setFixedSize(QSize(16, 16));
        button->setProperty("iconButton", "true");
        button->setMouseTracking(true);
        button->setIcon(closeIcon);

        frameLayout->addWidget(tagLabel);
        frameLayout->addWidget(button);

        connect(button, &QPushButton::clicked, this, &FilterCriteriaButton::RequestClose);
    }

    SearchTypeSelector::SearchTypeSelector(QWidget* parent /* = nullptr */)
        : QMenu(parent)
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(QMargins());

        m_tree = new QTreeView(this);
        layout->addWidget(m_tree);

        m_model = new QStandardItemModel(this);
        m_tree->setModel(m_model);
        m_tree->setHeaderHidden(true);

        connect(m_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item)
        {
            // Don't emit itemChanged while Setup is running
            if (!m_settingUp)
            {
                emit TypeToggled(item->data().toInt(), item->checkState() == Qt::Checked);
            }
        });

        setMaximumSize(500, 750);
    }

    QSize SearchTypeSelector::sizeHint() const
    {
        return m_tree->sizeHint();
    }

    void SearchTypeSelector::Setup(const SearchTypeFilterList& searchTypes)
    {
        m_model->clear();

        QScopedValueRollback<bool> setupGuard(m_settingUp, true);
        QMap<QString, QStandardItem*> categories;
        for (int i = 0, length = searchTypes.length(); i < length; ++i)
        {
            const SearchTypeFilter& filter = searchTypes[i];
            QStandardItem* categoryItem;
            if (categories.contains(filter.category))
            {
                categoryItem = categories[filter.category];
            }
            else
            {
                categoryItem = new QStandardItem(filter.category);
                categories[filter.category] = categoryItem;
                m_model->appendRow(categoryItem);
                categoryItem->setEditable(false);
            }

            QStandardItem* item = new QStandardItem(filter.displayName);
            item->setCheckable(true);
            item->setCheckState(filter.enabled ? Qt::Checked : Qt::Unchecked);
            item->setData(i);
            categoryItem->appendRow(item);
            item->setEditable(false);
        }

        m_tree->expandAll();
    }

    FilteredSearchWidget::FilteredSearchWidget(QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::FilteredSearchWidget)
        , m_flowLayout(new FlowLayout())
        , m_filterMenu(new QMenu(this))
        , m_selector(new SearchTypeSelector(this))
    {
        m_ui->setupUi(this);
        m_ui->filteredLayout->setLayout(m_flowLayout);
        m_ui->filteredParent->hide();
        m_ui->assetTypeSelector->setMenu(m_selector);
        UpdateClearIcon();
        SetTypeFilterVisible(false);

        connect(m_ui->clearLabel, &AzQtComponents::ExtendedLabel::clicked, this, &FilteredSearchWidget::ClearTypeFilter);
        connect(m_ui->textSearch, &QLineEdit::textChanged, this, &FilteredSearchWidget::UpdateClearIcon);
        connect(m_ui->textSearch, &QLineEdit::textChanged, this, &FilteredSearchWidget::TextFilterChanged);
        connect(m_ui->buttonClearFilter, &QPushButton::clicked, this, [this](){m_ui->textSearch->setText(QString());});
        connect(m_selector, &SearchTypeSelector::aboutToShow, this, [this](){m_selector->Setup(m_typeFilters);});
        connect(m_selector, &SearchTypeSelector::TypeToggled, this, &FilteredSearchWidget::SetFilterState);
    }

    FilteredSearchWidget::~FilteredSearchWidget()
    {
        delete m_ui;
    }

    void FilteredSearchWidget::SetTypeFilterVisible(bool visible)
    {
        m_ui->assetTypeSelector->setVisible(visible);
    }

    void FilteredSearchWidget::SetTypeFilters(const SearchTypeFilterList& typeFilters)
    {
        ClearTypeFilter();

        for (const SearchTypeFilter& typeFilter : typeFilters)
        {
            AddTypeFilter(typeFilter);
        }
    }

    void FilteredSearchWidget::AddTypeFilter(const SearchTypeFilter &typeFilter)
    {
        if (!typeFilter.displayName.isEmpty())
        {
            m_typeFilters.append(typeFilter);
            SetFilterState(m_typeFilters.length() - 1, typeFilter.enabled);
        }

        SetTypeFilterVisible(true);
    }

    void FilteredSearchWidget::SetTextFilterVisible(bool visible)
    {
        m_ui->textSearch->setVisible(visible);
    }

    void FilteredSearchWidget::ClearTextFilter()
    {
        m_ui->textSearch->clear();
    }

    void FilteredSearchWidget::ClearTypeFilter()
    {
        for (auto it = m_typeButtons.begin(), end = m_typeButtons.end(); it != end; ++it)
        {
            delete it.value();
        }
        m_typeButtons.clear();

        for (SearchTypeFilter& filter : m_typeFilters)
        {
            filter.enabled = false;
        }

        m_ui->filteredParent->setVisible(false);
        
        SearchTypeFilterList checkedTypes;
        emit TypeFilterChanged(checkedTypes);
    }

    void FilteredSearchWidget::SetFilterState(int index, bool enabled)
    {
        SearchTypeFilter& filter = m_typeFilters[index];
        
        filter.enabled = enabled;
        auto buttonIt = m_typeButtons.find(index);
        if (enabled && buttonIt == m_typeButtons.end())
        {
            FilterCriteriaButton* button = new FilterCriteriaButton(filter.displayName, this);
            connect(button, &FilterCriteriaButton::RequestClose, this, [this, index]() {SetFilterState(index, false);});
            m_flowLayout->addWidget(button);
            m_typeButtons[index] = button;
        }
        else if (!enabled)
        {
            if (buttonIt != m_typeButtons.end())
            {
                delete buttonIt.value();
                m_typeButtons.remove(index);
            }
        }

        SearchTypeFilterList checkedTypes;
        for (const SearchTypeFilter& typeFilter : m_typeFilters)
        {
            if (typeFilter.enabled)
            {
                checkedTypes.append(typeFilter);
            }
        }

        m_ui->filteredParent->setVisible(!checkedTypes.isEmpty());
        emit TypeFilterChanged(checkedTypes);
    }

    void FilteredSearchWidget::UpdateClearIcon()
    {
        m_ui->buttonClearFilter->setHidden(m_ui->textSearch->text().isEmpty());
    }
}

#include <Components/FilteredSearchWidget.moc>
