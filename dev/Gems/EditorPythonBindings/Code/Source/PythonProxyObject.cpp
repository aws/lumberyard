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

#include <PythonProxyObject.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <Source/PythonCommon.h>
#include <Source/PythonUtility.h>
#include <Source/PythonMarshalComponent.h>
#include <Source/PythonTypeCasters.h>
#include <Source/PythonSymbolsBus.h>

#include <pybind11/embed.h>

#include <AzCore/PlatformDef.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/AttributeReader.h>

namespace EditorPythonBindings
{
    PythonProxyObject::PythonProxyObject(const AZ::TypeId& typeId)
    {
        const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(typeId);
        if (behaviorClass)
        {
            CreateDefault(behaviorClass);
        }
    }

    PythonProxyObject::PythonProxyObject(const char* typeName)
    {
        SetByTypeName(typeName);
    }

    pybind11::object PythonProxyObject::Construct(const AZ::BehaviorClass& behaviorClass, pybind11::args args)
    {
        // nothing to construct with ...
        if (args.size() == 0 || behaviorClass.m_constructors.empty())
        {
            if (!CreateDefault(&behaviorClass))
            {
                return pybind11::cast<pybind11::none>(Py_None);
            }
            return pybind11::cast(this);
        }

        // find the right constructor
        for (AZ::BehaviorMethod* constructor : behaviorClass.m_constructors)
        {
            const size_t numArgsPlusSelf = args.size() + 1;
            AZ_Error("python", constructor, "Missing constructor value in behavior class %s", behaviorClass.m_name.c_str());
            if (constructor && constructor->GetNumArguments() == numArgsPlusSelf)
            {
                bool match = true;
                for (size_t index = 0; index < args.size(); ++index)
                {
                    const AZ::BehaviorParameter* behaviorArg = constructor->GetArgument(index + 1);
                    pybind11::object pythonArg = args[index];
                    const auto traits = static_cast<PythonMarshalTypeRequests::BehaviorTraits>(behaviorArg->m_traits);

                    bool canConvert = false;
                    PythonMarshalTypeRequestBus::EventResult(canConvert, behaviorArg->m_typeId, &PythonMarshalTypeRequestBus::Events::CanConvertPythonToBehaviorValue, traits, pythonArg);
                    if (!behaviorArg || !canConvert)
                    {
                        match = false;
                        break;
                    }
                }

                if (match)
                {
                    // prepare wrapped object instance
                    m_wrappedObject.m_address = behaviorClass.Allocate();
                    m_wrappedObject.m_typeId = behaviorClass.m_typeId;
                    PrepareWrappedObject(behaviorClass);

                    // execute constructor
                    Call::ClassMethod(constructor, m_wrappedObject, args);
                    return pybind11::cast(this);
                }
            }
        }
        return pybind11::cast<pybind11::none>(Py_None);
    }

    PythonProxyObject::PythonProxyObject(const AZ::BehaviorObject& object)
    {
        const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(object.m_typeId);
        if (behaviorClass)
        {
            m_wrappedObject = behaviorClass->Clone(object);
            PrepareWrappedObject(*behaviorClass);
        }
    }

    PythonProxyObject::~PythonProxyObject()
    {
        ReleaseWrappedObject();
    }

    const char* PythonProxyObject::GetWrappedTypeName() const
    {
        return m_wrappedObjectTypeName.c_str();
    }

    void PythonProxyObject::SetPropertyValue(const char* attributeName, pybind11::object value)
    {
        if (!m_wrappedObject.IsValid())
        {
            PyErr_SetString(PyExc_RuntimeError, "The wrapped Proxy Object has not been setup correctly; missing call set_type()?");
            AZ_Error("python", false, "The wrapped Proxy Object has not been setup correctly; missing call set_type()?");
            return;
        }

        auto behaviorPropertyIter = m_properties.find(AZ::Crc32(attributeName));
        if (behaviorPropertyIter != m_properties.end())
        {
            AZ::BehaviorProperty* property = behaviorPropertyIter->second;
            AZ_Error("python", property->m_setter, "%s is not a writable property in class %s.", attributeName, m_wrappedObjectTypeName.c_str());
            if (property->m_setter)
            {
                EditorPythonBindings::Call::ClassMethod(property->m_setter, m_wrappedObject, pybind11::args(pybind11::make_tuple(value)));
            }
        }
    }

