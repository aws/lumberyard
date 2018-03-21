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

#include <AzCore/base.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/string.h>

// Generate a clang style annotation for a data member
#define AZCG_CreateAnnotation(annotation) __attribute__((annotate(annotation)))
#define AZCG_CreateArgumentAnnotation(annotation_name, ...) AZCG_CreateAnnotation(AZ_STRINGIZE(annotation_name) "(" AZ_STRINGIZE((__VA_ARGS__)) ")")

// \page tags "Code-generator tags" are use to tag you C++ code and provide that information to the codegenerator tool. When the code generator tool is defined
// AZ_CODE_GENERATOR will be defined. Depending on the settings your file can output to multiple outputs (generated) files.
// When including a generated file make sure it's the last include in your file, as we "current file" defines to automate including of the generated code as
// much as possible.
// Example:
// (MyTest.h file)
// #include <MyModule/... includes>
// ...
//
// #include "MyTest.generated.h"  // <- has the be the last include (technically the last include with generated code)
//
// class MyTestClass
// {
//   AzClass(uuid(...) ...);  //< When the code generator runs, it will use all tags to generate the code. When we run the compiler it will define a macro that pulls
//                            //< the generated code from "MyTest.generated.h"
//   ...
// };
//


#if defined(AZ_CODE_GENERATOR)
#   define AZCG_Class(ClassName, ...) AZCG_CreateArgumentAnnotation(AzClass, Class_Attribute, Identifier(ClassName), __VA_ARGS__) int AZ_JOIN(m_azCodeGenInternal, __COUNTER__);
#   define AZCG_Enum(EnumName, ...) AZCG_CreateArgumentAnnotation(AzEnum, Identifier(EnumName), __VA_ARGS__) EnumName AZ_JOIN(g_azCodeGenInternal, __COUNTER__);
#   define AZCG_Field(...) AZCG_CreateArgumentAnnotation(AzField, __VA_ARGS__)
#   define AZCG_Function(...) AZCG_CreateArgumentAnnotation(AzFunction, __VA_ARGS__)
#else
#   define AZCG_Class(ClassName, ...) AZ_JOIN(AZ_GENERATED_, ClassName)
#   define AZCG_Enum(EnumName, ...)
#   define AZCG_Field(...)
#   define AZCG_Function(...)
#endif

// Tags that might be common to multiple contexts, should be aliased into those namespaces and not used directly
namespace AzCommon
{
    struct Uuid
    {
        Uuid(const char* uuid) {}
    };

    struct Name
    {
        Name(const char* name) {}
    };

    struct Description
    {
        Description(const char* description) {}
        Description(const char* name, const char* description) {}
    };

    namespace Attributes
    {
        struct AutoExpand
        {
            AutoExpand(bool autoExpand=true) {}
        };

        struct ChangeNotify
        {
            using ChangeNotifyCallback = AZ::u32();

            ChangeNotify(const char* action) {}
            ChangeNotify(const AZ::Crc32& actionCrc) {}
            ChangeNotify(ChangeNotifyCallback callback) {}
        };

        struct DescriptionTextOverride
        {
            using DescriptionGetter = const AZStd::string&();

            DescriptionTextOverride(const char* description) {}
            DescriptionTextOverride(DescriptionGetter getter) {}
        };

        struct NameLabelOverride
        {
            using NameLabelGetter = const AZStd::string&();

            NameLabelOverride(const char* name) {}
            NameLabelOverride(NameLabelGetter getter) {}
        };

        struct Visibility
        {
            Visibility(const char* visibility) {}
            Visibility(const class AZ::Crc32& visibilityCrc) {}

            template <class Function>
            Visibility(Function getter) {}
        };

        struct Min
        {
            Min(float min) {}
            Min(int min) {}
        };

        struct Max
        {
            Max(float min) {}
            Max(int min) {}
        };

    }

    namespace Enum
    {
        struct Value
        {
            template <class Enum>
            Value(Enum value, const char* name) {}
        };
    }
}

namespace AzClass
{
    using AzCommon::Uuid;
    using AzCommon::Name;
    using AzCommon::Description;

    template <class Base, class ...Bases>
    struct Rtti
    {
        Rtti() = default;
    };

    struct Serialize
    {
        Serialize() {}

        template <class Parent, class ...Parents>
        struct Base 
        {
            Base() = default;
        };
    };

    struct Version
    {
        using ConverterFunction = bool(class AZ::SerializeContext& context, class AZ::SerializeContext::DataElementNode& classElement);
        Version(unsigned int version) {}
        Version(unsigned int version, ConverterFunction converter) {}
    };

