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
#include "CryFile.h"
#include "PerforceSourceControl.h"
#include "PasswordDlg.h"
#include "platform_impl.h"

#include <QSettings>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QApplication>
#include <QTimer>

#include <Util/PathUtil.h>
#include <AzCore/base.h>

namespace
{
    CryCriticalSection g_cPerforceValues;
}

void CMyClientUser::HandleError(Error* e)
{
    /*
    StrBuf  m;
    e->Fmt( &m );
    if ( strstr( m.Text(), "file(s) not in client view." ) )
    e->Clear();
    else if ( strstr( m.Text(), "no such file(s)" ) )
    e->Clear();
    else if ( strstr( m.Text(), "access denied" ) )
    e->Clear();
    */
    m_e = *e;
}

void CMyClientUser::Init()
{
    m_bIsClientUnknown = false;
    m_bIsSetup = false;
    m_bIsPreCreateFile = false;
    m_output.clear();
    m_input.clear();
    m_e.Clear();

    memset(m_action, 0, sizeof(m_action));
    memset(m_headAction, 0, sizeof(m_headAction));
    memset(m_otherAction, 0, sizeof(m_otherAction));
    memset(m_clientHasLatestRev, 0, sizeof(m_clientHasLatestRev));
    memset(m_desc, 0, sizeof(m_desc));
    memset(m_file, 0, sizeof(m_file));
    memset(m_findedFile, 0, sizeof(m_findedFile));
    memset(m_depotFile, 0, sizeof(m_depotFile));
    memset(m_movedFile, 0, sizeof(m_movedFile));
    memset(m_otherUser, 0, sizeof(m_otherUser));
    memset(m_lockedBy, 0, sizeof(m_lockedBy));
}

void CMyClientUser::PreCreateFileName(const char* file)
{
    m_bIsPreCreateFile = true;
    strcpy(m_file, file);
    m_findedFile[0] = 0;
}

void CMyClientUser::OutputStat(StrDict* varList)
{
    if (m_bIsSetup && !m_bIsPreCreateFile)
    {
        return;
    }

    StrRef var, val;
    *m_action = 0;
    *m_headAction = 0;
    *m_otherAction = 0;
    *m_depotFile = 0;
    *m_otherUser = 0;
    m_lockStatus = SCC_LOCK_STATUS_UNLOCKED;

    for (int i = 0; varList->GetVar(i, var, val); i++)
    {
        if (m_bIsPreCreateFile)
        {
            if (var == "clientFile")
            {
                char tmpval[MAX_PATH];
                strcpy(tmpval, val.Text());
                char* ch = tmpval;
                while (ch = strchr(ch, '/'))
                {
                    *ch = '\\';
                }

                if (!stricmp(tmpval, m_file))
                {
                    strcpy(m_findedFile, val.Text());
                    m_bIsPreCreateFile = false;
                }
            }
        }
        else
        {
            if (var == "action")
            {
                strcpy(m_action, val.Text());
            }
            else if (var == "headAction")
            {
                strcpy(m_headAction, val.Text());
            }
            else if (var == "headRev")
            {
                strcpy(m_clientHasLatestRev, val.Text());
                m_nFileHeadRev = val.Atoi64();
            }
            else if (!strncmp(var.Text(), "otherAction", 11) && !strcmp(val.Text(), "edit"))
            {
                strcpy(m_otherAction, val.Text());
            }
            else if (var == "haveRev")
            {
                if (val != m_clientHasLatestRev)
                {
                    m_clientHasLatestRev[0] = 0;
                }
                m_nFileHaveRev = val.Atoi64();
            }
            else if (var == "depotFile")
            {
                cry_strcpy(m_depotFile, val.Text());
            }
            else if (var == "movedFile")
            {
                cry_strcpy(m_movedFile, val.Text());
            }
            else if (var == "otherOpen0")
            {
                cry_strcpy(m_otherUser, val.Text());
            }
            else if (!strncmp(var.Text(), "otherLock0", 10))
            {
                m_lockStatus = SCC_LOCK_STATUS_LOCKED_BY_OTHERS;
                cry_strcpy(m_lockedBy, val.Text());
            }
            else if (var == "ourLock")
            {
                m_lockStatus = SCC_LOCK_STATUS_LOCKED_BY_US;
            }
        }
    }
    m_bIsSetup = true;
}

void CMyClientUser::OutputInfo(char level, const char* data)
{
    m_output.append(data);

    QString strData(data);
    if ("Client unknown." == strData)
    {
        m_bIsClientUnknown = true;
        return;
    }

    QString var = strData.contains(":") ? strData.split(":").first() : QString();
    QString val;
    if (!var.isEmpty())
    {
        val = strData.mid(var.length() + 1).trimmed();
    }

    if ("Client root" == var)
    {
        m_clientRoot = val;
        m_clientRoot.replace('/', '\\');
    }
    else if ("Client host" == var)
    {
        m_clientHost = val;
    }
    else if ("Client name" == var)
    {
        m_clientName = val;
    }
    else if ("Current directory" == var)
    {
        m_currentDirectory = val;
        m_currentDirectory.replace('/', '\\');
    }

    if (!m_bIsPreCreateFile)
    {
        return;
    }

    const char* ch = strrchr(data, '/');
    if (ch)
    {
        if (!stricmp(ch + 1, m_file))
        {
            strcpy(m_findedFile, ch + 1);
            m_bIsPreCreateFile = false;
        }
    }
}

