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

#include <vector>
#include <map>
#include "Strings.h"

namespace CharacterTool
{
    using std::vector;
    using std::map;

    class DependencyManager
    {
    public:
        void SetDependencies(const char* asset, vector<string>& usedAssets);

        void FindUsers(vector<string>* users, const char* asset) const;
        void FindDepending(vector<string>* assets, const char* user) const;
    private:
        typedef map<string, vector<string>, stl::less_stricmp<string> > UsedAssets;
        UsedAssets m_usedAssets;

        typedef map<string, vector<string>, stl::less_stricmp<string> > AssetUsers;
        AssetUsers m_assetUsers;
    };
}
