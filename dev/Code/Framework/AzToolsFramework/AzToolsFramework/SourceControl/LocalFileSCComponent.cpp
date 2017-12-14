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

#include "stdafx.h"

#include "NullSCComponent.h"
#include <AzCore/Component/TickBus.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    void RefreshInfoFromFileSystem(SourceControlFileInfo& fileInfo)
    {
        if (!AZ::IO::SystemFile::Exists(fileInfo.m_filePath.c_str()))
        {
            fileInfo.m_status = SCS_NotTracked;
            fileInfo.m_flags = 0;
        }
        else if (!AZ::IO::SystemFile::IsWritable(fileInfo.m_filePath.c_str()))
        {
            fileInfo.m_status = SCS_Tracked;
            fileInfo.m_flags = 0;
        }
        else
        {
            fileInfo.m_status = SCS_OpenByUser;
            fileInfo.m_flags = SCF_Writeable;
        }
    }

    void RemoveReadOnly(const SourceControlFileInfo& fileInfo)
    {
        if ((fileInfo.m_flags & SCF_Writeable) != SCF_Writeable)
        {
            if (fileInfo.m_status != SCS_NotTracked)
            {
                AZ::IO::SystemFile::SetWritable(fileInfo.m_filePath.c_str(), true);
            }
        }
    }

    void NullSCComponent::Activate()
    {
        SourceControlConnectionRequestBus::Handler::BusConnect();
        SourceControlCommandBus::Handler::BusConnect();
    }

    void NullSCComponent::Deactivate()
    {
        SourceControlCommandBus::Handler::BusDisconnect();
        SourceControlConnectionRequestBus::Handler::BusDisconnect();
    }

    void NullSCComponent::GetFileInfo(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        SourceControlFileInfo fileInfo(fullFilePath);
        auto job = AZ::CreateJobFunction([fileInfo, respCallback, this]() mutable
                {
                    RefreshInfoFromFileSystem(fileInfo);
                    if (fileInfo.m_status == SCS_NotTracked) //file doesn't exist
                    {
                        EBUS_QUEUE_FUNCTION(AZ::TickBus, respCallback, false, fileInfo);
                    }
                    else
                    {
                        EBUS_QUEUE_FUNCTION(AZ::TickBus, respCallback, true, fileInfo);
                    }
                }, true);
        job->Start();
    }

    void NullSCComponent::RequestEdit(const char* fullFilePath, bool /*allowMultiCheckout*/, const SourceControlResponseCallback& respCallback)
    {
        SourceControlFileInfo fileInfo(fullFilePath);
        auto job = AZ::CreateJobFunction([fileInfo, respCallback, this]() mutable
                {
                    RefreshInfoFromFileSystem(fileInfo);
                    RemoveReadOnly(fileInfo);
                    RefreshInfoFromFileSystem(fileInfo);

                    // As a quality of life improvement for our users, we want request edit 
                    // to report success in the case where a file doesn't exist.  We do this so
                    // developers can always call RequestEdit before a save operation; instead of:
                    //   File Exists            --> RequestEdit, then SaveOperation
                    //   File Does not Exist    --> SaveOperation, then RequestEdit
                    if (fileInfo.m_flags == SCF_Writeable || fileInfo.m_status == SCS_NotTracked)
                    {
                        EBUS_QUEUE_FUNCTION(AZ::TickBus, respCallback, true, fileInfo);
                    }
                    else
                    {
                        EBUS_QUEUE_FUNCTION(AZ::TickBus, respCallback, false, fileInfo);
                    }
                }, true);
        job->Start();
    }

    void NullSCComponent::RequestDelete(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        SourceControlFileInfo fileInfo(fullFilePath);
        auto job = AZ::CreateJobFunction([fileInfo, respCallback, this]() mutable
                {
                    RefreshInfoFromFileSystem(fileInfo);
                    RemoveReadOnly(fileInfo);
            auto succeeded = AZ::IO::SystemFile::Delete(fileInfo.m_filePath.c_str());
                    RefreshInfoFromFileSystem(fileInfo);
            EBUS_QUEUE_FUNCTION(AZ::TickBus, respCallback, succeeded, fileInfo);
                }, true);
        job->Start();
    }

    void NullSCComponent::RequestRevert(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        // Get the info, and fail if the file doesn't exist.
        GetFileInfo(fullFilePath, respCallback);
    }

    void NullSCComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<NullSCComponent, AZ::Component>()
                ->SerializerForEmptyClass()
            ;
        }
    }
}   // namespace AzToolsFramework