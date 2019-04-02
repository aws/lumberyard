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

#include "CommandManager.h"
#include "LogManager.h"
#include "CommandManagerCallback.h"
#include "StringConversions.h"


//#define MCORE_COMMANDMANAGER_PERFORMANCE

namespace MCore
{
    // constructor
    CommandManager::CommandHistoryEntry::CommandHistoryEntry(CommandGroup* group, Command* command, const CommandLine& parameters, AZ::u32 historyItemNr)
    {
        mCommandGroup       = group;
        mExecutedCommand    = command;
        mParameters         = parameters;
        m_historyItemNr     = historyItemNr;
    }


    // destructor
    CommandManager::CommandHistoryEntry::~CommandHistoryEntry()
    {
        // remark: the mCommand and mCommandGroup are automatically deleted after popping from the history
    }


    AZStd::string CommandManager::CommandHistoryEntry::ToString(CommandGroup* group, Command* command, AZ::u32 historyItemNr)
    {
        if (group)
        {
            return AZStd::string::format("%.3d - %s", historyItemNr, group->GetGroupName());
        }
        else if (command)
        {
            return AZStd::string::format("%.3d - %s", historyItemNr, command->GetHistoryName());
        }

        return "";
    }


    AZStd::string CommandManager::CommandHistoryEntry::ToString() const
    {
        return ToString(mCommandGroup, mExecutedCommand, m_historyItemNr);
    }


    // constructor
    CommandManager::CommandManager()
    {
        mCommands.Reserve(1000);

        // set default values
        mMaxHistoryEntries      = 100;
        mHistoryIndex           = -1;
        m_totalNumHistoryItems   = 0;

        // preallocate history entries
        mCommandHistory.Reserve(mMaxHistoryEntries);

        m_commandsInExecution = 0;
    }


    // destructor
    CommandManager::~CommandManager()
    {
        for (auto element : mRegisteredCommands)
        {
            Command* command = element.second;
            delete command;
        }
        mRegisteredCommands.clear();

        // remove all callbacks
        RemoveCallbacks();

        // destroy the command history
        while (mCommandHistory.GetIsEmpty() == false)
        {
            PopCommandHistory();
        }
    }


    // push a command group on the history
    void CommandManager::PushCommandHistory(CommandGroup* commandGroup)
    {
        // if we reached the maximum number of history entries remove the oldest one
        if (mCommandHistory.GetLength() >= mMaxHistoryEntries)
        {
            PopCommandHistory();
            mHistoryIndex = mCommandHistory.GetLength() - 1;
        }

        uint32 i;
        if (mCommandHistory.GetLength() > 0)
        {
            // remove unneeded commands
            const uint32 numToRemove = mCommandHistory.GetLength() - mHistoryIndex - 1;
            const uint32 numCallbacks = mCallbacks.GetLength();
            for (i = 0; i < numCallbacks; ++i)
            {
                for (uint32 a = 0; a < numToRemove; ++a)
                {
                    mCallbacks[i]->OnRemoveCommand(mHistoryIndex + 1);
                }
            }

            for (uint32 a = 0; a < numToRemove; ++a)
            {
                delete mCommandHistory[mHistoryIndex + 1].mExecutedCommand;
                delete mCommandHistory[mHistoryIndex + 1].mCommandGroup;
                mCommandHistory.Remove(mHistoryIndex + 1);
            }
        }

        // resize the command history
        mCommandHistory.Resize(mHistoryIndex + 1);

        // add a command history entry
        m_totalNumHistoryItems++;
        mCommandHistory.Add(CommandHistoryEntry(commandGroup, nullptr, CommandLine(), m_totalNumHistoryItems));

        // increase the history index
        mHistoryIndex++;

        // perform callbacks
        const uint32 numCallbacks = mCallbacks.GetLength();
        for (i = 0; i < numCallbacks; ++i)
        {
            mCallbacks[i]->OnAddCommandToHistory(mHistoryIndex, commandGroup, nullptr, CommandLine());
        }
    }


