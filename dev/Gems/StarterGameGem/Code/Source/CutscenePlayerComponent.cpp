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
#include "StarterGameGem_precompiled.h"
#include "CutscenePlayerComponent.h"

#include "StarterGameUtility.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <GameplayEventBus.h>
#include <AzCore/Math/Crc.h>
#include <IMovieSystem.h>

namespace StarterGameGem
{
    CutscenePlayerComponent::CutscenePlayerComponent()
        : m_cutsceneName("")
		, m_StartEventName("StartCutscene")
		, m_disablePlayer(true)
		, m_StopEventName("StopCutscene")
		, m_StartedEventName("CutsceneHasStarted")
		, m_FinishedEventName("CutsceneHasStopped")
		, m_DisablePlayerEventName("Disable")
		, m_EnablePlayerEventName("Enable")
		, m_PlayerTag("PlayerCharacter")
		, m_sequence(NULL)
    {}

    void CutscenePlayerComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<CutscenePlayerComponent, AZ::Component>()
                ->Version(1)
                ->Field("CutsceneName", &CutscenePlayerComponent::m_cutsceneName)
				->Field("DisablePlayer", &CutscenePlayerComponent::m_disablePlayer)
				->Field("StartEventName", &CutscenePlayerComponent::m_StartEventName)
				->Field("StopEventName", &CutscenePlayerComponent::m_StopEventName)
				->Field("StartedEventName", &CutscenePlayerComponent::m_StartedEventName)
				->Field("FinishedEventName", &CutscenePlayerComponent::m_FinishedEventName)
 				->Field("DisablePlayerEventName", &CutscenePlayerComponent::m_DisablePlayerEventName)
 				->Field("EnablePlayerEventName", &CutscenePlayerComponent::m_EnablePlayerEventName)
 				->Field("PlayerTag", &CutscenePlayerComponent::m_PlayerTag)
               ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<CutscenePlayerComponent>("Cutscene Component", "Playes Sequences and manages interactions")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Game")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->DataElement(0, &CutscenePlayerComponent::m_cutsceneName, "Cutscene", "The Name of the sequence to play.")
					->DataElement(0, &CutscenePlayerComponent::m_disablePlayer, "Disable Player", "If this is set the player will be disabled while the cutscene is playing.")
					->ClassElement(AZ::Edit::ClassElements::Group, "Messages In")
					->DataElement(0, &CutscenePlayerComponent::m_StartEventName, "Start Event", "Message to send to start playing the Cutscene.")
                    ->DataElement(0, &CutscenePlayerComponent::m_StopEventName, "Stop Event", "Message to send to stop playing the Cutscene.")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Messages Out")
                    ->DataElement(0, &CutscenePlayerComponent::m_StartedEventName, "Started Event", "This message is sent when the cutscene starts playing.")
                    ->DataElement(0, &CutscenePlayerComponent::m_FinishedEventName, "Finished Event", "This message is sent when the cutscene stops playing.")
					->DataElement(0, &CutscenePlayerComponent::m_DisablePlayerEventName, "Disable Player Event", "This message is to the player to disable them.")
					->DataElement(0, &CutscenePlayerComponent::m_EnablePlayerEventName, "Enable Player Event", "This message is to the player to Enable them.")
					->DataElement(0, &CutscenePlayerComponent::m_PlayerTag, "Player Tag", "This tag identifies the player to send the medssage at.")
					;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behaviorContext->EBus<CutscenePlayerRequestsBus>("CutscenePlayerRequestsBus")
				->Event("Start", &CutscenePlayerRequestsBus::Events::Start)
				->Event("StartSpecific", &CutscenePlayerRequestsBus::Events::StartSpecific)
				->Event("Stop", &CutscenePlayerRequestsBus::Events::Stop)
				->Event("IsPlaying", &CutscenePlayerRequestsBus::Events::IsPlaying)
				->Event("SetCutsceneName", &CutscenePlayerRequestsBus::Events::SetCutsceneName)
				->Event("SetStartEvent", &CutscenePlayerRequestsBus::Events::SetStartEvent)
				->Event("SetStopEvent", &CutscenePlayerRequestsBus::Events::SetStopEvent)
				->Event("SetStartedEvent", &CutscenePlayerRequestsBus::Events::SetStartedEvent)
				->Event("SetFinishedEvent", &CutscenePlayerRequestsBus::Events::SetFinishedEvent)
				->Event("GetCutsceneName", &CutscenePlayerRequestsBus::Events::GetCutsceneName)
				->Event("GetStartEvent", &CutscenePlayerRequestsBus::Events::GetStartEvent)
				->Event("GetStopEvent", &CutscenePlayerRequestsBus::Events::GetStopEvent)
				->Event("GetStartedEvent", &CutscenePlayerRequestsBus::Events::GetStartedEvent)
				->Event("GetFinishedEvent", &CutscenePlayerRequestsBus::Events::GetFinishedEvent)
                ;
        }
    }

    void CutscenePlayerComponent::Activate()
    {
        AZ::EntityId myID = GetEntityId();

        CutscenePlayerRequestsBus::Handler::BusConnect(myID);
        AZ::TickBus::Handler::BusConnect();

        // set IDs
		m_StartEventID = AZ::GameplayNotificationId(myID, AZ::Crc32(m_StartEventName.c_str()), StarterGameUtility::GetUuid("float"));
		m_StopEventID = AZ::GameplayNotificationId(myID, AZ::Crc32(m_StopEventName.c_str()), StarterGameUtility::GetUuid("float"));
		
		m_StartedEventID = AZ::GameplayNotificationId(myID, AZ::Crc32(m_StartedEventName.c_str()), StarterGameUtility::GetUuid("float"));
		m_FinishedEventID = AZ::GameplayNotificationId(myID, AZ::Crc32(m_FinishedEventName.c_str()), StarterGameUtility::GetUuid("float"));

        //AZ_TracePrintf("StarterGame", "CutscenePlayerComponent::Activate.  [%llu]", (AZ::u64)myID);

        // attach
        AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_StartEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_StopEventID);
    }

	void CutscenePlayerComponent::Deactivate()
	{
		if (m_sequence != NULL)
		{
			IMovieSystem* movieSys = gEnv->pMovieSystem;
			if (movieSys == NULL)
			{
				// something is terribly wrong
				return;
			}
			else
			{
				movieSys->RemoveMovieListener(m_sequence, this);
			}
			m_sequence = NULL;
		}

        //AZ::TickBus::Handler::BusDisconnect();
        CutscenePlayerRequestsBus::Handler::BusDisconnect();

        // disconnect
        AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_StartEventID);
		AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_StopEventID);
    }

	AZ::EntityId CutscenePlayerComponent::GetEntityByTag(const LmbrCentral::Tag& tag)
	{
		AZ::EBusAggregateResults<AZ::EntityId> results;
		AZ::EntityId ret = AZ::EntityId();
		EBUS_EVENT_ID_RESULT(results, tag, LmbrCentral::TagGlobalRequestBus, RequestTaggedEntities);
		for (const AZ::EntityId& entity : results.values)
		{
			if (entity.IsValid())
			{
				ret = entity;
				break;
			}
		}

		return ret;
	}

	void CutscenePlayerComponent::OnMovieEvent(EMovieEvent movieEvent, IAnimSequence* pAnimSequence)
	{
		IMovieSystem* movieSys = gEnv->pMovieSystem;
		if (movieSys == NULL)
		{
			// something is terribly wrong
			return;
		}

		// do things based upon the message recieved
		if (pAnimSequence->GetName() == m_cutsceneName)
		{
			switch(movieEvent)
			{
			case EMovieEvent::eMovieEvent_Started:
				if (m_StartedEventName != "")
					AZ::GameplayNotificationBus::Event(m_StartedEventID, &AZ::GameplayNotificationBus::Events::OnEventBegin, AZStd::any(true));
				if (m_disablePlayer)
				{
					AZ::GameplayNotificationBus::Event( 
						AZ::GameplayNotificationId(GetEntityByTag(AZ::Crc32(m_PlayerTag.c_str())), AZ::Crc32(m_DisablePlayerEventName.c_str()), StarterGameUtility::GetUuid("float")), 
						&AZ::GameplayNotificationBus::Events::OnEventBegin, AZStd::any(true));
				}
				break;
			case EMovieEvent::eMovieEvent_Stopped:
			case EMovieEvent::eMovieEvent_Aborted:				
				if (m_FinishedEventName != "")
					AZ::GameplayNotificationBus::Event(m_FinishedEventID, &AZ::GameplayNotificationBus::Events::OnEventBegin, AZStd::any(movieEvent == EMovieEvent::eMovieEvent_Stopped));
				if(m_sequence != NULL)
					movieSys->RemoveMovieListener(m_sequence, this); 
				m_sequence = NULL;
				if (m_disablePlayer)
				{
					AZ::GameplayNotificationBus::Event(
						AZ::GameplayNotificationId(GetEntityByTag(AZ::Crc32(m_PlayerTag.c_str())), AZ::Crc32(m_EnablePlayerEventName.c_str()), StarterGameUtility::GetUuid("float")),
						&AZ::GameplayNotificationBus::Events::OnEventBegin, AZStd::any(true));
				}				
				break;
			case EMovieEvent::eMovieEvent_Updated:
			case EMovieEvent::eMovieEvent_RecordModeStarted:
			case EMovieEvent::eMovieEvent_RecordModeStopped:
				return;
			}
		}
	}


    void CutscenePlayerComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
  
    }


	void CutscenePlayerComponent::Start()
	{
		IMovieSystem* movieSys = gEnv->pMovieSystem;
		if (movieSys == NULL)
		{
			// AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::PlaySequence: There is no system");
		}
		else
		{
			// attach listeners!
			m_sequence = movieSys->FindLegacySequenceByName(m_cutsceneName.c_str());
			
			if (m_sequence != NULL)
			{
				movieSys->AddMovieListener(m_sequence, this);

				movieSys->PlaySequence(m_cutsceneName.c_str(), NULL, true, false);
			}
		}
	}

	void CutscenePlayerComponent::StartSpecific(const char* name)
	{
		if (m_sequence != NULL)
		{
			// caution!
		}

		SetCutsceneName(name);
		Start();
	}

	bool CutscenePlayerComponent::Stop()
	{
		if (m_sequence == NULL)
		{
			// i am not playing a thing
			return false;
		}

		IMovieSystem* movieSys = gEnv->pMovieSystem;
		if (movieSys == NULL)
		{
			// AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::PlaySequence: There is no system");
		}
		else
		{
			// Deattach listeners!
			bool returnVal = movieSys->StopSequence(m_cutsceneName.c_str());
			movieSys->RemoveMovieListener(m_sequence, this);
			m_sequence = NULL;
			return returnVal;
		}
		return false;
	}

	bool CutscenePlayerComponent::IsPlaying()
	{
		return  (m_sequence != NULL);
	}

	void CutscenePlayerComponent::SetCutsceneName(const char* name)
	{
		if (m_sequence != NULL)
		{
			// Caution!
		}

		m_cutsceneName = name;
	}

	void CutscenePlayerComponent::SetStartEvent(const char* name)
	{
		if (m_StartedEventName == name)
			return;

		if (m_StartedEventName != "")
		{
			// detach the listeners
			AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_StartEventID);
			m_StartEventID = AZ::GameplayNotificationId();
		}

		m_StartEventName = (name != NULL) ? name : "";

		if (m_StartEventName != "")
		{
			m_StartEventID = AZ::GameplayNotificationId(GetEntityId(), AZ::Crc32(m_StartEventName.c_str()), StarterGameUtility::GetUuid("float"));
			AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_StartEventID);
		}
	}

	void CutscenePlayerComponent::SetStopEvent(const char* name)
	{
		// possibly change this to only attach when i am playing
		if (m_StopEventName == name)
			return;

		if (m_StopEventName != "")
		{
			// detach the listeners
			AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_StopEventID);
			m_StopEventID = AZ::GameplayNotificationId();
		}

		m_StopEventName = (name != NULL) ? name : "";

		if (m_StopEventName != "")
		{
			m_StopEventID = AZ::GameplayNotificationId(GetEntityId(), AZ::Crc32(m_StopEventName.c_str()), StarterGameUtility::GetUuid("float"));
			AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_StopEventID);
		}
	}


	void CutscenePlayerComponent::SetStartedEvent(const char* name)
	{
		m_StartedEventName = (name != NULL) ? name : "";

		if (m_StartedEventName != "")
		{
			m_StartedEventID = AZ::GameplayNotificationId(GetEntityId(), AZ::Crc32(m_StartedEventName.c_str()), StarterGameUtility::GetUuid("float"));
		}
	}

	void CutscenePlayerComponent::SetFinishedEvent(const char* name)
	{
		m_FinishedEventName = (name != NULL) ? name : "";

		if (m_FinishedEventName != "")
		{
			m_FinishedEventID = AZ::GameplayNotificationId(GetEntityId(), AZ::Crc32(m_FinishedEventName.c_str()), StarterGameUtility::GetUuid("float"));
		}
	}

	AZStd::string CutscenePlayerComponent::GetCutsceneName()
	{
		return m_cutsceneName;
	}

	AZStd::string CutscenePlayerComponent::GetStartEvent()
	{
		return m_StartEventName;
	}

	AZStd::string CutscenePlayerComponent::GetStopEvent()
	{
		return m_StopEventName;
	}

	AZStd::string CutscenePlayerComponent::GetStartedEvent()
	{
		return m_StartedEventName;
	}

	AZStd::string CutscenePlayerComponent::GetFinishedEvent()
	{
		return m_FinishedEventName;
	}

    void CutscenePlayerComponent::OnEventBegin(const AZStd::any& value)
    {
        float valueAsFloat = 0;
        bool isNumber = AZStd::any_numeric_cast<float>(&value, valueAsFloat);
        AZ::EntityId valueAsEntityID;
        if (value.is<AZ::EntityId>()) // if you are not this type the below function crashes
            valueAsEntityID = AZStd::any_cast<AZ::EntityId>(value);

        AZ::EntityId myID = GetEntityId();
        //AZ_TracePrintf("StarterGame", "CutscenePlayerComponent::OnEventBegin. [%llu], is a number %d : %.03f", (AZ::u64)myID, isNumber, valueAsFloat);

        if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_StartEventID)
        {
            Start();
        }
        else if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_StopEventID)
        {
            Stop();
        }
    }

    void CutscenePlayerComponent::OnEventUpdating(const AZStd::any& value)
    {
    }

    void CutscenePlayerComponent::OnEventEnd(const AZStd::any& value)
    {
    }


}