    pybind11::object PythonProxyObject::GetPropertyValue(const char* attributeName)
    {
        if (!m_wrappedObject.IsValid())
        {
            PyErr_SetString(PyExc_RuntimeError, "The wrapped Proxy Object has not been setup correctly; missing call set_type()?");
            AZ_Error("python", false, "The wrapped Proxy Object has not been setup correctly; missing call set_type()?");
            return pybind11::cast<pybind11::none>(Py_None);
        }

        AZ::Crc32 crcAttributeName(attributeName);

        // the attribute could refer to a method
        auto methodEntry = m_methods.find(crcAttributeName);
        if (methodEntry != m_methods.end())
        {
            AZ::BehaviorMethod* method = methodEntry->second;
            return pybind11::cpp_function([this, method](pybind11::args pythonArgs)
            {
                return EditorPythonBindings::Call::ClassMethod(method, m_wrappedObject, pythonArgs);
            });
        }

        // the attribute could refer to a property
        auto behaviorPropertyIter = m_properties.find(crcAttributeName);
        if (behaviorPropertyIter != m_properties.end())
        {
            AZ::BehaviorProperty* property = behaviorPropertyIter->second;
            AZ_Error("python", property->m_getter, "%s is not a readable property in class %s.", attributeName, m_wrappedObjectTypeName.c_str());
            if (property->m_getter)
            {
                return EditorPythonBindings::Call::ClassMethod(property->m_getter, m_wrappedObject, pybind11::args());
            }
        }

        return pybind11::cast<pybind11::none>(Py_None);
    }

    bool PythonProxyObject::SetByTypeName(const char* typeName) 
    {
        const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(AZStd::string(typeName));
        if (behaviorClass)
        {
            return CreateDefault(behaviorClass);
        }
        return false;
    }

    pybind11::object PythonProxyObject::Invoke(const char* methodName, pybind11::args pythonArgs)
    {
        const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(m_wrappedObject.m_typeId);
        if (behaviorClass)
        {
            auto behaviorMethodIter = behaviorClass->m_methods.find(methodName);
            if (behaviorMethodIter != behaviorClass->m_methods.end())
            {
                AZ::BehaviorMethod* method = behaviorMethodIter->second;
                AZ_Error("python", method, "%s is not a method in class %s!", methodName, m_wrappedObjectTypeName.c_str());

                if (method && PythonProxyObjectManagement::IsMemberLike(*method, m_wrappedObject.m_typeId))
                {
                    return EditorPythonBindings::Call::ClassMethod(method, m_wrappedObject, pythonArgs);
                }
            }
        }
        return pybind11::cast<pybind11::none>(Py_None);
    }

    AZStd::optional<AZ::TypeId> PythonProxyObject::GetWrappedType() const
    {
        if (m_wrappedObject.IsValid())
        {
            return AZStd::make_optional(m_wrappedObject.m_typeId);
        }
        return AZStd::nullopt;
    }

    AZStd::optional<AZ::BehaviorObject*> PythonProxyObject::GetBehaviorObject()
    {
        if (m_wrappedObject.IsValid())
        {
            return AZStd::make_optional(&m_wrappedObject);
        }
        return AZStd::nullopt;
    }
    
