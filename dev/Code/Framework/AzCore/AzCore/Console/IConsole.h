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

#include <AzCore/Console/ConsoleFunctor.h>
#include <AzCore/Console/ConsoleDataWrapper.h>
#include <AzCore/RTTI/RTTI.h>


namespace AZ
{
    //! @class Console
    //! 
    //! A simple console class for providing text based variable and process interaction.
    class IConsole
    {
    public:

        class FunctorVisitorInterface
        {
        public:
            virtual void Visit(ConsoleFunctorBase* functor) = 0;
        };

        AZ_RTTI(IConsole, "{20001930-119D-4A80-BD67-825B7E4AEB3D}");

        IConsole() = default;
        virtual ~IConsole() = default;

        //! Invokes a single console command, optionally returning the command output.
        //! 
        //! @param command       the command string to parse and execute on
        //! @param requiredSet   a set of flags that must be set on the functor for it to execute
        //! @param requiredClear a set of flags that must *NOT* be set on the functor for it to execute
        //! 
        //! @return boolean true on success, false otherwise
        virtual bool PerformCommand(const char* command, ConsoleFunctorFlags requiredSet = ConsoleFunctorFlags::Null, ConsoleFunctorFlags requiredClear = ConsoleFunctorFlags::ReadOnly) = 0;

        //! Invokes all of the commands as contained in a concatenated command-line string.
        //! 
        //! @param commandLine the concatenated command-line string to execute
        virtual void ExecuteCommandLine(const char* commandLine) = 0;

        //! HasCommand is used to determine if the console knows about a command.
        //! 
        //! @param command the command we are checking for
        //! 
        //! @return boolean true on if the command is registered, false otherwise
        virtual bool HasCommand(const char* command) = 0;

        //! FindCommand finds the console command with the specified console string.
        //! 
        //! @param command the command that is being searched for
        //! 
        //! @return non-null pointer to the console command if found
        virtual ConsoleFunctorBase* FindCommand(const char* command) = 0;

        //! Prints all commands of which the input is a prefix.
        //! 
        //! @param command the prefix string to dump all matching commands for
        //! 
        //! @return boolean true on success, false otherwise
        virtual AZStd::string AutoCompleteCommand(const char* command) = 0;

        //! Retrieves the value of the requested cvar.
        //! 
        //! @param command the name of the cvar to find and retrieve the current value of
        //! @param outValue reference to the instance to write the current cvar value to
        //! 
        //! @return GetValueResult::Success if the operation succeeded, or an error result if the operation failed
        template<typename RETURN_TYPE>
        GetValueResult GetCvarValue(const char* command, RETURN_TYPE& outValue);

        //! Visits all registered console functors.
        //! 
        //! @param visitor the instance to visit all functors with
        virtual void VisitRegisteredFunctors(FunctorVisitorInterface& visitor) = 0;

        //! Internal access to the head element, used for construction and destruction of cvars and cfuncs.
        //! 
        //! @return pointer to the consoles head element
        virtual ConsoleFunctorBase*& RetrieveHeadElement() = 0;

        //! Should be invoked for every module that gets loaded.
        virtual void LinkDeferredFunctors(ConsoleFunctorBase*& deferredHead) = 0;

        AZ_DISABLE_COPY_MOVE(IConsole);

    };
}


template<typename RETURN_TYPE>
inline AZ::GetValueResult AZ::IConsole::GetCvarValue(const char* command, RETURN_TYPE& outValue)
{
    ConsoleFunctorBase* cvarFunctor = FindCommand(command);

    if (cvarFunctor == nullptr)
    {
        return GetValueResult::ConsoleVarNotFound;
    }

    return cvarFunctor->GetValue(outValue);
}

template <typename _TYPE, typename = void>
static constexpr AZ::ThreadSafety ConsoleThreadSafety = AZ::ThreadSafety::RequiresLock;
template <typename _TYPE>
static constexpr AZ::ThreadSafety ConsoleThreadSafety<_TYPE, std::enable_if_t<std::is_arithmetic_v<_TYPE>>> = AZ::ThreadSafety::UseStdAtomic;


