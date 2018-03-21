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

#include "stdafx.h"
#include "AssetBrowserManager.h"
#include "Util/FileUtil.h"
#include "Util/IndexedFiles.h"
#include "Include/IAssetItemDatabase.h"
#include "Include/IAssetItem.h"
#include "AssetBrowser/AssetBrowserDialog.h"
#include "IAssetTagging.h"
#include "IEditor.h"
#include "AssetBrowser/AssetBrowserMetaTaggingDlg.h"
#include "Objects/BrushObject.h"

static CAssetBrowserManager* s_pInstance = 0;

namespace AssetBrowser
{
    // details at http://www.cse.yorku.ca/~oz/hash.html
    unsigned int HashStringSbdm(const char* pStr)
    {
        unsigned long hash = 0;
        int c;

        while (c = *pStr++)
        {
            hash = c + (hash << 6) + (hash << 16) - hash;
        }

        return hash;
    }
}

CAssetBrowserManager* CAssetBrowserManager::Instance()
{
    if (!s_pInstance)
    {
        s_pInstance = new CAssetBrowserManager();
        s_pInstance->Initialize();
    }

    return s_pInstance;
}

void CAssetBrowserManager::DeleteInstance()
{
    if (s_pInstance)
    {
        s_pInstance->Shutdown();
    }

    delete s_pInstance;
    s_pInstance = 0;
}

bool CAssetBrowserManager::IsInstanceExist()
{
    return s_pInstance ? true : false;
}

CAssetBrowserManager::CAssetBrowserManager()
{
}

CAssetBrowserManager::~CAssetBrowserManager()
{
}

void CAssetBrowserManager::Initialize()
{
    CreateAssetDatabases();
    LoadCache();
    InitializeTagging();
}

void CAssetBrowserManager::Shutdown()
{
    FreeAssetDatabases();

    if (GetIEditor()->GetObjectManager())
    {
        GetIEditor()->GetObjectManager()->RemoveObjectEventListener(functor(*this, &CAssetBrowserManager::OnObjectEvent));
    }

    GetIEditor()->UnregisterNotifyListener(this);
}

bool CAssetBrowserManager::LoadCache()
{
    // Load the asset info file DB mainly for the asset browser.
    QString fileName = Path::GetUserSandboxFolder();

    fileName += "assetInfoDB.xml";

    XmlNodeRef root = ReadAndVerifyMainDB(fileName);

    if (root == NULL)
    {
        return false;
    }

    QString fullPath = Path::GetResolvedUserSandboxFolder();
    ReadTransactionsAndUpdateDB(fullPath, root);
    // Save the updated DB
    root->saveToFile(fileName.toUtf8().data());
    ClearTransactionsAndApplyMetaData(fullPath, root);

    return true;
}

void CAssetBrowserManager::CreateThumbsFolderPath()
{
    AZStd::string strUserFolder;
    AzFramework::StringFunc::Path::ConstructFull(Path::GetUserSandboxFolder().toUtf8().data(), AssetBrowser::kThumbnailsRoot, strUserFolder);

    // create the thumbs folder
    AZ::IO::FileIOBase::GetInstance()->CreatePath(strUserFolder.c_str());
}

