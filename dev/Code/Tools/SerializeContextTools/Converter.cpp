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

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Application.h>
#include <Converter.h>
#include <GemInfo.h>
#include <Utilities.h>

namespace AZ
{
    namespace SerializeContextTools
    {
        bool Converter::ConvertObjectStreamFiles(Application& application)
        {
            using namespace AZ::JsonSerializationResult;

            const AzFramework::CommandLine* commandLine = application.GetCommandLine();
            if (!commandLine)
            {
                AZ_Error("SerializeContextTools", false, "Command line not available.");
                return false;
            }
            
            JsonSerializerSettings convertSettings;
            convertSettings.m_keepDefaults = commandLine->HasSwitch("keepdefaults");
            convertSettings.m_registrationContext = application.GetJsonRegistrationContext();
            convertSettings.m_serializeContext = application.GetSerializeContext();
            if (!convertSettings.m_serializeContext)
            {
                AZ_Error("Convert", false, "No serialize context found.");
                return false;
            }
            if (!convertSettings.m_registrationContext)
            {
                AZ_Error("Convert", false, "No json registration context found.");
                return false;
            }
            AZStd::string logggingScratchBuffer;
            SetupLogging(logggingScratchBuffer, convertSettings.m_reporting, *commandLine);

            if (!commandLine->HasSwitch("ext"))
            {
                AZ_Error("Convert", false, "No extension provided through the 'ext' argument.");
                return false;
            }

            const AZStd::string& extension = commandLine->GetSwitchValue("ext", 0);
            bool isDryRun = commandLine->HasSwitch("dryrun");
            bool skipVerify = commandLine->HasSwitch("skipverify");

            JsonDeserializerSettings verifySettings;
            if (!skipVerify)
            {
                verifySettings.m_registrationContext = application.GetJsonRegistrationContext();
                verifySettings.m_serializeContext = application.GetSerializeContext();
                SetupLogging(logggingScratchBuffer, verifySettings.m_reporting, *commandLine);
            }


            bool result = true;
            rapidjson::StringBuffer scratchBuffer;

            AZStd::vector<AZStd::string> fileList = Utilities::ReadFileListFromCommandLine(application, "files");
            for (AZStd::string& filePath : fileList)
            {
                AZ_Printf("Convert", "Converting '%s'\n", filePath.c_str());

                PathDocumentContainer documents;
                auto callback = [&result, &documents, &extension, &convertSettings, &verifySettings, skipVerify]
                    (void* classPtr, const Uuid& classId, SerializeContext* /*context*/)
                {
                    rapidjson::Document document;
                    ResultCode parseResult = JsonSerialization::Store(document.SetObject(), document.GetAllocator(), classPtr, nullptr, classId, convertSettings);
                    if (parseResult.GetProcessing() != Processing::Halted)
                    {
                        if (skipVerify || VerifyConvertedData(document, classPtr, classId, verifySettings))
                        {
                            if (parseResult.GetOutcome() == Outcomes::DefaultsUsed)
                            {
                                AZ_Printf("Convert", "  File not converted as only default values were found.\n");
                            }
                            else
                            {
                                documents.emplace_back(GetClassName(classId, convertSettings.m_serializeContext), AZStd::move(document));
                            }
                        }
                        else
                        {
                            AZ_Printf("Convert", "  Verification of the converted file failed.\n");
                            result = false;
                        }
                    }
                    else
                    {
                        AZ_Printf("Convert", "  Conversion to JSON failed.\n");
                        result = false;
                    }
                    return true;
                };
                if (!Utilities::InspectSerializedFile(filePath, convertSettings.m_serializeContext, callback))
                {
                    AZ_Warning("Convert", false, "Failed to load '%s'. File may not contain an object stream.", filePath.c_str());
                    result = false;
                }
                
                // If there's only one file, then use the original name instead of the extended name
                AzFramework::StringFunc::Path::ReplaceExtension(filePath, extension.c_str());
                if (documents.size() == 1)
                {
                    AZ_Printf("Convert", "  Exporting to '%s'\n", filePath.c_str());
                    if (!isDryRun)
                    {
                        result = WriteDocumentToDisk(filePath, documents[0].second, scratchBuffer) && result;
                        scratchBuffer.Clear();
                    }
                }
                else
                {
                    AZStd::string fileName;
                    AzFramework::StringFunc::Path::GetFileName(filePath.c_str(), fileName);
                    for (PathDocumentPair& document : documents)
                    {
                        AZStd::string fileNameExtended = fileName;
                        fileNameExtended += '_';
                        fileNameExtended += document.first;
                        Utilities::SanitizeFilePath(fileNameExtended);
                        AZStd::string finalFilePath = filePath;
                        AzFramework::StringFunc::Path::ReplaceFullName(finalFilePath, fileNameExtended.c_str(), extension.c_str());

                        AZ_Printf("Convert", "  Exporting to '%s'\n", finalFilePath.c_str());
                        if (!isDryRun)
                        {
                            result = WriteDocumentToDisk(finalFilePath, document.second, scratchBuffer) && result;
                            scratchBuffer.Clear();
                        }
                    }
                }
            }

            return result;
        }