void CMyClientUser::InputData(StrBuf* buf, Error* e)
{
    if (m_bIsCreatingChangelist)
    {
        buf->Set(m_input.toUtf8().data());
    }
}

void CMyClientUser::Edit(FileSys* f1, Error* e)
{
    char msrc[4000];
    char mdst[4000];

    char* src = &msrc[0];
    char* dst = &mdst[0];

    f1->Open(FOM_READ, e);
    int size = f1->Read(msrc, 10240, e);
    msrc[size] = 0;
    f1->Close(e);

    while (*dst = *src)
    {
        if (!strnicmp(src, "\nDescription", 11))
        {
            src++;
            while (*src != '\n' && *src != '\0')
            {
                src++;
            }
            src++;
            while (*src != '\n' && *src != '\0')
            {
                src++;
            }
            strcpy(dst, "\nDescription:\n\t");
            dst += 15;
            strcpy(dst, m_desc);
            dst += strlen(m_desc);
            strcpy(dst, " (by Perforce Plug-in)");
            dst += 22;
        }
        else
        {
            src++;
            dst++;
        }
    }

    f1->Open(FOM_WRITE, e);
    f1->Write(mdst, strlen(mdst), e);
    f1->Close(e);

    strcpy(m_desc, m_initDesc);
}

////////////////////////////////////////////////////////////
void CMyClientApi::Run(const char* func)
{
    // The "enableStreams" var has to be set prior to any Run call in order to be able to support Perforce streams
    ClientApi::SetVar("enableStreams");
    ClientApi::Run(func);
}

void CMyClientApi::Run(const char* func, ClientUser* ui)
{
    ClientApi::SetVar("enableStreams");
    ClientApi::Run(func, ui);
}

////////////////////////////////////////////////////////////
CPerforceSourceControl::CPerforceSourceControl()
    : m_ref(0)
{
    m_bIsWorkOffline = true;
    m_bIsWorkOfflineBecauseOfConnectionLoss = true;
    m_bIsFailConnectionLogged = false;
    m_configurationInvalid = false;
    m_dwLastAccessTime = 0;
    m_client = nullptr;
    m_nextHandle = 0;
    m_lastWorkOfflineResult = true;
    m_lastWorkOfflineBecauseOfConnectionLossResult = true;
}

ULONG STDMETHODCALLTYPE CPerforceSourceControl::Release()
{
    if ((--m_ref) == 0)
    {
        g_cPerforceValues.Lock();
        FreeData();
        delete this;
        g_cPerforceValues.Unlock();
        return 0;
    }
    else
    {
        return m_ref;
    }
}

CPerforceSourceControl::~CPerforceSourceControl()
{
    delete m_client;
}

bool CPerforceSourceControl::Connect()
{
    // before we call Init we must set up the connection!
    // the user may have overridden the connection on a per-project basis.
    // check the config file.
    AUTO_LOCK(g_cPerforceValues);

    FreeData();

    m_client = new CMyClientApi();

    m_client->Init(&m_e);
    if (m_e.Test())
    {
        if (!m_bIsWorkOffline)
        {
            m_bIsWorkOfflineBecauseOfConnectionLoss = true;
        }

        m_bIsWorkOffline = true;
        if (!m_bIsFailConnectionLogged)
        {
            GetIEditor()->GetSystem()->GetILog()->Log("\nPerforce plugin: Failed to connect.");
            m_bIsFailConnectionLogged = true;
        }
        if (GetIEditor()->IsSourceControlAvailable())
        {
            //Only Check Connection and than Notify Listeners if SourceControl is available at that instance
            CheckConnectionAndNotifyListeners();
        }
        return false;
    }
    else
    {
        GetIEditor()->GetSystem()->GetILog()->Log("\nPerforce plugin: Connected.");
        m_bIsFailConnectionLogged = false;
        if (m_bIsWorkOfflineBecauseOfConnectionLoss)
        {
            m_bIsWorkOffline = false;
            m_bIsWorkOfflineBecauseOfConnectionLoss = false;
        }
    }

    return true;
}

bool CPerforceSourceControl::Reconnect()
{
    AUTO_LOCK(g_cPerforceValues);

    if ((m_client && m_client->Dropped()) || m_bIsWorkOfflineBecauseOfConnectionLoss)
    {
        if (!m_bIsFailConnectionLogged)
        {
            GetIEditor()->GetSystem()->GetILog()->Log("\nPerforce connection dropped: attempting reconnect");
        }

        FreeData();
        if (!Connect())
        {
            if (!m_bIsWorkOffline)
            {
                m_bIsWorkOfflineBecauseOfConnectionLoss = true;
            }
            m_bIsWorkOffline = true;
            return false;
        }
        else
        {
            CheckConnectionAndNotifyListeners();
        }
    }
    return true;
}

