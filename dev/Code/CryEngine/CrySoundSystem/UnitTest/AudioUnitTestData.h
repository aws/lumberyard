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

#ifndef CRYINCLUDE_CRYSOUNDSYSTEM_UNITTEST_AUDIOUNITTESTDATA_H
#define CRYINCLUDE_CRYSOUNDSYSTEM_UNITTEST_AUDIOUNITTESTDATA_H
#pragma once

#include "ISystem.h"
#include "AudioSystem.h"

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioUnitTestData
    {
    public:
        //! Fetch the current Audio System Implementation.
        static IAudioSystemImplementation* GetAudioSystemImplementation();

        //! Creates and returns IATLTriggerImplData for testing
        /*!
            \param : fileName nullable file name, for sending different file names if needed for multiple tests
            \return : Valid IATLTriggerImplData
        */
        static const IATLTriggerImplData* GenerateTriggerImplData(const char* fileName = nullptr);

        //! Creates and returns IATLRtpcImplData for testing
        /*!
            \param : fileName nullable file name, for sending different file names if needed for multiple tests
            \return : Valid IATLRtpcImplData
        */
        static const IATLRtpcImplData* GenerateRtpcImplData(const char* fileName = nullptr);

        //! Creates and returns IATLSwitchStateImplData for testing
        /*!
            \param : fileName nullable file name, for sending different file names if needed for multiple tests
            \return : Valid IATLSwitchStateImplData
        */
        static const IATLSwitchStateImplData* GenerateSwitchStateImplData(const char* fileName = nullptr);

        //! Creates and returns IATLRtpcImplData for testing
        /*!
            \param : fileName nullable file name, for sending different file names if needed for multiple tests
            \return : Valid IATLEnvironmentImplData
        */
        static const IATLEnvironmentImplData* GenerateEnvironmentImplData(const char* fileName = nullptr);

        //! Creates and returns IATLRtpcImplData for testing
        /*!
            \param : fileName nullable file name, for sending different file names if needed for multiple tests
            \return : Valid SATLAudioFileEntryInfo*
        */
        static SATLAudioFileEntryInfo* const GenerateATLFileEntryInfoData(const char* fileName = nullptr);
        static void ReleaseATLFileEntryInfoData(SATLAudioFileEntryInfo* audioFileEntryInfo);

        //! Loads a bank file for trigger preparation
        /*!
            \param : audioFileEntryInfo Parsed file entry info that needs to be populated with loaded file info
        */
        static void LoadBankFile(SATLAudioFileEntryInfo* audioFileEntryInfo);

        //! Initializes the Unit test system
        static void Init();

        //! Increments and returs uint32's used as Audio object id's
        static uint32 GetNextUniqueId();

    private:
        // private constructor to prevent any funny business
        CAudioUnitTestData();

        static IAudioSystemImplementation* ms_audioSystemImpl;
        static uint32 ms_audioObjectNextId;
        static string ms_audioSystemImplementationName;
    };
} // namespace Audio

#endif // CRYINCLUDE_CRYSOUNDSYSTEM_UNITTEST_AUDIOUNITTESTDATA_H
