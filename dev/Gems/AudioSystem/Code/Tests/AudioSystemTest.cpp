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

#include <AzTest/AzTest.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string.h>

#include <AudioAllocators.h>
#include <ATLUtils.h>
#include <ATL.h>

#include <Mocks/IAudioSystemImplementationMock.h>
#include <Mocks/IAudioSystemMock.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ISystemMock.h>


using namespace Audio;
using ::testing::NiceMock;

void CreateAudioAllocators()
{
    if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ::SystemAllocator::Descriptor systemAllocDesc;
        systemAllocDesc.m_allocationRecords = true;
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create(systemAllocDesc);
    }

    if (!AZ::AllocatorInstance<Audio::AudioSystemAllocator>::IsReady())
    {
        Audio::AudioSystemAllocator::Descriptor allocDesc;
        allocDesc.m_allocationRecords = true;
        allocDesc.m_heap.m_fixedMemoryBlocksByteSize[0] = 0;
        AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Create(allocDesc);
    }
}

void DestroyAudioAllocators()
{
    if (AZ::AllocatorInstance<Audio::AudioSystemAllocator>::IsReady())
    {
        AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Destroy();
    }

    if (AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }
}


// This is the global test environment (global to the module under test).
// Use it to stub out an environment with mocks.
class AudioSystemTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    AZ_TEST_CLASS_ALLOCATOR(AudioSystemTestEnvironment)

    ~AudioSystemTestEnvironment() override = default;

protected:
    struct MockHolder
    {
        AZ_TEST_CLASS_ALLOCATOR(MockHolder)

        NiceMock<ConsoleMock> m_console;
        NiceMock<SystemMock> m_system;
    };

    void SetupEnvironment() override
    {
        CreateAudioAllocators();

        m_mocks = new MockHolder;

        m_stubEnv.bProfilerEnabled = false;
        m_stubEnv.pFrameProfileSystem = nullptr;
        m_stubEnv.callbackStartSection = nullptr;
        m_stubEnv.callbackEndSection = nullptr;

        // setup mocks
        m_stubEnv.pConsole = &m_mocks->m_console;
        m_stubEnv.pSystem = &m_mocks->m_system;
        gEnv = &m_stubEnv;

    }

    void TeardownEnvironment() override
    {
        delete m_mocks;
        DestroyAudioAllocators();
    }

private:
    SSystemGlobalEnvironment m_stubEnv;
    MockHolder* m_mocks = nullptr;
};


AZ_UNIT_TEST_HOOK(new AudioSystemTestEnvironment);


// Sanity Check
TEST(AudioSystemSanityTest, Sanity)
{
    ASSERT_EQ(1, 1);
}



/*
// This is an example of a test fixture:
// Use it to set up a common scenario across multiple related unit tests.
// Each test will execute SetUp and TearDown before and after running the test.

class AudioExampleTestFixture
    : public ::testing::Test
{
public:
    AudioExampleTestFixture()
    {
        // use ctor to initialize data
    }

    void SetUp() override
    {
        // additional setup, called before the test begins
    }

    void TearDown() override
    {
        // called after test ends
    }

    // data used for testing...
};

// To test using this fixture, do:
TEST_F(AudioExampleTestFixture, ThingUnderTest_WhatThisTests_ExpectedResult)
{
    ...
}
*/


TEST(ATLWorldPositionTest, TransformGetColumn_GetColumn_Passes)
{
    SATLWorldPosition position;
    AZ::Vector3 column0 = position.m_transform.GetColumn(0);
    EXPECT_TRUE(column0.IsClose(AZ::Vector3::CreateAxisX()));
    AZ::Vector3 column1 = position.m_transform.GetColumn(1);
    EXPECT_TRUE(column1.IsClose(AZ::Vector3::CreateAxisY()));
    AZ::Vector3 column2 = position.m_transform.GetColumn(2);
    EXPECT_TRUE(column2.IsClose(AZ::Vector3::CreateAxisZ()));
}

