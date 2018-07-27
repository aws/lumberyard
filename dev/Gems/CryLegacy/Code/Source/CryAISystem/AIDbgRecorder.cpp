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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Simple text AI debugging event recorder

// Notes       : Don't include the header in CAISystem - below it is redundant



#include "CryLegacy_precompiled.h"
#include "AIDbgRecorder.h"
#include <AzFramework/IO/FileOperations.h>

#ifdef CRYAISYSTEM_DEBUG

#define AIRECORDER_FILENAME "AILog.log"
#define AIRECORDER_SECONDARYFILENAME    "AISignals.csv"

#define BUFFER_SIZE 256

//
//----------------------------------------------------------------------------------------------
bool CAIDbgRecorder::IsRecording(const IAIObject* pTarget, IAIRecordable::e_AIDbgEvent event) const
{
    if (!m_sFile.empty())
    {
        return false;
    }

    if (event == IAIRecordable::E_RESET)
    {
        return true;
    }

    if (!pTarget)
    {
        return false;
    }

    return !strcmp(gAIEnv.CVars.StatsTarget, "all")
           || !strcmp(gAIEnv.CVars.StatsTarget, pTarget->GetName());
}

//
//----------------------------------------------------------------------------------------------
void CAIDbgRecorder::Record(const IAIObject* pTarget, IAIRecordable::e_AIDbgEvent event, const char* pString) const
{
    if (event == IAIRecordable::E_RESET)
    {
        LogString("\n\n--------------------------------------------------------------------------------------------\n");
        LogString("<RESETTING AI SYSTEM>\n");
        LogString("aiTick:startTime:<entity> ai_event    details\n");
        LogString("--------------------------------------------------------------------------------------------\n");
        // (MATT) Since some gAIEnv settings change only on a reset, this would be the time to list them {2008/11/20}
        return;
    }
#ifdef AI_LOG_SIGNALS
    else if (event == IAIRecordable::E_SIGNALEXECUTEDWARNING)
    {
        char bufferString[BUFFER_SIZE];
        sprintf_s(bufferString, "%s\n", pString);
        LogStringSecondary(bufferString);
        return;
    }
#endif

    if (!pTarget)
    {
        return;
    }

    // Filter to only log the targets we are interested in
    // I.e. if not "all" and if not our current target, return
    const char* sStatsTarget = gAIEnv.CVars.StatsTarget;
    if ((strcmp(sStatsTarget, "all") != 0) && (strcmp(sStatsTarget, pTarget->GetName()) != 0))
    {
        return;
    }


    const char* pEventString("");
    switch (event)
    {
    case    IAIRecordable::E_SIGNALRECIEVED:
        pEventString = "signal_recieved      ";
        break;
    case    IAIRecordable::E_SIGNALRECIEVEDAUX:
        pEventString = "auxsignal_recieved   ";
        break;
    case    IAIRecordable::E_SIGNALEXECUTING:
        pEventString = "signal_executing     ";
        break;
    case    IAIRecordable::E_GOALPIPEINSERTED:
        pEventString = "goalpipe_inserted    ";
        break;
    case    IAIRecordable::E_GOALPIPESELECTED:
        pEventString = "goalpipe_selected    ";
        break;
    case    IAIRecordable::E_GOALPIPERESETED:
        pEventString = "goalpipe_reseted     ";
        break;
    case    IAIRecordable::E_BEHAVIORSELECTED:
        pEventString = "behaviour_selected   ";
        break;
    case    IAIRecordable::E_BEHAVIORDESTRUCTOR:
        pEventString = "behaviour_destructor ";
        break;
    case    IAIRecordable::E_BEHAVIORCONSTRUCTOR:
        pEventString = "behaviour_constructor";
        break;
    case    IAIRecordable::E_ATTENTIONTARGET:
        pEventString = "atttarget_change     ";
        break;
    case    IAIRecordable::E_REGISTERSTIMULUS:
        pEventString = "register_stimulus    ";
        break;
    case    IAIRecordable::E_HANDLERNEVENT:
        pEventString = "handler_event        ";
        break;
    case    IAIRecordable::E_ACTIONSTART:
        pEventString = "action_start         ";
        break;
    case    IAIRecordable::E_ACTIONSUSPEND:
        pEventString = "action_suspend       ";
        break;
    case    IAIRecordable::E_ACTIONRESUME:
        pEventString = "action_resume        ";
        break;
    case    IAIRecordable::E_ACTIONEND:
        pEventString = "action_end           ";
        break;
    case    IAIRecordable::E_REFPOINTPOS:
        pEventString = "refpoint_pos         ";
        break;
    case    IAIRecordable::E_EVENT:
        pEventString = "triggering event     ";
        break;
    case    IAIRecordable::E_LUACOMMENT:
        pEventString = "lua comment          ";
        break;
    default:
        pEventString = "undefined            ";
        break;
    }

    // Fetch the current AI tick and the time that tick started
    int frame = GetAISystem()->GetAITickCount();
    float time = GetAISystem()->GetFrameStartTimeSeconds();

    char bufferString[BUFFER_SIZE];

    if (!pString)
    {
        pString = "<null>";
    }
    sprintf_s(bufferString, "%6d:%9.3f: <%s> %s\t\t\t%s\n", frame, time, pTarget->GetName(), pEventString, pString);
    LogString(bufferString);
}


