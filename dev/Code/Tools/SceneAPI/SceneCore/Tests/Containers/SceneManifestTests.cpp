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
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>
#include <string>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            static decltype(SceneManifest::s_invalidIndex) INVALID_INDEX(SceneManifest::s_invalidIndex); // gtest cannot compare static consts

            class MockManifestInt : public DataTypes::IManifestObject
            {
            public:
                AZ_RTTI(MockManifestInt, "{D6F96B49-4E6F-4EE8-A5A3-959B76F90DA8}", IManifestObject);
                AZ_CLASS_ALLOCATOR(MockManifestInt, AZ::SystemAllocator, 0)

                MockManifestInt()
                    : m_value(0)
                {
                }

                MockManifestInt(int64_t value)
                    : m_value(value)
                {
                }

                int64_t GetValue() const
                {
                    return m_value;
                }

                void SetValue(int64_t value)
                {
                    m_value = value;
                }

                static void Reflect(AZ::ReflectContext* context)
                {
                    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                    if (serializeContext)
                    {
                        serializeContext->
                            Class<MockManifestInt, IManifestObject>()->
                            Version(1)->
                            Field("value", &MockManifestInt::m_value);
                    }
                }

            protected:
                int64_t m_value;
            };


            class SceneManifestTest
                : public ::testing::Test
                , public AZ::Debug::TraceMessageBus::Handler
            {
            public:
                SceneManifestTest()
                {
                    m_firstDataObject = AZStd::make_shared<MockManifestInt>(1);
                    m_secondDataObject = AZStd::make_shared<MockManifestInt>(2);
                    m_testDataObject = AZStd::make_shared<MockManifestInt>(3);

                    m_testManifest.AddEntry(m_firstDataObject);
                    m_testManifest.AddEntry(m_secondDataObject);
                    m_testManifest.AddEntry(m_testDataObject);

                    MockManifestInt::Reflect(&m_context);
                    SceneManifest::Reflect(&m_context);
                }

                void SetUp() override
                {
                    BusConnect();
                }

                void TearDown() override
                {
                    BusDisconnect();
                }

                bool OnPreAssert(const char* /*message*/, int /*line*/, const char* /*func*/, const char* /*message*/) override
                {
                    m_assertTriggered = true;
                    return true;
                }
                bool m_assertTriggered = false;

                AZStd::shared_ptr<MockManifestInt> m_firstDataObject;
                AZStd::shared_ptr<MockManifestInt> m_secondDataObject;
                AZStd::shared_ptr<MockManifestInt> m_testDataObject;
                SceneManifest m_testManifest;
                SerializeContext m_context;
            };

            TEST(SceneManifest, IsEmpty_Empty_True)
            {
                SceneManifest testManifest;
                EXPECT_TRUE(testManifest.IsEmpty());
            }

            TEST(SceneManifest, AddEntry_AddNewValue_ResultTrue)
            {
                SceneManifest testManifest;
                AZStd::shared_ptr<MockManifestInt> testDataObject = AZStd::make_shared<MockManifestInt>(100);
                bool result = testManifest.AddEntry(testDataObject);
                EXPECT_TRUE(result);
            }

            TEST(SceneManifest, AddEntry_MoveNewValue_ResultTrueAndPointerClear)
            {
                SceneManifest testManifest;
                AZStd::shared_ptr<MockManifestInt> testDataObject = AZStd::make_shared<MockManifestInt>(100);
                bool result = testManifest.AddEntry(AZStd::move(testDataObject));
                EXPECT_TRUE(result);
                EXPECT_EQ(nullptr, testDataObject.get());
            }

            //dependent on AddEntry
            TEST(SceneManifest, IsEmpty_NotEmpty_False)
            {
                SceneManifest testManifest;
                AZStd::shared_ptr<MockManifestInt> testDataObject = AZStd::make_shared<MockManifestInt>(100);
                bool result = testManifest.AddEntry(AZStd::move(testDataObject));
                EXPECT_FALSE(testManifest.IsEmpty());
            }

            //dependent on AddEntry and IsEmpty
            TEST(SceneManifest, Clear_NotEmpty_EmptyTrue)
            {
                SceneManifest testManifest;
                AZStd::shared_ptr<MockManifestInt> testDataObject = AZStd::make_shared<MockManifestInt>(100);
                bool result = testManifest.AddEntry(AZStd::move(testDataObject));
                EXPECT_FALSE(testManifest.IsEmpty());
                testManifest.Clear();
                EXPECT_TRUE(testManifest.IsEmpty());
            }

            //RemoveEntry
            TEST(SceneManifest, RemoveEntry_NameInList_ResultTrueAndNotStillInList)
            {
                SceneManifest testManifest;
                AZStd::shared_ptr<MockManifestInt> testDataObject = AZStd::make_shared<MockManifestInt>(1);
                testManifest.AddEntry(testDataObject);

                bool result = testManifest.RemoveEntry(testDataObject);
                EXPECT_TRUE(result);
            }

            TEST_F(SceneManifestTest, RemoveEntry_NameNotInList_ResultFalse)
            {
                SceneManifest testManifest;
                AZStd::shared_ptr<MockManifestInt> testDataObject = AZStd::make_shared<MockManifestInt>(1);
                
                testManifest.RemoveEntry(testDataObject);
                EXPECT_TRUE(m_assertTriggered);
            }

            // GetEntryCount
            TEST(SceneManifest, GetEntryCount_EmptyManifest_CountIsZero)
            {
                SceneManifest testManifest;
                EXPECT_TRUE(testManifest.IsEmpty());
                EXPECT_EQ(0, testManifest.GetEntryCount());
            }

            TEST_F(SceneManifestTest, GetEntryCount_FilledManifest_CountIsThree)
            {
                EXPECT_EQ(3, m_testManifest.GetEntryCount());
            }

            // GetValue
            TEST_F(SceneManifestTest, GetValue_ValidIndex_ReturnsInt2)
            {
                AZStd::shared_ptr<MockManifestInt> result = azrtti_cast<MockManifestInt*>(m_testManifest.GetValue(1));
                ASSERT_TRUE(result);
                EXPECT_EQ(2, result->GetValue());
            }

            TEST_F(SceneManifestTest, GetValue_InvalidIndex_ReturnsNullPtr)
            {
                EXPECT_EQ(nullptr, m_testManifest.GetValue(42));
            }

            // FindIndex
            TEST_F(SceneManifestTest, FindIndex_ValidValue_ResultIsOne)
            {
                EXPECT_EQ(1, m_testManifest.FindIndex(m_secondDataObject.get()));
            }

            TEST_F(SceneManifestTest, FindIndex_InvalidValueFromSharedPtr_ResultIsInvalidIndex)
            {
                AZStd::shared_ptr<DataTypes::IManifestObject> invalid = AZStd::make_shared<MockManifestInt>(42);
                EXPECT_EQ(INVALID_INDEX, m_testManifest.FindIndex(invalid.get()));
            }

            TEST_F(SceneManifestTest, FindIndex_InvalidValueFromNullptr_ResultIsInvalidIndex)
            {
                DataTypes::IManifestObject* invalid = nullptr;
                EXPECT_EQ(INVALID_INDEX, m_testManifest.FindIndex(invalid));
            }

            // RemoveEntry - continued
            TEST(SceneManifest, RemoveEntry_IndexAdjusted_IndexReduced)
            {
                SceneManifest testManifest;
                AZStd::shared_ptr<MockManifestInt> testDataObject1 = AZStd::make_shared<MockManifestInt>(1);
                AZStd::shared_ptr<MockManifestInt> testDataObject2 = AZStd::make_shared<MockManifestInt>(2);
                AZStd::shared_ptr<MockManifestInt> testDataObject3 = AZStd::make_shared<MockManifestInt>(3);
                testManifest.AddEntry(testDataObject1);
                testManifest.AddEntry(testDataObject2);
                testManifest.AddEntry(testDataObject3);

                bool result = testManifest.RemoveEntry(testDataObject2);
                ASSERT_TRUE(result);

                EXPECT_EQ(1, azrtti_cast<MockManifestInt*>(testManifest.GetValue(0))->GetValue());
                EXPECT_EQ(3, azrtti_cast<MockManifestInt*>(testManifest.GetValue(1))->GetValue());

                EXPECT_EQ(0, testManifest.FindIndex(testDataObject1));
                EXPECT_EQ(INVALID_INDEX, testManifest.FindIndex(testDataObject2));
                EXPECT_EQ(1, testManifest.FindIndex(testDataObject3));
            }

            // SaveToStream
            TEST_F(SceneManifestTest, SaveToStream_SaveFilledManifestToMemoryStream_ReturnsTrue)
            {
                char buffer[64 * 1024];
                IO::MemoryStream memoryStream(IO::MemoryStream(buffer, sizeof(buffer), 0));
                bool result = m_testManifest.SaveToStream(memoryStream, &m_context);
                EXPECT_TRUE(result);
            }

            TEST_F(SceneManifestTest, SaveToStream_SaveEmptyManifestToMemoryStream_ReturnsTrue)
            {
                char buffer[64 * 1024];
                IO::MemoryStream memoryStream(IO::MemoryStream(buffer, sizeof(buffer), 0));
                
                SceneManifest empty;
                bool result = empty.SaveToStream(memoryStream, &m_context);
                EXPECT_TRUE(result);
            }

            // LoadFromStream
            TEST_F(SceneManifestTest, LoadFromStream_LoadEmptyManifestFromMemoryStream_ReturnsTrue)
            {
                char buffer[64 * 1024];
                IO::MemoryStream memoryStream(IO::MemoryStream(buffer, sizeof(buffer), 0));

                SceneManifest empty;
                bool result = empty.SaveToStream(memoryStream, &m_context);
                ASSERT_TRUE(result);

                memoryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);

                SceneManifest loaded;
                result = loaded.LoadFromStream(memoryStream, &m_context);
                EXPECT_TRUE(result);
                EXPECT_TRUE(loaded.IsEmpty());
            }

            TEST_F(SceneManifestTest, LoadFromStream_LoadFilledManifestFromMemoryStream_ReturnsTrue)
            {
                char buffer[64 * 1024];
                IO::MemoryStream memoryStream(IO::MemoryStream(buffer, sizeof(buffer), 0));
                bool result = m_testManifest.SaveToStream(memoryStream, &m_context);
                ASSERT_TRUE(result);

                memoryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);

                SceneManifest loaded;
                result = loaded.LoadFromStream(memoryStream, &m_context);
                EXPECT_TRUE(result);
                
                ASSERT_EQ(loaded.GetEntryCount(), m_testManifest.GetEntryCount());
            }
        }
    }
}
