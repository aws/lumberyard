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

#pragma once

#include <ATLEntityData.h>
#include <IAudioInterfacesCommonData.h>
#include <IXml.h>

#include <AzCore/Component/Component.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title EAudioRequestStatus>
    // Summary:
    //      An enum that lists possible statuses of an AudioRequest.
    //      Used as a return type for many function used by the AudioSystem internally,
    //      and also for most of the IAudioSystemImplementation calls.
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioRequestStatus : TATLEnumFlagsType
    {
        eARS_NONE                           = 0,
        eARS_SUCCESS                        = 1,
        eARS_PARTIAL_SUCCESS                = 2,
        eARS_FAILURE                        = 3,
        eARS_PENDING                        = 4,
        eARS_FAILURE_INVALID_OBJECT_ID      = 5,
        eARS_FAILURE_INVALID_CONTROL_ID     = 6,
        eARS_FAILURE_INVALID_REQUEST        = 7,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title BoolToARS>
    // Summary:
    //      A utility function to convert a boolean value to an EAudioRequestStatus.
    // Arguments:
    //      bResult - a boolean value.
    // Returns:
    //      eARS_SUCCESS if bResult is true, eARS_FAILURE if bResult is false.
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    inline EAudioRequestStatus BoolToARS(bool bResult)
    {
        return bResult ? eARS_SUCCESS : eARS_FAILURE;
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioSystemImplementationNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioSystemImplementationNotifications() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides - Single Bus, Multiple Handlers
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        ///////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title OnAudioSystemLoseFocus>
        // Summary:
        //      This method is called every time the main Game (or Editor) window loses focus.
        // Returns:
        //      void
        // See Also:
        //      OnAudioSystemGetFocus
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void OnAudioSystemLoseFocus() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title OnAudioSystemGetFocus>
        // Summary:
        //      This method is called every time the main Game (or Editor) window gets focus.
        // Returns:
        //      void
        // See Also:
        //      OnAudioSystemLoseFocus
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void OnAudioSystemGetFocus() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title OnAudioSystemMuteAll>
        // Summary:
        //      Mute all sounds, after this call there should be no audio coming from the audio
        //      middleware.
        // Returns:
        //      void
        // See Also:
        //      OnAudioSystemUnmuteAll, StopAllSounds
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void OnAudioSystemMuteAll() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title OnAudioSystemUnmuteAll>
        // Summary:
        //      Restore the audio output of the audio middleware after a call to MuteAll().
        // Returns:
        //      void
        // See Also:
        //      OnAudioSystemMuteAll
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void OnAudioSystemUnmuteAll() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title OnAudioSystemRefresh>
        // Summary:
        //      Perform a "hot restart" of the audio middleware. Reset all of the internal data.
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void OnAudioSystemRefresh() = 0;
    };

    using AudioSystemImplementationNotificationBus = AZ::EBus<AudioSystemImplementationNotifications>;


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioSystemImplementationRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioSystemImplementationRequests() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides - Single Bus, Single Handler
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        ///////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title Update>
        // Summary:
        //      Update all of the internal components that require regular updates, update the audio
        //      middleware state.
        // Arguments:
        //      fUpdateIntervalMS - Time since the last call to Update in milliseconds.
        // Returns:
        //      void
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void Update(const float fUpdateIntervalMS) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title Initialize>
        // Summary:
        //      Initialize all internal components and the audio middleware.
        // Returns:
        //      eARS_SUCCESS if the initialization was successful, eARS_FAILURE otherwise.
        // See Also:
        //      ShutDown
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus Initialize() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title ShutDown>
        // Summary:
        //      Shuts down all of the internal components and the audio middleware.
        //      Note: After calling ShutDown(), the system can still be reinitialized by calling Init().
        // Returns:
        //      eARS_SUCCESS if the shutdown was successful, eARS_FAILURE otherwise.
        // See Also:
        //      Release, Initialize
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus ShutDown() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title Release>
        // Summary:
        //      Frees all of the resources used by the class and destroys the instance. This action is
        //      not reversible.
        // Returns:
        //      eARS_SUCCESS if the action was successful, eARS_FAILURE otherwise.
        // See Also:
        //      ShutDown, Initialize
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus Release() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title StopAllSounds>
        // Summary:
        //      Stop all currently playing sounds. Has no effect on anything triggered after this
        //      method is called.
        // Returns:
        //      eARS_SUCCESS if the action was successful, eARS_FAILURE otherwise.
        // See Also:
        //      OnAudioSystemMuteAll
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus StopAllSounds() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title RegisterAudioObject>
        // Summary:
        //      Register an audio object with the audio middleware. An object needs to be registered
        //      for one to set position, execute triggers on it, or set Rtpcs and switches.
        //      This version of the method is meant be used in debug builds.
        // Arguments:
        //      pObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject being registered.
        //      sObjectName - the AudioObject name shown in debug info.
        // Returns:
        //      eARS_SUCCESS if the object has been registered, eARS_FAILURE if the registration
        //      failed.
        // See Also:
        //      UnregisterAudioObject, ResetAudioObject
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus RegisterAudioObject(IATLAudioObjectData* const pObjectData, const char* const sObjectName = nullptr) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title UnregisterAudioObject>
        // Summary:
        //      Unregister an audio object with the audio middleware. After this action executing
        //      triggers, setting position, states or rtpcs should have no effect on the object
        //      referenced by pObjectData.
        // Arguments:
        //      pObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject being unregistered.
        // Returns:
        //      eARS_SUCCESS if the object has been unregistered, eARS_FAILURE if the un-registration
        //      failed.
        // See Also:
        //      RegisterAudioObject, ResetAudioObject
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus UnregisterAudioObject(IATLAudioObjectData* const pObjectData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title ResetAudioObject>
        // Summary:
        //      Clear out the object data and resets it so that the object can be returned to the pool
        //      of available Audio Objects for reuse.
        // Arguments:
        //      pObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject being reset.
        // Returns:
        //      eARS_SUCCESS if the object has been reset, eARS_FAILURE otherwise.
        // See Also:
        //      RegisterAudioObject, UnregisterAudioObject
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus ResetAudioObject(IATLAudioObjectData* const pObjectData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title UpdateAudioObject>
        // Summary:
        //      Performs actions that need to be executed regularly on an AudioObject.
        // Arguments:
        //      pObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject being updated.
        // Returns:
        //      eARS_SUCCESS if the object has been reset, eARS_FAILURE otherwise.
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus UpdateAudioObject(IATLAudioObjectData* const pObjectData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title PrepareTriggerSync>
        // Summary:
        //      Load the metadata and media needed by the audio middleware to execute the
        //      ATLTriggerImpl described by pTriggerData.  Preparing Triggers manually is only
        //      necessary if their data have not been loaded via PreloadRequests.
        // Arguments:
        //      pAudioObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject on which the
        //              ATLTriggerImpl is prepared.
        //      pTriggerData - implementation-specific data for the ATLTriggerImpl being prepared.
        // Returns:
        //      eARS_SUCCESS if the the data was successfully loaded, eARS_FAILURE otherwise.
        // See Also:
        //      UnprepareTriggerSync, PrepareTriggerAsync, UnprepareTriggerAsync
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus PrepareTriggerSync(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLTriggerImplData* const pTriggerData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title UnprepareTriggerSync>
        // Summary:
        //      Release the metadata and media needed by the audio middleware to execute the
        //      ATLTriggerImpl described by pTriggerData.  Un-preparing Triggers manually is only
        //      necessary if their data are not managed via PreloadRequests.
        // Arguments:
        //      pAudioObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject on which the
        //              ATLTriggerImpl is un-prepared.
        //      pTriggerData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to handle the ATLTriggerImpl being unprepared.
        // Returns:
        //      eARS_SUCCESS if the the data was successfully unloaded, eARS_FAILURE otherwise.
        // See Also:
        //      PrepareTriggerSync, PrepareTriggerAsync, UnprepareTriggerAsync
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus UnprepareTriggerSync(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLTriggerImplData* const pTriggerData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title PrepareTriggerAsync>
        // Summary:
        //      Load the metadata and media needed by the audio middleware to execute the
        //      ATLTriggerImpl described by pTriggerData asynchronously. An active event that
        //      references pEventData is created on the corresponding AudioObject.
        //      The callback called once the loading is done must report that this event is finished.
        //      Preparing Triggers manually is only necessary if their data have not been loaded via
        //      PreloadRequests.
        // Arguments:
        //      pAudioObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject on which the
        //              ATLTriggerImpl is prepared.
        //      pTriggerData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to handle the ATLTriggerImpl being prepared.
        //      pEventData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the ATLEvent corresponding to the
        //              loading process started.
        // Returns:
        //      eARS_SUCCESS if the the request was successfully sent to the audio middleware,
        //      eARS_FAILURE otherwise.
        // See Also:
        //      UnprepareTriggerAsync, PrepareTriggerSync, UnprepareTriggerSync
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus PrepareTriggerAsync(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLTriggerImplData* const pTriggerData,
            IATLEventData* const pEventData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title UnprepareTriggerAsync>
        // Summary:
        //      Unload the metadata and media needed by the audio middleware to execute the
        //      ATLTriggerImpl described by pTriggerData asynchronously. An active event that
        //      references pEventData is created on the corresponding AudioObject.
        //      The callback called once the unloading is done must report that this event is finished.
        //      Un-preparing Triggers manually is only necessary if their data are not managed via
        //      PreloadRequests.
        // Arguments:
        //      pAudioObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject on which the
        //              ATLTriggerImpl is un-prepared.
        //      pTriggerData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to handle the ATLTriggerImpl.
        //      pEventData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the ATLEvent corresponding to the
        //              unloading process started.
        // Returns:
        //      eARS_SUCCESS if the the request was successfully sent to the audio middleware,
        //      eARS_FAILURE otherwise.
        // See Also:
        //      PrepareTriggerAsync, PrepareTriggerSync, UnprepareTriggerSync
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus UnprepareTriggerAsync(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLTriggerImplData* const pTriggerData,
            IATLEventData* const pEventData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title ActivateTrigger>
        // Summary:
        //      Activate an audio-middleware-specific ATLTriggerImpl.
        // Arguments:
        //      pAudioObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject on which the
        //              ATLTriggerImpl should be executed.
        //      pTriggerData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to handle the ATLTriggerImpl.
        //      pEventData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the ATLEvent corresponding to the
        //              process started by the activation of ATLTriggerImpl.
        // Returns:
        //      eARS_SUCCESS if the ATLTriggerImpl has been successfully activated by the audio
        //      middleware, eARS_FAILURE otherwise.
        // See Also:
        //      SetRtpc, SetSwitchState, StopEvent
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus ActivateTrigger(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLTriggerImplData* const pTriggerData,
            IATLEventData* const pEventData,
            const SATLSourceData* const pSourceData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title StopEvent>
        // Summary:
        //      Stop an ATLEvent active on an AudioObject.
        // Arguments:
        //      pAudioObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject on which the
        //              specified ATLEvent is active.
        //      pEventData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to identify the ATLEvent that needs to be
        //              stopped.
        // Returns:
        //      eARS_SUCCESS if the ATLEvent has been successfully stopped, eARS_FAILURE otherwise.
        // See Also:
        //      StopAllEvents, PrepareTriggerAsync, UnprepareTriggerAsync, ActivateTrigger
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus StopEvent(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLEventData* const pEventData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title StopAllEvents>
        // Summary:
        //      Stop all ATLEvents currently active on an AudioObject.
        // Arguments:
        //      pAudioObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject on which the
        //              ATLEvent should be stopped.
        // Returns:
        //      eARS_SUCCESS if all of the ATLEvents have been successfully stopped, eARS_FAILURE
        //      otherwise.
        // See Also:
        //      StopEvent, PrepareTriggerAsync, UnprepareTriggerAsync, ActivateTrigger
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus StopAllEvents(
            IATLAudioObjectData* const pAudioObjectData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title SetPosition>
        // Summary:
        //      Set the world position of an AudioObject inside the audio middleware.
        // Arguments:
        //      pAudioObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject whose position is
        //              being set.
        //      oWorldPosition - a struct containing all of the geometric information about the
        //              AudioObject necessary to correctly play the audio produced on it.
        // Returns:
        //      eARS_SUCCESS if the AudioObject's position has been successfully set, eARS_FAILURE
        //      otherwise.
        // See Also:
        //      StopEvent, PrepareTriggerAsync, UnprepareTriggerAsync, ActivateTrigger
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus SetPosition(
            IATLAudioObjectData* const pAudioObjectData,
            const SATLWorldPosition& oWorldPosition) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title SetMultiplePositions>
        // Summary:
        //      Sets multiple world positions of an AudioObject inside the audio middleware.
        // Arguments:
        //      pAudioObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject whose position is
        //              being set.
        //      multiPositions - a vector of world positions and other parameters as struct.
        // Returns:
        //      eARS_SUCCESS if the AudioObject's position has been successfully set, eARS_FAILURE
        //      otherwise.
        // See Also:
        //      SetPosition
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus SetMultiplePositions(
            IATLAudioObjectData* const pAudioObjectData,
            const MultiPositionParams& multiPositions) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title SetRtpc>
        // Summary:
        //      Set the ATLRtpcImpl to the specified value on the provided AudioObject.
        // Arguments:
        //      pAudioObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject on which the
        //              ATLRtpcImpl is being set.
        //      pRtpcData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to handle the ATLRtpcImpl which is being set.
        //      fValue - the value to be set.
        // Returns:
        //      eARS_SUCCESS if the provided value has been successfully set on the passed ATLRtpcImpl,
        //      eARS_FAILURE otherwise.
        // See Also:
        //      ActivateTrigger, SetSwitchState, SetEnvironment, SetPosition
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus SetRtpc(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLRtpcImplData* const pRtpcData,
            const float fValue) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title SetSwitchState>
        // Summary:
        //      Set the ATLSwitchStateImpl on the given AudioObject.
        // Arguments:
        //      pAudioObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject on which the
        //              ATLSwitchStateImpl is being set.
        //      pSwitchStateData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to handle the ATLSwitchStateImpl which is being
        //              set.
        // Returns:
        //      eARS_SUCCESS if the provided ATLSwitchStateImpl has been successfully set, eARS_FAILURE
        //      otherwise.
        // See Also:
        //      ActivateTrigger, SetRtpc, SetEnvironment, SetPosition
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus SetSwitchState(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLSwitchStateImplData* const pSwitchStateData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title SetObstructionOcclusion>
        // Summary:
        //      Set the provided Obstruction and Occlusion values on the given AudioObject.
        // Arguments:
        //      pAudioObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject on which the
        //              Obstruction and Occlusion is being set.
        //      fObstruction - the obstruction value to be set; it describes how much the direct sound
        //              path from the AudioObject to the Listener is obstructed.
        //      fOcclusion - the occlusion value to be set; it describes how much all sound paths
        //              (direct and indirect) are obstructed.
        // Returns:
        //      eARS_SUCCESS if the provided values have been successfully set, eARS_FAILURE otherwise.
        // See Also:
        //      SetEnvironment
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus SetObstructionOcclusion(
            IATLAudioObjectData* const pAudioObjectData,
            const float fObstruction,
            const float fOcclusion) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title SetEnvironment>
        // Summary:
        //      Set the provided value for the specified ATLEnvironmentImpl on the given AudioObject.
        // Arguments:
        //      pAudioObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject on which the
        //              ATLEnvironmentImpl value is being set.
        //      pEnvironmentImplData - implementation-specific data needed by the audio middleware and
        //              the AudioSystemImplementation code to identify and use the ATLEnvironmentImpl
        //              being set.
        //      fValue - the fade value for the provided ATLEnvironmentImpl, 0.0f means no effect at
        //              all, 1.0f corresponds to the full effect.
        // Returns:
        //      eARS_SUCCESS if the provided value has been successfully set, eARS_FAILURE otherwise.
        // See Also:
        //      SetObstructionOcclusion
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus SetEnvironment(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLEnvironmentImplData* const pEnvironmentImplData,
            const float fAmount) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title SetListenerPosition>
        // Summary:
        //      Set the world position of an AudioListener inside the audio middleware.
        // Arguments:
        //      pListenerData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioListener whose position is
        //              being set.
        //      oWorldPosition - a struct containing the necessary geometric information about the
        //              AudioListener position.
        // Returns:
        //      eARS_SUCCESS if the AudioListener's position has been successfully set, eARS_FAILURE
        //      otherwise.
        // See Also:
        //      SetPosition
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus SetListenerPosition(
            IATLListenerData* const pListenerData,
            const SATLWorldPosition& oNewPosition) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title ResetRtpc>
        // Summary:
        //      Resets the ATLRtpcImpl to the default value on the provided AudioObject.
        // Arguments:
        //      pAudioObjectData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to manage the AudioObject on which the
        //              ATLRtpcImpl is being reset.
        //      pRtpcData - implementation-specific data needed by the audio middleware and the
        //              AudioSystemImplementation code to handle the ATLRtpcImpl which is being reset.
        // Returns:
        //      eARS_SUCCESS if the provided value has been successfully reset on the passed ATLRtpcImpl,
        //      eARS_FAILURE otherwise.
        // See Also:
        //      SetRtpc
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus ResetRtpc(
            IATLAudioObjectData* const pAudioObjectData,
            const IATLRtpcImplData* const pRtpcData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title RegisterInMemoryFile>
        // Summary:
        //      Inform the audio middleware about the memory location of a preloaded audio-data file.
        // Arguments:
        //      pAudioFileEntry - ATL-specific information describing the resources used by the
        //              preloaded file being reported.
        // Returns:
        //      eARS_SUCCESS if the audio middleware is able to use the preloaded file, eARS_FAILURE
        //      otherwise.
        // See Also:
        //      UnregisterInMemoryFile
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus RegisterInMemoryFile(SATLAudioFileEntryInfo* const pAudioFileEntry) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title UnregisterInMemoryFile>
        // Summary:
        //      Inform the audio middleware that the memory containing the preloaded audio-data file
        //      should no longer be used.
        // Arguments:
        //      pAudioFileEntry - ATL-specific information describing the resources used by the
        //              preloaded file being invalidated.
        // Returns:
        //      eARS_SUCCESS if the audio middleware was able to unregister the preloaded file
        //      supplied, eARS_FAILURE otherwise.
        // See Also:
        //      RegisterInMemoryFile
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus UnregisterInMemoryFile(SATLAudioFileEntryInfo* const pAudioFileEntry) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title ParseAudioFileEntry>
        // Summary:
        //      Parse the implementation-specific XML node that represents an AudioFileEntry, fill the
        //      fields of the struct referenced by pFileEntryInfo with the data necessary to correctly
        //      access and store the file's contents in memory.
        //      Create an object implementing IATLAudioFileEntryData to hold implementation-specific
        //      data about the file and store a pointer to it in a member of pFileEntryInfo.
        // Arguments:
        //      pAudioFileEntryNode - an XML node containing the necessary information about the file.
        //      pFileEntryInfo - a pointer to the struct containing the data used by the ATL to load
        //              the file into memory.
        // Returns:
        //      eARS_SUCCESS if the XML node was parsed successfully, eARS_FAILURE otherwise.
        // See Also:
        //      DeleteAudioFileEntryData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual EAudioRequestStatus ParseAudioFileEntry(
            const XmlNodeRef pAudioFileEntryNode,
            SATLAudioFileEntryInfo* const pFileEntryInfo) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title DeleteAudioFileEntryData>
        // Summary:
        //      Free the memory and potentially other resources used by the supplied
        //      IATLAudioFileEntryData instance.
        //      Normally, an IATLAudioFileEntryData instance is created by ParseAudioFileEntry() and a
        //      pointer is stored in a member of SATLAudioFileEntryInfo.
        // Arguments:
        //      pOldAudioFileEntryData - pointer to the object implementing IATLAudioFileEntryData to
        //              be discarded.
        // See Also:
        //      ParseAudioFileEntry
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void DeleteAudioFileEntryData(IATLAudioFileEntryData* const pOldAudioFileEntryData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title GetAudioFileLocation>
        // Summary:
        //      Get the full path to the folder containing the file described by the pFileEntryInfo.
        // Arguments:
        //      pFileEntryInfo - ATL-specific information describing the file whose location is being
        //              queried.
        // Returns:
        //      A C-string containing the path to the folder where the file corresponding to the
        //      pFileEntryInfo is stored.
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual const char* const GetAudioFileLocation(SATLAudioFileEntryInfo* const pFileEntryInfo) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title NewAudioTriggerImplData>
        // Summary:
        //      Parse the implementation-specific XML node that represents an ATLTriggerImpl, return a
        //      pointer to the data needed for identifying and using this ATLTriggerImpl instance
        //      inside the AudioSystemImplementation.
        // Arguments:
        //      pAudioTriggerNode - an XML node corresponding to the new ATLTriggerImpl to be created.
        // Returns:
        //      IATLRtpcImplData const* pointer to the audio implementation-specific data needed by the
        //      audio middleware and the AudioSystemImplementation code to use the corresponding
        //      ATLTriggerImpl; NULL if the new AudioTriggerImplData instance was not created.
        // See Also:
        //      DeleteAudioTriggerImplData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual const IATLTriggerImplData* NewAudioTriggerImplData(const XmlNodeRef pAudioTriggerNode) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title DeleteAudioTriggerImplData>
        // Summary:
        //      Free the memory and potentially other resources used by the supplied
        //      IATLTriggerImplData instance.
        // Arguments:
        //      pOldTriggerImplData - pointer to the object implementing IATLTriggerImplData to be
        //              discarded.
        // See Also:
        //      NewAudioTriggerImplData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void DeleteAudioTriggerImplData(const IATLTriggerImplData* const pOldTriggerImplData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title NewAudioRtpcImplData>
        // Summary:
        //      Parse the implementation-specific XML node that represents an ATLRtpcImpl, return a
        //      pointer to the data needed for identifying and using this ATLRtpcImpl instance inside
        //      the AudioSystemImplementation.
        // Arguments:
        //      pAudioRtpcNode - an XML node corresponding to the new ATLRtpcImpl to be created.
        // Returns:
        //      IATLRtpcImplData const* pointer to the audio implementation-specific data needed by the
        //      audio middleware and the AudioSystemImplementation code to use the corresponding
        //      ATLRtpcImpl; NULL if the new AudioTriggerImplData instance was not created.
        // See Also:
        //      DeleteAudioRtpcImplData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual const IATLRtpcImplData* NewAudioRtpcImplData(const XmlNodeRef pAudioRtpcNode) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title DeleteAudioRtpcImplData>
        // Syntax:
        //      DeleteAudioRtpcImplData(pOldRtpcImplData)
        // Summary:
        //      Free the memory and potentially other resources used by the supplied IATLRtpcImplData
        //      instance.
        // Arguments:
        //      pOldRtpcImplData - pointer to the object implementing IATLRtpcImplData to be discarded.
        // See Also:
        //      NewAudioRtpcImplData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void DeleteAudioRtpcImplData(const IATLRtpcImplData* const pOldRtpcImplData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title NewAudioSwitchStateImplData>
        // Summary:
        //      Parse the implementation-specific XML node that represents an ATLSwitchStateImpl,
        //      return a pointer to the data needed for identifying and using this ATLSwitchStateImpl
        //      instance inside the AudioSystemImplementation.
        // Arguments:
        //      pAudioSwitchStateImplNode - an XML node corresponding to the new ATLSwitchStateImpl to
        //              be created.
        // Returns:
        //      IATLRtpcImplData const* pointer to the audio implementation-specific data needed by the
        //      audio middleware and the AudioSystemImplementation code to use the corresponding
        //      ATLSwitchStateImpl; NULL if the new AudioTriggerImplData instance was not created.
        // See Also:
        //      DeleteAudioSwitchStateImplData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual const IATLSwitchStateImplData* NewAudioSwitchStateImplData(const XmlNodeRef pAudioSwitchStateImplNode) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title DeleteAudioSwitchStateImplData>
        // Syntax:
        //      DeleteAudioSwitchStateImplData(pOldAudioSwitchStateImplNode)
        // Summary:
        //      Free the memory and potentially other resources used by the supplied
        //      IATLSwitchStateImplData instance.
        // Arguments:
        //      pOldAudioSwitchStateImplNode - pointer to the object implementing
        //              IATLSwitchStateImplData to be discarded.
        // See Also:
        //      NewAudioSwitchStateImplData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void DeleteAudioSwitchStateImplData(const IATLSwitchStateImplData* const pOldAudioSwitchStateImplNode) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title NewAudioEnvironmentImplData>
        // Summary:
        //      Parse the implementation-specific XML node that represents an ATLEnvironmentImpl,
        //      return a pointer to the data needed for identifying and using this ATLEnvironmentImpl
        //      instance inside the AudioSystemImplementation.
        // Arguments:
        //      pAudioEnvironmentNode - an XML node corresponding to the new ATLEnvironmentImpl to be
        //              created.
        // Returns:
        //      IATLEnvironmentImplData const* pointer to the audio implementation-specific data needed
        //      by the audio middleware and the AudioSystemImplementation code to use the corresponding
        //      ATLEnvironmentImpl; NULL if the new IATLEnvironmentImplData instance was not created.
        // See Also:
        //      DeleteAudioEnvironmentImplData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual const IATLEnvironmentImplData* NewAudioEnvironmentImplData(const XmlNodeRef pAudioEnvironmentNode) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title DeleteAudioEnvironmentImplData>
        // Syntax:
        //      DeleteAudioEnvironmentImplData(pOldEnvironmentImplData)
        // Summary:
        //      Free the memory and potentially other resources used by the supplied
        //      IATLEnvironmentImplData instance.
        // Arguments:
        //      pOldEnvironmentImplData - pointer to the object implementing IATLEnvironmentImplData
        //              to be discarded.
        // See Also:
        //      NewAudioEnvironmentImplData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void DeleteAudioEnvironmentImplData(const IATLEnvironmentImplData* const pOldEnvironmentImplData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title NewGlobalAudioObjectData>
        // Summary:
        //      Create an object implementing IATLAudioObjectData that stores all of the data needed
        //      by the AudioSystemImplementation to identify and use the GlobalAudioObject with the
        //      provided AudioObjectID.
        // Arguments:
        //      nObjectID - the AudioObjectID to be used for the new GlobalAudioObject
        // Returns:
        //      IATLAudioObjectData* pointer to the audio implementation-specific data needed by the
        //      audio middleware and the AudioSystemImplementation code to use the corresponding
        //      GlobalAudioObject; NULL if the new IATLAudioObjectData instance was not created.
        // See Also:
        //      DeleteAudioObjectData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual IATLAudioObjectData* NewGlobalAudioObjectData(const TAudioObjectID nObjectID) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title NewAudioObjectData>
        // Summary:
        //      Create an object implementing IATLAudioObjectData that stores all of the data needed
        //      by the AudioSystemImplementation to identify and use the AudioObject with the provided
        //      AudioObjectID. Return a pointer to that object.
        // Arguments:
        //      nObjectID - the AudioObjectID to be used for the new AudioObject.
        // Returns:
        //      IATLAudioObjectData* pointer to the audio implementation-specific data needed by the
        //      audio middleware and the AudioSystemImplementation code to use the corresponding
        //      GlobalAudioObject; NULL if the new IATLAudioObjectData instance was not created.
        // See Also:
        //      DeleteAudioObjectData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual IATLAudioObjectData* NewAudioObjectData(const TAudioObjectID nObjectID) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title DeleteAudioObjectData>
        // Summary:
        //      Free the memory and potentially other resources used by the supplied
        //      IATLAudioObjectData instance.
        // Arguments:
        //      pOldObjectData - pointer to the object implementing IATLAudioObjectData to be
        //          discarded.
        // See Also:
        //      NewAudioObjectData, NewGlobalAudioObjectData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void DeleteAudioObjectData(IATLAudioObjectData* const pOldObjectData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title NewDefaultAudioListenerObjectData>
        // Summary:
        //      Create an object implementing IATLListenerData that stores all of the data needed by
        //      the AudioSystemImplementation to identify and use the DefaultAudioListener. Return a
        //      pointer to that object.
        // Returns:
        //      IATLListenerData* pointer to the audio implementation-specific data needed by the audio
        //      middleware and the AudioSystemImplementation code to use the DefaultAudioListener;
        //      NULL if the new IATLListenerData instance was not created.
        // See Also:
        //      DeleteAudioListenerObjectData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual IATLListenerData* NewDefaultAudioListenerObjectData(const TATLIDType nObjectID) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title NewAudioListenerObjectData>
        // Summary:
        //      Create an object implementing IATLListenerData that stores all of the data needed by
        //      the AudioSystemImplementation to identify and use an AudioListener. Return a pointer
        //      to that object.
        // Arguments:
        //      nIndex - index of the created AudioListener in the array of audio listeners available
        //              in the audio middleware.
        // Returns:
        //      IATLListenerData* pointer to the audio implementation-specific data needed by the audio
        //      middleware and the AudioSystemImplementation code to use the corresponding
        //      AudioListener; NULL if the new IATLListenerData instance was not created.
        // See Also:
        //      DeleteAudioListenerObjectData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual IATLListenerData* NewAudioListenerObjectData(const TATLIDType nObjectID) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title DeleteAudioListenerObjectData>
        // Summary:
        //      Free the memory and potentially other resources used by the supplied IATLListenerData
        //      instance.
        // Arguments:
        //      pOldListenerData - pointer to the object implementing IATLListenerData to be discarded.
        // See Also:
        //      NewDefaultAudioListenerObjectData, NewAudioListenerObjectData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void DeleteAudioListenerObjectData(IATLListenerData* const pOldListenerData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title NewAudioEventData>
        // Summary:
        //      Create an object implementing IATLEventData that stores all of the data needed by the
        //      AudioSystemImplementation to identify and use an AudioEvent. Return a pointer to that
        //      object.
        // Arguments:
        //      nEventID - AudioEventID to be used for the newly created AudioEvent.
        // Returns:
        //      IATLEventData* pointer to the audio implementation-specific data needed by the audio
        //      middleware and the AudioSystemImplementation code to use the corresponding AudioEvent;
        //      NULL if the new IATLEventData instance was not created.
        // See Also:
        //      DeleteAudioEventData, ResetAudioEventData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual IATLEventData* NewAudioEventData(const TAudioEventID nEventID) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title DeleteAudioEventData>
        // Summary:
        //      Free the memory and potentially other resources used by the supplied IATLEventData
        //      instance.
        // Arguments:
        //      pOldEventData - pointer to the object implementing IATLEventData to be discarded.
        // See Also:
        //      NewAudioEventData, ResetAudioEventData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void DeleteAudioEventData(IATLEventData* const pOldEventData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title ResetAudioEventData>
        // Syntax:
        //      ResetAudioEventData(pEventData)
        // Summary:
        //      Reset all the members of an IATLEventData instance without releasing the memory, so
        //      that the instance can be returned to the pool to be reused.
        // Arguments:
        //      pEventData - pointer to the object implementing IATLEventData to be reset.
        // See Also:
        //      NewAudioEventData, DeleteAudioEventData
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void ResetAudioEventData(IATLEventData* const pEventData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title SetLanguage>
        // Syntax:
        //      SetLanguage("english")
        // Summary:
        //      Informs the audio middleware that the localized sound banks and streamed files need to
        //      use a different language. NOTE: this function DOES NOT unload or reload the currently
        //      loaded audio files.
        // Arguments:
        //      sLanguage - a C-string representing the CryEngine language.
        // See Also:
        //      GetAudioFileLocation
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void SetLanguage(const char* const sLanguage) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title GetImplSubPath>
        // Summary:
        //      Returns the canonical subpath for this implementation.  Used for locating data under
        //      the Game folder.
        // Returns:
        //      A zero-terminated C-string of the subpath this implementation uses.
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual const char* const GetImplSubPath() const = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title GetImplementationNameString>
        // Summary:
        //      Return a string describing the audio middleware used. This string is printed on the
        //      first line of the AudioDebug header shown on the screen whenever s_DrawAudioDebug is
        //      not 0.
        // Returns:
        //      A zero-terminated C-string with the description of the audio middleware used by this
        //      AudioSystemImplementation.
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual const char* const GetImplementationNameString() const = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title GetMemoryInfo>
        // Summary:
        //      Fill in the oMemoryInfo describing the current memory usage of this
        //      AudioSystemImplementation.
        //      This data gets displayed in the AudioDebug header shown on the screen whenever
        //      s_DrawAudioDebug is not 0.
        // Arguments:
        //      oMemoryInfo - a reference to an instance of SAudioImplMemoryInfo.
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void GetMemoryInfo(SAudioImplMemoryInfo& oMemoryInfo) const = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title GetMemoryPoolInfo>
        // Summary:
        //      Retrieve a vector of memory pool infos.  The memory pools are defined and managed by
        //      the audio middleware.
        // Arguments:
        //      None
        // See Also:
        //      GetMemoryInfo
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual AZStd::vector<AudioImplMemoryPoolInfo> GetMemoryPoolInfo() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title CreateAudioSource>
        // Summary:
        //      Create a new Audio Input Source as specified by a configuration.
        // Arguments:
        //      sourceConfig - a configuration struct that specifies format or resource information.
        // See Also:
        //      DestroyAudioSource
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual bool CreateAudioSource(const SAudioInputConfig& sourceConfig) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title DestroyAudioSource>
        // Summary:
        //      Destroys an Audio Input Source by source ID.
        // Arguments:
        //      sourceId - ID of the Audio Input Source.
        // See Also:
        //      CreateAudioSource
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void DestroyAudioSource(TAudioSourceId sourceId) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title SetPanningMode>
        // Summary:
        // Arguments:
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual void SetPanningMode(PanningMode mode) {}
    };

    using AudioSystemImplementationRequestBus = AZ::EBus<AudioSystemImplementationRequests>;



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title IAudioSystemImplementationComponent>
    // Summary:
    //      The AudioTranslationLayer uses this interface to communicate with an audio middleware
    //      Shim Component Class which represents a full AudioSystemImplementation Bus Interface
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioSystemImplementationComponent
        : public AZ::Component
        , public AudioSystemImplementationNotificationBus::Handler
        , public AudioSystemImplementationRequestBus::Handler
    {
    public:
        ~AudioSystemImplementationComponent() override = default;
    };

} // namespace Audio
