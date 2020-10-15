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

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    struct Uuid;

    namespace SerializeContextTools
    {
        class GemInfo final
        {
        public:
            bool LoadGemInformation(const char* appRoot, const char* gameRoot);
            
            AZStd::string_view LocateGemPath(const Uuid& moduleClassId) const;
            const AZStd::unordered_set<AZStd::string>& GetGemFolders() const;

        private:
            AZStd::unordered_map<AZStd::string, AZStd::string> m_moduleToGemFolderLookup;
            AZStd::unordered_set<AZStd::string> m_gemFolders;
        };
    } // namespace SerializeContextTools
} // namespace AZ