//! Standard cvar macro, provides no external linkage.
//! Do not use this macro inside a header, use this macro inside cpp files only
//! 
//! @param _TYPE the data type of the cvar
//! @param _NAME the name of the cvar
//! @param _INIT the intial value to assign to the cvar
//! @param _FLAGS a set of AzFramework::ConsoleFunctorFlags used to mutate behaviour
//! @param _CALLBACK this is an optional callback function to get invoked when a cvar changes value
//!        ** YOU HAVE NO GUARANTEES AS TO WHAT THREAD WILL INVOKE YOUR CALLBACK ** It is the responsibility of the implementor of the callback handler to ensure thread safety
//! @param _DESC a description of the cvar
#define AZ_CVAR(_TYPE, _NAME, _INIT, _CALLBACK, _FLAGS, _DESC) \
    using CVarDataWrapperType##_NAME = AZ::ConsoleDataWrapper<_TYPE, ConsoleThreadSafety<_TYPE>>; \
    static CVarDataWrapperType##_NAME _NAME(_INIT, _CALLBACK, #_NAME, _DESC, _FLAGS)

//! Cvar macro that creates a console variable with external linkage.
//! Do not use this macro inside a header, use this macro inside cpp files only
//! 
//! @param _TYPE the data type of the cvar
//! @param _NAME the name of the cvar
//! @param _INIT the intial value to assign to the cvar
//! @param _FLAGS a set of AzFramework::ConsoleFunctorFlags used to mutate behaviour
//! @param _CALLBACK this is an optional callback function to get invoked when a cvar changes value
//!        ** YOU HAVE NO GUARANTEES AS TO WHAT THREAD WILL INVOKE YOUR CALLBACK ** It is the responsibility of the implementor of the callback handler to ensure thread safety
//! @param _DESC a description of the cvar
#define AZ_CVAR_EXTERNABLE(_TYPE, _NAME, _INIT, _CALLBACK, _FLAGS, _DESC) \
    using CVarDataWrapperType##_NAME = AZ::ConsoleDataWrapper<_TYPE, ConsoleThreadSafety<_TYPE>>; \
    CVarDataWrapperType##_NAME _NAME(_INIT,_CALLBACK, #_NAME, _DESC, _FLAGS)

//! Cvar macro that externs a console variable.
//! 
//! @param _TYPE the data type of the cvar to extern
//! @param _NAME the name of the cvar to extern
#define AZ_CVAR_EXTERNED(_TYPE, _NAME) \
    using CVarDataWrapperType##_NAME = AZ::ConsoleDataWrapper<_TYPE, ConsoleThreadSafety<_TYPE>>; \
    extern CVarDataWrapperType##_NAME _NAME;

//! Implements a console functor for a class member function.
//! 
//! @param _CLASS the that the function gets invoked on
//! @param _FUNCTION the method to invoke
//!        ** YOU HAVE NO GUARANTEES AS TO WHAT THREAD WILL INVOKE YOUR FUNCION ** It is the responsibility of the implementor of the console function to ensure thread safety
//! @param _INSTANCE the instance that the class method gets invoked on
//! @param _FLAGS a set of AzFramework::ConsoleFunctorFlags used to mutate behaviour
//! @param _DESC a description of the cvar
#define AZ_CONSOLEFUNC(_CLASS, _FUNCTION, _INSTANCE, _FLAGS, _DESC) \
    static AZ::ConsoleFunctor<_CLASS, false> Functor##_FUNCTION(#_CLASS "." #_FUNCTION, _DESC, _FLAGS, *(_INSTANCE), &_CLASS::_FUNCTION)

//! Implements a console functor for a non-member function.
//! 
//! @param _FUNCTION the method to invoke
//!        ** YOU HAVE NO GUARANTEES AS TO WHAT THREAD WILL INVOKE YOUR FUNCION ** It is the responsibility of the implementor of the console function to ensure thread safety
//! @param _FLAGS a set of AzFramework::ConsoleFunctorFlags used to mutate behaviour
//! @param _DESC a description of the cvar
#define AZ_CONSOLEFREEFUNC(_FUNCTION, _FLAGS, _DESC) \
    static AZ::ConsoleFunctor<void, false> \
        Functor##_NAME(#_FUNCTION, _DESC, _FLAGS, &_FUNCTION)