bool CAssetBrowserManager::CacheAssets(TAssetDatabases dbsToCache, bool bForceCache, TPfnOnUpdateCacheProgress pProgressCallback)
{
    CreateThumbsFolderPath();

    IAssetItemDatabase* pAssetDB = NULL;
    uint32 totalAssetCount = 0;
    m_cachedAssetCount = 0;

    Log("Loading asset db cache...");
    LoadCache();

    Log("Prepare to cache...");
    // Get the total count of all assets.
    for (size_t i = 0; i < dbsToCache.size(); ++i)
    {
        if (dbsToCache[i]->QueryInterface(__uuidof(IAssetItemDatabase), (void**)&pAssetDB) == S_OK)
        {
            // if we force the cache, lets set flag to none, so if cache not finished, it can resume next time from where left
            if (bForceCache)
            {
                IAssetItemDatabase::TFilenameAssetMap& assets = pAssetDB->GetAssets();
                size_t j = 0;

                for (auto iter = assets.begin(), iterEnd = assets.end(); iter != iterEnd; ++iter, ++j)
                {
                    IAssetItem* pAsset = iter->second;
                    pAsset->SetFlag(IAssetItem::eFlag_Cached, false);
                    pAsset->SetFlag(IAssetItem::eFlag_ThumbnailLoaded, false);
                }
            }

            totalAssetCount += pAssetDB->GetAssets().size();
        }
    }

    if (totalAssetCount == 0)
    {
        return true;
    }

    QString progressStr;
    bool bStop = false;

    Log("Caching asset thumbnails and info...");
    // Cache the meta data from all assets with a progress display.
    for (size_t i = 0; i < dbsToCache.size(); ++i)
    {
        if (dbsToCache[i]->QueryInterface(__uuidof(IAssetItemDatabase), (void**)&pAssetDB) == S_OK)
        {
            IAssetItemDatabase::TFilenameAssetMap& assets = pAssetDB->GetAssets();
            size_t j = 0;

            Log("Caching %s...", pAssetDB->GetDatabaseName());

            for (auto iter = assets.begin(), iterEnd = assets.end(); iter != iterEnd; ++iter, ++j)
            {
                IAssetItem* pAsset = iter->second;
                QString filename = pAsset->GetFilename();

                if (!pAsset->IsFlagSet(IAssetItem::eFlag_Cached) || bForceCache)
                {
                    if (!bForceCache)
                    {
                        pAsset->Cache();
                    }
                    else
                    {
                        pAsset->ForceCache();
                    }
                }

                // Progress update, the current count of processed assets from the total count
                ++m_cachedAssetCount;

                if (pProgressCallback)
                {
                    static int s_lastTime = GetTickCount();

                    if (GetTickCount() - s_lastTime > 500)
                    {
                        s_lastTime = GetTickCount();
                        int progress = (int)(100.0f * (float)m_cachedAssetCount / totalAssetCount);

                        // invalid filename, will have problems with varargs in Log/printf
                        if (filename.contains("%"))
                        {
                            filename.replace("%", "%%");
                        }

                        progressStr = QStringLiteral(
                            "Caching asset: %1\r\nDatabase progress: %2 from %3 %4\r\nTotal progress: %5 from %6 total assets (%7%%)")
                            .arg(filename)
                            .arg(j)
                            .arg(assets.size())
                            .arg(pAssetDB->GetDatabaseName())
                            .arg(m_cachedAssetCount)
                            .arg(totalAssetCount)
                            .arg(progress);
                        bStop = pProgressCallback(progress, progressStr.toUtf8().data());
                    }
                }

                if (bStop)
                {
                    break;
                }
            }
        }

        if (bStop)
        {
            break;
        }
    }

    Log("End asset browser caching.");

    return true;
}

static CryMutex transactionUpdateMutex;

bool CAssetBrowserManager::OnNewTransaction(const IAssetItem* pAssetItem)
{
    assert(pAssetItem);

    if (pAssetItem == NULL)
    {
        return false;
    }

    IAssetItemDatabase* pDatabaseInterface = pAssetItem->GetOwnerDatabase();
    CryAutoLock<CryMutex> scopedLock(transactionUpdateMutex);

    QString fullPath = Path::GetResolvedUserSandboxFolder();
    fullPath += pDatabaseInterface->GetTransactionFilename();

    //1. Get  the transaction data.
    XmlNodeRef newTransactionNode = XmlHelpers::CreateXmlNode("Transaction");
    pAssetItem->ToXML(newTransactionNode);
    IFileUtil::FileDesc fd;
    bool bFileExists = true;

    if (!newTransactionNode->haveAttr("timeStamp"))
    {
        bFileExists = CFileUtil::FileExists(newTransactionNode->getAttr("fileName"), &fd);

        if (bFileExists)
        {
            // Add the time stamp info.
            QString buf;
            buf = QString::number(fd.time_write);
            newTransactionNode->setAttr("timeStamp", buf.toUtf8().data());
        }
    }

    //2. Append the new transaction to the file.
    FILE* transactionFile = fopen(fullPath.toUtf8().data(), "r+t");

    if (transactionFile == NULL)
    {
        return false;
    }

    const char endTag[] = "</Transactions>";
    int result = fseek(transactionFile, -((long)sizeof(endTag)) + 1, SEEK_END); // To delete the end tag of "</Transactions>"

    if (result != 0)    // It means this is the first transaction saved.
    {
        fseek(transactionFile, 0, SEEK_SET);
        fprintf(transactionFile, "<Transactions>\n");
    }

    fprintf(transactionFile, "%s%s", newTransactionNode->getXML().c_str(), endTag);
    fclose(transactionFile);

    return true;
}

