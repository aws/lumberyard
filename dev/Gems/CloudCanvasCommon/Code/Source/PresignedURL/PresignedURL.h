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
#pragma once

#include <PresignedURL/PresignedURLBus.h>

#include <AzCore/std/string/string.h>
#include <AzCore/Jobs/Job.h>

namespace CloudCanvas
{

    class PresignedURLManager : public PresignedURLRequestBus::Handler
    {
    public:
        PresignedURLManager();
        virtual ~PresignedURLManager();

        void RequestDownloadSignedURLReceivedHandler(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id, DataReceivedEventHandler&& dataReceivedHandler) override;
        AZ::Job* RequestDownloadSignedURLJobReceivedHandler(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id, DataReceivedEventHandler&& dataReceivedHandler) override;

        void RequestDownloadSignedURL(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id) override;
        AZ::Job* RequestDownloadSignedURLJob(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id) override;
        AZStd::unordered_map<AZStd::string, AZStd::string> GetQueryParameters(const AZStd::string& signedURL) override;

        AZ::Job* CreateDownloadSignedURLJob(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id, DataReceivedEventHandler&& dataReceivedHandler) const;
    };
} // namespace CloudCanvas



