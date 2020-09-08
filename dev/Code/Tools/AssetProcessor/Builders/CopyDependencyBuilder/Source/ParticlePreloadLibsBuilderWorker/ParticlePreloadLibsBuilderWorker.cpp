/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "ParticlePreloadLibsBuilderWorker.h"

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace CopyDependencyBuilder
{
    ParticlePreloadLibsBuilderWorker::ParticlePreloadLibsBuilderWorker()
        : CopyDependencyBuilderWorker("PreloadLib", true, true)
    {
    }

    void ParticlePreloadLibsBuilderWorker::RegisterBuilderWorker()
    {
        AssetBuilderSDK::AssetBuilderDesc preloadLibBuilderDescriptor;
        preloadLibBuilderDescriptor.m_name = "ParticlePreloadLibsBuilderWorker";
        // The particle manager treats any requests to load a file starting with the "@" character as a preload file, stripping
        // that character and loading based on the rest of the string. This means there are zero markers in the file path or
        // contents of a file to let the builder know it's a preload library.
        // There is one hardcoded load to a preload library, '@PreloadLibs' in in 3dEngineLoad.cpp, if the preload all particles cvar is not set.
        // This worker only supports this PreloadLib, if you need to support more, update this builder pattern for your other preload files, or
        // add your other preload lib to the root or level PreloadLib.txt, it will load other preload libs recursively.
        preloadLibBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*preloadlibs.txt", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        preloadLibBuilderDescriptor.m_busId = azrtti_typeid<ParticlePreloadLibsBuilderWorker>();
        preloadLibBuilderDescriptor.m_version = 2;
        preloadLibBuilderDescriptor.m_createJobFunction =
            AZStd::bind(&ParticlePreloadLibsBuilderWorker::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        preloadLibBuilderDescriptor.m_processJobFunction =
            AZStd::bind(&ParticlePreloadLibsBuilderWorker::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

        BusConnect(preloadLibBuilderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, preloadLibBuilderDescriptor);
    }

    void ParticlePreloadLibsBuilderWorker::UnregisterBuilderWorker()
    {
        BusDisconnect();
    }

    bool ParticlePreloadLibsBuilderWorker::ParseProductDependencies(
        const AssetBuilderSDK::ProcessJobRequest& request,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& /*productDependencies*/,
        AssetBuilderSDK::ProductPathDependencySet& pathDependencies)
    {
        // ParticleManager.cpp's CParticleManager::LoadLibrary functions like so:
        // * It has two parameters used to determine what to load:
        //   * sParticlesLibrary: The library to attempt to load
        //   * sParticlesLibraryFile: A specific path to attempt to load the library from.
        // * If sParticlesLibrary has the '*' character it in, it is treated as a wildcard load:
        //   * If sParticlesLibraryFile is not set:
        //     * It attempts to load sParticlesLibrary from level local and global paths, recursively calling LoadLibrary.
        //   * If sParticlesLibraryFile is set:
        //     * It requests all matching file paths to PathUtil::Make(sParticlesLibraryFile,sParticlesLibrary,"xml") from the pak file system.
        //     * It recursively loads all of these files through a call to LoadLibrary.
        // * If it was not a wildcard load, and the first character in sParticlesLibrary is '@', it is a preload library load.
        //   * The system attempts to load a preload lib list at the given sParticlesLibraryFile + sParticlesLibrary path, relative to the level.
        //   * The system attempts to load a preload lib list at the root effects subpath, EFFECTS_SUBPATH ("GameRoot/Libs/Particles") + sParticlesLibrary
        // * If it wasn't a wildcard load or a preload lib load, then it attempts to load sParticlesLibrary as a particle library.
        //   * First, it gets an XmlNodeRef by loading files based on the state of sParticlesLibrary and sParticlesLibraryFile
        //     * If sParticlesLibraryFile is set, it loads sParticlesLibraryFile as an XML file into XmlNodeRef
        //     * If sParticlesLibraryFile was not set, then it calls ReadLibrary on sParticlesLibrary.
        //       * If the e_ParticlesUseLevelSpecificLibs cvar is set:
        //         * ReadLibrary attempts to load from all available library paths under the level (LevelPath/EFFECTS_SUBPATH/sParticlesLibrary.xml), loading the first one it finds.
        //       * If e_ParticlesUseLevelSpecificLibs is not set:
        //         * ReadLibrary attempts to load EFFECTS_SUBPATH/sParticlesLibrary.xml from the root.

        AZ::IO::FileIOStream fileStream;
        if (!fileStream.Open(request.m_fullPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            return false;
        }
        AZ::IO::SizeType length = fileStream.GetLength();
        if (length == 0)
        {
            return true;
        }

        AZStd::vector<char> charBuffer;
        charBuffer.resize_no_construct(length + 1);
        fileStream.Read(length, charBuffer.data());
        charBuffer.back() = 0;

        // Using CryString would more accurately match LoadPreloadLibList's behavior,
        // but it's not possible to include CryString from a builder.
        AZStd::vector<AZStd::string> fileLines;
        AzFramework::StringFunc::Tokenize(charBuffer.data(), fileLines, "\r\n");

        // Defining this locally because the other usage of the marker is deep in Cry code,
        // there isn't a great place to share code between legacy Cry logic and AZ logic.
        const char preloadLibMarker = '@';
        const char wildcardMarker[] = "*";
        for(AZStd::string& line : fileLines)
        {
            // We don't support wildcards in product dependencies yet.
            if (strstr(line.c_str(), wildcardMarker))
            {
                AZ_Error("ParticlePreloadLibsBuilderWorker",
                    false,
                    "Wildcards in product dependencies are not yet supported in preload files for particles. "
                    "Contact Lumberyard support if this is a feature you need. Wildcard line was '%s', found in file '%s'",
                    line.c_str(),
                    request.m_fullPath.c_str());
            }
            else if(line[0] == preloadLibMarker)
            {
                if (line.length() == 1)
                {
                    AZ_Error("ParticlePreloadLibsBuilderWorker",
                        false,
                        "Preload lib file '%s' references an invalid preload lib file. Please remove the line containing just '@'.",
                        request.m_fullPath.c_str());
                    continue;
                }
                line = line.substr(1, line.length());
                const char preloadLibExtension[] = ".txt";
                SetExtension(line, preloadLibExtension);
                SetToGlobalPath(line);
                
                pathDependencies.emplace(line.c_str(), AssetBuilderSDK::ProductPathDependencyType::ProductFile);

                // Level relative path dependencies are not yet being emitted. We don't yet support special markup,
                // but anything that matches "@levelPath@/Libs/Particles/{line}" should be a product dependency.
            }
            else
            {
                const char particleLibExtension[] = ".xml";
                SetExtension(line, particleLibExtension);
                SetToGlobalPath(line);
                pathDependencies.emplace(line.c_str(), AssetBuilderSDK::ProductPathDependencyType::ProductFile);

                // Level relative path dependencies are not yet being emitted. We don't yet support special markup,
                // but anything that matches "@levelPath@/Libs/Particles/{line}" should be a product dependency.
            }
        }
        return true;
    }

    void ParticlePreloadLibsBuilderWorker::SetExtension(AZStd::string& file, const char* newExtension) const
    {
        if (AzFramework::StringFunc::Path::HasExtension(file.c_str()))
        {
            AzFramework::StringFunc::Path::ReplaceExtension(file, newExtension);
        }
        else
        {
            AzFramework::StringFunc::Append(file, newExtension);
        }
    }

    void ParticlePreloadLibsBuilderWorker::SetToGlobalPath(AZStd::string& file) const
    {
        const char librarySubpath[] = "libs";
        const char effectsSubpath[] = "particles";
        AZStd::string gameProjectLibraryEffectsSubpath;
        AzFramework::StringFunc::Path::Join(librarySubpath, effectsSubpath, gameProjectLibraryEffectsSubpath);

        AzFramework::StringFunc::Path::Join(gameProjectLibraryEffectsSubpath.c_str(), file.c_str(), file);
        AZStd::replace(file.begin(), file.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);
    }
}