//
//----------------------------------------------------------------------------------------------
void CAIDbgRecorder::InitFile() const
{
    // Set the string
    m_sFile = "@log@/";
    m_sFile += AIRECORDER_FILENAME;

    // Open to wipe and write any preamble
    AZ::IO::HandleType fileHandle = fxopen(m_sFile.c_str(), "wt");
    if (fileHandle == AZ::IO::InvalidHandle)
    {
        return;
    }
    AZ::IO::FPutS("CryAISystem text debugging log\n", fileHandle);

    // (MATT) This should help in making clear which log this is
    // localtime appears to be standard - but elsewhere WIN32 is specified - to test on other platforms {2008/11/20}
#ifdef WIN32
    // Get time.
    time_t ltime;
    time(&ltime);
    tm today;
    localtime_s(&today, &ltime);

    char s[1024];
    strftime(s, 128, "Date(%d %b %Y) Time(%H %M %S)\n\n", &today);
    AZ::IO::FPutS(s, fileHandle);
#endif

    gEnv->pFileIO->Close(fileHandle);
}

//
//----------------------------------------------------------------------------------------------
void CAIDbgRecorder::InitFileSecondary() const
{
    m_sFileSecondary = "@root@/";
    m_sFileSecondary += AIRECORDER_SECONDARYFILENAME;

    AZ::IO::HandleType fileHandleSecondary = fxopen(m_sFileSecondary.c_str(), "wt");
    if (fileHandleSecondary == AZ::IO::InvalidHandle)
    {
        return;
    }
    AZ::IO::FPutS("Function,Time,Page faults,Entity\n", fileHandleSecondary);
    gEnv->pFileIO->Close(fileHandleSecondary);
}

//----------------------------------------------------------------------------------------------
void    CAIDbgRecorder::LogString(const char* pString) const
{
    bool mergeWithLog(gAIEnv.CVars.RecordLog != 0);
    if (!mergeWithLog)
    {
        if (m_sFile.empty())
        {
            InitFile();
        }

        AZ::IO::HandleType fileHandle = fxopen(m_sFile.c_str(), "at");
        if (fileHandle == AZ::IO::InvalidHandle)
        {
            return;
        }

        AZ::IO::FPutS(pString, fileHandle);
        gEnv->pFileIO->Close(fileHandle);
    }
    else
    {
        GetAISystem()->LogEvent("<AIrec>", pString);
    }
}

//
//----------------------------------------------------------------------------------------------
void    CAIDbgRecorder::LogStringSecondary(const char* pString) const
{
    if (m_sFile.empty())
    {
        InitFileSecondary();
    }

    AZ::IO::HandleType fileHandleSecondary = fxopen(m_sFileSecondary.c_str(), "at");
    if (fileHandleSecondary == AZ::IO::InvalidHandle)
    {
        return;
    }

    AZ::IO::FPutS(pString, fileHandleSecondary);
    gEnv->pFileIO->Close(fileHandleSecondary);
}

#endif //CRYAISYSTEM_DEBUG
