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

#include <AzFramework/StringFunc/StringFunc.h>

#include <QMenu>
#include <QLabel>
#include <QVBoxLayout>
#include <QTreeView>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QScopedValueRollback>
#include <QScreen>
#include <QDesktopwidget>
#include <QTimer>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyle>

Q_DECLARE_METATYPE(AZ::Uuid);

namespace AzQtComponents
{
    const char* FilteredSearchWidget::s_filterDataProperty = "filterData";
    const char* const s_filterSearchWidgetName = "filteredSearchWidget";
    const char* const s_searchLayout = "searchLayout";

    FilterCriteriaButton::FilterCriteriaButton(QString labelText, QWidget* parent)
        : QFrame(parent)
    {
        setStyleSheet(styleSheet() + "padding: 0px; border-radius: 2px; border: 0px; background-color: #2e2e2e");
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumSize(60, 16);

        m_frameLayout = new QHBoxLayout(this);
        m_frameLayout->setMargin(0);
        m_frameLayout->setSpacing(4);
        m_frameLayout->setContentsMargins(4, 1, 4, 1);

        m_tagLabel = new QLabel(this);
        m_tagLabel->setStyleSheet(m_tagLabel->styleSheet() + "border: 0px; background-color: transparent;");
        m_tagLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_tagLabel->setMinimumSize(24, 16);
        m_tagLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        m_tagLabel->setText(labelText);

        QIcon closeIcon(":/stylesheet/img/titlebar/titlebar_close.png");

        QPushButton* button = new QPushButton(this);
        button->setStyleSheet(button->styleSheet() + "border: 0px;");
        button->setFlat(true);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setFixedSize(QSize(10, 10));
        button->setProperty("iconButton", "true");
        button->setMouseTracking(true);
        button->setIcon(closeIcon);

        m_frameLayout->addWidget(m_tagLabel);
        m_frameLayout->addWidget(button);

        connect(button, &QPushButton::clicked, this, &FilterCriteriaButton::RequestClose);
    }

    SearchTypeSelector::SearchTypeSelector(QWidget* parent /* = nullptr */)
        : QMenu(parent)
        , m_unfilteredData(nullptr)
    {
        QVBoxLayout* verticalLayout = new QVBoxLayout(this);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QStringLiteral("vertLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);

        //make text search box
        QVBoxLayout* searchLayout = new QVBoxLayout();
        searchLayout->setContentsMargins(4, 4, 4, 4);
        searchLayout->setObjectName(s_searchLayout);

        QLineEdit* textSearch = new QLineEdit(this);
        textSearch->setObjectName(QStringLiteral("textSearch"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(textSearch->sizePolicy().hasHeightForWidth());
        textSearch->setSizePolicy(sizePolicy);
        textSearch->setMinimumSize(QSize(0, 25));
        textSearch->setObjectName(s_filterSearchWidgetName);
        textSearch->setFrame(false);
        textSearch->setText(QString());
        textSearch->setPlaceholderText(QObject::tr("Search..."));
        connect(textSearch, &QLineEdit::textChanged, this, &SearchTypeSelector::FilterTextChanged);

        searchLayout->addWidget(textSearch);

        //add in item tree
        QVBoxLayout* itemLayout = new QVBoxLayout();
        itemLayout->setContentsMargins(0, 0, 0, 0);

        m_tree = new QTreeView(this);
        itemLayout->addWidget(m_tree);

        m_model = new QStandardItemModel(this);
        m_tree->setModel(m_model);
        m_tree->setHeaderHidden(true);
        m_tree->setStyleSheet("QTreeView::item:hover{color: rgb(153, 153, 153)}");
        m_tree->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding));
        m_tree->setMinimumSize(QSize(0, 1));

        connect(m_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item)
        {
            // Don't emit itemChanged while Setup is running
            if (!m_settingUp)
            {
                int index = item->data().toInt();
                if (index < m_filteredItemIndices.size())
                {
                    index = m_filteredItemIndices[item->data().toInt()];
                }

                bool enabled = item->checkState() == Qt::Checked;
                emit TypeToggled(index, enabled);
            }
        });
        verticalLayout->addLayout(searchLayout);
        verticalLayout->addLayout(itemLayout);
    }


    QSize SearchTypeSelector::sizeHint() const
    {
        return m_tree->sizeHint();
    }

