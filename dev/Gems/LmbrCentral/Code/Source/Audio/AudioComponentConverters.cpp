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

#include "LmbrCentral_precompiled.h"

// Game Audio Components
#include "AudioEnvironmentComponent.h"
#include "AudioRtpcComponent.h"
#include "AudioSwitchComponent.h"
#include "AudioTriggerComponent.h"


#if defined(LMBR_CENTRAL_EDITOR)

// Editor Audio Components
#include "EditorAudioEnvironmentComponent.h"
#include "EditorAudioRtpcComponent.h"
#include "EditorAudioSwitchComponent.h"
#include "EditorAudioTriggerComponent.h"

#include <AzCore/RTTI/RTTI.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAudioCtrlTypes.h>

#endif // LMBR_CENTRAL_EDITOR

#include <AzCore/Serialization/SerializeContext.h>


namespace LmbrCentral
{
    namespace ClassConverters
    {
        //=========================================================================
        // AudioEnvironmentComponent
        //=========================================================================

        bool ConvertOldAudioEnvironmentComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            AZStd::string oldEnvironmentName;

            if (!classElement.GetChildData(AZ::Crc32("Environment name"), oldEnvironmentName))
            {
                AZ_Error("Serialization", false, "Failed to retrieve old Environment name.");
                return false;
            }

            bool convertResult = false;
        #if defined(LMBR_CENTRAL_EDITOR)
            if (classElement.GetName() == AZ::Crc32("m_template"))
            {
                if (convertResult = classElement.Convert<EditorAudioEnvironmentComponent>(context))
                {
                    AzToolsFramework::CReflectedVarAudioControl newEnvironment;
                    newEnvironment.m_propertyType = AzToolsFramework::AudioPropertyType::Environment;
                    newEnvironment.m_controlName = oldEnvironmentName;
                    if (classElement.AddElementWithData(context, "Environment name", newEnvironment) == -1)
                    {
                        AZ_Error("Serialization", false, "Failed to replace Environment name with new version.");
                        return false;
                    }
                }
            }
            else
        #endif // LMBR_CENTRAL_EDITOR
            {
                if (convertResult = classElement.Convert<AudioEnvironmentComponent>(context))
                {
                    if (classElement.AddElementWithData(context, "Environment name", oldEnvironmentName) == -1)
                    {
                        AZ_Error("Serialization", false, "Failed to replace Environment name with new version.");
                        return false;
                    }
                }
            }

            return convertResult;
        }



        //=========================================================================
        // AudioRtpcComponent
        //=========================================================================

        bool ConvertOldAudioRtpcComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            AZStd::string oldRtpcName;
            if (!classElement.GetChildData(AZ::Crc32("Rtpc Name"), oldRtpcName))
            {
                AZ_Error("Serialization", false, "Failed to retrieve old Rtpc name.");
                return false;
            }

            bool convertResult = false;
        #if defined(LMBR_CENTRAL_EDITOR)
            if (classElement.GetName() == AZ::Crc32("m_template"))
            {
                if (convertResult = classElement.Convert<EditorAudioRtpcComponent>(context))
                {
                    AzToolsFramework::CReflectedVarAudioControl newRtpc;
                    newRtpc.m_propertyType = AzToolsFramework::AudioPropertyType::Rtpc;
                    newRtpc.m_controlName = oldRtpcName;
                    if (classElement.AddElementWithData(context, "Rtpc Name", newRtpc) == -1)
                    {
                        AZ_Error("Serialization", false, "Failed to replace Rtpc name with new version.");
                        return false;
                    }
                }
            }
            else
        #endif // LMBR_CENTRAL_EDITOR
            {
                if (convertResult = classElement.Convert<AudioRtpcComponent>(context))
                {
                    if (classElement.AddElementWithData(context, "Rtpc Name", oldRtpcName) == -1)
                    {
                        AZ_Error("Serialization", false, "Failed to replace Rtpc name with new version.");
                        return false;
                    }
                }
            }

