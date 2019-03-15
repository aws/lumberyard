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

#include <Tests/TestTypes.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/IO/Streamer.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Component/ComponentApplication.h>

namespace UnitTest
{
    using namespace AZ;

    class SetRestoreFileIOBaseRAII
    {
    public:
        SetRestoreFileIOBaseRAII(AZ::IO::FileIOBase& fileIO)
            : m_prevFileIO(AZ::IO::FileIOBase::GetInstance())
        {
            AZ::IO::FileIOBase::SetInstance(&fileIO);
        }

        ~SetRestoreFileIOBaseRAII()
        {
            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
        }
    private:
        AZ::IO::FileIOBase* m_prevFileIO;
    };

    class GenAppDescriptors
        : public AllocatorsFixture
    {
    public:
        void run()
        {
            struct Config
            {
                const char* platformName;
                const char* configName;
                const char* libSuffix;
            };

            static const Config configs[] =
            {
                { "",           "Default",          ""},
                { "Win64",      "Debug",            ".dll"},
                { "Win64",      "DebugOpt",         ".dll"},
                { "Win64",      "Release",          ".dll"},
                { "Win64",      "DebugEditor",      ".dll"},
                { "Win64",      "DebugOptEditor",   ".dll"},
                { "Win64",      "ReleaseEditor",    ".dll"},
#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/GenAppDescriptors_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/GenAppDescriptors_cpp_provo.inl"
    #endif
#elif defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#if defined(TOOLS_SUPPORT_XENIA)
    #include "Xenia/GenAppDescriptors_cpp_xenia.inl"
#endif
#if defined(TOOLS_SUPPORT_PROVO)
    #include "Provo/GenAppDescriptors_cpp_provo.inl"
#endif
#endif
                { "Linux",      "Debug",            ".so"},
                { "Linux",      "DebugOpt",         ".so"},
                { "Linux",      "Release",          ".so"},
                { "Android",    "Debug",            ".so"},
                { "Android",    "DebugOpt",         ".so"},
                { "Android",    "Release",          ".so"},
                { "iOS",        "Debug",            ".dylib"},
                { "iOS",        "DebugOpt",         ".dylib"},
                { "iOS",        "Release",          ".dylib"},
                { "OSX",        "Debug",            ".dylib"},
                { "OSX",        "DebugOpt",         ".dylib"},
                { "OSX",        "Release",          ".dylib"},
            };

            ComponentApplication app;

            IO::Streamer::Create(IO::Streamer::Descriptor());

            AllocatorInstance<AZ::ThreadPoolAllocator>::Create(AZ::ThreadPoolAllocator::Descriptor());

            SerializeContext serializeContext;
            AZ::ComponentApplication::Descriptor::Reflect(&serializeContext, &app);
            AZ::Entity::Reflect(&serializeContext);
            DynamicModuleDescriptor::Reflect(&serializeContext);

            AZ::Entity dummySystemEntity(AZ::SystemEntityId, "SystemEntity");

            for (const auto& config : configs)
            {
                AZ::ComponentApplication::Descriptor descriptor;

                if (config.libSuffix && config.libSuffix[0])
                {
                    FakePopulateModules(descriptor, config.libSuffix);
                }

                const AZStd::string filename = AZStd::string::format("LYConfig_%s%s.xml", config.platformName, config.configName);

                IO::VirtualStream* fileStream = IO::Streamer::Instance().RegisterFileStream(filename.c_str(), IO::OpenMode::ModeWrite, false);
                AZ_Assert(fileStream, "Failed to create output file %s", filename.c_str());
                IO::StreamerStream stream(fileStream, false);
                ObjectStream* objStream = ObjectStream::Create(&stream, serializeContext, ObjectStream::ST_XML);
                bool descWriteOk = objStream->WriteClass(&descriptor);
                (void)descWriteOk;
                AZ_Warning("ComponentApplication", descWriteOk, "Failed to write memory descriptor to application descriptor file %s!", filename.c_str());
                bool entityWriteOk = objStream->WriteClass(&dummySystemEntity);
                (void)entityWriteOk;
                AZ_Warning("ComponentApplication", entityWriteOk, "Failed to write system entity to application descriptor file %s!", filename.c_str());
                bool flushOk = objStream->Finalize();
                (void)flushOk;
                AZ_Warning("ComponentApplication", flushOk, "Failed finalizing application descriptor file %s!", filename.c_str());
                IO::Streamer::Instance().UnRegisterStream(fileStream);
            }

            IO::Streamer::Destroy();
            AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
        }

        void FakePopulateModules(AZ::ComponentApplication::Descriptor& desc, const char* libSuffix)
        {
            static const char* modules[] =
            {
                "LySystemModule",
            };

            if (desc.m_modules.empty())
            {
                for (const char* module : modules)
                {
                    desc.m_modules.push_back();
                    desc.m_modules.back().m_dynamicLibraryPath = module;
                    desc.m_modules.back().m_dynamicLibraryPath += libSuffix;
                }
            }
        }
    };

    TEST_F(GenAppDescriptors, Test)
    {
        AZ::IO::LocalFileIO fileIO;
        SetRestoreFileIOBaseRAII restoreFileIOScope(fileIO);
        run();
    }
}
