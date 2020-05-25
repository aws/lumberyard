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

#include <stdint.h>
#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/variant.h>


namespace AZ
{
    class IConsole;
    class Console;

    using StringSet = AZStd::vector<AZStd::string>;

    enum class ConsoleFunctorFlags
    {
        Null           = 0        // Empty flags
    ,   DontReplicate  = (1 << 0) // Should not be replicated
    ,   ServerOnly     = (1 << 1) // Should never replicate to clients
    ,   ReadOnly       = (1 << 2) // Should not be invoked at runtime
    ,   IsCheat        = (1 << 3) // Command is a cheat, may require escalated privileges to modify
    ,   IsInvisible    = (1 << 4) // Should not be shown in the console for autocomplete
    ,   IsDeprecated   = (1 << 5) // Command is deprecated, show a warning when invoked
    ,   NeedsReload    = (1 << 6) // Level should be reloaded after executing this command
    ,   AllowClientSet = (1 << 7) // Allow clients to modify this cvar even in release (this alters the cvar for all connected servers and clients, be VERY careful enabling this flag)
    };

    inline ConsoleFunctorFlags operator |(ConsoleFunctorFlags left, ConsoleFunctorFlags right) { return static_cast<ConsoleFunctorFlags>(static_cast<uint32_t>(left) | static_cast<uint32_t>(right)); }
    inline ConsoleFunctorFlags operator &(ConsoleFunctorFlags left, ConsoleFunctorFlags right) { return static_cast<ConsoleFunctorFlags>(static_cast<uint32_t>(left) & static_cast<uint32_t>(right)); }


    enum class GetValueResult
    {
        Success,
        NotImplemented,
        TypeNotConvertible,
        ConsoleVarNotFound,
    };

    const char* GetEnumString(GetValueResult value);


    //! @class ConsoleFunctorBase
    //! 
    //! Base class for console functors.
    class ConsoleFunctorBase
    {
    public:

        //! Constructor.
        //! 
        //! @param name  the string name of the functor, used to identify and invoke the functor through the console interface
        //! @param desc  a string help description of the functor
        //! @param flags a set of flags that mutate the behaviour of the functor
        ConsoleFunctorBase(const char* name, const char* desc, ConsoleFunctorFlags flags);

        //! Destructor.
        virtual ~ConsoleFunctorBase();

        //! Returns the string name of this functor.
        //! 
        //! @return the string name of this functor
        const char* GetName() const;

        //! Returns the help descriptor of this functor.
        //! 
        //! @return the help descriptor of this functor
        const char* GetDesc() const;

        //! Returns the flags set on this functor instance.
        //! 
        //! @return the flags set on this functor instance
        ConsoleFunctorFlags GetFlags() const;

        //! Execute operator, calling this executes the functor.
        //! 
        //! @param arguments set of string inputs to the functor
        virtual void operator()(const StringSet& arguments) = 0;

        //! For functors that can be replicated (cvars).
        //! This will generate a replication string suitable for remote execution
        //! 
        //! @param outString the output string which can be remotely executed
        virtual void GetReplicationString(AZStd::string& outString) const = 0;

        //! Attempts to retrieve the functor object instance as the provided type.
        //! 
        //! @param outResult the value to store the result in
        //! 
        //! @return GetConsoleValueResult::Success or an appropriate error code
        template <typename RETURN_TYPE>
        GetValueResult GetValue(RETURN_TYPE& outResult) const;

        //! Used internally to link cvars and functors from various modules to the console as they are loaded.
        static ConsoleFunctorBase*& GetDeferredHead();

    protected:

        virtual GetValueResult GetValueAsString(AZStd::string& outString) const;

        void Link(ConsoleFunctorBase*& head);
        void Unlink(ConsoleFunctorBase*& head);

        const char*  m_name = "";
        const char*  m_desc = "";
        ConsoleFunctorFlags m_flags = ConsoleFunctorFlags::Null;

        IConsole* m_console = nullptr;

        ConsoleFunctorBase* m_prev = nullptr;
        ConsoleFunctorBase* m_next = nullptr;

        bool m_isDeferred = true;

        static ConsoleFunctorBase* s_deferredHead;
        static bool s_deferredHeadInvoked;

        friend class Console;

    };


    //! @class ConsoleFunctor
    //! 
    //! Console functor which wraps a function call into an object instance.
    template <typename _TYPE, bool _REPLICATES_VALUE>
    class ConsoleFunctor final
    :   public ConsoleFunctorBase
    {
    public:

        using MemberFunctorSignature = void(_TYPE::*)(const StringSet&);
        using RawFunctorSignature = void(*)(_TYPE&, const StringSet&);

        //! Constructors.
        //! 
        //! @param name  the string name of the functor, used to identify and invoke the functor through the console interface
        //! @param desc  a string help description of the functor
        //! @param flags a set of flags that mutate the behaviour of the functor
        //! @param object reference to the data storage object
        ConsoleFunctor(const char* name, const char* desc, ConsoleFunctorFlags flags, _TYPE& object, MemberFunctorSignature function);
        ConsoleFunctor(const char* name, const char* desc, ConsoleFunctorFlags flags, _TYPE& object, RawFunctorSignature function);

        ~ConsoleFunctor() override;

        //! Execute operator, calling this executes the functor.
        //! 
        //! @param arguments set of string inputs to the functor
        virtual void operator()(const StringSet& arguments) override;

        //! This will generate a replication string suitable for remote execution.
        //! For functors that can be replicated (cvars)
        //! 
        //! @param outString the output string which can be remotely executed
        virtual void GetReplicationString(AZStd::string& outString) const override;

        //! Returns reference typed stored type wrapped stored by ConsoleFunctor.
        //! 
        //! @return reference to underlying type
        _TYPE& GetValue();

    private:

        GetValueResult GetValueAsString(AZStd::string& outString) const override;

        // Console Functors are not copyable
        ConsoleFunctor& operator=(const ConsoleFunctor&) = delete;
        ConsoleFunctor(const ConsoleFunctor&) = delete;

        _TYPE& m_object;

        using FunctorUnion = AZStd::variant<RawFunctorSignature, MemberFunctorSignature>;
        FunctorUnion m_function;

    };


    //! @class ConsoleFunctor
    //! 
    //! Console functor specialization for non-member console functions (no instance to invoke on).
    template <bool _REPLICATES_VALUE>
    class ConsoleFunctor<void, _REPLICATES_VALUE> final
    :   public ConsoleFunctorBase
    {
    public:

        using FunctorSignature = void(*)(const StringSet&);

        ConsoleFunctor(const char* name, const char* desc, ConsoleFunctorFlags flags, FunctorSignature function);
        ~ConsoleFunctor() override;

        //! Execute operator, calling this executes the functor.
        //! 
        //! @param arguments set of string inputs to the functor
        virtual void operator()(const StringSet& arguments) override;

        //! This will generate a replication string suitable for remote execution.
        //! For functors that can be replicated (cvars)
        //! 
        //! @param outString the output string which can be remotely executed
        virtual void GetReplicationString(AZStd::string& outString) const override;

    private:

        GetValueResult GetValueAsString(AZStd::string& outString) const override;

        FunctorSignature m_function;

    };
}

#include <AzCore/Console/ConsoleFunctor.inl>
