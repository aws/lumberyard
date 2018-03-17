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

#include "pch.h"
#include "AnimationCompressionManager.h"
#include "IBackgroundTaskManager.h"
#include "CharacterTool/CafCompressionHelper.h"
#include "IEditorFileMonitor.h"
#include <StringUtils.h>
#include <CryFile.h>
#include <CryPath.h>
#include <ICryAnimation.h>
#include <Util/PathUtil.h> // for GetEditingGameDataFolder

// ---------------------------------------------------------------------------

static bool IsFileExists(const char* filename)
{
    string path = PathUtil::Make(Path::GetEditingGameDataFolder().c_str(), filename);
    DWORD fileAttributes = GetFileAttributes(path.c_str());
    return fileAttributes != INVALID_FILE_ATTRIBUTES;
}

static bool DeleteFileInGameFolder(const char* filename)
{
    string path = PathUtil::Make(Path::GetEditingGameDataFolder().c_str(), filename);
    DWORD fileAttributes = GetFileAttributes(path.c_str());
    if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }
    return QFile::remove(path.c_str());
}

// ---------------------------------------------------------------------------

class CAnimationCompressionManager::CBackgroundTaskCompressCAF
    : public IBackgroundTask
{
public:
    CBackgroundTaskCompressCAF(CAnimationCompressionManager* pManager, const char* animationName)
        : m_animationName(animationName)
        , m_pManager(pManager)
        , m_bSucceed(false)
    {
        m_description = "Compression of '";
        m_description += m_animationName;
        m_description += "'";
    }

    ETaskResult Work() override
    {
        bool failReported = false;
        if (!CafCompressionHelper::CompressAnimation(m_animationName.c_str(), m_errorMessage, &failReported))
        {
            if (failReported)
            {
                SetFailReported();
            }
            return eTaskResult_Failed;
        }

        m_bSucceed = true;
        return eTaskResult_Completed;
    }

    void Finalize() override
    {
        if (m_bSucceed)
        {
            m_pManager->OnCAFCompressed(this);
        }
    }

    void Delete() override { delete this; }

    const char* Description() const override { return m_description.c_str(); }
    const char* ErrorMessage() const override { return m_errorMessage.c_str(); }

    const char* AnimationName() const{ return m_animationName.c_str(); }

private:
    string m_animationName;
    string m_description;
    string m_errorMessage;
    CAnimationCompressionManager* m_pManager;
    bool m_bSucceed;
};

// ---------------------------------------------------------------------------

class CAnimationCompressionManager::CBackgroundTaskUpdateLocalAnimations
    : public IBackgroundTask
{
public:
    CBackgroundTaskUpdateLocalAnimations(CAnimationCompressionManager* pManager)
        : m_pManager(pManager)
    {
    }

    const char* Description() const override { return "Update Local Animations"; }

    ETaskResult Work() override
    {
        if (m_fileList.empty())
        {
            std::vector< string > filenames;
            const char* const filePattern = "*.i_caf";

            SDirectoryEnumeratorHelper dirHelper;
            dirHelper.ScanDirectoryRecursive(Path::GetEditingGameDataFolder().c_str(), "", filePattern, filenames);

            m_fileList.reserve(filenames.size());
            for (size_t i = 0; i < filenames.size(); ++i)
            {
                string animationName = PathUtil::ReplaceExtension(filenames[i].c_str(), "caf");
                if (CafCompressionHelper::CheckIfNeedUpdate(animationName.c_str()))
                {
                    m_fileList.push_back(animationName);
                }
            }
            m_totalFileCount = m_fileList.size();
            return m_totalFileCount > 0 ? eTaskResult_Resume : eTaskResult_Completed;
        }

        string animationName = m_fileList.back();
        m_fileList.pop_back();

        animationName.MakeLower();
        if (CafCompressionHelper::CheckIfNeedUpdate(animationName.c_str()))
        {
            int index = m_totalFileCount - m_fileList.size();
            gEnv->pLog->Log("Compressing Animation %i/%i: %s", index, m_totalFileCount, animationName.c_str());
            GetIEditor()->GetBackgroundTaskManager()->AddTask(new CBackgroundTaskCompressCAF(m_pManager, animationName.c_str()), eTaskPriority_FileUpdate, eTaskThreadMask_IO);
        }

        SetProgress(float(m_fileList.size()) / m_totalFileCount);

        return m_fileList.empty() ? eTaskResult_Completed : eTaskResult_Resume;
    }

    void Finalize() override
    {
        if (m_pManager->m_pRescanTask == this)
        {
            m_pManager->m_pRescanTask = 0;
        }
    }

    void Delete() override { delete this; }

private:
    CAnimationCompressionManager* m_pManager;
    std::vector<string> m_fileList;
    int m_totalFileCount;
    ;
};

// ---------------------------------------------------------------------------