    // save command in the history
    void CommandManager::PushCommandHistory(Command* command, const CommandLine& parameters)
    {
        uint32 i;
        if (mCommandHistory.GetLength() > 0)
        {
            // if we reached the maximum number of history entries remove the oldest one
            if (mCommandHistory.GetLength() >= mMaxHistoryEntries)
            {
                PopCommandHistory();
                mHistoryIndex = mCommandHistory.GetLength() - 1;
            }

            // remove unneeded commands
            uint32 numToRemove = mCommandHistory.GetLength() - mHistoryIndex - 1;
            const uint32 numCallbacks = mCallbacks.GetLength();
            for (i = 0; i < numCallbacks; ++i)
            {
                for (uint32 a = 0; a < numToRemove; ++a)
                {
                    mCallbacks[i]->OnRemoveCommand(mHistoryIndex + 1);
                }
            }

            for (uint32 a = 0; a < numToRemove; ++a)
            {
                delete mCommandHistory[mHistoryIndex + 1].mExecutedCommand;
                delete mCommandHistory[mHistoryIndex + 1].mCommandGroup;
                mCommandHistory.Remove(mHistoryIndex + 1);
            }
        }

        // resize the command history
        mCommandHistory.Resize(mHistoryIndex + 1);

        // add a command history entry
        m_totalNumHistoryItems++;
        mCommandHistory.Add(CommandHistoryEntry(nullptr, command, parameters, m_totalNumHistoryItems));

        // increase the history index
        mHistoryIndex++;

        // perform callbacks
        const uint32 numCallbacks = mCallbacks.GetLength();
        for (i = 0; i < numCallbacks; ++i)
        {
            mCallbacks[i]->OnAddCommandToHistory(mHistoryIndex, nullptr, command, parameters);
        }
    }


    // remove the oldest entry in the history stack
    void CommandManager::PopCommandHistory()
    {
        // perform callbacks
        const uint32 numCallbacks = mCallbacks.GetLength();
        for (uint32 i = 0; i < numCallbacks; ++i)
        {
            mCallbacks[i]->OnRemoveCommand(0);
        }

        // destroy the command and remove it from the command history
        Command* command = mCommandHistory.GetFirst().mExecutedCommand;
        if (command)
        {
            delete command;
        }
        else
        {
            delete mCommandHistory.GetFirst().mCommandGroup;
        }

        mCommandHistory.RemoveFirst();
    }


    bool CommandManager::ExecuteCommand(const AZStd::string& command, AZStd::string& outCommandResult, bool addToHistory, Command** outExecutedCommand, CommandLine* outExecutedParameters, bool callFromCommandGroup, bool clearErrors, bool handleErrors)
    {
        AZStd::string commandResult;
        bool result = ExecuteCommand(command.c_str(), commandResult, addToHistory, outExecutedCommand, outExecutedParameters, callFromCommandGroup, clearErrors, handleErrors);
        outCommandResult = commandResult.c_str();
        return result;
    }


    // parse and execute command
    bool CommandManager::ExecuteCommand(const char* command, AZStd::string& outCommandResult, bool addToHistory, Command** outExecutedCommand, CommandLine* outExecutedParameters, bool callFromCommandGroup, bool clearErrors, bool handleErrors)
    {
        // store the executed command
        if (outExecutedCommand)
        {
            *outExecutedCommand = nullptr;
        }

        if (outExecutedParameters)
        {
            *outExecutedParameters = CommandLine();
        }

        // build a local string object from the command
        AZStd::string commandString(command);

        // remove all spaces on the left and right
        AzFramework::StringFunc::TrimWhiteSpace(commandString, true, true);

        // check if the string actually contains text
        if (commandString.empty())
        {
            outCommandResult = "Command string is empty.";
            if (outExecutedCommand)
            {
                *outExecutedCommand = nullptr;
            }
            return false;
        }

        // find the first space after the command
        const size_t wordSeparatorIndex = commandString.find_first_of(MCore::CharacterConstants::wordSeparators);

        // get the command name
        const AZStd::string commandName = commandString.substr(0, wordSeparatorIndex);

        // find the corresponding command and execute it
        Command* commandObject = FindCommand(commandName);
        if (commandObject == nullptr)
        {
            if (outExecutedCommand)
            {
                *outExecutedCommand = nullptr;
            }
            outCommandResult = "Command has not been found, please make sure you have registered the command before using it.";
            return false;
        }

        // get the parameters
        AZStd::string commandParameters;
        if (wordSeparatorIndex != AZStd::string::npos)
        {
            commandParameters = commandString.substr(wordSeparatorIndex + 1);
        }
        AzFramework::StringFunc::TrimWhiteSpace(commandParameters, true, true);

        // show help when wanted
        if (AzFramework::StringFunc::Equal(commandParameters.c_str(), "-help", false /* no case */))
        {
            if (outExecutedCommand)
            {
                *outExecutedCommand = nullptr;
            }
            commandObject->GetSyntax().LogSyntax();
            return true;
        }

        // build the command line
        CommandLine commandLine(commandParameters);

        // check on syntax errors first
        outCommandResult.clear();
        if (commandObject->GetSyntax().CheckIfIsValid(commandLine, outCommandResult) == false)
        {
            if (outExecutedCommand)
            {
                *outExecutedCommand = nullptr;
            }
            return false;
        }

        // create and execute the command
        Command* newCommand = commandObject->Create();
        newCommand->SetCommandParameters(commandLine);
        const bool result = ExecuteCommand(newCommand, commandLine, outCommandResult, addToHistory, clearErrors, handleErrors);

        // delete the command object directly if we don't want to store it in the history
        if ((callFromCommandGroup == false && (result == false || commandObject->GetIsUndoable() == false || addToHistory == false)) ||
            (callFromCommandGroup  && (result == false || commandObject->GetIsUndoable() == false)))
        {
            delete newCommand;
            if (outExecutedCommand)
            {
                *outExecutedCommand = nullptr;
            }
            return result;
        }

        // store the executed command
        if (outExecutedCommand)
        {
            *outExecutedCommand = newCommand;
        }

        if (outExecutedParameters)
        {
            *outExecutedParameters = commandLine;
        }

        // return the command result
        return result;
    }


