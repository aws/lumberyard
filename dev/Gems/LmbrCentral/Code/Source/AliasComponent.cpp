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

#include "LmbrCentral_precompiled.h"

#include "AliasComponent.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace LmbrCentral
{
    AZStd::string AliasComponent::CacheLock::Acquire(AZStd::string cachePath, [[maybe_unused]] bool allowedRemoteIO)
    {
        AZStd::string finalCachePath = cachePath;

#if AZ_TRAIT_LMBRCENTRAL_USE_CACHE_LOCK
        // Search for a non-locked cache directory because shaders require separate caches for each running instance.
        // We only need to do this check for Windows, because consoles can't have multiple instances running simultaneously.
        // Ex: running editor and game, running multiple games, or multiple non-interactive editor instances 
        // for parallel level exports.
        
        AZStd::string originalPath = cachePath;
#if defined(REMOTE_ASSET_PROCESSOR)
        if (!allowedRemoteIO) // not running on VFS
#endif
        {
            int attemptNumber = 0;

            // The number of max attempts ultimately dictates the number of Lumberyard instances that can run
            // simultaneously.  This should be a reasonably high number so that it doesn't artificially limit
            // the number of instances (ex: parallel level exports via multiple Editor runs).  It also shouldn't 
            // be set *infinitely* high - each cache folder is GBs in size, and finding a free directory is a 
            // linear search, so the more instances we allow, the longer the search will take.  
            // 128 seems like a reasonable compromise.
            constexpr int maxAttempts = 128;

            char workBuffer[AZ_MAX_PATH_LEN] = { 0 };
            while (attemptNumber < maxAttempts)
            {
                finalCachePath = originalPath;
                if (attemptNumber != 0)
                {
                    azsnprintf(workBuffer, AZ_MAX_PATH_LEN, "%s%i", originalPath.c_str(), attemptNumber);
                    finalCachePath = workBuffer;
                }
                else
                {
                    finalCachePath = originalPath;
                }

                ++attemptNumber; // do this here so we don't forget

                // if the directory already exists, check for locked file
                AZStd::string outLockPath;
                AZ::StringFunc::Path::Join(finalCachePath.c_str(), "lockfile.txt", outLockPath);

                AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

                if (fileIO && !fileIO->Exists(finalCachePath.c_str()))
                {
                    fileIO->CreatePath(finalCachePath.c_str());
                }

                // note, the zero here after GENERIC_READ|GENERIC_WRITE indicates no share access at all
                m_handle = CreateFileA(outLockPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, 0);
                if (m_handle != INVALID_HANDLE_VALUE)
                {
                    break;
                }
            }

            if (attemptNumber >= maxAttempts)
            {
                AZ_Assert(false, "Couldn't find a valid asset cache folder for the Asset Processor after %i attempts.", attemptNumber);
                AZ_Printf("FileSystem", "Couldn't find a valid asset cache folder for the Asset Processor after %i attempts.", attemptNumber);
            }
        }

#endif // AZ_TRAIT_LMBRCENTRAL_USE_CACHE_LOCK

        return m_cachePath = finalCachePath;
    }

    void AliasComponent::CacheLock::Release()
    {
#if AZ_TRAIT_LMBRCENTRAL_USE_CACHE_LOCK
        if (m_handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
            m_cachePath = "";
        }
#endif
    }

    void AliasComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            
            serializeContext->Class<AliasComponent, AZ::Component>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AliasComponent>(
                    "LmbrCentral", "Coordinates initialization of systems within LmbrCentral")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Game")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b));
            }
        }
    }

    void AliasComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("FileSystemAlias", 0x772683b7));
    }

    void AliasComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("FileSystemAlias", 0x772683b7));
    }

    void AliasComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void AliasComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void AliasComponent::Activate()
    {
        AZ::Interface<IAliasConfiguration>::Register(this);
        SetupAliases();
    }

    void AliasComponent::Deactivate()
    {
        AZ::Interface<IAliasConfiguration>::Unregister(this);

        m_cacheLock.Release();
    }

    void AliasComponent::SetupAliases()
    {
        const auto& config = AliasConfigurationStorage::GetConfig();
        auto io = AZ::IO::FileIOBase::GetInstance();

        if (io)
        {
            SetAlias(io, "@root@", config.m_rootPath.c_str());
            SetAlias(io, "@assets@", config.m_assetsPath.c_str());
            SetAlias(io, "@user@", config.m_userPath.c_str());
            SetAlias(io, "@log@", config.m_logPath.c_str());

            if(!config.m_cachePath.empty())
            {
                if(m_cacheLock.m_cachePath.empty())
                {
                    char resolvedPath[AZ_MAX_PATH_LEN] = "";

                    io->ResolvePath(config.m_cachePath.c_str(), resolvedPath, AZ_ARRAY_SIZE(resolvedPath));

                    m_cacheLock.Acquire(resolvedPath, config.m_allowedRemoteIo);
                }

                io->SetAlias("@cache@", m_cacheLock.m_cachePath.c_str());
            }

            SetAlias(io, "@engroot@", config.m_engineRootPath.c_str());
            SetAlias(io, "@devroot@", config.m_devRootPath.c_str());
            SetAlias(io, "@devassets@", config.m_devAssetsPath.c_str());
        }
    }

    void AliasComponent::ReleaseCache()
    {
        m_cacheLock.Release();
    }

    void AliasComponent::SetAlias(AZ::IO::FileIOBase* io, const char* alias, AZStd::string path)
    {
        char resolvedPath[AZ_MAX_PATH_LEN] = "";

        if(path.empty())
        {
            return;
        }

        if(path.starts_with('@'))
        {
            io->ResolvePath(path.c_str(), resolvedPath, AZ_ARRAY_SIZE(resolvedPath));
            io->SetAlias(alias, resolvedPath);
        }
        else
        {
            io->SetAlias(alias, path.c_str());
        }
    }
}
