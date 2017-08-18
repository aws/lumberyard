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

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        template<typename EntryType>
        void AssetBrowserEntry::GetChildren(AZStd::vector<const EntryType*>& entries) const
        {
            entries.reserve(entries.size() + m_children.size());
            for (auto child : m_children)
            {
                if (azrtti_istypeof<EntryType*>(child))
                {
                    entries.push_back(azrtti_cast<const EntryType*>(child));
                }
            }
        }

        template<typename EntryType>
        void AssetBrowserEntry::GetChildrenRecursively(AZStd::vector<const EntryType*>& entries) const
        {
            if (azrtti_istypeof<EntryType*>(this))
            {
                entries.push_back(azrtti_cast<const EntryType*>(this));
            }
            for (auto child : m_children)
            {
                child->GetChildrenRecursively<EntryType>(entries);
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework