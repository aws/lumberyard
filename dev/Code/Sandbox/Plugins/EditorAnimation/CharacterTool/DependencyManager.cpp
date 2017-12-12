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

#include "pch.h"
#include "DependencyManager.h"
#include <StlUtils.h>

namespace CharacterTool
{
    void DependencyManager::SetDependencies(const char* user, vector<string>& newUsedAssets)
    {
        vector<string>& usedAssets = m_usedAssets[user];

        for (size_t i = 0; i < usedAssets.size(); ++i)
        {
            vector<string>& users = m_assetUsers[usedAssets[i]];
            stl::find_and_erase_all(users, string(user));
            if (users.empty())
            {
                m_assetUsers.erase(usedAssets[i]);
            }
        }

        usedAssets = newUsedAssets;

        for (size_t i = 0; i < usedAssets.size(); ++i)
        {
            vector<string>& users = m_assetUsers[usedAssets[i]];
            stl::push_back_unique(users, string(user));
        }
    }

    void DependencyManager::FindUsers(vector<string>* outUsers, const char* asset) const
    {
        AssetUsers::const_iterator it = m_assetUsers.find(asset);
        if (it == m_assetUsers.end())
        {
            return;
        }
        const vector<string>& users = it->second;
        for (size_t i = 0; i < users.size(); ++i)
        {
            if (stl::push_back_unique(*outUsers, users[i]))
            {
                FindUsers(outUsers, users[i]);
            }
        }
    }

    void DependencyManager::FindDepending(vector<string>* outAssets, const char* user) const
    {
        AssetUsers::const_iterator it = m_usedAssets.find(user);
        if (it == m_usedAssets.end())
        {
            return;
        }
        const vector<string>& assets = it->second;
        for (size_t i = 0; i < assets.size(); ++i)
        {
            if (stl::push_back_unique(*outAssets, assets[i]))
            {
                FindUsers(&*outAssets, assets[i]);
            }
        }
    }
}
