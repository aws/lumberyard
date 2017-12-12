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

#include <functional>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

#include <AzCore/EBus/EBus.h>

namespace LmbrAWS
{
    // This will be moved to CloudCanvasCommon once LmbrAWS is completely deprecated (See below)
    // Until that happens it needs to live here so we don't need to copy it
    enum RequestRootCAFileResult
    {
        ROOTCA_USER_FILE_RESOLVE_FAILED = -3,
        ROOTCA_FILE_COPY_FAILED = -2,
        ROOTCA_PLATFORM_NOT_SUPPORTED = -1,
        ROOTCA_USER_FILE_NOT_FOUND = 0,
        ROOTCA_FOUND_FILE_SUCCESS = 1,
    };
    // AWS API initialization is now the responsibility of the CloudCanvasCommon
    // Gem. As a temporary solution, until all of LmbrAWS can be deprecated, 
    // LmbrAWS will continue to initialize the API when that gem isn't present. 
    // When present, that gem provides the handler for the following bus. If 
    // there is no handler for this bus then LmbrAWS initializes the API.
    class AwsApiInitRequests
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual bool IsAwsApiInitialized() = 0;

        virtual RequestRootCAFileResult LmbrRequestRootCAFile(AZStd::string& resultStr) = 0;
    };
    using AwsApiInitRequestBus = AZ::EBus<AwsApiInitRequests>;
} // namespace LmbrAWS


namespace LmbrAWS
{
    class IClientManager;
    class MaglevConfig;
}

class IAWSMainThreadRunnable
{
public:

    virtual ~IAWSMainThreadRunnable() {}

    virtual void RunOnMainThread() = 0;
};

class ILmbrAWS
{
public:

    using AWSMainThreadTask = std::function<void()>;
    virtual ~ILmbrAWS() {}

    virtual void PostCompletedRequests() = 0;
    virtual void Release() = 0;
    virtual LmbrAWS::IClientManager* GetClientManager() = 0;
    virtual LmbrAWS::MaglevConfig* GetMaglevConfig() = 0;

    /// Runs an IAWSMainThreadRunnable on the main thread. Threadsafe to call. Object will be deleted as soon as its RunOnMainThread function finishes executing
    virtual void PostToMainThread(IAWSMainThreadRunnable* runnable) = 0;
    virtual void PostToMainThread(const AWSMainThreadTask& task) = 0;

    /// Settings
    using AwsCloudSettings = AZStd::unordered_map< AZStd::string, AZStd::string >;
    virtual void LoadCloudSettings(const char* applicationName, AwsCloudSettings& settings) = 0;
    virtual void SaveCloudSettings(const char* applicationName, const AwsCloudSettings& settings) = 0;

    virtual void Initialize() = 0;
};


