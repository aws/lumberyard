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

#include <AzQtComponents/Components/FilteredSearchWidget.h>

#include <QStyledItemDelegate>
#include <QStandardItem>

Q_DECLARE_METATYPE(AZ::Uuid);

namespace Ui
{
    class OutlinerSearchWidget;
}

namespace AzQtComponents
{
    class OutlinerSearchTypeSelector
        : public SearchTypeSelector
    {
    public:
        OutlinerSearchTypeSelector(QWidget* parent = nullptr);

    protected:
        void RepopulateDataModel() override;
    };

    class OutlinerCriteriaButton
        : public FilterCriteriaButton
    {
        Q_OBJECT

    public:
        explicit OutlinerCriteriaButton(QString labelText, QWidget* parent = nullptr, int index = -1);
   };

    class OutlinerSearchWidget
        : public FilteredSearchWidget
    {
        Q_OBJECT
    public:
        explicit OutlinerSearchWidget(QWidget* parent = nullptr);

        void SetFilterStateByIndex(int index, bool enabled) override;
        void ClearTypeFilter() override;

        enum class GlobalSearchCriteria : int
        {
            Unlocked,
            Locked,
            Visible,
            Hidden,
            Separator,
            FirstRealFilter
        };
    };

    class OutlinerIcons
    {
    public:
        static OutlinerIcons& GetInstance()
        {
            static OutlinerIcons instance;
            return instance;
        }
        OutlinerIcons(OutlinerIcons const &) = delete;
        void operator=(OutlinerIcons const &) = delete;

        QIcon& GetIcon(int iconWanted) { return m_globalIcons[iconWanted]; }
    private:
        OutlinerIcons();

        QIcon m_globalIcons[static_cast<int>(AzQtComponents::OutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter)];
    };

    class OutlinerSearchItemDelegate : public QStyledItemDelegate
    {
    public:
        OutlinerSearchItemDelegate(QWidget* parent = nullptr);

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    };
}