    // use this version when calling a command from inside a command execute or undo function
    bool CommandManager::ExecuteCommandInsideCommand(const char* command, AZStd::string& outCommandResult)
    {
        return ExecuteCommand(command, outCommandResult, false, nullptr, nullptr, false, false, false);
    }


    bool CommandManager::ExecuteCommandInsideCommand(const AZStd::string& command, AZStd::string& outCommandResult)
    {
        return ExecuteCommand(command.c_str(), outCommandResult, false, nullptr, nullptr, false, false, false);
    }

    bool CommandManager::ExecuteCommandOrAddToGroup(const AZStd::string& command, MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        if (!commandGroup)
        {
            bool commandResult;
            AZStd::string commandResultString;
            if (executeInsideCommand)
            {
                commandResult = ExecuteCommandInsideCommand(command, commandResultString);
            }
            else
            {
                commandResult = ExecuteCommand(command, commandResultString);
            }

            if (!commandResult)
            {
                AZ_Error("EMotionFX", false, commandResultString.c_str());
                return false;
            }
        }
        else
        {
            commandGroup->AddCommandString(command);
        }

        return true;
    }

    // execute a group of commands
    bool CommandManager::ExecuteCommandGroup(CommandGroup& commandGroup, AZStd::string& outCommandResult, bool addToHistory, bool clearErrors, bool handleErrors)
    {
        // if there is nothing to do
        const uint32 numCommands = commandGroup.GetNumCommands();
        if (numCommands == 0)
        {
            return true;
        }

        ++m_commandsInExecution;

        // execute command manager callbacks
        const uint32 numCallbacks = mCallbacks.GetLength();
        for (uint32 i = 0; i < numCallbacks; ++i)
        {
            mCallbacks[i]->OnPreExecuteCommandGroup(&commandGroup, false);
        }

        // clear the output result
        AZStd::vector<AZStd::string> intermediateCommandResults;
        intermediateCommandResults.resize(numCommands);

        // execute all commands inside the group
        bool hadError = false;
        CommandGroup* newGroup = commandGroup.Clone();
        AZStd::string commandString;
        AZStd::string tmpStr;

        for (uint32 i = 0; i < numCommands; ++i)
        {
            Command*        executedCommand = nullptr;
            CommandLine     executedParameters;

            // feed the last result into the next command
            commandString = commandGroup.GetCommandString(i);

            size_t lastResultIndex = commandString.find("%LASTRESULT");
            bool replaceHappen = false;
            while (lastResultIndex != AZStd::string::npos)
            {
                // Find what index the string is referring to
                // 11 is the amount of characters in "%LASTRESULT"
                const size_t rightPercentagePos = commandString.find("%", lastResultIndex + 11);
                if (rightPercentagePos == AZStd::string::npos)
                {
                    // if the right pound is not found, stop replacing, the command is ill-formed
                    MCore::LogError("Execution of command '%s' failed, right '%' delimiter was not found", commandString.c_str());
                    hadError = true;
                    break;
                }

                // Get the relative index of the command execution we want the result from
                // For example, %LASTRESULT2% means we want the result of two commands ago
                // %LASTRESULT% == %LASTRESULT1% which is the result of the last command
                int32 relativeIndex = 1;

                // 11 is the amount of characters in "%LASTRESULT"
                tmpStr = commandString.substr(lastResultIndex + 11, rightPercentagePos - 11 - lastResultIndex);
                if (!tmpStr.empty())
                {
                    // check if it can be safely converted to number
                    if (!AzFramework::StringFunc::LooksLikeInt(tmpStr.c_str(), &relativeIndex))
                    {
                        MCore::LogError("Execution of command '%s' failed, characters between '%LASTRESULT' and '%' cannot be converted to integer", commandString.c_str());
                        hadError = true;
                        break;
                    }
                    if (relativeIndex == 0)
                    {
                        MCore::LogError("Execution of command '%s' failed, command trying to access its own result", commandString.c_str());
                        hadError = true;
                        break;
                    }
                }
                if (static_cast<int32>(i) < relativeIndex)
                {
                    MCore::LogError("Execution of command '%s' failed, command trying to access results from %d commands back, but there are only %d", commandString.c_str(), relativeIndex, i - 1);
                    hadError = true;
                    break;
                }

                tmpStr = commandString.substr(lastResultIndex, rightPercentagePos - lastResultIndex + 1);
                AzFramework::StringFunc::Replace(commandString, tmpStr.c_str(), intermediateCommandResults[i - relativeIndex].c_str());
                replaceHappen = true;

                // Search again in case the command group is referring to other results
                lastResultIndex = commandString.find("%LASTRESULT");
            }
            if (replaceHappen)
            {
                commandGroup.SetCommandString(i, commandString.c_str());
            }

            // try to execute the command
            const bool result = ExecuteCommand(commandString, intermediateCommandResults[i], false, &executedCommand, &executedParameters, true, false, false);
            if (result == false)
            {
                MCore::LogError("Execution of command '%s' failed (result='%s')", commandString.c_str(), intermediateCommandResults[i].c_str());
                AddError(intermediateCommandResults[i]);
                hadError = true;
                if (commandGroup.GetContinueAfterError() == false)
                {
                    if (commandGroup.GetAddToHistoryAfterError() == false || addToHistory == false)
                    {
                        delete newGroup;

                        // execute command manager callbacks
                        for (uint32 c = 0; c < numCallbacks; ++c)
                        {
                            mCallbacks[c]->OnPostExecuteCommandGroup(&commandGroup, false);
                        }

                        // Let the callbacks handle error reporting (e.g. show an error report window).
                        if (handleErrors && !mErrors.empty())
                        {
                            // Execute error report callbacks.
                            for (uint32 c = 0; c < numCallbacks; ++c)
                            {
                                mCallbacks[c]->OnShowErrorReport(mErrors);
                            }
                        }

                        // Clear errors after reporting if specified.
                        if (clearErrors)
                        {
                            mErrors.clear();
                        }

                        --m_commandsInExecution;
                        return false;
                    }
                    else
                    {
                        for (uint32 c = i; c < numCommands; ++c)
                        {
                            newGroup->SetCommand(c, nullptr);
                        }
                        break;
                    }
                }
            }

            // store the parameter list and the command object that was used during executing
            // we will need these in order to undo the command
            newGroup->SetCommand(i, executedCommand);
            newGroup->SetParameters(i, executedParameters);
        }

        //  if we want to add the group to the history
        if (hadError == false)
        {
            if (addToHistory) // also check if all commands inside the group are undoable?
            {
                PushCommandHistory(newGroup);
            }
            else
            {
                delete newGroup; // delete the cloned command group in case we don't want to add it to the history
            }
        }
        else
        {
            if (commandGroup.GetAddToHistoryAfterError() && addToHistory)
            {
                PushCommandHistory(newGroup);
            }
            else
            {
                delete newGroup; // delete the cloned command group in case we don't want to add it to the history
            }
        }

        // execute command manager callbacks
        for (uint32 i = 0; i < numCallbacks; ++i)
        {
            mCallbacks[i]->OnPostExecuteCommandGroup(&commandGroup, true);
        }

        // Let the callbacks handle error reporting (e.g. show an error report window).
        const bool errorsOccured = !mErrors.empty();
        if (handleErrors && errorsOccured)
        {
            // Execute error report callbacks.
            for (uint32 i = 0; i < numCallbacks; ++i)
            {
                mCallbacks[i]->OnShowErrorReport(mErrors);
            }
        }

        // Return the result of the last command
        // TODO: once we convert to AZStd::string we can do a swap here
        outCommandResult = intermediateCommandResults.back();

        // Clear errors after reporting if specified.
        if (clearErrors)
        {
            mErrors.clear();
        }

        --m_commandsInExecution;

        // check if we need to let the whole command group fail in case an error occured
        if (errorsOccured && commandGroup.GetReturnFalseAfterError())
        {
            return false;
        }

        return true;
    }


