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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_CONTROLS_REPORTCONTROLFILTERHELPER_H
#define CRYINCLUDE_EDITOR_CONTROLS_REPORTCONTROLFILTERHELPER_H
#pragma once


namespace ReportControlFilterHelper
{
    struct STextFilter
    {
        std::vector< string > includeFilters;
        std::vector< string > excludeFilters;

        STextFilter(const string& filterText)
        {
            string lowercaseFilterText = filterText;
            lowercaseFilterText.MakeLower();

            int pos = 0;
            while (pos != -1)
            {
                const string filter = lowercaseFilterText.Tokenize(" ", pos);
                if (filter.size() != 0)
                {
                    const bool isExcludeFilter = (filter[ 0 ] == '-');
                    if (isExcludeFilter)
                    {
                        const string excludeFilter = filter.c_str() + 1;
                        if (excludeFilter.size() != 0)
                        {
                            excludeFilters.push_back(excludeFilter);
                        }
                    }
                    else
                    {
                        includeFilters.push_back(filter);
                    }
                }
            }
        }

        template< class TRecord >
        bool operator()(const TRecord* pRecord)
        {
            for (size_t i = 0; i < includeFilters.size(); i++)
            {
                const string& includeFilter = includeFilters[ i ];
                const bool containsFilter = pRecord->MatchesFilter(includeFilter);
                if (!containsFilter)
                {
                    return false;
                }
            }

            for (size_t i = 0; i < excludeFilters.size(); i++)
            {
                const string& excludeFilter = excludeFilters[ i ];
                const bool containsFilter = pRecord->MatchesFilter(excludeFilter);
                if (containsFilter)
                {
                    return false;
                }
            }
            return true;
        }
    };

    template< typename TRecord, typename TFilter >
    void ApplyFilter(CXTPReportControl* pReportControl, TFilter& filter)
    {
        assert(pReportControl != NULL);

        pReportControl->BeginUpdate();
        {
            CXTPReportRecords* pGroupRecords = pReportControl->GetRecords();
            ApplyFilterListsRec< TRecord >(pGroupRecords, filter);
        }
        pReportControl->EndUpdate();
        pReportControl->Populate();
    }

    template< typename TRecord, typename TFilter >
    bool ApplyFilterListsRec(CXTPReportRecords* pRecordList, TFilter& filter)
    {
        if (pRecordList == NULL)
        {
            return false;
        }

        bool hasVisibleChildren = false;
        for (int i = 0; i < pRecordList->GetCount(); i++)
        {
            TRecord* pRecord = static_cast< TRecord* >(pRecordList->GetAt(i));

            CXTPReportRecords* pChilds = pRecord->GetChilds();
            const bool hasChildren = (pChilds != NULL && 0 < pChilds->GetCount());
            if (hasChildren)
            {
                const bool groupHasVisibleChildren = ApplyFilterListsRec< TRecord >(pChilds, filter);
                pRecord->SetVisible(groupHasVisibleChildren);
                hasVisibleChildren |= groupHasVisibleChildren;
            }
            else
            {
                const bool matchesFilter = filter(pRecord);
                pRecord->SetVisible(matchesFilter);
                hasVisibleChildren |= matchesFilter;
            }
        }

        return hasVisibleChildren;
    }
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_REPORTCONTROLFILTERHELPER_H