void CPerforceSourceControl::FreeData()
{
    AUTO_LOCK(g_cPerforceValues);

    if (m_client)
    {
        m_client->Final(&m_e);
        delete m_client;
        m_client = nullptr;
    }
    m_e.Clear();
}

void CPerforceSourceControl::Init()
{
    m_p4Icon = QPixmap(":/res/p4.ico");
    m_p4ErrorIcon = QPixmap(":/res/p4_error.ico");

    Connect();

    {
        AzToolsFramework::SourceControlState state = AzToolsFramework::SourceControlState::Disabled;
        using SCRequestBus = AzToolsFramework::SourceControlConnectionRequestBus;
        SCRequestBus::BroadcastResult(state, &SCRequestBus::Events::GetSourceControlState);
        SetSourceControlState(state);
    }
}

/*
Note:

ConvertFileNameCS doesn't seem to do any conversion.

First it checks if the path is available on local HDD, then if it hasn't found
the path locally it looks in perforce (passing in local paths to P4v's API).

The path stored in sDst is the location which either exists and was found, or
will exist once you get latest...

If the file neither exists locally or in the depot, sDst will be as far as exists in Perforce...
Why is this a problem? Well, if you called GetFileAttributes on a file that doesn't exist in P4V
you'll be given the attributes for the deepest folder of the path you provided which exists on
your local machine - which is probably why you're here reading this comment :)

*/
void CPerforceSourceControl::ConvertFileNameCS(char* sDst, const char* sSrcFilename)
{
    *sDst = 0;
    bool bFinded = true;

    char szAdjustedFile[ICryPak::g_nMaxPath];
    cry_strcpy(szAdjustedFile, sSrcFilename);

    char csPath[ICryPak::g_nMaxPath] = { 0 };

    char* ch, * ch1;

    ch = strrchr(szAdjustedFile, '\\');
    ch1 = strrchr(szAdjustedFile, '/');
    if (ch < ch1)
    {
        ch = ch1;
    }
    bool bIsFirstTime = true;

    bool bIsEndSlash = false;
    if (ch && ch - szAdjustedFile + 1 == strlen(szAdjustedFile))
    {
        bIsEndSlash = true;
    }

    while (ch)
    {
        QFileInfo fi(szAdjustedFile);
        if (fi.exists())
        {
            char tmp[ICryPak::g_nMaxPath];
            cry_strcpy(tmp, csPath);
            cry_strcpy(csPath, fi.fileName().toUtf8().data());
            if (!bIsFirstTime)
            {
                cry_strcat(csPath, "\\");
            }
            bIsFirstTime = false;
            cry_strcat(csPath, tmp);
        }

        *ch = 0;
        ch = strrchr(szAdjustedFile, '\\');
        ch1 = strrchr(szAdjustedFile, '/');
        if (ch < ch1)
        {
            ch = ch1;
        }
    }

    if (!*csPath)
    {
        return;
    }

    cry_strcat(szAdjustedFile, "\\");
    cry_strcat(szAdjustedFile, csPath);
    if (bIsEndSlash || strlen(szAdjustedFile) < strlen(sSrcFilename))
    {
        cry_strcat(szAdjustedFile, "\\");
    }

    // if we have only folder on disk find in perforce
    if (strlen(szAdjustedFile) < strlen(sSrcFilename))
    {
        if (IsFileManageable(szAdjustedFile))
        {
            char file[MAX_PATH];
            char clienFile[MAX_PATH] = { 0 };

            bool bCont = true;
            while (bCont)
            {
                cry_strcpy(file, &sSrcFilename[strlen(szAdjustedFile)]);
                char* ch = strchr(file, '/');
                char* ch1 = strchr(file, '\\');
                if (ch < ch1)
                {
                    ch = ch1;
                }

                if (ch)
                {
                    *ch = 0;
                    bFinded = bCont = FindDir(clienFile, szAdjustedFile, file);
                }
                else
                {
                    bFinded = FindFile(clienFile, szAdjustedFile, file);
                    bCont = false;
                }
                cry_strcpy(szAdjustedFile, clienFile);
                if (bCont && strlen(clienFile) >= strlen(sSrcFilename))
                {
                    bCont = false;
                }
            }
        }
    }

    if (bFinded)
    {
        strcpy(sDst, szAdjustedFile);
    }
}

bool CPerforceSourceControl::FindDir(char* clientFile, const char* folder, const char* dir)
{
    AUTO_LOCK(g_cPerforceValues);

    if (!Reconnect())
    {
        return false;
    }

    char fl[MAX_PATH];
    cry_strcpy(fl, folder);
    cry_strcat(fl, "*");
    char* argv[] = { fl };
    m_ui.PreCreateFileName(dir);

    m_client->SetArgv(1, argv);
    m_client->Run("dirs", &m_ui);
    m_client->WaitTag();

    if (m_ui.m_e.Test())
    {
        return false;
    }

    strcpy(clientFile, folder);
    strcat(clientFile, m_ui.m_findedFile);
    strcat(clientFile, "\\");
    if (*m_ui.m_findedFile)
    {
        return true;
    }

    return false;
}

