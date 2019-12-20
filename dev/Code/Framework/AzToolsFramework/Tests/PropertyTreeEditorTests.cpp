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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditor.h>

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <QtTest/QtTest>
#include <QApplication>

namespace UnitTest
{
    using namespace AzToolsFramework;

    //! Test class
    struct PropertyTreeEditorTester
    {
        AZ_TYPE_INFO(PropertyTreeEditorTester, "{D3E17BE6-0FEB-4A04-B8BE-105A4666E79F}");

        int m_myInt = 42;
        int m_myNewInt = 43;
        bool m_myBool = true;
        float m_myFloat = 42.0f;
        AZStd::string m_myString = "StringValue";

        struct PropertyTreeEditorNestedTester
        {
            AZ_TYPE_INFO(PropertyTreeEditorTester, "{F5814544-424D-41C5-A5AB-632371615B6A}");

            AZStd::string m_myNestedString = "NestedString";
        };

        PropertyTreeEditorNestedTester m_nestedTester;

        void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PropertyTreeEditorTester>()
                    ->Version(1)
                    ->Field("myInt", &PropertyTreeEditorTester::m_myInt)
                    ->Field("myBool", &PropertyTreeEditorTester::m_myBool)
                    ->Field("myFloat", &PropertyTreeEditorTester::m_myFloat)
                    ->Field("myString", &PropertyTreeEditorTester::m_myString)
                    ->Field("NestedTester", &PropertyTreeEditorTester::m_nestedTester)
                    ->Field("myNewInt", &PropertyTreeEditorTester::m_myNewInt)
                    ;

                serializeContext->Class<PropertyTreeEditorNestedTester>()
                    ->Version(1)
                    ->Field("myNestedString", &PropertyTreeEditorNestedTester::m_myNestedString)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<PropertyTreeEditorTester>(
                        "PropertyTreeEditor Tester", "Tester for the PropertyTreeEditor")

                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myInt, "My Int", "A test int.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myBool, "My Bool", "A test bool.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myFloat, "My Float", "A test float.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myString, "My String", "A test string.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_nestedTester, "Nested", "A nested class.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorTester::m_myNewInt, "My New Int", "A test int.", "My Old Int")
                        ;

                    editContext->Class<PropertyTreeEditorNestedTester>(
                        "PropertyTreeEditor Nested Tester", "SubClass Tester for the PropertyTreeEditor")

