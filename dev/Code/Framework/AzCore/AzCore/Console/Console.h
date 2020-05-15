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

#include <AzCore/Console/IConsole.h>


namespace AZ
{
    //! @class Console
    //! 
    //! A simple console class for providing text based variable and process interaction.
    class Console final
        : public IConsole
    {
    public:

        AZ_RTTI(Console, "{CF6DCDE7-1A66-442C-BA87-01A432C13E7D}", IConsole);

        Console();
        ~Console() override;

        bool PerformCommand(const char* command, ConsoleFunctorFlags requiredSet = ConsoleFunctorFlags::Null, ConsoleFunctorFlags requiredClear = ConsoleFunctorFlags::ReadOnly) override;

        void ExecuteCommandLine(const char* commandLine) override;

        bool HasCommand(const char* command) override;

        ConsoleFunctorBase* FindCommand(const char* command) override;

        AZStd::string AutoCompleteCommand(const char* command) override;

        void VisitRegisteredFunctors(FunctorVisitorInterface& visitor) override;

        ConsoleFunctorBase*& RetrieveHeadElement() override;

        void LinkDeferredFunctors(ConsoleFunctorBase*& deferredHead) override;

    private:

        //! Invokes a single console command, optionally returning the command output.
        //! 
        //! @param command       the function to invoke
        //! @param inputs        the set of inputs to provide the function
        //! @param requiredSet   a set of flags that must be set on the functor for it to execute
        //! @param requiredClear a set of flags that must *NOT* be set on the functor for it to execute
        //! @param outFlags      if invoked, these are the flags associated with the invoked command
        //! 
        //! @return boolean true on success, false otherwise
        bool DispatchCommand(const AZStd::string& command, const StringSet& inputs, ConsoleFunctorFlags requiredSet, ConsoleFunctorFlags requiredClear, ConsoleFunctorFlags& outFlags);

        AZ_DISABLE_COPY_MOVE(Console);

        ConsoleFunctorBase* m_head;

        friend class ConsoleFunctorBase;

    };
}
