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

#ifndef CRYINCLUDE_PERFORCEPLUGIN_PERFORCESOURCECONTROL_H
#define CRYINCLUDE_PERFORCEPLUGIN_PERFORCESOURCECONTROL_H
#pragma once


// CRC TODO: Retab! file

#define USERNAME_LENGTH 64

#include "IEditorClassFactory.h"
#include "Include/IEditorClassFactory.h"

#include "p4/clientapi.h"
#include "p4/errornum.h"
#include "Include/ISourceControl.h"

#include <QPixmap>

struct CPerforceThread;

class CMyClientUser
    : public ClientUser
{
public:
    CMyClientUser()
    {
        m_initDesc = "Automatic.";
        strcpy(m_desc, m_initDesc);
        Init();
    }
    void HandleError(Error* e);
    void OutputStat(StrDict* varList);
    void Edit(FileSys* f1, Error* e);
    void OutputInfo(char level, const char* data);
    void Init();
    void PreCreateFileName(const char* file);
    void InputData(StrBuf* buf, Error* e);

    Error m_e;
    char m_action[64];
    char m_headAction[64];
    char m_otherAction[64];
    char m_clientHasLatestRev[64];
    char m_desc[1024];
    char m_file[MAX_PATH];
    char m_findedFile[MAX_PATH];
    char m_depotFile[MAX_PATH];
    char m_movedFile[MAX_PATH];
    char m_otherUser[USERNAME_LENGTH];
    char m_lockedBy[USERNAME_LENGTH];
    QString m_clientRoot;
    QString m_clientName;
    QString m_clientHost;
    QString m_currentDirectory;
    bool m_bIsClientUnknown;

    const char* m_initDesc;
    bool m_bIsSetup;
    bool m_bIsPreCreateFile;
    bool m_bIsCreatingChangelist;
    QString m_output;
    QString m_input;

    int64 m_nFileHeadRev;
    int64 m_nFileHaveRev;
    ESccLockStatus m_lockStatus;
};

class CMyClientApi
    : public ClientApi
{
public:
    void Run(const char* func);
    void Run(const char* func, ClientUser* ui);
};

class CPerforceSourceControl
    : public ISourceControl
    , public IClassDesc
{
public:
    typedef std::unordered_map<uint, connectivityChangedFunction> CallbackMap;
    // constructor
    CPerforceSourceControl();
    virtual ~CPerforceSourceControl();

    bool Connect();
    bool Reconnect();
    void FreeData();
    void Init();
    bool CheckConnectionAndNotifyListeners();

    // from ISourceControl
    uint32 GetFileAttributes(const char* filename) override;

    // Thread processing
    void GetFileAttributesThread(const char* filename);

    bool DoesChangeListExist(const char* pDesc, char* changeid, int nLen);
    bool CreateChangeList(const char* pDesc, char* changeid, int nLen);
    bool Add(const char* filename, const char* desc, int nFlags, char* changelistId = NULL);
    bool CheckOut(const char* filename, int nFlags, char* changelistId = NULL);
    bool UndoCheckOut(const char* filename, int nFlags);
    bool Rename(const char* filename, const char* newfilename, const char* desc, int nFlags);
    bool GetLatestVersion(const char* filename, int nFlags);
    bool GetOtherUser(const char* filename, char* outUser, int nOutUserSize);

    virtual uint RegisterCallback(connectivityChangedFunction func) override;
    virtual void UnRegisterCallback(uint handle) override;
    virtual void SetSourceControlState(SourceControlState state) override;
    virtual ConnectivityState GetConnectivityState() override;

    virtual void ShowSettings() override;

    bool Run(const char* func, int nArgs, char* argv[], bool bOnlyFatal = false);
    uint32 GetFileAttributesAndFileName(const char* filename, char* FullFileName);
    bool IsFolder(const char* filename, char* FullFileName);

    // from IClassDesc
    virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_SCM_PROVIDER; };
    REFGUID ClassID()
    {
        // {3c209e66-0728-4d43-897d-168962d5c8b5}
        static const GUID guid = {
            0x3c209e66, 0x0728, 0x4d43, { 0x89, 0x7d, 0x16, 0x89, 0x62, 0xd5, 0xc8, 0xb5 }
        };
        return guid;
    }
    virtual QString ClassName() { return "Perforce source control"; };
    virtual QString Category() { return "SourceControl"; };
    virtual void ShowAbout() {};

    // from IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObj)
    {
        if (riid == __uuidof(ISourceControl) /* && m_pIntegrator*/)
        {
            *ppvObj = this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() { return ++m_ref; };
    ULONG STDMETHODCALLTYPE Release();


protected:
    bool IsFileManageable(const char* sFilename, bool bCheckFatal = true);
    bool IsFileExistsInDatabase(const char* sFilename);
    bool IsFileCheckedOutByUser(const char* sFilename, bool* isByAnotherUser = 0, bool* isForAdd = 0, bool* isForMove = 0);
    bool IsFileLatestVersion(const char* sFilename);
    void ConvertFileNameCS(char* sDst, const char* sSrcFilename);
    void MakePathCS(char* sDst, const char* sSrcFilename);
    void RenameFolders(const char* path, const char* oldPath);
    bool FindFile(char* clientFile, const char* folder, const char* file);
    bool FindDir(char* clientFile, const char* folder, const char* dir);
    bool IsSomeTimePassed();
    void NotifyListeners();
    void CacheData();

    const char* GetErrorByGenericCode(int nGeneric);

private:
    CMyClientUser m_ui;
    CMyClientApi* m_client;
    Error m_e;

    bool m_bIsWorkOffline;
    bool m_bIsWorkOfflineBecauseOfConnectionLoss;
    bool m_bIsFailConnectionLogged;
    bool m_configurationInvalid;

    CPerforceThread* m_thread;
    uint32 m_unRetValue;
    bool m_isSetuped;
    bool m_isSecondThread;
    bool m_isSkipThread;
    DWORD m_dwLastAccessTime;
    QPixmap m_p4Icon;
    QPixmap m_p4ErrorIcon;

    ULONG m_ref;

    void LoadSettingsIfAvailable();

    uint m_nextHandle;

    CallbackMap m_callbacks;
    bool m_lastWorkOfflineResult;
    bool m_lastWorkOfflineBecauseOfConnectionLossResult;
};




#endif // CRYINCLUDE_PERFORCEPLUGIN_PERFORCESOURCECONTROL_H