                        ->DataElement(AZ::Edit::UIHandlers::Default, &PropertyTreeEditorNestedTester::m_myNestedString, "My Nested String", "A test string.")
                        ;
                }
            }
        }
    };

    class PropertyTreeEditorTests
        : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            m_app.Start(AzFramework::Application::Descriptor());
        }

        void TearDown() override
        {
            m_app.Stop();
        }

        ToolsApplication m_app;
        AZ::SerializeContext* m_serializeContext = nullptr;
    };

    TEST_F(PropertyTreeEditorTests, ReadPropertyTreeValues)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        PropertyTreeEditorTester propertyTreeEditorTester;
        propertyTreeEditorTester.Reflect(m_serializeContext);

        PropertyTreeEditor propertyTree = PropertyTreeEditor(&propertyTreeEditorTester, AZ::AzTypeInfo<PropertyTreeEditorTester>::Uuid());


        // Test existing properties of different types

        {
            PropertyTreeEditor::PropertyAccessOutcome boolOutcome = propertyTree.GetProperty("My Bool");
            EXPECT_TRUE(boolOutcome.IsSuccess());
            EXPECT_TRUE(AZStd::any_cast<bool>(boolOutcome.GetValue()));
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome intOutcome = propertyTree.GetProperty("My Int");
            EXPECT_TRUE(intOutcome.IsSuccess());
            EXPECT_EQ(AZStd::any_cast<int>(intOutcome.GetValue()), propertyTreeEditorTester.m_myInt);
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome floatOutcome = propertyTree.GetProperty("My Float");
            EXPECT_TRUE(floatOutcome.IsSuccess());
            EXPECT_FLOAT_EQ(AZStd::any_cast<float>(floatOutcome.GetValue()), propertyTreeEditorTester.m_myFloat);
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome stringOutcome = propertyTree.GetProperty("My String");
            EXPECT_TRUE(stringOutcome.IsSuccess());
            EXPECT_STREQ(AZStd::any_cast<AZStd::string>(stringOutcome.GetValue()).data(), propertyTreeEditorTester.m_myString.data());
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome nestedOutcome = propertyTree.GetProperty("Nested|My Nested String");
            EXPECT_TRUE(nestedOutcome.IsSuccess());
            EXPECT_STREQ(AZStd::any_cast<AZStd::string>(nestedOutcome.GetValue()).data(), propertyTreeEditorTester.m_nestedTester.m_myNestedString.data());
        }


        // Test non-existing properties

        {
            PropertyTreeEditor::PropertyAccessOutcome intOutcome = propertyTree.GetProperty("Wrong Property");
            EXPECT_FALSE(intOutcome.IsSuccess());
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome nestedOutcome = propertyTree.GetProperty("Nested|Wrong Nested Property");
            EXPECT_FALSE(nestedOutcome.IsSuccess());
        }

    }
    
    TEST_F(PropertyTreeEditorTests, WritePropertyTreeValues)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        PropertyTreeEditorTester propertyTreeEditorTester;
        propertyTreeEditorTester.Reflect(m_serializeContext);

        PropertyTreeEditor propertyTree = PropertyTreeEditor(&propertyTreeEditorTester, AZ::AzTypeInfo<PropertyTreeEditorTester>::Uuid());


        // Test existing properties of different types

        {
            PropertyTreeEditor::PropertyAccessOutcome boolOutcomeSet = propertyTree.SetProperty("My Bool", AZStd::any(false));
            EXPECT_TRUE(boolOutcomeSet.IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome boolOutcomeGet = propertyTree.GetProperty("My Bool");
            EXPECT_TRUE(boolOutcomeGet.IsSuccess());
            EXPECT_FALSE(AZStd::any_cast<bool>(boolOutcomeGet.GetValue()));
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome intOutcomeSet = propertyTree.SetProperty("My Int", AZStd::any(48));
            EXPECT_TRUE(intOutcomeSet.IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome intOutcomeGet = propertyTree.GetProperty("My Int");
            EXPECT_TRUE(intOutcomeGet.IsSuccess());
            EXPECT_EQ(AZStd::any_cast<int>(intOutcomeGet.GetValue()), AZStd::any_cast<int>(intOutcomeSet.GetValue()));
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome floatOutcomeSet = propertyTree.SetProperty("My Float", AZStd::any(48.0f));
            EXPECT_TRUE(floatOutcomeSet.IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome floatOutcomeGet = propertyTree.GetProperty("My Float");
            EXPECT_TRUE(floatOutcomeGet.IsSuccess());
            EXPECT_FLOAT_EQ(AZStd::any_cast<float>(floatOutcomeGet.GetValue()), AZStd::any_cast<float>(floatOutcomeSet.GetValue()));
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome stringOutcomeSet = propertyTree.SetProperty("My String", AZStd::make_any<AZStd::string>("New Value"));
            EXPECT_TRUE(stringOutcomeSet.IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome stringOutcomeGet = propertyTree.GetProperty("My String");
            EXPECT_TRUE(stringOutcomeGet.IsSuccess());
            EXPECT_STREQ(AZStd::any_cast<AZStd::string>(stringOutcomeSet.GetValue()).data(), AZStd::any_cast<AZStd::string>(stringOutcomeGet.GetValue()).data());
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome stringOutcomeSet = propertyTree.SetProperty("Nested|My Nested String", AZStd::make_any<AZStd::string>("New Nested Value"));
            EXPECT_TRUE(stringOutcomeSet.IsSuccess());

            PropertyTreeEditor::PropertyAccessOutcome stringOutcomeGet = propertyTree.GetProperty("Nested|My Nested String");
            EXPECT_TRUE(stringOutcomeGet.IsSuccess());
            EXPECT_STREQ(AZStd::any_cast<AZStd::string>(stringOutcomeSet.GetValue()).data(), AZStd::any_cast<AZStd::string>(stringOutcomeGet.GetValue()).data());
        }


        // Test non-existing properties

        {
            PropertyTreeEditor::PropertyAccessOutcome intOutcome = propertyTree.SetProperty("Wrong Property", AZStd::any(12));
            EXPECT_FALSE(intOutcome.IsSuccess());
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome nestedOutcome = propertyTree.SetProperty("Nested|Wrong Nested Property", AZStd::make_any<AZStd::string>("Some Value"));
            EXPECT_FALSE(nestedOutcome.IsSuccess());
        }


        // Test existing properties with wrong type

        {
            PropertyTreeEditor::PropertyAccessOutcome intOutcome = propertyTree.SetProperty("My Int", AZStd::any(12.0f));
            EXPECT_FALSE(intOutcome.IsSuccess());
        }

        {
            PropertyTreeEditor::PropertyAccessOutcome nestedOutcome = propertyTree.SetProperty("Nested|My Nested String", AZStd::any(42.0f));
            EXPECT_FALSE(nestedOutcome.IsSuccess());
        }
    }

    TEST_F(PropertyTreeEditorTests, PropertyTreeDeprecatedNamesSupport)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        PropertyTreeEditorTester propertyTreeEditorTester;
        propertyTreeEditorTester.Reflect(m_serializeContext);

        PropertyTreeEditor propertyTree = PropertyTreeEditor(&propertyTreeEditorTester, AZ::AzTypeInfo<PropertyTreeEditorTester>::Uuid());

        // Test that new and deprecated name both refer to the same property
        {
            int newIntValue = 0;

            // get current value of My New Int
            PropertyTreeEditor::PropertyAccessOutcome intOutcomeGet = propertyTree.GetProperty("My New Int");
            EXPECT_TRUE(intOutcomeGet.IsSuccess());

            newIntValue = AZStd::any_cast<int>(intOutcomeGet.GetValue());

            // Set new value to My Old Int
            PropertyTreeEditor::PropertyAccessOutcome intOutcomeSet = propertyTree.SetProperty("My Old Int", AZStd::any(12));
            EXPECT_TRUE(intOutcomeSet.IsSuccess());

            // Read value of My New Int again
            PropertyTreeEditor::PropertyAccessOutcome intOutcomeGetAgain = propertyTree.GetProperty("My New Int");
            EXPECT_TRUE(intOutcomeGetAgain.IsSuccess());

            // Verify that My Old Int and My New Int refer to the same property
            EXPECT_TRUE(AZStd::any_cast<int>(intOutcomeGetAgain.GetValue()) != newIntValue);
        }

    }
    
} // namespace UnitTest