    // use this version when calling a command group from inside another command
    bool CommandManager::ExecuteCommandGroupInsideCommand(CommandGroup& commandGroup, AZStd::string& outCommandResult)
    {
        return ExecuteCommandGroup(commandGroup, outCommandResult, false, false, false);
    }


    // internal helper functions to easily execute all undo callbacks for a given command
    void CommandManager::ExecuteUndoCallbacks(Command* command, const CommandLine& parameters, bool preUndo)
    {
        Command*    orgCommand  = command->GetOriginalCommand();
        uint32      numFailed   = 0;

        // get the number of callbacks and iterate through them
        const uint32 numCommandCallbacks = orgCommand->GetNumCallbacks();
        for (uint32 i = 0; i < numCommandCallbacks; ++i)
        {
            // get the current callback
            Command::Callback* callback = orgCommand->GetCallback(i);

            if (preUndo)
            {
                const uint32 numCallbacks = mCallbacks.GetLength();
                for (uint32 j = 0; j < numCallbacks; ++j)
                {
                    mCallbacks[j]->OnPreUndoCommand(command, parameters);
                }
            }

            // check if we need to execute the callback
            if (callback->GetExecutePreUndo() == preUndo)
            {
                if (callback->Undo(command, parameters) == false)
                {
                    numFailed++;
                }
            }

            if (!preUndo)
            {
                const uint32 numCallbacks = mCallbacks.GetLength();
                for (uint32 j = 0; j < numCallbacks; ++j)
                {
                    mCallbacks[j]->OnPostUndoCommand(command, parameters);
                }
            }
        }

        if (numFailed > 0)
        {
            LogWarning("%d out of %d %s-undo callbacks of command '%s' (%s) returned a failure.", numFailed, numCommandCallbacks, preUndo ? "pre" : "post", command->GetName(), command->GetHistoryName());
        }
    }