        bool Converter::ConvertApplicationDescriptor(Application& application)
        {
            const AzFramework::CommandLine* commandLine = application.GetCommandLine();
            if (!commandLine)
            {
                AZ_Error("SerializeContextTools", false, "Command line not available.");
                return false;
            }

            JsonSerializerSettings convertSettings;
            convertSettings.m_keepDefaults = commandLine->HasSwitch("keepdefaults");
            convertSettings.m_registrationContext = application.GetJsonRegistrationContext();
            convertSettings.m_serializeContext = application.GetSerializeContext();
            if (!convertSettings.m_serializeContext)
            {
                AZ_Error("Convert", false, "No serialize context found.");
                return false;
            }
            if (!convertSettings.m_registrationContext)
            {
                AZ_Error("Convert", false, "No json registration context found.");
                return false;
            }
            AZStd::string logggingScratchBuffer;
            SetupLogging(logggingScratchBuffer, convertSettings.m_reporting, *commandLine);

            JsonDeserializerSettings verifySettings;
            verifySettings.m_registrationContext = application.GetJsonRegistrationContext();
            verifySettings.m_serializeContext = application.GetSerializeContext();
            SetupLogging(logggingScratchBuffer, verifySettings.m_reporting, *commandLine);

            bool skipGems = commandLine->HasSwitch("skipgems");
            bool skipSystem = commandLine->HasSwitch("skipsystem");
            bool isDryRun = commandLine->HasSwitch("dryrun");

            const char* appRoot = const_cast<const Application&>(application).GetAppRoot();
            GemInfo gems;
            if (!gems.LoadGemInformation(appRoot, application.GetGameRoot()))
            {
                AZ_Error("Convert", false, "Unable to collect gem information.");
                return false;
            }

            PathDocumentContainer documents;
            bool result = true;
            const AZStd::string& filePath = application.GetConfigFilePath();
            AZ_Printf("Convert", "Reading '%s' for conversion.\n", filePath.c_str());
            AZStd::string configurationName;
            if (!AzFramework::StringFunc::Path::GetFileName(filePath.c_str(), configurationName) ||
                configurationName.empty())
            {
                AZ_Error("Convert", false, "Unable to extract configuration from '%s'.", filePath.c_str());
                return false;
            }
            // Most folder names start with a capital letter, but most files with lower case. As the configuration name
            // will be used as a folder, turn the first letter into a capital one.
            AZStd::to_upper(configurationName.begin(), configurationName.begin() + 1);

            const AZStd::string& projectFolder = application.GetGameRoot();
            
            auto callback = 
                [&result, skipGems, skipSystem, &configurationName, &projectFolder, &appRoot, &documents, &gems, &convertSettings, &verifySettings]
                (void* classPtr, const Uuid& classId, SerializeContext* /*context*/)
            {
                if (classId == azrtti_typeid<AZ::ComponentApplication::Descriptor>())
                {
                    if (!skipSystem)
                    {
                        result = ConvertSystemSettings(documents, *reinterpret_cast<AZ::ComponentApplication::Descriptor*>(classPtr), gems, 
                            configurationName, projectFolder, appRoot) && result;
                    }
                }
                else if (classId == azrtti_typeid<Entity>())
                {
                    if (!skipSystem)
                    {
                        result = ConvertSystemComponents(documents, *reinterpret_cast<Entity*>(classPtr), configurationName, projectFolder, 
                            convertSettings, verifySettings) && result;
                    }
                }
                else if (classId == azrtti_typeid<ModuleEntity>())
                {
                    if (!skipGems)
                    {
                        result = ConvertModuleComponents(documents, *reinterpret_cast<ModuleEntity*>(classPtr), gems, configurationName, 
                            convertSettings, verifySettings) && result;
                    }
                }
                else
                {
                    AZ_Warning("Convert", false, "Unable to process component in Application Descriptor of type '%s'.", 
                        classId.ToString<AZStd::string>().c_str());
                    result = false;
                }
                return true;
            };
            if (!Utilities::InspectSerializedFile(filePath, convertSettings.m_serializeContext, callback))
            {
                AZ_Warning("Convert", false, "Failed to load '%s'. File may not contain an object stream.", filePath.c_str());
                result = false;
            }
            
            if (!isDryRun)
            {
                rapidjson::StringBuffer scratchBuffer;
                for (auto& pathDocPair : documents)
                {
                    result = WriteDocumentToDisk(pathDocPair.first, pathDocPair.second, scratchBuffer) && result;
                    scratchBuffer.Clear();
                }
            }

            return result;
        }

