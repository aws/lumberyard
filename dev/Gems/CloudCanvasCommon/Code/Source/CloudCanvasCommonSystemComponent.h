/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <AzCore/Component/Component.h>

#include <CloudCanvasCommon/CloudCanvasCommonBus.h>

#include <aws/core/Aws.h>
#include <aws/core/utils/memory/MemorySystemInterface.h>

#include <CloudCanvas/ICloudCanvas.h>
#include <CrySystemBus.h>

namespace CloudCanvasCommon
{
    class CloudCanvasCommonSystemComponent
        : public AZ::Component
        , protected CloudCanvasCommonRequestBus::Handler
        , protected CloudCanvas::AwsApiInitRequestBus::Handler,
          public CrySystemEventBus::Handler
    {
    public:

        static const char* COMPONENT_DISPLAY_NAME;
        static const char* COMPONENT_DESCRIPTION;
        static const char* COMPONENT_CATEGORY;
        static const char* SERVICE_NAME;

        static const char* AWS_API_ALLOC_TAG;
        static const char* AWS_API_LOG_PREFIX;

        AZ_COMPONENT(CloudCanvasCommonSystemComponent, "{35921485-494D-43B6-B893-E557088C3FFE}");

        CloudCanvasCommonSystemComponent() = default;
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // LmbrAWS::AwsApiInitRequestBus interface implementation
        bool IsAwsApiInitialized() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        // Some platforms (Android) need to point the http client at the certificate bundle to avoid SSL errors
        virtual CloudCanvas::RequestRootCAFileResult RequestRootCAFile(AZStd::string& resultPath) override;
        // Skips over platform checks - called by RequestRootCAFile
        virtual CloudCanvas::RequestRootCAFileResult GetUserRootCAFile(AZStd::string& resultPath) override;

        virtual CloudCanvas::RequestRootCAFileResult LmbrRequestRootCAFile(AZStd::string& resultPath) override;

        virtual int GetEndpointHttpResponseCode(const AZStd::string& endPoint) override;

        ////////////////////////////////////////////////////////////////////////
        // CrySystemEvents
        void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;
        void OnCrySystemShutdown(ISystem&) override;
    private:
        CloudCanvasCommonSystemComponent(const CloudCanvasCommonSystemComponent&) = delete;

        // Early out for platforms which don't need RootCA File in the ClientConfig
        static bool DoesPlatformUseRootCAFile();
        // We may need to copy the rootCA file outside of our game files to the ResolvedUserPath so cURL can see it
        void CheckCopyRootCAFile();
        // Return the unresolved path (@user@/...)
        AZStd::string GetRootCAUserPath() const;
        // Return the resolved version of GetRootCAUserPath ( /storage/emulated/0/CloudGemSamples/user/certs/aws/cacert.pem )
        bool GetResolvedUserPath(AZStd::string& resolvedPath) const;

        void InitAwsApi();
        void ShutdownAwsApi();
        AZStd::string m_resolvedCertPath;
        AZStd::atomic<bool> m_resolvedCert;
        AZStd::mutex m_certPathMutex;
        mutable AZStd::mutex m_resolveUserPathMutex;
        AZStd::mutex m_certCopyMutex;
    };
}