    // internal helper functions to easily execute all command callbacks for a given command
    void CommandManager::ExecuteCommandCallbacks(Command* command, const CommandLine& parameters, bool preCommand)
    {
        Command*    orgCommand  = command->GetOriginalCommand();
        uint32      numFailed   = 0;

        // get the number of callbacks and iterate through them
        const uint32 numCommandCallbacks = orgCommand->GetNumCallbacks();
        for (uint32 i = 0; i < numCommandCallbacks; ++i)
        {
            // get the current callback
            Command::Callback* callback = orgCommand->GetCallback(i);

            // check if we need to execute the callback
            if (callback->GetExecutePreCommand() == preCommand)
            {
                if (callback->Execute(command, parameters) == false)
                {
                    numFailed++;
                }
            }
        }

        if (numFailed > 0)
        {
            LogWarning("%d out of %d %s-command callbacks of command '%s' (%s) returned a failure.", numFailed, numCommandCallbacks, preCommand ? "pre" : "post", command->GetName(), command->GetHistoryName());
        }
    }


    // undo the last called command
    bool CommandManager::Undo(AZStd::string& outCommandResult)
    {
        // check if we can undo
        if (mCommandHistory.GetLength() <= 0 && mHistoryIndex >= 0)
        {
            outCommandResult = "Cannot undo command. The command history is empty";
            return false;
        }

        // get the last called command from the command history
        const CommandHistoryEntry& lastEntry = mCommandHistory[mHistoryIndex];
        Command* command = lastEntry.mExecutedCommand;

        ++m_commandsInExecution;

        // if we are dealing with a regular command
        bool result = true;
        if (command)
        {
            // execute pre-undo callbacks
            ExecuteUndoCallbacks(command, lastEntry.mParameters, true);

            // undo the command, get the result and reset it
            result = command->Undo(lastEntry.mParameters, outCommandResult);

            // execute post-undo callbacks
            ExecuteUndoCallbacks(command, lastEntry.mParameters, false);
        }
        // we are dealing with a command group
        else
        {
            CommandGroup* group = lastEntry.mCommandGroup;
            MCORE_ASSERT(group);

            // perform callbacks
            const uint32 numCallbacks = mCallbacks.GetLength();
            for (uint32 i = 0; i < numCallbacks; ++i)
            {
                mCallbacks[i]->OnPreExecuteCommandGroup(group, true);
            }

            const int32 numCommands = group->GetNumCommands() - 1;
            for (int32 g = numCommands; g >= 0; --g)
            {
                Command* groupCommand = group->GetCommand(g);
                if (groupCommand == nullptr)
                {
                    continue;
                }

                ++m_commandsInExecution;

                // execute pre-undo callbacks
                ExecuteUndoCallbacks(groupCommand, group->GetParameters(g), true);

                // undo the command, get the result and reset it
                // TODO: add option to stop execution of undo's when one fails?
                if (groupCommand->Undo(group->GetParameters(g), outCommandResult) == false)
                {
                    result = false;
                }

                // execute post-undo callbacks
                ExecuteUndoCallbacks(groupCommand, group->GetParameters(g), false);

                --m_commandsInExecution;
            }

            // perform callbacks
            for (uint32 i = 0; i < numCallbacks; ++i)
            {
                mCallbacks[i]->OnPostExecuteCommandGroup(group, result);
            }
        }

        // go one step back in the command history
        mHistoryIndex--;

        // perform callbacks
        const uint32 numCallbacks = mCallbacks.GetLength();
        for (uint32 i = 0; i < numCallbacks; ++i)
        {
            mCallbacks[i]->OnSetCurrentCommand(mHistoryIndex);
        }

        // Let the callbacks handle error reporting (e.g. show an error report window).
        if (!mErrors.empty())
        {
            for (uint32 i = 0; i < numCallbacks; ++i)
            {
                mCallbacks[i]->OnShowErrorReport(mErrors);
            }
            mErrors.clear();
        }

        --m_commandsInExecution;

        return result;
    }


