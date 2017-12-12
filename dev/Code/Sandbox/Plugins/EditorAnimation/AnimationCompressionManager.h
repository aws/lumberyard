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

#pragma once

#include <IBackgroundTaskManager.h>
#include <IAnimationCompressionManager.h>
#include <IEditor.h>
#include <IFileChangeMonitor.h>

class CBackgroundTaskUpdateLocalAnimations;
class CBackgroundTaskCompressCAF;
class CAnimationCompressionManager
    : public IAnimationCompressionManager
    , public IEditorNotifyListener
{
public:
    CAnimationCompressionManager();
    ~CAnimationCompressionManager();


    bool IsEnabled() const override;
    void UpdateLocalAnimations() override;

    void OnEditorNotifyEvent(EEditorNotifyEvent ev) override;
private:
    class CBackgroundTaskCompressCAF;
    class CBackgroundTaskUpdateLocalAnimations;
    class CBackgroundTaskImportAnimation;
    class CBackgroundTaskReloadCHRPARAMS;
    class CBackgroundTaskCleanUpAnimations;

    void Start();

    void OnCAFCompressed(CBackgroundTaskCompressCAF* pTask);
    void OnImportTriggered(const char* animationPath);
    void OnReloadCHRPARAMSComplete(CBackgroundTaskReloadCHRPARAMS* pTask);

    struct SAnimationInWorks
    {
        CBackgroundTaskCompressCAF* pTask;
    };

    CBackgroundTaskUpdateLocalAnimations* m_pRescanTask;
    CBackgroundTaskReloadCHRPARAMS* m_pReloadCHRPARAMSTask;
    typedef std::map<string, SAnimationInWorks> TAnimationMap;
    TAnimationMap m_animationsInWork;
    std::vector<SAnimationInWorks> m_canceledAnimations;
    bool m_enabled;
};