TEST(ATLWorldPositionTest, TransformNormalize_NormalizeNonUnitVectors_GivesUnitLengthVectors)
{
    AZ::Transform transformTest;
    transformTest.SetBasisX(AZ::Vector3::CreateAxisX());
    transformTest.SetBasisY(1.f, 2.f, 1.f);
    transformTest.SetBasisZ(1.f, 1.f, 2.f);

    SATLWorldPosition position(transformTest);

    position.NormalizeForwardVec();
    AZ::Vector3 forward = position.GetForwardVec();

    EXPECT_TRUE(AZ::IsClose(forward.GetLength(), 1.f, 1e-3f));

    position.NormalizeUpVec();
    AZ::Vector3 up = position.GetUpVec();

    EXPECT_TRUE(AZ::IsClose(up.GetLength(), 1.f, 1e-3f));
}

TEST(ATLWorldPositionTest, TransformNormalize_NormalizeZeroVectors_GivesBasisVectors)
{
    AZ::Transform transformTest = AZ::Transform::CreateZero();

    SATLWorldPosition position(transformTest);

    EXPECT_EQ(position.GetForwardVec(), AZ::Vector3::CreateZero());
    EXPECT_EQ(position.GetUpVec(), AZ::Vector3::CreateZero());

    position.NormalizeForwardVec();
    EXPECT_EQ(position.GetForwardVec(), AZ::Vector3::CreateAxisY());

    position.NormalizeUpVec();
    EXPECT_EQ(position.GetUpVec(), AZ::Vector3::CreateAxisZ());
}




//---------------//
// Test ATLUtils //
//---------------//


class ATLUtilsTestFixture
    : public ::testing::Test
{
protected:
    using KeyType = AZStd::string;
    using ValType = int;
    using MapType = AZStd::map<KeyType, ValType, AZStd::less<KeyType>, Audio::AudioSystemStdAllocator>;

    void SetUp() override
    {
        m_testMap["Hello"] = 10;
        m_testMap["World"] = 15;
        m_testMap["GoodBye"] = 20;
        m_testMap["Orange"] = 25;
        m_testMap["Apple"] = 30;
    }

    void TearDown() override
    {
        m_testMap.clear();
    }

    MapType m_testMap;
};


TEST_F(ATLUtilsTestFixture, FindPlace_ContainerContainsItem_FindsItem)
{
    MapType::iterator placeIterator;

    EXPECT_TRUE(FindPlace(m_testMap, KeyType { "Hello" }, placeIterator));
    EXPECT_NE(placeIterator, m_testMap.end());
}


TEST_F(ATLUtilsTestFixture, FindPlace_ContainerDoesntContainItem_FindsNone)
{
    MapType::iterator placeIterator;

    EXPECT_FALSE(FindPlace(m_testMap, KeyType { "goodbye" }, placeIterator));
    EXPECT_EQ(placeIterator, m_testMap.end());
}

TEST_F(ATLUtilsTestFixture, FindPlaceConst_ContainerContainsItem_FindsItem)
{
    MapType::const_iterator placeIterator;

    EXPECT_TRUE(FindPlaceConst(m_testMap, KeyType { "Orange" }, placeIterator));
    EXPECT_NE(placeIterator, m_testMap.end());
}

TEST_F(ATLUtilsTestFixture, FindPlaceConst_ContainerDoesntContainItem_FindsNone)
{
    MapType::const_iterator placeIterator;

    EXPECT_FALSE(FindPlaceConst(m_testMap, KeyType { "Bananas" }, placeIterator));
    EXPECT_EQ(placeIterator, m_testMap.end());
}



//---------------------------------//
// Test CAudioEventListenerManager //
//---------------------------------//