static void CopyNode(XmlNodeRef dst, XmlNodeRef src)
{
    dst->copyAttributes(src);
    dst->removeAllChilds();

    for (int i = 0; i < src->getChildCount(); ++i)
    {
        dst->addChild(src->getChild(i));
    }

    dst->setContent(src->getContent());
}

static void FastErase(XmlNodeRef node, int i)
{
    XmlNodeRef child = node->getChild(i);
    XmlNodeRef last = node->getChild(node->getChildCount() - 1);

    // Copy the last child to the index i.
    CopyNode(child, last);

    // And pop back the last.
    node->deleteChildAt(node->getChildCount() - 1);
}

XmlNodeRef CAssetBrowserManager::ReadAndVerifyMainDB(const QString& fileName)
{
    XmlNodeRef root;

    ///1. Read the main DB first.
    if (CFileUtil::FileExists(fileName) == false)
    {
        root = GetISystem()->CreateXmlNode("AssetMetaDataFileDB");
    }
    else
    {
        root = GetISystem()->LoadXmlFromFile(fileName.toUtf8().data());

        if (root == NULL)
        {
            return NULL;
        }
    }

    ///2. Check if each file still exists/has the same timestamp.
    for (int i = 0; i < root->getChildCount(); ++i)
    {
        XmlNodeRef db = root->getChild(i);

        for (int k = 0; k < db->getChildCount(); ++k)
        {
            XmlNodeRef entry = db->getChild(k);
            IFileUtil::FileDesc fd;

            QString savedTimeStamp = entry->getAttr("timeStamp");

            if (savedTimeStamp == "1")
            {
                continue;
            }

            if (CFileUtil::FileExists(entry->getAttr("fileName"), &fd) == false)
            {
                // The file no longer exists, remove the entry from the DB.
                FastErase(db, k);
            }
            else
            {
                QString actualTimeStamp;
                actualTimeStamp = QString::number(fd.time_write);

                if (actualTimeStamp != savedTimeStamp)
                {
                    // The file changed, remove the entry from the DB.
                    FastErase(db, k);
                }
            }
        }
    }

    return root;
}

void CAssetBrowserManager::ReadTransactionsAndUpdateDB(const QString& fullPath, XmlNodeRef& root)
{
    ///3. Now read transaction files for each DB and process each.
    IAssetItemDatabase* pCurrentDatabaseInterface = NULL;
    std::vector<IClassDesc*> assetDatabasePlugins;
    IEditorClassFactory* pClassFactory = GetIEditor()->GetClassFactory();
    pClassFactory->GetClassesByCategory("Asset Item DB", assetDatabasePlugins);

    for (size_t i = 0; i < assetDatabasePlugins.size(); ++i)
    {
        if (assetDatabasePlugins[i]->QueryInterface(__uuidof(IAssetItemDatabase), (void**)&pCurrentDatabaseInterface) == S_OK)
        {
            QString filePath = fullPath + pCurrentDatabaseInterface->GetTransactionFilename();

            if (!CFileUtil::FileExists(filePath))
            {
                continue;
            }

            uint64 size;

            if (CFileUtil::GetDiskFileSize(filePath.toUtf8().data(), size))
            {
                if (!size)
                {
                    continue;
                }
            }
            else
            {
                continue;
            }

            XmlNodeRef rootTransaction = GetISystem()->LoadXmlFromFile(filePath.toUtf8().data());

            if (rootTransaction == NULL)
            {
                continue;
            }

            // Update with transactions.
            XmlNodeRef dbNode = root->findChild(pCurrentDatabaseInterface->GetDatabaseName());

            if (dbNode == NULL)
            {
                dbNode = root->newChild(pCurrentDatabaseInterface->GetDatabaseName());
            }

            for (int k = 0; k < rootTransaction->getChildCount(); ++k)
            {
                XmlNodeRef transaction = rootTransaction->getChild(k);
                bool found = false;

                for (int m = 0; m < dbNode->getChildCount(); ++m)
                {
                    XmlNodeRef entry = dbNode->getChild(m);

                    if (strcmp(entry->getAttr("fileName"), transaction->getAttr("fileName")) == 0)
                    {
                        CopyNode(entry, transaction);       // Update the entry if it already exists.
                        found = true;
                        break;
                    }
                }

                if (found == false)
                {
                    dbNode->addChild(transaction);      // Or add a new one.
                }
            }
        }
    }
}

