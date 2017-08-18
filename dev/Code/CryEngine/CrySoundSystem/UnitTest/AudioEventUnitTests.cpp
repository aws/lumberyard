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
#include "ATL.h"

using namespace Audio;

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 8. Prepare a trigger Synchronously
// Case 1.1 : NULL IATLAudioObjectData & NULL IATLTriggerImplData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T8_1_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLAudioObjectData* pATLObjData = nullptr;

    EAudioRequestStatus result = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 8. Prepare a trigger Synchronously
// Case 1.2 : NULL IATLAudioObjectData & Correct IATLTriggerImplData
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T8_1_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = nullptr;

    EAudioRequestStatus result = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 8. Prepare a trigger Synchronously
// Case 2.1 : Correct IATLAudioObjectData & NULL IATLTriggerImplData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T8_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 8. Prepare a trigger Synchronously
// Case 2.2.1 : Correct IATLAudioObjectData & Correct IATLTriggerImplData , but IATLAudioObjectData is not registered
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T8_2_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 8. Prepare a trigger Synchronously
// Case 2.2.2 : Correct IATLAudioObjectData & Correct IATLTriggerImplData , IATLAudioObjectData is registered
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T8_2_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    asi->RegisterAudioObject(pATLObjData);

    EAudioRequestStatus result = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 8. Prepare a trigger Synchronously
// Case 3.1 : Prepare twice , but IATLAudioObjectData is not registered
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T8_3_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);
    EAudioRequestStatus resultSecondTime = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultSecondTime);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 8. Prepare a trigger Synchronously
// Case 3.2 : Prepare twice, IATLAudioObjectData is registered
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T8_3_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    asi->RegisterAudioObject(pATLObjData);

    EAudioRequestStatus result = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);
    EAudioRequestStatus resultSecondTime = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultSecondTime);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 9. UnPrepare a trigger Synchronously
// Case 1.1 : NULL IATLAudioObjectData & NULL IATLTriggerImplData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T9_1_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLAudioObjectData* pATLObjData = nullptr;

    EAudioRequestStatus result = asi->UnprepareTriggerSync(pATLObjData, pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 9. UnPrepare a trigger Synchronously
// Case 1.2 : NULL IATLAudioObjectData & Correct IATLTriggerImplData
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T9_1_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = nullptr;

    EAudioRequestStatus result = asi->UnprepareTriggerSync(pATLObjData, pTriggerImplData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 9. UnPrepare a trigger Synchronously
// Case 2.1 : Correct IATLAudioObjectData & NULL IATLTriggerImplData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T9_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->UnprepareTriggerSync(pATLObjData, pTriggerImplData);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 9. UnPrepare a trigger Synchronously
// Case 2.2.1 : Correct IATLAudioObjectData & Correct IATLTriggerImplData, but IATLAudioObjectData is not registered
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T9_2_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultPrepare = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

    EAudioRequestStatus result = asi->UnprepareTriggerSync(pATLObjData, pTriggerImplData);

    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultPrepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 9. UnPrepare a trigger Synchronously
// Case 2.2.2 : Correct IATLAudioObjectData & Correct IATLTriggerImplData, IATLAudioObjectData is registered
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T9_2_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    asi->RegisterAudioObject(pATLObjData);

    EAudioRequestStatus resultPrepare = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

    EAudioRequestStatus result = asi->UnprepareTriggerSync(pATLObjData, pTriggerImplData);

    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultPrepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 9. UnPrepare the same trigger Synchronously twice
// Case 3.1 : UnPrepare twice, but IATLAudioObjectData is not registered
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T9_3_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultPrepare = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

    EAudioRequestStatus resultUnprepare = asi->UnprepareTriggerSync(pATLObjData, pTriggerImplData);

    EAudioRequestStatus resultSecondTime = asi->UnprepareTriggerSync(pATLObjData, pTriggerImplData);

    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultPrepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnprepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultSecondTime);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 9. Unprepare Triggers Synchronously
// Case 3.3 : Prepare a trigger Async and Unprepare it Synchronously
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T9_3_3)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultRegister = asi->RegisterAudioObject(pATLObjData);

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    EAudioRequestStatus resultUnRegister = asi->UnregisterAudioObject(pATLObjData);

    EAudioRequestStatus resultUnprep = asi->UnprepareTriggerSync(pATLObjData, pTriggerImplData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnprep);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 9. UnPrepare the same trigger Synchronously twice
