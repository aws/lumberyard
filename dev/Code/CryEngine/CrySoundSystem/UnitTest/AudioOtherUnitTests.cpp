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
// Test 21. Set an Rtpc
// Case 1.1 : NULL IATLAudioObjectData & NULL IATLRtpcImplData
//          Expectation : FAILURE
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T21_1_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = nullptr;
    const IATLRtpcImplData* pATLRtpcData = nullptr;

    EAudioRequestStatus result = asi->SetRtpc(pATLObjData, pATLRtpcData, FLT_MIN);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 21. Set an Rtpc
// Case 1.2 : NULL IATLAudioObjectData  & Correct IATLRtpcImplData
//          Expectation : FAILURE
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T21_1_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = nullptr;
    const IATLRtpcImplData* pATLRtpcData = CAudioUnitTestData::GenerateRtpcImplData();

    EAudioRequestStatus result = asi->SetRtpc(pATLObjData, pATLRtpcData, FLT_MIN);

    asi->DeleteAudioRtpcImplData(pATLRtpcData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 21. Set an Rtpc
// Case 2.1 : Correct IATLAudioObjectData  & NULL IATLRtpcImplData
//          Expectation : FAILURE
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T21_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    const IATLRtpcImplData* pATLRtpcData = nullptr;

    EAudioRequestStatus result = asi->SetRtpc(pATLObjData, pATLRtpcData, FLT_MIN);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 21. Set an Rtpc
// Case 2.2.1 : Correct IATLAudioObjectData  & Correct IATLRtpcImplData , but IATLAudioObjectData is not registered
//          Expectation : SUCCESS
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T21_2_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    const IATLRtpcImplData* pATLRtpcData = CAudioUnitTestData::GenerateRtpcImplData();

    EAudioRequestStatus result = asi->SetRtpc(pATLObjData, pATLRtpcData, FLT_MIN);

    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioRtpcImplData(pATLRtpcData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 21. Set an Rtpc
// Case 2.2.2 : Correct IATLAudioObjectData  & Correct IATLRtpcImplData , IATLAudioObjectData is registered
//          Expectation : SUCCESS
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T21_2_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    asi->RegisterAudioObject(pATLObjData);
    const IATLRtpcImplData* pATLRtpcData = CAudioUnitTestData::GenerateRtpcImplData();

    EAudioRequestStatus result = asi->SetRtpc(pATLObjData, pATLRtpcData, FLT_MIN);

    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioRtpcImplData(pATLRtpcData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 21. Set an Rtpc on an a registered and then unregistered Audio Object
// Case 2.3 : Correct IATLAudioObjectData  & Correct IATLRtpcImplData
//          Expectation : SUCCESS
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T21_2_3)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    EAudioRequestStatus result = asi->RegisterAudioObject(pATLObjData);

    EAudioRequestStatus resultUnreg = asi->UnregisterAudioObject(pATLObjData);

    const IATLRtpcImplData* pATLRtpcData = CAudioUnitTestData::GenerateRtpcImplData();

    EAudioRequestStatus resultSetRtpc = asi->SetRtpc(pATLObjData, pATLRtpcData, FLT_MIN);

    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioRtpcImplData(pATLRtpcData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnreg);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultSetRtpc);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 21. Set an Rtpc
// Case 3.1.1 : Correct IATLAudioObjectData & Correct IATLRtpcImplData, negative maximum float
//    value used as rtpc, but IATLAudioObjectData is not registered
//          Expectation : FAILURE
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T21_3_1_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    const IATLRtpcImplData* pATLRtpcData = CAudioUnitTestData::GenerateRtpcImplData();

    EAudioRequestStatus result = asi->SetRtpc(pATLObjData, pATLRtpcData, -FLT_MAX);

    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioRtpcImplData(pATLRtpcData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 21. Set an Rtpc
// Case 3.1.2 : Correct IATLAudioObjectData & Correct IATLRtpcImplData, negative maximum float
//    value used as rtpc, IATLAudioObjectData is registered
//          Expectation : SUCCESS
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T21_3_1_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    asi->RegisterAudioObject(pATLObjData);
    const IATLRtpcImplData* pATLRtpcData = CAudioUnitTestData::GenerateRtpcImplData();

    EAudioRequestStatus result = asi->SetRtpc(pATLObjData, pATLRtpcData, -FLT_MAX);

    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioRtpcImplData(pATLRtpcData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 21. Set an Rtpc
// Case 3.2.1 : Correct IATLAudioObjectData & Correct IATLRtpcImplData, positive maximum float
//    value used as rtpc, but IATLAudioObjectData is not registered
//          Expectation : FAILURE
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T21_3_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    const IATLRtpcImplData* pATLRtpcData = CAudioUnitTestData::GenerateRtpcImplData();

    EAudioRequestStatus result = asi->SetRtpc(pATLObjData, pATLRtpcData, FLT_MAX);

    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioRtpcImplData(pATLRtpcData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 21. Set an Rtpc
// Case 3.2.2 : Correct IATLAudioObjectData & Correct IATLRtpcImplData, positive maximum float
//    value used as rtpc, IATLAudioObjectData is registered
//          Expectation : SUCCESS
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T21_3_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    asi->RegisterAudioObject(pATLObjData);
    const IATLRtpcImplData* pATLRtpcData = CAudioUnitTestData::GenerateRtpcImplData();

    EAudioRequestStatus result = asi->SetRtpc(pATLObjData, pATLRtpcData, FLT_MAX);

    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioRtpcImplData(pATLRtpcData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

/*********************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 22. Set a Switch state
// Case 1.1 : NULL IATLAudioObjectData & NULL IATLSwitchStateImplData
//          Expectation : FAILURE
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T22_1_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = nullptr;
    const IATLSwitchStateImplData* pATLSwitchStateData = nullptr;

    EAudioRequestStatus result = asi->SetSwitchState(pATLObjData, pATLSwitchStateData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 22. Set a Switch state
// Case 1.2 : NULL IATLAudioObjectData & Correct IATLSwitchStateImplData
//          Expectation : FAILURE
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T22_1_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = nullptr;
    const IATLSwitchStateImplData* pATLSwitchStateData = CAudioUnitTestData::GenerateSwitchStateImplData();

    EAudioRequestStatus result = asi->SetSwitchState(pATLObjData, pATLSwitchStateData);

    asi->DeleteAudioSwitchStateImplData(pATLSwitchStateData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 22. Set a Switch state
// Case 2.1 : Correct IATLAudioObjectData & NULL IATLSwitchStateImplData
//          Expectation : FAILURE
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T22_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    const IATLSwitchStateImplData* pATLSwitchStateData = nullptr;

    EAudioRequestStatus result = asi->SetSwitchState(pATLObjData, pATLSwitchStateData);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 22. Set a Switch state
// Case 2.2.1 : Correct IATLAudioObjectData & Correct IATLSwitchStateImplData, but IATLAudioObjectData
//    is not registered
//          Expectation : FAILURE
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T22_2_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    const IATLSwitchStateImplData* pATLSwitchStateData = CAudioUnitTestData::GenerateSwitchStateImplData();

    EAudioRequestStatus result = asi->SetSwitchState(pATLObjData, pATLSwitchStateData);

    asi->DeleteAudioSwitchStateImplData(pATLSwitchStateData);
    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 22. Set a Switch state
// Case 2.2.2 : Correct IATLAudioObjectData & Correct IATLSwitchStateImplData, IATLAudioObjectData
//    is registered
//          Expectation : SUCCESS
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T22_2_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    asi->RegisterAudioObject(pATLObjData);
    const IATLSwitchStateImplData* pATLSwitchStateData = CAudioUnitTestData::GenerateSwitchStateImplData();

    EAudioRequestStatus result = asi->SetSwitchState(pATLObjData, pATLSwitchStateData);

    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioSwitchStateImplData(pATLSwitchStateData);
    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

/*********************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 25. Register an in Memory file
// Case 1 : NULL SATLAudioFileEntryInfo
//          Expectation : FAILURE
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T25_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    SATLAudioFileEntryInfo* pAudioFileEntryInfo = nullptr;

    EAudioRequestStatus result = asi->RegisterInMemoryFile(pAudioFileEntryInfo);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 25. Register an in Memory file
// Case 2 : Valid SATLAudioFileEntryInfo
//          Expectation : SUCCESS
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T25_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    EAudioRequestStatus result = EAudioRequestStatus::eARS_FAILURE;

    SATLAudioFileEntryInfo* pAudioFileEntryInfo = CAudioUnitTestData::GenerateATLFileEntryInfoData();

    if (pAudioFileEntryInfo != nullptr)
    {
        string fileNameBuffer = pAudioFileEntryInfo->sFileName;

        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sFullPath(asi->GetAudioFileLocation(pAudioFileEntryInfo));
        pAudioFileEntryInfo->sFileName = sFullPath + pAudioFileEntryInfo->sFileName;

        CAudioUnitTestData::LoadBankFile(pAudioFileEntryInfo);

        pAudioFileEntryInfo->sFileName = fileNameBuffer.c_str();

        result = asi->RegisterInMemoryFile(pAudioFileEntryInfo);
        EAudioRequestStatus resultUnregister = asi->UnregisterInMemoryFile(pAudioFileEntryInfo);

        // done with the file data, release it.
        CAudioUnitTestData::ReleaseATLFileEntryInfoData(pAudioFileEntryInfo);
    }

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 26. UnRegister an in Memory file
// Case 1 : Null SATLAudioFileEntryInfo
//          Expectation : FAILURE
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T26_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    SATLAudioFileEntryInfo* pAudioFileEntryInfo = nullptr;

    EAudioRequestStatus result = asi->UnregisterInMemoryFile(pAudioFileEntryInfo);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 26. UnRegister an in Memory file
// Case 2 : Valid but unregistered SATLAudioFileEntryInfo
//          Expectation : FAILURE
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T26_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    EAudioRequestStatus result = EAudioRequestStatus::eARS_FAILURE;

    SATLAudioFileEntryInfo* pAudioFileEntryInfo = CAudioUnitTestData::GenerateATLFileEntryInfoData();

    if (pAudioFileEntryInfo != nullptr)
    {
        string fileNameBuffer = pAudioFileEntryInfo->sFileName;

        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sFullPath(asi->GetAudioFileLocation(pAudioFileEntryInfo));
        pAudioFileEntryInfo->sFileName = sFullPath + pAudioFileEntryInfo->sFileName;

        CAudioUnitTestData::LoadBankFile(pAudioFileEntryInfo);

        pAudioFileEntryInfo->sFileName = fileNameBuffer.c_str();

        result = asi->UnregisterInMemoryFile(pAudioFileEntryInfo);

        // done with the file data, release it.
        CAudioUnitTestData::ReleaseATLFileEntryInfoData(pAudioFileEntryInfo);
    }

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 26. UnRegister an in Memory file
// Case 3 : Valid and registered SATLAudioFileEntryInfo
//          Expectation : SUCCESS
TEST(AudioOtherUnitTestSuite, AudioOtherUnitTest_T26_3)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    EAudioRequestStatus resultRegister = EAudioRequestStatus::eARS_FAILURE;
    EAudioRequestStatus resultUnregister = EAudioRequestStatus::eARS_FAILURE;

    SATLAudioFileEntryInfo* pAudioFileEntryInfo = CAudioUnitTestData::GenerateATLFileEntryInfoData();

    if (pAudioFileEntryInfo != nullptr)
    {
        string fileNameBuffer = pAudioFileEntryInfo->sFileName;

        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sFullPath(asi->GetAudioFileLocation(pAudioFileEntryInfo));
        pAudioFileEntryInfo->sFileName = sFullPath + pAudioFileEntryInfo->sFileName;

        CAudioUnitTestData::LoadBankFile(pAudioFileEntryInfo);

        pAudioFileEntryInfo->sFileName = fileNameBuffer.c_str();

        resultRegister = asi->RegisterInMemoryFile(pAudioFileEntryInfo);
        // running the UnitTests caused a crash here once.  If anyone gets a crash here, maybe
        // putting a sleep here will fix it.
        resultUnregister = asi->UnregisterInMemoryFile(pAudioFileEntryInfo);

        // done with the file data, release it.
        CAudioUnitTestData::ReleaseATLFileEntryInfoData(pAudioFileEntryInfo);
    }

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnregister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultRegister);
}

/*********************************************************************************************/