bool CPerforceSourceControl::FindFile(char* clientFile, const char* folder, const char* file)
{
    if (!Reconnect())
    {
        return false;
    }

    char fullPath[MAX_PATH];

    cry_strcpy(fullPath, folder);
    cry_strcat(fullPath, file);

    char fl[MAX_PATH];
    cry_strcpy(fl, folder);
    cry_strcat(fl, "*");
    char* argv[] = { fl };

    AUTO_LOCK(g_cPerforceValues);

    m_ui.PreCreateFileName(fullPath);

    m_client->SetArgv(1, argv);
    m_client->Run("fstat", &m_ui);
    m_client->WaitTag();

    if (m_ui.m_e.Test())
    {
        return false;
    }

    strcpy(clientFile, m_ui.m_findedFile);
    if (*clientFile)
    {
        return true;
    }

    return false;
}

bool CPerforceSourceControl::IsFileManageable(const char* sFilename, bool bCheckFatal)
{
    if (!Reconnect())
    {
        return false;
    }

    bool bRet = false;
    bool fatal = false;

    if (m_bIsWorkOffline)
    {
        return false;
    }

    char fl[MAX_PATH];
    strcpy(fl, sFilename);
    //ConvertFileNameCS(fl, sFilename);

    char* argv[] = { fl };
    Run("fstat", 1, argv, true);

    AUTO_LOCK(g_cPerforceValues);

    m_ui.Init();
    m_client->SetArgv(1, argv);
    m_client->Run("fstat", &m_ui);
    m_client->WaitTag();

    if (bCheckFatal && m_ui.m_e.IsFatal())
    {
        fatal = true;
    }

    if (!fatal && !m_ui.m_e.IsError())
    {
        bRet = true;
    }

    m_ui.m_e.Clear();
    return bRet;
}

bool CPerforceSourceControl::IsFileExistsInDatabase(const char* sFilename)
{
    if (!Reconnect())
    {
        return false;
    }

    bool bRet = false;

    char fl[MAX_PATH];
    strcpy(fl, sFilename);
    char* argv[] = { fl };
    m_ui.Init();
    m_client->SetArgv(1, argv);
    m_client->Run("fstat", &m_ui);
    m_client->WaitTag();

    if (!m_ui.m_e.Test())
    {
        if (strcmp(m_ui.m_headAction, "delete"))
        {
            bRet = true;
        }
    }
    else
    {
        //  StrBuf errorMsg;
        //  m_ui.m_e.Fmt(&errorMsg);
    }
    m_ui.m_e.Clear();
    return bRet;
}

bool CPerforceSourceControl::IsFileCheckedOutByUser(const char* sFilename, bool* isByAnotherUser /*= 0*/, bool* isForAdd /*= 0*/, bool* isForMove /*= 0*/)
{
    if (!Reconnect())
    {
        return false;
    }

    bool bRet = false;

    char fl[MAX_PATH];
    strcpy(fl, sFilename);
    char* argv[] = { fl };
    m_ui.Init();
    m_client->SetArgv(1, argv);
    m_client->Run("fstat", &m_ui);
    m_client->WaitTag();

    bool isEdit = strcmp(m_ui.m_action, "edit") == 0;
    bool isAdd = strcmp(m_ui.m_action, "add") == 0;
    bool isMove = strcmp(m_ui.m_action, "move/add") == 0;

    if ((isEdit || isAdd) && !m_ui.m_e.Test())
    {
        bRet = true;
    }

    if (isForAdd)
    {
        *isForAdd = (isAdd && !m_ui.m_e.Test());
    }

    if (isForMove)
    {
        *isForMove = (isMove && !m_ui.m_e.Test());
    }

    if (isByAnotherUser)
    {
        *isByAnotherUser = false;
        if (!strcmp(m_ui.m_otherAction, "edit") && !m_ui.m_e.Test())
        {
            *isByAnotherUser = true;
        }
    }

    m_ui.m_e.Clear();
    return bRet;
}

bool CPerforceSourceControl::IsFileLatestVersion(const char* sFilename)
{
    if (!Reconnect())
    {
        return false;
    }

    bool bRet = false;

    char fl[MAX_PATH];
    strcpy(fl, sFilename);
    char* argv[] = { fl };
    m_ui.Init();
    m_client->SetArgv(1, argv);
    m_client->Run("fstat", &m_ui);
    m_client->WaitTag();

    if (m_ui.m_clientHasLatestRev[0] != 0)
    {
        bRet = true;
    }
    m_ui.m_e.Clear();
    return bRet;
}

