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

#include <AzCore/Component/Component.h>

#include <AzFramework/API/BootstrapReaderBus.h>
#include <AzFramework/Asset/CfgFileAsset.h>

namespace AzFramework
{
    class BootstrapReaderComponent
        : public AZ::Component
        , private BootstrapReaderRequestBus::Handler
    {
    public:
        BootstrapReaderComponent();
        ~BootstrapReaderComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        AZ_COMPONENT(BootstrapReaderComponent, "{35F2A89D-ECB0-49ED-8306-E65A358F354B}");
        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        /// Overrides the file contents loaded with the provided text block.
        void OverrideFileContents(const AZStd::string& newContents);

    private:
        //////////////////////////////////////////////////////////////////////////
        // BootstrapReaderRequestBus::Handler
        bool SearchConfigurationForKey(const AZStd::string& key, bool checkPlatform, AZStd::string& value) override;
        //////////////////////////////////////////////////////////////////////////

        bool ReadFieldImpl(const AZStd::string& key, AZStd::string& value);

        // This uses a simple asset reference instead of a full asset reference because there's
        // an upgrade path from raw string to simple asset reference, but not from raw string to full asset reference.
        AzFramework::SimpleAssetReference<AzFramework::CfgFileAsset> m_configFile;
        AZStd::string m_fileContents;
        AZStd::unordered_map<AZStd::string, AZStd::string> m_keyValueCache;
    };
} // namespace AzFramework