class AudioEventListenerManagerTestFixture
    : public ::testing::Test
{
public:
    AudioEventListenerManagerTestFixture()
        : m_callbackReceiver()
    {
        m_eventListener.m_callbackOwner = &m_callbackReceiver;
        m_eventListener.m_fnOnEvent = &EventListenerCallbackReceiver::AudioRequestCallback;
        m_eventListener.m_requestType = eART_AUDIO_ALL_REQUESTS;
        m_eventListener.m_specificRequestMask = eACMRT_REPORT_STARTED_EVENT;
    }

    void SetUp() override
    {
        m_callbackReceiver.Reset();
    }

    void TearDown() override
    {
    }

    // Eventually the callback will actually get called and we can check that.
    // For now, this is mostly here to act as a callback object placeholder.
    class EventListenerCallbackReceiver
    {
    public:
        static void AudioRequestCallback(const SAudioRequestInfo* const requestInfo)
        {
            ++s_numCallbacksReceived;
        }

        static int GetNumCallbacksReceived()
        {
            return s_numCallbacksReceived;
        }

        static void Reset()
        {
            s_numCallbacksReceived = 0;
        }

    protected:
        static int s_numCallbacksReceived;
    };

protected:
    EventListenerCallbackReceiver m_callbackReceiver;
    CAudioEventListenerManager m_eventListenerManager;

    SAudioEventListener m_eventListener;
};

int AudioEventListenerManagerTestFixture::EventListenerCallbackReceiver::s_numCallbacksReceived = 0;


TEST_F(AudioEventListenerManagerTestFixture, AudioEventListenerManager_AddListener_Succeeds)
{
    // add request listener...
    m_eventListenerManager.AddRequestListener(m_eventListener);

    EXPECT_EQ(m_eventListenerManager.GetNumEventListeners(), 1);
}

TEST_F(AudioEventListenerManagerTestFixture, AudioEventListenerManager_RemoveListener_Fails)
{
    // attempt removal when no request listeners have been added yet...
    EXPECT_EQ(m_eventListenerManager.GetNumEventListeners(), 0);

    m_eventListenerManager.RemoveRequestListener(m_eventListener);

    EXPECT_EQ(m_eventListenerManager.GetNumEventListeners(), 0);
}

TEST_F(AudioEventListenerManagerTestFixture, AudioEventListenerManager_AddListenerAndRemoveListener_Succeeds)
{
    // add a request listener, then remove it...
    EXPECT_EQ(m_eventListenerManager.GetNumEventListeners(), 0);

    m_eventListenerManager.AddRequestListener(m_eventListener);

    EXPECT_EQ(m_eventListenerManager.GetNumEventListeners(), 1);

    m_eventListenerManager.RemoveRequestListener(m_eventListener);

    EXPECT_EQ(m_eventListenerManager.GetNumEventListeners(), 0);
}

TEST_F(AudioEventListenerManagerTestFixture, AudioEventListenerManager_AddListenerAndTwiceRemoveListener_Succeeds)
{
    // add a request listener, then try to remove it twice...
    EXPECT_EQ(m_eventListenerManager.GetNumEventListeners(), 0);

    m_eventListenerManager.AddRequestListener(m_eventListener);

    EXPECT_EQ(m_eventListenerManager.GetNumEventListeners(), 1);

    m_eventListenerManager.RemoveRequestListener(m_eventListener);

    EXPECT_EQ(m_eventListenerManager.GetNumEventListeners(), 0);

    m_eventListenerManager.RemoveRequestListener(m_eventListener);

    EXPECT_EQ(m_eventListenerManager.GetNumEventListeners(), 0);
}

TEST_F(AudioEventListenerManagerTestFixture, AudioEventListenerManager_AddListenerAndRemoveWithNullCallbackFunc_Succeeds)
{
    // adds a request listener with a real callback function, then removes it with nullptr callback specified...
    // this should be a success...
    EXPECT_EQ(m_eventListenerManager.GetNumEventListeners(), 0);

    m_eventListenerManager.AddRequestListener(m_eventListener);

    EXPECT_EQ(m_eventListenerManager.GetNumEventListeners(), 1);

    SAudioEventListener nullCallbackListener;
    nullCallbackListener.m_callbackOwner = &m_callbackReceiver;
    nullCallbackListener.m_fnOnEvent = nullptr;
    m_eventListenerManager.RemoveRequestListener(nullCallbackListener);

    EXPECT_EQ(m_eventListenerManager.GetNumEventListeners(), 0);
}


