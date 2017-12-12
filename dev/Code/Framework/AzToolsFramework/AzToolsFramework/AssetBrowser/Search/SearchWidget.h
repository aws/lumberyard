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

#include <AzToolsFramework/AssetBrowser/Search/Filter.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

#include <QWidget>
#include <QScopedPointer>
#include <QSharedPointer>

namespace Ui
{
    class SearchWidgetClass;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class SearchWidget
            : public QWidget
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(SearchWidget, AZ::SystemAllocator, 0);

            explicit SearchWidget(QWidget* parent = nullptr);
            ~SearchWidget() override;

            void Setup(bool stringFilter, bool assetTypeFilter);

            QSharedPointer<CompositeFilter> GetFilter() const;

            void ClearAssetTypeFilter() const;
            void ClearStringFilter() const;
            QString GetFilterString() const;

        private:
            QScopedPointer<Ui::SearchWidgetClass> m_ui;            
            QSharedPointer<CompositeFilter> m_filter;
            QSharedPointer<CompositeFilter> m_stringFilter;

        private Q_SLOTS:
            void TextChangedSlot(const QString& text) const;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