    template <class ClassAllocator>
    struct Allocator
    {
        Allocator() = default;
    };

    struct PersistentId
    {
        using PersistentIdFunction = AZ::u64(const void*);
        PersistentId(PersistentIdFunction getter) {}
    };

    template <class SerializerType>
    struct Serializer
    {
        Serializer() = default;
    };

    template <class EventHandlerType>
    struct EventHandler
    {
        EventHandler() = default;
    };

    namespace Edit
    {
        using AzCommon::Attributes::AutoExpand;
        using AzCommon::Attributes::ChangeNotify;
        using AzCommon::Attributes::NameLabelOverride;
        using AzCommon::Attributes::DescriptionTextOverride;
        using AzCommon::Attributes::Visibility;

        struct AddableByUser
        {
            AddableByUser(bool addable = true) {}
        };

        struct AppearsInAddComponentMenu
        {
            AppearsInAddComponentMenu(const char* category) {}
            AppearsInAddComponentMenu(const AZ::Crc32& categoryCrc) {}
        };

        struct Category
        {
            Category(const char* category) {}
        };

        struct HelpPageURL
        {
            HelpPageURL(const char* url) {}
            template <class Function>
            HelpPageURL(Function getter) {}
        };
        
        struct HideIcon
        {
            HideIcon(bool hide=true) {}
        };

        struct Icon
        {
            Icon(const char* pathToIcon) {}
        };

        template <class AssetType>
        struct PrimaryAssetType
        {
            PrimaryAssetType() = default;
        };

        struct ViewportIcon
        {
            ViewportIcon(const char* pathToIcon) {}
        };
    }
}

namespace AzComponent
{
    struct Component
    {
        Component() = default;
    };

    struct Provides
    {
        Provides(std::initializer_list<const char*> services) {}
    };

    struct Requires
    {
        Requires(std::initializer_list<const char*> services) {}
    };

    struct Dependent
    {
        Dependent(std::initializer_list<const char*> services) {}
    };

    struct Incompatible
    {
        Incompatible(std::initializer_list<const char*> services) {}
    };
}

namespace AzField
{
    using AzCommon::Name;
    using AzCommon::Description;

    struct Serialize
    {
        Serialize(bool serialize = true) {}

        struct Name
        {
            Name(const char* name) {}
        };
    };

    struct Edit
    {
        Edit(bool edit = true) {}
    };

    struct UIHandler
    {
        UIHandler(const AZ::Crc32& uiHandlerCrc) {}
        UIHandler(const char* uiHandler) {}
    };

    struct Group
    {
        Group(const char* name) {}
    };

    namespace Attributes
    {
        using AzCommon::Attributes::AutoExpand;
        using AzCommon::Attributes::ChangeNotify;
        using AzCommon::Attributes::NameLabelOverride;
        using AzCommon::Attributes::DescriptionTextOverride;
        using AzCommon::Attributes::Visibility;

        struct ButtonText
        {
            ButtonText(const char* text) {}
        };

        struct ContainerCanBeModified
        {
            ContainerCanBeModified(bool isMutable=true) {}
        };

        struct EnumValues
        {
            using Value = AzCommon::Enum::Value;

            EnumValues(std::initializer_list<const char*> values) {}

            EnumValues(std::initializer_list<Value> valuesAndDisplayNames) {}
        };

        struct IncompatibleService
        {
            IncompatibleService(const char* serviceName) {}
        };

        struct Min
        {
            Min(int) {}
            Min(float) {}
            Min(double) {}
        };

        struct Max
        {
            Max(int) {}
            Max(float) {}
            Max(double) {}
        };

        struct ReadOnly
        {
            ReadOnly(bool readOnly=true) {}
        };

        struct RequiredService
        {
            RequiredService(const char* serviceName) {}
        };

        struct Step
        {
            Step(int) {}
            Step(float) {}
            Step(double) {}
        };

        struct StringList
        {
            template <class T>
            StringList(AZStd::vector<AZStd::string> T::* vectorOfStrings) {}

            template <class Function>
            StringList(Function getter) {}
        };

        struct Suffix
        {
            Suffix(const char* suffix) {}
        };
    }
}

namespace AzEnum
{
    using AzCommon::Uuid;
    using AzCommon::Name;
    using AzCommon::Description;

    using AzCommon::Enum::Value;

    struct Values
    {
        Values(std::initializer_list<Value> values) {}
    };
}

namespace AzFunction
{
    using AzCommon::Name;
}