void CAssetBrowserManager::ClearTransactionsAndApplyMetaData(const QString& fullPath, XmlNodeRef& root)
{
    IAssetItemDatabase* pCurrentDatabaseInterface = NULL;
    std::vector<IClassDesc*> assetDatabasePlugins;
    IEditorClassFactory* pClassFactory = GetIEditor()->GetClassFactory();
    pClassFactory->GetClassesByCategory("Asset Item DB", assetDatabasePlugins);

    for (size_t i = 0; i < assetDatabasePlugins.size(); ++i)
    {
        // Clear transaction files.
        if (assetDatabasePlugins[i]->QueryInterface(__uuidof(IAssetItemDatabase), (void**)&pCurrentDatabaseInterface) == S_OK)
        {
            QString filePath = fullPath + pCurrentDatabaseInterface->GetTransactionFilename();
            FILE* file = fopen(filePath.toUtf8().data(), "wt");

            if (file == NULL)
            {
                continue;
            }

            fclose(file);
        }

        // Initialize each asset browser database.
        pCurrentDatabaseInterface->Refresh();

        // Precache each asset browser database with the info DB.
        XmlNodeRef dbNode = root->findChild(pCurrentDatabaseInterface->GetDatabaseName());

        if (dbNode)
        {
            pCurrentDatabaseInterface->PrecacheFieldsInfoFromFileDB(dbNode);
        }
    }
}

IAssetItemDatabase* CAssetBrowserManager::GetDatabaseByName(const char* pName)
{
    for (size_t i = 0, iCount = m_assetDatabases.size(); i < iCount; ++i)
    {
        if (!_stricmp(m_assetDatabases[i]->GetDatabaseName(), pName))
        {
            return m_assetDatabases[i];
        }
    }

    return NULL;
}

void CAssetBrowserManager::EnqueueAssetForThumbLoad(IAssetItem* pAsset)
{
    m_thumbLoadQueue.insert(pAsset);
}

void CAssetBrowserManager::OnObjectEvent(CBaseObject* pObject, int nEvent)
{
    if (!pObject)
    {
        return;
    }
}

void CAssetBrowserManager::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginSceneOpen:
        m_bLevelLoading = true;
        break;

    case eNotify_OnEndSceneOpen:
        m_bLevelLoading = false;
        break;

    case eNotify_OnLayerImportBegin:
        m_bLevelLoading = true;
        break;

    case eNotify_OnLayerImportEnd:
        m_bLevelLoading = false;
        break;
    }
}

void CAssetBrowserManager::Run()
{
}

void CAssetBrowserManager::InitializeTagging()
{
    if (GetIEditor()->GetObjectManager())
    {
        GetIEditor()->GetObjectManager()->AddObjectEventListener(functor(*this, &CAssetBrowserManager::OnObjectEvent));
    }

    GetIEditor()->RegisterNotifyListener(this);

    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (pAssetTagging)
    {
        pAssetTagging->Initialize(Path::GetResolvedUserSandboxFolder().toUtf8().data());
    }
}

int CAssetBrowserManager::CreateTag(const QString& tag, const QString& category)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (pAssetTagging)
    {
        return pAssetTagging->CreateTag(tag.toUtf8().data(), category.toUtf8().data());
    }

    return 0;
}

int CAssetBrowserManager::CreateAsset(const QString& path, const QString& project)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (pAssetTagging)
    {
        return pAssetTagging->CreateAsset(path.toUtf8().data(), project.toUtf8().data());
    }

    return 0;
}

int CAssetBrowserManager::CreateProject(const QString& project)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (pAssetTagging)
    {
        return pAssetTagging->CreateProject(project.toUtf8().data());
    }

    return 0;
}