        bool Converter::ConvertSystemSettings(PathDocumentContainer& documents, const ComponentApplication::Descriptor& descriptor, 
            const GemInfo& gems, const AZStd::string& configurationName, const AZStd::string& projectFolder, const AZStd::string& applicationRoot)
        {
            AZStd::string memoryFilePath = projectFolder;
            AzFramework::StringFunc::Path::ConstructFull(memoryFilePath.c_str(), "Registry", memoryFilePath, true);
            AzFramework::StringFunc::Path::ConstructFull(memoryFilePath.c_str(), configurationName.c_str(), memoryFilePath, false);
            
            AZStd::string modulesFilePath = memoryFilePath;
            AzFramework::StringFunc::Path::ConstructFull(memoryFilePath.c_str(), "memory.settings", memoryFilePath, false);
            AzFramework::StringFunc::Path::ConstructFull(modulesFilePath.c_str(), "module.settings", modulesFilePath, false);

            AZ_Printf("Convert", "  Exporting application descriptor to '%s' and '%s'.\n", memoryFilePath.c_str(), modulesFilePath.c_str());
            
            rapidjson::Document modulesDoc;
            modulesDoc.SetObject();
            rapidjson::Value moduleList(rapidjson::kArrayType);
            for (auto& module : descriptor.m_modules)
            {
                moduleList.PushBack(rapidjson::StringRef(module.m_dynamicLibraryPath.c_str()), modulesDoc.GetAllocator());
            }
            modulesDoc.AddMember(rapidjson::StringRef("Modules"), AZStd::move(moduleList), modulesDoc.GetAllocator());
            
            rapidjson::Value gemPathList(rapidjson::kArrayType);
            for (const AZStd::string& gemPath : gems.GetGemFolders())
            {
                if (gemPath.starts_with(applicationRoot.c_str()))
                {
                    gemPathList.PushBack(rapidjson::StringRef(gemPath.c_str() + applicationRoot.size()), modulesDoc.GetAllocator());
                }
                else
                {
                    AZ_Warning("Convert", false, "Gem '%s' was found outside the current project folder '%s'.", gemPath.c_str(), applicationRoot.c_str());
                }
            }
            modulesDoc.AddMember(rapidjson::StringRef("GemFolders"), AZStd::move(gemPathList), modulesDoc.GetAllocator());
            documents.emplace_back(AZStd::move(modulesFilePath), AZStd::move(modulesDoc));

            rapidjson::Document memoryDoc;
            memoryDoc.SetObject();
            memoryDoc.AddMember(rapidjson::StringRef("useExistingAllocator"), rapidjson::Value(descriptor.m_useExistingAllocator), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("grabAllMemory"), rapidjson::Value(descriptor.m_grabAllMemory), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("allocationRecords"), rapidjson::Value(descriptor.m_allocationRecords), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("allocationRecordsSaveNames"), rapidjson::Value(descriptor.m_allocationRecordsSaveNames), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("allocationRecordsAttemptDecodeImmediately"), rapidjson::Value(descriptor.m_allocationRecordsAttemptDecodeImmediately), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("recordingMode"), rapidjson::Value(descriptor.m_recordingMode), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("stackRecordLevels"), rapidjson::Value(descriptor.m_stackRecordLevels), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("autoIntegrityCheck"), rapidjson::Value(descriptor.m_autoIntegrityCheck), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("markUnallocatedMemory"), rapidjson::Value(descriptor.m_markUnallocatedMemory), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("doNotUsePools"), rapidjson::Value(descriptor.m_doNotUsePools), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("enableScriptReflection"), rapidjson::Value(descriptor.m_enableScriptReflection), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("pageSize"), rapidjson::Value(descriptor.m_pageSize), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("poolPageSize"), rapidjson::Value(descriptor.m_poolPageSize), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("blockAlignment"), rapidjson::Value(descriptor.m_memoryBlockAlignment), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("blockSize"), rapidjson::Value(descriptor.m_memoryBlocksByteSize), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("reservedOS"), rapidjson::Value(descriptor.m_reservedOS), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("reservedDebug"), rapidjson::Value(descriptor.m_reservedDebug), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("enableDrilling"), rapidjson::Value(descriptor.m_enableDrilling), memoryDoc.GetAllocator());
            documents.emplace_back(AZStd::move(memoryFilePath), AZStd::move(memoryDoc));
            
            return true;
        }