uint32 CPerforceSourceControl::GetFileAttributesAndFileName(const char* filename, char* FullFileName)
{
    //  g_pSystem->GetILog()->Log("\n checking connection");

    if (!Reconnect())
    {
        return false;
    }

    if (FullFileName)
    {
        FullFileName[0] = 0;
    }

    CCryFile file;
    bool bCryFile = file.Open(filename, "rb");

    uint32 attributes = 0;
    QString fullPathName = Path::GamePathToFullPath(filename);

    char sFullFilename[ICryPak::g_nMaxPath];
    ConvertFileNameCS(sFullFilename, fullPathName.toUtf8().data());

    if (FullFileName)
    {
        strcpy(FullFileName, sFullFilename);
    }

    if (bCryFile && file.IsInPak())
    {
        //      g_pSystem->GetILog()->Log("\n file is in pak");
        attributes = SCC_FILE_ATTRIBUTE_READONLY | SCC_FILE_ATTRIBUTE_INPAK;

        if (IsFileManageable(sFullFilename) && IsFileExistsInDatabase(sFullFilename))
        {
            //          g_pSystem->GetILog()->Log("\n file is managable and also exists in the database");
            attributes |= SCC_FILE_ATTRIBUTE_MANAGED;
            bool isByAnotherUser;
            if (IsFileCheckedOutByUser(sFullFilename, &isByAnotherUser))
            {
                attributes |= SCC_FILE_ATTRIBUTE_CHECKEDOUT;
            }
            if (isByAnotherUser)
            {
                attributes |= SCC_FILE_ATTRIBUTE_BYANOTHER;
            }
            if (!IsFileLatestVersion(sFullFilename))
            {
                attributes |= SCC_FILE_ATTRIBUTE_NOTATHEAD;
            }
        }
    }
    else
    {
        DWORD dwAttrib = ::GetFileAttributes(sFullFilename);
        if (dwAttrib != INVALID_FILE_ATTRIBUTES)
        {
            //      g_pSystem->GetILog()->Log("\n we have valid file attributes");
            attributes = SCC_FILE_ATTRIBUTE_NORMAL;
            if (dwAttrib & FILE_ATTRIBUTE_READONLY)
            {
                attributes |= SCC_FILE_ATTRIBUTE_READONLY;
            }

            if (IsFileManageable(sFullFilename))
            {
                //          g_pSystem->GetILog()->Log("\n file is manageable");
                if (IsFileExistsInDatabase(sFullFilename))
                {
                    attributes |= SCC_FILE_ATTRIBUTE_MANAGED;
                    //              g_pSystem->GetILog()->Log("\n file exists in database");
                    bool isByAnotherUser;
                    bool isForAdd;
                    bool isForMove;
                    if (IsFileCheckedOutByUser(sFullFilename, &isByAnotherUser, &isForAdd, &isForMove))
                    {
                        //                  g_pSystem->GetILog()->Log("\n file is checked out");
                        attributes |= SCC_FILE_ATTRIBUTE_CHECKEDOUT;
                    }
                    if (isByAnotherUser)
                    {
                        //                  g_pSystem->GetILog()->Log("\n by another user");
                        attributes |= SCC_FILE_ATTRIBUTE_BYANOTHER;
                    }
                    if (isForAdd)
                    {
                        attributes |= SCC_FILE_ATTRIBUTE_ADD;
                    }
                    if (isForMove)
                    {
                        attributes |= SCC_FILE_ATTRIBUTE_MOVED;
                    }
                    if (!IsFileLatestVersion(sFullFilename))
                    {
                        attributes |= SCC_FILE_ATTRIBUTE_NOTATHEAD;
                    }
                }
                else
                {
                    if (*sFullFilename && sFullFilename[strlen(sFullFilename) - 1] == '\\')
                    {
                        attributes |= SCC_FILE_ATTRIBUTE_MANAGED | SCC_FILE_ATTRIBUTE_FOLDER;
                    }
                }
            }
        }
    }

    //  g_pSystem->GetILog()->Log("\n file has invalid file attributes");
    if (!attributes && !bCryFile)
    {
        if (IsFileManageable(sFullFilename))
        {
            if (IsFileExistsInDatabase(sFullFilename))
            {
                //              g_pSystem->GetILog()->Log("\n file is managable and exists in the database");
                attributes = SCC_FILE_ATTRIBUTE_MANAGED | SCC_FILE_ATTRIBUTE_NORMAL | SCC_FILE_ATTRIBUTE_READONLY;
                bool isByAnotherUser;
                if (IsFileCheckedOutByUser(sFullFilename, &isByAnotherUser))
                {
                    attributes |= SCC_FILE_ATTRIBUTE_CHECKEDOUT;
                }
                if (isByAnotherUser)
                {
                    attributes |= SCC_FILE_ATTRIBUTE_BYANOTHER;
                }
                if (!IsFileLatestVersion(sFullFilename))
                {
                    attributes |= SCC_FILE_ATTRIBUTE_NOTATHEAD;
                }
            }
            else
            {
                if (*sFullFilename && sFullFilename[strlen(sFullFilename) - 1] == '\\')
                {
                    attributes |= SCC_FILE_ATTRIBUTE_MANAGED | SCC_FILE_ATTRIBUTE_FOLDER;
                }
            }
        }
    }

    if (!attributes && FullFileName && IsFolder(filename, FullFileName))
    {
        attributes = SCC_FILE_ATTRIBUTE_FOLDER;
    }

    if (FullFileName && (attributes & SCC_FILE_ATTRIBUTE_FOLDER))
    {
        strcat(FullFileName, "...");
    }

    return attributes ? attributes : SCC_FILE_ATTRIBUTE_INVALID;
}