// Case 3.2 : UnPrepare twice, IATLAudioObjectData is registered
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T9_3_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    asi->RegisterAudioObject(pATLObjData);

    EAudioRequestStatus resultPrepare = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

    EAudioRequestStatus result = asi->UnprepareTriggerSync(pATLObjData, pTriggerImplData);

    EAudioRequestStatus resultSecondTime = asi->UnprepareTriggerSync(pATLObjData, pTriggerImplData);

    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultPrepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultSecondTime);
}


/*********************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 10. Prepare a trigger Asynchronously
// Case 1.1.1 : NULL IATLAudioObjectData & NULL IATLTriggerImplData & NULL EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T10_1_1_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLAudioObjectData* pATLObjData = nullptr;

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus result = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 10. Prepare a trigger Asynchronously
// Case 1.1.2 : NULL IATLAudioObjectData & NULL IATLTriggerImplData & Correct EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T10_1_1_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLAudioObjectData* pATLObjData = nullptr;

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 10. Prepare a trigger Asynchronously
// Case 1.2.1 : NULL IATLAudioObjectData & Correct IATLTriggerImplData & NULL EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T10_1_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = nullptr;

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus result = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 10. Prepare a trigger Asynchronously
// Case 1.2.2 : NULL IATLAudioObjectData & Correct IATLTriggerImplData & Correct EventData
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T10_1_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = nullptr;

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);
    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 10. Prepare a trigger Asynchronously
// Case 2.1.1 : Correct IATLAudioObjectData & NULL IATLTriggerImplData & NULL EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T10_2_1_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus result = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 10. Prepare a trigger Asynchronously
// Case 2.1.2 : Correct IATLAudioObjectData & NULL IATLTriggerImplData & Correct EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T10_2_1_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();
    ;

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus result = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 10. Prepare a trigger Asynchronously
// Case 2.2.1 : Correct IATLAudioObjectData & Correct IATLTriggerImplData & NULL EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T10_2_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus result = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 10. Prepare a trigger Asynchronously
// Case 2.2.2.1 : Correct IATLAudioObjectData & Correct IATLTriggerImplData & Correct EventData Registered IATLAudioObjectData
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T10_2_2_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultRegister = asi->RegisterAudioObject(pATLObjData);

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultPrepare = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    EAudioRequestStatus resultUnRegister = asi->UnregisterAudioObject(pATLObjData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultPrepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnRegister);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 10. Prepare a trigger Asynchronously
// Case 2.2.2 : Correct IATLAudioObjectData & Correct IATLTriggerImplData & Correct EventData UnRegistered IATLAudioObjectData
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T10_2_2_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

/*********************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 11. UnPrepare a trigger Asynchronously
// Case 1.1.1 : NULL IATLAudioObjectData & NULL IATLTriggerImplData & NULL EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T11_1_1_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLAudioObjectData* pATLObjData = nullptr;

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus result = asi->UnprepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 11. UnPrepare a trigger Asynchronously
// Case 1.1.2 : NULL IATLAudioObjectData & NULL IATLTriggerImplData & Correct EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T11_1_1_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLAudioObjectData* pATLObjData = nullptr;

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->UnprepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 11. UnPrepare a trigger Asynchronously
// Case 1.2.1 : NULL IATLAudioObjectData & Correct IATLTriggerImplData & NULL EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T11_1_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = nullptr;

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus result = asi->UnprepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 11. UnPrepare a trigger Asynchronously
// Case 1.2.2 : NULL IATLAudioObjectData & Correct IATLTriggerImplData & Correct EventData
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T11_1_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = nullptr;

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->UnprepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);
    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 11. UnPrepare a trigger Asynchronously
// Case 2.1.1 : Correct IATLAudioObjectData & NULL IATLTriggerImplData & NULL EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T11_2_1_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus result = asi->UnprepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 11. UnPrepare a trigger Asynchronously
// Case 2.1.2 : Correct IATLAudioObjectData & NULL IATLTriggerImplData & Correct EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T11_2_1_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus result = asi->UnprepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 11. UnPrepare a trigger Asynchronously
// Case 2.2.1 : Correct IATLAudioObjectData & Correct IATLTriggerImplData & NULL EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T11_2_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus result = asi->UnprepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 11. UnPrepare a trigger Asynchronously after Async Prep
// Case 2.2.2.1 : Correct IATLAudioObjectData & Correct IATLTriggerImplData & Correct EventData
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T11_2_2_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultRegister = asi->RegisterAudioObject(pATLObjData);

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultPrepare = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    EAudioRequestStatus resultUnRegister = asi->UnregisterAudioObject(pATLObjData);

    EAudioRequestStatus resultUnprepare = asi->UnprepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultPrepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnprepare);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 11. UnPrepare a trigger Asynchronously after Sync Prep
// Case 2.2.2.2 : Correct IATLAudioObjectData & Correct IATLTriggerImplData & Correct EventData
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T11_2_2_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultRegister = asi->RegisterAudioObject(pATLObjData);

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultPrepare = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

    EAudioRequestStatus resultUnRegister = asi->UnregisterAudioObject(pATLObjData);

    EAudioRequestStatus resultUnprepare = asi->UnprepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultPrepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnprepare);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 11. UnPrepare a trigger Asynchronously twice after Sync Prep
// Case 2.2.2.3 : Correct IATLAudioObjectData & Correct IATLTriggerImplData & Correct EventData
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T11_2_2_2_3)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultRegister = asi->RegisterAudioObject(pATLObjData);

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultPrepare = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

    EAudioRequestStatus resultUnRegister = asi->UnregisterAudioObject(pATLObjData);

    EAudioRequestStatus resultUnprepare = asi->UnprepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    EAudioRequestStatus resultUnprepSecond = asi->UnprepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultPrepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnprepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnprepSecond);
}


/*********************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 12. Activate a trigger
// Case 1.1.1 : NULL IATLAudioObjectData & NULL IATLTriggerImplData & NULL EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T12_1_1_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLAudioObjectData* pATLObjData = nullptr;

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus result = asi->ActivateTrigger(pATLObjData, pTriggerImplData, pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 12. Activate a trigger
// Case 1.1.2 : NULL IATLAudioObjectData & NULL IATLTriggerImplData & Correct EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T12_1_1_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLAudioObjectData* pATLObjData = nullptr;

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->ActivateTrigger(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 12. Activate a trigger
// Case 1.2.1 : NULL IATLAudioObjectData & Correct IATLTriggerImplData & NULL EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T12_1_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = nullptr;

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus result = asi->ActivateTrigger(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 12. Activate a trigger
// Case 1.2.2 : NULL IATLAudioObjectData & Correct IATLTriggerImplData & Correct EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T12_1_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = nullptr;

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->ActivateTrigger(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);
    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 12. Activate a trigger
// Case 2.1.1 : Correct IATLAudioObjectData & NULL IATLTriggerImplData & NULL EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T12_2_1_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    const IATLTriggerImplData* const pTriggerImplData = nullptr;

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus result = asi->ActivateTrigger(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 12. Activate a trigger
// Case 2.1.2 : Correct IATLAudioObjectData & NULL IATLTriggerImplData & Correct EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T12_2_1_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();
    ;

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus result = asi->ActivateTrigger(pATLObjData, pTriggerImplData, pATLEventData);

    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioTriggerImplData(pTriggerImplData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 12. Activate a trigger
// Case 2.2.1 : Correct IATLAudioObjectData & Correct IATLTriggerImplData & NULL EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T12_2_2_1)
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

        const IATLTriggerImplData* const pTriggerImplData = nullptr;

        IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

        IATLEventData* pATLEventData = nullptr;

        result = asi->ActivateTrigger(pATLObjData, pTriggerImplData, pATLEventData);
        asi->DeleteAudioObjectData(pATLObjData);

        // Unloading Sound banks
        EAudioRequestStatus resultUnregister = asi->UnregisterInMemoryFile(pAudioFileEntryInfo);

        CAudioUnitTestData::ReleaseATLFileEntryInfoData(pAudioFileEntryInfo);
        // Unloading Sound banks
    }

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 12. Activate a trigger
// Case 2.2.2.1 : Correct IATLAudioObjectData & Correct IATLTriggerImplData & Correct EventData
//    Registered IATLAudioObjectData with Async trigger prep
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T12_2_2_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    EAudioRequestStatus resultRegister = EAudioRequestStatus::eARS_FAILURE;
    EAudioRequestStatus resultPrepare = EAudioRequestStatus::eARS_FAILURE;
    EAudioRequestStatus resultActivation = EAudioRequestStatus::eARS_FAILURE;
    EAudioRequestStatus resultUnRegister = EAudioRequestStatus::eARS_FAILURE;

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

        const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

        IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

        resultRegister = asi->RegisterAudioObject(pATLObjData);

        IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

        resultPrepare = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

        resultActivation = asi->ActivateTrigger(pATLObjData, pTriggerImplData, pATLEventData);

        resultUnRegister = asi->UnregisterAudioObject(pATLObjData);

        asi->DeleteAudioTriggerImplData(pTriggerImplData);
        asi->DeleteAudioObjectData(pATLObjData);
        asi->DeleteAudioEventData(pATLEventData);

        // Unloading Sound banks
        EAudioRequestStatus resultUnregister = asi->UnregisterInMemoryFile(pAudioFileEntryInfo);

        CAudioUnitTestData::ReleaseATLFileEntryInfoData(pAudioFileEntryInfo);
        // Unloading Sound banks
    }

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultPrepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultActivation);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnRegister);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 12. Activate a trigger
// Case 2.2.2.3 : Correct IATLAudioObjectData & Correct IATLTriggerImplData & Correct EventData
//    Registered IATLAudioObjectData with Sync trigger prep
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T12_2_2_2_3)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    EAudioRequestStatus resultRegister = EAudioRequestStatus::eARS_FAILURE;
    EAudioRequestStatus resultPrepare = EAudioRequestStatus::eARS_FAILURE;
    EAudioRequestStatus resultActivation = EAudioRequestStatus::eARS_FAILURE;
    EAudioRequestStatus resultUnRegister = EAudioRequestStatus::eARS_FAILURE;

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

        const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

        IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

        resultRegister = asi->RegisterAudioObject(pATLObjData);

        IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

        resultPrepare = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

        resultActivation = asi->ActivateTrigger(pATLObjData, pTriggerImplData, pATLEventData);

        resultUnRegister = asi->UnregisterAudioObject(pATLObjData);

        asi->DeleteAudioTriggerImplData(pTriggerImplData);
        asi->DeleteAudioObjectData(pATLObjData);
        asi->DeleteAudioEventData(pATLEventData);

        // Unloading Sound banks
        EAudioRequestStatus resultUnregister = asi->UnregisterInMemoryFile(pAudioFileEntryInfo);

        CAudioUnitTestData::ReleaseATLFileEntryInfoData(pAudioFileEntryInfo);
        // Unloading Sound banks
    }

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultPrepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultActivation);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnRegister);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 12. Activate a trigger
// Case 2.2.2.2 : Correct IATLAudioObjectData & Correct IATLTriggerImplData & Correct EventData
//    UnRegistered IATLAudioObjectData
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T12_2_2_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    EAudioRequestStatus resultPrepare = EAudioRequestStatus::eARS_FAILURE;
    EAudioRequestStatus resultActivation = EAudioRequestStatus::eARS_FAILURE;

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

        const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

        IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

        IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

        resultPrepare = asi->PrepareTriggerSync(pATLObjData, pTriggerImplData);

        resultActivation = asi->ActivateTrigger(pATLObjData, pTriggerImplData, pATLEventData);

        asi->DeleteAudioTriggerImplData(pTriggerImplData);
        asi->DeleteAudioObjectData(pATLObjData);
        asi->DeleteAudioEventData(pATLEventData);

        // Unloading Sound banks
        EAudioRequestStatus resultUnregister = asi->UnregisterInMemoryFile(pAudioFileEntryInfo);

        CAudioUnitTestData::ReleaseATLFileEntryInfoData(pAudioFileEntryInfo);
        // Unloading Sound banks
    }

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultPrepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultActivation);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 12. Activate a trigger
// Case 12.3 : Correct IATLAudioObjectData & Correct IATLTriggerImplData & Correct EventData
//    Registered IATLAudioObjectData without trigger prep
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T12_3)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    EAudioRequestStatus resultRegister = EAudioRequestStatus::eARS_FAILURE;
    EAudioRequestStatus resultActivation = EAudioRequestStatus::eARS_FAILURE;
    EAudioRequestStatus resultUnRegister = EAudioRequestStatus::eARS_FAILURE;

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

        const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

        IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

        resultRegister = asi->RegisterAudioObject(pATLObjData);

        IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

        resultActivation = asi->ActivateTrigger(pATLObjData, pTriggerImplData, pATLEventData);

        resultUnRegister = asi->UnregisterAudioObject(pATLObjData);

        asi->DeleteAudioTriggerImplData(pTriggerImplData);
        asi->DeleteAudioObjectData(pATLObjData);
        asi->DeleteAudioEventData(pATLEventData);

        Sleep(10000);

        // Unloading Sound banks
        EAudioRequestStatus resultUnregister = asi->UnregisterInMemoryFile(pAudioFileEntryInfo);

        CAudioUnitTestData::ReleaseATLFileEntryInfoData(pAudioFileEntryInfo);
        // Unloading Sound banks
    }

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultActivation);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnRegister);
}

/*********************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 14. Stop An event
// Case 1.1 : NULL IATLAudioObjectData & NULL EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T14_1_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = nullptr;

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus resultStopEvent = asi->StopEvent(pATLObjData, pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, resultStopEvent);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 14. Stop An event
// Case 1.2 : null IATLAudioObjectData & Correct EventData, On an event with a trigger being prepared
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T14_1_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultPrepare = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    IATLEventData* pATLEventActivationData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultActivation = asi->ActivateTrigger(pATLObjData, pTriggerImplData, pATLEventActivationData);

    EAudioRequestStatus resultStopEvent = asi->StopEvent(nullptr, pATLEventActivationData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultPrepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultStopEvent);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultActivation);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 14. Stop An event
// Case 2.1 : Correct IATLAudioObjectData & Correct EventData, On an event with a trigger being prepared
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T14_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultPrepare = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    IATLEventData* pATLEventActivationData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultActivation = asi->ActivateTrigger(pATLObjData, pTriggerImplData, pATLEventActivationData);

    EAudioRequestStatus resultStopEvent = asi->StopEvent(pATLObjData, pATLEventActivationData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultPrepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultStopEvent);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultActivation);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 14. Stop An event
// Case 2.2 : Correct IATLAudioObjectData & NULL EventData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T14_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    IATLEventData* pATLEventData = nullptr;

    EAudioRequestStatus resultStopEvent = asi->StopEvent(pATLObjData, pATLEventData);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, resultStopEvent);
}


/*********************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 15. Stop all events on an Audio Object Data
// Case 1 : Correct IATLAudioObjectData , On an event with a trigger being Activated
//          Expectation : SUCCESS
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T15_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    const IATLTriggerImplData* const pTriggerImplData = CAudioUnitTestData::GenerateTriggerImplData();

    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    IATLEventData* pATLEventData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultPrepare = asi->PrepareTriggerAsync(pATLObjData, pTriggerImplData, pATLEventData);

    IATLEventData* pATLEventActivationData = asi->NewAudioEventData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultActivation = asi->ActivateTrigger(pATLObjData, pTriggerImplData, pATLEventActivationData);

    EAudioRequestStatus resultStopEvent = asi->StopAllEvents(pATLObjData);

    asi->DeleteAudioTriggerImplData(pTriggerImplData);
    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioEventData(pATLEventData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultPrepare);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultStopEvent);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultActivation);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 15. Stop all events on an Audio Object Data
// Case 2 : NULL IATLAudioObjectData
//          Expectation : FAILURE
TEST(AudioEventUnitTestSuite, AudioEventUnitTest_T15_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = nullptr;

    EAudioRequestStatus resultStopEvent = asi->StopAllEvents(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, resultStopEvent);
}
