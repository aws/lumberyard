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
#include "ProjectSettings.h"
#include "GemRegistry.h"

#include <fstream>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/PlatformIncl.h>

#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/error/en.h>

#if defined(AZ_PLATFORM_ANDROID)
#include <errno.h>
#endif

#define MAX_ERROR_STRING_SIZE 512

namespace Gems
{
    AZ_CLASS_ALLOCATOR_IMPL(ProjectSettings, AZ::SystemAllocator, 0)

    ProjectSettings::ProjectSettings(GemRegistry* registry)
        : m_registry(registry)
        , m_initialized(false)
    {
    }

    AZ::Outcome<void, AZStd::string> ProjectSettings::Initialize(const AZStd::string& projectFolder)
    {
        AZ_Assert(!m_initialized, "ProjectSettings has been initialized already.");

        // Project file lives in (ProjectFolder)/gems.json - which might be @assets@/gems.json or an absolute path (in tools)
        AzFramework::StringFunc::Path::Join(projectFolder.c_str(), GEMS_PROJECT_FILE, m_settingsFilePath);

        auto loadOutcome = LoadSettings();
        m_initialized = loadOutcome.IsSuccess();
        return loadOutcome;
    }

    bool ProjectSettings::EnableGem(const ProjectGemSpecifier& spec)
    {
        auto it = m_gems.find(spec.m_id);
        if (it != m_gems.end())
        {
            // If the Gem is already enabled, update the version and path of the entry.
            it->second.m_version = spec.m_version;
            it->second.m_path = spec.m_path;
        }
        else
        {
            // create entry based on data from registry
            m_gems.insert(AZStd::make_pair(spec.m_id, spec));
        }
        return true;
    }
    bool ProjectSettings::DisableGem(const GemSpecifier& spec)
    {
        auto it = m_gems.find(spec.m_id);
        // If the Gem is enabled at the version specified, disable it.
        if (it != m_gems.end() && spec.m_version == it->second.m_version)
        {
            m_gems.erase(it);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool ProjectSettings::IsGemEnabled(const GemSpecifier& spec) const
    {
        auto it = m_gems.find(spec.m_id);
        return it != m_gems.end()
               && it->second.m_version == spec.m_version;
    }

    bool ProjectSettings::IsGemEnabled(const AZ::Uuid& id, const AZStd::vector<AZStd::string>& versionConstraints) const
    {
        AZStd::shared_ptr<GemDependency> dependency = AZStd::make_shared<GemDependency>();
        dependency->SetID(id);
        auto parseOutcome = dependency->ParseVersions(versionConstraints);
        if (!parseOutcome.IsSuccess())
        {
            AZ_Assert(false, parseOutcome.GetError().c_str());
            return false;
        }

        return IsGemDependencyMet(dependency);
    }

    bool ProjectSettings::IsGemDependencyMet(const AZStd::shared_ptr<GemDependency> dep) const
    {
        // Gems can depend on other Gems
        auto it = m_gems.find(dep->GetID());
        return it != m_gems.end()
            && dep->IsFullfilledBy(it->second);
    }

    bool ProjectSettings::IsEngineDependencyMet(const AZStd::shared_ptr<EngineDependency> dep, const EngineVersion& againstVersion) const
    {
        EngineSpecifier engineSpecifier(AZ::Uuid::CreateNull(), againstVersion);
        return dep->IsFullfilledBy(engineSpecifier);
    }

    class GemDependencyInfo : public GemDependency
    {

    public:

        GemDependencyInfo(IGemDescriptionConstPtr gem) 
            : GemDependency()
            , m_gem{gem}
        {
        }

        IGemDescriptionConstPtr GetGem() const
        {
            return m_gem;
        }

    private:

        IGemDescriptionConstPtr m_gem;

    };

    AZ::Outcome<void, AZStd::string> ProjectSettings::ValidateDependencies(const EngineVersion& engineVersion) const
    {
        AZStd::unordered_map<AZ::Uuid, GemDependencyInfo> globalDeps;

        // Build list of required Gems
        for (const auto& pair : m_gems)
        {
            const ProjectGemSpecifier& spec = pair.second;

            auto gem = m_registry->GetGemDescription(spec);
            if (!gem)
            {
                return AZ::Failure(AZStd::string::format("Gem with Id \"%s\" not found.", pair.first.ToString<AZStd::string>().c_str()));
            }

            for (auto && gemDep : gem->GetGemDependencies())
            {
                const AZ::Uuid id = gemDep->GetID();
                GemDependency* dep;

                // If the dependency isn't tracked globally, create a new one
                auto depIt = globalDeps.find(id);
                if (depIt == globalDeps.end())
                {
                    globalDeps.insert(AZStd::make_pair(id, GemDependencyInfo(gem)));
                    dep = &globalDeps.at(id);
                    dep->m_id = id;
                }
                else
                {
                    dep = &depIt->second;
                }

                // These bounds should be normalized before verification to make sure there aren't conflicting bounds
                dep->m_bounds.insert(dep->m_bounds.end(), gemDep->GetBounds().begin(), gemDep->GetBounds().end());
            }
        }

        AZStd::string errorString;
        bool isTreeValid = true;

        // Verify all engine dependencies are met
        for(const auto& pair : m_gems)
        {
            const ProjectGemSpecifier& spec = pair.second;

            auto gem = m_registry->GetGemDescription(spec);
            if (!gem)
            {
                errorString += AZStd::string::format("Gem with Id \"%s\" not found.", pair.first.ToString<AZStd::string>().c_str());
                isTreeValid = false;
                continue;
            }

            // do not verify the engine version if input is default constructed
            if (engineVersion == EngineVersion())
            {
                continue;
            }

            // Check the Gem's engine dependency
            auto engineDepPtr = gem->GetEngineDependency();
            if (engineDepPtr && !IsEngineDependencyMet(engineDepPtr, engineVersion))
            {
                AZStd::string errmsg = AZStd::string::format("Gem with Id \"%s\" does not meet the Lumberyard engine version requirement.\n",
                    pair.first.ToString<AZStd::string>().c_str());

                // do not force an assertion to happen here, we are just printing the warning and letting the user
                // decide on how to handle it if the engine start up fails.
                errorString += errmsg;
                AZ_Warning("GemRegistry", false, errmsg.c_str());
            }
        }

        // Verify all gems dependencies are all met
        for (auto && pair : globalDeps)
        {
            const GemDependencyInfo& dep = pair.second;

            // Find candidate in project's listed gems
            auto candidateIt = m_gems.find(dep.GetID());
            if (candidateIt == m_gems.end())
            {
                // no candidate found
                char depIdStr[UUID_STR_BUF_LEN];
                dep.GetID().ToString(depIdStr, UUID_STR_BUF_LEN, true, true);
                char gemIdStr[UUID_STR_BUF_LEN];
                dep.GetGem()->GetID().ToString(gemIdStr, UUID_STR_BUF_LEN, true, true);
                errorString += AZStd::string::format(
                    "Gem \"%s\" (%s) dependency on Gem with ID %s is unmet.\n", 
                    dep.GetGem()->GetDisplayName().c_str(), 
                    gemIdStr,
                    depIdStr
                );
                isTreeValid = false;
            }
            else if (!dep.IsFullfilledBy(candidateIt->second))
            {
                // candidate found, but it doesn't fulfill all dependency requirements
                AZStd::string boundsStr;
                for (auto && bound : dep.m_bounds)
                {
                    if (boundsStr.length() == 0)
                    {
                        boundsStr = bound.ToString();
                    }
                    else
                    {
                        boundsStr += ", " + bound.ToString();
                    }
                }

                char depIdStr[UUID_STR_BUF_LEN];
                dep.GetID().ToString(depIdStr, UUID_STR_BUF_LEN, true, true);
                char gemIdStr[UUID_STR_BUF_LEN];
                dep.GetGem()->GetID().ToString(gemIdStr, UUID_STR_BUF_LEN, true, true);
                errorString += AZStd::string::format(
                    "Gem \"%s\" (%s) dependency on Gem with ID %s is unmet. It must fall within the following version bounds: [%s]\n", 
                    dep.GetGem()->GetDisplayName().c_str(),
                    gemIdStr,
                    depIdStr,
                    boundsStr.c_str()
                );
                isTreeValid = false;
            }
        }

        if (!isTreeValid)
        {
            return AZ::Failure(errorString);
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> ProjectSettings::Save() const
    {
        using namespace AZ::IO;
        FileIOBase* fileIo = FileIOBase::GetInstance();

        HandleType projectSettingsHandle = InvalidHandle;
        if (fileIo->Open(m_settingsFilePath.c_str(), OpenMode::ModeWrite | OpenMode::ModeText, projectSettingsHandle))
        {
            rapidjson::Document jsonRep = GetJsonRepresentation();

            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            jsonRep.Accept(writer);

            AZ::u64 bytesWritten = 0;
            if (!fileIo->Write(projectSettingsHandle, buffer.GetString(), buffer.GetSize(), &bytesWritten))
            {
                return AZ::Failure(AZStd::string::format("Failed to write Gems settings to file: %s", m_settingsFilePath.c_str()));
            }

            if (bytesWritten != buffer.GetSize())
            {
                return AZ::Failure(AZStd::string::format("Failed to write complete Gems settings to file: %s", m_settingsFilePath.c_str()));
            }

            fileIo->Close(projectSettingsHandle);

            return AZ::Success();
        }
        else
        {
            char errorBuffer[MAX_ERROR_STRING_SIZE];
#if defined(AZ_PLATFORM_WINDOWS)
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                nullptr,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                errorBuffer,
                MAX_ERROR_STRING_SIZE,
                nullptr);
#else
            azstrerror_s(errorBuffer, MAX_ERROR_STRING_SIZE, errno);
#endif // defined(AZ_PLATFORM_WINDOWS)
            return AZ::Failure(AZStd::string::format("Failed to open %s for write: %s", m_settingsFilePath.c_str(), errorBuffer));
        }
    }

    AZ::Outcome<void, AZStd::string> ProjectSettings::LoadSettings()
    {
        AZ::IO::FileIOBase* fileReader = AZ::IO::FileIOBase::GetInstance();

        AZStd::string fileBuffer;

        // an engine compatible file reader has been attached, so use that.
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::u64 fileSize = 0;
        if (!fileReader->Open(m_settingsFilePath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, fileHandle))
        {
            return AZ::Failure(AZStd::string::format("Failed to read %s - unable to open file", m_settingsFilePath.c_str()));
        }

        if ((!fileReader->Size(fileHandle, fileSize)) || (fileSize == 0))
        {
            fileReader->Close(fileHandle);
            return AZ::Failure(AZStd::string::format("Failed to read %s - file truncated.", m_settingsFilePath.c_str()));
        }
        fileBuffer.resize(fileSize);
        if (!fileReader->Read(fileHandle, fileBuffer.data(), fileSize, true))
        {
            fileBuffer.resize(0);
            fileReader->Close(fileHandle);
            return AZ::Failure(AZStd::string::format("Failed to read %s - file read failed.", m_settingsFilePath.c_str()));
        }
        fileReader->Close(fileHandle);

        rapidjson::Document document;
        document.Parse(fileBuffer.data());
        if (document.HasParseError())
        {
            const char* errorStr = rapidjson::GetParseError_En(document.GetParseError());
            return AZ::Failure(AZStd::string::format("Failed to parse %s: %s.", m_settingsFilePath.c_str(), errorStr));
        }
        else
        {
            return ParseJson(document);
        }
    }

    rapidjson::Document ProjectSettings::GetJsonRepresentation() const
    {
        rapidjson::Document rootObj(rapidjson::kObjectType);
        rootObj.AddMember<int>(GPF_TAG_LIST_FORMAT_VERSION, GEMS_PROJECT_FILE_VERSION, rootObj.GetAllocator());

        // We want to write out Gems in the same order each time.
        // Create vector for sorting.
        AZStd::vector<const ProjectGemSpecifier*> sortedGems(m_gems.size());
        auto transformFn = [](const ProjectGemSpecifierMap::value_type& pair) { return &pair.second; };
        AZStd::transform(m_gems.begin(), m_gems.end(), sortedGems.begin(), transformFn);

        // we'll sort based on ID.
        AZStd::sort(sortedGems.begin(), sortedGems.end(), [](const ProjectGemSpecifier* a, const ProjectGemSpecifier* b) -> bool
            {
                return a->m_id < b->m_id;
            });

        auto addMember = [&rootObj](rapidjson::Value& obj, const char* key, const char* str)
            {
                rapidjson::Value k(rapidjson::StringRef(key), rootObj.GetAllocator());
                rapidjson::Value v(rapidjson::StringRef(str), rootObj.GetAllocator());
                obj.AddMember(k.Move(), v.Move(), rootObj.GetAllocator());
            };

        // build Gems array
        rapidjson::Value gemsArray(rapidjson::kArrayType);
        for (const ProjectGemSpecifier* gemSpec : sortedGems)
        {
            char idStr[UUID_STR_BUF_LEN];
            gemSpec->m_id.ToString(idStr, UUID_STR_BUF_LEN, false, false);
            AZStd::to_lower(idStr, idStr + strlen(idStr));

            AZStd::string path = gemSpec->m_path;
            // Replace '\' with '/'
            AZStd::replace(path.begin(), path.end(), '\\', '/');
            // Remove trailing slash
            if (*path.rbegin() == '/')
            {
                path.pop_back();
            }

            rapidjson::Value gemObj(rapidjson::kObjectType);
            addMember(gemObj, GPF_TAG_PATH, path.c_str());
            addMember(gemObj, GPF_TAG_UUID, idStr);
            addMember(gemObj, GPF_TAG_VERSION, gemSpec->m_version.ToString().c_str());

            // write name in comment (if possible)
            if (IGemDescriptionConstPtr gemDesc = m_registry->GetGemDescription(*gemSpec))
            {
                addMember(gemObj, GPF_TAG_COMMENT, gemDesc->GetName().c_str());
            }

            gemsArray.PushBack(gemObj, rootObj.GetAllocator());
        }
        rootObj.AddMember(GPF_TAG_GEM_ARRAY, gemsArray, rootObj.GetAllocator());

        return rootObj;
    }

    AZ::Outcome<void, AZStd::string> ProjectSettings::ParseJson(const rapidjson::Document& jsonRep)
    {
        // check version
        if (!RAPIDJSON_IS_VALID_MEMBER(jsonRep, GPF_TAG_LIST_FORMAT_VERSION, IsInt))
        {
            return AZ::Failure(AZStd::string(GPF_TAG_LIST_FORMAT_VERSION " number is required."));
        }

        int gemListFormatVersion = jsonRep[GPF_TAG_LIST_FORMAT_VERSION].GetInt();
        if (gemListFormatVersion != GEMS_PROJECT_FILE_VERSION)
        {
            return AZ::Failure(AZStd::string::format(
                    GPF_TAG_LIST_FORMAT_VERSION " is version %d, but %d is expected.",
                    gemListFormatVersion,
                    GEMS_PROJECT_FILE_VERSION));
        }

        // read gems
        if (!RAPIDJSON_IS_VALID_MEMBER(jsonRep, GPF_TAG_GEM_ARRAY, IsArray))
        {
            return AZ::Failure(AZStd::string(GPF_TAG_GEM_ARRAY " list is required"));
        }

        const rapidjson::Value& gemList = jsonRep[GPF_TAG_GEM_ARRAY];
        const auto& end = gemList.End();
        for (auto it = gemList.Begin(); it != end; ++it)
        {
            const auto& elem = *it;

            // gem id
            if (!RAPIDJSON_IS_VALID_MEMBER(elem, GPF_TAG_UUID, IsString))
            {
                return AZ::Failure(AZStd::string(GPF_TAG_UUID " string is required for Gem."));
            }
            const char* idStr = elem[GPF_TAG_UUID].GetString();
            AZ::Uuid id = AZ::Uuid::CreateString(idStr);
            if (id.IsNull())
            {
                return AZ::Failure(AZStd::string(GPF_TAG_UUID " string is invalid for Gem."));
            }

            // gem version
            if (!RAPIDJSON_IS_VALID_MEMBER(elem, GPF_TAG_VERSION, IsString))
            {
                return AZ::Failure(AZStd::string::format(
                        GPF_TAG_VERSION " string is missing for Gem with ID %s.",
                        idStr));
            }

            auto versionOutcome = GemVersion::ParseFromString(elem[GPF_TAG_VERSION].GetString());
            if (!versionOutcome)
            {
                return AZ::Failure(AZStd::string::format(
                        GPF_TAG_VERSION " string is invalid for Gem with ID %s: %s",
                        idStr, versionOutcome.GetError().c_str()));
            }
            GemVersion version = versionOutcome.GetValue();

            // gem path
            if (!RAPIDJSON_IS_VALID_MEMBER(elem, GPF_TAG_PATH, IsString))
            {
                return AZ::Failure(AZStd::string(GPF_TAG_PATH " string is required for Gem"));
            }
            const char* path = elem[GPF_TAG_PATH].GetString();

            EnableGem(ProjectGemSpecifier(id, version, path));
        }

        return AZ::Success();
    }
} // namespace Gems
