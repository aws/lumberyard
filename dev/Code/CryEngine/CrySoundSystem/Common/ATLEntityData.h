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

#include <IAudioInterfacesCommonData.h>


///////////////////////////////////////////////////////////////////////////////////////////////////
// Description:
//          This file defines the data-types used in the IAudioSystemImplementation.h
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title IATLAudioObjectData>
    // Summary:
    //          An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
    //          data needed for identifying and using the corresponding ATLAudioObject (e.g. a middleware-specific unique ID)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLAudioObjectData
    {
        virtual ~IATLAudioObjectData() {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title IATLListenerData>
    // Summary:
    //          An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
    //          data needed for identifying and using the corresponding ATLListenerObject (e.g. a middleware-specific unique ID)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLListenerData
    {
        virtual ~IATLListenerData() {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title IATLTriggerImplData>
    // Summary::
    //          An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
    //          data needed for identifying and using the corresponding ATLTriggerImpl
    //          (e.g. a middleware-specific event ID or name, a sound-file name to be passed to an API function)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLTriggerImplData
    {
        virtual ~IATLTriggerImplData() {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title IATLRtpcImplData>
    // Summary:
    //          An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
    //          data needed for identifying and using the corresponding ATLRtpcImpl
    //          (e.g. a middleware-specific control ID or a parameter name to be passed to an API function)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLRtpcImplData
    {
        virtual ~IATLRtpcImplData() {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title IATLSwitchStateImplData>
    // Summary:
    //          An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
    //          data needed for identifying and using the corresponding ATLSwitchStateImpl
    //          (e.g. a middleware-specific control ID or a switch and state names to be passed to an API function)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLSwitchStateImplData
    {
        virtual ~IATLSwitchStateImplData() {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title IATLEnvironmentImplData>
    // Summary:
    //          An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
    //          data needed for identifying and using the corresponding ATLEnvironmentImpl
    //          (e.g. a middleware-specific bus ID or name to be passed to an API function)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLEnvironmentImplData
    {
        virtual ~IATLEnvironmentImplData() {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title IATLEventData>
    // Summary:
    //          An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
    //          data needed for identifying and using the corresponding ATLEvent
    //          (e.g. a middleware-specific playingID of an active event/sound for a play event)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLEventData
    {
        virtual ~IATLEventData() {}

        TAudioControlID m_triggerId = INVALID_AUDIO_CONTROL_ID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title IATLAudioFileEntryData>
    // Summary:
    //          An AudioSystemImplementation may use this interface to define a class for storing implementation-specific
    //          data needed for identifying and using the corresponding ATLAudioFileEntry.
    //          (e.g. a middleware-specific bank ID if the AudioFileEntry represents a soundbank)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLAudioFileEntryData
    {
        virtual ~IATLAudioFileEntryData() {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title SATLSourceData>
    // Summary:
    //          An AudioSystemImplementation may use this interface to define a class for storing
    //          implementation-specific data needed for identifying and using the corresponding ATLSource.
    //          (e.g. a middleware-specific sourceID, language, collection, and filename of an external
    //          source for a play event)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLSourceData
    {
        SAudioSourceInfo m_sourceInfo;

        SATLSourceData()
        {}

        SATLSourceData(const SAudioSourceInfo& sourceInfo)
            : m_sourceInfo(sourceInfo)
        {}

        ~SATLSourceData() {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title SATLAudioFileEntryInfo>
    // Summary:
    //          This is a POD structure used to pass the information about a file preloaded into memory between
    //          the CryAudioSystem and an AudioSystemImplementation
    //          Note: This struct cannot define a constructor, it needs to be a POD!
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLAudioFileEntryInfo
    {
        void* pFileData;                // pointer to the memory location of the file's contents
        size_t nMemoryBlockAlignment;   // memory alignment to be used for storing this file's contents in memory
        size_t nSize;                   // file size
        const char* sFileName;          // file name
        bool bLocalized;                // is the file localized
        IATLAudioFileEntryData* pImplData; // pointer to the implementation-specific data needed for this AudioFileEntry

        SATLAudioFileEntryInfo()
            : pFileData(nullptr)
            , nMemoryBlockAlignment(0)
            , nSize(0)
            , sFileName(nullptr)
            , bLocalized(false)
            , pImplData(nullptr)
        {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title SAudioImplMemoryInfo>
    // Summary:
    //          This is a POD structure used to pass the information about an AudioSystemImplementation's memory usage
    //          Note: This struct cannot define a constructor, it needs to be a POD!
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioImplMemoryInfo
    {
        size_t nPrimaryPoolSize;        // total size in bytes of the Primary Memory Pool used by an AudioSystemImplementation
        size_t nPrimaryPoolUsedSize;    // bytes allocated inside the Primary Memory Pool used by an AudioSystemImplementation
        size_t nPrimaryPoolAllocations; // number of allocations performed in the Primary Memory Pool used by an AudioSystemImplementation
        size_t nSecondaryPoolSize;      // total size in bytes of the Secondary Memory Pool used by an AudioSystemImplementation
        size_t nSecondaryPoolUsedSize;  // bytes allocated inside the Secondary Memory Pool used by an AudioSystemImplementation
        size_t nSecondaryPoolAllocations; // number of allocations performed in the Secondary Memory Pool used by an AudioSystemImplementation
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct AudioImplMemoryPoolInfo
    {
        char m_poolName[64];            // friendly name of the pool
        AZ::s32 m_poolId = -1;          // -1 is invalid/default
        AZ::u32 m_memoryReserved = 0;   // size of the pool in bytes
        AZ::u32 m_memoryUsed = 0;       // amount of the pool used in bytes
        AZ::u32 m_peakUsed = 0;         // peak used size in bytes
        AZ::u32 m_numAllocs = 0;        // number of alloc calls
        AZ::u32 m_numFrees = 0;         // number of free calls
    };
} // namespace Audio
