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
#include <AzCore/base.h>
#include <AzCore/Memory/AllocatorScope.h>

#include <AudioControlsLoader.h>
#include <ATLControlsModel.h>

#include <platform.h>
#include <Mocks/ICryPakMock.h>

using ::testing::NiceMock;
using namespace AudioControls;

namespace CustomMocks
{
    class AudioControlsEditorTest_CryPakMock
        : public CryPakMock
    {
    public:
        AudioControlsEditorTest_CryPakMock(const char* levelName)
            : m_levelName(levelName)
        {}

        intptr_t FindFirst(const char* dir, _finddata_t* fd, unsigned int flags, bool allowUseFileSystem) override
        {
            if (fd)
            {
                ::memset(fd, 0, sizeof(_finddata_t));
                fd->size = sizeof(_finddata_t);
                azstrcpy(fd->name, AZ_ARRAY_SIZE(fd->name), m_levelName.c_str());    // fake the finding of a file
            }
            return static_cast<intptr_t>(1);    // signifies: non-zero handle (found a file)
        }

        int FindNext(intptr_t handle, _finddata_t* fd) override
        {
            return -1;  // signifies: done finding
        }

        // public: for easy resetting...
        AZStd::string m_levelName;
    };

} // namespace CustomMocks


class AudioControlsEditorTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    AZ_TEST_CLASS_ALLOCATOR(AudioControlsEditorTestEnvironment);

    ~AudioControlsEditorTestEnvironment() override = default;

protected:
    void SetupEnvironment() override
    {
        m_allocatorScope.ActivateAllocators();

        m_stubEnv.pCryPak = nullptr;
        m_stubEnv.pFileIO = nullptr;
        gEnv = &m_stubEnv;
    }

    void TeardownEnvironment() override
    {
        m_allocatorScope.DeactivateAllocators();
    }

private:
    AZ::AllocatorScope<AZ::OSAllocator, AZ::SystemAllocator, AZ::LegacyAllocator, CryStringAllocator> m_allocatorScope;
    SSystemGlobalEnvironment m_stubEnv;
};

AZ_UNIT_TEST_HOOK(new AudioControlsEditorTestEnvironment);

TEST(EditorAudioControlsEditorSanityTest, Sanity)
{
    EXPECT_EQ(1, 1);
}

TEST(AudioControlsEditorTest, AudioControlsLoader_LoadScopes_ScopesAreAdded)
{
    ASSERT_TRUE(gEnv != nullptr);
    ASSERT_TRUE(gEnv->pCryPak == nullptr);

    NiceMock<CustomMocks::AudioControlsEditorTest_CryPakMock> m_cryPakMock("ly_extension.ly");
    gEnv->pCryPak = &m_cryPakMock;

    CATLControlsModel atlModel;
    CAudioControlsLoader loader(&atlModel, nullptr, nullptr);

    loader.LoadScopes();
    EXPECT_TRUE(atlModel.ScopeExists("ly_extension"));

    m_cryPakMock.m_levelName = "cry_extension.cry";
    loader.LoadScopes();
    EXPECT_TRUE(atlModel.ScopeExists("cry_extension"));

    atlModel.ClearScopes();
    gEnv->pCryPak = nullptr;
}