        bool Converter::ConvertSystemComponents(PathDocumentContainer& documents, const Entity& entity, const AZStd::string& configurationName, 
            const AZStd::string& projectFolder, const JsonSerializerSettings& convertSettings, const JsonDeserializerSettings& verifySettings)
        {
            using namespace AZ::JsonSerializationResult;

            AZStd::string systemFilePath = projectFolder;
            AzFramework::StringFunc::Path::ConstructFull(systemFilePath.c_str(), "Registry", systemFilePath, true);
            AzFramework::StringFunc::Path::ConstructFull(systemFilePath.c_str(), configurationName.c_str(), systemFilePath, false);
            AzFramework::StringFunc::Path::ConstructFull(systemFilePath.c_str(), "system.settings", systemFilePath, false);
            AZ_Printf("Convert", "  Exporting Entity to '%s'\n", systemFilePath.c_str());

            rapidjson::Document systemSettings;
            ResultCode result = JsonSerialization::Store(systemSettings.SetObject(), systemSettings.GetAllocator(), entity, convertSettings);
            if (result.GetProcessing() != Processing::Halted)
            {
                if (!VerifyConvertedData(systemSettings, &entity, azrtti_typeid(entity), verifySettings))
                {
                    // Errors will already be reported by VerifyConvertedData.
                    return false;
                }

                if (result.GetProcessing() != Processing::Halted)
                {
                    if (result.GetOutcome() == Outcomes::DefaultsUsed)
                    {
                        AZ_Printf("Convert", "  System settings not exported as only default values were found.\n");
                    }
                    else
                    {
                        documents.emplace_back(AZStd::move(systemFilePath), AZStd::move(systemSettings));
                    }
                }
                else
                {
                    AZ_Printf("Convert", "  System settings not exported.\n");
                }
                return true;
            }
            else
            {
                // Other errors will already have been reported by the JsonSerialierManager.
                return false;
            }
        }

