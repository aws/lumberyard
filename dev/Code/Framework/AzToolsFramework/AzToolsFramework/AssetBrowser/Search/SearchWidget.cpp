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

#include <AzToolsFramework/AssetBrowser/Search/SearchWidget.h>
#include <AssetBrowser/Search/ui_SearchWidget.h>

#include <QString>
#include <QLineEdit>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        SearchWidget::SearchWidget(QWidget* parent)
            : QWidget(parent)
            , m_ui(new Ui::SearchWidgetClass())
            , m_filter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND))
            , m_stringFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND))
        {
            m_ui->setupUi(this);

            m_stringFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Up);
            m_stringFilter->SetTag("String");

            m_filter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);
        }

        SearchWidget::~SearchWidget() = default;

        void SearchWidget::Setup(bool stringFilter, bool assetTypeFilter)
        {
            m_ui->m_textSearch->setVisible(stringFilter);
            m_ui->m_assetTypeSelector->setVisible(assetTypeFilter);

            AZStd::vector<FilterConstType> filters;
            if (stringFilter)
            {
                filters.push_back(m_stringFilter);
                connect(m_ui->m_textSearch, &QLineEdit::textChanged, this, &SearchWidget::TextChangedSlot);
                connect(m_ui->m_buttonClearFilter, &QPushButton::clicked, this, &SearchWidget::ClearStringFilter);
            }

            // do not show assets in Hidden group
            auto hiddenGroupFilter = new AssetGroupFilter();
            hiddenGroupFilter->SetAssetGroup("Hidden");
            auto inverseFilter = new InverseFilter();
            inverseFilter->SetFilter(FilterConstType(hiddenGroupFilter));
            filters.push_back(FilterConstType(inverseFilter));
            // if source has exactly one product with the same name, hide it
            auto productsFilter = new ProductsFilter();
            filters.push_back(FilterConstType(productsFilter));

            if (assetTypeFilter)
            {
                filters.push_back(m_ui->m_assetTypeSelector->GetFilter());
            }

            for (auto& filter : filters)
            {
                m_filter->AddFilter(filter);
            }
        }

        QSharedPointer<CompositeFilter> SearchWidget::GetFilter() const
        {
            return m_filter;
        }

        void SearchWidget::ClearAssetTypeFilter() const
        {
            if (!m_ui->m_assetTypeSelector->IsLocked())
            {
                m_ui->m_assetTypeSelector->ClearAll();
            }
        }

        void SearchWidget::ClearStringFilter() const
        {
            m_ui->m_textSearch->setText("");
        }

        QString SearchWidget::GetFilterString() const
        {
            return m_ui->m_textSearch->text();
        }

        void SearchWidget::TextChangedSlot(const QString& text) const
        {
            m_stringFilter->RemoveAllFilters();

            auto stringList = text.split(' ', QString::SkipEmptyParts);
            for (auto& str : stringList)
            {
                auto stringFilter = new StringFilter();
                stringFilter->SetFilterString(str);
                m_stringFilter->AddFilter(FilterConstType(stringFilter));
            }

            m_ui->m_buttonClearFilter->setIcon(m_stringFilter->GetSubFilters().empty() ?
                QIcon() :
                QIcon(":/AssetBrowser/Resources/close.png"));
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
#include <AssetBrowser/Search/SearchWidget.moc>
#include <AssetBrowser/Search/rcc_search.h>