    // redo the last undoed command
    bool CommandManager::Redo(AZStd::string& outCommandResult)
    {
        /*  // check if there are still commands to undo in the history
            if (mHistoryIndex >= mCommandHistory.GetLength())
            {
                outCommandResult = "Cannot redo command. Either the history is empty or the history index is out of range.";
                return false;
            }*/

        // get the last called command from the command history
        const CommandHistoryEntry& lastEntry = mCommandHistory[mHistoryIndex + 1];

        // if we just redo one single command
        bool result = true;
        if (lastEntry.mExecutedCommand)
        {
            // redo the command, get the result and reset it
            result = ExecuteCommand(lastEntry.mExecutedCommand, lastEntry.mParameters, outCommandResult, false);
        }
        else
        {
            ++m_commandsInExecution;
            CommandGroup* group = lastEntry.mCommandGroup;
            MCORE_ASSERT(group);

            // redo all commands inside the group
            const uint32 numCommands = group->GetNumCommands();
            for (uint32 g = 0; g < numCommands; ++g)
            {
                if (group->GetCommand(g))
                {
                    if (ExecuteCommand(group->GetCommand(g), group->GetParameters(g), outCommandResult, false) == false)
                    {
                        result = false;
                    }
                }
            }
            --m_commandsInExecution;
        }

        // go one step forward in the command history
        mHistoryIndex++;

        // perform callbacks
        const uint32 numCallbacks = mCallbacks.GetLength();
        for (uint32 i = 0; i < numCallbacks; ++i)
        {
            mCallbacks[i]->OnSetCurrentCommand(mHistoryIndex);
        }

        // Let the callbacks handle error reporting (e.g. show an error report window).
        if (!mErrors.empty())
        {
            for (uint32 i = 0; i < numCallbacks; ++i)
            {
                mCallbacks[i]->OnShowErrorReport(mErrors);
            }
            mErrors.clear();
        }

        return result;
    }


    // register the command in the command manager
    bool CommandManager::RegisterCommand(Command* command)
    {
        // check if the command object is valid
        if (command == nullptr)
        {
            LogError("Cannot register command. Command object is not valid.");
            return false;
        }

        // check if the string actually contains text
        if (command->GetNameString().empty())
        {
            LogError("Cannot register command. Command name is empty.");
            return false;
        }

        // check if the command is already registered
        Command* commandObject = FindCommand(command->GetNameString());
        if (commandObject)
        {
            LogError("Cannot register command. There is already a command registered as '%s'.", command->GetName());
            return false;
        }

        // add the command to the hash table
        mRegisteredCommands.insert(AZStd::make_pair<AZStd::string, Command*>(command->GetNameString(), command));

        // we're going to insert the command in a sorted way now
        bool found = false;
        const uint32 numCommands = mCommands.GetLength();
        for (uint32 i = 0; i < numCommands; ++i)
        {
            if (azstricmp(mCommands[i]->GetName(), command->GetName()) > 0)
            {
                found = true;
                mCommands.Insert(i, command);
                break;
            }
        }

        // if no insert location has been found, add it to the back of the array
        if (found == false)
        {
            mCommands.Add(command);
        }

        // initialize the command syntax
        command->InitSyntax();

        // success
        return true;
    }


