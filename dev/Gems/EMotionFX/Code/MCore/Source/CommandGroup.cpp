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

// include the required headers
#include "CommandGroup.h"


namespace MCore
{
    // default constructor
    CommandGroup::CommandGroup()
    {
        mCommands.SetMemoryCategory(MCore::MCORE_MEMCATEGORY_COMMANDSYSTEM);
        ReserveCommands(5);
        SetContinueAfterError(true);
        SetReturnFalseAfterError(false);
    }


    // extended constructor
    CommandGroup::CommandGroup(const AZStd::string& groupName, uint32 numCommandsToReserve)
    {
        mCommands.SetMemoryCategory(MCore::MCORE_MEMCATEGORY_COMMANDSYSTEM);
        SetGroupName(groupName);
        ReserveCommands(numCommandsToReserve);
        SetContinueAfterError(true);
        SetAddToHistoryAfterError(true);
    }


    // the destructor
    CommandGroup::~CommandGroup()
    {
        RemoveAllCommands();
    }


    // reserve memory
    void CommandGroup::ReserveCommands(uint32 numToReserve)
    {
        if (numToReserve > 0)
        {
            mCommands.Reserve(numToReserve);
        }
    }


    // add a command string to the group
    void CommandGroup::AddCommandString(const char* commandString)
    {
        mCommands.AddEmpty();
        mCommands.GetLast().mCommandString = commandString;
    }


    // add a command string to the group
    void CommandGroup::AddCommandString(const AZStd::string& commandString)
    {
        mCommands.AddEmpty();
        mCommands.GetLast().mCommandString = commandString;
    }


    // get the command string for a given command
    const char* CommandGroup::GetCommandString(uint32 index) const
    {
        return mCommands[index].mCommandString.c_str();
    }


    // get the command string for a given command
    const AZStd::string& CommandGroup::GetCommandStringAsString(uint32 index)   const
    {
        return mCommands[index].mCommandString;
    }


    // get a given command
    Command* CommandGroup::GetCommand(uint32 index)
    {
        return mCommands[index].mCommand;
    }


    // get the parameter list for a given command
    const CommandLine& CommandGroup::GetParameters(uint32 index) const
    {
        return mCommands[index].mCommandLine;
    }


    // get the name of the group
    const char* CommandGroup::GetGroupName() const
    {
        return mGroupName.c_str();
    }


    // get the name of the group
    const AZStd::string& CommandGroup::GetGroupNameString() const
    {
        return mGroupName;
    }


    // set the name of the group
    void CommandGroup::SetGroupName(const char* groupName)
    {
        mGroupName = groupName;
    }


    // set the name of the group
    void CommandGroup::SetGroupName(const AZStd::string& groupName)
    {
        mGroupName = groupName;
    }


    // set the command string for a given command
    void CommandGroup::SetCommandString(uint32 index, const char* commandString)
    {
        mCommands[index].mCommandString = commandString;
    }


    // set the parameters for a given command
    void CommandGroup::SetParameters(uint32 index, const CommandLine& params)
    {
        mCommands[index].mCommandLine = params;
    }


    // set the command pointer for a given command
    void CommandGroup::SetCommand(uint32 index, Command* command)
    {
        mCommands[index].mCommand = command;
    }


    // get the number of commands
    uint32 CommandGroup::GetNumCommands() const
    {
        return mCommands.GetLength();
    }


    // remove all commands from the group
    void CommandGroup::RemoveAllCommands(bool delFromMem)
    {
        if (delFromMem)
        {
            const uint32 numCommands = mCommands.GetLength();
            for (uint32 i = 0; i < numCommands; ++i)
            {
                delete mCommands[i].mCommand;
            }
        }

        mCommands.Clear();
    }


    // clone the command group
    CommandGroup* CommandGroup::Clone() const
    {
        CommandGroup* newGroup          = new CommandGroup(mGroupName, 0);
        newGroup->mCommands             = mCommands;
        newGroup->mHistoryAfterError    = mHistoryAfterError;
        newGroup->mContinueAfterError   = mContinueAfterError;
        newGroup->mReturnFalseAfterError = mReturnFalseAfterError;
        return newGroup;
    }


    // continue execution of the remaining commands after one fails to execute?
    void CommandGroup::SetContinueAfterError(bool continueAfter)
    {
        mContinueAfterError = continueAfter;
    }


    // add group to the history even when one internal command failed to execute?
    void CommandGroup::SetAddToHistoryAfterError(bool addAfterError)
    {
        mHistoryAfterError = addAfterError;
    }


    // check to see if we continue executing internal commands even if one failed
    bool CommandGroup::GetContinueAfterError() const
    {
        return mContinueAfterError;
    }


    // check if we add this group to the history, even if one internal command failed
    bool CommandGroup::GetAddToHistoryAfterError() const
    {
        return mHistoryAfterError;
    }


    // set if the command group shall return false after an error occured or not
    void CommandGroup::SetReturnFalseAfterError(bool returnAfterError)
    {
        mReturnFalseAfterError = returnAfterError;
    }


    // returns true in case the group returns false when executing it
    bool CommandGroup::GetReturnFalseAfterError() const
    {
        return mReturnFalseAfterError;
    }
} // namespace MCore
