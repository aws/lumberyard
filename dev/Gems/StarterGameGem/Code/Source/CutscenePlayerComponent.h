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

#include <AzCore/Component/Component.h>

#include <AzCore/Math/Transform.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/any.h>
#include <GameplayEventBus.h>
#include <IMovieSystem.h>
#include <LmbrCentral/Scripting/TagComponentBus.h>

namespace StarterGameGem
{

	/*!
	* CutscenePlayerRequests
	* Messages serviced by the CutscenePlayerComponent.
	*/
	class CutscenePlayerRequests : public AZ::ComponentBus
	{
	public:
		virtual ~CutscenePlayerRequests() {}

		virtual void Start() = 0;
		virtual void StartSpecific(const char* name) = 0;
		virtual bool Stop() = 0;
		virtual bool IsPlaying() = 0;


		virtual void SetCutsceneName(const char* name) = 0;
		virtual void SetStartEvent(const char* name) = 0;
		virtual void SetStopEvent(const char* name) = 0;
		virtual void SetFinishedEvent(const char* name) = 0;
		virtual void SetStartedEvent(const char* name) = 0;

		virtual AZStd::string GetCutsceneName() = 0;
		virtual AZStd::string GetStartEvent() = 0;
		virtual AZStd::string GetStopEvent() = 0;
		virtual AZStd::string GetStartedEvent() = 0;
		virtual AZStd::string GetFinishedEvent() = 0;
	};
	using CutscenePlayerRequestsBus = AZ::EBus<CutscenePlayerRequests>;
	

	class CutscenePlayerComponent
		: public AZ::Component
		, private CutscenePlayerRequestsBus::Handler
		, public AZ::GameplayNotificationBus::MultiHandler
		, private AZ::TickBus::Handler
		, private IMovieListener
	{
	public:
		AZ_COMPONENT(CutscenePlayerComponent, "{8CBB904C-04EE-4107-8FA8-3ACF61D79CB7}");

		CutscenePlayerComponent();

		//////////////////////////////////////////////////////////////////////////
		// AZ::Component interface implementation
		void Init() override		{}
		void Activate() override;
		void Deactivate() override;
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// AZ::TickBus interface implementation
		void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
		//////////////////////////////////////////////////////////////////////////
		
		//////////////////////////////////////////////////////////////////////////
		// IMovieListener interface implementation
		void OnMovieEvent(EMovieEvent movieEvent, IAnimSequence* pAnimSequence);
		//////////////////////////////////////////////////////////////////////////

		// Required Reflect function.
		static void Reflect(AZ::ReflectContext* context);

		// Optional functions for defining provided and dependent services.
		//static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
		//{
		//	required.push_back(AZ_CRC("TransformService"));
		//}

		//////////////////////////////////////////////////////////////////////////
		// CutscenePlayerComponentRequestBus::Handler

		void Start();
		void StartSpecific(const char* name);
		bool Stop();
		bool IsPlaying();


		void SetCutsceneName(const char* name);
		void SetStartEvent(const char* name);
		void SetStopEvent(const char* name);
		void SetStartedEvent(const char* name);
		void SetFinishedEvent(const char* name);

		AZStd::string GetCutsceneName();
		AZStd::string GetStartEvent();
		AZStd::string GetStopEvent();
		AZStd::string GetStartedEvent();
		AZStd::string GetFinishedEvent();
	
		void OnEventBegin(const AZStd::any& value) override;
		void OnEventUpdating(const AZStd::any&) override;
		void OnEventEnd(const AZStd::any&) override;

	protected:
		AZStd::string m_cutsceneName;
		bool m_disablePlayer;
		// messages in
		AZStd::string m_StartEventName;
		AZStd::string m_StopEventName;
		// messages out
		AZStd::string m_StartedEventName;
		AZStd::string m_FinishedEventName;
		AZStd::string m_DisablePlayerEventName;
		AZStd::string m_EnablePlayerEventName;
		AZStd::string m_PlayerTag;

	private:
		IAnimSequence* m_sequence;

		AZ::GameplayNotificationId m_StartEventID;
		AZ::GameplayNotificationId m_StopEventID;

		AZ::GameplayNotificationId m_StartedEventID;
		AZ::GameplayNotificationId m_FinishedEventID;

		static AZ::EntityId GetEntityByTag(const LmbrCentral::Tag& tag);
	};

}
