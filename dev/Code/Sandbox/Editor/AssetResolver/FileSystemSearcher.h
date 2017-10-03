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

#ifndef CRYINCLUDE_EDITOR_ASSETRESOLVER_FILESYSTEMSEARCHER_H
#define CRYINCLUDE_EDITOR_ASSETRESOLVER_FILESYSTEMSEARCHER_H
#pragma once


#include "IAssetSearcher.h"

////////////////////////////////////////////////////////////////////////////
struct SFileSystemAssetDesc
{
    SFileSystemAssetDesc();
    SFileSystemAssetDesc(const char* _typeName, char _varType = IVariable::DT_SIMPLE);

    void AddExt(const char* ext);
    QString GetTypeName() const;
    int GetExtCount() const;
    QString GetExt(int i) const;
    char GetVarType() const;

    bool Accept(const char* ext) const;
    bool Accept(char _varType) const;

private:
    QStringList extensions;
    QString typeName;
    char varType;
};
typedef std::map<int, SFileSystemAssetDesc> TFileSystemAssetTypes;

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
struct SFileSystemSearchFolder;
struct SFileSystemSearchRequest
{
    SFileSystemSearchRequest(const char* file);
    ~SFileSystemSearchRequest();

    void AddRef();
    void Release();

    bool GetResult(QStringList& result, bool& doneSearching);
    void AddResult(const QString& filepath);

    inline int GetPrio() const { return m_currPrio; }
    static int GetMinPrio();

    inline const QString& GetFile() const { return m_file; }

    void FolderVisited(SFileSystemSearchFolder* pFolder);
    bool AlreadyVisited(SFileSystemSearchFolder* pFolder) const;

public:
    enum EState
    {
        eAdded,
        eProcessing,
        eRemoved,
    };

    volatile EState state;

    static const int MAX_PRIO_COUNT = 10;

private:
    CryCriticalSection m_lock;
    QString m_file;
    volatile int m_refCount;
    int m_currPrio;
    QStringList m_foundFiles;
    std::vector<SFileSystemSearchFolder*> m_folderVisited;

    static volatile int s_totalCount;

    static volatile int s_minPrio[MAX_PRIO_COUNT];
};

TYPEDEF_AUTOPTR(SFileSystemSearchRequest);
typedef SFileSystemSearchRequest_AutoPtr TFileSystemSearchRequestPtr;

typedef std::vector<TFileSystemSearchRequestPtr> TFileSystemReqFiles;
typedef std::map< IAssetSearcher::TAssetSearchId, std::pair<SFileSystemSearchRequest*, QString> > TFileSystemRequests;

////////////////////////////////////////////////////////////////////////////
struct SFileSystemSearchFolder
{
    SFileSystemSearchFolder(const volatile bool& bRunning, const QString& path, SFileSystemSearchFolder* pParent = NULL);
    ~SFileSystemSearchFolder();

    void AddRequest(TFileSystemSearchRequestPtr req, const QString& path);
    void Process();
    bool IsSearching() const;

private:
    SFileSystemSearchFolder* GetOrCreateSubdir(const QString& subdir);
    void AddFileSearch(TFileSystemSearchRequestPtr pSearch, SFileSystemSearchFolder* pSender);
    QString GetFolderName() const;

private:
    const volatile bool& m_bRunning;
    QString m_path;
    QString m_name;
    TFileSystemReqFiles m_filesToSearch;

    typedef std::list<SFileSystemSearchFolder*> TChildren;
    TChildren m_subFolders;
    SFileSystemSearchFolder* m_pParent;
};

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
class CFileSystemSearcher
    : public IAssetSearcher
    , public CrySimpleThread<>
{
public:
    CFileSystemSearcher();
    virtual ~CFileSystemSearcher();

    virtual QString GetName() const { return "FileSystem"; }

    virtual bool Accept(const char* asset, int& assetTypeId) const;
    virtual bool Accept(const char varType, int& assetTypeId) const;

    virtual QString GetAssetTypeName(int assetTypeId);

    virtual bool Exists(const char* asset, int assetTypeId) const;
    virtual bool GetReplacement(QString& replacement, int assetTypeId);

    virtual void StartSearcher();
    virtual void StopSearcher();

    virtual TAssetSearchId AddSearch(const char* asset, int assetTypeId);
    virtual void CancelSearch(TAssetSearchId id);
    virtual bool GetResult(TAssetSearchId id, QStringList& result, bool& doneSearching);

public:
    // CrySimpleThread
    virtual void Run();
    // ~CrySimpleThread

private:
    TAssetSearchId GetNextFreeId() const;

private:
    QString m_lastPath;
    TFileSystemAssetTypes m_assetTypes;
    TFileSystemRequests m_requests;
    SFileSystemSearchFolder* m_pRoot;

    CryMutex m_requestsCS;
    CryConditionVariable m_requestsCV;
};

////////////////////////////////////////////////////////////////////////////
#endif // CRYINCLUDE_EDITOR_ASSETRESOLVER_FILESYSTEMSEARCHER_H
