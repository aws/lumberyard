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

#include <PythonLogSymbolsComponent.h>

#include <Source/PythonCommon.h>
#include <Source/PythonUtility.h>
#include <Source/PythonTypeCasters.h>
#include <Source/PythonProxyBus.h>
#include <Source/PythonProxyObject.h>
#include <pybind11/embed.h>

#include <AzCore/PlatformDef.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Math/VectorFloat.h>

#include <AzFramework/CommandLine/CommandRegistrationBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace EditorPythonBindings
{
    namespace Internal
    {
        struct FileHandle final
        {
            explicit FileHandle(AZ::IO::HandleType handle)
                : m_handle(handle)
            {}

            ~FileHandle()
            {
                Close();
            }

            void Close()
            {
                if (IsValid())
                {
                    AZ::IO::FileIOBase::GetInstance()->Close(m_handle);
                }
                m_handle = AZ::IO::InvalidHandle;
            }

            bool IsValid() const
            {
                return m_handle != AZ::IO::InvalidHandle;
            }

            operator AZ::IO::HandleType() const { return m_handle; }

            AZ::IO::HandleType m_handle;
        };

        void Indent(int level, AZStd::string& buffer)
        {
            buffer.append(level * 4, ' ');
        }

        void AddCommentBlock(int level, const AZStd::string& comment, AZStd::string& buffer)
        {
            Indent(level, buffer);
            AzFramework::StringFunc::Append(buffer, "\"\"\"\n");
            Indent(level, buffer);
            AzFramework::StringFunc::Append(buffer, comment.c_str());
            Indent(level, buffer);
            AzFramework::StringFunc::Append(buffer, "\"\"\"\n");
        }

        AZStd::string_view FetchPythonTypeAndTraits(const AZ::TypeId& typeId, AZ::u32 traits)
        {
            if (AZ::AzTypeInfo<AZStd::string_view>::Uuid() == typeId || 
                AZ::AzTypeInfo<AZStd::string>::Uuid() == typeId)
            {
                return "str";
            }
            else if (AZ::AzTypeInfo<char>::Uuid() == typeId && 
                    traits & AZ::BehaviorParameter::TR_POINTER && 
                    traits & AZ::BehaviorParameter::TR_CONST)
            {
                return "str";
            }
            else if (AZ::AzTypeInfo<float>::Uuid() == typeId || 
                     AZ::AzTypeInfo<double>::Uuid() == typeId ||
                     AZ::AzTypeInfo<AZ::VectorFloat>::Uuid() == typeId)
            {
                return "float";
            }
            else if (AZ::AzTypeInfo<bool>::Uuid() == typeId)
            {
                return "bool";
            }
            else if (AZ::AzTypeInfo<AZ::s8>::Uuid() == typeId ||
                     AZ::AzTypeInfo<AZ::u8>::Uuid() == typeId ||
                     AZ::AzTypeInfo<AZ::s16>::Uuid() == typeId ||
                     AZ::AzTypeInfo<AZ::u16>::Uuid() == typeId ||
                     AZ::AzTypeInfo<AZ::s32>::Uuid() == typeId ||
                     AZ::AzTypeInfo<AZ::u32>::Uuid() == typeId ||
                     AZ::AzTypeInfo<AZ::s64>::Uuid() == typeId ||
                     AZ::AzTypeInfo<AZ::u64>::Uuid() == typeId)
            {
                return "int";
            }
            return "";
        }

        AZStd::string_view FetchPythonType(const AZ::BehaviorParameter& param)
        {
            return FetchPythonTypeAndTraits(param.m_typeId, param.m_traits);
        }
    }

    void PythonLogSymbolsComponent::Activate()
    {
        PythonSymbolEventBus::Handler::BusConnect();
        EditorPythonBindingsNotificationBus::Handler::BusConnect();
        AZ::Interface<AzToolsFramework::EditorPythonConsoleInterface>::Register(this);
    }

    void PythonLogSymbolsComponent::Deactivate()
    {
        AZ::Interface<AzToolsFramework::EditorPythonConsoleInterface>::Unregister(this);
        PythonSymbolEventBus::Handler::BusDisconnect();
        EditorPythonBindingsNotificationBus::Handler::BusDisconnect();
    }

    void PythonLogSymbolsComponent::OnPostInitialize()
    {
        m_basePath.clear();
        if (AZ::IO::FileIOBase::GetInstance()->GetAlias("@user@"))
        {
            // clear out the previous symbols path
            char hydraPath[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@user@/python_symbols", hydraPath, AZ_MAX_PATH_LEN);
            AZ::IO::FileIOBase::GetInstance()->CreatePath(hydraPath);
            m_basePath = hydraPath;
        }
        EditorPythonBindingsNotificationBus::Handler::BusDisconnect();
    }

    void PythonLogSymbolsComponent::WriteMethod(AZ::IO::HandleType handle, AZStd::string_view methodName, const AZ::BehaviorMethod& behaviorMethod, const AZ::BehaviorClass* behaviorClass)
    {
        AZStd::string buffer;
        int indentLevel = 0;
        AZStd::vector<AZStd::string> pythonArgs;
        const bool isMemberLike = behaviorClass ? PythonProxyObjectManagement::IsMemberLike(behaviorMethod, behaviorClass->m_typeId) : false;

        if (isMemberLike)
        {
            indentLevel = 1;
            Internal::Indent(indentLevel, buffer);
            pythonArgs.emplace_back("self");
        }
        else
        {
            indentLevel = 0;
        }

        AzFramework::StringFunc::Append(buffer, "def ");
        if (isMemberLike || !behaviorClass)
        {
            AzFramework::StringFunc::Append(buffer, methodName.data());
        }
        else
        {
            AzFramework::StringFunc::Append(buffer, behaviorClass->m_name.c_str());
            AzFramework::StringFunc::Append(buffer, "_");
            AzFramework::StringFunc::Append(buffer, methodName.data());
        }
        AzFramework::StringFunc::Append(buffer, "( ");

        AZStd::string bufferArg;
        for (size_t argIndex = 0; argIndex < behaviorMethod.GetNumArguments(); ++argIndex)
        {
            const AZStd::string* name = behaviorMethod.GetArgumentName(argIndex);
            if (!name || name->empty())
            {
                bufferArg = AZStd::string::format(" arg%d", argIndex);
            }
            else
            {
                bufferArg = *name;
            }

            AZStd::string_view type = Internal::FetchPythonType(*behaviorMethod.GetArgument(argIndex));
            if (!type.empty())
            {
                AzFramework::StringFunc::Append(bufferArg, ": ");
                AzFramework::StringFunc::Append(bufferArg, type.data());
            }

            pythonArgs.push_back(bufferArg);
            bufferArg.clear();
        }

        AZStd::string argsList;
        AzFramework::StringFunc::Join(buffer, pythonArgs.begin(), pythonArgs.end(), ",");
        AzFramework::StringFunc::Append(buffer, ") -> None:\n");
        Internal::Indent(indentLevel + 1, buffer);
        AzFramework::StringFunc::Append(buffer, "pass\n\n");

        AZ::IO::FileIOBase::GetInstance()->Write(handle, buffer.c_str(), buffer.size());
    }

    void PythonLogSymbolsComponent::WriteProperty(AZ::IO::HandleType handle, int level, AZStd::string_view propertyName, const AZ::BehaviorProperty& property, const AZ::BehaviorClass * behaviorClass)
    {
        AZStd::string buffer;

        // property declaration
        Internal::Indent(level, buffer);
        AzFramework::StringFunc::Append(buffer, "@property\n");

        Internal::Indent(level, buffer);
        AzFramework::StringFunc::Append(buffer, "def ");
        AzFramework::StringFunc::Append(buffer, propertyName.data());
        AzFramework::StringFunc::Append(buffer, "(self) -> ");

        AZStd::string_view type = Internal::FetchPythonTypeAndTraits(property.GetTypeId(), 0);
        if (type.empty())
        {
            AzFramework::StringFunc::Append(buffer, "Any");
        }
        else
        {
            AzFramework::StringFunc::Append(buffer, type.data());
        }
        AzFramework::StringFunc::Append(buffer, ":\n");
        Internal::Indent(level + 1, buffer);
        AzFramework::StringFunc::Append(buffer, "pass\n");

        AZ::IO::FileIOBase::GetInstance()->Write(handle, buffer.c_str(), buffer.size());
    }

    void PythonLogSymbolsComponent::LogClass(AZStd::string_view moduleName, AZ::BehaviorClass* behaviorClass)
    {
        Internal::FileHandle fileHandle(OpenModuleAt(moduleName));
        if (fileHandle.IsValid())
        {
            // Behavior Class types with member methods  and properties
            AZStd::string buffer;
            AzFramework::StringFunc::Append(buffer, "class ");
            AzFramework::StringFunc::Append(buffer, behaviorClass->m_name.c_str());
            AzFramework::StringFunc::Append(buffer, ":\n");
            AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, buffer.c_str(), buffer.size());
            buffer.clear();

            if (behaviorClass->m_methods.empty() && behaviorClass->m_properties.empty())
            {
                AZStd::string body{ "    # behavior class type with no methods or properties \n" };
                Internal::Indent(1, body);
                AzFramework::StringFunc::Append(body, "pass\n\n");
                AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, body.c_str(), body.size());
            }
            else
            {
                for (const auto& properyEntry : behaviorClass->m_properties)
                {
                    AZ::BehaviorProperty* property = properyEntry.second;
                    AZStd::string propertyName{ properyEntry.first };
                    Scope::FetchScriptName(property->m_attributes, propertyName);
                    WriteProperty(fileHandle, 1, propertyName, *property, behaviorClass);
                }

                for (const auto& methodEntry : behaviorClass->m_methods)
                {
                    AZ::BehaviorMethod* method = methodEntry.second;
                    if (method && PythonProxyObjectManagement::IsMemberLike(*method, behaviorClass->m_typeId))
                    {
                        AZStd::string baseMethodName{ methodEntry.first };
                        Scope::FetchScriptName(method->m_attributes, baseMethodName);
                        WriteMethod(fileHandle, baseMethodName, *method, behaviorClass);
                    }
                }
            }
        }
    }

    void PythonLogSymbolsComponent::LogClassMethod(AZStd::string_view moduleName, AZStd::string_view globalMethodName, AZ::BehaviorClass* behaviorClass, AZ::BehaviorMethod* behaviorMethod)
    {
        AZ_UNUSED(behaviorClass);
        Internal::FileHandle fileHandle(OpenModuleAt(moduleName));
        if (fileHandle.IsValid())
        {
            WriteMethod(fileHandle, globalMethodName, *behaviorMethod, nullptr);
        }
    }

    void PythonLogSymbolsComponent::LogBus(AZStd::string_view moduleName, AZStd::string_view busName, AZ::BehaviorEBus* behaviorEBus)
    {
        if (behaviorEBus->m_events.empty())
        {
            return;
        }

        Internal::FileHandle fileHandle(OpenModuleAt(moduleName));
        if (fileHandle.IsValid())
        {
            AZStd::string buffer;

            const auto& eventSenderEntry = behaviorEBus->m_events.begin();
            const AZ::BehaviorEBusEventSender& sender = eventSenderEntry->second;

            AzFramework::StringFunc::Append(buffer, "def ");
            AzFramework::StringFunc::Append(buffer, busName.data());
            if (sender.m_event)
            {
                AZStd::string_view addressType = Internal::FetchPythonType(behaviorEBus->m_idParam);
                if (addressType.empty())
                {
                    AzFramework::StringFunc::Append(buffer, "(busCallType: int, busEventName: str, address: Any, args: Tuple())");
                }
                else
                {
                    AzFramework::StringFunc::Append(buffer, "(busCallType: int, busEventName: str, address: ");
                    AzFramework::StringFunc::Append(buffer, addressType.data());
                    AzFramework::StringFunc::Append(buffer, ", args: Tuple())");
                }
            }
            else
            {
                AzFramework::StringFunc::Append(buffer, "(busCallType: int, busEventName: str, args: Tuple())");
            }
            AzFramework::StringFunc::Append(buffer, " -> None:\n");
            AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, buffer.c_str(), buffer.size());
            buffer.clear();

            // record the event names of the behavior can sent
            AZStd::string comment = behaviorEBus->m_toolTip;
            for (const auto& eventEntry : behaviorEBus->m_events)
            {
                const AZStd::string& eventName = eventEntry.first;
                AzFramework::StringFunc::Append(comment, "\n");
                Internal::Indent(1, comment);
                AzFramework::StringFunc::Append(comment, eventName.c_str());
            }
            Internal::AddCommentBlock(1, comment, buffer);

            Internal::Indent(1, buffer);
            AzFramework::StringFunc::Append(buffer, "pass\n\n");

            AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, buffer.c_str(), buffer.size());

            // can the EBus create & destroy a handler?
            if (behaviorEBus->m_createHandler && behaviorEBus->m_destroyHandler)
            {
                buffer.clear();
                AzFramework::StringFunc::Append(buffer, "def ");
                AzFramework::StringFunc::Append(buffer, busName.data());
                AzFramework::StringFunc::Append(buffer, "Handler() -> None:\n");
                Internal::Indent(1, buffer);
                AzFramework::StringFunc::Append(buffer, "pass\n\n");
                AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, buffer.c_str(), buffer.size());
            }
        }
    }

    void PythonLogSymbolsComponent::LogGlobalMethod(AZStd::string_view moduleName, AZStd::string_view methodName, AZ::BehaviorMethod* behaviorMethod)
    {
        Internal::FileHandle fileHandle(OpenModuleAt(moduleName));
        if (fileHandle.IsValid())
        {
            WriteMethod(fileHandle, methodName, *behaviorMethod, nullptr);
        }

        auto functionMapIt = m_globalFunctionMap.find(moduleName);
        if (functionMapIt == m_globalFunctionMap.end())
        {
            auto moduleSetIt = m_moduleSet.find(moduleName);
            if (moduleSetIt != m_moduleSet.end())
            {
                m_globalFunctionMap[*moduleSetIt] = { AZStd::make_pair(behaviorMethod, methodName) };
            }
        }
        else
        {
            GlobalFunctionList& globalFunctionList = functionMapIt->second;
            globalFunctionList.emplace_back(AZStd::make_pair(behaviorMethod, methodName));
        }
    }

    void PythonLogSymbolsComponent::LogGlobalProperty(AZStd::string_view moduleName, AZStd::string_view propertyName, AZ::BehaviorProperty* behaviorProperty)
    {
        if (!behaviorProperty->m_getter || !behaviorProperty->m_getter->GetResult())
        {
            return;
        }

        Internal::FileHandle fileHandle(OpenModuleAt(moduleName));
        if (fileHandle.IsValid())
        {
            AZStd::string buffer;

            // add header 
            AZ::u64 filesize = 0;
            AZ::IO::FileIOBase::GetInstance()->Size(fileHandle, filesize);
            if (filesize == 0)
            {
                AzFramework::StringFunc::Append(buffer, "class property():\n");
            }

            Internal::Indent(1, buffer);
            AzFramework::StringFunc::Append(buffer, propertyName.data());
            AzFramework::StringFunc::Append(buffer, ": ClassVar[");

            const AZ::BehaviorParameter* resultParam = behaviorProperty->m_getter->GetResult();
            AZStd::string_view type = Internal::FetchPythonTypeAndTraits(resultParam->m_typeId, resultParam->m_traits);
            if (type.empty())
            {
                AzFramework::StringFunc::Append(buffer, "Any");
            }
            else
            {
                AzFramework::StringFunc::Append(buffer, type.data());
            }
            AzFramework::StringFunc::Append(buffer, "] = None");

            if (behaviorProperty->m_getter && !behaviorProperty->m_setter)
            {
                AzFramework::StringFunc::Append(buffer, " # read only");
            }
            AzFramework::StringFunc::Append(buffer, "\n");

            AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, buffer.c_str(), buffer.size());
        }
    }

    void PythonLogSymbolsComponent::Finalize()
    {
        Internal::FileHandle fileHandle(OpenInitFileAt("azlmbr.bus"));
        if (fileHandle)
        {
            AZStd::string buffer;
            AzFramework::StringFunc::Append(buffer, "# Bus dispatch types:\n");
            AzFramework::StringFunc::Append(buffer, "from typing_extensions import Final\n");
            AzFramework::StringFunc::Append(buffer, "Broadcast: Final[int] = 0\n");
            AzFramework::StringFunc::Append(buffer, "Event: Final[int] = 1\n");
            AzFramework::StringFunc::Append(buffer, "QueueBroadcast: Final[int] = 2\n");
            AzFramework::StringFunc::Append(buffer, "QueueEvent: Final[int] = 3\n");
            AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, buffer.c_str(), buffer.size());
        }
        fileHandle.Close();
    }

    void PythonLogSymbolsComponent::GetModuleList(AZStd::vector<AZStd::string_view>& moduleList) const
    {
        moduleList.clear();
        moduleList.reserve(m_moduleSet.size());
        AZStd::copy(m_moduleSet.begin(), m_moduleSet.end(), AZStd::back_inserter(moduleList));
    }

    void PythonLogSymbolsComponent::GetGlobalFunctionList(GlobalFunctionCollection& globalFunctionCollection) const
    {
        globalFunctionCollection.clear();

        for (const auto& globalFunctionMapEntry : m_globalFunctionMap)
        {
            const AZStd::string_view moduleName{ globalFunctionMapEntry.first };
            const GlobalFunctionList& moduleFunctionList = globalFunctionMapEntry.second;

            AZStd::transform(moduleFunctionList.begin(), moduleFunctionList.end(), AZStd::back_inserter(globalFunctionCollection), [moduleName](auto& entry) -> auto
            {
                const GlobalFunctionEntry& globalFunctionEntry = entry;
                const AZ::BehaviorMethod* behaviorMethod = entry.first;
                return AzToolsFramework::EditorPythonConsoleInterface::GlobalFunction({ moduleName, globalFunctionEntry.second, behaviorMethod->m_debugDescription });
            });
        }
    }

    AZ::IO::HandleType PythonLogSymbolsComponent::OpenInitFileAt(AZStd::string_view moduleName)
    {
        if (m_basePath.empty())
        {
            return AZ::IO::InvalidHandle;
        }

        // creates the __init__.py file in this path
        AZStd::string modulePath(moduleName);
        AzFramework::StringFunc::Replace(modulePath, ".", AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);

        AZStd::string initFile;
        AzFramework::StringFunc::Path::Join(m_basePath.c_str(), modulePath.c_str(), initFile);
        AzFramework::StringFunc::Append(initFile, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        AzFramework::StringFunc::Append(initFile, "__init__.pyi");
        
        AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeText | AZ::IO::OpenMode::ModeWrite;
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::IO::Result result = AZ::IO::FileIOBase::GetInstance()->Open(initFile.c_str(), openMode, fileHandle);
        AZ_Warning("python", result, "Could not open %s to write Python symbols.", initFile.c_str());
        if (result)
        {
            return fileHandle;
        }

        return AZ::IO::InvalidHandle;
    }

    AZ::IO::HandleType PythonLogSymbolsComponent::OpenModuleAt(AZStd::string_view moduleName)
    {
        if (m_basePath.empty())
        {
            return AZ::IO::InvalidHandle;
        }

        bool resetFile = false;
        if (m_moduleSet.find(moduleName) == m_moduleSet.end())
        {
            m_moduleSet.insert(moduleName);
            resetFile = true;
        }

        AZStd::vector<AZStd::string> moduleParts;
        AzFramework::StringFunc::Tokenize(moduleName.data(), moduleParts, '.');

        // prepare target PYI file
        AZStd::string targetModule = moduleParts.back();
        moduleParts.pop_back();
        AzFramework::StringFunc::Append(targetModule, ".pyi");

        AZStd::string modulePath;
        AzFramework::StringFunc::Append(modulePath, m_basePath.c_str());
        AzFramework::StringFunc::Append(modulePath, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        AzFramework::StringFunc::Join(modulePath, moduleParts.begin(), moduleParts.end(), AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);

        // prepare the path
        AZ::IO::FileIOBase::GetInstance()->CreatePath(modulePath.c_str());

        // assemble the file path
        AzFramework::StringFunc::Append(modulePath, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        AzFramework::StringFunc::Append(modulePath, targetModule.c_str());
        AzFramework::StringFunc::AssetDatabasePath::Normalize(modulePath);

        AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeText;
        if (AZ::IO::SystemFile::Exists(modulePath.c_str()))
        {
            openMode |= (resetFile) ? AZ::IO::OpenMode::ModeWrite : AZ::IO::OpenMode::ModeAppend;
        }
        else
        {
            openMode |= AZ::IO::OpenMode::ModeWrite;
        }

        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::IO::Result result = AZ::IO::FileIOBase::GetInstance()->Open(modulePath.c_str(), openMode, fileHandle);
        AZ_Warning("python", result, "Could not open %s to write Python module symbols.", modulePath.c_str());
        if (result)
        {
            return fileHandle;
        }

        return AZ::IO::InvalidHandle;
    }
}
