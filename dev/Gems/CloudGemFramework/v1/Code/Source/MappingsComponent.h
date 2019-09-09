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

#include "CloudGemFramework_precompiled.h"

#include <AzCore/Component/Component.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

#include <CloudCanvas/CloudCanvasMappingsBus.h>

#include <CrySystemBus.h>
#include <IConsole.h>

#undef GetObject

#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/json/JsonSerializer.h>

namespace CloudGemFramework
{


    using UserPoolDataInfo = AZStd::shared_ptr<UserPoolClientInfo>;
    using UserPoolMappingData = AZStd::unordered_map<AZStd::string, AZStd::unordered_map<AZStd::string,UserPoolDataInfo>>;

    class CloudCanvasMappingsComponent
        : public AZ::Component,
        public CloudCanvasMappingsBus::Handler,
        public CloudCanvasUserPoolMappingsBus::Handler,
        public CrySystemEventBus::Handler
    {
    public:
        AZ_COMPONENT(CloudCanvasMappingsComponent, "{4DE985CB-9AC4-472F-B647-E5E634C5C3A7}", AZ::Component);

        static const char* SERVICE_NAME;

        CloudCanvasMappingsComponent() = default;
        virtual ~CloudCanvasMappingsComponent() = default;

        CloudCanvasMappingsComponent(const CloudCanvasMappingsComponent& otherComponent) :
            m_isProtectedMapping{ otherComponent.m_isProtectedMapping },
            m_ignoreProtection{ otherComponent.m_ignoreProtection }
        {
            m_mappingData = otherComponent.m_mappingData;
            m_userPoolMappingData = otherComponent.m_userPoolMappingData;
        }

        CloudCanvasMappingsComponent& operator=(const CloudCanvasMappingsComponent& rhs)
        {
            m_mappingData = rhs.m_mappingData;
            m_userPoolMappingData = rhs.m_userPoolMappingData;
            m_isProtectedMapping = rhs.m_isProtectedMapping;
            m_ignoreProtection = rhs.m_ignoreProtection;

            return *this;
        }

        CloudCanvasMappingsComponent& operator=(const CloudCanvasMappingsComponent&& rhs)
        {
            m_mappingData = AZStd::move(rhs.m_mappingData);
            m_userPoolMappingData = AZStd::move(rhs.m_userPoolMappingData);
            m_isProtectedMapping = rhs.m_isProtectedMapping;
            m_ignoreProtection = rhs.m_ignoreProtection;

            return *this;
        }

        CloudCanvasMappingsComponent(CloudCanvasMappingsComponent&& rhs) :
            m_isProtectedMapping{ rhs.m_isProtectedMapping },
            m_ignoreProtection{ rhs.m_ignoreProtection }
        {
            m_mappingData = AZStd::move(rhs.m_mappingData);
            m_userPoolMappingData = AZStd::move(rhs.m_userPoolMappingData);
        }
        static void Reflect(AZ::ReflectContext* reflection);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        //////////////////////////////////////////////////////////////////////////
        // CloudCanvasCommonMappingsBus
        virtual AZStd::string GetLogicalToPhysicalResourceMapping(const AZStd::string& logicalResourceName) override;
        virtual void SetLogicalMapping(AZStd::string resourceType, AZStd::string logicalName, AZStd::string physicalName) override;
        virtual AZStd::vector<AZStd::string> GetMappingsOfType(const AZStd::string& resourceType) override;
        virtual bool LoadLogicalMappingsFromFile(const AZStd::string& mappingsFileName) override;
        virtual MappingData GetAllMappings() override;

        // If the protected flag is set in the mapping this will return true indicating
        // the user should be warned that they will be connecting to protected resources
        // (likely live customer facing) and given a chance to not connect
        virtual bool IsProtectedMapping() override { return m_isProtectedMapping; }
        virtual void SetProtectedMapping(bool isProtected) override { m_isProtectedMapping = isProtected; }
        virtual bool IgnoreProtection() override { return m_ignoreProtection; }
        virtual void SetIgnoreProtection(bool setIgnore) override { m_ignoreProtection = setIgnore; }

        //////////////////////////////////////////////////////////////////////////
        // CloudCanvasUserPoolMappingsBus
        virtual void SetLogicalUserPoolClientMapping(const AZStd::string& logicalName, const AZStd::string& clientName, AZStd::string clientId, AZStd::string clientSecret) override;;
        AZStd::shared_ptr<UserPoolClientInfo> GetUserPoolClientInfo(const AZStd::string& logicalName, const AZStd::string& clientName) override;

        ////////////////////////////////////////////////////////////////////////
        // CrySystemEvents
        void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;
        void OnCrySystemShutdown(ISystem&) override;
    protected:
        virtual void ClearData();

        virtual bool LoadLogicalMappingsFromJson(const Aws::Utils::Json::JsonValue& mappingsJsonData);
        void HandleCustomResourceMapping(const Aws::String& logicalName, const Aws::String& resourceType, const std::pair<Aws::String, Aws::Utils::Json::JsonValue>& mapping);

        static void ConsoleCommandSetLauncherDeployment(IConsoleCmdArgs* pCmdArgs);

        void InitializeGameMappings() override;
        AZStd::string GetLogicalMappingsPath() const;
        AZStd::string GetCurrentDeploymentFromConfig() const;
        AZStd::string GetMappingsFileName(const AZStd::string &dep, const AZStd::string &role) const;
        AZStd::string GetOverrideDeployment() const;

        static void SetOverrideDeployment(AZStd::string_view newDeployment);

        // Any special mapping handling (Such as for Configuration values)
        void HandleMappingType(const AZStd::string& resourceType, const AZStd::string& logicalName, const AZStd::string& physicalName);

        MappingData m_mappingData;
        AZStd::mutex m_dataMutex;

        UserPoolMappingData m_userPoolMappingData;
        AZStd::mutex m_userPoolDataMutex;

        bool m_isProtectedMapping{ false };
        bool m_ignoreProtection{ false };

    };
}