        bool Converter::ConvertModuleComponents(PathDocumentContainer& documents, const ModuleEntity& entity, const GemInfo& gems, 
            const AZStd::string& configurationName, const JsonSerializerSettings& convertSettings, const JsonDeserializerSettings& verifySettings)
        {
            using namespace AZ::JsonSerializationResult;

            AZStd::string registryPath = gems.LocateGemPath(entity.m_moduleClassId);
            if (registryPath.empty())
            {
                AZ_Warning("Convert", false, "Unable to find gem folder to write output to for '%s'.", entity.GetName().c_str());
                return false;
            }
            AzFramework::StringFunc::Path::ConstructFull(registryPath.c_str(), "Registry", registryPath, true);
            AzFramework::StringFunc::Path::ConstructFull(registryPath.c_str(), configurationName.c_str(), registryPath, false);
            AzFramework::StringFunc::Path::ConstructFull(registryPath.c_str(), "gem.settings", registryPath, false);
            AZ_Printf("Convert", "  Exporting ModuleEntity to '%s'\n", registryPath.c_str());

            rapidjson::Document moduleSettings;
            ResultCode result = JsonSerialization::Store(moduleSettings.SetObject(), moduleSettings.GetAllocator(), entity, convertSettings);
            if (result.GetProcessing() != Processing::Halted)
            {
                if (!VerifyConvertedData(moduleSettings, &entity, azrtti_typeid(entity), verifySettings))
                {
                    // Errors will already be reported by VerifyConvertedData.
                    return false;
                }

                if (result.GetProcessing() != Processing::Halted)
                {
                    if (result.GetOutcome() == Outcomes::DefaultsUsed)
                    {
                        AZ_Printf("Convert", "  Gem settings not exported as only default values were found.\n");
                    }
                    else
                    {
                        documents.emplace_back(AZStd::move(registryPath), AZStd::move(moduleSettings));
                    }
                }
                else
                {
                    AZ_Printf("Convert", "  Gem settings not exported.\n");
                }
                return true;
            }
            else
            {
                // Other errors will already have been reported by the JsonSerialization.
                return false;
            }
        }

        bool Converter::VerifyConvertedData(rapidjson::Document& convertedData, const void* original, const Uuid& originalType, 
            const JsonDeserializerSettings& settings)
        {
            using namespace AZ::JsonSerializationResult;

            AZStd::any convertedDeserialized = settings.m_serializeContext->CreateAny(originalType);
            if (convertedDeserialized.empty())
            {
                AZ_Printf("Convert", "  Failed to deserialized from converted document.\n");
                return false;
            }

            ResultCode loadResult = JsonSerialization::Load(AZStd::any_cast<void>(&convertedDeserialized), originalType, convertedData, settings);
            if (loadResult.GetProcessing() == Processing::Halted)
            {
                AZ_Printf("Convert", "  Failed to verify converted document because it couldn't be loaded.\n");
                return false;
            }

            const SerializeContext::ClassData* data = settings.m_serializeContext->FindClassData(originalType);
            if (!data)
            {
                AZ_Printf("Convert", "  Failed to find serialization information for type '%s'.\n",
                    originalType.ToString<AZStd::string>().c_str());
                return false;
            }
            
            bool result = false;
            if (data->m_serializer)
            {
                result = data->m_serializer->CompareValueData(original, AZStd::any_cast<void>(&convertedDeserialized));
            }
            else
            {
                AZStd::vector<AZ::u8> originalData;
                AZ::IO::ByteContainerStream<decltype(originalData)> orignalStream(&originalData);
                AZ::Utils::SaveObjectToStream(orignalStream, AZ::ObjectStream::ST_BINARY, original, originalType);

                AZStd::vector<AZ::u8> loadedData;
                AZ::IO::ByteContainerStream<decltype(loadedData)> loadedStream(&loadedData);
                AZ::Utils::SaveObjectToStream(loadedStream, AZ::ObjectStream::ST_BINARY,
                    AZStd::any_cast<void>(&convertedDeserialized), convertedDeserialized.type());
                
                result = 
                    (originalData.size() == loadedData.size()) &&
                    (memcmp(originalData.data(), loadedData.data(), originalData.size()) == 0);
            }

            if (!result)
            {
                AZ_Printf("Convert", "  Differences found between the original and converted data.\n");
            }
            return result;
        }

