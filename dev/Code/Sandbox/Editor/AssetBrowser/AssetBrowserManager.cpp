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

#include "StdAfx.h"
#include "AssetBrowserManager.h"
#include "Util/FileUtil.h"
#include "Util/IndexedFiles.h"
#include "Include/IAssetItemDatabase.h"
#include "Include/IAssetItem.h"
#include "AssetBrowser/AssetBrowserDialog.h"
#include "IEditor.h"
#include "Objects/BrushObject.h"

static CAssetBrowserManager* s_pInstance = 0;

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
}

void CAssetBrowserManager::Shutdown()
{
    FreeAssetDatabases();
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
    AzFramework::StringFunc::Path::ConstructFull(Path::GetUserSandboxFolder().toUtf8().data(), AssetBrowserCommon::kThumbnailsRoot, strUserFolder);

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
    FILE* transactionFile = nullptr;
    azfopen(&transactionFile, fullPath.toUtf8().data(), "r+t");

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
            FILE* file = nullptr;
            azfopen(&file, filePath.toUtf8().data(), "wt");

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
        if (!azstricmp(m_assetDatabases[i]->GetDatabaseName(), pName))
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

void CAssetBrowserManager::Run()
{
}

unsigned int CAssetBrowserManager::HashStringSbdm(const char* pStr)
{
    // details at http://www.cse.yorku.ca/~oz/hash.html
    unsigned long hash = 0;
    int c;

    while (c = *pStr++)
    {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
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
