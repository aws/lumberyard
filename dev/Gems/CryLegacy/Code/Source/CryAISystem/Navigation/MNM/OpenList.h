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

// Description : This is a helper class to implement a open list


#ifndef OPENLIST_H
#define OPENLIST_H

#pragma once

template<typename ElementNode, class BestNodePredicate>
class OpenList
{
public:
    typedef std::vector<ElementNode> ElementList;

    OpenList(const size_t maxExpectedSize)
    {
        SetupOpenList(maxExpectedSize);
    }

    ILINE void SetupOpenList(const size_t maxExpectedSize)
    {
        openElements.reserve(maxExpectedSize);
    }

    ElementNode PopBestElement()
    {
        //FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

        CRY_ASSERT_MESSAGE(!openElements.empty(), "PopBestElement has been requested for an empty ElementNode open list.");
        BestNodePredicate predicate;
        typename ElementList::iterator bestElementIt = std::min_element(openElements.begin(), openElements.end(), predicate);
        ElementNode bestElement = *bestElementIt;
        *bestElementIt = openElements.back();
        openElements.pop_back();

        return bestElement;
    }

    ILINE void Reset()
    {
        openElements.clear();
    }

    ILINE bool IsEmpty() const
    {
        return openElements.empty();
    }

    ILINE void InsertElement(const ElementNode& newElement)
    {
        //FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

        assert(!stl::find(openElements, newElement));
        stl::push_back_unique(openElements, newElement);
    }

private:
    ElementList openElements;
};

#endif // OPENLIST_H