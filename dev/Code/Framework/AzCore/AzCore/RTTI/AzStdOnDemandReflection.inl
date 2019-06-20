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

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/ScriptCanvas/ScriptCanvasAttributes.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/tokenize.h>
#include <AzCore/RTTI/AzStdOnDemandPrettyName.inl>

// forward declare specialized types
namespace AZStd
{
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class vector;
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class list;
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class forward_list;
    template< class T, size_t Capacity >
    class fixed_vector;
    template< class T, size_t N >
    class array;
    template<class Key, class MappedType, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/ >
    class unordered_map;
    template<class Key, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/>
    class unordered_set;
    template<AZStd::size_t NumBits>
    class bitset;
    template<class Element, class Traits, class Allocator>
    class basic_string;
    template<class Element>
    struct char_traits;

    template<class T>
    class intrusive_ptr;
    template<class T>
    class shared_ptr;
}

namespace AZ
{
    class BehaviorContext;
    class ScriptDataContext;

    /// OnDemand reflection for AZStd::basic_string
    template<class Element, class Traits, class Allocator>
    struct OnDemandReflection< AZStd::basic_string<Element, Traits, Allocator> >
    {
        using ContainerType = AZStd::basic_string<Element, Traits, Allocator>;
        using SizeType = typename ContainerType::size_type;
        using ValueType = typename ContainerType::value_type;

        // TODO: Count reflection types for a proper un-reflect 

