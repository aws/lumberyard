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
#include "stdafx.h"

#include "OutlinerSearchWidget.h"

#include <AzToolsFramework/UI/Qt/FlowLayout.h>




namespace AzQtComponents
{
    OutlinerIcons::OutlinerIcons()
    {
        m_globalIcons[static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::Unlocked)] = QIcon(QString(":Icons/unlocked.svg"));
        m_globalIcons[static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::Locked)] = QIcon(QString(":Icons/locked.svg"));
        m_globalIcons[static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::Visible)] = QIcon(QString(":Icons/visb.svg"));
        m_globalIcons[static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::Hidden)] = QIcon(QString(":Icons/visb_hidden.svg"));
        m_globalIcons[static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::Separator)] = QIcon();
    }

    OutlinerSearchTypeSelector::OutlinerSearchTypeSelector(QWidget* parent)
        : SearchTypeSelector(parent)
    {
    }

    void OutlinerSearchTypeSelector::RepopulateDataModel()
    {
        const int HEADER_HEIGHT = 34;

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

            if (!itemMatchesFilter && !categoryMatchesFilter && unfilteredDataIndex >= static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter))
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
            if (filter.displayName != "--------")
            {
                item->setCheckable(true);
                item->setCheckState(filter.enabled ? Qt::Checked : Qt::Unchecked);
            }
            item->setData(unfilteredDataIndex);
            if (unfilteredDataIndex < static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter))
            {
                item->setIcon(OutlinerIcons::GetInstance().GetIcon(unfilteredDataIndex));
            }
            m_filteredItemIndices.append(unfilteredDataIndex);
            categoryItem->appendRow(item);
            item->setEditable(false);
            ++m_numRows;
        }

        m_tree->expandAll();

        // Calculate the maximum size of the tree
        QFontMetrics fontMetric{ m_tree->font() };
        int height = HEADER_HEIGHT + fontMetric.height() * m_numRows;
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

        // Set the size of the menu
        const QSize menuSize{ 256, height };
        setMaximumSize(menuSize);
        setMinimumSize(menuSize);

        QPoint menuPosition{ globalPos.x(), yPos };
        // Wait until end of frame to move the menu as QMenu will make its own move after the
        // function
        QTimer::singleShot(0, [=] { this->move(menuPosition); });
    }

    OutlinerCriteriaButton::OutlinerCriteriaButton(QString labelText, QWidget* parent, int index)
        : FilterCriteriaButton(labelText, parent)
    {
        if (index >= 0 && index < static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter))
        {
            QLabel* icon = new QLabel(this);
            icon->setStyleSheet(m_tagLabel->styleSheet() + "border: 0px; background-color: transparent;");
            icon->setPixmap(OutlinerIcons::GetInstance().GetIcon(index).pixmap(10, 10));
            m_frameLayout->insertWidget(0, icon);
        }
    }

    OutlinerSearchWidget::OutlinerSearchWidget(QWidget* parent)
        : FilteredSearchWidget(parent, true)
    {
        SetupOwnSelector(new OutlinerSearchTypeSelector(this));
        connect(m_selector, &OutlinerSearchTypeSelector::aboutToShow, this, [this]() {m_selector->Setup(m_typeFilters); });
        connect(m_selector, &OutlinerSearchTypeSelector::TypeToggled, this, &OutlinerSearchWidget::SetFilterStateByIndex);

        const SearchTypeFilterList globalMenu{
            {"Global Settings", "Unlocked"},
            {"Global Settings", "Locked"},
            {"Global Settings", "Visible"},
            {"Global Settings", "Hidden"},
            {"Global Settings", "--------"}
        };
        int value = 0;
        for (const SearchTypeFilter filter : globalMenu)
        {
            AddTypeFilter(filter.category, filter.displayName, QVariant::fromValue<AZ::Uuid>(AZ::Uuid::Create()), value);
            ++value;
        }
        m_selector->GetTree()->setItemDelegate(new OutlinerSearchItemDelegate(m_selector->GetTree()));
    }

    void OutlinerSearchWidget::SetFilterStateByIndex(int index, bool enabled)
    {
        SearchTypeFilter& filter = m_typeFilters[index];

        filter.enabled = enabled;
        auto buttonIt = m_typeButtons.find(index);
        if (enabled && buttonIt == m_typeButtons.end())
        {
            OutlinerCriteriaButton* button = new OutlinerCriteriaButton(filter.displayName, this, index);
            connect(button, &FilterCriteriaButton::RequestClose, this,
                [this, index]() { SetFilterStateByIndex(index, false); });
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

        SetFilteredParentVisible(!checkedTypes.isEmpty());
        emit TypeFilterChanged(checkedTypes);
    }

    void OutlinerSearchWidget::ClearTypeFilter()
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

        SetFilteredParentVisible(false);

        SearchTypeFilterList checkedTypes;
        emit TypeFilterChanged(checkedTypes);
    }

    OutlinerSearchItemDelegate::OutlinerSearchItemDelegate(QWidget* parent) : QStyledItemDelegate(parent)
    {
    }

    void OutlinerSearchItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const
    {
        painter->save();

        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        const QWidget* widget = option.widget;
        QStyle* style = widget ? widget->style() : QApplication::style();
        if (!opt.icon.isNull())
        {
            QRect r = style->subElementRect(QStyle::SubElement::SE_ItemViewItemDecoration, &opt, widget);
            r.setX(-r.width());

            QIcon::Mode mode = QIcon::Normal;
            QIcon::State state = QIcon::On;
            opt.icon.paint(painter, r, opt.decorationAlignment, mode, state);

            opt.icon = QIcon();
            opt.decorationSize = QSize(0, 0);
        }

        // Handle the seperator
        if (opt.text.contains("-------"))
        {
            painter->setPen(QColor(96, 96, 96));
            painter->drawLine(0, opt.rect.center().y() + 3, opt.rect.right(), opt.rect.center().y() + 4);
        }
        else
        {
            style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
        }

        painter->restore();
    }

    QSize OutlinerSearchItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        if (opt.text.contains("-------"))
        {
            return QSize(0, 6);
        }
        return QStyledItemDelegate::sizeHint(option, index);
    }

}

#include <UI/Outliner/OutlinerSearchWidget.moc>
