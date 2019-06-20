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
#include <AzQtComponents/Components/FilteredSearchWidget.h>

#include <QSharedPointer>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class SearchWidget
            : public AzQtComponents::FilteredSearchWidget
        {
            Q_OBJECT

        public:
            explicit SearchWidget(QWidget* parent = nullptr);

            void Setup(bool stringFilter, bool assetTypeFilter);

            QSharedPointer<CompositeFilter> GetFilter() const;

            QString GetFilterString() const { return textFilter(); }
            void ClearStringFilter() { ClearTextFilter(); }

        private:
            QSharedPointer<CompositeFilter> m_filter;
            QSharedPointer<CompositeFilter> m_stringFilter;
            QSharedPointer<CompositeFilter> m_typesFilter;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
