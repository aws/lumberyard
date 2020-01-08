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

#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERMANAGER_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERMANAGER_H
#pragma once
#include <vector>
#include <set>
#include "IAssetItem.h"
#include "IAssetItemDatabase.h"

// Asset items vector
typedef std::vector<IAssetItem*> TAssetItems;
// Asset database vector
typedef std::vector<IAssetItemDatabase*> TAssetDatabases;

class CAssetBrowserManager
    : public CryThread<CAssetBrowserManager>
{
public:
    typedef QStringList StrVector;
    typedef bool (* TPfnOnUpdateCacheProgress)(int totalProgressPercent, const char* pMessage);

    static CAssetBrowserManager* Instance();
    static void DeleteInstance();
    static bool IsInstanceExist();

    CAssetBrowserManager();
    ~CAssetBrowserManager();

    //////////////////////////////////////////////////////////////////////////
    // Initialize/shutdown and caching
    //////////////////////////////////////////////////////////////////////////
    void Initialize();
    void Shutdown();
    bool LoadCache();
    void CreateThumbsFolderPath();
    bool CacheAssets(TAssetDatabases dbsToCache, bool bForceCache = false, TPfnOnUpdateCacheProgress pProgressCallback = NULL);
    void MarkUsedInLevelAssets();

    //////////////////////////////////////////////////////////////////////////
    // Misc.
    //////////////////////////////////////////////////////////////////////////
    TAssetDatabases& GetAssetDatabases()
    {
        return m_assetDatabases;
    }
    QString GetProjectName();

    static unsigned int HashStringSbdm(const char* pStr);

    IAssetItemDatabase* GetDatabaseByName(const char* pName);

    void EnqueueAssetForThumbLoad(IAssetItem* pAsset);

    void Run();

    void RefreshAssetDatabases();

private:
    void CreateAssetDatabases();
    void FreeAssetDatabases();
    XmlNodeRef ReadAndVerifyMainDB(const QString& fileName);
    void ReadTransactionsAndUpdateDB(const QString& fullPath, XmlNodeRef& root);
    void ClearTransactionsAndApplyMetaData(const QString& fullPath, XmlNodeRef& root);
    // Callback for appending newly cached meta data to the transaction DB
    static bool OnNewTransaction(const IAssetItem* pAssetItem);

    std::set<IAssetItem*> m_thumbLoadQueue;
    TAssetDatabases m_assetDatabases;
    uint32 m_cachedAssetCount;
};

#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERMANAGER_H