//-------------------//
// Test Audio::Flags //
//-------------------//

TEST(AudioFlagsTest, AudioFlags_ZeroFlags_NoFlagsAreSet)
{
    const AZ::u8 noFlags = 0;
    const AZ::u8 allFlags = static_cast<AZ::u8>(~0);
    Audio::Flags<AZ::u8> testFlags;
    EXPECT_FALSE(testFlags.AreAnyFlagsActive(allFlags));
    EXPECT_FALSE(testFlags.AreMultipleFlagsActive());
    EXPECT_FALSE(testFlags.IsOneFlagActive());
    EXPECT_EQ(testFlags.GetRawFlags(), noFlags);
}

TEST(AudioFlagsTest, AudioFlags_OneFlag_OneFlagIsSet)
{
    const AZ::u8 flagBit = 1 << 4;
    Audio::Flags<AZ::u8> testFlags(flagBit);
    EXPECT_FALSE(testFlags.AreAnyFlagsActive(~flagBit));
    EXPECT_TRUE(testFlags.AreAnyFlagsActive(flagBit));
    EXPECT_TRUE(testFlags.AreAnyFlagsActive(flagBit | 1));
    EXPECT_TRUE(testFlags.AreAllFlagsActive(flagBit));
    EXPECT_FALSE(testFlags.AreAllFlagsActive(flagBit | 1));
    EXPECT_FALSE(testFlags.AreMultipleFlagsActive());
    EXPECT_TRUE(testFlags.IsOneFlagActive());
    EXPECT_EQ(testFlags.GetRawFlags(), flagBit);
}

TEST(AudioFlagsTest, AudioFlags_MultipleFlags_MultipleFlagsAreSet)
{
    const AZ::u8 flagBits = (1 << 5) | (1 << 2) | (1 << 3);
    Audio::Flags<AZ::u8> testFlags(flagBits);
    EXPECT_FALSE(testFlags.AreAnyFlagsActive(~flagBits));
    EXPECT_TRUE(testFlags.AreAnyFlagsActive(flagBits));
    EXPECT_TRUE(testFlags.AreAllFlagsActive(flagBits));
    EXPECT_FALSE(testFlags.AreAllFlagsActive(flagBits | 1));
    EXPECT_TRUE(testFlags.AreMultipleFlagsActive());
    EXPECT_FALSE(testFlags.IsOneFlagActive());
    EXPECT_EQ(testFlags.GetRawFlags(), flagBits);
}

TEST(AudioFlagsTest, AudioFlags_AddAndClear_FlagsAreCorrect)
{
    const AZ::u8 flagBits = (1 << 2) | (1 << 6);
    Audio::Flags<AZ::u8> testFlags;
    Audio::Flags<AZ::u8> zeroFlags;

    testFlags.AddFlags(flagBits);
    EXPECT_TRUE(testFlags != zeroFlags);

    testFlags.ClearFlags(flagBits);
    EXPECT_TRUE(testFlags == zeroFlags);
}

TEST(AudioFlagsTest, AudioFlags_SetAndClearAll_FlagsAreCorrect)
{
    const AZ::u8 flagBits = (1 << 3) | (1 << 5) | (1 << 7);
    Audio::Flags<AZ::u8> testFlags;
    Audio::Flags<AZ::u8> zeroFlags;

    testFlags.SetFlags(flagBits, true);
    EXPECT_TRUE(testFlags != zeroFlags);
    EXPECT_EQ(testFlags.GetRawFlags(), flagBits);

    testFlags.SetFlags((1 << 3), false);
    EXPECT_TRUE(testFlags != zeroFlags);
    EXPECT_NE(testFlags.GetRawFlags(), flagBits);

    testFlags.ClearAllFlags();
    EXPECT_TRUE(testFlags == zeroFlags);
}



