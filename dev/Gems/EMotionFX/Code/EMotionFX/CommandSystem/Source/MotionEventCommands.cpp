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
#include "MotionEventCommands.h"
#include "CommandManager.h"
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/MotionManager.h>
#include <MCore/Source/Compare.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/MotionEventTable.h>


namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CommandCreateMotionEventTrack
    //--------------------------------------------------------------------------------

    // execute
    bool CommandClearMotionEvents::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion id and find the corresponding object
        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot create motion event track. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        // get the motion event table as well as the event track name
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();

        // remove all motion event tracks including all motion events
        eventTable->RemoveAllTracks();

        // set the dirty flag
        AZStd::string commandString;
        commandString = AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", motionID);
        return GetCommandManager()->ExecuteCommandInsideCommand(commandString.c_str(), outResult);
    }


    // undo the command
    bool CommandClearMotionEvents::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // init the syntax of the command
    void CommandClearMotionEvents::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("motionID", "The id of the motion.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    // get the description
    const char* CommandClearMotionEvents::GetDescription() const
    {
        return "Removes all the motion event tracks including all motion events for the given motion.";
    }


    //--------------------------------------------------------------------------------
    // CommandCreateMotionEventTrack
    //--------------------------------------------------------------------------------

    // constructor
    CommandCreateMotionEventTrack::CommandCreateMotionEventTrack(MCore::Command* orgCommand)
        : MCore::Command("CreateMotionEventTrack", orgCommand)
    {
    }


    // destructor
    CommandCreateMotionEventTrack::~CommandCreateMotionEventTrack()
    {
    }


    // execute
    bool CommandCreateMotionEventTrack::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion id and find the corresponding object
        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        // find the motion
        EMotionFX::Motion*  motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot create motion event track. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        // get the motion event table as well as the event track name
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();

        // get the event track name
        AZStd::string valueString;
        parameters.GetValue("eventTrackName", this, &valueString);

        // check if the sync track is already there, if not create it
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(valueString.c_str());
        if (eventTrack == nullptr)
        {
            eventTrack = EMotionFX::MotionEventTrack::Create(valueString.c_str(), motion);
        }

        const uint32    eventTrackIndex = static_cast<uint32>(parameters.GetValueAsInt("index", this));
        const bool      isEnabled       = parameters.GetValueAsBool("enabled", this);

        // add the motion event track
        if (eventTrackIndex == MCORE_INVALIDINDEX32)
        {
            eventTable->AddTrack(eventTrack);
        }
        else
        {
            eventTable->InsertTrack(eventTrackIndex, eventTrack);
        }

        // set the enable flag
        eventTrack->SetIsEnabled(isEnabled);

        // make sure there is a sync track
        motion->GetEventTable()->AutoCreateSyncTrack(motion);

        // set the dirty flag
        valueString = AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", motionID);
        return GetCommandManager()->ExecuteCommandInsideCommand(valueString.c_str(), outResult);
    }


    // undo the command
    bool CommandCreateMotionEventTrack::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string eventTrackName;
        parameters.GetValue("eventTrackName", this, &eventTrackName);

        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        AZStd::string command;
        command = AZStd::string::format("RemoveMotionEventTrack -motionID %i -eventTrackName \"%s\"", motionID, eventTrackName.c_str());
        return GetCommandManager()->ExecuteCommandInsideCommand(command.c_str(), outResult);
    }


    // init the syntax of the command
    void CommandCreateMotionEventTrack::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        GetSyntax().AddRequiredParameter("motionID",         "The id of the motion.",                                        MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("eventTrackName",   "The name of the motion event track.",                          MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("index",            "The index of the event track in the event table.",             MCore::CommandSyntax::PARAMTYPE_INT,        "-1");
        GetSyntax().AddParameter("enabled",          "Flag which indicates if the event track is enabled or not.",   MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "true");
    }


    // get the description
    const char* CommandCreateMotionEventTrack::GetDescription() const
    {
        return "Create a motion event track with the given name for the given motion.";
    }

    //--------------------------------------------------------------------------------
    // CommandRemoveMotionEventTrack
    //--------------------------------------------------------------------------------

    // constructor
    CommandRemoveMotionEventTrack::CommandRemoveMotionEventTrack(MCore::Command* orgCommand)
        : MCore::Command("RemoveMotionEventTrack", orgCommand)
    {
        mOldTrackIndex = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandRemoveMotionEventTrack::~CommandRemoveMotionEventTrack()
    {
    }


    // execute
    bool CommandRemoveMotionEventTrack::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion id and find the corresponding object
        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove motion event track. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        // get the motion event table as well as the event track name
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();

        AZStd::string valueString;
        parameters.GetValue("eventTrackName", this, &valueString);

        // check if the motion event track is valid and return failure in case it is not
        const uint32 eventTrackIndex = eventTable->FindTrackIndexByName(valueString.c_str());
        if (eventTrackIndex == MCORE_INVALIDINDEX32)
        {
            outResult = AZStd::string::format("Cannot remove motion event track. Motion event track '%s' does not exist for motion with id='%i'.", valueString.c_str(), motionID);
            return false;
        }

        // store information for undo
        mOldTrackIndex  = eventTrackIndex;
        mOldEnabled     = eventTable->GetTrack(eventTrackIndex)->GetIsEnabled();

        // remove the motion event track
        eventTable->RemoveTrack(eventTrackIndex);

        // set the dirty flag
        valueString = AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", motionID);
        return GetCommandManager()->ExecuteCommandInsideCommand(valueString.c_str(), outResult);
    }


    // undo the command
    bool CommandRemoveMotionEventTrack::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string eventTrackName;
        parameters.GetValue("eventTrackName", this, &eventTrackName);

        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        AZStd::string command;
        command = AZStd::string::format("CreateMotionEventTrack -motionID %i -eventTrackName \"%s\" -index %d -enabled %d", motionID, eventTrackName.c_str(), mOldTrackIndex, mOldEnabled);
        return GetCommandManager()->ExecuteCommandInsideCommand(command.c_str(), outResult);
    }


    // init the syntax of the command
    void CommandRemoveMotionEventTrack::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("motionID",        "The id of the motion.",                MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("eventTrackName",  "The name of the motion event track.",  MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    // get the description
    const char* CommandRemoveMotionEventTrack::GetDescription() const
    {
        return "Remove a motion event track from the given motion.";
    }


    //--------------------------------------------------------------------------------
    // CommandAdjustMotionEventTrack
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustMotionEventTrack::CommandAdjustMotionEventTrack(MCore::Command* orgCommand)
        : MCore::Command("AdjustMotionEventTrack", orgCommand)
    {
    }


    // destructor
    CommandAdjustMotionEventTrack::~CommandAdjustMotionEventTrack()
    {
    }


    // execute
    bool CommandAdjustMotionEventTrack::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion id and find the corresponding object
        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust motion event track. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        // get the motion event table
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        AZStd::string valueString;
        parameters.GetValue("eventTrackName", this, &valueString);

        // check if the motion event track is valid and return failure in case it is not
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(valueString.c_str());
        if (eventTrack == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust motion event track. Motion event track '%s' does not exist for motion with id='%i'.", valueString.c_str(), motionID);
            return false;
        }

        if (parameters.CheckIfHasParameter("newName"))
        {
            mOldName = eventTrack->GetName();
            parameters.GetValue("newName", this, &valueString);
            eventTrack->SetName(valueString.c_str());
        }

        if (parameters.CheckIfHasParameter("enabled"))
        {
            mOldEnabled = eventTrack->GetIsEnabled();
            const bool enabled = parameters.GetValueAsBool("enabled", this);
            eventTrack->SetIsEnabled(enabled);
        }

        // set the dirty flag
        valueString = AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", motionID);
        return GetCommandManager()->ExecuteCommandInsideCommand(valueString.c_str(), outResult);
    }


    // undo the command
    bool CommandAdjustMotionEventTrack::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion id and find the corresponding object
        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust motion event track. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        // get the motion event table
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();

        // get the event track on which we're working on
        AZStd::string valueString;
        EMotionFX::MotionEventTrack* eventTrack = nullptr;
        if (parameters.CheckIfHasParameter("newName"))
        {
            parameters.GetValue("newName", this, &valueString);
            eventTrack = eventTable->FindTrackByName(valueString.c_str());
        }
        else
        {
            parameters.GetValue("eventTrackName", this, &valueString);
            eventTrack = eventTable->FindTrackByName(valueString.c_str());
        }

        // check if the motion event track is valid and return failure in case it is not
        if (eventTrack == nullptr)
        {
            outResult = AZStd::string::format("Cannot undo adjust motion event track. Motion event track does not exist for motion with id='%i'.", motionID);
            return false;
        }

        if (parameters.CheckIfHasParameter("newName"))
        {
            eventTrack->SetName(mOldName.c_str());
        }

        if (parameters.CheckIfHasParameter("enabled"))
        {
            eventTrack->SetIsEnabled(mOldEnabled);
        }

        // undo remove of the motion
        valueString = AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", motionID);
        return GetCommandManager()->ExecuteCommandInsideCommand(valueString.c_str(), outResult);
    }


    // init the syntax of the command
    void CommandAdjustMotionEventTrack::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        GetSyntax().AddRequiredParameter("motionID",         "The id of the motion.",                                            MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("eventTrackName",   "The name of the motion event track.",                              MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("newName",          "The new name of the motion event track.",                          MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("enabled",          "True in case the motion event track is enabled, false if not.",    MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
    }


    // get the description
    const char* CommandAdjustMotionEventTrack::GetDescription() const
    {
        return "Adjust the attributes of a given motion event track.";
    }

    //--------------------------------------------------------------------------------
    // CommandCreateMotionEvent
    //--------------------------------------------------------------------------------

    // constructor
    CommandCreateMotionEvent::CommandCreateMotionEvent(MCore::Command* orgCommand)
        : MCore::Command("CreateMotionEvent", orgCommand)
    {
    }


    // destructor
    CommandCreateMotionEvent::~CommandCreateMotionEvent()
    {
    }


    // execute
    bool CommandCreateMotionEvent::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion id and find the corresponding object
        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot create motion event. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        // get the motion event table and find the track on which we will work on
        AZStd::string valueString;
        parameters.GetValue("eventTrackName", this, &valueString);
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(valueString.c_str());

        // check if the motion event track is valid and return failure in case it is not
        if (eventTrack == nullptr)
        {
            outResult = AZStd::string::format("Cannot create motion event. Motion event track '%s' does not exist for motion with id='%i'.", valueString.c_str(), motionID);
            return false;
        }

        const float startTime       = parameters.GetValueAsFloat("startTime", this);
        const float endTime         = parameters.GetValueAsFloat("endTime", this);

        parameters.GetValue("eventType", this, &valueString);

        AZStd::string eventParameters;
        parameters.GetValue("parameters", this, &eventParameters);

        // if the event type hasn't been registered yet (automatic event type registration)
        uint32 eventTypeID = EMotionFX::GetEventManager().FindEventID(valueString.c_str());
        if (eventTypeID == MCORE_INVALIDINDEX32)
        {
            // register/link the event type with the free ID
            eventTypeID = EMotionFX::GetEventManager().RegisterEventType(valueString.c_str());
        }

        uint32 mirrorTypeID = MCORE_INVALIDINDEX32;
        if (parameters.CheckIfHasParameter("mirrorType"))
        {
            parameters.GetValue("mirrorType", this, &valueString);
            mirrorTypeID = EMotionFX::GetEventManager().FindEventID(valueString.c_str());
            if (mirrorTypeID == MCORE_INVALIDINDEX32)
            {
                mirrorTypeID = EMotionFX::GetEventManager().RegisterEventType(valueString.c_str());
            }
        }

        // add the motion event and check if everything worked fine
        mMotionEventNr = eventTrack->AddEvent(startTime, endTime, eventTypeID, eventParameters.c_str(), mirrorTypeID);
        if (mMotionEventNr == MCORE_INVALIDINDEX32)
        {
            outResult = AZStd::string::format("Cannot create motion event. The returned motion event index is not valid.");
            return false;
        }

        // set the dirty flag
        valueString = AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", motionID);
        return GetCommandManager()->ExecuteCommandInsideCommand(valueString.c_str(), outResult);
    }


    // undo the command
    bool CommandCreateMotionEvent::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string eventTrackName;
        parameters.GetValue("eventTrackName", this, &eventTrackName);

        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        AZStd::string command;
        command = AZStd::string::format("RemoveMotionEvent -motionID %i -eventTrackName \"%s\" -eventNr %i", motionID, eventTrackName.c_str(), mMotionEventNr);
        return GetCommandManager()->ExecuteCommandInsideCommand(command.c_str(), outResult);
    }


    // init the syntax of the command
    void CommandCreateMotionEvent::InitSyntax()
    {
        GetSyntax().ReserveParameters(6);
        GetSyntax().AddRequiredParameter("motionID",         "The id of the motion.",                                                                                                                                                                        MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("eventTrackName",   "The name of the motion event track.",                                                                                                                                                          MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("startTime",        "The start time value, in seconds, when the motion event should start.",                                                                                                                        MCore::CommandSyntax::PARAMTYPE_FLOAT);
        GetSyntax().AddRequiredParameter("endTime",          "The end time value, in seconds, when the motion event should end. When this is equal to the start time value we won't trigger an end event, but only a start event at the specified time.",    MCore::CommandSyntax::PARAMTYPE_FLOAT);
        GetSyntax().AddRequiredParameter("eventType",        "The string describing the type of the event, this could for example be 'SOUND' or whatever your game can process.",                                                                            MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("parameters",       "The parameters of the event, which could be the filename of a sound file or anything else.",                                                                                                   MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("mirrorType",       "The string describing the mirrored type of the event, this could for example be 'LeftFoot' for an event type of 'RightFoot'.",                                                                 MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    // get the description
    const char* CommandCreateMotionEvent::GetDescription() const
    {
        return "Create a motion event with the given parameters for the given motion.";
    }

    //--------------------------------------------------------------------------------
    // CommandRemoveMotionEvent
    //--------------------------------------------------------------------------------

    // constructor
    CommandRemoveMotionEvent::CommandRemoveMotionEvent(MCore::Command* orgCommand)
        : MCore::Command("RemoveMotionEvent", orgCommand)
    {
    }


    // destructor
    CommandRemoveMotionEvent::~CommandRemoveMotionEvent()
    {
    }


    // execute
    bool CommandRemoveMotionEvent::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion id and find the corresponding object
        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove motion event. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        // get the motion event table and find the track on which we will work on
        AZStd::string valueString;
        parameters.GetValue("eventTrackName", this, &valueString);
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(valueString.c_str());

        // check if the motion event track is valid and return failure in case it is not
        if (eventTrack == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove motion event. Motion event track '%s' does not exist for motion with id='%i'.", valueString.c_str(), motionID);
            return false;
        }

        // get the event index and check if it is in range
        const int32 eventNr = parameters.GetValueAsInt("eventNr", this);
        if (eventNr < 0 || eventNr >= (int32)eventTrack->GetNumEvents())
        {
            outResult = AZStd::string::format("Cannot remove motion event. Motion event number '%i' is out of range.", eventNr);
            return false;
        }

        // get the motion event and store the old values of the motion event for undo
        const EMotionFX::MotionEvent& motionEvent   = eventTrack->GetEvent(eventNr);
        mOldStartTime                               = motionEvent.GetStartTime();
        mOldEndTime                                 = motionEvent.GetEndTime();
        mOldEventType                               = motionEvent.GetEventTypeString();
        mOldParameters                              = motionEvent.GetParameterString(eventTrack);

        // remove the motion event
        eventTrack->RemoveEvent(eventNr);
        eventTrack->RemoveUnusedParameters();

        // set the dirty flag
        valueString = AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", motionID);
        return GetCommandManager()->ExecuteCommandInsideCommand(valueString.c_str(), outResult);
    }


    // undo the command
    bool CommandRemoveMotionEvent::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string eventTrackName;
        parameters.GetValue("eventTrackName", this, &eventTrackName);

        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        AZStd::string command;
        command = AZStd::string::format("CreateMotionEvent -motionID %i -eventTrackName \"%s\" -startTime %f -endTime %f -eventType \"%s\" -parameters \"%s\"", motionID, eventTrackName.c_str(), mOldStartTime, mOldEndTime, mOldEventType.c_str(), mOldParameters.c_str());
        return GetCommandManager()->ExecuteCommandInsideCommand(command.c_str(), outResult);
    }


    // init the syntax of the command
    void CommandRemoveMotionEvent::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddRequiredParameter("motionID",        "The id of the motion.",                    MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("eventTrackName",  "The name of the motion event track.",      MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("eventNr",         "The index of the motion event to remove.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    // get the description
    const char* CommandRemoveMotionEvent::GetDescription() const
    {
        return "Remove a motion event from the given motion.";
    }



    //--------------------------------------------------------------------------------
    // CommandAdjustMotionEvent
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustMotionEvent::CommandAdjustMotionEvent(MCore::Command* orgCommand)
        : MCore::Command("AdjustMotionEvent", orgCommand)
    {
    }


    // destructor
    CommandAdjustMotionEvent::~CommandAdjustMotionEvent()
    {
    }


    // execute
    bool CommandAdjustMotionEvent::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion id and find the corresponding object
        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust motion event. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        // get the motion event table and find the track on which we will work on
        AZStd::string valueString;
        parameters.GetValue("eventTrackName", this, &valueString);
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(valueString.c_str());

        // check if the motion event track is valid and return failure in case it is not
        if (eventTrack == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust motion event. Motion event track '%s' does not exist for motion with id='%i'.", valueString.c_str(), motionID);
            return false;
        }

        // get the event index and check if it is in range
        const int32 eventNr = parameters.GetValueAsInt("eventNr", this);
        if (eventNr < 0 || eventNr >= (int32)eventTrack->GetNumEvents())
        {
            outResult = AZStd::string::format("Cannot adjust motion event. Motion event number '%i' is out of range.", eventNr);
            return false;
        }

        // get the motion event
        EMotionFX::MotionEvent& motionEvent = eventTrack->GetEvent(eventNr);

        mOldStartTime   = motionEvent.GetStartTime();
        mOldEndTime     = motionEvent.GetEndTime();

        // adjust the event start time
        if (parameters.CheckIfHasParameter("startTime"))
        {
            const float startTime = parameters.GetValueAsFloat("startTime", this);
            if (startTime > motionEvent.GetEndTime())
            {
                motionEvent.SetEndTime(startTime);
            }

            motionEvent.SetStartTime(startTime);
        }

        // adjust the event end time
        if (parameters.CheckIfHasParameter("endTime"))
        {
            const float endTime = parameters.GetValueAsFloat("endTime", this);
            if (endTime < motionEvent.GetStartTime())
            {
                motionEvent.SetStartTime(endTime);
            }

            motionEvent.SetEndTime(endTime);
        }

        // adjust the event type
        if (parameters.CheckIfHasParameter("eventType"))
        {
            mOldEventTypeID = motionEvent.GetEventTypeID();
            mOldEventType   = motionEvent.GetEventTypeString();
            parameters.GetValue("eventType", this, &valueString);

            // if the event type hasn't been registered yet (automatic event type registration)
            uint32 eventTypeID = EMotionFX::GetEventManager().FindEventID(valueString.c_str());
            if (eventTypeID == MCORE_INVALIDINDEX32)
            {
                // register/link the event type with the free ID
                eventTypeID = EMotionFX::GetEventManager().RegisterEventType(valueString.c_str());
            }

            motionEvent.SetEventTypeID(eventTypeID);
        }

        // adjust the event parameters
        if (parameters.CheckIfHasParameter("parameters"))
        {
            mOldParamIndex = motionEvent.GetParameterIndex();
            parameters.GetValue("parameters", this, &valueString);
            const uint32 paramIndex = eventTrack->AddParameter(valueString.c_str());
            motionEvent.SetParameterIndex(static_cast<uint16>(paramIndex));
        }

        if (parameters.CheckIfHasParameter("mirrorType"))
        {
            mOldMirrorTypeID = motionEvent.GetMirrorEventID();
            mOldMirrorType = motionEvent.GetMirrorEventTypeString();
            AZStd::string mirrorType;
            parameters.GetValue("mirrorType", this, &mirrorType);
            uint32 mirrorEventID = EMotionFX::GetEventManager().FindEventID(mirrorType.c_str());
            if (mirrorEventID == MCORE_INVALIDINDEX32)
            {
                mirrorEventID = EMotionFX::GetEventManager().RegisterEventType(mirrorType.c_str());
            }
            motionEvent.SetMirrorEventID(mirrorEventID);
        }

        // set the dirty flag
        valueString = AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", motionID);
        return GetCommandManager()->ExecuteCommandInsideCommand(valueString.c_str(), outResult);
    }


    // undo the command
    bool CommandAdjustMotionEvent::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion id and find the corresponding object
        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust motion event. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        // get the motion event table and find the track on which we will work on
        AZStd::string valueString;
        parameters.GetValue("eventTrackName", this, &valueString);
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(valueString.c_str());

        // check if the motion event track is valid and return failure in case it is not
        if (eventTrack == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust motion event. Motion event track '%s' does not exist for motion with id='%i'.", valueString.c_str(), motionID);
            return false;
        }

        // get the event index and check if it is in range
        const int32 eventNr = parameters.GetValueAsInt("eventNr", this);
        if (eventNr < 0 || eventNr >= (int32)eventTrack->GetNumEvents())
        {
            outResult = AZStd::string::format("Cannot adjust motion event. Motion event number '%i' is out of range.", eventNr);
            return false;
        }

        // get the motion event
        EMotionFX::MotionEvent& motionEvent = eventTrack->GetEvent(eventNr);

        // undo start time change
        if (parameters.CheckIfHasParameter("startTime"))
        {
            motionEvent.SetStartTime(mOldStartTime);
            motionEvent.SetEndTime(mOldEndTime);
        }

        // undo end time change
        if (parameters.CheckIfHasParameter("endTime"))
        {
            motionEvent.SetStartTime(mOldStartTime);
            motionEvent.SetEndTime(mOldEndTime);
        }

        // undo event type string change
        if (parameters.CheckIfHasParameter("eventType"))
        {
            motionEvent.SetEventTypeID(mOldEventTypeID);
        }

        // undo event type string change
        if (parameters.CheckIfHasParameter("mirrorType"))
        {
            motionEvent.SetMirrorEventID(mOldMirrorTypeID);
        }

        // adjust the event parameters
        if (parameters.CheckIfHasParameter("parameters"))
        {
            motionEvent.SetParameterIndex(static_cast<uint16>(mOldParamIndex));
        }

        // undo remove of the motion
        valueString = AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", motionID);
        return GetCommandManager()->ExecuteCommandInsideCommand(valueString.c_str(), outResult);
    }


    // init the syntax of the command
    void CommandAdjustMotionEvent::InitSyntax()
    {
        GetSyntax().ReserveParameters(7);
        GetSyntax().AddRequiredParameter("motionID",         "The id of the motion.",                                                                                                                                                                        MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("eventTrackName",   "The name of the motion event track.",                                                                                                                                                          MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("eventNr",          "The index of the motion event to remove.",                                                                                                                                                     MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("startTime",        "The start time value, in seconds, when the motion event should start.",                                                                                                                        MCore::CommandSyntax::PARAMTYPE_FLOAT,  "0.0");
        GetSyntax().AddParameter("endTime",          "The end time value, in seconds, when the motion event should end. When this is equal to the start time value we won't trigger an end event, but only a start event at the specified time.",    MCore::CommandSyntax::PARAMTYPE_FLOAT,  "0.0");
        GetSyntax().AddParameter("eventType",        "The string describing the type of the event, this could for example be 'SOUND' or whatever your game can process.",                                                                            MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("parameters",       "The parameters of the event, which could be the filename of a sound file or anything else.",                                                                                                   MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("mirrorType",       "The string describing the type mirrored event type. For an event type of LeftFoot this could be RightFoot. You can leave it empty if mirroring isn't having any effect on this event.",        MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    // get the description
    const char* CommandAdjustMotionEvent::GetDescription() const
    {
        return "Adjust the attributes of a given motion event.";
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // add event track
    void CommandAddEventTrack(EMotionFX::Motion* motion)
    {
        // make sure the motion is valid
        if (motion == nullptr)
        {
            return;
        }

        EMotionFX::MotionEventTable* motionEventTable = motion->GetEventTable();
        uint32 trackNr = motionEventTable->GetNumTracks() + 1;

        AZStd::string eventTrackName;
        eventTrackName = AZStd::string::format("Event Track %i", trackNr);
        while (motionEventTable->FindTrackByName(eventTrackName.c_str()))
        {
            trackNr++;
            eventTrackName = AZStd::string::format("Event Track %i", trackNr);
        }

        AZStd::string outResult;
        AZStd::string command = AZStd::string::format("CreateMotionEventTrack -motionID %i -eventTrackName \"%s\"", motion->GetID(), eventTrackName.c_str());
        if (GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    // add event track
    void CommandAddEventTrack()
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        CommandAddEventTrack(motion);
    }


    // remove event track
    void CommandRemoveEventTrack(EMotionFX::Motion* motion, uint32 trackIndex)
    {
        // make sure the motion is valid
        if (motion == nullptr)
        {
            return;
        }

        // get the motion event table and the given event track
        EMotionFX::MotionEventTable*    eventTable  = motion->GetEventTable();
        EMotionFX::MotionEventTrack*    eventTrack  = eventTable->GetTrack(trackIndex);

        AZStd::string outResult;
        MCore::CommandGroup commandGroup;

        // get the number of motion events and iterate through them
        const uint32 numMotionEvents = eventTrack->GetNumEvents();
        for (uint32 j = 0; j < numMotionEvents; ++j)
        {
            commandGroup.AddCommandString(AZStd::string::format("RemoveMotionEvent -motionID %i -eventTrackName \"%s\" -eventNr %i", motion->GetID(), eventTrack->GetName(), 0).c_str());
        }

        // finally remove the event track
        commandGroup.AddCommandString(AZStd::string::format("RemoveMotionEventTrack -motionID %i -eventTrackName \"%s\"", motion->GetID(), eventTrack->GetName()).c_str());

        // execute the command group
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    // remove event track
    void CommandRemoveEventTrack(uint32 trackIndex)
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        CommandRemoveEventTrack(motion, trackIndex);
    }


    // rename event track
    void CommandRenameEventTrack(EMotionFX::Motion* motion, uint32 trackIndex, const char* newName)
    {
        // make sure the motion is valid
        if (motion == nullptr)
        {
            return;
        }

        // get the motion event table and the given event track
        EMotionFX::MotionEventTable*    eventTable  = motion->GetEventTable();
        EMotionFX::MotionEventTrack*    eventTrack  = eventTable->GetTrack(trackIndex);

        AZStd::string outResult;
        AZStd::string command = AZStd::string::format("AdjustMotionEventTrack -motionID %i -eventTrackName \"%s\" -newName \"%s\"", motion->GetID(), eventTrack->GetName(), newName);
        if (GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    // rename event track
    void CommandRenameEventTrack(uint32 trackIndex, const char* newName)
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        CommandRenameEventTrack(motion, trackIndex, newName);
    }


    // enable or disable event track
    void CommandEnableEventTrack(EMotionFX::Motion* motion, uint32 trackIndex, bool isEnabled)
    {
        // make sure the motion is valid
        if (motion == nullptr)
        {
            return;
        }

        // get the motion event table and the given event track
        EMotionFX::MotionEventTable*    eventTable  = motion->GetEventTable();
        EMotionFX::MotionEventTrack*    eventTrack  = eventTable->GetTrack(trackIndex);

        AZStd::string outResult;
        AZStd::string command = AZStd::string::format("AdjustMotionEventTrack -motionID %i -eventTrackName \"%s\" -enabled %s", 
            motion->GetID(), 
            eventTrack->GetName(), 
            AZStd::to_string(isEnabled).c_str());
        if (GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    // enable or disable event track
    void CommandEnableEventTrack(uint32 trackIndex, bool isEnabled)
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        CommandEnableEventTrack(motion, trackIndex, isEnabled);
    }


    // add a new motion event
    void CommandHelperAddMotionEvent(EMotionFX::Motion* motion, const char* trackName, float startTime, float endTime, const char* eventType, const char* eventParameters, MCore::CommandGroup* commandGroup)
    {
        // make sure the motion is valid
        if (motion == nullptr)
        {
            return;
        }

        // create our command group
        MCore::CommandGroup internalCommandGroup("Add motion event");

        // execute the create motion event command
        AZStd::string command;
        command = AZStd::string::format("CreateMotionEvent -motionID %i -eventTrackName \"%s\" -startTime %f -endTime %f -eventType \"%s\" -parameters \"%s\"", motion->GetID(), trackName, startTime, endTime, eventType, eventParameters);

        // add the command to the command group
        if (commandGroup == nullptr)
        {
            internalCommandGroup.AddCommandString(command.c_str());
        }
        else
        {
            commandGroup->AddCommandString(command.c_str());
        }

        // execute the group command
        if (commandGroup == nullptr)
        {
            AZStd::string outResult;
            if (GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, outResult) == false)
            {
                MCore::LogError(outResult.c_str());
            }
        }
    }


    // add a new motion event
    void CommandHelperAddMotionEvent(const char* trackName, float startTime, float endTime, const char* eventType, const char* eventParameters, MCore::CommandGroup* commandGroup)
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        CommandHelperAddMotionEvent(motion, trackName, startTime, endTime, eventType, eventParameters, commandGroup);
    }


    // remove motion event
    void CommandHelperRemoveMotionEvent(EMotionFX::Motion* motion, const char* trackName, uint32 eventNr, MCore::CommandGroup* commandGroup)
    {
        // make sure the motion is valid
        if (motion == nullptr)
        {
            return;
        }

        // create our command group
        MCore::CommandGroup internalCommandGroup("Remove motion event");

        // execute the create motion event command
        AZStd::string command;
        command = AZStd::string::format("RemoveMotionEvent -motionID %i -eventTrackName \"%s\" -eventNr %i", motion->GetID(), trackName, eventNr);

        // add the command to the command group
        if (commandGroup == nullptr)
        {
            internalCommandGroup.AddCommandString(command.c_str());
        }
        else
        {
            commandGroup->AddCommandString(command.c_str());
        }

        // execute the group command
        if (commandGroup == nullptr)
        {
            AZStd::string outResult;
            if (GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, outResult) == false)
            {
                MCore::LogError(outResult.c_str());
            }
        }
    }


    // remove motion event
    void CommandHelperRemoveMotionEvent(uint32 motionID, const char* trackName, uint32 eventNr, MCore::CommandGroup* commandGroup)
    {
        // find the motion by id
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            return;
        }

        CommandHelperRemoveMotionEvent(motion, trackName, eventNr, commandGroup);
    }

    // remove motion event
    void CommandHelperRemoveMotionEvent(const char* trackName, uint32 eventNr, MCore::CommandGroup* commandGroup)
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        if (motion == nullptr)
        {
            return;
        }

        CommandHelperRemoveMotionEvent(motion, trackName, eventNr, commandGroup);
    }


    // remove motion event
    void CommandHelperRemoveMotionEvents(uint32 motionID, const char* trackName, const MCore::Array<uint32>& eventNumbers, MCore::CommandGroup* commandGroup)
    {
        // find the motion by id
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            return;
        }

        // create our command group
        MCore::CommandGroup internalCommandGroup("Remove motion events");

        // get the number of events to remove and iterate through them
        const int32 numEvents = eventNumbers.GetLength();
        for (int32 i = 0; i < numEvents; ++i)
        {
            // remove the events from back to front
            uint32 eventNr = eventNumbers[numEvents - 1 - i];

            // add the command to the command group
            if (commandGroup == nullptr)
            {
                CommandHelperRemoveMotionEvent(motion, trackName, eventNr, &internalCommandGroup);
            }
            else
            {
                CommandHelperRemoveMotionEvent(motion, trackName, eventNr, commandGroup);
            }
        }

        // execute the group command
        if (commandGroup == nullptr)
        {
            AZStd::string outResult;
            if (GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, outResult) == false)
            {
                MCore::LogError(outResult.c_str());
            }
        }
    }


    // remove motion event
    void CommandHelperRemoveMotionEvents(const char* trackName, const MCore::Array<uint32>& eventNumbers, MCore::CommandGroup* commandGroup)
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        if (motion == nullptr)
        {
            return;
        }

        CommandHelperRemoveMotionEvents(motion->GetID(), trackName, eventNumbers, commandGroup);
    }


    void CommandHelperMotionEventTrackChanged(EMotionFX::Motion* motion, uint32 eventNr, float startTime, float endTime, const char* oldTrackName, const char* newTrackName)
    {
        // get the motion event track
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(oldTrackName);
        if (eventTrack == nullptr)
        {
            return;
        }

        // check if the motion event number is valid and return in case it is not
        if (eventNr >= eventTrack->GetNumEvents())
        {
            return;
        }

        // create the command group
        AZStd::string result;
        MCore::CommandGroup commandGroup("Change motion event track");

        // get the motion event
        EMotionFX::MotionEvent& motionEvent = eventTrack->GetEvent(eventNr);
        const AZStd::string type            = motionEvent.GetEventTypeString();
        const AZStd::string parameters      = motionEvent.GetParameterString(eventTrack);

        commandGroup.AddCommandString(AZStd::string::format("RemoveMotionEvent -motionID %i -eventTrackName \"%s\" -eventNr %i", motion->GetID(), oldTrackName, eventNr));
        commandGroup.AddCommandString(AZStd::string::format("CreateMotionEvent -motionID %i -eventTrackName \"%s\" -startTime %f -endTime %f -eventType \"%s\" -parameters \"%s\"", motion->GetID(), newTrackName, startTime, endTime, type.c_str(), parameters.c_str()));

        // execute the command group
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, result) == false)
        {
            MCore::LogError(result.c_str());
        }
    }


    void CommandHelperMotionEventTrackChanged(uint32 eventNr, float startTime, float endTime, const char* oldTrackName, const char* newTrackName)
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        CommandHelperMotionEventTrackChanged(motion, eventNr, startTime, endTime, oldTrackName, newTrackName);
    }
} // namespace CommandSystem