    void SearchTypeSelector::RepopulateDataModel()
    {
        m_filteredItemIndices.clear();
        m_model->clear();
        m_numRows = 0;

        bool amFiltering = m_filterString.length() > 0;

        QScopedValueRollback<bool> setupGuard(m_settingUp, true);
        QMap<QString, QStandardItem*> categories;

        if (!m_unfilteredData)
        {
            return;
        }

        for (int unfilteredDataIndex = 0, length = m_unfilteredData->length(); unfilteredDataIndex < length; ++unfilteredDataIndex)
        {
            const SearchTypeFilter& filter = m_unfilteredData->at(unfilteredDataIndex);
            bool addItem = true;

            bool itemMatchesFilter = true;
            bool categoryMatchesFilter = true;

            if (amFiltering)
            {
                itemMatchesFilter = filter.displayName.contains(m_filterString, Qt::CaseSensitivity::CaseInsensitive);
                categoryMatchesFilter = filter.category.contains(m_filterString, Qt::CaseSensitivity::CaseInsensitive);
            }

            if (!itemMatchesFilter && !categoryMatchesFilter)
            {
                addItem = false;
            }

            QStandardItem* categoryItem = nullptr;
            if (categories.contains(filter.category))
            {
                categoryItem = categories[filter.category];
            }
            else
            {
                categoryItem = new QStandardItem(filter.category);
                if (categoryMatchesFilter || addItem)
                {
                    categories[filter.category] = categoryItem;
                    m_model->appendRow(categoryItem);
                    categoryItem->setEditable(false);
                    ++m_numRows;
                }
            }

            if (!addItem)
            {
                continue;
            }

            QStandardItem* item = new QStandardItem(filter.displayName);
            item->setCheckable(true);
            item->setCheckState(filter.enabled ? Qt::Checked : Qt::Unchecked);
            item->setData(unfilteredDataIndex);
            m_filteredItemIndices.append(unfilteredDataIndex);
            categoryItem->appendRow(item);
            item->setEditable(false);
            ++m_numRows;
        }

        m_tree->expandAll();

        // Calculate the search layout size (above the tree view)
        int searchLayoutHeight = 0;
        QVBoxLayout* searchLayout = findChild<QVBoxLayout*>(s_searchLayout);
        if (searchLayout)
        {
            const QLineEdit* textSearch = findChild<QLineEdit*>(s_filterSearchWidgetName);
            searchLayoutHeight = textSearch ? textSearch->height() : 0;

            const QMargins margins = searchLayout->contentsMargins();
            searchLayoutHeight += margins.top() + margins.bottom();
        }

        // Calculate the maximum size of the tree
        QFontMetrics fontMetric{m_tree->font()};
        int height = (fontMetric.height() * m_numRows) + searchLayoutHeight;
        // Decide whether the menu should go up or down from current point.
        QPoint globalPos;
        if (parentWidget())
        {
            globalPos = parentWidget()->mapToGlobal(parentWidget()->rect().bottomRight());
        }
        else
        {
            globalPos = mapToGlobal(QCursor::pos());
        }
        QDesktopWidget* desktop = QApplication::desktop();
        int mouseScreen = desktop->screenNumber(globalPos);
        QRect mouseScreenGeometry = desktop->screen(mouseScreen)->geometry();
        QPoint localPos = globalPos - mouseScreenGeometry.topLeft();
        int screenHeight;
        int yPos;
        if (localPos.y() > mouseScreenGeometry.height() / 2)
        {
            screenHeight = localPos.y();
            yPos = mouseScreenGeometry.y();
        }
        else
        {
            screenHeight = mouseScreenGeometry.height() - localPos.y();
            yPos = globalPos.y();
        }
        if (height > screenHeight)
        {
            height = screenHeight;
        }

        // Set the size of the main widget
        const QSize menuSize{ 256, height };
        setMaximumSize(menuSize);
        setMinimumSize(menuSize);

        QPoint menuPosition{ globalPos.x(), yPos };

        // Wait until end of frame to move the menu as QMenu will make its own move after the function
        QTimer::singleShot(0, [=] { this->move(menuPosition); });
    }

    void SearchTypeSelector::Setup(const SearchTypeFilterList& searchTypes)
    {
        m_unfilteredData = &searchTypes;

        RepopulateDataModel();
    }

    void SearchTypeSelector::FilterTextChanged(const QString& newFilter)
    {
        m_filterString = newFilter;

        RepopulateDataModel();
    }

