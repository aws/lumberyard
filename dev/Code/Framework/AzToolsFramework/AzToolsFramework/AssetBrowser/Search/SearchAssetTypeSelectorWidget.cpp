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
#include <AssetBrowser/Search/ui_SearchAssetTypeSelectorWidget.h>

#include <AzToolsFramework/AssetBrowser/Search/SearchAssetTypeSelectorWidget.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/std/containers/vector.h>
#include <algorithm>

#include <QPushButton>
#include <QMenu>
#include <QCheckBox>
#include <QWidgetAction>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        namespace
        {
            template<class T>
            struct EBusAggregateUniqueResults
            {
                AZStd::vector<T> values;
                AZ_FORCE_INLINE void operator=(const T& rhs)
                {
                    if (AZStd::find(values.begin(), values.end(), rhs) == values.end())
                    {
                        values.push_back(rhs);
                    }
                }
            };
            
            struct EBusAggregateAssetTypesIfBelongsToGroup
            {
                EBusAggregateAssetTypesIfBelongsToGroup(const QString& group)
                    : m_group(group)
                {
                }

                EBusAggregateAssetTypesIfBelongsToGroup& operator=(const EBusAggregateAssetTypesIfBelongsToGroup&) = delete;
                
                AZStd::vector<AZ::Data::AssetType> values;

                AZ_FORCE_INLINE void operator=(const AZ::Data::AssetType& assetType)
                {
                    if (BelongsToGroup(assetType))
                    {
                        values.push_back(assetType);
                    }
                }

            private:
                const QString& m_group;

                bool BelongsToGroup(const AZ::Data::AssetType& assetType)
                {
                    QString group;
                    AZ::AssetTypeInfoBus::EventResult(group, assetType, &AZ::AssetTypeInfo::GetGroup);
                    return group.compare(m_group, Qt::CaseInsensitive);
                }
            };
        }

        SearchAssetTypeSelectorWidget::SearchAssetTypeSelectorWidget(QWidget* parent)
            : QWidget(parent)
            , m_ui(new Ui::SearchAssetTypeSelectorWidgetClass())
            , m_filter(QSharedPointer<CompositeFilter>(new CompositeFilter(CompositeFilter::LogicOperatorType::OR)))
            , m_locked(false)
        {
            m_ui->setupUi(this);

            QMenu* menu = new QMenu(this);
            AddAllAction(menu);
            menu->addSeparator();

            EBusAggregateUniqueResults<QString> results;
            AZ::AssetTypeInfoBus::BroadcastResult(results, &AZ::AssetTypeInfo::GetGroup);
            std::sort(results.values.begin(), results.values.end(),
                [](const QString& a, const QString& b) { return QString::compare(a, b, Qt::CaseInsensitive) < 0; });

            for (QString& group : results.values)
            {
                // Group "Other" should be in the end of the list, and "Hidden" should not be on the list at all
                if (group == "Other" || group == "Hidden")
                {
                    continue;
                }
                AddAssetTypeGroup(menu, group);
            }
            AddAssetTypeGroup(menu, "Other");
            menu->setLayoutDirection(Qt::LeftToRight);
            menu->setStyleSheet("border: none; background-color: #333333;");
            m_ui->m_showSelectionButton->setMenu(menu);

            m_filter->SetTag("AssetTypes");
        }

        SearchAssetTypeSelectorWidget::~SearchAssetTypeSelectorWidget()
        {
        }

        void SearchAssetTypeSelectorWidget::ClearAll() const
        {
            if (m_locked || m_allCheckbox->isChecked())
            {
                return;
            }
            m_allCheckbox->click();
        }

        FilterConstType SearchAssetTypeSelectorWidget::GetFilter() const
        {
            return m_filter;
        }

        bool SearchAssetTypeSelectorWidget::IsLocked() const
        {
            return m_locked;
        }

        void SearchAssetTypeSelectorWidget::AddAssetTypeGroup(QMenu* menu, const QString& group)
        {
            EBusAggregateAssetTypesIfBelongsToGroup results(group);
            AZ::AssetTypeInfoBus::BroadcastResult(results, &AZ::AssetTypeInfo::GetAssetType);

            if (!results.values.empty())
            {
                QCheckBox* checkbox = new QCheckBox(group, menu);
                checkbox->setChecked(true);
                QWidgetAction* action = new QWidgetAction(menu);
                action->setDefaultWidget(checkbox);
                menu->addAction(action);
                m_assetTypeCheckboxes.push_back(checkbox);

                AssetGroupFilter* groupFilter = new AssetGroupFilter();
                groupFilter->SetAssetGroup(group);

                m_actionFiltersMapping[checkbox] = FilterConstType(groupFilter);

                connect(checkbox, &QCheckBox::clicked,
                    [=](bool checked)
                    {
                        if (checked)
                        {
                            bool allChecked = true;
                            // if all filter types are checked, then check "all" button
                            for (auto assetTypeCheckbox : m_assetTypeCheckboxes)
                            {
                                if (!assetTypeCheckbox->isChecked())
                                {
                                    allChecked = false;
                                    break;
                                }
                            }
                            // easiest to filter for all asset types is to remove filters altogether
                            if (allChecked)
                            {
                                m_allCheckbox->click();
                            }
                            else
                            {
                                m_filter->AddFilter(m_actionFiltersMapping[checkbox]);
                            }
                        }
                        else
                        {
                            // if all is checked then asset filters need to be repopulated
                            if (m_allCheckbox->isChecked())
                            {
                                for (auto assetTypeCheckbox : m_assetTypeCheckboxes)
                                {
                                    if (assetTypeCheckbox != checkbox)
                                    {
                                        m_filter->AddFilter(m_actionFiltersMapping[assetTypeCheckbox]);
                                    }
                                }
                                m_filter->SetEmptyResult(false);
                                m_allCheckbox->setChecked(false);
                            }
                            else
                            {
                                m_filter->RemoveFilter(m_actionFiltersMapping[checkbox]);
                            }
                        }
                    });
            }
        }

        void SearchAssetTypeSelectorWidget::AddAllAction(QMenu* menu)
        {
            m_allCheckbox = new QCheckBox("All", menu);
            auto action = new QWidgetAction(menu);
            action->setDefaultWidget(m_allCheckbox);
            menu->addAction(action);
            m_allCheckbox->setChecked(true);
            connect(m_allCheckbox, &QCheckBox::clicked,
                [=](bool checked)
                {
                    if (checked)
                    {
                        // check all other asset types
                        for (auto assetTypeCheckbox : m_assetTypeCheckboxes)
                        {
                            if (!assetTypeCheckbox->isChecked())
                            {
                                assetTypeCheckbox->setChecked(true);
                            }
                        }
                        m_filter->RemoveAllFilters();
                        m_filter->SetEmptyResult(true);
                    }
                    else
                    {
                        // uncheck all asset types
                        for (auto assetTypeCheckbox : m_assetTypeCheckboxes)
                        {
                            if (assetTypeCheckbox->isChecked())
                            {
                                // click even is used to dispatch QCheckBox::clicked signal
                                assetTypeCheckbox->click();
                            }
                        }
                        m_filter->SetEmptyResult(false);
                    }
                });
        }
    }                                                              // namespace AssetBrowser
}                                                                  // namespace AzToolsFramework
#include <AssetBrowser/Search/SearchAssetTypeSelectorWidget.moc>