        static void CustomConstructor(ContainerType* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsString(0))
                {
                    ContainerType str;
                    dc.ReadArg(0, str);
                    new(thisPtr) ContainerType(AZStd::move(str));
                }
                else if (dc.IsClass<ContainerType>(0))
                {
                    dc.ReadArg(0, thisPtr);
                }
            }
        }

        // Store any string as a Lua native string
        static void ToLua(lua_State* lua, BehaviorValueParameter& value)
        {
            ScriptValue<const char*>::StackPush(lua, reinterpret_cast<ContainerType*>(value.GetValueAddress())->c_str());
        }

        // Read any native string as any AZStd::string
        static bool FromLua(lua_State* lua, int stackIndex, BehaviorValueParameter& value, BehaviorClass* valueClass, ScriptContext::StackVariableAllocator* stackTempAllocator)
        {
            const char* str = ScriptValue<const char*>::StackRead(lua, stackIndex);
            if (stackTempAllocator)
            {
                // Allocate space for the string.
                // If passing by reference, we need to construct it ahead of time (in this allocated space)
                // If not, StoreResult will construct it for us, and we can pass in a temp
                value.m_value = stackTempAllocator->allocate(sizeof(ContainerType), AZStd::alignment_of<ContainerType>::value, 0);

                if (valueClass->m_defaultConstructor)
                {
                    valueClass->m_defaultConstructor(value.m_value, valueClass->m_userData);
                }

            }
            else if (value.m_value)
            {
                // If there's no allocator, it's possible the destination has already been set-up
            }
            else
            {
                AZ_Assert(false, "Invalid call to FromLua! Either a stack allocator must be passed, or value.m_value must be a valid storage location.");
            }

            // Can't construct string from nullptr, so replace it with empty string
            if (str == nullptr)
            {
                str = "";
            }

            if (value.m_traits & BehaviorParameter::TR_REFERENCE)
            {
                new(value.m_value) ContainerType(str);
            }
            else
            {
                // Otherwise, we can construct the string as we pass it, but need to allocate value.m_value ahead of time
                value.StoreResult<ContainerType>(ContainerType(str));
            }
            return true;
        }

        static void Reflect(ReflectContext* context)
        {
            BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->template Constructor<typename ContainerType::value_type*>()
                        ->Attribute(AZ::Script::Attributes::ConstructorOverride, &CustomConstructor)
                        ->Attribute(AZ::Script::Attributes::ReaderWriterOverride, ScriptContext::CustomReaderWriter(&ToLua, &FromLua))
                    ->template WrappingMember<const char*>(&ContainerType::c_str)
                    ->Method("c_str", &ContainerType::c_str)
                    ->Method("Length", [](ContainerType* thisPtr) { return aznumeric_cast<int>(thisPtr->length()); })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                    ->Method("Equal", [](const ContainerType& lhs, const ContainerType& rhs)
                    {
                        return lhs == rhs;
                    })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                    ->Method("Find", [](ContainerType* thisPtr, const ContainerType& stringToFind, const int& startPos)
                    {
                        return aznumeric_cast<int>(thisPtr->find(stringToFind, startPos));
                    })
                    ->Method("Substring", [](ContainerType* thisPtr, const int& pos, const int& len)
                    {
                        return thisPtr->substr(pos, len);
                    })
                    ->Method("Replace", [](ContainerType* thisPtr, const ContainerType& stringToReplace, const ContainerType& replacementString)
                    { 
                        SizeType startPos = 0;
                        while ((startPos = thisPtr->find(stringToReplace, startPos)) != ContainerType::npos && !stringToReplace.empty())
                        {
                            thisPtr->replace(startPos, stringToReplace.length(), replacementString);
                            startPos += replacementString.length();
                        }

                        return *thisPtr;
                    })
                    ->Method("ReplaceByIndex", [](ContainerType* thisPtr, const int& beginIndex, const int& endIndex, const ContainerType& replacementString)
                    {
                        thisPtr->replace(beginIndex, endIndex - beginIndex + 1, replacementString);
                        return *thisPtr;
                    })
                    ->Method("Add", [](ContainerType* thisPtr, const ContainerType& addend)
                    {
                        return *thisPtr + addend;
                    })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Concat)
                    ->Method("TrimLeft", [](ContainerType* thisPtr)
                    {
                        auto wsfront = AZStd::find_if_not(thisPtr->begin(), thisPtr->end(), [](char c) {return AZStd::is_space(c);});
                        thisPtr->erase(thisPtr->begin(), wsfront);
                        return *thisPtr;
                    })
                    ->Method("TrimRight", [](ContainerType* thisPtr)
                    {
                        auto wsend = AZStd::find_if_not(thisPtr->rbegin(), thisPtr->rend(), [](char c) {return AZStd::is_space(c);});
                        thisPtr->erase(wsend.base(), thisPtr->end());
                        return *thisPtr;
                    })
                    ->Method("ToLower", [](ContainerType* thisPtr)
                    {
                        ContainerType toLowerString;
                        for (auto itr = thisPtr->begin(); itr < thisPtr->end(); itr++)
                        {
                            toLowerString.push_back(static_cast<ValueType>(tolower(*itr)));
                        }                    
                        return toLowerString;
                    })
                    ->Method("ToUpper", [](ContainerType* thisPtr)
                    {
                        ContainerType toUpperString;
                        for (auto itr = thisPtr->begin(); itr < thisPtr->end(); itr++)
                        {
                            toUpperString.push_back(static_cast<ValueType>(toupper(*itr)));
                        }
                        return toUpperString;
                    })
                    ->Method("Join", [](AZStd::vector<ContainerType>* stringsToJoinPtr, const ContainerType& joinStr)
                    {
                        ContainerType joinString;
                        for (auto& stringToJoin : *stringsToJoinPtr)
                        {
                            joinString.append(stringToJoin).append(joinStr);
                        }
                        //Cut off the last join str
                        if (!stringsToJoinPtr->empty())
                        {
                            joinString = joinString.substr(0, joinString.length() - joinStr.length());
                        }
                        return joinString;
                    })

                    ->Method("Split", [](ContainerType* thisPtr, const ContainerType& splitter)
                    {
                        AZStd::vector<ContainerType> splitStringList;
                        AZStd::tokenize(*thisPtr, splitter, splitStringList);
                        return splitStringList;
                    })
                    ;
            }
        }
    };

    /// OnDemand reflection for AZStd::basic_string_view
    template<class Element, class Traits>
    struct OnDemandReflection< AZStd::basic_string_view<Element, Traits> >
    {
        using ContainerType = AZStd::basic_string_view<Element, Traits>;
        using SizeType = typename ContainerType::size_type;
        using ValueType = typename ContainerType::value_type;

        // TODO: Count reflection types for a proper un-reflect 

        static void CustomConstructor(ContainerType* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsString(0))
                {
                    AZStd::basic_string<Element, Traits, AZStd::allocator> str;
                    dc.ReadArg(0, str);
                    new(thisPtr) ContainerType(str);
                }
                else if (dc.IsClass<ContainerType>(0))
                {
                    dc.ReadArg(0, thisPtr);
                }
            }
        }

        // Store any string as a Lua native string
        static void ToLua(lua_State* lua, BehaviorValueParameter& value)
        {
            ScriptValue<const char*>::StackPush(lua, reinterpret_cast<ContainerType*>(value.GetValueAddress())->data());
        }

        // Read any native string as an AZStd::string_view
        static bool FromLua(lua_State* lua, int stackIndex, BehaviorValueParameter& value, BehaviorClass* valueClass, ScriptContext::StackVariableAllocator* stackTempAllocator)
        {
            const char* str = ScriptValue<const char*>::StackRead(lua, stackIndex);
            if (stackTempAllocator)
            {
                // Allocate space for the string.
                // If passing by reference, we need to construct it ahead of time (in this allocated space)
                // If not, StoreResult will construct it for us, and we can pass in a temp
                value.m_value = stackTempAllocator->allocate(sizeof(ContainerType), AZStd::alignment_of<ContainerType>::value, 0);

                if (valueClass->m_defaultConstructor)
                {
                    valueClass->m_defaultConstructor(value.m_value, valueClass->m_userData);
                }

            }
            else if (value.m_value)
            {
                // If there's no allocator, it's possible the destination has already been set-up
            }
            else
            {
                AZ_Assert(false, "Invalid call to FromLua! Either a stack allocator must be passed, or value.m_value must be a valid storage location.");
            }

            if (value.m_traits & BehaviorParameter::TR_REFERENCE)
            {
                new(value.m_value) ContainerType(str);
            }
            else
            {
                // Otherwise, we can construct the string as we pass it, but need to allocate value.m_value ahead of time
                value.StoreResult<ContainerType>(ContainerType(str));
            }
            return true;
        }

        static void Reflect(ReflectContext* context)
        {
            BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::Category, "Core")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->template Constructor<typename ContainerType::value_type*>()
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &CustomConstructor)
                    ->Attribute(AZ::Script::Attributes::ReaderWriterOverride, ScriptContext::CustomReaderWriter(&ToLua, &FromLua))
                    ->Method("ToString", [](const ContainerType& stringView) { return static_cast<AZStd::string>(stringView).c_str(); }, { { { "Reference", "String view object being converted to string" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Converts string_view to string")
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                    ->template WrappingMember<const char*>(&ContainerType::data)
                    ->Method("data", &ContainerType::data)
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Returns reference to raw string data")
                    ->Method("length", [](ContainerType* thisPtr) { return aznumeric_cast<int>(thisPtr->length()); }, { { { "This", "Reference to the object the method is being performed on" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Returns length of string view")
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                    ->Method("size", [](ContainerType* thisPtr) { return aznumeric_cast<int>(thisPtr->size()); }, { { { "This", "Reference to the object the method is being performed on" }} })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Returns length of string view")
                    ->Method("find", [](ContainerType* thisPtr, ContainerType stringToFind, int startPos)
                    {
                        return aznumeric_cast<int>(thisPtr->find(stringToFind, startPos));
                    }, { { { "This", "Reference to the object the method is being performed on" }, { "View", "View to search " }, { "Position", "Index in view to start search" }} })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Searches for supplied string within this string")
                    ->Method("substr", [](ContainerType* thisPtr, int pos, int len)
                    {
                        return thisPtr->substr(pos, len);
                    }, { {{"This", "Reference to the object the method is being performed on"}, {"Position", "Index in view that indicates the beginning of the sub string"}, {"Count", "Length of characters that sub string view occupies" }} })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Creates a sub view of this string view. The string data is not actually modified")
                    ->Method("remove_prefix", [](ContainerType* thisPtr, int n) {thisPtr->remove_prefix(n); },
                    { { { "This", "Reference to the object the method is being performed on" }, { "Count", "Number of characters to remove from start of view" }} })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Moves the supplied number of characters from the beginning of this sub view")
                    ->Method("remove_suffix", [](ContainerType* thisPtr, int n) {thisPtr->remove_suffix(n); },
                    { { { "This", "Reference to the object the method is being performed on" } ,{ "Count", "Number of characters to remove from end of view" }} })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Moves the supplied number of characters from the end of this sub view")
                    ;
            }
        }
    };

    /// OnDemand reflection for AZStd::intrusive_ptr
    template<class T>
    struct OnDemandReflection< AZStd::intrusive_ptr<T> >
    {
        using ContainerType = AZStd::intrusive_ptr<T>;

        // TODO: Count reflection types for a proper un-reflect 

        static void CustomConstructor(ContainerType* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsClass<T>(0))
                {
                    T* value = nullptr;
                    dc.ReadArg(0, value);
                    dc.AcquireOwnership(0, false); // the smart pointer will be owner by the object
                    new(thisPtr) ContainerType(value);
                }
                else if (dc.IsClass<ContainerType>(0))
                {
                    dc.ReadArg(0, thisPtr);
                }
            }
        }

        static void Reflect(ReflectContext* context)
        {
            BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<ContainerType>::Get)
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AZ::AttributeIsValid::IfPresent)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ConstructibleFromNil, true)
                    ->template Constructor<typename ContainerType::value_type*>()
                        ->Attribute(AZ::Script::Attributes::ConstructorOverride, &CustomConstructor)
                    ->template WrappingMember<typename ContainerType::value_type>(&ContainerType::get)
                    ->Method("get", &ContainerType::get)
                    ;
            }
        }
    };

    /// OnDemand reflection for AZStd::shared_ptr
    template<class T>
    struct OnDemandReflection< AZStd::shared_ptr<T> >
    {
        using ContainerType = AZStd::shared_ptr<T>;

        // TODO: Count reflection types for a proper un-reflect 

        static void CustomConstructor(ContainerType* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsClass<T>(0))
                {
                    T* value = nullptr;
                    dc.ReadArg(0, value);
                    dc.AcquireOwnership(0, false); // the smart pointer will be owner by the object
                    new(thisPtr) ContainerType(value);
                }
                else if (dc.IsClass<ContainerType>(0))
                {
                    dc.ReadArg(0, thisPtr);
                }
            }
        }

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<ContainerType>::Get)
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AZ::AttributeIsValid::IfPresent)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ConstructibleFromNil, true)
                    ->template Constructor<typename ContainerType::value_type*>()
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &CustomConstructor)
                    ->template WrappingMember<typename ContainerType::value_type>(&ContainerType::get)
                    ->Method("get", &ContainerType::get)
                    ;
            }
        }
    };

    /// OnDemand reflection for AZStd::vector
    template<class T, class A>
    struct OnDemandReflection< AZStd::vector<T, A> >
    {
        using ContainerType = AZStd::vector<T, A>;

        static AZ::Outcome<T, AZStd::string> At(ContainerType& thisContainer, size_t index)
        {
            if (index >= 0 && index < thisContainer.size())
            {
                return AZ::Success(thisContainer.at(index));
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Index out of bounds: %zu (size: %zu)", index, thisContainer.size()));
            }
        }

        static AZ::Outcome<T, void> Back(ContainerType& thisContainer)
        {
            if (thisContainer.empty())
            {
                return AZ::Failure();
            }

            return AZ::Success(thisContainer.back());
        }

        static AZ::Outcome<void, void> Erase(ContainerType& thisContainer, size_t index)
        {
            if (index >= 0 && index < thisContainer.size())
            {
                thisContainer.erase(thisContainer.begin() + index);
                return AZ::Success();
            }

            return AZ::Failure();
        }

        static AZ::Outcome<void, void> EraseRange(ContainerType& thisContainer, size_t indexStart, size_t indexEnd)
        {
            if (indexStart >= 0 && indexStart <= indexEnd && indexEnd < thisContainer.size())
            {
                thisContainer.erase(thisContainer.begin() + indexStart, thisContainer.begin() + indexEnd + 1);
                return AZ::Success();
            }

            return AZ::Failure();
        }
        
        static AZ::Outcome<T, void> Front(ContainerType& thisContainer)
        {
            if (thisContainer.empty())
            {
                return AZ::Failure();
            }
            
            return AZ::Success(thisContainer.front());
        }
        
        static void Insert(ContainerType& thisPtr, AZ::u64 index, const T& value)
        {
            if (index >= thisPtr.size())
            {
                thisPtr.resize(index + 1);
                thisPtr[index] = value;
            }
            else
            {
                thisPtr.insert(thisPtr.begin() + index, value);
            }
        }

        static AZ::Outcome<void, void> PopBack(ContainerType& thisContainer)
        {
            if (thisContainer.empty())
            {
                return AZ::Failure();
            }

            thisContainer.pop_back();
            return AZ::Success();
        }

        static AZ::Outcome<void, void> Replace(ContainerType& thisContainer, size_t index, T& value)
        {
            if (index >= 0 && index < thisContainer.size())
            {
                thisContainer[index] = value;
                return AZ::Success();
            }

            return AZ::Failure();
        }

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::ScriptOwn)
                    ->template Method<void(ContainerType::*)(typename ContainerType::const_reference)>("push_back", &ContainerType::push_back)
                        ->Attribute(AZ::Script::Attributes::Deprecated, true)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Method("pop_back", &ContainerType::pop_back)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                        ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->template Method<typename ContainerType::reference(ContainerType::*)(typename ContainerType::size_type)>("at", &ContainerType::at, {{ { "Index", "The index to read from", nullptr, BehaviorParameter::Traits::TR_INDEX } }})
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::IndexRead)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                        ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Method("AssignAt", &AssignAt, {{ {}, { "Index", "The index at which to assign the element to", nullptr, BehaviorParameter::Traits::TR_INDEX } }})
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::IndexWrite)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Method("size", [](ContainerType& thisPtr) { return aznumeric_cast<int>(thisPtr.size()); })
                        ->Attribute(AZ::Script::Attributes::Deprecated, true)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Method("clear", &ContainerType::clear)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                        ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Method("At", &At, { { {}, { "Index", "The index to read from", nullptr, BehaviorParameter::Traits::TR_INDEX } } })
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeFailureSlotName, "Index out of range")
                    ->Method("Back", &Back)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeFailureSlotName, "Empty vector")
                    ->Method("Capacity", &ContainerType::capacity)
                    ->Method("Clear", &ContainerType::clear)
                    ->Method("Empty", &ContainerType::empty)
                    ->Method("Erase", &Erase, { { {},{ "Index", "The index to erase", nullptr, BehaviorParameter::Traits::TR_INDEX } } })
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeFailureSlotName, "Index out of range")
                    ->Method("EraseRange", &EraseRange, { { {}, { "First Index", "The index of the first element to erase", nullptr, BehaviorParameter::Traits::TR_INDEX },{ "Last Index", "The index of the last element to erase", nullptr, BehaviorParameter::Traits::TR_INDEX } } })
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeFailureSlotName, "Index out of range")
                    ->Method("Front", &Front)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeFailureSlotName, "Empty vector")
                    ->Method("Insert", &Insert, { { {}, {}, { "Index", "The index at which to insert the value", nullptr, BehaviorParameter::Traits::TR_INDEX } } })
                        ->Attribute(AZ::Script::Attributes::Ignore, true)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)

                    ->Method("PopBack", &PopBack)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeFailureSlotName, "Empty vector")
                    ->Method("PushBack", static_cast<void(ContainerType::*)(typename ContainerType::const_reference)>(&ContainerType::push_back))
                    ->Method("Replace", &Replace, { { {}, { "Index", "The index to replace", nullptr, BehaviorParameter::Traits::TR_INDEX }, {} } })
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeFailureSlotName, "Index out of range")
                    ->Method("Reserve", &ContainerType::reserve)
                    ->Method("Resize", static_cast<void(ContainerType::*)(size_t)>(&ContainerType::resize))
                    ->Method("Size", [](ContainerType& thisPtr) { return aznumeric_cast<int>(thisPtr.size()); })
                    ->Method("Swap", static_cast<void(ContainerType::*)(ContainerType&)>(&ContainerType::swap))
                    ;
            }
        }

        // resize the container to the appropriate size if needed and set the element
        static void AssignAt(ContainerType& thisPtr, AZ::u64 index, T& value)
        {
            size_t uindex = static_cast<size_t>(index);
            if (thisPtr.size() <= uindex)
            {
                thisPtr.resize(uindex + 1);
            }
            thisPtr[uindex] = value;
        }
    };

    /// OnDemand reflection for AZStd::array
    template<class T, AZStd::size_t N>
    struct OnDemandReflection< AZStd::array<T, N> >
    {
        using ContainerType = AZStd::array<T, N>;

        static AZ::Outcome<T, AZStd::string> At(ContainerType& thisContainer, size_t index)
        {
            if (index >= 0 && index < thisContainer.size())
            {
                return AZ::Success(thisContainer.at(index));
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Index out of bounds: %zu (size: %zu)", index, thisContainer.size()));
            }
        }
        
        static AZ::Outcome<void, void> Replace(ContainerType& thisContainer, size_t index, T& value)
        {
            if (index >= 0 && index < thisContainer.size())
            {
                thisContainer[index] = value;
                return AZ::Success();
            }

            return AZ::Failure();
        }

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::ScriptOwn)
                    ->template Method<typename ContainerType::reference(ContainerType::*)(typename ContainerType::size_type)>("at", &ContainerType::at, {{ { "Index", "The index to read from", nullptr, BehaviorParameter::Traits::TR_INDEX } }})
                        ->Attribute(AZ::Script::Attributes::Deprecated, true)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::IndexRead)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Method("size", [](ContainerType*) { return aznumeric_cast<int>(N); })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                        ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    
                    ->Method("At", &At, {{ {}, { "Index", "The index to read from", nullptr, BehaviorParameter::Traits::TR_INDEX }}})
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeFailureSlotName, "Index out of range")
                    ->Method("Size", [](ContainerType*) { return aznumeric_cast<int>(N); })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)                      
                    ->template Method<typename ContainerType::reference(ContainerType::*)()>("Back", &ContainerType::back)
                    ->Method("Fill", &ContainerType::fill)
                    ->template Method<typename ContainerType::reference(ContainerType::*)()>("Front", &ContainerType::front)
                    ->Method("Replace", &Replace, { { {}, { "Index", "The index to replace", nullptr, BehaviorParameter::Traits::TR_INDEX }, {} } })
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeFailureSlotName, "Index out of range")
                    ->template Method<void(ContainerType::*)(ContainerType&)>("Swap", &ContainerType::swap)
                    ;
            }
        }
    };

    template<typename ValueT, typename ErrorT>
    struct OnDemandReflection<AZ::Outcome<ValueT, ErrorT>>
    {
        using OutcomeType = AZ::Outcome<ValueT, ErrorT>;

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                auto getValueFunc = [](OutcomeType* outcomePtr) { return outcomePtr->GetValue(); };
                auto getErrorFunc = [](OutcomeType* outcomePtr) { return outcomePtr->GetError(); };
                // note we can reflect iterator types and support iterators, as of know we want to keep it simple
                behaviorContext->Class<OutcomeType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<OutcomeType>::Get)
                    ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<OutcomeType>::Get)
                    ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<OutcomeType>::Get)
                    ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, AttributeIsValid::IfPresent)
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AttributeIsValid::IfPresent)
                    ->Method("Failure", [](const ErrorT failure) -> OutcomeType { return AZ::Failure(failure); })
                    ->Method("Success", [](const ValueT success) -> OutcomeType { return AZ::Success(success); })
                    ->Method("GetValue", getValueFunc)
                    ->Attribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, 0)
                    ->Method("GetError", getErrorFunc)
                    ->Attribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, 1)
                    ->Method("IsSuccess", &OutcomeType::IsSuccess)
                    ;
            }
        }
    };

    template<typename ValueT>
    struct OnDemandReflection<AZ::Outcome<ValueT, void>>
    {
        using OutcomeType = AZ::Outcome<ValueT, void>;

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                auto getValueFunc = [](OutcomeType* outcomePtr) { return outcomePtr->GetValue(); };
                // note we can reflect iterator types and support iterators, as of know we want to keep it simple
                behaviorContext->Class<OutcomeType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, true)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<OutcomeType>::Get)
                    ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<OutcomeType>::Get)
                    ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<OutcomeType>::Get)
                    ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, AttributeIsValid::IfPresent)
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AttributeIsValid::IfPresent)
                    ->Method("Failure", []() -> OutcomeType { return AZ::Failure(); })
                    ->Method("Success", [](const ValueT& success) -> OutcomeType { return AZ::Success(success); })
                    ->Method("GetValue", getValueFunc)
                    ->Attribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, 0)
                    ->Method("IsSuccess", &OutcomeType::IsSuccess)
                    ;
            }
        }
    };

    template<typename ErrorT>
    struct OnDemandReflection<AZ::Outcome<void, ErrorT>>
    {
        using OutcomeType = AZ::Outcome<void, ErrorT>;

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                auto getErrorFunc = [](OutcomeType* outcomePtr) { return outcomePtr->GetError(); };
                // note we can reflect iterator types and support iterators, as of know we want to keep it simple
                behaviorContext->Class<OutcomeType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, true)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<OutcomeType>::Get)
                    ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<OutcomeType>::Get)
                    ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<OutcomeType>::Get)
                    ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, AttributeIsValid::IfPresent)
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AttributeIsValid::IfPresent)
                    ->Method("Failure", [](const ErrorT& failure) -> OutcomeType { return AZ::Failure(failure); })
                    ->Method("Success", []() -> OutcomeType { return AZ::Success(); })
                    ->Method("GetError", getErrorFunc)
                    ->Attribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, 1)
                    ->Method("IsSuccess", &OutcomeType::IsSuccess)
                    ;
            }
        }
    };

    template<> // in case someone has an issue with bool
    struct OnDemandReflection<AZ::Outcome<void, void>>
    {
        using OutcomeType = AZ::Outcome<void, void>;

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                // note we can reflect iterator types and support iterators, as of know we want to keep it simple
                behaviorContext->Class<OutcomeType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, true)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<OutcomeType>::Get)
                    ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<OutcomeType>::Get)
                    ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<OutcomeType>::Get)
                    ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, AttributeIsValid::IfPresent)
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AttributeIsValid::IfPresent)
                    ->Method("Failure", []() -> OutcomeType { return AZ::Failure(); })
                    ->Method("Success", []() -> OutcomeType { return AZ::Success(); })
                    ->Method("IsSuccess", &OutcomeType::IsSuccess)
                    ;
            }
        }
    };

    template<typename T1, typename T2>
    struct OnDemandReflection<AZStd::pair<T1, T2>>
    {
        using ContainerType = AZStd::pair<T1, T2>;
        static T1& GetFirst(ContainerType& pair) { return pair.first; };
        static T2& GetSecond(ContainerType& pair) { return pair.second; };

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<ContainerType>::Get)
                    ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AttributeIsValid::IfPresent)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("first", [](ContainerType& thisPtr) { return thisPtr.first; }, [](ContainerType& thisPtr, const T1& value) { thisPtr.first = value; })
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                        ->Attribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, 0)
                    ->Property("second", [](ContainerType& thisPtr) { return thisPtr.second; }, [](ContainerType& thisPtr, const T2& value) { thisPtr.second = value; })
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                        ->Attribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, 1)
                    ->Method("ConstructTuple", [](const T1& first, const T2& second) { return AZStd::make_pair(first, second); })
                    ;
            }
        }
    };

    /// OnDemand reflection for AZStd::map
    template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator>
    struct OnDemandReflection< AZStd::unordered_map<Key, MappedType, Hasher, EqualKey, Allocator> >
    {
        using ContainerType = AZStd::unordered_map<Key, MappedType, Hasher, EqualKey, Allocator>;
        using KeyListType = AZStd::vector<Key, Allocator>;

        static AZ::Outcome<MappedType, AZStd::string> At(ContainerType& thisMap, Key& key)
        {
            auto it = thisMap.find(key);
            if (it != thisMap.end())
            {
                return AZ::Success(it->second);
            }

            return AZ::Failure(AZStd::string("Item not found for key"));
        }

        static AZ::Outcome<void, void> Erase(ContainerType& thisMap, Key& key)
        {
            const auto result = thisMap.erase(key);
            if (result)
            {
                return AZ::Success();
            }
            else
            {
                return AZ::Failure();
            }
        }

        static KeyListType GetKeys(ContainerType& thisMap)
        {
            KeyListType keys;
            for (auto entry : thisMap)
            {
                keys.push_back(entry.first);
            }
            return keys;
        }

        static void Insert(ContainerType& thisMap, Key& key, MappedType& value)
        {
            thisMap.insert(AZStd::make_pair(key, value));
        }

        static void Swap(ContainerType& thisMap, ContainerType& otherMap)
        {
            thisMap.swap(otherMap);
        }

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::ScriptOwn)
                    ->Method("At", &At)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeFailureSlotName, "Key not found")
                    ->Method("BucketCount", static_cast<typename ContainerType::size_type(ContainerType::*)() const>(&ContainerType::bucket_count))
                    ->Method("Empty", [](ContainerType& thisMap)->bool { return thisMap.empty(); })
                    ->Method("Erase", &Erase)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeFailureSlotName, "Key not found")
                    ->Method("GetKeys", &GetKeys)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Method("Has", [](ContainerType& map, Key& key)->bool { return map.find(key) != map.end(); })
                    ->Method("Insert", &Insert)
                    ->Method("Reserve", static_cast<void(ContainerType::*)(typename ContainerType::size_type)>(&ContainerType::reserve))
                    ->Method("Size", [](ContainerType& thisPtr) { return aznumeric_cast<int>(thisPtr.size()); })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                    ->Method("Swap", &Swap)
                    ;
            }
        }
    };

    /// OnDemand reflection for AZStd::set
    template<class Key, class Hasher, class EqualKey, class Allocator>
    struct OnDemandReflection< AZStd::unordered_set<Key, Hasher, EqualKey, Allocator> >
    {
        using ContainerType = AZStd::unordered_set<Key, Hasher, EqualKey, Allocator>;
        using KeyListType = AZStd::vector<Key, Allocator>;

        static AZ::Outcome<void, void> Erase(ContainerType& thisMap, Key& key)
        {
            const auto result = thisMap.erase(key);
            if (result)
            {
                return AZ::Success();
            }
            else
            {
                return AZ::Failure();
            }
        }

        static void Insert(ContainerType& thisSet, Key& key)
        {
            thisSet.insert(key);
        }

        static KeyListType GetKeys(ContainerType& thisSet)
        {
            KeyListType keys;
            for (auto entry : thisSet)
            {
                keys.push_back(entry);
            }
            return keys;
        }

        static void Swap(ContainerType& thisSet, ContainerType& otherSet)
        {
            thisSet.swap(otherSet);
        }

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<ContainerType>::Get)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::ScriptOwn)
                    ->Method("BucketCount", static_cast<typename ContainerType::size_type(ContainerType::*)() const>(&ContainerType::bucket_count))
                    ->Method("Erase", &Erase)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AttributeIsValid::IfPresent)
                        ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeFailureSlotName, "Key not found")
                    ->Method("Empty", [](ContainerType& thisSet)->bool { return thisSet.empty(); })
                    ->Method("Has", [](ContainerType& thisSet, Key& key)->bool { return thisSet.find(key) != thisSet.end(); })
                    ->Method("Insert", &Insert)
                    ->Method("Size", [](ContainerType* thisPtr) { return aznumeric_cast<int>(thisPtr->size()); })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                    ->Method("GetKeys", &GetKeys)
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Method("Reserve", static_cast<void(ContainerType::*)(typename ContainerType::size_type)>(&ContainerType::reserve))
                    ->Method("Swap", &Swap)
                    ;
            }
        }

    };

    template <>
    struct OnDemandReflection<AZStd::any>
    {
        // Reads the value in on top of the stack into an any
        static bool FromLua(lua_State* lua, int stackIndex, BehaviorValueParameter& value, BehaviorClass* valueClass, ScriptContext::StackVariableAllocator* stackTempAllocator)
        {
            if (stackTempAllocator)
            {
                // Note, this is safe to do in this case, even if we are reading an any pointer type
                // The BehaviorValueParameter remains in scope for the duration of AZ::LuaScriptCaller::Call, so
                // StoreResult, even though it's using temporarily allocated memory, should remain in scope for the lifetime of the CustomReaderWriter invocation
                value.m_value = stackTempAllocator->allocate(sizeof(AZStd::any), AZStd::alignment_of<AZStd::any>::value);

                // if a reference type, the StoreResult call will point to returned, temporary memory, so a value copy is necessary
                // this value was created by the stackTempAllocator, so it is acceptable to modify the BVP.
                value.m_traits = 0;

                if (valueClass->m_defaultConstructor)
                {
                    valueClass->m_defaultConstructor(value.m_value, valueClass->m_userData);
                }
            }
            AZ_Assert(value.m_value, "Invalid call to FromLua! Either a stack allocator must be passed, or value.m_value must be a valid storage location.");

            return value.StoreResult(AZStd::move(ScriptValue<AZStd::any>::StackRead(lua, stackIndex)));
        }

        // Pushes an any onto the Lua stack
        static void ToLua(lua_State* lua, BehaviorValueParameter& param)
        {
            AZStd::any* value = param.GetAsUnsafe<AZStd::any>();
            if (value)
            {
                ScriptValue<AZStd::any>::StackPush(lua, *value);
            }
        }

        static void Reflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<AZStd::any>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(Script::Attributes::Ignore, true)   // Don't reflect any type to script (there should never be an any instance in script)
                    ->Attribute(Script::Attributes::ReaderWriterOverride, ScriptContext::CustomReaderWriter(&ToLua, &FromLua))
                    ;
            }
        }
    };
}

