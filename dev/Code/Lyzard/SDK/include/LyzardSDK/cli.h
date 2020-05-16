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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Outcome/Outcome.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/std/functional.h>

#include <AzFramework/CommandLine/CommandLine.h>

#include <LyzardSDK/Base.h>

namespace Lyzard
{
    namespace CLI
    {
        namespace Internal
        {
            struct Argument;
            struct PosArgument;
        }

        class Command
        {
        public:
            AZ_RTTI(Command, "{DCBC307D-3128-4365-9F9A-2974285F86E9}");

            //////////////////////////////////////////////////////////////////////////
            // Interface

            Command(const AZStd::string& name, const AZStd::string& description);
            virtual ~Command() = default;

            //! Execute the command. Returns error message if failure.
            virtual StringOutcome Execute() = 0;

            //////////////////////////////////////////////////////////////////////////
            // Utilities for child commands

            //! Register an argument that doesn't have a value, only name
            void RegisterPositionalArgument(const AZStd::string& name, AZStd::string* store, const AZStd::string& description);

            //! Register an argument
            void RegisterArgument(const AZStd::string& name, AZStd::string* store, const AZStd::string& description);
            void RegisterArgument(const AZStd::string& name, AZStd::vector<AZStd::string>* store, const AZStd::string& description);
            void RegisterArgument(const AZStd::string& name, bool* store, const AZStd::string& description);

            //! Get the name of the command
            const AZStd::string& GetName() const { return m_name; }
            const AZStd::string& GetDescription() const { return m_description; }
            bool ShouldPrintHelp() const { return m_printHelp; }

            //////////////////////////////////////////////////////////////////////////
            // Utilities for Namespace / Framework

            //! Update command's depth
            size_t GetDepth() const { return m_depth; }
            virtual void SetDepth(size_t newDepth) { m_depth = newDepth; }

            //! Processes arguments
            virtual StringOutcome ProcessArgs(const AzFramework::CommandLine& args);

            //! Prints generated help text
            virtual AZStd::string GetHelpText();

        private:
            bool m_printHelp = false;
            size_t m_depth = 0;
            AZStd::string m_name;
            AZStd::string m_description;

            // Switches
            AZStd::vector<AZStd::shared_ptr<Internal::Argument>> m_params;

            // Positional args
            AZStd::vector<AZStd::shared_ptr<Internal::PosArgument>> m_posParams;
        };

        class Namespace final
            : public Command
        {
        public:
            AZ_RTTI(Namespace, "{9DFFC621-930F-41F2-BF00-CBA4B4572335}", Command);

            Namespace(const AZStd::string& name);

            //////////////////////////////////////////////////////////////////////////
            // Command overrides
            StringOutcome Execute() override final;
            StringOutcome ProcessArgs(const AzFramework::CommandLine& args) override final;
            AZStd::string GetHelpText() override final;
            void SetDepth(size_t newDepth) override final;
            //////////////////////////////////////////////////////////////////////////

            //! Register a child command. Returns this.
            Namespace* RegisterCommand(AZStd::shared_ptr<Command> ptr);

            //! Helper for RegisterCommand(AZStd::make_shared<CommandT>()).
            template <class CommandT>
            Namespace* CreateCommand() { return RegisterCommand(AZStd::make_shared<CommandT>()); }

            //! Creates a child namespace
            AZStd::shared_ptr<Namespace> WithNamespace(const AZStd::string& name);

        private:
            // Child commands
            AZStd::unordered_map<AZStd::string, AZStd::shared_ptr<Command>> m_commands;

            // Arguments
            AZStd::string m_cmdNameToExecute;
            AZStd::shared_ptr<Command> m_cmdToExecute;
        };
    }
}
