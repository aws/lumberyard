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
#include "stdafx.h"

#include "AssetTreeEntry.h"
#include "EditorCommon.h"

#include <AzCore/std/containers/map.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
AssetTreeEntry::~AssetTreeEntry()
{
    for (auto folderEntry : m_folders)
    {
        delete folderEntry.second;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AssetTreeEntry::Insert(const AZStd::string& path, const AZStd::string& menuName, const AZ::Data::AssetId& assetId)
{
    if (path.empty())
    {
        // there are no more folders in the pathname - add the leaf file entry
        m_files.insert(AZStd::make_pair(menuName, assetId));
    }
    else
    {
        AZStd::string folderName;
        AZStd::string remainderPath;
        size_t separator = path.find('/');
        if (separator == string::npos)
        {
            folderName = path;
        }
        else
        {
            folderName = path.substr(0, separator);
            if (path.length() > separator+1)
            {
                remainderPath = path.substr(separator+1);
            }
        }

        AssetTreeEntry* folderEntry = nullptr;
        // check if the folder already exists
        if (m_folders.count(folderName) == 0)
        {
            // does not exist, create it and insert it in tree
            folderEntry = new AssetTreeEntry;
            m_folders.insert(AZStd::make_pair(folderName, folderEntry));
        }
        else
        {
            // already exists
            folderEntry = m_folders[folderName];
        }

        // recurse down the pathname creating folders until we get to the leaf folder
        // to insert the file entry
        folderEntry->Insert(remainderPath, menuName, assetId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AssetTreeEntry* AssetTreeEntry::BuildAssetTree(const AZ::Data::AssetType& assetType, const AZStd::string& pathToSearch)
{
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

    AZ_Assert(fileIO, "Falied to get FileIOBase instance");

    using namespace AzToolsFramework::AssetBrowser;

    AssetBrowserModel* assetBrowserModel;
    EBUS_EVENT_RESULT(assetBrowserModel, AssetBrowserComponentRequestsBus, GetAssetBrowserModel);
    AZ_Assert(assetBrowserModel, "Failed to get asset browser model");

    const auto rootEntry = assetBrowserModel->GetRootEntry();
    AZStd::vector<const ProductAssetBrowserEntry*> products;
    rootEntry->GetChildrenRecursively<ProductAssetBrowserEntry>(products);

    AssetTreeEntry* assetTree = new AssetTreeEntry;
    for (auto product : products)
    {
        // We are only interested in assets of the given asset type
        if (product->GetAssetType() == assetType)
        {
            AssetBrowserEntry::AssetEntryType entryType = product->GetEntryType();
            AZ_Assert(entryType == AssetBrowserEntry::AssetEntryType::Product, "Error");

            AssetBrowserEntry* sourceEntry = product->GetParent();
            AZ_Assert(sourceEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source, "Error");

            AssetBrowserEntry* folderEntry = sourceEntry->GetParent();
            AZ_Assert(folderEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder, "Error");

            // build the relative path to this asset from the nearest scan folder
            // a scan folder for be the game project or a gem's asset root
            FolderAssetBrowserEntry* folder = static_cast<FolderAssetBrowserEntry*>(folderEntry);
            AZStd::string relPath;
            while (!folder->GetIsScanFolder())
            {
                relPath = AZStd::string(folder->GetName().c_str()) + AZStd::string("/") + relPath;

                folderEntry = folderEntry->GetParent();
                AZ_Assert(folderEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder, "Error");
                folder = static_cast<FolderAssetBrowserEntry*>(folderEntry);
            }
                
            // transform relPath to lowercase
            AZStd::string lowercaseRelPath = relPath;
#if defined(AZ_PLATFORM_WINDOWS)
            // Need to change the iterator for the second argument to avoid an
            // warning/error in Micorsoft Debug builds. The warning specifies
            // that transform does not check the iterators passed in to see if
            // they are valid (i.e, not the same iterator and they don't
            // overlap). By using make_unchecked_array_iterator we are tell MS
            // compiler that the iterator is good and satsify the warning/error 
            std::transform( lowercaseRelPath.begin(), lowercaseRelPath.end(), stdext::make_unchecked_array_iterator(lowercaseRelPath.begin()), ::tolower );
#else
            std::transform( lowercaseRelPath.begin(), lowercaseRelPath.end(), lowercaseRelPath.begin(), ::tolower );
#endif
    
            // if the start of the relPath matchs the path to search then this asset should be added to the tree
            if (lowercaseRelPath.substr(0, pathToSearch.length()) == pathToSearch)
            {
                // build the full path name from the asset root, including filename and extension
                // this can be used to open the asset
                AZStd::string fullRelativePath = relPath + sourceEntry->GetName().c_str();

                // build the path from the pathToSearch down to the file name, this can be used to build
                // a hierarchical menu
                AZStd::string menuPath = relPath.substr(pathToSearch.length());

                // Get the name without extension. This would be used for a menu entry for example
                AZStd::string name = sourceEntry->GetName().c_str();
                size_t dotPos = name.rfind('.');
                if (dotPos != AZStd::string::npos)
                {
                    name = name.substr(0, dotPos);
                }

                // add this entry into the asset tree
                assetTree->Insert(menuPath, name, product->GetAssetId());
            }
        }
    }

    return assetTree;
}