void CAssetBrowserManager::AddAssetsToTag(const QString& tag, const QString& category, const StrVector& assets)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (pAssetTagging)
    {
        const int assetArrayLen = assets.size();

        if (assetArrayLen == 0)
        {
            return;
        }

        const int kMaxStrLen = pAssetTagging->GetMaxStringLen();

        char** assetsStr = new char*[assetArrayLen];

        for (int idx = 0; idx < assetArrayLen; ++idx)
        {
            assetsStr[idx] = new char[kMaxStrLen];
            _snprintf(assetsStr[idx], kMaxStrLen, "%s", assets[idx]);
        }

        pAssetTagging->AddAssetsToTag(tag.toUtf8().data(), category.toUtf8().data(), GetProjectName().toUtf8().data(), assetsStr, assetArrayLen);

        for (int idx = 0; idx < assetArrayLen; ++idx)
        {
            delete[] assetsStr[idx];
        }

        delete[] assetsStr;
    }
}

void CAssetBrowserManager::AddTagsToAsset(const QString& asset, const QString& tag, const QString& category)
{
    StrVector newtags;
    StrVector assets;

    assets.push_back(asset);
    AddAssetsToTag(tag, category, assets);
}

void CAssetBrowserManager::RemoveAssetsFromTag(const QString& tag, const QString& category, const StrVector& assets)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (pAssetTagging)
    {
        const int kAssetArrayLen = assets.size();

        if (kAssetArrayLen == 0)
        {
            return;
        }

        const int maxStrLen = pAssetTagging->GetMaxStringLen();

        char** assetsStr = new char*[kAssetArrayLen];

        for (int idx = 0; idx < kAssetArrayLen; ++idx)
        {
            assetsStr[idx] = new char[maxStrLen];
            _snprintf(assetsStr[idx], maxStrLen, "%s", assets[idx]);
        }

        pAssetTagging->RemoveAssetsFromTag(tag.toUtf8().data(), category.toUtf8().data(), GetProjectName().toUtf8().data(), assetsStr, kAssetArrayLen);

        for (int idx = 0; idx < kAssetArrayLen; ++idx)
        {
            delete[] assetsStr[idx];
        }

        delete[] assetsStr;
    }
}

void CAssetBrowserManager::RemoveTagFromAsset(const QString& tag, const QString& category, const QString& asset)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (!pAssetTagging)
    {
        return;
    }

    pAssetTagging->RemoveTagFromAsset(tag.toUtf8().data(), category.toUtf8().data(), GetProjectName().toUtf8().data(), asset.toUtf8().data());
}

void CAssetBrowserManager::DestroyTag(const QString& tag)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (pAssetTagging)
    {
        pAssetTagging->DestroyTag(tag.toUtf8().data());
    }
}

int CAssetBrowserManager::GetTagsForAsset(StrVector& tags, const QString& asset)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (!pAssetTagging)
    {
        return 0;
    }

    const int kNumTags = pAssetTagging->GetNumTagsForAsset(asset.toUtf8().data());

    if (!kNumTags)
    {
        return 0;
    }

    const int kMaxStrLen = pAssetTagging->GetMaxStringLen();

    char** ppTagsRet = new char*[kNumTags];

    for (int idx = 0; idx < kNumTags; ++idx)
    {
        ppTagsRet[idx] = new char[kMaxStrLen];
        memset(ppTagsRet[idx], '\0', kMaxStrLen);
    }

    const int kCountRet = pAssetTagging->GetTagsForAsset(ppTagsRet, kNumTags, asset.toUtf8().data());

    if (kCountRet > 0)
    {
        for (int idx = 0; idx < kCountRet; ++idx)
        {
            QString tag = ppTagsRet[idx];
            tags.push_back(tag);
        }
    }

    for (int idx = 0; idx < kNumTags; ++idx)
    {
        delete[] ppTagsRet[idx];
    }

    delete[] ppTagsRet;

    return kCountRet;
}

int CAssetBrowserManager::GetTagForAssetInCategory(QString& tag, const QString& asset, const QString& category)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (!pAssetTagging)
    {
        return 0;
    }

    const int kMaxStrLen = pAssetTagging->GetMaxStringLen();

    char* tagsRet = new char[kMaxStrLen];
    memset(tagsRet, '\0', kMaxStrLen);
    pAssetTagging->GetTagForAssetInCategory(tagsRet, asset.toUtf8().data(), category.toUtf8().data());
    tag = tagsRet;

    delete[] tagsRet;
    return 1;
}