    void PythonProxyObject::PrepareWrappedObject(const AZ::BehaviorClass& behaviorClass)
    {
        m_ownership = Ownership::Owned;
        m_wrappedObjectTypeName = behaviorClass.m_name;

        // is this Behavior Class flagged to usage for Editor.exe bindings?
        if (!Scope::IsBehaviorFlaggedForEditor(behaviorClass.m_attributes))
        {
            return;
        }

        PopulateMethodsAndProperties(behaviorClass);

        for (auto&& baseClassId : behaviorClass.m_baseClasses)
        {
            const AZ::BehaviorClass* baseClass = AZ::BehaviorContextHelper::GetClass(baseClassId);
            if (baseClass)
            {
                PopulateMethodsAndProperties(*baseClass);
            }
        }
    }

    void PythonProxyObject::PopulateMethodsAndProperties(const AZ::BehaviorClass& behaviorClass)
    {
        AZStd::string baseName;

        // cache all the methods for this behavior class
        for (const auto& methodEntry : behaviorClass.m_methods)
        {
            AZ::BehaviorMethod* method = methodEntry.second;
            AZ_Error("python", method, "Missing method entry:%s value in behavior class:%s", methodEntry.first.c_str(), m_wrappedObjectTypeName.c_str());
            if (method && PythonProxyObjectManagement::IsMemberLike(*method, m_wrappedObject.m_typeId))
            {
                baseName = methodEntry.first;
                Scope::FetchScriptName(method->m_attributes, baseName);
                AZ::Crc32 namedKey(baseName);
                if (m_methods.find(namedKey) == m_methods.end())
                {
                    m_methods[namedKey] = method;
                }
                else
                {
                    AZ_TracePrintf("python", "Skipping duplicate method named %s\n", baseName.c_str());
                }
            }
        }

        // cache all the properties for this behavior class
        for (const auto& behaviorProperty : behaviorClass.m_properties)
        {
            AZ::BehaviorProperty* property = behaviorProperty.second;
            AZ_Error("python", property, "Missing property %s in behavior class:%s", behaviorProperty.first.c_str(), m_wrappedObjectTypeName.c_str());
            if (property)
            {
                baseName = behaviorProperty.first;
                Scope::FetchScriptName(property->m_attributes, baseName);
                AZ::Crc32 namedKey(baseName);
                if (m_properties.find(namedKey) == m_properties.end())
                {
                    m_properties[namedKey] = property;
                }
                else
                {
                    AZ_TracePrintf("python", "Skipping duplicate property named %s\n", baseName.c_str());
                }
            }
        }
    }

    void PythonProxyObject::ReleaseWrappedObject()
    {
        if (m_wrappedObject.IsValid() && m_ownership == Ownership::Owned)
        {
            const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(m_wrappedObject.m_typeId);
            if (behaviorClass)
            {
                behaviorClass->Destroy(m_wrappedObject);
                m_wrappedObject = {};
                m_wrappedObjectTypeName.clear();
                m_methods.clear();
                m_properties.clear();
            }
        }
    }

    bool PythonProxyObject::CreateDefault(const AZ::BehaviorClass* behaviorClass)
    {
        AZ_Error("python", behaviorClass, "Expecting a non-null BehaviorClass");
        if (behaviorClass)
        {
            if (Scope::IsBehaviorFlaggedForEditor(behaviorClass->m_attributes))
            {
                m_wrappedObject = behaviorClass->Create();
                PrepareWrappedObject(*behaviorClass);
                return true;
            }
            AZ_Warning("python", false, "The behavior class (%s) is not flagged for Editor use.", behaviorClass->m_name.c_str());
        }
        return false;
    }


    namespace PythonProxyObjectManagement
    {
        bool IsMemberLike(const AZ::BehaviorMethod& method, const AZ::TypeId& typeId)
        {
            return method.IsMember() || (method.GetNumArguments() > 0 && method.GetArgument(0)->m_typeId == typeId);
        }