    Command* CommandManager::FindCommand(const AZStd::string& commandName)
    {
        auto iterator = mRegisteredCommands.find(commandName);
        if (iterator == mRegisteredCommands.end())
        {
            return nullptr;
        }

        return iterator->second;
    }


    // execute command object and store the history entry if the command is undoable
    bool CommandManager::ExecuteCommand(Command* command, AZStd::string& outCommandResult, bool addToHistory, bool clearErrors, bool handleErrors)
    {
        return ExecuteCommand(command, CommandLine(), outCommandResult, addToHistory, clearErrors, handleErrors);
    }

    bool CommandManager::ExecuteCommand(Command* command, const CommandLine& commandLine, AZStd::string& outCommandResult, bool addToHistory, bool clearErrors, bool handleErrors)
    {
#ifdef MCORE_COMMANDMANAGER_PERFORMANCE
        Timer commandTimer;
#endif

        if (command == nullptr)
        {
            return false;
        }

#ifdef MCORE_COMMANDMANAGER_PERFORMANCE
        Timer preCallbacksTimer;
#endif
        ++m_commandsInExecution;

        // execute command manager callbacks
        const uint32 numCallbacks = mCallbacks.GetLength();
        for (uint32 i = 0; i < numCallbacks; ++i)
        {
            mCallbacks[i]->OnPreExecuteCommand(nullptr, command, commandLine);
        }

        // execute pre-command callbacks
        ExecuteCommandCallbacks(command, commandLine, true);

#ifdef MCORE_COMMANDMANAGER_PERFORMANCE
        LogInfo("   + PreExecuteCallbacks(%i): %.2f ms.", command->GetOriginalCommand()->CalcNumPreCommandCallbacks(), preCallbacksTimer.GetTime() * 1000);
        Timer executeTimer;
#endif

        // execute the command object, get the result and reset it
        outCommandResult.clear();
        const bool result = command->Execute(commandLine, outCommandResult);

#ifdef MCORE_COMMANDMANAGER_PERFORMANCE
        LogInfo("   + Execution: %.2f ms.", executeTimer.GetTime() * 1000);
        Timer postCallbacksTimer;
#endif

        // if it was successful, execute all the post-command callbacks
        if (result)
        {
            ExecuteCommandCallbacks(command, commandLine, false);
        }

        // save command in the command history if it is undoable
        if (addToHistory && command->GetIsUndoable() && result)
        {
            PushCommandHistory(command, commandLine);
        }

        // execute all post execute callbacks
        for (uint32 i = 0; i < numCallbacks; ++i)
        {
            mCallbacks[i]->OnPostExecuteCommand(nullptr, command, commandLine, result, outCommandResult);
        }

        // Let the callbacks handle error reporting (e.g. show an error report window).
        if (handleErrors && !mErrors.empty())
        {
            // Execute error report callbacks.
            for (uint32 i = 0; i < numCallbacks; ++i)
            {
                mCallbacks[i]->OnShowErrorReport(mErrors);
            }
        }

        // Clear errors after reporting if specified.
        if (clearErrors)
        {
            mErrors.clear();
        }

#ifdef MCORE_COMMANDMANAGER_PERFORMANCE
        LogInfo("   + PostExecuteCallbacks(%i): %.2f ms.", command->GetOriginalCommand()->CalcNumPostCommandCallbacks(), postCallbacksTimer.GetTime() * 1000);
        LogInfo("   + Total time: %.2f ms.", commandTimer.GetTime() * 1000);
#endif
        --m_commandsInExecution;

        return result;
    }


    // log the command history to the console
    void CommandManager::LogCommandHistory()
    {
        LogDetailedInfo("----------------------------------");

        // get the number of entries in the command history
        const uint32 numHistoryEntries = mCommandHistory.GetLength();
        LogDetailedInfo("Command History (%i entries) - oldest (top entry) to newest (bottom entry):", numHistoryEntries);

        // print the command history entries
        for (uint32 i = 0; i < numHistoryEntries; ++i)
        {
            AZStd::string text = AZStd::string::format("%.3i: name='%s', num parameters=%i", i, mCommandHistory[i].mExecutedCommand->GetName(), mCommandHistory[i].mParameters.GetNumParameters());
            if (i == (uint32)mHistoryIndex)
            {
                LogDetailedInfo("-> %s", text.c_str());
            }
            else
            {
                LogDetailedInfo(text.c_str());
            }
        }

        LogDetailedInfo("----------------------------------");
    }