    FilteredSearchWidget::FilteredSearchWidget(QWidget* parent, bool willUseOwnSelector)
        : QWidget(parent)
        , m_ui(new Ui::FilteredSearchWidget)
        , m_flowLayout(new FlowLayout())
        , m_filterMenu(new QMenu(this))
        , m_selector(willUseOwnSelector ? nullptr : new SearchTypeSelector(this))
    {
        m_ui->setupUi(this);
        m_ui->filteredLayout->setLayout(m_flowLayout);
        m_ui->filteredParent->hide();
        m_ui->filteredLayout->setContextMenuPolicy(Qt::CustomContextMenu);
        if (!willUseOwnSelector)
        {
            m_ui->assetTypeSelector->setMenu(m_selector);
        }
        UpdateClearIcon();
        SetTypeFilterVisible(false);

        connect(m_ui->filteredLayout, &QWidget::customContextMenuRequested, this, &FilteredSearchWidget::OnClearFilterContextMenu);
        connect(m_ui->textSearch, &QLineEdit::textChanged, this, &FilteredSearchWidget::UpdateClearIcon);
        connect(m_ui->textSearch, &QLineEdit::textChanged, this, &FilteredSearchWidget::TextFilterChanged);
        connect(m_ui->textSearch, &QLineEdit::returnPressed, this, &FilteredSearchWidget::UpdateTextFilter);

        connect(m_ui->buttonClearFilter, &QPushButton::clicked, this, [this]() {m_ui->textSearch->setText(QString()); });
        if (!willUseOwnSelector)
        {
            connect(m_selector, &SearchTypeSelector::aboutToShow, this, [this]() {m_selector->Setup(m_typeFilters); });
            connect(m_selector, &SearchTypeSelector::TypeToggled, this, &FilteredSearchWidget::SetFilterStateByIndex);
        }

        connect(&m_inputTimer, &QTimer::timeout, this, &FilteredSearchWidget::UpdateTextFilter);

        m_inputTimer.setInterval(0);
        m_inputTimer.setSingleShot(true);
    }

    FilteredSearchWidget::~FilteredSearchWidget()
    {
        delete m_ui;
    }

    void FilteredSearchWidget::SetupOwnSelector(SearchTypeSelector* selector)
    {
        m_selector = selector;
        m_ui->assetTypeSelector->setMenu(m_selector);
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
            SetFilterStateByIndex(m_typeFilters.length() - 1, typeFilter.enabled);
        }

        SetTypeFilterVisible(true);
    }

    void FilteredSearchWidget::SetTextFilterVisible(bool visible)
    {
        m_ui->textSearch->setVisible(visible);
    }

    void FilteredSearchWidget::SetTextFilter(const QString& textFilter)
    {
        {
            QSignalBlocker blocker(m_ui->textSearch);
            m_ui->textSearch->setText(textFilter);
        }
     
        UpdateClearIcon();
        UpdateTextFilter();
    }

    void FilteredSearchWidget::ClearTextFilter()
    {
        m_ui->textSearch->clear();
    }

    void FilteredSearchWidget::SetFilterInputInterval(AZStd::chrono::milliseconds milliseconds)
    {
        m_inputTimer.setInterval(milliseconds.count());
    }

    void FilteredSearchWidget::SetFilterState(const QString& category, const QString& displayName, bool enabled)
    {
        const int typeFilterIndex = FindFilterIndex(category, displayName);
        if (typeFilterIndex != -1)
        {
            SetFilterStateByIndex(typeFilterIndex, enabled);
        }
    }

    int FilteredSearchWidget::FindFilterIndex(const QString& category, const QString& displayName) const
    {
        const int typeFilterCount = m_typeFilters.size();
        for (int i = 0; i < typeFilterCount; ++i)
        {
            const SearchTypeFilter& typeFilter = m_typeFilters[i];
            if (typeFilter.category == category && typeFilter.displayName == displayName)
            {
                return i;
            }
        }

        return -1;
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

    void FilteredSearchWidget::SetFilterStateByIndex(int index, bool enabled)
    {
        SearchTypeFilter& filter = m_typeFilters[index];

        filter.enabled = enabled;
        auto buttonIt = m_typeButtons.find(index);
        if (enabled && buttonIt == m_typeButtons.end())
        {
            FilterCriteriaButton* button = new FilterCriteriaButton(filter.displayName, this);
            connect(button, &FilterCriteriaButton::RequestClose, this, [this, index]() {SetFilterStateByIndex(index, false);});
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

    void FilteredSearchWidget::AddWidgetToSearchWidget(QWidget* w)
    {
        m_ui->horizontalLayout_2->addWidget(w);
    }

    void FilteredSearchWidget::OnClearFilterContextMenu(const QPoint& pos)
    {
        QMenu* contextMenu = new QMenu(this);

        QAction* action = nullptr;

        action = contextMenu->addAction(QObject::tr("Clear All"));
        QObject::connect(action, &QAction::triggered, [this] { ClearTypeFilter(); });

        contextMenu->exec(m_ui->filteredLayout->mapToGlobal(pos));
        delete contextMenu;
    }

    void FilteredSearchWidget::SetFilteredParentVisible(bool visible)
    {
        m_ui->filteredParent->setVisible(visible);
    }

    void FilteredSearchWidget::OnTextChanged(const QString& activeTextFilter)
    {
        if (m_inputTimer.interval() == 0 || activeTextFilter.isEmpty())
        {
            UpdateTextFilter();
        }
        else
        {
            m_inputTimer.stop();
            m_inputTimer.start();
        }
    }

    void FilteredSearchWidget::UpdateTextFilter()
    {
        m_inputTimer.stop();
        emit TextFilterChanged(m_ui->textSearch->text());
    }
} // namespace AzQtComponents

#include <Components/FilteredSearchWidget.moc>
