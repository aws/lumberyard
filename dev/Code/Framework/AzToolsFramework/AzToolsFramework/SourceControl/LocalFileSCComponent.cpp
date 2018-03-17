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

#include "StdAfx.h"

#include <AzToolsFramework/SourceControl/LocalFileSCComponent.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    void RefreshInfoFromFileSystem(SourceControlFileInfo& fileInfo)
    {
        fileInfo.m_flags = 0;
        if (!AZ::IO::SystemFile::Exists(fileInfo.m_filePath.c_str()))
        {
            fileInfo.m_flags |= SCF_Writeable;  // Non-existent files are not read only
        }
        else
        {
            fileInfo.m_flags |= SCF_Tracked;
            if (AZ::IO::SystemFile::IsWritable(fileInfo.m_filePath.c_str()))
            {
                fileInfo.m_flags |= SCF_Writeable | SCF_OpenByUser;
            }
        }

        fileInfo.m_status = SCS_OpSuccess;
    }

    void RemoveReadOnly(const SourceControlFileInfo& fileInfo)
    {
        if (!fileInfo.HasFlag(SCF_Writeable) && fileInfo.HasFlag(SCF_Tracked))
        {
            AZ::IO::SystemFile::SetWritable(fileInfo.m_filePath.c_str(), true);
        }
    }

    void LocalFileSCComponent::Activate()
    {
        SourceControlConnectionRequestBus::Handler::BusConnect();
        SourceControlCommandBus::Handler::BusConnect();
    }

    void LocalFileSCComponent::Deactivate()
    {
        SourceControlCommandBus::Handler::BusDisconnect();
        SourceControlConnectionRequestBus::Handler::BusDisconnect();
    }

    void LocalFileSCComponent::GetFileInfo(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        SourceControlFileInfo fileInfo(fullFilePath);
        auto job = AZ::CreateJobFunction([fileInfo, respCallback]() mutable
                {
                    RefreshInfoFromFileSystem(fileInfo);
                    AZ::TickBus::QueueFunction(respCallback, fileInfo.CompareStatus(SCS_OpSuccess), fileInfo);
                }, true);
        job->Start();
    }

    void LocalFileSCComponent::RequestEdit(const char* fullFilePath, bool /*allowMultiCheckout*/, const SourceControlResponseCallback& respCallback)
    {
        SourceControlFileInfo fileInfo(fullFilePath);
        auto job = AZ::CreateJobFunction([fileInfo, respCallback]() mutable
                {
                    RefreshInfoFromFileSystem(fileInfo);
                    RemoveReadOnly(fileInfo);
                    RefreshInfoFromFileSystem(fileInfo);

                    // As a quality of life improvement for our users, we want request edit
                    // to report success in the case where a file doesn't exist.  We do this so
                    // developers can always call RequestEdit before a save operation; instead of:
                    //   File Exists            --> RequestEdit, then SaveOperation
                    //   File Does not Exist    --> SaveOperation, then RequestEdit
                    AZ::TickBus::QueueFunction(respCallback, fileInfo.HasFlag(SCF_Writeable), fileInfo);
                }, true);
        job->Start();
    }

    void LocalFileSCComponent::RequestDelete(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        SourceControlFileInfo fileInfo(fullFilePath);
        auto job = AZ::CreateJobFunction([fileInfo, respCallback]() mutable
                {
                    RefreshInfoFromFileSystem(fileInfo);
                    RemoveReadOnly(fileInfo);
                    auto succeeded = AZ::IO::SystemFile::Delete(fileInfo.m_filePath.c_str());
                    RefreshInfoFromFileSystem(fileInfo);
                    AZ::TickBus::QueueFunction(respCallback, succeeded, fileInfo);
                }, true);
        job->Start();
    }

    void LocalFileSCComponent::RequestRevert(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        // Get the info, and fail if the file doesn't exist.
        GetFileInfo(fullFilePath, respCallback);
    }

    void LocalFileSCComponent::RequestLatest(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        SourceControlFileInfo fileInfo(fullFilePath);
        auto job = AZ::CreateJobFunction([fileInfo, respCallback, this]() mutable
        {
            RefreshInfoFromFileSystem(fileInfo);
            AZ::TickBus::QueueFunction(respCallback, true, fileInfo);
        }, true);
        job->Start();
    }

    void LocalFileSCComponent::RequestRename(const char* sourcePathFull, const char* destPathFull, const SourceControlResponseCallback& respCallback)
    {
        SourceControlFileInfo fileInfoSrc(sourcePathFull);
        SourceControlFileInfo fileInfoDst(destPathFull);
        auto job = AZ::CreateJobFunction([fileInfoSrc, fileInfoDst, respCallback, this]() mutable
        {
            RefreshInfoFromFileSystem(fileInfoSrc);
            auto succeeded = AZ::IO::SystemFile::Rename(fileInfoSrc.m_filePath.c_str(), fileInfoDst.m_filePath.c_str());
            RefreshInfoFromFileSystem(fileInfoDst);
            fileInfoDst.m_status = succeeded ? SCS_OpSuccess : SCS_ProviderError;
            AZ::TickBus::QueueFunction(respCallback, succeeded, fileInfoDst);
        }, true);
        job->Start();
    }

    void LocalFileSCComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<LocalFileSCComponent, AZ::Component>()
                ->SerializerForEmptyClass()
            ;
        }
    }
}   // namespace AzToolsFramework