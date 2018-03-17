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
#include "StarterGameGemModule.h"

#include "StarterGameGemSystemComponent.h"

#include "AISoundManagerSystemComponent.h"
#include "PersistentDataSystemComponent.h"
#include "VisualiseAIStatesSystemComponent.h"
#include "VisualisePathSystemComponent.h"

#include "AudioMultiListenerComponent.h"
#include "CameraSettingsComponent.h"
#include "ConsoleVarListenerComponent.h"
#include "CutscenePlayerComponent.h"
#include "DebugManagerComponent.h"
#include "DecalSelectorComponent.h"
#include "LineRendererComponent.h"
#include "ParticleManagerComponent.h"
#include "PersistentDataSystemComparitorComponent.h"
#include "PersistentDataSystemManipulatorComponent.h"
#include "PlayFromHereComponent.h"
#include "StarterGameNavigationComponent.h"
#include "StatComponent.h"
#include "VisualiseRangeComponent.h"
#include "WaypointSettingsComponent.h"
#include "WaypointsComponent.h"
#include "EMFXFootstepComponent.h"
#include "StarterGameCVars.h"

#ifdef STARTER_GAME_EDITOR
#include "EditorWaypointsComponent.h"

#include "PlayFromHereEditorSystemComponent.h"
#endif


namespace StarterGameGem
{

	StarterGameGemModule::StarterGameGemModule()
		: CryHooksModule()
	{
		// Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
		m_descriptors.insert(m_descriptors.end(), {
			StarterGameGemSystemComponent::CreateDescriptor(),

            AISoundManagerSystemComponent::CreateDescriptor(),
            PersistentDataSystemComponent::CreateDescriptor(),
            StarterGameNavigationComponent::CreateDescriptor(),
            VisualiseAIStatesSystemComponent::CreateDescriptor(),
            VisualisePathSystemComponent::CreateDescriptor(),

            AudioMultiListenerComponent::CreateDescriptor(),
            CameraSettingsComponent::CreateDescriptor(),
            ConsoleVarListenerComponent::CreateDescriptor(),
            CutscenePlayerComponent::CreateDescriptor(),
            DebugManagerComponent::CreateDescriptor(),
            DecalSelectorComponent::CreateDescriptor(),
            LineRendererComponent::CreateDescriptor(),
            ParticleManagerComponent::CreateDescriptor(),
            PersistentDataSystemComparitorComponent::CreateDescriptor(),
            PersistentDataSystemManipulatorComponent::CreateDescriptor(),
            PlayFromHereComponent::CreateDescriptor(),
			StatComponent::CreateDescriptor(),
            VisualiseRangeComponent::CreateDescriptor(),
            WaypointSettingsComponent::CreateDescriptor(),
            WaypointsComponent::CreateDescriptor(),
			EMFXFootstepComponent::CreateDescriptor(),

#ifdef STARTER_GAME_EDITOR
			EditorWaypointsComponent::CreateDescriptor(),

			PlayFromHereEditorSystemComponent::CreateDescriptor(),
#endif
        });
	}

	void StarterGameGemModule::OnSystemEvent(ESystemEvent e, UINT_PTR wparam, UINT_PTR lparam)
	{
		switch(e)
		{
			case ESYSTEM_EVENT_GAME_POST_INIT:
				PostSystemInit();
				break;

			case ESYSTEM_EVENT_FULL_SHUTDOWN:
			case ESYSTEM_EVENT_FAST_SHUTDOWN:
				Shutdown();
				break;
		}
	}

	void StarterGameGemModule::PostSystemInit()
	{
		StarterGameCVars::GetInstance();
	}

	void StarterGameGemModule::Shutdown()
	{
		StarterGameCVars::DeregisterCVars();
	}

	AZ::ComponentTypeList StarterGameGemModule::GetRequiredSystemComponents() const
	{
		return AZ::ComponentTypeList{
			azrtti_typeid<StarterGameGemSystemComponent>(),

            azrtti_typeid<AISoundManagerSystemComponent>(),
            azrtti_typeid<PersistentDataSystemComponent>(),
            azrtti_typeid<VisualiseAIStatesSystemComponent>(),
            azrtti_typeid<VisualisePathSystemComponent>(),

#ifdef STARTER_GAME_EDITOR
            azrtti_typeid<PlayFromHereEditorSystemComponent>(),
#endif
		};
	}

}
