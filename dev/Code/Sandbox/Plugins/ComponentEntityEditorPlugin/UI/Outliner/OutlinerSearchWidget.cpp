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

    OutlinerSearchTypeSelector::OutlinerSearchTypeSelector(QPushButton* parent)
        : SearchTypeSelector(parent)
    {
    }

    bool OutlinerSearchTypeSelector::filterItemOut(int unfilteredDataIndex, bool itemMatchesFilter, bool categoryMatchesFilter)
    {
        bool unfilteredIndexInvalid = (unfilteredDataIndex >= static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter));
        return SearchTypeSelector::filterItemOut(unfilteredDataIndex, itemMatchesFilter, categoryMatchesFilter) && unfilteredIndexInvalid;
    }

    void OutlinerSearchTypeSelector::initItem(QStandardItem* item, const SearchTypeFilter& filter, int unfilteredDataIndex)
    {
        if (filter.displayName != "--------")
        {
            item->setCheckable(true);
            item->setCheckState(filter.enabled ? Qt::Checked : Qt::Unchecked);
        }

        if (unfilteredDataIndex < static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter))
        {
            item->setIcon(OutlinerIcons::GetInstance().GetIcon(unfilteredDataIndex));
        }
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
        SetupOwnSelector(new OutlinerSearchTypeSelector(assetTypeSelectorButton()));

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
    }

    FilterCriteriaButton* OutlinerSearchWidget::createCriteriaButton(const SearchTypeFilter& filter, int filterIndex)
    {
        return new OutlinerCriteriaButton(filter.displayName, this, filterIndex);
    }

    OutlinerSearchItemDelegate::OutlinerSearchItemDelegate(QWidget* parent) : QStyledItemDelegate(parent)
    {
    }

    void OutlinerSearchItemDelegate::PaintRichText(QPainter* painter, QStyleOptionViewItemV4& opt, QString& text) const
    {
        int textDocDrawYOffset = 3;
        QPoint paintertextDocRenderOffset = QPoint(1, 4);

        QTextDocument textDoc;
        textDoc.setDefaultFont(opt.font);
        opt.palette.color(QPalette::Text);
        textDoc.setDefaultStyleSheet("body {color: " + opt.palette.color(QPalette::Text).name() + "}");
        textDoc.setHtml("<body>" + text + "</body>");
        QRect textRect = opt.widget->style()->proxy()->subElementRect(QStyle::SE_ItemViewItemText, &opt);
        painter->translate(textRect.topLeft() - paintertextDocRenderOffset);
        textDoc.setTextWidth(textRect.width());
        textDoc.drawContents(painter, QRectF(0, textDocDrawYOffset, textRect.width(), textRect.height() + textDocDrawYOffset));
    }

    void OutlinerSearchItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const
    {
        bool isGlobalOption = false;
        painter->save();

        QStyleOptionViewItemV4 opt = option;
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
            isGlobalOption = true;
        }

        // Handle the seperator
        if (opt.text.contains("-------"))
        {
            painter->setPen(QColor(96, 96, 96));
            painter->drawLine(0, opt.rect.center().y() + 3, opt.rect.right(), opt.rect.center().y() + 4);
        }
        else
        {
            if (m_selector->GetFilterString().length() > 0 && !isGlobalOption && opt.features & QStyleOptionViewItemV4::ViewItemFeature::HasCheckIndicator)
            {
                // Create rich text menu text to show filterstring
                QString label{ opt.text };
                opt.text = "";

                style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

                int highlightTextIndex = 0;
                do
                {
                    highlightTextIndex = label.lastIndexOf(m_selector->GetFilterString(), highlightTextIndex - 1, Qt::CaseInsensitive);
                    if (highlightTextIndex >= 0)
                    {
                        static const QColor outlinerHighlightColor(GetIEditor()->GetColorByName("OutlinerSelectionColor"));
                        const QString BACKGROUND_COLOR{ outlinerHighlightColor.name() };
                        label.insert(highlightTextIndex + m_selector->GetFilterString().length(), "</span>");
                        label.insert(highlightTextIndex, "<span style=\"background-color: " + BACKGROUND_COLOR + "\">");
                    }
                } while (highlightTextIndex > 0);
                PaintRichText(painter, opt, label);  
            }
            else
            {          
                QString label = opt.text;
                opt.text = "";
                style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
                PaintRichText(painter, opt, label);
            }
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