int CAssetBrowserManager::GetAssetsForTag(StrVector& assets, const QString& tag)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (!pAssetTagging)
    {
        return 0;
    }

    const int kNumAssets = pAssetTagging->GetNumAssetsForTag(tag.toUtf8().data());

    if (!kNumAssets)
    {
        return 0;
    }

    const int kMaxStrLen = pAssetTagging->GetMaxStringLen();

    char** ppAssetsRet = new char*[kNumAssets];

    for (int idx = 0; idx < kNumAssets; idx++)
    {
        ppAssetsRet[idx] = new char[kMaxStrLen];
        memset(ppAssetsRet[idx], '\0', kMaxStrLen);
    }

    const int kRetAssets = pAssetTagging->GetAssetsForTag(ppAssetsRet, kNumAssets, tag.toUtf8().data());

    if (kRetAssets > 0)
    {
        for (int idx = 0; idx < kRetAssets; ++idx)
        {
            QString asset(ppAssetsRet[idx]);
            assets.push_back(asset);
        }
    }

    for (int idx = 0; idx < kNumAssets; ++idx)
    {
        delete[] ppAssetsRet[idx];
    }

    delete[] ppAssetsRet;

    return kRetAssets;
}

int CAssetBrowserManager::GetAssetCountForTag(const QString& tag)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (!pAssetTagging)
    {
        return 0;
    }

    return pAssetTagging->GetNumAssetsForTag(tag.toUtf8().data());
}

int CAssetBrowserManager::GetAssetsWithDescription(StrVector& assets, const QString& description)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (!pAssetTagging)
    {
        return 0;
    }

    const int kNumAssets = pAssetTagging->GetNumAssetsWithDescription(description.toUtf8().data());

    if (!kNumAssets)
    {
        return 0;
    }

    const int kMaxStrLen = pAssetTagging->GetMaxStringLen();

    char** ppAssetsRet = new char*[kNumAssets];

    for (int idx = 0; idx < kNumAssets; idx++)
    {
        ppAssetsRet[idx] = new char[kMaxStrLen];
        memset(ppAssetsRet[idx], '\0', kMaxStrLen);
    }

    const int kRetAssets = pAssetTagging->GetAssetsWithDescription(ppAssetsRet, kNumAssets, description.toUtf8().data());

    if (kRetAssets > 0)
    {
        for (int idx = 0; idx < kRetAssets; ++idx)
        {
            QString asset(ppAssetsRet[idx]);
            assets.push_back(asset);
        }
    }

    for (int idx = 0; idx < kNumAssets; ++idx)
    {
        delete[] ppAssetsRet[idx];
    }

    delete[] ppAssetsRet;

    return kRetAssets;
}

int CAssetBrowserManager::GetAllTags(StrVector& tags)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (!pAssetTagging)
    {
        return 0;
    }

    const int kNumTags = pAssetTagging->GetNumTags();

    if (kNumTags == 0)
    {
        return 0;
    }

    const int kMaxStrLen = pAssetTagging->GetMaxStringLen();

    char** ppTagsRet = new char*[kNumTags];

    for (int idx = 0; idx < kNumTags; idx++)
    {
        ppTagsRet[idx] = new char[kMaxStrLen];
        memset(ppTagsRet[idx], '\0', kMaxStrLen);
    }

    const int kRetTags = pAssetTagging->GetAllTags(ppTagsRet, kNumTags);

    if (kRetTags > 0)
    {
        for (int idx = 0; idx < kRetTags; idx++)
        {
            QString tag(ppTagsRet[idx]);
            tags.push_back(tag);
        }
    }

    for (int idx = 0; idx < kNumTags; idx++)
    {
        delete[] ppTagsRet[idx];
    }

    delete[] ppTagsRet;

    return kRetTags;
}