//-------------------------//
// Test CATLDebugNameStore //
//-------------------------//

class ATLDebugNameStoreTestFixture
    : public ::testing::Test
{
public:
    ATLDebugNameStoreTestFixture()
        : m_audioObjectName("SomeAudioObject1")
        , m_audioTriggerName("SomeAudioTrigger1")
        , m_audioRtpcName("SomeAudioRtpc1")
        , m_audioSwitchName("SomeAudioSwitch1")
        , m_audioSwitchStateName("SomeAudioSwitchState1")
        , m_audioEnvironmentName("SomeAudioEnvironment1")
        , m_audioPreloadRequestName("SomeAudioPreloadRequest1")
    {}

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

protected:
    CATLDebugNameStore m_atlNames;
    AZStd::string m_audioObjectName;
    AZStd::string m_audioTriggerName;
    AZStd::string m_audioRtpcName;
    AZStd::string m_audioSwitchName;
    AZStd::string m_audioSwitchStateName;
    AZStd::string m_audioEnvironmentName;
    AZStd::string m_audioPreloadRequestName;
};

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_InitiallyDirty_ReturnsFalse)
{
    // expect that no changes are detected after construction.
    EXPECT_FALSE(m_atlNames.AudioObjectsChanged());
    EXPECT_FALSE(m_atlNames.AudioTriggersChanged());
    EXPECT_FALSE(m_atlNames.AudioRtpcsChanged());
    EXPECT_FALSE(m_atlNames.AudioSwitchesChanged());
    EXPECT_FALSE(m_atlNames.AudioEnvironmentsChanged());
    EXPECT_FALSE(m_atlNames.AudioPreloadsChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioObject_IsDirty)
{
    auto audioObjectID = AudioStringToID<TAudioObjectID>(m_audioObjectName.c_str());
    m_atlNames.AddAudioObject(audioObjectID, m_audioObjectName.c_str());

    EXPECT_TRUE(m_atlNames.AudioObjectsChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioObjectAndLookupName_FindsName)
{
    auto audioObjectID = AudioStringToID<TAudioObjectID>(m_audioObjectName.c_str());
    m_atlNames.AddAudioObject(audioObjectID, m_audioObjectName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioObjectName(audioObjectID), m_audioObjectName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioTrigger_IsDirty)
{
    auto audioTriggerID = AudioStringToID<TAudioControlID>(m_audioTriggerName.c_str());
    m_atlNames.AddAudioTrigger(audioTriggerID, m_audioTriggerName.c_str());

    EXPECT_TRUE(m_atlNames.AudioTriggersChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioTriggerAndLookupName_FindsName)
{
    auto audioTriggerID = AudioStringToID<TAudioControlID>(m_audioTriggerName.c_str());
    m_atlNames.AddAudioTrigger(audioTriggerID, m_audioTriggerName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioTriggerName(audioTriggerID), m_audioTriggerName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioRtpc_IsDirty)
{
    auto audioRtpcID = AudioStringToID<TAudioControlID>(m_audioRtpcName.c_str());
    m_atlNames.AddAudioRtpc(audioRtpcID, m_audioRtpcName.c_str());

    EXPECT_TRUE(m_atlNames.AudioRtpcsChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioRtpcAndLookupName_FindsName)
{
    auto audioRtpcID = AudioStringToID<TAudioControlID>(m_audioRtpcName.c_str());
    m_atlNames.AddAudioRtpc(audioRtpcID, m_audioRtpcName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioRtpcName(audioRtpcID), m_audioRtpcName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioSwitch_IsDirty)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());

    EXPECT_TRUE(m_atlNames.AudioSwitchesChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioSwitchAndLookupName_FindsName)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioSwitchName(audioSwitchID), m_audioSwitchName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioSwitchState_IsDirty)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());

    auto audioSwitchStateID = AudioStringToID<TAudioSwitchStateID>(m_audioSwitchStateName.c_str());
    m_atlNames.AddAudioSwitchState(audioSwitchID, audioSwitchStateID, m_audioSwitchStateName.c_str());

    EXPECT_TRUE(m_atlNames.AudioSwitchesChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioSwitchStateAndLookupNames_FindsNames)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());

    auto audioSwitchStateID = AudioStringToID<TAudioSwitchStateID>(m_audioSwitchStateName.c_str());
    m_atlNames.AddAudioSwitchState(audioSwitchID, audioSwitchStateID, m_audioSwitchStateName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioSwitchName(audioSwitchID), m_audioSwitchName.c_str());
    EXPECT_STREQ(m_atlNames.LookupAudioSwitchStateName(audioSwitchID, audioSwitchStateID), m_audioSwitchStateName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioPreload_IsDirty)
{
    auto audioPreloadID = AudioStringToID<TAudioPreloadRequestID>(m_audioPreloadRequestName.c_str());
    m_atlNames.AddAudioPreloadRequest(audioPreloadID, m_audioPreloadRequestName.c_str());

    EXPECT_TRUE(m_atlNames.AudioPreloadsChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioPreloadAndLookupName_FindsName)
{
    auto audioPreloadID = AudioStringToID<TAudioPreloadRequestID>(m_audioPreloadRequestName.c_str());
    m_atlNames.AddAudioPreloadRequest(audioPreloadID, m_audioPreloadRequestName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioPreloadRequestName(audioPreloadID), m_audioPreloadRequestName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioEnvironment_IsDirty)
{
    auto audioEnvironmentID = AudioStringToID<TAudioEnvironmentID>(m_audioEnvironmentName.c_str());
    m_atlNames.AddAudioEnvironment(audioEnvironmentID, m_audioEnvironmentName.c_str());

    EXPECT_TRUE(m_atlNames.AudioEnvironmentsChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_AddAudioEnvironmentAndLookupName_FindsName)
{
    auto audioEnvironmentID = AudioStringToID<TAudioEnvironmentID>(m_audioEnvironmentName.c_str());
    m_atlNames.AddAudioEnvironment(audioEnvironmentID, m_audioEnvironmentName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioEnvironmentName(audioEnvironmentID), m_audioEnvironmentName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioObjectNotFound_NotDirty)
{
    auto audioObjectID = AudioStringToID<TAudioObjectID>(m_audioObjectName.c_str());
    m_atlNames.RemoveAudioObject(audioObjectID);

    EXPECT_FALSE(m_atlNames.AudioObjectsChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioTriggerNotFound_NotDirty)
{
    auto audioTriggerID = AudioStringToID<TAudioControlID>(m_audioTriggerName.c_str());
    m_atlNames.RemoveAudioTrigger(audioTriggerID);

    EXPECT_FALSE(m_atlNames.AudioTriggersChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioRtpcNotFound_NotDirty)
{
    auto audioRtpcID = AudioStringToID<TAudioControlID>(m_audioRtpcName.c_str());
    m_atlNames.RemoveAudioRtpc(audioRtpcID);

    EXPECT_FALSE(m_atlNames.AudioRtpcsChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioSwitchNotFound_NotDirty)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    m_atlNames.RemoveAudioSwitch(audioSwitchID);

    EXPECT_FALSE(m_atlNames.AudioSwitchesChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioSwitchStateNotFound_NotDirty)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    auto audioSwitchStateID = AudioStringToID<TAudioSwitchStateID>(m_audioSwitchStateName.c_str());
    m_atlNames.RemoveAudioSwitchState(audioSwitchID, audioSwitchStateID);

    // todo: Revisit this test!
    // The last expect will be true unless we clear the dirty flags (SyncChanges) after adding the switch.
    // Could setup a separate fixture for this, and given that issue is resolved, can split this into two tests.

    EXPECT_FALSE(m_atlNames.AudioSwitchesChanged());

    // now add the switch and test again.
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());
    m_atlNames.RemoveAudioSwitchState(audioSwitchID, audioSwitchStateID);

    EXPECT_TRUE(m_atlNames.AudioSwitchesChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioPreloadRequestNotFound_NotDirty)
{
    auto audioPreloadID = AudioStringToID<TAudioPreloadRequestID>(m_audioPreloadRequestName.c_str());
    m_atlNames.RemoveAudioPreloadRequest(audioPreloadID);

    EXPECT_FALSE(m_atlNames.AudioPreloadsChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioEnvironmentNotFound_NotDirty)
{
    auto audioEnvironmentID = AudioStringToID<TAudioEnvironmentID>(m_audioEnvironmentName.c_str());
    m_atlNames.RemoveAudioEnvironment(audioEnvironmentID);

    EXPECT_FALSE(m_atlNames.AudioEnvironmentsChanged());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioObjectAndLookupName_FindsNone)
{
    auto audioObjectID = AudioStringToID<TAudioObjectID>(m_audioObjectName.c_str());
    m_atlNames.AddAudioObject(audioObjectID, m_audioObjectName.c_str());
    m_atlNames.RemoveAudioObject(audioObjectID);

    EXPECT_TRUE(m_atlNames.AudioObjectsChanged());
    EXPECT_EQ(m_atlNames.LookupAudioObjectName(audioObjectID), nullptr);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioTriggerAndLookupName_FindsNone)
{
    auto audioTriggerID = AudioStringToID<TAudioControlID>(m_audioTriggerName.c_str());
    m_atlNames.AddAudioTrigger(audioTriggerID, m_audioTriggerName.c_str());
    m_atlNames.RemoveAudioTrigger(audioTriggerID);

    EXPECT_TRUE(m_atlNames.AudioTriggersChanged());
    EXPECT_EQ(m_atlNames.LookupAudioTriggerName(audioTriggerID), nullptr);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioRtpcAndLookupName_FindsNone)
{
    auto audioRtpcID = AudioStringToID<TAudioControlID>(m_audioRtpcName.c_str());
    m_atlNames.AddAudioRtpc(audioRtpcID, m_audioRtpcName.c_str());
    m_atlNames.RemoveAudioRtpc(audioRtpcID);

    EXPECT_TRUE(m_atlNames.AudioRtpcsChanged());
    EXPECT_EQ(m_atlNames.LookupAudioRtpcName(audioRtpcID), nullptr);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioSwitchAndLookupName_FindsNone)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());
    m_atlNames.RemoveAudioSwitch(audioSwitchID);

    EXPECT_TRUE(m_atlNames.AudioSwitchesChanged());
    EXPECT_EQ(m_atlNames.LookupAudioSwitchName(audioSwitchID), nullptr);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioSwitchStateAndLookupName_FindsNone)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    auto audioSwitchStateID = AudioStringToID<TAudioSwitchStateID>(m_audioSwitchStateName.c_str());
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());
    m_atlNames.AddAudioSwitchState(audioSwitchID, audioSwitchStateID, m_audioSwitchStateName.c_str());
    m_atlNames.RemoveAudioSwitchState(audioSwitchID, audioSwitchStateID);

    EXPECT_TRUE(m_atlNames.AudioSwitchesChanged());
    EXPECT_EQ(m_atlNames.LookupAudioSwitchStateName(audioSwitchID, audioSwitchStateID), nullptr);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioPreloadRequestAndLookupName_FindsNone)
{
    auto audioPreloadID = AudioStringToID<TAudioPreloadRequestID>(m_audioPreloadRequestName.c_str());
    m_atlNames.AddAudioPreloadRequest(audioPreloadID, m_audioPreloadRequestName.c_str());
    m_atlNames.RemoveAudioPreloadRequest(audioPreloadID);

    EXPECT_TRUE(m_atlNames.AudioPreloadsChanged());
    EXPECT_EQ(m_atlNames.LookupAudioPreloadRequestName(audioPreloadID), nullptr);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_RemoveAudioEnvironmentAndLookupName_FindsNone)
{
    auto audioEnvironmentID = AudioStringToID<TAudioEnvironmentID>(m_audioEnvironmentName.c_str());
    m_atlNames.AddAudioEnvironment(audioEnvironmentID, m_audioEnvironmentName.c_str());
    m_atlNames.RemoveAudioEnvironment(audioEnvironmentID);

    EXPECT_TRUE(m_atlNames.AudioEnvironmentsChanged());
    EXPECT_EQ(m_atlNames.LookupAudioEnvironmentName(audioEnvironmentID), nullptr);
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupGlobalAudioObjectName_FindsName)
{
    const char* globalAudioObjectName = m_atlNames.LookupAudioObjectName(GLOBAL_AUDIO_OBJECT_ID);
    EXPECT_STREQ(globalAudioObjectName, "GlobalAudioObject");
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupAudioObjectName_FindsName)
{
    auto audioObjectID = AudioStringToID<TAudioObjectID>(m_audioObjectName.c_str());
    m_atlNames.AddAudioObject(audioObjectID, m_audioObjectName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioObjectName(audioObjectID), m_audioObjectName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupAudioTriggerName_FindsName)
{
    auto audioTriggerID = AudioStringToID<TAudioControlID>(m_audioTriggerName.c_str());
    m_atlNames.AddAudioTrigger(audioTriggerID, m_audioTriggerName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioTriggerName(audioTriggerID), m_audioTriggerName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupAudioRtpcName_FindsName)
{
    auto audioRtpcID = AudioStringToID<TAudioControlID>(m_audioRtpcName.c_str());
    m_atlNames.AddAudioRtpc(audioRtpcID, m_audioRtpcName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioRtpcName(audioRtpcID), m_audioRtpcName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupAudioSwitchName_FindsName)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioSwitchName(audioSwitchID), m_audioSwitchName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupAudioSwitchStateName_FindsName)
{
    auto audioSwitchID = AudioStringToID<TAudioControlID>(m_audioSwitchName.c_str());
    auto audioSwitchStateID = AudioStringToID<TAudioSwitchStateID>(m_audioSwitchStateName.c_str());
    m_atlNames.AddAudioSwitch(audioSwitchID, m_audioSwitchName.c_str());
    m_atlNames.AddAudioSwitchState(audioSwitchID, audioSwitchStateID, m_audioSwitchStateName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioSwitchStateName(audioSwitchID, audioSwitchStateID), m_audioSwitchStateName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupAudioPreloadRequestName_FindsName)
{
    auto audioPreloadID = AudioStringToID<TAudioPreloadRequestID>(m_audioPreloadRequestName.c_str());
    m_atlNames.AddAudioPreloadRequest(audioPreloadID, m_audioPreloadRequestName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioPreloadRequestName(audioPreloadID), m_audioPreloadRequestName.c_str());
}

TEST_F(ATLDebugNameStoreTestFixture, ATLDebugNameStore_LookupAudioEnvironmentName_FindsName)
{
    auto audioEnvironmentID = AudioStringToID<TAudioEnvironmentID>(m_audioEnvironmentName.c_str());
    m_atlNames.AddAudioEnvironment(audioEnvironmentID, m_audioEnvironmentName.c_str());

    EXPECT_STREQ(m_atlNames.LookupAudioEnvironmentName(audioEnvironmentID), m_audioEnvironmentName.c_str());
}
