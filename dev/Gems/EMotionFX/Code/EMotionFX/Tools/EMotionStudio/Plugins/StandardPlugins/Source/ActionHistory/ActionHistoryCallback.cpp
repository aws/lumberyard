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

// include required headers
#include "ActionHistoryCallback.h"
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/EventManager.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include <QListWidget>
#include <QTextEdit>
#include <QApplication>
#include <QHBoxLayout>

namespace EMStudio
{
    ActionHistoryCallback::ActionHistoryCallback(QListWidget* list)
        : MCore::CommandManagerCallback()
    {
        mList               = list;
        mIndex              = 0;
        mIsRemoving         = false;
        mGroupExecuting     = false;
        mExecutedGroup      = nullptr;
        mErrorWindow        = nullptr;
        mNumGroupCommands   = 0;
        mCurrentCommandIndex = 0;
    }


    ActionHistoryCallback::~ActionHistoryCallback()
    {
        delete mErrorWindow;
    }


    // Before executing a command.
    void ActionHistoryCallback::OnPreExecuteCommand(MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(group);

        if (command)
        {
            if (MCore::GetLogManager().GetLogLevels() & MCore::LogCallback::LOGLEVEL_DEBUG)
            {
                mTempString = command->GetName();
                const uint32 numParameters = commandLine.GetNumParameters();
                for (uint32 i = 0; i < numParameters; ++i)
                {
                    mTempString += " -";
                    mTempString += commandLine.GetParameterName(i);
                    mTempString += " ";
                    mTempString += commandLine.GetParameterValue(i);
                }
                MCore::LogDebugMsg(mTempString.c_str());
            }
        }
    }


    // After executing a command.
    void ActionHistoryCallback::OnPostExecuteCommand(MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine, bool wasSuccess, const AZStd::string& outResult)
    {
        MCORE_UNUSED(group);
        MCORE_UNUSED(commandLine);
        MCORE_UNUSED(outResult);
        if (mGroupExecuting && mExecutedGroup)
        {
            mCurrentCommandIndex++;
            if (mCurrentCommandIndex % 32 == 0)
            {
                EMotionFX::GetEventManager().OnProgressValue(((float)mCurrentCommandIndex / (mNumGroupCommands + 1)) * 100.0f);
            }
        }

        if (command && MCore::GetLogManager().GetLogLevels() & MCore::LogCallback::LOGLEVEL_DEBUG)
        {
            mTempString = AZStd::string::format("%sExecution of command '%s' %s", wasSuccess ?  "    " : "*** ", command->GetName(), wasSuccess ? "completed successfully" : " FAILED");
            MCore::LogDebugMsg(mTempString.c_str()); 
        }
    }


    // Before executing a command group.
    void ActionHistoryCallback::OnPreExecuteCommandGroup(MCore::CommandGroup* group, bool undo)
    {
        if (!mGroupExecuting && group->GetNumCommands() > 64)
        {
            mGroupExecuting     = true;
            mExecutedGroup      = group;
            mCurrentCommandIndex = 0;
            mNumGroupCommands   = group->GetNumCommands();

            GetManager()->SetAvoidRendering(true);

            EMotionFX::GetEventManager().OnProgressStart();

            mTempString = AZStd::string::format("%s%s", undo ? "Undo: " : "", group->GetGroupName());
            EMotionFX::GetEventManager().OnProgressText(mTempString.c_str());
        }

        if (group && MCore::GetLogManager().GetLogLevels() & MCore::LogCallback::LOGLEVEL_DEBUG)
        {
            mTempString = AZStd::string::format("Starting %s of command group '%s'", undo ? "undo" : "execution", group->GetGroupName());
            MCore::LogDebugMsg(mTempString.c_str());
        }
    }


    // After executing a command group.
    void ActionHistoryCallback::OnPostExecuteCommandGroup(MCore::CommandGroup* group, bool wasSuccess)
    {
        if (mExecutedGroup == group)
        {
            EMotionFX::GetEventManager().OnProgressEnd();

            mGroupExecuting      = false;
            mExecutedGroup       = nullptr;
            mNumGroupCommands    = 0;
            mCurrentCommandIndex = 0;

            GetManager()->SetAvoidRendering(false);
        }

        if (group && MCore::GetLogManager().GetLogLevels() & MCore::LogCallback::LOGLEVEL_DEBUG)
        {
            mTempString = AZStd::string::format("%sExecution of command group '%s' %s", wasSuccess ?  "    " : "*** ", group->GetGroupName(), wasSuccess ? "completed successfully" : " FAILED");
            MCore::LogDebugMsg(mTempString.c_str());
        }
    }


    // Add a new item to the history.
    void ActionHistoryCallback::OnAddCommandToHistory(uint32 historyIndex, MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);
        mTempString = MCore::CommandManager::CommandHistoryEntry::ToString(group, command, mIndex++).c_str();

