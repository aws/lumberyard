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
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Script/ScriptContext.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzCore/std/string/tokenize.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/algorithm.h>

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
                    typename ContainerType::basic_string str;
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
                    ->Method("ToString", [](const ContainerType& stringView) { return stringView.to_string(); }, { { { "Reference", "String view object being converted to string" } } })
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
            BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
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

        // TODO: Count reflection types for a proper un-reflect 

        // vector (like most containers uses size_t for an index, currently in script we handle u64 specially (which we should address), till then we need a wrapper
        static T& ScriptAt(ContainerType* thisPtr, int index)
        {
            return thisPtr->at(index - 1);
        }

        // resize the container to the appropriate size if needed and set the element
        static void Insert(ContainerType* thisPtr, int index, T& value)
        {
            if (thisPtr->size() <= index)
            {
                thisPtr->resize(index + 1);
            }
            (*thisPtr)[index] = value;
        }

        static void LuaInsert(ContainerType* thisPtr, int index, T& value)
        {
            Insert(thisPtr, index - 1, value);
        }

        static void Reflect(ReflectContext* context)
        {
            BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
            if (behaviorContext)
            {
                // note we can reflect iterator types and support iterators, as of know we want to keep it simple
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::ScriptOwn)
                    ->template Method<void(ContainerType::*)(typename ContainerType::const_reference)>("push_back", &ContainerType::push_back)
                    ->Method("pop_back", &ContainerType::pop_back)
                    ->template Method<typename ContainerType::reference(ContainerType::*)(typename ContainerType::size_type)>("at", &ContainerType::at)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::IndexRead)
                        ->Attribute(AZ::Script::Attributes::MethodOverride, &ScriptAt)
                    ->Method("insert", &Insert)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::IndexWrite)
                        ->Attribute(AZ::Script::Attributes::MethodOverride, &LuaInsert)
                    ->Method("size", [](ContainerType* thisPtr) { return aznumeric_cast<int>(thisPtr->size()); })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                    ->Method("clear", &ContainerType::clear)
                    /// ... iterate
                    ;
            }
        }
    };

    /// OnDemand reflection for AZStd::array
    template<class T, AZStd::size_t N>
    struct OnDemandReflection< AZStd::array<T, N> >
    {
        using ContainerType = AZStd::array<T, N>;

        // TODO: Count reflection types for a proper un-reflect 

        static T& ScriptAt(ContainerType* thisPtr, int index)
        {
            return thisPtr->at(index - 1);
        }

        static void Reflect(ReflectContext* context)
        {
            BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
            if (behaviorContext)
            {
                // note we can reflect iterator types and support iterators, as of know we want to keep it simple
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::ScriptOwn)
                    ->template Method<typename ContainerType::reference(ContainerType::*)(typename ContainerType::size_type)>("at", &ContainerType::at)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::IndexRead)
                        ->Attribute(AZ::Script::Attributes::MethodOverride, &ScriptAt)
                    ->Method("size", [](ContainerType*) { return aznumeric_cast<int>(N); })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                    /// ... iterate
                    ;
            }
        }
    };

    template<typename T1, typename T2>
    struct OnDemandReflection<AZStd::pair<T1, T2>>
    {
        using ContainerType = AZStd::pair<T1, T2>;

        static void Reflect(ReflectContext* context)
        {
            BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
            if (behaviorContext)
            {
                // note we can reflect iterator types and support iterators, as of know we want to keep it simple
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("first", [](ContainerType* thisPtr) { return thisPtr->first; }, [](ContainerType* thisPtr, const T1& value) { thisPtr->first = value; })
                    ->Property("second", [](ContainerType* thisPtr) { return thisPtr->second; }, [](ContainerType* thisPtr, const T2& value) { thisPtr->second = value; })
                    ;
            }
        }
    };

    template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator>
    struct OnDemandReflection< AZStd::unordered_map<Key, MappedType, Hasher, EqualKey, Allocator> >
    {
        using ContainerType = AZStd::unordered_map<Key, MappedType, Hasher, EqualKey, Allocator>;

        static void Reflect(ReflectContext* context)
        {
            BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<ContainerType>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::ScriptOwn);
            }
        }
    };

    
}

