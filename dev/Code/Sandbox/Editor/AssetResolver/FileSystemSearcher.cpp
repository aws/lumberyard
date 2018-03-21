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
#include "FileSystemSearcher.h"
#include <StringUtils.h>


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
SFileSystemAssetDesc::SFileSystemAssetDesc()
    : typeName("UNDEFINED")
    , varType(IVariable::DT_SIMPLE)
{
}

////////////////////////////////////////////////////////////////////////////
SFileSystemAssetDesc::SFileSystemAssetDesc(const char* _typeName, char _varType)
    : typeName(_typeName)
    , varType(_varType)
{
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemAssetDesc::AddExt(const char* ext)
{
    extensions.push_back(ext);
}

////////////////////////////////////////////////////////////////////////////
QString SFileSystemAssetDesc::GetTypeName() const
{
    return typeName;
}

////////////////////////////////////////////////////////////////////////////
int SFileSystemAssetDesc::GetExtCount() const
{
    return extensions.size();
}

////////////////////////////////////////////////////////////////////////////
QString SFileSystemAssetDesc::GetExt(int i) const
{
    return i >= 0 && i < extensions.size() ? extensions[i] : "";
}

////////////////////////////////////////////////////////////////////////////
char SFileSystemAssetDesc::GetVarType() const
{
    return varType;
}

////////////////////////////////////////////////////////////////////////////
bool SFileSystemAssetDesc::Accept(const char* ext) const
{
    for (int i = 0; i < GetExtCount(); ++i)
    {
        if (QString::compare(ext, GetExt(i), Qt::CaseInsensitive) == 0)
        {
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////
bool SFileSystemAssetDesc::Accept(char _varType) const
{
    return varType != IVariable::DT_SIMPLE && varType == _varType;
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
volatile int SFileSystemSearchRequest::s_totalCount = 0;
volatile int SFileSystemSearchRequest::s_minPrio[SFileSystemSearchRequest::MAX_PRIO_COUNT];

////////////////////////////////////////////////////////////////////////////
SFileSystemSearchRequest::SFileSystemSearchRequest(const char* file)
    : m_refCount(1)
    , m_file(file)
    , m_currPrio(0)
    , state(eAdded)
{
    long refCnt = CryInterlockedIncrement(&s_totalCount);
    CryInterlockedIncrement(&s_minPrio[m_currPrio]);
}

////////////////////////////////////////////////////////////////////////////
SFileSystemSearchRequest::~SFileSystemSearchRequest()
{
    long refCnt = CryInterlockedDecrement(&s_totalCount);
    CryInterlockedDecrement(&s_minPrio[m_currPrio]);
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemSearchRequest::AddRef()
{
    CryInterlockedIncrement(&m_refCount);
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemSearchRequest::Release()
{
    long refCnt = CryInterlockedDecrement(&m_refCount);
    if (refCnt <= 0)
    {
        delete this;
    }
}

////////////////////////////////////////////////////////////////////////////
bool SFileSystemSearchRequest::GetResult(QStringList& result, bool& doneSearching)
{
    CryAutoCriticalSection lock(m_lock);
    bool res = !m_foundFiles.empty();
    for (QStringList::iterator it = m_foundFiles.begin(); it != m_foundFiles.end(); ++it)
    {
        result.push_back(*it);
    }
    m_foundFiles.clear();
    doneSearching = state == eProcessing && m_refCount == 1;
    return res;
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemSearchRequest::AddResult(const QString& filepath)
{
    CryAutoCriticalSection lock(m_lock);
    m_foundFiles.push_back(filepath);
    CryInterlockedDecrement(&s_minPrio[m_currPrio]);
    m_currPrio = min(m_currPrio + 1, MAX_PRIO_COUNT - 1);
    CryInterlockedIncrement(&s_minPrio[m_currPrio]);
}

////////////////////////////////////////////////////////////////////////////
int SFileSystemSearchRequest::GetMinPrio()
{
    for (int i = 0; i < MAX_PRIO_COUNT; ++i)
    {
        if (s_minPrio[i] > 0)
        {
            return i;
        }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemSearchRequest::FolderVisited(SFileSystemSearchFolder* pFolder)
{
    m_folderVisited.push_back(pFolder);
}

////////////////////////////////////////////////////////////////////////////
bool SFileSystemSearchRequest::AlreadyVisited(SFileSystemSearchFolder* pFolder) const
{
    return stl::find(m_folderVisited, pFolder);
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
SFileSystemSearchFolder::SFileSystemSearchFolder(const volatile bool& bRunning, const QString& path, SFileSystemSearchFolder* pParent)
    : m_bRunning(bRunning)
    , m_pParent(pParent)
    , m_path(path.toLower())
{
    while (m_path.startsWith('/'))
    {
        m_path = m_path.mid(1);
    }
    while (m_path.endsWith('/'))
    {
        m_path = m_path.left(m_path.length() - 1);
    }
    m_name = GetFolderName();
}

////////////////////////////////////////////////////////////////////////////
SFileSystemSearchFolder::~SFileSystemSearchFolder()
{
    for (TChildren::iterator it = m_subFolders.begin(); it != m_subFolders.end(); ++it)
    {
        delete *it;
    }
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemSearchFolder::AddRequest(TFileSystemSearchRequestPtr req, const QString& path)
{
    int pos = path.indexOf('/');
    if (pos > 0)
    {
        QString remaining = path.mid(pos + 1);
        pos = remaining.indexOf('/');
        QString next = pos > 0 ? remaining.left(pos) : remaining;

        SFileSystemSearchFolder* pFolder = GetOrCreateSubdir(next);
        pFolder->AddRequest(req, remaining);
    }
    else
    {
        req->state = SFileSystemSearchRequest::eProcessing;
        m_filesToSearch.push_back(req);
    }
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemSearchFolder::Process()
{
    if (!m_bRunning)
    {
        return;
    }

    int minPrio = SFileSystemSearchRequest::GetMinPrio();
    int folderPrio = SFileSystemSearchRequest::MAX_PRIO_COUNT;
    for (TFileSystemReqFiles::iterator it = m_filesToSearch.begin(); it != m_filesToSearch.end(); )
    {
        if ((*it)->state == SFileSystemSearchRequest::eRemoved)
        {
            it = m_filesToSearch.erase(it);
        }
        else
        {
            folderPrio = min((*it++)->GetPrio(), folderPrio);
        }
    }

    if (folderPrio <= minPrio)
    {
        ICryPak* pCryPak = gEnv->pCryPak;
        _finddata_t fd;

        QString search = m_path + "/*.*";
        intptr_t handle = pCryPak->FindFirst(search.toUtf8().data(), &fd);
        if (handle != -1)
        {
            int res = 0;
            do
            {
                if (fd.name[0] != 0 && (fd.attrib & _A_SUBDIR) != 0 && strcmpi(fd.name, ".") != 0 && strcmpi(fd.name, "..") != 0)
                {
                    QString dir = fd.name;
                    GetOrCreateSubdir(dir.toLower());
                }
                else
                {
                    for (TFileSystemReqFiles::iterator it = m_filesToSearch.begin(); it != m_filesToSearch.end(); ++it)
                    {
                        if (QString::compare(fd.name, (*it)->GetFile(), Qt::CaseInsensitive) == 0)
                        {
                            QString filepath = m_path + "/";
                            int p = filepath.indexOf('/');
                            filepath = filepath.mid(p + 1); // remove the game folder!
                            filepath += fd.name;
                            (*it)->AddResult(filepath);
                        }
                    }
                }
                res = pCryPak->FindNext(handle, &fd);
            } while (res >= 0);
            pCryPak->FindClose(handle);
        }

        for (TFileSystemReqFiles::iterator it = m_filesToSearch.begin(); it != m_filesToSearch.end(); ++it)
        {
            (*it)->FolderVisited(this);
            if (m_pParent)
            {
                m_pParent->AddFileSearch(*it, this);
            }
            for (TChildren::iterator sub = m_subFolders.begin(); sub != m_subFolders.end(); ++sub)
            {
                (*sub)->AddFileSearch(*it, this);
            }
        }

        m_filesToSearch.clear();
    }

    for (TChildren::iterator it = m_subFolders.begin(); it != m_subFolders.end(); )
    {
        SFileSystemSearchFolder* subfolder = (*it);

        if (subfolder->IsSearching())
        {
            subfolder->Process();
            ++it;
        }
        else
        {
            SAFE_DELETE(subfolder);
            it = m_subFolders.erase(it);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
SFileSystemSearchFolder* SFileSystemSearchFolder::GetOrCreateSubdir(const QString& subdir)
{
    for (TChildren::iterator it = m_subFolders.begin(); it != m_subFolders.end(); ++it)
    {
        if ((*it)->m_name == subdir)
        {
            return *it;
        }
    }
    QString path = m_path + "/";
    path += subdir;
    SFileSystemSearchFolder* pNewFolder = new SFileSystemSearchFolder(m_bRunning, path, this);
    m_subFolders.push_back(pNewFolder);
    return pNewFolder;
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemSearchFolder::AddFileSearch(TFileSystemSearchRequestPtr pSearch, SFileSystemSearchFolder* pSender)
{
    if (!m_bRunning)
    {
        return;
    }

    if (pSearch->AlreadyVisited(this))
    {
        if (m_pParent && m_pParent != pSender)
        {
            m_pParent->AddFileSearch(pSearch, this);
        }
        for (TChildren::iterator it = m_subFolders.begin(); it != m_subFolders.end(); ++it)
        {
            if (*it != pSender)
            {
                (*it)->AddFileSearch(pSearch, this);
            }
        }
        return;
    }

    stl::push_back_unique(m_filesToSearch, pSearch);
}

////////////////////////////////////////////////////////////////////////////
QString SFileSystemSearchFolder::GetFolderName() const
{
    QString res = m_path;
    QString folder = res;
    while (folder.length() > 1)
    {
        if (folder.startsWith('/'))
        {
            res = folder.mid(1);
        }
        folder = folder.mid(1);
    }
    return res;
}

bool SFileSystemSearchFolder::IsSearching() const
{
    bool isSearching = !m_filesToSearch.empty();
    for (TChildren::const_iterator it = m_subFolders.begin(); it != m_subFolders.end(); ++it)
    {
        isSearching |= (*it)->IsSearching();
    }

    return isSearching;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
CFileSystemSearcher::CFileSystemSearcher()
{
    m_lastPath = Path::GetEditingGameDataFolder().c_str();
    m_pRoot = new SFileSystemSearchFolder(GetStartedState(), m_lastPath);

    m_assetTypes[0] = SFileSystemAssetDesc("Object", IVariable::DT_OBJECT);
    m_assetTypes[0].AddExt("cgf");

    m_assetTypes[1] = SFileSystemAssetDesc("Texture", IVariable::DT_TEXTURE);
    for (unsigned int i = 0; i < IResourceCompilerHelper::GetNumEngineImageFormats(); ++i)
    {
        m_assetTypes[1].AddExt(IResourceCompilerHelper::GetEngineImageFormat(i, false));
    }
    for (unsigned int i = 0; i < IResourceCompilerHelper::GetNumSourceImageFormats(); ++i)
    {
        m_assetTypes[1].AddExt(IResourceCompilerHelper::GetSourceImageFormat(i, false));
    }

    m_assetTypes[2] = SFileSystemAssetDesc("Animation", IVariable::DT_ANIMATION);
    m_assetTypes[2].AddExt("caf");

    m_assetTypes[3] = SFileSystemAssetDesc("SoundFile");
    m_assetTypes[3].AddExt("mp2");
    m_assetTypes[3].AddExt("mp3");
    m_assetTypes[3].AddExt("wav");

    m_assetTypes[4] = SFileSystemAssetDesc("Scripts");
    m_assetTypes[4].AddExt("lua");

    m_assetTypes[5] = SFileSystemAssetDesc("XML");
    m_assetTypes[5].AddExt("xml");

    m_assetTypes[6] = SFileSystemAssetDesc("Material", IVariable::DT_MATERIAL);
    m_assetTypes[6].AddExt("mtl");
}

////////////////////////////////////////////////////////////////////////////
CFileSystemSearcher::~CFileSystemSearcher()
{
    StopSearcher();
    WaitForThread();
    SAFE_DELETE(m_pRoot);
}

////////////////////////////////////////////////////////////////////////////
bool CFileSystemSearcher::Accept(const char* asset, int& assetTypeId) const
{
    const char* pExt = CryStringUtils::FindExtension(asset);
    for (TFileSystemAssetTypes::const_iterator it = m_assetTypes.begin(); it != m_assetTypes.end(); ++it)
    {
        if (it->second.Accept(pExt))
        {
            assetTypeId = it->first;
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////
bool CFileSystemSearcher::Accept(const char varType, int& assetTypeId) const
{
    for (TFileSystemAssetTypes::const_iterator it = m_assetTypes.begin(); it != m_assetTypes.end(); ++it)
    {
        if (it->second.Accept(varType))
        {
            assetTypeId = it->first;
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////
QString CFileSystemSearcher::GetAssetTypeName(int assetTypeId)
{
    TFileSystemAssetTypes::const_iterator it = m_assetTypes.find(assetTypeId);
    return it != m_assetTypes.end() ? it->second.GetTypeName() : "UNDEFINED";
}

////////////////////////////////////////////////////////////////////////////
bool CFileSystemSearcher::Exists(const char* asset, int assetTypeId) const
{
    QString assetName = asset;
    const char* pExt = PathUtil::GetExt(asset);

    TFileSystemAssetTypes::const_iterator it = m_assetTypes.find(assetTypeId);
    assert(it != m_assetTypes.end());

    if (strcmp(pExt, "") == 0)
    {
        assetName += ".";
        assetName += it->second.GetExt(0);
    }

    bool fileExists = gEnv->pCryPak->IsFileExist(assetName.toUtf8().data());

    if (!fileExists && (it->second.GetVarType() == IVariable::DT_TEXTURE))
    {
        //check if the file exists under a different texture extension
        for (int i = 0; i < it->second.GetExtCount(); i++)
        {
            fileExists = gEnv->pCryPak->IsFileExist(PathUtil::ReplaceExtension(assetName.toUtf8().data(), it->second.GetExt(i).toUtf8().data()));
            if (fileExists)
            {
                break;
            }
        }
    }

    return fileExists;
}

////////////////////////////////////////////////////////////////////////////
bool CFileSystemSearcher::GetReplacement(QString& replacement, int assetTypeId)
{
    TFileSystemAssetTypes::const_iterator it = m_assetTypes.find(assetTypeId);
    assert(it != m_assetTypes.end());

    QStringList filters;
    for (int i = 0; i < it->second.GetExtCount(); ++i)
    {
        filters.append(QString("%1 (*.%2)").arg(it->second.GetTypeName(), it->second.GetExt(i)));
    }
    filters.append(QStringLiteral("All files (*)"));

    const QString filter = filters.join(";;");

    QString fullFileName;
    if (CFileUtil::SelectFile(filter, m_lastPath, fullFileName))
    {
        QFileInfo info(fullFileName);
        m_lastPath = info.path();
        string gamePath = Path::FullPathToGamePath(fullFileName).toUtf8().data();
        gamePath = PathUtil::ToUnixPath(gamePath);
        replacement = gamePath.c_str();
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////
void CFileSystemSearcher::StartSearcher()
{
    Start();
}

////////////////////////////////////////////////////////////////////////////
void CFileSystemSearcher::StopSearcher()
{
    Stop();
    m_requestsCV.Notify();

    CryAutoLock<CryMutex> lock(m_requestsCS);
    for (TFileSystemRequests::iterator it = m_requests.begin(); it != m_requests.end(); ++it)
    {
        it->second.first->Release();
    }
    m_requests.clear();
}

////////////////////////////////////////////////////////////////////////////
IAssetSearcher::TAssetSearchId CFileSystemSearcher::AddSearch(const char* asset, int assetTypeId)
{
    QString assetName = asset;
    const char* pExt = CryStringUtils::FindExtension(asset);
    if (strcmp(pExt, "") == 0)
    {
        TFileSystemAssetTypes::const_iterator it = m_assetTypes.find(assetTypeId);
        assert(it != m_assetTypes.end());
        assetName += ".";
        assetName += it->second.GetExt(0);
    }
    assetName = assetName.toLower();

    string file = PathUtil::GetFile(assetName.toUtf8().data());
    string path = PathUtil::GetPath(assetName.toUtf8().data());
    path = PathUtil::ToUnixPath(path);
    path.TrimLeft('/');
    path.TrimRight('/');

    TAssetSearchId id = INVALID_ID;

    {
        CryAutoLock<CryMutex> lock(m_requestsCS);

        if (IsStarted())
        {
            id = GetNextFreeId();
            m_requests[id] = std::make_pair(new SFileSystemSearchRequest(file.c_str()), path.c_str());
        }
    }

    m_requestsCV.Notify();

    return id;
}

////////////////////////////////////////////////////////////////////////////
void CFileSystemSearcher::CancelSearch(TAssetSearchId id)
{
    CryAutoLock<CryMutex> lock(m_requestsCS);
    TFileSystemRequests::iterator it = m_requests.find(id);
    assert(it != m_requests.end());
    it->second.first->state = SFileSystemSearchRequest::eRemoved;
    it->second.first->Release();
    m_requests.erase(it);
}

////////////////////////////////////////////////////////////////////////////
bool CFileSystemSearcher::GetResult(TAssetSearchId id, QStringList& result, bool& doneSearching)
{
    CryAutoLock<CryMutex> lock(m_requestsCS);
    TFileSystemRequests::iterator it = m_requests.find(id);
    assert(it != m_requests.end());
    return it->second.first->GetResult(result, doneSearching);
}

////////////////////////////////////////////////////////////////////////////
void CFileSystemSearcher::Run()
{
    SetName("FileSystemSearcher");

    while (IsStarted())
    {
        m_pRoot->Process();

        CryAutoLock<CryMutex> lock(m_requestsCS);
        while (IsStarted() && m_requests.empty())
        {
            m_requestsCV.Wait(m_requestsCS);
        }

        for (TFileSystemRequests::iterator it = m_requests.begin(); it != m_requests.end(); ++it)
        {
            if (it->second.first->state == SFileSystemSearchRequest::eAdded)
            {
                m_pRoot->AddRequest(it->second.first, it->second.second);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////
IAssetSearcher::TAssetSearchId CFileSystemSearcher::GetNextFreeId() const
{
    TAssetSearchId id = IAssetSearcher::FIRST_VALID_ID;
    for (TFileSystemRequests::const_iterator it = m_requests.begin(); it != m_requests.end() && it->first == id; ++it, ++id)
    {
        ;
    }
    return id;
}

////////////////////////////////////////////////////////////////////////////