        mList->insertItem(historyIndex, new QListWidgetItem(mTempString.c_str(), mList));
        mList->setCurrentRow(historyIndex);
    }


    // Remove an item from the history.
    void ActionHistoryCallback::OnRemoveCommand(uint32 historyIndex)
    {
        // Remove the item.
        mIsRemoving = true;
        delete mList->takeItem(historyIndex);
        mIsRemoving = false;

        // Disable the undo/redo if the list is empty.
        if (mList->count() == 0)
        {
            GetMainWindow()->DisableUndoRedo();
        }
    }


    // Set the current command.
    void ActionHistoryCallback::OnSetCurrentCommand(uint32 index)
    {
        if (mIsRemoving)
        {
            return;
        }

        if (index == MCORE_INVALIDINDEX32)
        {
            mList->setCurrentRow(-1);

            // Darken all history items.
            const uint32 numCommands = GetCommandManager()->GetNumHistoryItems();
            for (uint32 i = 0; i < numCommands; ++i)
            {
                mList->item(i)->setForeground(QBrush(QColor(110, 110, 110))); // TODO: use style sheet color
            }
            return;
        }

        // get the list of selected items
        mList->setCurrentRow(index);

        // Get the current history index.
        const uint32 historyIndex = GetCommandManager()->GetHistoryIndex();
        if (historyIndex == MCORE_INVALIDINDEX32)
        {
            AZStd::string outResult;
            const uint32 numRedos = index + 1;
            for (uint32 i = 0; i < numRedos; ++i)
            {
                outResult.clear();
                const bool result = GetCommandManager()->Redo(outResult);
                if (outResult.size() > 0)
                {
                    if (!result)
                    {
                        MCore::LogError(outResult.c_str());
                    }
                }
            }
        }
        else if (historyIndex > index) // if we need to perform undo's
        {
            AZStd::string outResult;
            const int32 numUndos = historyIndex - index;
            for (int32 i = 0; i < numUndos; ++i)
            {
                // try to undo
                outResult.clear();
                const bool result = GetCommandManager()->Undo(outResult);
                if (outResult.size() > 0)
                {
                    if (!result)
                    {
                        MCore::LogError(outResult.c_str());
                    }
                }
            }
        }
        else if (historyIndex < index) // if we need to redo commands
        {
            AZStd::string outResult;
            const int32 numRedos = index - historyIndex;
            for (int32 i = 0; i < numRedos; ++i)
            {
                outResult.clear();
                const bool result = GetCommandManager()->Redo(outResult);
                if (outResult.size() > 0)
                {
                    if (!result)
                    {
                        MCore::LogError(outResult.c_str());
                    }
                }
            }
        }

        // Darken disabled commands.
        const uint32 orgIndex = index;
        if (index == MCORE_INVALIDINDEX32)
        {
            index = 0;
        }

        const uint32 numCommands = GetCommandManager()->GetNumHistoryItems();
        for (uint32 i = index; i < numCommands; ++i)
        {
            mList->item(i)->setForeground(QBrush(QColor(110, 110, 110))); // TODO: use style sheet color
        }

        // Color enabled ones.
        if (orgIndex != MCORE_INVALIDINDEX32)
        {
            for (uint32 i = 0; i <= index; ++i)
            {
                mList->item(index)->setForeground(QBrush(QColor(200, 200, 200))); // TODO: use style sheet color
            }
        }

        // Update the undo/redo menu items.
        GetMainWindow()->UpdateUndoRedo();
    }


    // Called when the errors shall be shown.
    void ActionHistoryCallback::OnShowErrorReport(const AZStd::vector<AZStd::string>& errors)
    {
        if (!mErrorWindow)
        {
            mErrorWindow = new ErrorWindow(GetMainWindow());
        }

        EMStudio::GetApp()->restoreOverrideCursor();
        mErrorWindow->Init(errors);
        mErrorWindow->exec();
    }


    ErrorWindow::ErrorWindow(QWidget* parent)
        : QDialog(parent)
    {
        QHBoxLayout* mainLayout = new QHBoxLayout();
        mainLayout->setMargin(0);

        mTextEdit = new QTextEdit();
        mTextEdit->setTextInteractionFlags(Qt::NoTextInteraction | Qt::TextSelectableByMouse);
        mainLayout->addWidget(mTextEdit);

        setMinimumWidth(600);
        setMinimumHeight(400);
        setLayout(mainLayout);

        setStyleSheet("background-color: rgb(30,30,30);");
    }


    ErrorWindow::~ErrorWindow()
    {
    }


    void ErrorWindow::Init(const AZStd::vector<AZStd::string>& errors)
    {
        const size_t numErrors = errors.size();

        // Update title of the dialog.
        AZStd::string windowTitle;
        if (numErrors > 1)
        {
            windowTitle = AZStd::string::format("%i errors occurred", numErrors);
        }
        else
        {
            windowTitle = AZStd::string::format("%i error occurred", numErrors);
        }

        setWindowTitle(windowTitle.c_str());

        // Iterate over the errors and construct the rich text string.
        AZStd::string text;
        for (size_t i = 0; i < numErrors; ++i)
        {
            text += AZStd::string::format("<font color=\"#F49C1C\"><b>#%i</b>:</font> ", i + 1);
            text += "<font color=\"#CBCBCB\">";
            text += errors[i];
            text += "</font>";
            text += "<br><br>";
        }

        mTextEdit->setText(text.c_str());
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/ActionHistory/ActionHistoryCallback.moc>