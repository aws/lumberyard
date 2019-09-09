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

#include <AzCore/EBus/EBus.h>

namespace CloudGemFramework
{
    class MappingInfo
    {
    public:
        MappingInfo(AZStd::string& physicalName, AZStd::string& resourceType) :
            m_physicalName{ physicalName },
            m_resourceType{ resourceType }
        {

        }

        MappingInfo(AZStd::string&& physicalName, AZStd::string&& resourceType) :
            m_physicalName{ AZStd::move(physicalName) },
            m_resourceType{ AZStd::move(resourceType) }
        {

        }
        const AZStd::string& GetPhysicalName() const { return m_physicalName; }
        const AZStd::string& GetResourceType() const { return m_resourceType; }

    private:
        AZStd::string m_physicalName;
        AZStd::string m_resourceType;
    };

    using MappingDataInstance = AZStd::shared_ptr<MappingInfo>;
    using MappingData = AZStd::unordered_map<AZStd::string, MappingDataInstance>;

    class CloudCanvasMappingsRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual AZStd::string GetLogicalToPhysicalResourceMapping(const AZStd::string& logicalResourceName) = 0;
        virtual void SetLogicalMapping(AZStd::string resourceType, AZStd::string logicalName, AZStd::string physicalName) = 0;
        virtual AZStd::vector<AZStd::string> GetMappingsOfType(const AZStd::string& resourceType) = 0;
        virtual MappingData GetAllMappings() = 0;

        virtual bool LoadLogicalMappingsFromFile(const AZStd::string& mappingsFileName) = 0;
        virtual void InitializeGameMappings() {}

        virtual bool IsProtectedMapping() = 0;
        virtual void SetProtectedMapping(bool isProtected) = 0;
        virtual bool IgnoreProtection() = 0;
        virtual void SetIgnoreProtection(bool ignore) = 0;

    };
    using CloudCanvasMappingsBus = AZ::EBus<CloudCanvasMappingsRequests>;

    class UserPoolClientInfo
    {
    public:
        UserPoolClientInfo(AZStd::string& clientId, AZStd::string& clientSecret) :
            m_clientId{ clientId },
            m_clientSecret{ clientSecret }
        {

        }

        UserPoolClientInfo(AZStd::string&& clientId, AZStd::string&& clientSecret) :
            m_clientId{ AZStd::move(clientId) },
            m_clientSecret{ AZStd::move(clientSecret) }
        {

        }

        const AZStd::string& GetClientId() const { return m_clientId; }
        const AZStd::string& GetClientSecret() const { return m_clientSecret; }
    private:
        AZStd::string m_clientId;
        AZStd::string m_clientSecret;
    };

    class CloudCanvasUserPoolMappingsRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual void SetLogicalUserPoolClientMapping(const AZStd::string& logicalName, const AZStd::string& clientName, AZStd::string clientId, AZStd::string clientSecret) = 0;
        virtual AZStd::shared_ptr<UserPoolClientInfo> GetUserPoolClientInfo(const AZStd::string& logicalName, const AZStd::string& clientName) = 0;
    };
    using CloudCanvasUserPoolMappingsBus = AZ::EBus<CloudCanvasUserPoolMappingsRequests>;
} // namespace CloudCanvasCommon
