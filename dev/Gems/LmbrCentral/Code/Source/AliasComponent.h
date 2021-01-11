/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <LmbrCentral/FileSystem/AliasConfiguration.h>
#include <LmbrCentral_Traits_Platform.h>

namespace AZ
{
    namespace IO
    {
        class FileIOBase;
    }
}

namespace LmbrCentral
{
    class AliasComponent
        : public AZ::Component
        , public IAliasConfiguration
    {

        struct CacheLock
        {
            ~CacheLock()
            {
                Release();
            }

            AZStd::string Acquire(AZStd::string cachePath, bool allowedRemoteIO);

            void Release();

            AZStd::string m_cachePath;

#if AZ_TRAIT_LMBRCENTRAL_USE_CACHE_LOCK
            HANDLE m_handle = INVALID_HANDLE_VALUE;
#endif
        };

    public:
        AZ_COMPONENT(AliasComponent, "{4CCC5CFF-01C4-4D63-878A-A726EFCC41AA}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        ////

        // IAliasConfiguration interface implementation
        void SetupAliases() override;
        void ReleaseCache() override;
        ////
    protected:

        static void SetAlias(AZ::IO::FileIOBase* io, const char* alias, AZStd::string path);

        AliasConfiguration m_config;
        CacheLock m_cacheLock;
    };
}
