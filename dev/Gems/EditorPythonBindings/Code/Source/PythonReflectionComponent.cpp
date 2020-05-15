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

#include <PythonReflectionComponent.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <Source/PythonCommon.h>
#include <Source/PythonUtility.h>
#include <Source/PythonTypeCasters.h>
#include <Source/PythonProxyBus.h>
#include <Source/PythonProxyObject.h>
#include <Source/PythonSymbolsBus.h>
#include <pybind11/embed.h>

#include <AzCore/PlatformDef.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/PlatformDef.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/SystemFile.h>
#include <AzFramework/IO/LocalFileIO.h>

namespace EditorPythonBindings
{
    namespace Internal
    {
        static constexpr const char* s_azlmbr = "azlmbr";
        static constexpr const char* s_default = "default";
        static constexpr const char* s_globals = "globals";

        // a dummy structure for pybind11 to bind to for static constants and enums from the Behavior Context 
        struct StaticPropertyHolder {};

        AZStd::string PyResolvePath(AZStd::string_view path)
        {
            char pyPath[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(path.data(), pyPath, AZ_MAX_PATH_LEN);
            return { pyPath };
        }

        void RegisterPaths(pybind11::module parentModule)
        {
            pybind11::module pathsModule = parentModule.def_submodule("paths");
            pathsModule.def("resolve_path", [](const char* path)
            {
                return PyResolvePath(path);
            });
            pathsModule.attr("devroot") = PyResolvePath("@devroot@").c_str();
            pathsModule.attr("engroot") = PyResolvePath("@engroot@").c_str();
        }
    }

    void PythonReflectionComponent::Reflect(AZ::ReflectContext* context)
    {
    }

    void PythonReflectionComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PythonLegacyService", 0x56c1a561));
    }

    void PythonReflectionComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("PythonLegacyService", 0x56c1a561));
    }

    void PythonReflectionComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("PythonSystemService", 0x98e7cd4d));
    }

    void PythonReflectionComponent::Activate()
    {
        EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler::BusConnect();
    }

    void PythonReflectionComponent::Deactivate()
    {
        EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler::BusDisconnect();
    }

    void PythonReflectionComponent::ExportGlobalsFromBehaviorContext(pybind11::module parentModule)
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Error("Editor", behaviorContext, "Behavior context not available");
        if (!behaviorContext)
        {
            return;
        }

        // when a global method does not have a Module attribute put into the 'azlmbr.globals' module
        auto globalsModule = parentModule.def_submodule(Internal::s_globals);
        Module::PackageMapType modulePackageMap;

        // add global methods flagged for Automation as Python global functions
        for (const auto& methodEntry : behaviorContext->m_methods)
        {
            const AZStd::string& methodName = methodEntry.first;
            AZ::BehaviorMethod* behaviorMethod = methodEntry.second;
            if (Scope::IsBehaviorFlaggedForEditor(behaviorMethod->m_attributes))
            {
                pybind11::module targetModule;
                auto moduleNameResult = Module::GetName(behaviorMethod->m_attributes);
                if(moduleNameResult)
                {
                    targetModule = Module::DeterminePackageModule(modulePackageMap, *moduleNameResult, parentModule, globalsModule, false);
                }
                else
                {
                    targetModule = globalsModule;
                }

                if (behaviorMethod->HasResult())
                {
                    targetModule.def(methodName.c_str(), [behaviorMethod](pybind11::args args)
                    {
                        return Call::StaticMethod(behaviorMethod, args);
                    });
                }
                else
                {
                    targetModule.def(methodName.c_str(), [behaviorMethod](pybind11::args args)
                    {
                        Call::StaticMethod(behaviorMethod, args);
                    });
                }

                // log global method symbol
                AZStd::string subModuleName = pybind11::cast<AZStd::string>(targetModule.attr("__name__"));
                PythonSymbolEventBus::Broadcast(&PythonSymbolEventBus::Events::LogGlobalMethod, subModuleName, methodName, behaviorMethod);
            }
        }

        // add global properties flagged for Automation as Python static class properties
        pybind11::class_<Internal::StaticPropertyHolder> staticPropertyHolder(globalsModule, "property");
        for (const auto& propertyEntry : behaviorContext->m_properties)
        {
            const AZStd::string& propertyName = propertyEntry.first;
            AZ::BehaviorProperty* behaviorProperty = propertyEntry.second;
            if (Scope::IsBehaviorFlaggedForEditor(behaviorProperty->m_attributes))
            {
                //  log global property symbol
                AZStd::string subModuleName = pybind11::cast<AZStd::string>(globalsModule.attr("__name__"));
                PythonSymbolEventBus::Broadcast(&PythonSymbolEventBus::Events::LogGlobalProperty, subModuleName, propertyName, behaviorProperty);

                if (behaviorProperty->m_getter && behaviorProperty->m_setter)
                {
                    staticPropertyHolder.def_property_static(
                        propertyName.c_str(),
                        [behaviorProperty](pybind11::object) { return Call::StaticMethod(behaviorProperty->m_getter, {}); },
                        [behaviorProperty](pybind11::object, pybind11::args args) { return Call::StaticMethod(behaviorProperty->m_setter, args); }
                    );
                }
                else if (behaviorProperty->m_getter)
                {
                    staticPropertyHolder.def_property_static(
                        propertyName.c_str(),
                        [behaviorProperty](pybind11::object) { return Call::StaticMethod(behaviorProperty->m_getter, {}); },
                        pybind11::cpp_function()
                    );
                }
                else if (behaviorProperty->m_setter)
                {
                    AZ_Warning("python", false, "Global property %s only has a m_setter; write only properties not supported", propertyName.c_str());
                }
                else
                {
                    AZ_Error("python", false, "Global property %s has neither a m_getter or m_setter", propertyName.c_str());
                }
            }
        }
    }

    void PythonReflectionComponent::OnImportModule(PyObject* module)
    {
        pybind11::module parentModule = pybind11::cast<pybind11::module>(module);
        std::string pythonModuleName = pybind11::cast<std::string>(parentModule.attr("__name__"));
        if (AzFramework::StringFunc::Equal(pythonModuleName.c_str(), Internal::s_azlmbr))
        {
            // declare the default module to capture behavior that did not define a "Module" attribute
            pybind11::module defaultModule = parentModule.def_submodule(Internal::s_default);

            ExportGlobalsFromBehaviorContext(parentModule);
            PythonProxyObjectManagement::CreateSubmodule(parentModule, defaultModule);
            PythonProxyBusManagement::CreateSubmodule(parentModule);
            Internal::RegisterPaths(parentModule);

            PythonSymbolEventBus::Broadcast(&PythonSymbolEventBus::Events::Finalize);
        }
    }
}
