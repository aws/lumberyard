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
#include "stdafx.h"
#include <gtest/gtest.h>

#include "AudioUnitTestData.h"

using namespace Audio;

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 16. Set the position of an Audio Object
// Case 1 : Null IATLAudioObjectData
//          Expectation : FAILURE
TEST(AudioEnvironmentUnitTestSuite, AudioEnvironmentUnitTest_T16_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = nullptr;

    SATLWorldPosition pWorldPosition;

    EAudioRequestStatus result = asi->SetPosition(pATLObjData, pWorldPosition);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 16. Set the position of an Audio Object
// Case 2.1 : Valid IATLAudioObjectData, but IATLAudioObjectData is not registered
//          Expectation : FAILURE
TEST(AudioEnvironmentUnitTestSuite, AudioEnvironmentUnitTest_T16_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    SATLWorldPosition pWorldPosition;

    EAudioRequestStatus result = asi->SetPosition(pATLObjData, pWorldPosition);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 16. Set the position of an Audio Object
// Case 2.2 : Valid IATLAudioObjectData, IATLAudioObjectData is registered
//          Expectation : SUCCESS
TEST(AudioEnvironmentUnitTestSuite, AudioEnvironmentUnitTest_T16_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    asi->RegisterAudioObject(pATLObjData);

    SATLWorldPosition pWorldPosition;

    EAudioRequestStatus result = asi->SetPosition(pATLObjData, pWorldPosition);

    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 17. Set the Audio Objects (IATLAudioObjectData) Environment Data (IATLEnvironmentImplData)
// Case 1.1 : Null IATLAudioObjectData & IATLEnvironmentImplData
//          Expectation : FAILURE
TEST(AudioEnvironmentUnitTestSuite, AudioEnvironmentUnitTest_T17_1_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = nullptr;
    const IATLEnvironmentImplData* pATLEnvironData = nullptr;

    EAudioRequestStatus result = asi->SetEnvironment(pATLObjData, pATLEnvironData, FLT_MIN);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 17. Set the Audio Objects (IATLAudioObjectData) Environment Data (IATLEnvironmentImplData)
// Case 1.2 : Null IATLAudioObjectData & Correct IATLEnvironmentImplData
//          Expectation : FAILURE
TEST(AudioEnvironmentUnitTestSuite, AudioEnvironmentUnitTest_T17_1_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();

    // Loading Sound banks
    EAudioRequestStatus result = EAudioRequestStatus::eARS_FAILURE;

    SATLAudioFileEntryInfo* pAudioFileEntryInfo = CAudioUnitTestData::GenerateATLFileEntryInfoData();

    if (pAudioFileEntryInfo != nullptr)
    {
        string fileNameBuffer = pAudioFileEntryInfo->sFileName;

        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sFullPath(asi->GetAudioFileLocation(pAudioFileEntryInfo));
        pAudioFileEntryInfo->sFileName = sFullPath + pAudioFileEntryInfo->sFileName;

        CAudioUnitTestData::LoadBankFile(pAudioFileEntryInfo);

        pAudioFileEntryInfo->sFileName = fileNameBuffer.c_str();

        asi->RegisterInMemoryFile(pAudioFileEntryInfo);
        // Loading Sound banks

        IATLAudioObjectData* pATLObjData = nullptr;
        const IATLEnvironmentImplData* pATLEnvironData = CAudioUnitTestData::GenerateEnvironmentImplData();

        result = asi->SetEnvironment(pATLObjData, pATLEnvironData, FLT_MIN);

        asi->DeleteAudioEnvironmentImplData(pATLEnvironData);

        // Unloading Sound banks
        EAudioRequestStatus resultUnregister = asi->UnregisterInMemoryFile(pAudioFileEntryInfo);

        CAudioUnitTestData::ReleaseATLFileEntryInfoData(pAudioFileEntryInfo);
        // Unloading Sound banks
    }

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 17. Set the Audio Objects (IATLAudioObjectData) Environment Data (IATLEnvironmentImplData)
// Case 2.1 : Correct IATLAudioObjectData & Null IATLEnvironmentImplData
//          Expectation : FAILURE
TEST(AudioEnvironmentUnitTestSuite, AudioEnvironmentUnitTest_T17_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();

    EAudioRequestStatus result = EAudioRequestStatus::eARS_FAILURE;

    // Loading Sound banks
    SATLAudioFileEntryInfo* pAudioFileEntryInfo = CAudioUnitTestData::GenerateATLFileEntryInfoData();

    if (pAudioFileEntryInfo != nullptr)
    {
        string fileNameBuffer = pAudioFileEntryInfo->sFileName;

        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sFullPath(asi->GetAudioFileLocation(pAudioFileEntryInfo));
        pAudioFileEntryInfo->sFileName = sFullPath + pAudioFileEntryInfo->sFileName;

        CAudioUnitTestData::LoadBankFile(pAudioFileEntryInfo);

        pAudioFileEntryInfo->sFileName = fileNameBuffer.c_str();

        asi->RegisterInMemoryFile(pAudioFileEntryInfo);
        // Loading Sound banks

        IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
        const IATLEnvironmentImplData* pATLEnvironData = nullptr;

        result = asi->SetEnvironment(pATLObjData, pATLEnvironData, FLT_MIN);

        asi->DeleteAudioObjectData(pATLObjData);

        // Unloading Sound banks
        EAudioRequestStatus resultUnregister = asi->UnregisterInMemoryFile(pAudioFileEntryInfo);

        CAudioUnitTestData::ReleaseATLFileEntryInfoData(pAudioFileEntryInfo);
        // Unloading Sound banks
    }

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 17. Set the Audio Objects (IATLAudioObjectData) Environment Data (IATLEnvironmentImplData)
// Case 2.2.1 : Correct IATLAudioObjectData & Correct IATLEnvironmentImplData , but IATLAudioObjectData is not registered
//          Expectation : FAILURE
TEST(AudioEnvironmentUnitTestSuite, AudioEnvironmentUnitTest_T17_2_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();

    EAudioRequestStatus result = EAudioRequestStatus::eARS_FAILURE;

    // Loading Sound banks
    SATLAudioFileEntryInfo* pAudioFileEntryInfo = CAudioUnitTestData::GenerateATLFileEntryInfoData();

    if (pAudioFileEntryInfo != nullptr)
    {
        string fileNameBuffer = pAudioFileEntryInfo->sFileName;

        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sFullPath(asi->GetAudioFileLocation(pAudioFileEntryInfo));
        pAudioFileEntryInfo->sFileName = sFullPath + pAudioFileEntryInfo->sFileName;

        CAudioUnitTestData::LoadBankFile(pAudioFileEntryInfo);

        pAudioFileEntryInfo->sFileName = fileNameBuffer.c_str();

        asi->RegisterInMemoryFile(pAudioFileEntryInfo);
        // Loading Sound banks

        IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
        const IATLEnvironmentImplData* pATLEnvironData = CAudioUnitTestData::GenerateEnvironmentImplData();

        result = asi->SetEnvironment(pATLObjData, pATLEnvironData, FLT_MIN);

        asi->DeleteAudioObjectData(pATLObjData);
        asi->DeleteAudioEnvironmentImplData(pATLEnvironData);

        // Unloading Sound banks
        EAudioRequestStatus resultUnregister = asi->UnregisterInMemoryFile(pAudioFileEntryInfo);

        CAudioUnitTestData::ReleaseATLFileEntryInfoData(pAudioFileEntryInfo);
        // Unloading Sound banks
    }

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 17. Set the Audio Objects (IATLAudioObjectData) Environment Data (IATLEnvironmentImplData)
// Case 2.2.2 : Correct IATLAudioObjectData & Correct IATLEnvironmentImplData , IATLAudioObjectData is registered
//          Expectation : SUCCESS
TEST(AudioEnvironmentUnitTestSuite, AudioEnvironmentUnitTest_T17_2_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();

    EAudioRequestStatus result = EAudioRequestStatus::eARS_FAILURE;

    // Loading Sound banks
    SATLAudioFileEntryInfo* pAudioFileEntryInfo = CAudioUnitTestData::GenerateATLFileEntryInfoData();

    if (pAudioFileEntryInfo != nullptr)
    {
        string fileNameBuffer = pAudioFileEntryInfo->sFileName;

        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sFullPath(asi->GetAudioFileLocation(pAudioFileEntryInfo));
        pAudioFileEntryInfo->sFileName = sFullPath + pAudioFileEntryInfo->sFileName;

        CAudioUnitTestData::LoadBankFile(pAudioFileEntryInfo);

        pAudioFileEntryInfo->sFileName = fileNameBuffer.c_str();

        asi->RegisterInMemoryFile(pAudioFileEntryInfo);
        // Loading Sound banks

        IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
        asi->RegisterAudioObject(pATLObjData);
        const IATLEnvironmentImplData* pATLEnvironData = CAudioUnitTestData::GenerateEnvironmentImplData();

        result = asi->SetEnvironment(pATLObjData, pATLEnvironData, FLT_MIN);

        asi->UnregisterAudioObject(pATLObjData);
        asi->DeleteAudioObjectData(pATLObjData);
        asi->DeleteAudioEnvironmentImplData(pATLEnvironData);

        // Unloading Sound banks
        EAudioRequestStatus resultUnregister = asi->UnregisterInMemoryFile(pAudioFileEntryInfo);

        CAudioUnitTestData::ReleaseATLFileEntryInfoData(pAudioFileEntryInfo);
        // Unloading Sound banks
    }

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 18. Set the position of an Audio Listener
// Case 1 : Null IATLAudioObjectData
//          Expectation : FAILURE
TEST(AudioEnvironmentUnitTestSuite, AudioEnvironmentUnitTest_T18_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLListenerData* pATLListenerObjData = nullptr;

    SATLWorldPosition pWorldPosition;

    EAudioRequestStatus result = asi->SetListenerPosition(pATLListenerObjData, pWorldPosition);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 18. Set the position of an Audio Listener
// Case 2 : Valid _IATLListenerData
//          Expectation : SUCCESS
TEST(AudioEnvironmentUnitTestSuite, AudioEnvironmentUnitTest_T18_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLListenerData* pATLListenerObjData = asi->NewDefaultAudioListenerObjectData();

    SATLWorldPosition pWorldPosition;

    EAudioRequestStatus result = asi->SetListenerPosition(pATLListenerObjData, pWorldPosition);

    asi->DeleteAudioListenerObjectData(pATLListenerObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 23 Set the Occlusion and obstruction
// Case 1 : NULL IATLAudioObjectData
//          Expectation : FAILURE
TEST(AudioEnvironmentUnitTestSuite, AudioEnvironmentUnitTest_T23_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = nullptr;

    EAudioRequestStatus result = asi->SetObstructionOcclusion(pATLObjData, 0.5f, 0.5f);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 23 Set the Occlusion and obstruction
// Case 2.1 : Correct IATLAudioObjectData, in range fObstruction and fOcclusion [0.0 - 1.0], but IATLAudioObjectData is not registered
//       Expectation : FAILURE
TEST(AudioEnvironmentUnitTestSuite, AudioEnvironmentUnitTest_T23_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->SetObstructionOcclusion(pATLObjData, 0.5f, 0.5f);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 23 Set the Occlusion and obstruction
// Case 2.2 : Correct IATLAudioObjectData, in range fObstruction and fOcclusion [0.0 - 1.0], IATLAudioObjectData is registered
//       Expectation : SUCCESS
TEST(AudioEnvironmentUnitTestSuite, AudioEnvironmentUnitTest_T23_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    asi->RegisterAudioObject(pATLObjData);

    EAudioRequestStatus result = asi->SetObstructionOcclusion(pATLObjData, 0.5f, 0.5f);

    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 23 Set the Occlusion and obstruction
// Case 2.1 : Correct IATLAudioObjectData , Out of range fObstruction and fOcclusion [0.0 - 1.0]
//       Expectation : SUCCESS with a warning
TEST(AudioEnvironmentUnitTestSuite, AudioEnvironmentUnitTest_T23_3)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->SetObstructionOcclusion(pATLObjData, 10.0f, -10.0f);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}