class CAnimationCompressionManager::CBackgroundTaskCleanUpAnimations
    : public IBackgroundTask
{
public:
    const char* Description() const override { return "Clean up compiled animations"; }

    ETaskResult Work() override
    {
        std::vector< string > filenames;
        const char* const filePattern = "*.caf";

        SDirectoryEnumeratorHelper dirHelper;
        dirHelper.ScanDirectoryRecursive("", "", filePattern, filenames);

        std::vector< string> filesToRemove;
        filesToRemove.reserve(filenames.size());
        for (size_t i = 0; i < filenames.size(); ++i)
        {
            string iCafName = PathUtil::ReplaceExtension(filenames[i].c_str(), "i_caf");
            string animSettingsName = PathUtil::ReplaceExtension(filenames[i].c_str(), "animsettings");
            if (!IsFileExists(animSettingsName.c_str()) && !IsFileExists(iCafName.c_str()))
            {
                filesToRemove.push_back(filenames[i]);
            }
        }

        size_t numFiles = filesToRemove.size();
        for (size_t i = 0; i < numFiles; ++i)
        {
            SetProgress(float(i) / numFiles);
            const char* filename = filesToRemove[i];
            if (IsFileExists(filename))
            {
                gEnv->pLog->Log("Cleaning up compiled animation: %s", filename);
                if (!DeleteFileInGameFolder(filename))
                {
                    gEnv->pLog->LogError("Failed to delete file: %s", filename);
                }
            }
        }

        return eTaskResult_Completed;
    }
};

// ---------------------------------------------------------------------------

class CAnimationCompressionManager::CBackgroundTaskReloadCHRPARAMS
    : public IBackgroundTask
{
public:
    CBackgroundTaskReloadCHRPARAMS(CAnimationCompressionManager* pManager)
        : m_pManager(pManager) {}
    ETaskResult Work() override { return eTaskResult_Completed; }
    const char* Description() const override { return "Reload CHRPARAMS"; }

    void Finalize() override
    {
        gEnv->pCharacterManager->ReloadAllCHRPARAMS();
        if (m_pManager)
        {
            m_pManager->OnReloadCHRPARAMSComplete(this);
        }
    }

    void Delete() override { delete this; }

    CAnimationCompressionManager* m_pManager;
};

// ---------------------------------------------------------------------------

CAnimationCompressionManager::CAnimationCompressionManager()
    : m_pRescanTask(0)
    , m_pReloadCHRPARAMSTask(0)
    , m_enabled(false)
{
    GetIEditor()->RegisterNotifyListener(this);
}

CAnimationCompressionManager::~CAnimationCompressionManager()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

void CAnimationCompressionManager::Start()
{
    // Enable animation monitoring only for projects that
    // transitioned to new animation file structure
    m_enabled = gEnv->pCryPak->IsFileExist("Animations/skeletonList.xml");
    if (m_enabled)
    {
        if (!GetIEditor()->IsInConsolewMode())
        {
            UpdateLocalAnimations();
        }
    }
}

void CAnimationCompressionManager::OnEditorNotifyEvent(EEditorNotifyEvent ev)
{
    if (ev == eNotify_OnInit)
    {
        Start();
    }
}

void CAnimationCompressionManager::UpdateLocalAnimations()
{
    // Disabled for now as VDG team are loosing their working files.
    // GetIEditor()->GetBackgroundTaskManager()->AddTask(new CBackgroundTaskCleanUpAnimations(), eTaskPriority_BackgroundScan);

    if (m_pRescanTask)
    {
        m_pRescanTask->Cancel();
    }
    m_pRescanTask = new CBackgroundTaskUpdateLocalAnimations(this);
    GetIEditor()->GetBackgroundTaskManager()->AddTask(m_pRescanTask, eTaskPriority_BackgroundScan, eTaskThreadMask_IO);
}

bool CAnimationCompressionManager::IsEnabled() const
{
    return m_enabled;
}



void CAnimationCompressionManager::OnCAFCompressed(CBackgroundTaskCompressCAF* pTask)
{
    TAnimationMap::iterator it = m_animationsInWork.find(pTask->AnimationName());
    if (it != m_animationsInWork.end())
    {
        for (size_t i = 0; i < m_canceledAnimations.size(); ++i)
        {
            if (m_canceledAnimations[i].pTask == pTask)
            {
                m_canceledAnimations.erase(m_canceledAnimations.begin() + i);
                --i;
            }
        }
        m_animationsInWork.erase(it);
    }

    if (!pTask->IsCanceled())
    {
        ICharacterManager* pCharacterManager = GetISystem()->GetIAnimationSystem();
        if (pCharacterManager->ReloadCAF(pTask->AnimationName()) == CR_RELOAD_GAH_NOT_IN_ARRAY)
        {
            if (!m_pReloadCHRPARAMSTask)
            {
                m_pReloadCHRPARAMSTask = new CBackgroundTaskReloadCHRPARAMS(this);
                GetIEditor()->GetBackgroundTaskManager()->AddTask(m_pReloadCHRPARAMSTask, eTaskPriority_FileUpdateFinal, eTaskThreadMask_IO);
            }
        }
    }
}

void CAnimationCompressionManager::OnReloadCHRPARAMSComplete(CBackgroundTaskReloadCHRPARAMS* pTask)
{
    if (pTask == m_pReloadCHRPARAMSTask)
    {
        m_pReloadCHRPARAMSTask = 0;
    }
}
