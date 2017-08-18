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
#ifndef ASSETPROCESSOR_RCCOMMON_H
#define ASSETPROCESSOR_RCCOMMON_H

#include <QString>

namespace AssetProcessor
{
    // RCCommon contains common structs used by the RC Job System

    //! Job Exit codes.
    // note that this is NOT a complete list of return codes, and RC.EXE itself may return unknown return codes here.

    enum JobExitCodes : int
    {
        JobExitCode_Success = 0,
        JobExitCode_Failed = -10,
        JobExitCode_InvalidParams = -9,
        JobExitCode_UnableToCreateTempDir = -8,
        JobExitCode_CopyFailed = -6,
        JobExitCode_Unknown = -1,
        JobExitCode_RCNotFound = -4,
        JobExitCode_RCCouldNotBeLaunched = -5,
        JobExitCode_JobCancelled = -7,
    };


    //! Identifies a queued job uniquely.  Not a case sensitive compare
    class QueueElementID
    {
    public:
        QueueElementID() = default;
        QueueElementID(QString inputAssetName, QString platform, QString jobDescriptor);

        QString GetInputAssetName() const;
        QString GetPlatform() const;
        QString GetJobDescriptor() const;
        bool operator==(const QueueElementID& other) const;
        bool operator<(const QueueElementID& other) const;

    protected:
        QString m_inputAssetName;
        QString m_platform;
        QString m_jobDescriptor;
    };
} // namespace AssetProcessor

// qt's qHash for that type must be in global scope.
uint qHash(const AssetProcessor::QueueElementID& key, uint seed = 0);

#endif //ASSETPROCESSOR_RCQUEUESORTMODEL_H