        AZStd::string Converter::GetClassName(const Uuid& classId, SerializeContext* context)
        {
            const SerializeContext::ClassData* data = context->FindClassData(classId);
            if (data)
            {
                if (data->m_editData)
                {
                    return data->m_editData->m_name;
                }
                else
                {
                    return data->m_name;
                }
            }
            else
            {
                return classId.ToString<AZStd::string>();
            }
        }

        bool Converter::WriteDocumentToDisk(const AZStd::string& filename, const rapidjson::Document& document, rapidjson::StringBuffer& scratchBuffer)
        {
            IO::SystemFile outputFile;
            if (!outputFile.Open(filename.c_str(),
                IO::SystemFile::OpenMode::SF_OPEN_CREATE |
                IO::SystemFile::OpenMode::SF_OPEN_CREATE_PATH |
                IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY))
            {
                AZ_Error("SerializeContextTools", false, "Unable to open output file '%s'.", filename.c_str());
                return false;
            }

            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(scratchBuffer);
            document.Accept(writer);

            outputFile.Write(scratchBuffer.GetString(), scratchBuffer.GetSize());
            outputFile.Close();

            scratchBuffer.Clear();
            return true;
        }

        void Converter::SetupLogging(AZStd::string& scratchBuffer, JsonSerializationResult::JsonIssueCallback& callback,
            const AzFramework::CommandLine& commandLine)
        {
            if (commandLine.HasSwitch("verbose"))
            {
                callback = [&scratchBuffer](
                    AZStd::string_view message, JsonSerializationResult::ResultCode result, AZStd::string_view path)
                    ->JsonSerializationResult::ResultCode
                {
                    return VerboseLogging(scratchBuffer, message, result, path);
                };
            }
            else
            {
                callback = [&scratchBuffer](
                    AZStd::string_view message, JsonSerializationResult::ResultCode result, AZStd::string_view path)
                    ->JsonSerializationResult::ResultCode
                {
                    return SimpleLogging(scratchBuffer, message, result, path);
                };
            }
        }

        AZ::JsonSerializationResult::ResultCode Converter::VerboseLogging(AZStd::string& scratchBuffer, AZStd::string_view message,
            AZ::JsonSerializationResult::ResultCode result, AZStd::string_view path)
        {
            scratchBuffer.append(message.begin(), message.end());
            scratchBuffer.append("\n    Reason: ");
            result.AppendToString(scratchBuffer, path);
            scratchBuffer.append(".\n");
            AZ_Printf("SerializeContextTools", "%s", scratchBuffer.c_str());
            scratchBuffer.clear();
            
            return result;
        }

        AZ::JsonSerializationResult::ResultCode Converter::SimpleLogging(AZStd::string& scratchBuffer, AZStd::string_view message,
            AZ::JsonSerializationResult::ResultCode result, AZStd::string_view path)
        {
            using namespace JsonSerializationResult;

            if (result.GetProcessing() != Processing::Completed)
            {
                scratchBuffer.append(message.begin(), message.end());
                scratchBuffer.append(" @ ");
                scratchBuffer.append(path.begin(), path.end());
                scratchBuffer.append(".\n");
                AZ_Printf("SerializeContextTools", "%s", scratchBuffer.c_str());

                scratchBuffer.clear();
            }
            return result;
        }
    } // namespace SerializeContextTools
} // namespace AZ