bool CPerforceSourceControl::IsFolder(const char* filename, char* FullFileName)
{
    Reconnect();

    bool bFolder = false;

    char sFullFilename[ICryPak::g_nMaxPath];
    ConvertFileNameCS(sFullFilename, filename);

    uint32 attr = ::GetFileAttributes(sFullFilename);
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        if (*sFullFilename && sFullFilename[strlen(sFullFilename) - 1] == '\\')
        {
            bFolder = true;
        }
        else
        {
            return false;
        }
    }
    else
    if ((attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        bFolder = true;
    }

    if (bFolder && FullFileName)
    {
        strcpy(FullFileName, sFullFilename);
    }

    return bFolder;
}

bool CPerforceSourceControl::CheckOut(const char* filename, int nFlags, char* changelistId)
{
    AUTO_LOCK(g_cPerforceValues);
    if (!Reconnect())
    {
        return false;
    }

    bool bRet = false;

    char FullFileName[MAX_PATH];

    QString files = filename;
    for (auto file : files.split(";", QString::SkipEmptyParts))
    {
        file = file.trimmed();
        if (file.isEmpty())
        {
            continue;
        }
        uint32 attrib = GetFileAttributesAndFileName(file.toUtf8().data(), FullFileName);

        if ((attrib & SCC_FILE_ATTRIBUTE_FOLDER) || ((attrib & SCC_FILE_ATTRIBUTE_MANAGED) && !(attrib & SCC_FILE_ATTRIBUTE_CHECKEDOUT)))
        {
            if (nFlags & ADD_CHANGELIST && changelistId)
            {
                const char* argv[] = { "-c", changelistId, FullFileName };
                m_client->SetArgv(3, const_cast<char**>(argv));
                m_client->Run("edit", &m_ui);
            }
            else
            {
                char* argv[] = { FullFileName };
                m_client->SetArgv(1, argv);
                m_client->Run("edit", &m_ui);
            }

            if (!m_ui.m_e.Test())
            {
                bRet = true;
            }
        }
    }
    return bRet;
}

bool CPerforceSourceControl::UndoCheckOut(const char* filename, int nFlags)
{
    AUTO_LOCK(g_cPerforceValues);
    if (!Reconnect())
    {
        return false;
    }

    bool bRet = false;

    char FullFileName[MAX_PATH];

    QString files = filename;
    for (auto file : files.split(";", QString::SkipEmptyParts))
    {
        file = file.trimmed();
        if (file.isEmpty())
        {
            continue;
        }
        uint32 attrib = GetFileAttributesAndFileName(file.toUtf8().data(), FullFileName);

        if ((attrib & SCC_FILE_ATTRIBUTE_FOLDER) || ((attrib & SCC_FILE_ATTRIBUTE_MANAGED) && (attrib & SCC_FILE_ATTRIBUTE_CHECKEDOUT)))
        {
            char* argv[] = { FullFileName };
            m_client->SetArgv(1, argv);
            m_client->Run("revert", &m_ui);
            if (!m_ui.m_e.Test())
            {
                bRet = true;
            }
        }
    }
    return true; // always return true to avoid error message box on folder operation
}

bool CPerforceSourceControl::Rename(const char* filename, const char* newname, const char* desc, int nFlags)
{
    AUTO_LOCK(g_cPerforceValues);
    if (!Reconnect())
    {
        return false;
    }

    bool bRet = false;
    char FullFileName[MAX_PATH];
    uint32 attrib = GetFileAttributesAndFileName(filename, FullFileName);

    if (!(attrib & SCC_FILE_ATTRIBUTE_MANAGED))
    {
        return true;
    }

    if ((attrib & SCC_FILE_ATTRIBUTE_CHECKEDOUT) && ((nFlags & RENAME_WITHOUT_REVERT) == 0))
    {
        UndoCheckOut(filename, 0);
    }

    //if(m_pIntegrator->FileCheckedOutByAnotherUser(FullFileName))
    //return false;

    char FullNewFileName[MAX_PATH];
    strcpy(FullNewFileName, newname);

    {
        char* argv[] = { FullFileName, FullNewFileName };
        m_client->SetArgv(2, argv);
        m_client->Run("move", &m_ui);
    }

    if ((nFlags & RENAME_WITHOUT_SUBMIT) == 0)
    {
        if (desc)
        {
            strcpy(m_ui.m_desc, desc);
        }
        {
            char* argv[] = { FullFileName };
            m_client->SetArgv(1, argv);
            m_client->Run("submit", &m_ui);
        }
    }

    if (!m_ui.m_e.Test())
    {
        bRet = true;
    }

    //p4 integrate source_file target_file
    //p4 delete source_file
    //p4 submit

    return bRet;
}

