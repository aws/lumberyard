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

#pragma once

#include <smartptr.h>

namespace Serialization {
    class IArchive;
}

struct SAnimationFilterItem
{
    string path;
    std::vector<string> tags;
    string skeletonAlias;
    int selectedRule;

    SAnimationFilterItem()
        : selectedRule(-1)
    {
    }

    bool operator==(const SAnimationFilterItem& rhs) const
    {
        if (path != rhs.path)
        {
            return false;
        }
        if (tags.size() != rhs.tags.size())
        {
            return false;
        }
        for (size_t i = 0; i < tags.size(); ++i)
        {
            if (tags[i] != rhs.tags[i])
            {
                return false;
            }
        }
        return true;
    }
};

typedef std::vector<SAnimationFilterItem> SAnimationFilterItems;

struct IAnimationFilterCondition
    : public _reference_target_t
{
    bool m_not;
    virtual bool Check(const SAnimationFilterItem& item) const = 0;
    virtual void FindTags(std::vector<string>* tags) const {}
    virtual void Serialize(Serialization::IArchive& ar);
    IAnimationFilterCondition()
        : m_not (false) {}
};

struct SAnimationFilter
{
    _smart_ptr<IAnimationFilterCondition> condition;

    bool MatchesFilter(const SAnimationFilterItem& item) const;
    void Filter(std::vector<SAnimationFilterItem>* matchingItems, const std::vector<SAnimationFilterItem>& items) const;
    void Serialize(Serialization::IArchive& ar);

    void FindTags(std::vector<string>* tags) const;
    void SetInFolderCondition(const char* folder);
};
