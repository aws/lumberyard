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
#include "EditorFileMonitor.h"
#include "IResourceCompilerHelper.h" // used in RecompileColladaFile
#include "GameEngine.h"
#include "Include/IAnimationCompressionManager.h"
#include <StringUtils.h>
#include <QDir>

//////////////////////////////////////////////////////////////////////////
CEditorFileMonitor::CEditorFileMonitor()
{
    GetIEditor()->RegisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
CEditorFileMonitor::~CEditorFileMonitor()
{
    CFileChangeMonitor::DeleteInstance();
}

//////////////////////////////////////////////////////////////////////////
void CEditorFileMonitor::OnEditorNotifyEvent(EEditorNotifyEvent ev)
{
    if (ev == eNotify_OnInit)
    {
        // Setup file change monitoring
        gEnv->pSystem->SetIFileChangeMonitor(this);

        // We don't want the file monitor to be enabled while
        // in console mode...
        if (!GetIEditor()->IsInConsolewMode())
        {
            MonitorDirectories();
        }

        CFileChangeMonitor::Instance()->Subscribe(this);
    }
    else if (ev == eNotify_OnQuit)
    {
        gEnv->pSystem->SetIFileChangeMonitor(NULL);
        CFileChangeMonitor::Instance()->StopMonitor();
        GetIEditor()->UnregisterNotifyListener(this);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CEditorFileMonitor::RegisterListener(IFileChangeListener* pListener, const char* sMonitorItem)
{
    return RegisterListener(pListener, sMonitorItem, "*");
}


//////////////////////////////////////////////////////////////////////////
static string CanonicalizePath(const char* path)
{
    auto canon = QFileInfo(path).canonicalFilePath();
    return canon.isEmpty() ? string(path) : string(canon.toUtf8());
}

//////////////////////////////////////////////////////////////////////////
bool CEditorFileMonitor::RegisterListener(IFileChangeListener* pListener, const char* sFolderRelativeToGame, const char* sExtension)
{
    bool success = true;

    string gameFolder = Path::GetEditingGameDataFolder().c_str();
    const char* modDirectory = gameFolder.c_str();
    string naivePath;
    CFileChangeMonitor* fileChangeMonitor = CFileChangeMonitor::Instance();
    AZ_Assert(fileChangeMonitor, "CFileChangeMonitor singleton missing.");

    int modIndex = 0;
    for (;; )
    {
        naivePath += modDirectory;
        // Append slash in preparation for appending the second part.
        naivePath = PathUtil::AddSlash(naivePath);
        naivePath += sFolderRelativeToGame;
        naivePath.replace('/', '\\');

        // Remove the final slash if the given item is a folder so the file change monitor correctly picks up on it.
        naivePath = PathUtil::RemoveSlash(naivePath);

        string canonicalizedPath = CanonicalizePath(naivePath.c_str());

        if (fileChangeMonitor->IsDirectory(canonicalizedPath.c_str()) || fileChangeMonitor->IsFile(canonicalizedPath.c_str()))
        {
            if (fileChangeMonitor->MonitorItem(canonicalizedPath.c_str()))
            {
                m_vecFileChangeCallbacks.push_back(SFileChangeCallback(pListener, sFolderRelativeToGame, sExtension));
            }
            else
            {
                CryLogAlways("File Monitor: [%s] not found outside of PAK files. Monitoring disabled for this item", sFolderRelativeToGame);
                success = false;
            }
        }
        modDirectory = gEnv->pCryPak->GetMod(modIndex);
        ++modIndex;

        if (modDirectory != 0)
        {
            // Clear the naivePath and set it to the appropriate default for mods.
            naivePath = QString(GetIEditor()->GetMasterCDFolder()).toUtf8().data();
            naivePath = PathUtil::AddSlash(naivePath);
        }
        else
        {
            break;
        }
    }
    ;

    return success;
}

bool CEditorFileMonitor::UnregisterListener(IFileChangeListener* pListener)
{
    bool bRet = false;

    // Note that we remove the listener, but we don't currently remove the monitored item
    // from the file monitor. This is fine, but inefficient

    std::vector<SFileChangeCallback>::iterator iter = m_vecFileChangeCallbacks.begin();
    while (iter != m_vecFileChangeCallbacks.end())
    {
        if (iter->pListener == pListener)
        {
            iter = m_vecFileChangeCallbacks.erase(iter);
            bRet = true;
        }
        else
        {
            iter++;
        }
    }

    return bRet;
}

//////////////////////////////////////////////////////////////////////////
void CEditorFileMonitor::MonitorDirectories()
{
    QString masterCD = Path::AddPathSlash(QString(GetIEditor()->GetMasterCDFolder()));

    // NOTE: Instead of monitoring each sub-directory we monitor the whole root
    // folder. This is needed since if the sub-directory does not exist when
    // we register it it will never get monitored properly.
    CFileChangeMonitor::Instance()->MonitorItem(QStringLiteral("%1/%2/").arg(masterCD).arg(QString::fromLatin1(Path::GetEditingGameDataFolder().c_str())));

    // Add mod paths too
    for (int index = 0;; index++)
    {
        const char* sModPath = gEnv->pCryPak->GetMod(index);
        if (!sModPath)
        {
            break;
        }
        CFileChangeMonitor::Instance()->MonitorItem(QStringLiteral("%1/%2/").arg(masterCD).arg(QString::fromLatin1(sModPath)));
    }

    // Add editor directory for scripts
    CFileChangeMonitor::Instance()->MonitorItem(QStringLiteral("%1/Editor/").arg(masterCD));
}

//////////////////////////////////////////////////////////////////////////
static bool IsFilenameEndsWithDotDaeDotZip(const char* fln)
{
    size_t len = strlen(fln);
    if (len < 8)
    {
        return false;
    }

    if (_stricmp(fln + len - 8, ".dae.zip") == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////
static bool RecompileColladaFile(const char* path)
{
    string pathWithGameFolder = PathUtil::ToUnixPath(PathUtil::AddSlash(Path::GetEditingGameDataFolder().c_str())) + string(path);

    if ((!gEnv) || (!gEnv->pResourceCompilerHelper))
    {
        return false;  // if we have no compiling capability do not say that you succeeded!
    }
    if (gEnv->pResourceCompilerHelper->CallResourceCompiler(
            pathWithGameFolder.c_str(), "/refresh", NULL, false, IResourceCompilerHelper::eRcExePath_currentFolder, true, true, L".")
        != IResourceCompilerHelper::eRcCallResult_success)
    {
        return true;
    }
    else
    {
        return false;
    }
}


//////////////////////////////////////////////////////////////////////////
const char* GetPathRelativeToModFolder(const char* pathRelativeToGameFolder)
{
    if (pathRelativeToGameFolder[0] == '\0')
    {
        return pathRelativeToGameFolder;
    }

    string gameFolder = Path::GetEditingGameDataFolder().c_str();
    string modLocation;
    const char* modFolder = gameFolder.c_str();
    int modIndex = 0;
    do
    {
        if (_strnicmp(modFolder, pathRelativeToGameFolder, strlen(modFolder)) == 0)
        {
            const char* result = pathRelativeToGameFolder + strlen(modFolder);
            if (*result == '\\' || *result == '/')
            {
                ++result;
            }
            return result;
        }

        modFolder = gEnv->pCryPak->GetMod(modIndex);
        ++modIndex;
    } while (modFolder != 0);

    return "";
}

QString RemoveGameName(const QString &filename)
{
    // Remove first part of path.  File coming in has the game name included
    // eg (SamplesProject/Animations/Chicken/anim_chicken_flapping.i_caf)->(Animations/Chicken/anim_chicken_flapping.i_caf)

    int indexOfFirstSlash = filename.indexOf('/');
    int indexOfFirstBackSlash = filename.indexOf('\\');
    if (indexOfFirstSlash >= 0)
    {
        if (indexOfFirstBackSlash >= 0 && indexOfFirstBackSlash < indexOfFirstSlash)
        {
            indexOfFirstSlash = indexOfFirstBackSlash;
        }
    }
    else
    {
        indexOfFirstSlash = indexOfFirstBackSlash;
    }
    return filename.mid(indexOfFirstSlash + 1);
}
///////////////////////////////////////////////////////////////////////////

// Called when file monitor message is received
void CEditorFileMonitor::OnFileMonitorChange(const SFileChangeInfo& rChange)
{
    CCryEditApp* app = CCryEditApp::instance();
    if (app == NULL || app->IsExiting())
    {
        return;
    }

    // skip folders!
    if (QFileInfo(rChange.filename).isDir())
    {
        return;
    }

    // Process updated file.
    // Make file relative to MasterCD folder.
    QString filename = rChange.filename;

    // Remove game directory if present in path.
    const QString rootPath =
        QDir::fromNativeSeparators(QString::fromLatin1(Path::GetEditingRootFolder().c_str()));
    if (filename.startsWith(rootPath, Qt::CaseInsensitive))
    {
        filename = filename.right(filename.length() - rootPath.length());
    }

    // Make sure there is no leading slash
    if (!filename.isEmpty() && (filename[0] == '\\' || filename[0] == '/'))
    {
        filename = filename.mid(1);
    }

    if (!filename.isEmpty())
    {
        //remove game name. Make it relative to the game folder
        const QString filenameRelGame = RemoveGameName(filename);
        const int extIndex = filename.lastIndexOf('.');
        const QString ext = filename.right(filename.length() - 1 - extIndex);

        // Check for File Monitor callback
        std::vector<SFileChangeCallback>::iterator iter;
        for (iter = m_vecFileChangeCallbacks.begin(); iter != m_vecFileChangeCallbacks.end(); ++iter)
        {
            SFileChangeCallback& sCallback = *iter;

            // We compare against length of callback string, so we get directory matches as well as full filenames
            if (sCallback.pListener)
            {
                if (sCallback.extension == "*" || ext.compare(sCallback.extension, Qt::CaseInsensitive) == 0)
                {
                    if (filenameRelGame.compare(sCallback.item, Qt::CaseInsensitive) == 0)
                    {
                        sCallback.pListener->OnFileChange(qPrintable(filenameRelGame), IFileChangeListener::EChangeType(rChange.changeType));
                    }
                    else if (filename.compare(sCallback.item, Qt::CaseInsensitive) == 0)
                    {
                        sCallback.pListener->OnFileChange(qPrintable(filename), IFileChangeListener::EChangeType(rChange.changeType));
                    }
                }
            }
        }

        //TODO:  have all these file types encapsulated in some IFileChangeFileTypeHandler to deal with each of them in a more generic way
        bool isCAF = ext.compare(QStringLiteral("caf"), Qt::CaseInsensitive) == 0;
        bool isLMG = ext.compare(QStringLiteral("lmg"), Qt::CaseInsensitive) == 0 || ext.compare("bspace", Qt::CaseInsensitive) == 0 || ext.compare("comb", Qt::CaseInsensitive) == 0;

        bool isExportLog = ext.compare(QStringLiteral("exportlog"), Qt::CaseInsensitive) == 0;
        bool isRCDone = ext.compare(QStringLiteral("rcdone"), Qt::CaseInsensitive) == 0;
        bool isCOLLADA = ext.compare(QStringLiteral("dae"), Qt::CaseInsensitive) == 0 || IsFilenameEndsWithDotDaeDotZip(qPrintable(filename));

        if (isCOLLADA)
        {
            // Make a corresponding .cgf path.
            QString cgfFileName = filename.replace(extIndex + 1, ext.length(), QStringLiteral("cgf"));
            IStatObj* pStatObjectToReload = GetIEditor()->Get3DEngine()->FindStatObjectByFilename(qPrintable(cgfFileName));

            // If the corresponding .cgf file exists, recompile the changed COLLADA file.
            if (pStatObjectToReload)
            {
                CryLog("Recompile DAE file: %s", qPrintable(filename));
                RecompileColladaFile(qPrintable(filename));
            }
        }
        else
        {
            ICharacterManager* pICharacterManager = GetISystem()->GetIAnimationSystem();
            stack_string strPath = qPrintable(filenameRelGame);
            CryStringUtils::UnifyFilePath(strPath);

            if (isLMG)
            {
                CryLog("Reload blendspace file: %s", (LPCTSTR)strPath);
                pICharacterManager->ReloadLMG(strPath.c_str());
            }
        }
        // Set this flag to make sure that the viewport will update at least once,
        // so that the changes will be shown, even if the app does not have focus.
        CCryEditApp::instance()->ForceNextIdleProcessing();
    }
}