        pybind11::object CreatePythonProxyObject(const AZ::TypeId& typeId, void* data)
        {
            PythonProxyObject* instance = nullptr;
            if (!data)
            {
                instance = aznew PythonProxyObject(typeId);
            }
            else
            {
                instance = aznew PythonProxyObject(AZ::BehaviorObject(data, typeId));
            }

            if (!instance->GetWrappedType())
            {
                delete instance;
                PyErr_SetString(PyExc_TypeError, "Failed to create proxy object by type name.");
                return pybind11::cast<pybind11::none>(Py_None);
            }
            return pybind11::cast(instance);
        }

        pybind11::object CreatePythonProxyObjectByTypename(const char* classTypename)
        {
            const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(AZStd::string(classTypename));
            AZ_Warning("python", behaviorClass, "Missing Behavior Class for typename:%s", classTypename);
            if (!behaviorClass)
            {
                return pybind11::cast<pybind11::none>(Py_None);
            }
            return CreatePythonProxyObject(behaviorClass->m_typeId, nullptr);
        }

        pybind11::object ConstructPythonProxyObjectByTypename(const char* classTypename, pybind11::args args)
        {
            const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(AZStd::string(classTypename));
            AZ_Warning("python", behaviorClass, "Missing Behavior Class for typename:%s", classTypename);
            if (!behaviorClass)
            {
                return pybind11::cast<pybind11::none>(Py_None);
            }

            PythonProxyObject* instance = aznew PythonProxyObject();
            pybind11::object pythonInstance = instance->Construct(*behaviorClass, args);
            if (pythonInstance.is_none())
            {
                delete instance;
                PyErr_SetString(PyExc_TypeError, "Failed to construct proxy object with provided args.");
                return pybind11::cast<pybind11::none>(Py_None);
            }
            return pybind11::cast(instance);
        }

        void ExportStaticBehaviorClassElements(pybind11::module parentModule, pybind11::module defaultModule)
        {
            AZ::BehaviorContext* behaviorContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            AZ_Error("python", behaviorContext, "Behavior context not available");
            if (!behaviorContext)
            {
                return;
            }

            // this will make the base package modules for namespace "azlmbr.*" and "azlmbr.default" for behavior that does not specify a module name
            Module::PackageMapType modulePackageMap;

            for (const auto& classEntry : behaviorContext->m_classes)
            {
                AZ::BehaviorClass* behaviorClass = classEntry.second;

                // is this Behavior Class flagged to usage for Editor.exe bindings?
                if (!Scope::IsBehaviorFlaggedForEditor(behaviorClass->m_attributes))
                {
                    continue; // skip this class
                }

                // find the target module of the behavior's static methods
                auto moduleName = Module::GetName(behaviorClass->m_attributes);
                pybind11::module subModule = Module::DeterminePackageModule(modulePackageMap, moduleName ? *moduleName : "", parentModule, defaultModule, false);

                bool exportBehaviorClass = false;

                // does this class define methods that may be reflected in a Python  module?
                if (!behaviorClass->m_methods.empty())
                {
                    // add the non-member methods as Python 'free' function
                    for (const auto& methodEntry : behaviorClass->m_methods)
                    {
                        const AZStd::string& methodName = methodEntry.first;
                        AZ::BehaviorMethod* behaviorMethod = methodEntry.second;
                        if (!PythonProxyObjectManagement::IsMemberLike(*behaviorMethod, behaviorClass->m_typeId))
                        {
                            // the name of the static method will be "azlmbr.<sub_module>.<Behavior Class>_<Behavior Method>"
                            AZStd::string globalMethodName = AZStd::string::format("%s_%s", behaviorClass->m_name.c_str(), methodName.c_str());

                            if (behaviorMethod->HasResult())
                            {
                                subModule.def(globalMethodName.c_str(), [behaviorMethod](pybind11::args args)
                                {
                                    return Call::StaticMethod(behaviorMethod, args);
                                });
                            }
                            else
                            {
                                subModule.def(globalMethodName.c_str(), [behaviorMethod](pybind11::args args)
                                {
                                    Call::StaticMethod(behaviorMethod, args);
                                });
                            }

                            AZStd::string subModuleName = pybind11::cast<AZStd::string>(subModule.attr("__name__"));
                            PythonSymbolEventBus::Broadcast(&PythonSymbolEventBus::Events::LogClassMethod, subModuleName, globalMethodName, behaviorClass, behaviorMethod);
                        }
                        else
                        {
                            // any member method means the class should be exported to Python
                            exportBehaviorClass = true;
                        }
                    }
                }

                // if the Behavior Class has any properties, then export it
                if (!exportBehaviorClass)
                {
                    exportBehaviorClass = !behaviorClass->m_properties.empty();
                }

                // register all Behavior Class types with a Python function to construct an instance
                if (exportBehaviorClass)
                {
                    const char* behaviorClassName = behaviorClass->m_name.c_str();
                    subModule.attr(behaviorClassName) = pybind11::cpp_function([behaviorClassName](pybind11::args pythonArgs)
                    {
                        return ConstructPythonProxyObjectByTypename(behaviorClassName, pythonArgs);
                    });

                    AZStd::string subModuleName = pybind11::cast<AZStd::string>(subModule.attr("__name__"));
                    PythonSymbolEventBus::Broadcast(&PythonSymbolEventBus::Events::LogClass, subModuleName, behaviorClass);
                }
            }
        }

