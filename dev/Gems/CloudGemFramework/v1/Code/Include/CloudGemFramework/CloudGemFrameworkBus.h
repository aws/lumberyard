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

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

#include <CloudCanvas/ICloudCanvas.h>

namespace AZ
{
    class JobContext;
}

namespace Aws
{
    namespace Auth
    {
        class AWSCredentialsProvider;
    }
}

namespace CloudGemFramework
{
    class AwsApiJobConfig;
}

namespace CloudGemFramework
{

    class CloudGemFrameworkRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /// Returns the base url for a Cloud Gem service.
        /// \param serviceName - The name of the Gem or Cloud Canvas Resource Group that provides the services.
        /// \return the service URL without a trailing / character.
        virtual AZStd::string GetServiceUrl(const AZStd::string& serviceName) = 0;

        /// Returns the JobContext that should be used for AWS jobs.
        virtual AZ::JobContext* GetDefaultJobContext() = 0;

        // Root CA File needed on some platforms (Android)
        virtual CloudCanvas::RequestRootCAFileResult GetRootCAFile(AZStd::string& filePath) = 0;

        // Returns the default client configuration settings for CloudGemFramework.  Because this is implemented in a static library potentially
        // there are other copies around - for the purposes of Cloud Canvas code the CloudGemFramework version is the "master" and covers the ground
        // the client settings in LmbrAWS did before
        virtual AwsApiJobConfig* GetDefaultConfig() = 0;
#ifdef _DEBUG
        /// Tracks job instances. All job instances must be deleted before 
        /// CloudGemFrameworkSystemComponent is deactivated.
        virtual void IncrementJobCount() = 0;
        virtual void DecrementJobCount() = 0;
#endif

    };

    using CloudGemFrameworkRequestBus = AZ::EBus<CloudGemFrameworkRequests>;


    /// Bus used to send notifications to the code in the CloudGemFramework-
    /// StaticLibrary. Using the framework does not require implementing these
    /// handlers.
    class InternalCloudGemFrameworkNotifications
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /// Called when the CloudGemFrameworkSystemComponent is deactivated.
        /// This tells code in the CloudGemFrameworkStaticLibrary that the
        /// framework can no longer be used and allows the configuration 
        /// objects to delete any cached clients or other data allocated using 
        /// the AWS API's allocator (which may no longer be valid after 
        /// deactivation).
        virtual void OnCloudGemFrameworkDeactivated() = 0;

    };
    using InternalCloudGemFrameworkNotificationBus = AZ::EBus<InternalCloudGemFrameworkNotifications>;

} // namespace CloudGemFramework