int CAssetBrowserManager::GetAllTagCategories(StrVector& categories)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (!pAssetTagging)
    {
        return 0;
    }

    const int numCategories = pAssetTagging->GetNumCategories();

    if (numCategories == 0)
    {
        return 0;
    }

    const int maxStrLen = pAssetTagging->GetMaxStringLen();

    char** categoriesRet = new char*[numCategories];

    for (int idx = 0; idx < numCategories; idx++)
    {
        categoriesRet[idx] = new char[maxStrLen];
        memset(categoriesRet[idx], '\0', maxStrLen);
    }

    const int nRetCategories = pAssetTagging->GetAllCategories(categoriesRet, numCategories);

    if (nRetCategories > 0)
    {
        for (int idx = 0; idx < nRetCategories; idx++)
        {
            QString category(categoriesRet[idx]);
            categories.push_back(category);
        }
    }

    for (int idx = 0; idx < numCategories; idx++)
    {
        delete[] categoriesRet[idx];
    }

    delete[] categoriesRet;

    return nRetCategories;
}

int CAssetBrowserManager::GetTagsForCategory(const QString& category, StrVector& tags)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (!pAssetTagging)
    {
        return 0;
    }

    const int kNumTags = pAssetTagging->GetNumTagsForCategory(category.toUtf8().data());

    if (!kNumTags)
    {
        return 0;
    }

    const int kMaxStrLen = pAssetTagging->GetMaxStringLen();

    char** ppTagsRet = new char*[kNumTags];

    for (int idx = 0; idx < kNumTags; idx++)
    {
        ppTagsRet[idx] = new char[kMaxStrLen];
        memset(ppTagsRet[idx], '\0', kMaxStrLen);
    }

    const int kRetTags = pAssetTagging->GetTagsForCategory(category.toUtf8().data(), ppTagsRet, kNumTags);

    if (kRetTags > 0)
    {
        for (int idx = 0; idx < kRetTags; idx++)
        {
            QString tag(ppTagsRet[idx]);
            tags.push_back(tag);
        }
    }

    for (int idx = 0; idx < kNumTags; idx++)
    {
        delete[] ppTagsRet[idx];
    }

    delete[] ppTagsRet;

    return kRetTags;
}

int CAssetBrowserManager::TagExists(const QString& tag, const QString& category)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (pAssetTagging)
    {
        return pAssetTagging->TagExists(tag.toUtf8().data(), category.toUtf8().data());
    }

    return 0;
}

int CAssetBrowserManager::AssetExists(const QString& relpath)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (pAssetTagging)
    {
        return pAssetTagging->AssetExists(relpath.toUtf8().data());
    }

    return 0;
}

int CAssetBrowserManager::ProjectExists(const QString& project)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (pAssetTagging)
    {
        return pAssetTagging->ProjectExists(project.toUtf8().data());
    }

    return 0;
}

bool CAssetBrowserManager::GetAssetDescription(const QString& relpath, QString& description)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (!pAssetTagging)
    {
        return false;
    }

    const int maxStrLen = pAssetTagging->GetMaxStringLen();

    char* assetDescription = new char[maxStrLen];
    memset(assetDescription, '\0', maxStrLen);

    bool bGot = pAssetTagging->GetAssetDescription(relpath.toUtf8().data(), assetDescription, maxStrLen);

    if (bGot)
    {
        description = assetDescription;
    }

    delete[] assetDescription;
    assetDescription = NULL;

    return bGot;
}

bool CAssetBrowserManager::GetAutocompleteDescription(const QString& partDesc, QString& description)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (!pAssetTagging)
    {
        return false;
    }

    const int maxStrLen = pAssetTagging->GetMaxStringLen();

    char* fullDescription = new char[maxStrLen];
    memset(fullDescription, '\0', maxStrLen);

    bool bGot = pAssetTagging->AutoCompleteDescription(partDesc.toUtf8().data(), fullDescription, maxStrLen);

    if (bGot)
    {
        description = fullDescription;
    }

    delete[] fullDescription;
    fullDescription = NULL;

    return bGot;
}

void CAssetBrowserManager::SetAssetDescription(const QString& relpath, const QString& description)
{
    IAssetTagging* pAssetTagging = GetIEditor()->GetAssetTagging();

    if (!pAssetTagging)
    {
        return;
    }

    pAssetTagging->SetAssetDescription(relpath.toUtf8().data(), GetProjectName().toUtf8().data(), description.toUtf8().data());
}

