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
// Test 4. Register an audio object
// Case 1.1 : Valid IATLAudioObjectData
//          Expectation : SUCCESS
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T4_1_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    EAudioRequestStatus result = asi->RegisterAudioObject(pATLObjData);

    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 4. Register an audio object
// Case 1.2 : Valid IATLAudioObjectData
//          Expectation : SUCCESS
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T4_1_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->RegisterAudioObject(pATLObjData, "Test Audio Object");

    // NOTE: when create an unit test, whenever register an audio object, always remember to unregister it afterwards, otherwise would affect subsequent unit tests
    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 4. Register an audio object
// Case 2.1 : null IATLAudioObjectData
//          Expectation : FAILURE
// Fails with error Message, Wwise impl does not do any checking for null pointers (perhaps because it always expects a valid pointer)
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T4_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = nullptr;

    EAudioRequestStatus result = asi->RegisterAudioObject(pATLObjData);

    asi->UnregisterAudioObject(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 4. Register an audio object
// Case 2.2 : null IATLAudioObjectData and null Object name
//          Expectation : FAILURE
// Fails with error Message, Wwise impl does not do any checking for null pointers (perhaps because it always expects a valid pointer)
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T4_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = nullptr;

    EAudioRequestStatus result = asi->RegisterAudioObject(pATLObjData, nullptr);

    asi->UnregisterAudioObject(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 4. Register an audio object
// Case 3 : Valid IATLAudioObjectData registered twice
//          Expectation : SUCCESS
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T4_3)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    // Find a better way to generate id's ?
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());
    EAudioRequestStatus result = asi->RegisterAudioObject(pATLObjData);

    EAudioRequestStatus resultSecondRegister = asi->RegisterAudioObject(pATLObjData);

    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultSecondRegister);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 5. UnRegister an audio object
// Case 1 : Null IATLAudioObjectData
//          Expectation : FAILURE
// Fails with error Message, Wwise impl does not do any checking for null pointers (perhaps because it always expects a valid pointer)
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T5_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = nullptr;

    EAudioRequestStatus result = asi->UnregisterAudioObject(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 5. UnRegister an audio object
// Case 2.1 : Correct IATLAudioObjectData, Attempted deregister of unregistered AudioObject
//          Expectation : SUCCESS
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T5_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus eaudioReq = asi->UnregisterAudioObject(pATLObjData);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, eaudioReq);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 5. UnRegister an audio object
// Case 2.2 : Correct IATLAudioObjectData, Attempted deregister of registered AudioObject
//          Expectation : SUCCESS
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T5_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultRegister = asi->RegisterAudioObject(pATLObjData);

    EAudioRequestStatus resultUnRegister = asi->UnregisterAudioObject(pATLObjData);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnRegister);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 5. UnRegister an audio object
// Case 2.3 : Correct IATLAudioObjectData, Attempted deregister of deregistered AudioObject
//          Expectation : SUCCESS
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T5_2_3)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultRegister = asi->RegisterAudioObject(pATLObjData);

    EAudioRequestStatus resultUnRegister = asi->UnregisterAudioObject(pATLObjData);

    EAudioRequestStatus resultUnRegisterSecond = asi->UnregisterAudioObject(pATLObjData);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUnRegisterSecond);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 6. Reset an audio object
// Case 1 : Null IATLAudioObjectData
//          Expectation : FAILURE
// Note: this test case is temporarily disabled because for wWise, it will (crash? and) hang up the unit test.
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T6_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = nullptr;

    EAudioRequestStatus result = asi->ResetAudioObject(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 6. Reset an audio object
// Case 2.1 : Valid and registered IATLAudioObjectData
//          Expectation : SUCCESS
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T6_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultRegister = asi->RegisterAudioObject(pATLObjData);

    EAudioRequestStatus resultReset = asi->ResetAudioObject(pATLObjData);

    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultReset);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultRegister);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 6. Reset an audio object
// Case 2.2 : Valid but Unregistered IATLAudioObjectData
//          Expectation : SUCCESS
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T6_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus result = asi->ResetAudioObject(pATLObjData);

    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 7. Update Audio Object
// Case 1 : Null IATLAudioObjectData
//          Expectation : FAILURE
// Fails with error Message, Wwise impl does not do any checking for null pointers (perhaps because it always expects a valid pointer)
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T7_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = nullptr;

    EAudioRequestStatus result = asi->UpdateAudioObject(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 7. Update Audio Object
// Case 2.1 : Correct IATLAudioObjectData which was registered
// [NOTE : Fails because Audio objects only update when it really needs to update environment Need to find the success case]
//          Expectation : FAIL
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T7_2_1)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    EAudioRequestStatus resultRegister = asi->RegisterAudioObject(pATLObjData);

    EAudioRequestStatus resultUpdate = asi->UpdateAudioObject(pATLObjData);

    asi->UnregisterAudioObject(pATLObjData);
    asi->DeleteAudioObjectData(pATLObjData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultRegister);
    EXPECT_EQ(EAudioRequestStatus::eARS_FAILURE, resultUpdate);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Test 7. Update Audio Object
// Case 2.2 : Correct IATLAudioObjectData which was registered
//          Expectation : SUCCESS
TEST(AudioObjectUnitTestSuite, AudioObjectUnitTest_T7_2_2)
{
    auto asi = CAudioUnitTestData::GetAudioSystemImplementation();
    IATLAudioObjectData* pATLObjData = asi->NewAudioObjectData(CAudioUnitTestData::GetNextUniqueId());

    const IATLEnvironmentImplData* pATLEnvironData = CAudioUnitTestData::GenerateEnvironmentImplData();

    EAudioRequestStatus result = asi->SetEnvironment(pATLObjData, pATLEnvironData, FLT_MIN);

    EAudioRequestStatus resultUpdate = asi->UpdateAudioObject(pATLObjData);

    asi->DeleteAudioObjectData(pATLObjData);
    asi->DeleteAudioEnvironmentImplData(pATLEnvironData);

    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, result);
    EXPECT_EQ(EAudioRequestStatus::eARS_SUCCESS, resultUpdate);
}
