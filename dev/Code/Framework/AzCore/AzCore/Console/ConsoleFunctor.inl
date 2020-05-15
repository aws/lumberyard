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

#include <AzCore/Console/ConsoleTypeHelpers.h>


namespace AZ
{
    inline const char* ConsoleFunctorBase::GetName() const
    {
        return m_name;
    }


    inline const char* ConsoleFunctorBase::GetDesc() const
    {
        return m_desc;
    }


    inline ConsoleFunctorFlags ConsoleFunctorBase::GetFlags() const
    {
        return m_flags;
    }


    template <typename RETURN_TYPE>
    inline GetValueResult ConsoleFunctorBase::GetValue(RETURN_TYPE& outResult) const
    {
        AZStd::string buffer;
        const GetValueResult resultCode = GetValueAsString(buffer);

        if (resultCode != GetValueResult::Success)
        {
            return resultCode;
        }

        return ConsoleTypeHelpers::StringToValue(outResult, buffer)
            ? GetValueResult::Success
            : GetValueResult::TypeNotConvertible;
    }


    // This is forcibly inlined because we must guarantee this is compiled into and invoked from the calling module rather than the .exe that links AzCore
    AZ_FORCE_INLINE ConsoleFunctorBase*& ConsoleFunctorBase::GetDeferredHead()
    {
        s_deferredHeadInvoked = true;
        return s_deferredHead;
    }


    template <typename _TYPE, bool _REPLICATES_VALUE>
    inline ConsoleFunctor<_TYPE, _REPLICATES_VALUE>::ConsoleFunctor(const char* name, const char* desc, ConsoleFunctorFlags flags, _TYPE& object, RawFunctorSignature function)
        : ConsoleFunctorBase(name, desc, flags)
        , m_object(object)
        , m_function(function)
    {
        ;
    }


    template <typename _TYPE, bool _REPLICATES_VALUE>
    inline ConsoleFunctor<_TYPE, _REPLICATES_VALUE>::ConsoleFunctor(const char* name, const char* desc, ConsoleFunctorFlags flags, _TYPE& object, MemberFunctorSignature function)
        : ConsoleFunctorBase(name, desc, flags)
        , m_object(object)
        , m_function(function)
    {
        ;
    }


    template <typename _TYPE, bool _REPLICATES_VALUE>
    inline ConsoleFunctor<_TYPE, _REPLICATES_VALUE>::~ConsoleFunctor()
    {
        ;
    }


    template <typename _TYPE, bool _REPLICATES_VALUE>
    inline void ConsoleFunctor<_TYPE, _REPLICATES_VALUE>::operator()(const StringSet& arguments)
    {
        auto functorVisitor = [this, arguments](auto&& functor)
        {
            AZStd::invoke(functor, m_object, arguments);
        };
        AZStd::visit(functorVisitor, m_function);

        AZStd::string value;
        const GetValueResult result = ConsoleFunctorBase::GetValue(value);

        if ((m_flags & ConsoleFunctorFlags::IsInvisible) != ConsoleFunctorFlags::IsInvisible)
        {
            AZ_TracePrintf("Az Console", "> %s : %s\n", GetName(), value.empty() ? "<empty>" : value.c_str());
        }
    }


    template <typename _TYPE, bool _REPLICATES_VALUE>
    struct ConsoleReplicateHelper;


    template <typename _TYPE>
    struct ConsoleReplicateHelper<_TYPE, false>
    {
        static void GetReplicationString(_TYPE&, const char* name, AZStd::string& outString)
        {
            outString = name;
        }

        static bool StringToValue(_TYPE&, const StringSet&)
        {
            return false;
        }

        static bool ValueToString(_TYPE&, AZStd::string&)
        {
            return false;
        }
    };


    template <typename _TYPE>
    struct ConsoleReplicateHelper<_TYPE, true>
    {
        static void GetReplicationString(_TYPE& instance, const char* name, AZStd::string& outString)
        {
            AZStd::string valueString;
            ValueToString(instance, valueString);
            outString = AZStd::string(name) + " " + valueString;
        }

        static bool StringToValue(_TYPE& instance, const StringSet& arguments)
        {
            return instance.StringToValue(arguments);
        }

        static void ValueToString(_TYPE& instance, AZStd::string& outString)
        {
            instance.ValueToString(outString);
        }
    };


    template <typename _TYPE, bool _REPLICATES_VALUE>
    inline void ConsoleFunctor<_TYPE, _REPLICATES_VALUE>::GetReplicationString(AZStd::string& outString) const
    {
        ConsoleReplicateHelper<_TYPE, _REPLICATES_VALUE>::GetReplicationString(m_object, GetName(), outString);
    }


    template <typename _TYPE, bool _REPLICATES_VALUE>
    inline _TYPE& ConsoleFunctor<_TYPE, _REPLICATES_VALUE>::GetValue()
    {
        return m_object;
    }


    template <typename _TYPE, bool _REPLICATES_VALUE>
    inline GetValueResult ConsoleFunctor<_TYPE, _REPLICATES_VALUE>::GetValueAsString(AZStd::string& outString) const
    {
        ConsoleReplicateHelper<_TYPE, _REPLICATES_VALUE>::ValueToString(m_object, outString);
        return GetValueResult::Success;
    }


    // ConsoleFunctor Specialization for running non-member console functions with no instance
    template <bool _REPLICATES_VALUE>
    inline ConsoleFunctor<void, _REPLICATES_VALUE>::ConsoleFunctor(const char* name, const char* desc, ConsoleFunctorFlags flags, FunctorSignature function)
    :   ConsoleFunctorBase(name, desc, flags)
    ,   m_function(function)
    {
        ;
    }


    template <bool _REPLICATES_VALUE>
    inline ConsoleFunctor<void, _REPLICATES_VALUE>::~ConsoleFunctor()
    {
        ;
    }


    template <bool _REPLICATES_VALUE>
    inline void ConsoleFunctor<void, _REPLICATES_VALUE>::operator()(const StringSet& arguments)
    {
        (*m_function)(arguments);
    }


    template <bool _REPLICATES_VALUE>
    inline void ConsoleFunctor<void, _REPLICATES_VALUE>::GetReplicationString(AZStd::string& outString) const
    {
        outString = GetName();
    }


    template <bool _REPLICATES_VALUE>
    inline GetValueResult ConsoleFunctor<void, _REPLICATES_VALUE>::GetValueAsString(AZStd::string& outString) const
    {
        return GetValueResult::NotImplemented;
    }
}