QString CAssetBrowserManager::GetProjectName()
{
    ICVar* pCvar = gEnv->pConsole->GetCVar("sys_game_folder");

    if (pCvar)
    {
        return QString(pCvar->GetString());
    }

    return QString("unknown");
}

void CAssetBrowserManager::CreateAssetDatabases()
{
    size_t totalDatabaseCount = 0;
    std::vector<IClassDesc*> assetDatabasePlugins;
    IEditorClassFactory* pClassFactory = NULL;
    IAssetItemDatabase* pCurrentDatabaseInterface = NULL;

    pClassFactory = GetIEditor()->GetClassFactory();
    pClassFactory->GetClassesByCategory("Asset Item DB", assetDatabasePlugins);
    totalDatabaseCount = assetDatabasePlugins.size();
    m_assetDatabases.clear();

    for (size_t i = 0; i < totalDatabaseCount; ++i)
    {
        if (assetDatabasePlugins[i]->QueryInterface(
                __uuidof(IAssetItemDatabase),
                (void**)&pCurrentDatabaseInterface) == S_OK)
        {
            pCurrentDatabaseInterface->AddRef();
            m_assetDatabases.push_back(pCurrentDatabaseInterface);
            pCurrentDatabaseInterface->AddMetaDataChangeListener(functor(CAssetBrowserManager::OnNewTransaction));
        }
    }
}

void CAssetBrowserManager::FreeAssetDatabases()
{
    for (size_t i = 0; i < m_assetDatabases.size(); ++i)
    {
        Log("Freeing asset database %s", m_assetDatabases[i]->GetDatabaseName());
        m_assetDatabases[i]->FreeData();
    }

    m_assetDatabases.clear();
}

void CAssetBrowserManager::RefreshAssetDatabases()
{
    for (auto assetDatabase : m_assetDatabases)
    {
        assetDatabase->Refresh();
    }
}

void CAssetBrowserManager::MarkUsedInLevelAssets()
{
    IAssetItemDatabase* pAssetDB = NULL;

    // we will gather all resource filenames from the current level
    CUsedResources usedResources;

    GetIEditor()->GetObjectManager()->GatherUsedResources(usedResources);

    // gather also engine resources not specified in objects
    // engine textures:
    SRendererQueryGetAllTexturesParam param;

    gEnv->pRenderer->EF_Query(EFQ_GetAllTextures, param);

    if (param.numTextures && param.pTextures)
    {
        for (int i = 0; i < param.numTextures; i++)
        {
            usedResources.Add(param.pTextures[i]->GetName());
        }
    }

    CUsedResources::TResourceFiles resLowercase;

    for (auto iter = usedResources.files.begin();
         iter != usedResources.files.end(); ++iter)
    {
        QString str(*iter);
        resLowercase.insert(str.toLower());
    }

    usedResources.files = resLowercase;

    for (size_t i = 0; i < m_assetDatabases.size(); ++i)
    {
        if (m_assetDatabases[i]->QueryInterface(__uuidof(IAssetItemDatabase), (void**)&pAssetDB) == S_OK)
        {
            for (auto iterAsset = pAssetDB->GetAssets().begin();
                 iterAsset != pAssetDB->GetAssets().end();
                 ++iterAsset)
            {
                IAssetItem* pItem = iterAsset->second;

                bool bUsed = (usedResources.files.end() != std::find(usedResources.files.begin(), usedResources.files.end(), iterAsset->first));

                // special case for images
                if (!bUsed && IResourceCompilerHelper::IsGameImageFormatSupported(pItem->GetFileExtension().toUtf8().data()))
                {
                    QString fileName = pItem->GetRelativePath() + pItem->GetFilename();

                    for (unsigned int i = 0; !bUsed && i < IResourceCompilerHelper::GetNumSourceImageFormats(); ++i)
                    {
                        fileName = Path::ReplaceExtension(fileName, IResourceCompilerHelper::GetSourceImageFormat(i, false));
                        bUsed = (usedResources.files.end() != std::find(usedResources.files.begin(), usedResources.files.end(), fileName));
                    }
                }

                pItem->SetFlag(IAssetItem::eFlag_UsedInLevel, bUsed);
            }
        }
    }
}