void CPerforceSourceControl::ShowSettings()
{
    AUTO_LOCK(g_cPerforceValues);
    bool workOffline = (m_bIsWorkOffline && !m_bIsWorkOfflineBecauseOfConnectionLoss);

    if (PerforceConnection::OpenPasswordDlg())
    {
        m_bIsWorkOfflineBecauseOfConnectionLoss = false;
        bool onlineMode = false;
        {
            using SCRequestBus = AzToolsFramework::SourceControlConnectionRequestBus;
            SCRequestBus::BroadcastResult(onlineMode, &SCRequestBus::Events::IsActive);
        }
        m_bIsWorkOffline = !onlineMode;

        // reset the connection!
        if ((!m_bIsWorkOfflineBecauseOfConnectionLoss) && onlineMode)
        {
            Connect();
        }

        CheckConnectionAndNotifyListeners();
    }
}

const char* CPerforceSourceControl::GetErrorByGenericCode(int nGeneric)
{
    switch (nGeneric)
    {
    case EV_USAGE:
        return "Request is not consistent with documentation or cannot support a server version";
    case EV_UNKNOWN:
        return "Using unknown entity";
    case EV_CONTEXT:
        return "Using entity in wrong context";
    case EV_ILLEGAL:
        return "Trying to do something you can't";
    case EV_NOTYET:
        return "Something must be corrected first";
    case EV_PROTECT:
        return "Operation was prevented by protection level";

    // No fault at all
    case EV_EMPTY:
        return "Action returned empty results";

    // not the fault of the user
    case EV_FAULT:
        return "Inexplicable program fault";
    case EV_CLIENT:
        return "Client side program errors";
    case EV_ADMIN:
        return "Server administrative action required";
    case EV_CONFIG:
        return "Client configuration is inadequate";
    case EV_UPGRADE:
        return "Client or server is too old to interact";
    case EV_COMM:
        return "Communication error";
    case EV_TOOBIG:
        return "File is too big";
    }

    return "Undefined";
}

bool CPerforceSourceControl::Run(const char* func, int nArgs, char* argv[], bool bOnlyFatal)
{
    for (int x = 0; x < nArgs; ++x)
    {
        if (argv[x][0] == '\0')
        {
            return false;
        }
    }

    if (!Reconnect())
    {
        return false;
    }

    bool bRet = false;

    AUTO_LOCK(g_cPerforceValues);

    m_ui.Init();
    m_client->SetArgv(nArgs, argv);
    m_client->Run(func, &m_ui);
    m_client->WaitTag();

#if 0 // connection debug
    for (int argid = 0; argid < nArgs; argid++)
    {
        g_pSystem->GetILog()->Log("\n arg %d : %s", argid, argv[argid]);
    }
#endif // connection debug

    if (bOnlyFatal)
    {
        if (!m_ui.m_e.IsFatal())
        {
            bRet = true;
        }
    }
    else
    {
        if (!m_ui.m_e.Test())
        {
            bRet = true;
        }
    }

    if (m_ui.m_e.GetSeverity() == E_FAILED || m_ui.m_e.GetSeverity() == E_FATAL)
    {
        static int nGenericPrev = 0;
        const int nGeneric = m_ui.m_e.GetGeneric();

        if (IsSomeTimePassed())
        {
            nGenericPrev = 0;
        }

        if (nGenericPrev != nGeneric)
        {
            if ((!bOnlyFatal) || (m_ui.m_e.GetSeverity() == E_FATAL))
            {
                GetIEditor()->GetSystem()->GetILog()->LogError("Perforce plugin: %s", GetErrorByGenericCode(nGeneric));

                StrBuf  m;
                m_ui.m_e.Fmt(&m);
                GetIEditor()->GetSystem()->GetILog()->LogError("Perforce plugin: %s", m.Text());
            }

            if (!m_client || m_client->Dropped())
            {
                IMainStatusBar* statusBar = GetIEditor()->GetMainStatusBar();
                const QPixmap p4ErrorIcon = m_p4ErrorIcon; // Playing safe and copying instead of capturing 'this', no idea about its lifetime.
                QTimer::singleShot(0, qApp, [statusBar, p4ErrorIcon, nGeneric] {
                    // SetItem must be called on the main thread, otherwise we get "Cannot send events to objects owned by a different thread" and asserts in debug mode
                    // By using a singleShot with context object it calls the lambda on the thread of the context
                    statusBar->SetItem("source_control", "", CPerforceSourceControl::GetErrorByGenericCode(nGeneric), p4ErrorIcon);
                });
            }

            nGenericPrev = nGeneric;
        }
    }

    m_ui.m_e.Clear();
    return bRet;
}