        pybind11::list ListBehaviorAttributes(const PythonProxyObject& pythonProxyObject)
        {
            pybind11::list items;
            AZStd::string baseName;

            auto typeId = pythonProxyObject.GetWrappedType();
            if (!typeId)
            {
                return items;
            }

            const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(typeId.value());
            if (!behaviorClass)
            {
                return items;
            }

            if (!Scope::IsBehaviorFlaggedForEditor(behaviorClass->m_attributes))
            {
                return items;
            }

            for (const auto& methodEntry : behaviorClass->m_methods)
            {
                AZ::BehaviorMethod* method = methodEntry.second;
                if (method && PythonProxyObjectManagement::IsMemberLike(*method, typeId.value()))
                {
                    baseName = methodEntry.first;
                    Scope::FetchScriptName(method->m_attributes, baseName);
                    items.append(pybind11::str(baseName.c_str()));
                }
            }

            for (const auto& behaviorProperty : behaviorClass->m_properties)
            {
                AZ::BehaviorProperty* property = behaviorProperty.second;
                if (property)
                {
                    baseName = behaviorProperty.first;
                    Scope::FetchScriptName(property->m_attributes, baseName);
                    items.append(pybind11::str(baseName.c_str()));
                }
            }
            return items;
        }

        void CreateSubmodule(pybind11::module parentModule, pybind11::module defaultModule)
        {
            ExportStaticBehaviorClassElements(parentModule, defaultModule);

            auto objectModule = parentModule.def_submodule("object");
            objectModule.def("create", &CreatePythonProxyObjectByTypename);
            objectModule.def("construct", &ConstructPythonProxyObjectByTypename);
            objectModule.def("dir", &ListBehaviorAttributes);

            pybind11::class_<PythonProxyObject>(objectModule, "PythonProxyObject", pybind11::dynamic_attr())
                .def(pybind11::init<>())
                .def(pybind11::init<const char*>())
                .def_property_readonly("typename", &PythonProxyObject::GetWrappedTypeName)
                .def("set_type", &PythonProxyObject::SetByTypeName)
                .def("set_property", &PythonProxyObject::SetPropertyValue)
                .def("get_property", &PythonProxyObject::GetPropertyValue)
                .def("invoke", &PythonProxyObject::Invoke)
                .def("__setattr__", &PythonProxyObject::SetPropertyValue)
                .def("__getattr__", &PythonProxyObject::GetPropertyValue)
                ;
        }
    }
}