    // remove all callbacks
    void CommandManager::RemoveCallbacks()
    {
        const uint32 numCallbacks = mCallbacks.GetLength();
        for (uint32 i = 0; i < numCallbacks; ++i)
        {
            delete mCallbacks[i];
        }

        mCallbacks.Clear(true);
    }


    // register a new callback
    void CommandManager::RegisterCallback(CommandManagerCallback* callback)
    {
        mCallbacks.Add(callback);
    }


    // remove a given callback
    void CommandManager::RemoveCallback(CommandManagerCallback* callback, bool delFromMem)
    {
        mCallbacks.RemoveByValue(callback);

        if (delFromMem)
        {
            delete callback;
        }
    }


    // get the number of callbacks
    uint32 CommandManager::GetNumCallbacks() const
    {
        return mCallbacks.GetLength();
    }


    // get a given callback
    CommandManagerCallback* CommandManager::GetCallback(uint32 index)
    {
        return mCallbacks[index];
    }


    // set the max num history items
    void CommandManager::SetMaxHistoryItems(uint32 maxItems)
    {
        maxItems = AZStd::max(1u, maxItems);
        mMaxHistoryEntries = maxItems;

        while (mCommandHistory.GetLength() > mMaxHistoryEntries)
        {
            PopCommandHistory();
            mHistoryIndex = mCommandHistory.GetLength() - 1;
        }
    }


    // get the number of max history items
    uint32 CommandManager::GetMaxHistoryItems() const
    {
        return mMaxHistoryEntries;
    }


    // get the history index
    int32 CommandManager::GetHistoryIndex() const
    {
        return mHistoryIndex;
    }


    // get the number of history items
    uint32 CommandManager::GetNumHistoryItems() const
    {
        return mCommandHistory.GetLength();
    }


    const CommandManager::CommandHistoryEntry& CommandManager::GetHistoryItem(uint32 index) const
    {
        return mCommandHistory[index];
    }


    // get the history command
    Command* CommandManager::GetHistoryCommand(uint32 historyIndex)
    {
        return mCommandHistory[historyIndex].mExecutedCommand;
    }


    // clear the history
    void CommandManager::ClearHistory()
    {
        // clear the command history
        while (mCommandHistory.GetIsEmpty() == false)
        {
            PopCommandHistory();
        }

        // reset the history index
        mHistoryIndex = -1;
    }


    // get the history commandline
    const CommandLine& CommandManager::GetHistoryCommandLine(uint32 historyIndex) const
    {
        return mCommandHistory[historyIndex].mParameters;
    }


    // get the number of registered commands
    uint32 CommandManager::GetNumRegisteredCommands() const
    {
        return mCommands.GetLength();
    }


    // get a given command
    Command* CommandManager::GetCommand(uint32 index)
    {
        return mCommands[index];
    }


    // delete the given callback from all commands
    void CommandManager::RemoveCommandCallback(Command::Callback* callback, bool delFromMem)
    {
        // for all commands
        const uint32 numCommands = mCommands.GetLength();
        for (uint32 i = 0; i < numCommands; ++i)
        {
            mCommands[i]->RemoveCallback(callback, false); // false = don't delete from memory
        }
        if (delFromMem)
        {
            delete callback;
        }
    }


    // delete the callback from a given command
    void CommandManager::RemoveCommandCallback(const char* commandName, Command::Callback* callback, bool delFromMem)
    {
        // try to find the command
        Command* command = FindCommand(commandName);
        if (command)
        {
            command->RemoveCallback(callback, false); // false = don't delete from memory
        }
        if (delFromMem)
        {
            delete callback;
        }
    }


    // register a given callback to a command
    bool CommandManager::RegisterCommandCallback(const char* commandName, Command::Callback* callback)
    {
        // try to find the command
        Command* command = FindCommand(commandName);
        if (command == nullptr)
        {
            return false;
        }

        // if this command hasn't already registered this callback, add it
        if (command->CheckIfHasCallback(callback) == false)
        {
            command->AddCallback(callback);
            return true;
        }

        // command already has been registered
        return false;
    }


    // check if an error occurred and calls the error handling callbacks
    bool CommandManager::ShowErrorReport()
    {
        // Let the callbacks handle error reporting (e.g. show an error report window).
        const bool errorsOccured = !mErrors.empty();
        if (errorsOccured)
        {
            // Execute error report callbacks.
            const uint32 numCallbacks = mCallbacks.GetLength();
            for (uint32 i = 0; i < numCallbacks; ++i)
            {
                mCallbacks[i]->OnShowErrorReport(mErrors);
            }
        }

        // clear errors after reporting
        mErrors.clear();

        return errorsOccured;
    }
} // namespace MCore
