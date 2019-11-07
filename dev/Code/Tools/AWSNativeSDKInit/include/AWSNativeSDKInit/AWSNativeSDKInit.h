/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <AWSNativeSDKInit/AWSMemoryInterface.h>

#include <AzCore/Module/Environment.h>


#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
#include <aws/core/Aws.h>
#endif

namespace AWSNativeSDKInit
{
    // Entry point for Lumberyard managing the AWSNativeSDK's initialization and shutdown requirements
    // Use an AZ::Environment variable to enforce only one init and shutdown
    class InitializationManager
    {
    public:
        static const char* const initializationManagerTag;

        InitializationManager();
        ~InitializationManager();

        // Call to guarantee that the API is initialized with proper Lumberyard settings.
        // It's fine to call this from every module which needs to use the NativeSDK
        // Creates a static shared pointer using the AZ EnvironmentVariable system.
        // This will prevent a the AWS SDK from going through the shutdown routine until all references are gone, or
        // the AZ::EnvironmentVariable system is brought down.
        static void InitAwsApi();
        static bool IsInitialized();

        // Remove our reference
        static void Shutdown();
    private:    
        void InitializeAwsApiInternal();
        void ShutdownAwsApiInternal();

        MemoryManager m_memoryManager;

        static AZ::EnvironmentVariable<InitializationManager> s_initManager;
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        Aws::SDKOptions m_awsSDKOptions;
#endif
    };
}