void CPerforceSourceControl::SetSourceControlState(SourceControlState state)
{
    AUTO_LOCK(g_cPerforceValues);
    switch (state)
    {
    case SourceControlState::Disabled:
    {
        m_bIsWorkOffline = true;
        m_bIsWorkOfflineBecauseOfConnectionLoss = true;
        m_configurationInvalid = false;

        QString disableMsg("Perforce plugin disabled");
        GetIEditor()->GetSystem()->GetILog()->Log(disableMsg.toUtf8().data());
        GetIEditor()->GetMainStatusBar()->SetItem("source_control", "", disableMsg.toUtf8().data(), m_p4ErrorIcon);
        break;
    }
    case SourceControlState::Active:
        m_bIsWorkOffline = false;
        m_bIsWorkOfflineBecauseOfConnectionLoss = false;
        m_configurationInvalid = false;
        CheckConnectionAndNotifyListeners();
        break;
        
    case SourceControlState::ConfigurationInvalid:
        m_bIsWorkOffline = false;
        m_bIsWorkOfflineBecauseOfConnectionLoss = false;
        m_configurationInvalid = true;

        GetIEditor()->GetSystem()->GetILog()->Log("Perforce settings invalid!");
        GetIEditor()->GetMainStatusBar()->SetItem("source_control", "", "Perforce configuration invalid", m_p4ErrorIcon);
        break;

    default:
        break;
    }
}

CPerforceSourceControl::ConnectivityState CPerforceSourceControl::GetConnectivityState()
{
    ConnectivityState state = ConnectivityState::Disconnected;
    if (!m_bIsWorkOffline)
    {
        state = ConnectivityState::Connected;
    }
    else if (m_configurationInvalid)
    {
        state = ConnectivityState::BadConfiguration;
    }

    return state;
}

bool CPerforceSourceControl::CheckConnectionAndNotifyListeners()
{
    AUTO_LOCK(g_cPerforceValues);

    m_ui.m_bIsClientUnknown = false;
    bool bRet = false;

    bool wasWorkOffline = m_bIsWorkOffline;
    if (!m_bIsWorkOffline)
    {
        bRet = Run("info", 0, 0);
    }

    if (m_bIsWorkOffline || m_configurationInvalid)
    {
        if (!wasWorkOffline)
        {
            // only log once when transitioning offline.
            GetIEditor()->GetSystem()->GetILog()->LogError("Perforce plugin: Perforce is offline");
        }
        auto statusBar = GetIEditor()->GetMainStatusBar();
        if (statusBar)
        {
            const char* tooltip = m_bIsWorkOffline ? "Perforce is offline" : "Perforce configuration invalid";
            statusBar->SetItem("source_control", "", tooltip, m_p4ErrorIcon);
        }
        return false;
    }

    if (bRet)
    {
        GetIEditor()->GetMainStatusBar()->SetItem("source_control", "", "Perforce is online", m_p4Icon);
    }

    return bRet;
}

bool CPerforceSourceControl::GetLatestVersion(const char* filename, int nFlags)
{
    if (!Reconnect())
    {
        return false;
    }

    bool bRet = false;
    char FullFileName[MAX_PATH];

    AUTO_LOCK(g_cPerforceValues);

    QString files = filename;
    for (auto file : files.split(";", QString::SkipEmptyParts))
    {
        file = file.trimmed();
        if (file.isEmpty())
        {
            continue;
        }
        uint32 attrib = GetFileAttributesAndFileName(file.toUtf8().data(), FullFileName);

        if (!(attrib & SCC_FILE_ATTRIBUTE_MANAGED))
        {
            continue;
        }

        if ((attrib & SCC_FILE_ATTRIBUTE_CHECKEDOUT) && ((nFlags & GETLATEST_REVERT) == 0))
        {
            continue;
        }

        if (nFlags & GETLATEST_REVERT)
        {
            char* argv[] = { FullFileName };
            m_client->SetArgv(1, argv);
            m_client->Run("revert", &m_ui);
        }

        if (nFlags & GETLATEST_ONLY_CHECK)
        {
            if (attrib & SCC_FILE_ATTRIBUTE_FOLDER)
            {
                bRet = true;
            }
            else
            {
                bRet = IsFileLatestVersion(FullFileName);
            }
        }
        else
        {
            const char* argv[] = { "-f", FullFileName };
            m_client->SetArgv(2, const_cast<char**>(argv));
            m_client->Run("sync", &m_ui);
            bRet = true;
        }
    }

    return bRet;
}

bool CPerforceSourceControl::IsSomeTimePassed()
{
    AUTO_LOCK(g_cPerforceValues);
    const DWORD dwSomeTime = 10000; // 10 sec
    static DWORD dwLastTime = 0;
    DWORD dwCurTime = GetTickCount();

    if (dwCurTime - dwLastTime > dwSomeTime)
    {
        dwLastTime = dwCurTime;
        return true;
    }

    return false;
}