            return convertResult;
        }



        //=========================================================================
        // AudioSwitchComponent
        //=========================================================================

        bool ConvertOldAudioSwitchComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            AZStd::string oldSwitchName;
            AZStd::string oldStateName;

            if (!classElement.GetChildData(AZ::Crc32("Switch name"), oldSwitchName))
            {
                AZ_Error("Serialization", false, "Failed to retrieve old Switch name.");
                return false;
            }

            if (!classElement.GetChildData(AZ::Crc32("State name"), oldStateName))
            {
                AZ_Error("Serialization", false, "Failed to retrieve old State name.");
                return false;
            }

            bool convertResult = false;
        #if defined(LMBR_CENTRAL_EDITOR)
            if (classElement.GetName() == AZ::Crc32("m_template"))
            {
                if (convertResult = classElement.Convert<EditorAudioSwitchComponent>(context))
                {
                    AzToolsFramework::CReflectedVarAudioControl newSwitch;
                    newSwitch.m_propertyType = AzToolsFramework::AudioPropertyType::Switch;
                    newSwitch.m_controlName = oldSwitchName;
                    if (classElement.AddElementWithData(context, "Switch name", newSwitch) == -1)
                    {
                        AZ_Error("Serialization", false, "Failed to replace Switch name with new version.");
                        return false;
                    }

                    AzToolsFramework::CReflectedVarAudioControl newState;
                    newState.m_propertyType = AzToolsFramework::AudioPropertyType::SwitchState;
                    newState.m_controlName = oldStateName;
                    if (classElement.AddElementWithData(context, "State name", newState) == -1)
                    {
                        AZ_Error("Serialization", false, "Failed to replace State name with new version.");
                        return false;
                    }
                }
            }
            else
        #endif // LMBR_CENTRAL_EDITOR
            {
                if (convertResult = classElement.Convert<AudioSwitchComponent>(context))
                {
                    if (classElement.AddElementWithData(context, "Switch name", oldSwitchName) == -1)
                    {
                        AZ_Error("Serialization", false, "Failed to replace Switch name with new version.");
                        return false;
                    }

                    if (classElement.AddElementWithData(context, "State name", oldStateName) == -1)
                    {
                        AZ_Error("Serialization", false, "Failed to replace State name with new version.");
                        return false;
                    }
                }
            }

            return convertResult;
        }



        //=========================================================================
        // AudioComponent / AudioTriggerComponent
        //=========================================================================

        void ExtractOldFields(AZ::SerializeContext::DataElementNode& classElement, AZStd::string& playTrigger, AZStd::string& stopTrigger)
        {
            classElement.GetChildData(AZ::Crc32("Play Trigger"), playTrigger);
            classElement.GetChildData(AZ::Crc32("Stop Trigger"), stopTrigger);
        }

        void RestoreOldFields(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement, AZStd::string& playTrigger, AZStd::string& stopTrigger)
        {
            const int playTriggerIndex = classElement.AddElementWithData(context, "Play Trigger", playTrigger);
            (void)playTriggerIndex;
            AZ_Assert(playTriggerIndex >= 0, "Failed to add 'Play Trigger' element.");
            const int stopTriggerIndex = classElement.AddElementWithData(context, "Stop Trigger", stopTrigger);
            (void)stopTriggerIndex;
            AZ_Assert(stopTriggerIndex >= 0, "Failed to add 'Stop Trigger' element.");
        }

        bool ConvertOldEditorAudioComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
        #if defined(LMBR_CENTRAL_EDITOR)
            AZStd::string playTrigger, stopTrigger;
            ExtractOldFields(classElement, playTrigger, stopTrigger);

            // Convert to generic component wrapper. This is necessary since the old audio component
            // had an editor counterpart. The new one does not, so it needs to be wrapped.
            const bool result = classElement.Convert(context, azrtti_typeid<AzToolsFramework::Components::GenericComponentWrapper>());
            if (result)
            {
                int triggerComponentIndex = classElement.AddElement<AudioTriggerComponent>(context, "m_template");

                if (triggerComponentIndex >= 0)
                {
                    auto& triggerComponentElement = classElement.GetSubElement(triggerComponentIndex);
                    RestoreOldFields(context, triggerComponentElement, playTrigger, stopTrigger);

                    return true;
                }
            }
        #endif // LMBR_CENTRAL_EDITOR

            return false;
        }

        bool ConvertOldAudioComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            AZStd::string playTrigger, stopTrigger;
            ExtractOldFields(classElement, playTrigger, stopTrigger);

            // Convert from old audio component to audio trigger.
            const bool result = classElement.Convert<AudioTriggerComponent>(context);

            if (result)
            {
                RestoreOldFields(context, classElement, playTrigger, stopTrigger);
            }

            return result;
        }

        bool ConvertOldAudioTriggerComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            AZStd::string oldPlayTriggerName;
            AZStd::string oldStopTriggerName;
            bool oldPlaysImmediately;

            if (!classElement.GetChildData(AZ::Crc32("Play Trigger"), oldPlayTriggerName))
            {
                AZ_Error("Serialization", false, "Failed to retrieve old Play Trigger name.");
                return false;
            }

            if (!classElement.GetChildData(AZ::Crc32("Stop Trigger"), oldStopTriggerName))
            {
                AZ_Error("Serialization", false, "Failed to retrieve old Stop Trigger name.");
                return false;
            }

            if (!classElement.GetChildData(AZ::Crc32("Plays Immediately"), oldPlaysImmediately))
            {
                AZ_Error("Serialization", false, "Failed to retrieve old Play setting.");
            }

            bool convertResult = false;
        #if defined(LMBR_CENTRAL_EDITOR)
            if (classElement.GetName() == AZ::Crc32("m_template"))
            {
                if (convertResult = classElement.Convert<EditorAudioTriggerComponent>(context))
                {
                    AzToolsFramework::CReflectedVarAudioControl newPlayTrigger;
                    newPlayTrigger.m_propertyType = AzToolsFramework::AudioPropertyType::Trigger;
                    newPlayTrigger.m_controlName = oldPlayTriggerName;
                    if (classElement.AddElementWithData(context, "Play Trigger", newPlayTrigger) == -1)
                    {
                        AZ_Error("Serialization", false, "Failed to replace Play Trigger name with new version.");
                        return false;
                    }

                    AzToolsFramework::CReflectedVarAudioControl newStopTrigger;
                    newStopTrigger.m_propertyType = AzToolsFramework::AudioPropertyType::Trigger;
                    newStopTrigger.m_controlName = oldStopTriggerName;
                    if (classElement.AddElementWithData(context, "Stop Trigger", newStopTrigger) == -1)
                    {
                        AZ_Error("Serialization", false, "Failed to replace Stop Trigger name with new version.");
                        return false;
                    }

                    if (classElement.AddElementWithData(context, "Plays Immediately", oldPlaysImmediately) == -1)
                    {
                        AZ_Error("Serialization", false, "Failed to replace Play setting.");
                        return false;
                    }
                }
            }
            else
        #endif // LMBR_CENTRAL_EDITOR
            {
                if (convertResult = classElement.Convert<AudioTriggerComponent>(context))
                {
                    if (classElement.AddElementWithData(context, "Play Trigger", oldPlayTriggerName) == -1)
                    {
                        AZ_Error("Serialization", false, "Failed to replace Play Trigger name with new version.");
                        return false;
                    }

                    if (classElement.AddElementWithData(context, "Stop Trigger", oldStopTriggerName) == -1)
                    {
                        AZ_Error("Serialization", false, "Failed to replace Stop Trigger name with new version.");
                        return false;
                    }

                    if (classElement.AddElementWithData(context, "Plays Immediately", oldPlaysImmediately) == -1)
                    {
                        AZ_Error("Serialization", false, "Failed to replace Play setting.");
                        return false;
                    }
                }
            }

            return convertResult;
        }

    } // namespace ClassConverters

} // namespace LmbrCentral
