#pragma once

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

#include <QObject>
#include <QTimer>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>

namespace AzToolsFramework
{
    namespace AssetSystem
    {
        struct JobInfo;
    }
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            class SCENE_UI_API JobWatcher : public QObject
            {
                Q_OBJECT

            public:
                JobWatcher(const AZStd::string& sourceAssetFullPath, Uuid traceTag);

            signals:
                void JobQueryFailed();
                void JobsForSourceFileFound(const AZStd::vector<AZStd::string>& jobPlatforms);
                void JobProcessingComplete(const AZStd::string& platform, u64 jobId, bool success, const AZStd::string& fullLog);
                void AllJobsComplete();

            private slots:
                void OnQueryJobs();

            private:
                static const int s_jobQueryInterval;

                bool m_hasReportedAvailableJobs;
                QTimer* m_jobQueryTimer;
                AZStd::string m_sourceAssetFullPath;
                Uuid m_traceTag;
            };
        } // SceneUI
    } //  SceneAPI
} // AZ