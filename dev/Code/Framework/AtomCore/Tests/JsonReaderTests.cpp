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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <AtomCore/Serialization/JsonReader.h>

namespace UnitTest
{
    using namespace AZ;

    class JsonReaderTests
        : public AllocatorsTestFixture
        , public TraceBusRedirector
    {
    protected:
        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();
        }

        void TearDown() override
        {
            AllocatorsTestFixture::TearDown();
        }

        struct Status
        {
            float health;
            float stamina;
        };

        struct OptionalStats
        {
            int32_t hitCount;
            int32_t healCount;
        };

        struct Character
        {
            AZStd::string name;
            AZStd::string type;
            AZStd::vector<AZStd::string> gear;
            Status status;
            OptionalStats optionalStats;
        };

        /// This function processes an example file format that describes game characters.
        /// Returns the number of keys that were read.
        size_t ReadCharacterFile(AZStd::string_view jsonTxt, JsonReader& reader, AZStd::vector<Character>& characters)
        {
            AZ_UNUSED(jsonTxt);

            struct ReadCounter
            {
                size_t m_numReads = 0;
                void Count(bool readResult)
                {
                    if (readResult)
                    {
                        ++m_numReads;
                    }
                }
            } counter;

            counter.Count(reader.ReadObjectArray("characters", [&](size_t index, JsonReader& reader)
            {
                AZ_UNUSED(index);

                Character c;

                counter.Count(reader.ReadRequiredValue("name", c.name));
                counter.Count(reader.ReadRequiredValue("type", c.type));

                counter.Count(reader.ReadArrayValues<AZStd::string>("gear", [&](size_t index, const AZStd::string& value)
                {
                    EXPECT_EQ(index, c.gear.size());
                    c.gear.push_back(value);
                }));

                counter.Count(reader.ReadRequiredObject("status", [&](JsonReader& reader)
                {
                    counter.Count(reader.ReadRequiredValue("health", c.status.health));
                    counter.Count(reader.ReadOptionalValue("stamina", c.status.stamina, 0.0f));

                    reader.Ignore("comment");
                    reader.Ignore("ignore");
                }));

                counter.Count(reader.ReadOptionalObject("optionalStats", [&](JsonReader& reader)
                {
                    counter.Count(reader.ReadRequiredValue("hitCount", c.optionalStats.hitCount));
                    counter.Count(reader.ReadRequiredValue("healCount", c.optionalStats.healCount));
                }));

                characters.push_back(c);
            }));

            return counter.m_numReads;
        }

    };

    TEST_F(JsonReaderTests, ReadRequiredValue)
    {
        AZStd::string jsonTxt =
        "{                                                   "
        "    \"firstName\": \"Bilbo\",                       "
        "    \"lastName\" : \"Baggins\",                     "
        "    \"age\" : 111,                                  "
        "    \"height\" : 0.9                                "
        "}                                                   ";

        rapidjson::Document document;
        document.Parse(jsonTxt.data());
        EXPECT_FALSE(document.HasParseError());

        JsonReader reader(document);

        AZStd::string firstName;
        AZStd::string lastName;
        int age;
        float height;

        EXPECT_TRUE(reader.ReadRequiredValue("firstName", firstName));
        EXPECT_TRUE(reader.ReadRequiredValue("lastName", lastName));
        EXPECT_TRUE(reader.ReadRequiredValue("age", age));
        EXPECT_TRUE(reader.ReadRequiredValue("height", height));

        EXPECT_EQ(firstName, "Bilbo");
        EXPECT_EQ(lastName, "Baggins");
        EXPECT_EQ(age, 111);
        EXPECT_FLOAT_EQ(0.9f, height);
        EXPECT_EQ(0, reader.GetErrorCount());
    }

    TEST_F(JsonReaderTests, ReadRequiredValue_Missing)
    {
        rapidjson::Document d;
        JsonReader reader(d.Parse("{}"));

        AZStd::string s = "-1";
        int i = -1;
        float f = -1.0f;

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(reader.ReadRequiredValue("myString", s));
        EXPECT_FALSE(reader.ReadRequiredValue("myInt", i));
        EXPECT_FALSE(reader.ReadRequiredValue("myFloat", f));
        AZ_TEST_STOP_ASSERTTEST(3);

        EXPECT_EQ(3, reader.GetErrorCount());

        EXPECT_EQ("-1", s);
        EXPECT_EQ(-1, i);
        EXPECT_EQ(-1.0f, f);
    }

    TEST_F(JsonReaderTests, ReadRequiredValue_WrongType)
    {
        rapidjson::Document d;
        JsonReader reader(d.Parse("{ \"myString\":12, \"myInt\":\"hi\", \"myFloat\":\"there\" }"));

        AZStd::string s = "-1";
        int i = -1;
        float f = -1.0f;

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(reader.ReadRequiredValue("myString", s));
        EXPECT_FALSE(reader.ReadRequiredValue("myInt", i));
        EXPECT_FALSE(reader.ReadRequiredValue("myFloat", f));
        AZ_TEST_STOP_ASSERTTEST(3);

        EXPECT_EQ(3, reader.GetErrorCount());

        EXPECT_EQ("-1", s);
        EXPECT_EQ(-1, i);
        EXPECT_EQ(-1.0f, f);
    }

    TEST_F(JsonReaderTests, ReadOptionalValue)
    {
        AZStd::string jsonTxt =
            "{                               "
            "    \"firstName\": \"Bilbo\",   "
            "    \"lastName\" : \"Baggins\"  "
            "}                               ";

        rapidjson::Document document;
        document.Parse(jsonTxt.data());
        EXPECT_FALSE(document.HasParseError());

        JsonReader reader(document);

        AZStd::string firstName;
        AZStd::string lastName;
        AZStd::string nickname;
        int age = -1;
        float height = -1;

        EXPECT_TRUE(reader.ReadRequiredValue("firstName", firstName));
        EXPECT_TRUE(reader.ReadOptionalValue("lastName", lastName, AZStd::string("")));
        EXPECT_FALSE(reader.ReadOptionalValue("nickname", nickname, firstName));
        EXPECT_FALSE(reader.ReadOptionalValue("age", age, 33));
        EXPECT_FALSE(reader.ReadOptionalValue("height", height, 1.0f));

        EXPECT_EQ(firstName, "Bilbo");
        EXPECT_EQ(lastName, "Baggins");
        EXPECT_EQ(nickname, "Bilbo");
        EXPECT_EQ(age, 33);
        EXPECT_FLOAT_EQ(1.0f, height);
        EXPECT_EQ(0, reader.GetErrorCount());
    }

    TEST_F(JsonReaderTests, ReadOptionalValue_Missing)
    {
        rapidjson::Document d;
        JsonReader reader(d.Parse("{}"));

        AZStd::string s = "-1";
        int i = -1;
        float f = -1.0f;

        EXPECT_FALSE(reader.ReadOptionalValue("myString", s, AZStd::string("invalid")));
        EXPECT_FALSE(reader.ReadOptionalValue("myInt", i, 1));
        EXPECT_FALSE(reader.ReadOptionalValue("myFloat", f, 1.0f));

        EXPECT_EQ(0, reader.GetErrorCount());

        EXPECT_EQ("invalid", s);
        EXPECT_EQ(1, i);
        EXPECT_EQ(1.0f, f);
    }

    TEST_F(JsonReaderTests, ReadOptionalValue_WrongType)
    {
        rapidjson::Document d;
        JsonReader reader(d.Parse("{ \"myString\":12, \"myInt\":\"hi\", \"myFloat\":[1.0] }"));

        AZStd::string s = "-1";
        int i = -1;
        float f = -1.0f;

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(reader.ReadOptionalValue("myString", s, AZStd::string("invalid")));
        EXPECT_FALSE(reader.ReadOptionalValue("myInt", i, 1));
        EXPECT_FALSE(reader.ReadOptionalValue("myFloat", f, 1.0f));
        AZ_TEST_STOP_ASSERTTEST(3);

        EXPECT_EQ(3, reader.GetErrorCount());

        EXPECT_EQ("invalid", s);
        EXPECT_EQ(1, i);
        EXPECT_EQ(1.0f, f);
    }

    TEST_F(JsonReaderTests, ReadObjectsAndArrays)
    {
        AZStd::string jsonTxt =
            "{                                                                              "
            "    \"characters\": [                                                          "
            "        {                                                                      "
            "            \"name\": \"Frodo\",                                               "
            "            \"type\": \"Hobbit\",                                              "
            "            \"gear\": [ \"ring\", \"sword\", \"cloak\" ],                      "
            "            \"status\": {                                                      "
            "                \"health\": 0.5,                                               "
            "                \"stamina\": 0.6                                               "
            "            }                                                                  "
            "        },                                                                     "
            "        {                                                                      "
            "            \"name\": \"Sam\",                                                 "
            "            \"type\": \"Hobbit\",                                              "
            "            \"gear\": [ \"sword\", \"cloak\", \"rope\", \"taters\" ],          "
            "            \"status\": {                                                      "
            "                \"health\": 0.75,                                              "
            "                \"stamina\": 0.8                                               "
            "            }                                                                  "
            "        },                                                                     "
            "        {                                                                      "
            "            \"name\": \"Gandalf\",                                             "
            "            \"type\": \"Wizard\",                                              "
            "            \"gear\": [ \"hat\", \"staff\" ],                                  "
            "            \"status\": {                                                      "
            "                \"health\": 0.9,                                               "
            "                \"stamina\": 0.4                                               "
            "            }                                                                  "
            "        }                                                                      "
            "    ]                                                                          "
            "}                                                                              ";

        rapidjson::Document document;
        document.Parse(jsonTxt.data());
        EXPECT_FALSE(document.HasParseError());

        AZStd::vector<Character> characters;

        JsonReader reader(document);
        size_t numKeysReads = ReadCharacterFile(jsonTxt, reader, characters);

        EXPECT_EQ(0, reader.GetErrorCount());

        EXPECT_EQ(19, numKeysReads);

        EXPECT_EQ(3, characters.size());

        EXPECT_EQ("Frodo", characters[0].name);
        EXPECT_EQ("Hobbit", characters[0].type);
        EXPECT_EQ(3, characters[0].gear.size());
        EXPECT_EQ("ring", characters[0].gear[0]);
        EXPECT_EQ("sword", characters[0].gear[1]);
        EXPECT_EQ("cloak", characters[0].gear[2]);
        EXPECT_FLOAT_EQ(0.5f, characters[0].status.health);
        EXPECT_FLOAT_EQ(0.6f, characters[0].status.stamina);

        EXPECT_EQ("Sam", characters[1].name);
        EXPECT_EQ("Hobbit", characters[1].type);
        EXPECT_EQ(4, characters[1].gear.size());
        EXPECT_EQ("sword", characters[1].gear[0]);
        EXPECT_EQ("cloak", characters[1].gear[1]);
        EXPECT_EQ("rope", characters[1].gear[2]);
        EXPECT_EQ("taters", characters[1].gear[3]);
        EXPECT_FLOAT_EQ(0.75f, characters[1].status.health);
        EXPECT_FLOAT_EQ(0.8f, characters[1].status.stamina);

        EXPECT_EQ("Gandalf", characters[2].name);
        EXPECT_EQ("Wizard", characters[2].type);
        EXPECT_EQ(2, characters[2].gear.size());
        EXPECT_EQ("hat", characters[2].gear[0]);
        EXPECT_EQ("staff", characters[2].gear[1]);
        EXPECT_FLOAT_EQ(0.9f, characters[2].status.health);
        EXPECT_FLOAT_EQ(0.4f, characters[2].status.stamina);
    }

    TEST_F(JsonReaderTests, ReadRequiredObject_Missing)
    {
        rapidjson::Document d;
        JsonReader reader(d.Parse("{}"));

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(reader.ReadRequiredObject("myObject", [](JsonReader& reader)
        {
            AZ_UNUSED(reader);
            ADD_FAILURE(); // This callback shouldn't be called
        }));
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, reader.GetErrorCount());
    }

    TEST_F(JsonReaderTests, ReadRequiredObject_WrongType)
    {
        rapidjson::Document d;
        JsonReader reader(d.Parse("{ \"myObject\":[1,2,3] }"));
        
        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(reader.ReadRequiredObject("myObject", [](JsonReader& reader)
        {
            AZ_UNUSED(reader);
            ADD_FAILURE(); // This callback shouldn't be called
        }));
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, reader.GetErrorCount());
    }

    TEST_F(JsonReaderTests, ReadOptionalObject_Missing)
    {
        rapidjson::Document d;
        JsonReader reader(d.Parse("{}"));

        EXPECT_FALSE(reader.ReadOptionalObject("myObject", [](JsonReader& reader)
        {
            AZ_UNUSED(reader);
            ADD_FAILURE(); // This callback shouldn't be called
        }));

        EXPECT_EQ(0, reader.GetErrorCount());
    }

    TEST_F(JsonReaderTests, ReadOptionalObject_WrongType)
    {
        rapidjson::Document d;
        JsonReader reader(d.Parse("{ \"myObject\":[1,2,3] }"));
        
        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(reader.ReadOptionalObject("myObject", [](JsonReader& reader)
        {
            AZ_UNUSED(reader);
            ADD_FAILURE(); // This callback shouldn't be called
        }));
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, reader.GetErrorCount());
    }

    TEST_F(JsonReaderTests, PropagateErrorCounts)
    {
        // This test is a copy of the "ReadObjectsAndArrays" test, with some modifications

        AZStd::string jsonTxt =
            "{                                                                              "
            "    \"characters\": [                                                          "
            "        {                                                                      "
            "            \"name\": \"Frodo\",                                               "
            "            \"type\": \"Hobbit\",                                              "
            "            \"gear\": [ \"ring\", 1.0, \"cloak\" ],                            " // float instead of string
            "            \"status\": {                                                      "
            //"                \"health\": 0.5                                              " // missing field
            "                \"stamina\": \"great\"                                         " // string instead of float
            "            }                                                                  "
            "        },                                                                     "
            "        {                                                                      "
            "            \"name\": \"Sam\",                                                 "
            "            \"type\": \"Hobbit\",                                              "
            "            \"gear\": [ \"sword\", \"cloak\", \"rope\", \"taters\" ],          "
            "            \"status\": {                                                      "
            "                \"health\": {},                                                " // object instead of string
            "                \"stamina\": []                                                " // array instead of string
            "            }                                                                  "
            "        },                                                                     "
            "        {                                                                      "
            "            \"name\": \"Gandalf\",                                             "
            "            \"type\": \"Wizard\",                                              "
            "            \"gear\": [ \"hat\", \"staff\" ],                                  "
            "            \"status\": {                                                      "
            "                \"health\": 0.9,                                               "
            "                \"stamina\": 0.4                                               "
            "            },                                                                 " 
            "            \"optionalStats\": {}                                              " // Optional object is present but is missing 2 required fields
            "        }                                                                      "
            "    ]                                                                          "
            "}                                                                              ";

        rapidjson::Document document;
        document.Parse(jsonTxt.data());
        EXPECT_FALSE(document.HasParseError());

        AZStd::vector<Character> characters;

        JsonReader reader(document);

        AZ_TEST_START_ASSERTTEST;
        size_t numKeysReads = ReadCharacterFile(jsonTxt, reader, characters);
        AZ_TEST_STOP_ASSERTTEST(7);

        EXPECT_EQ(7, reader.GetErrorCount());
        EXPECT_EQ(15, numKeysReads);
    }

    TEST_F(JsonReaderTests, ReportErrorsForUnrecognizedFields)
    {
        // This test is a copy of the "ReadObjectsAndArrays" test, with some modifications

        // Note that we also test the Ignore() function to allow unused fields.

        AZStd::string jsonTxt =
            "{                                                                              "
            "    \"characters\": [                                                          "
            "        {                                                                      "
            "            \"nam\": \"Frodo\",                                                " // Misspelled required "name" (2 errors)
            "            \"type\": \"Hobbit\",                                              "
            "            \"gear\": [ \"ring\", \"sword\", \"cloak\" ],                      "
            "            \"status\": {                                                      "
            "                \"comment\": \"I wish it never came to me\",                   " // Testing Ignore()
            "                \"health\": 0.5,                                               "
            "                \"stamna\": 0.6                                                " // Misspelled optional "stamina" (1 error)
            "            }                                                                  "
            "        },                                                                     "
            "        {                                                                      "
            "            \"name\": \"Sam\",                                                 "
            "            \"type\": [],                                                      " // Key with incorrect type is still "recognized" (1 error)
            "            \"gear\": [ \"sword\", \"cloak\", \"rope\", \"taters\" ],          "
            "            \"status\": {                                                      "
            "                \"health\": 0.75,                                              "
            "                \"ignore\": \"I could carry it for a while\"                   " // Testing Ignore()
            "            }                                                                  "
            "        },                                                                     "
            "        {                                                                      "
            "            \"name\": \"Gandalf\",                                             "
            "            \"type\": \"Wizard\",                                              "
            "            \"gear\": [ \"hat\", \"staff\" ],                                  "
            "            \"status\": {                                                      "
            "                \"heath\": 0.9,                                                " // Misspelled required "health" (2 errors)
            "                \"stamina\": 0.4                                               "
            "            },                                                                 "
            "            \"optionalStats\": {                                               "
            "                \"hitCount\":20,                                               "
            "                \"healCount\":10,                                              "
            "                \"napCount\":2                                                 " // Unexpected "napCount", this time inside an optional object (1 errors)
            "            }                                                                  "
            "        }                                                                      "
            "    ],                                                                         "
            "    \"extra\": {}                                                              " // Extra section unexpected (1 error)
            "}                                                                              ";
        
        rapidjson::Document document;
        document.Parse(jsonTxt.data());
        EXPECT_FALSE(document.HasParseError());

        // Read the document with AllowUnrecognizedKeys disabled
        {
            AZStd::vector<Character> characters;

            JsonReader reader(document);

            AZ_TEST_START_ASSERTTEST;
            reader.SetAllowUnrecognizedKeys(false);
            size_t numKeysReads = ReadCharacterFile(jsonTxt, reader, characters);
            EXPECT_FALSE(reader.CheckUnrecognizedKeys());
            AZ_TEST_STOP_ASSERTTEST(8);

            EXPECT_EQ(8, reader.GetErrorCount());
            EXPECT_EQ(17, numKeysReads);
        }

        // Read the document again, but this time ignore the unrecognized fields. There a still a couple errors
        // where required keys are misspelled, but no errors for extra keys.
        {
            AZStd::vector<Character> characters;

            JsonReader reader(document);

            AZ_TEST_START_ASSERTTEST;
            size_t numKeysReads = ReadCharacterFile(jsonTxt, reader, characters);
            EXPECT_TRUE(reader.CheckUnrecognizedKeys());
            AZ_TEST_STOP_ASSERTTEST(3);

            EXPECT_EQ(3, reader.GetErrorCount());
            EXPECT_EQ(17, numKeysReads);
        }
    }

    /**
     * Test value-reading functions against a particular data type.
     * @param jsonValue  A string that will appear in a sample json file, representing a value of type DataType.
     * @param encodedValue  The actual value that is encoded in jsonValue
     * @param defaultValue  A default value that can be used when testing ReadOptionalValue(). This should not be the same value as encodedValue.
     */
    template<typename DataType, typename DefaultType=DataType>
    void TestReadValue(AZStd::string_view jsonValue, const DataType& encodedValue, const DefaultType& defaultValue)
    {
        AZStd::string jsonTxt = AZStd::string::format("{ \"value\": %s }", jsonValue.data());

        rapidjson::Document document;
        document.Parse(jsonTxt.data());
        EXPECT_FALSE(document.HasParseError());

        // ReadRequiredValue
        {
            // Normal value
            {
                JsonReader reader(document);
                DataType value;
                EXPECT_TRUE(reader.ReadRequiredValue("value", value));
                EXPECT_EQ(encodedValue, value);
            }

            // AZStd::any value
            {
                JsonReader reader(document);
                AZStd::any value;
                EXPECT_TRUE(reader.ReadRequiredAny<DataType>("value", value));
                EXPECT_EQ(encodedValue, AZStd::any_cast<DataType>(value));
            }
        }

        // ReadRequiredValue should fail for absent key
        {
            // Normal value
            {
                JsonReader reader(document);
                DataType value;
                AZ_TEST_START_ASSERTTEST;
                EXPECT_FALSE(reader.ReadRequiredValue("valueAbsent", value));
                AZ_TEST_STOP_ASSERTTEST(1);
                EXPECT_EQ(1, reader.GetErrorCount());
            }

            // AZStd::any value
            {
                JsonReader reader(document);
                AZStd::any value;
                AZ_TEST_START_ASSERTTEST;
                EXPECT_FALSE(reader.ReadRequiredAny<DataType>("valueAbsent", value));
                AZ_TEST_STOP_ASSERTTEST(1);
                EXPECT_EQ(1, reader.GetErrorCount());
            }
        }

        // ReadOptionalValue, key is present
        {
            // Normal value
            {
                // With default specified
                {
                    JsonReader reader(document);
                    DataType value;
                    EXPECT_TRUE(reader.ReadOptionalValue("value", value, defaultValue));
                    EXPECT_EQ(encodedValue, value);
                }

                // Without default specified
                {
                    JsonReader reader(document);
                    DataType value;
                    EXPECT_TRUE(reader.ReadOptionalValue("value", value));
                    EXPECT_EQ(encodedValue, value);
                }
            }

            // AZStd::any value
            {
                // With default specified
                {
                    JsonReader reader(document);
                    AZStd::any value;
                    EXPECT_TRUE(reader.ReadOptionalAny<DataType>("value", value, defaultValue));
                    EXPECT_EQ(encodedValue, AZStd::any_cast<DataType>(value));
                }

                // Without default specified
                {
                    JsonReader reader(document);
                    AZStd::any value;
                    EXPECT_TRUE(reader.ReadOptionalAny<DataType>("value", value));
                    EXPECT_EQ(encodedValue, AZStd::any_cast<DataType>(value));
                }
            }
        }

        // ReadOptionalValue, key is absent
        {
            // Normal value
            {
                // With default specified
                {
                    JsonReader reader(document);
                    DataType value;
                    EXPECT_FALSE(reader.ReadOptionalValue("valueAbsent", value, defaultValue));
                    EXPECT_EQ(defaultValue, value);
                }

                // Without default specified
                {
                    JsonReader reader(document);
                    DataType value = defaultValue;
                    EXPECT_FALSE(reader.ReadOptionalValue("valueAbsent", value));
                    EXPECT_EQ(defaultValue, value);
                }
            }

            // AZStd::any value
            {
                // With default specified
                {
                    JsonReader reader(document);
                    AZStd::any value;
                    EXPECT_FALSE(reader.ReadOptionalAny<DataType>("valueAbsent", value, defaultValue));
                    EXPECT_EQ(defaultValue, AZStd::any_cast<DataType>(value));
                }

                // Without default specified
                {
                    JsonReader reader(document);
                    AZStd::any value{defaultValue};
                    EXPECT_FALSE(reader.ReadOptionalAny<DataType>("valueAbsent", value));
                    EXPECT_EQ(defaultValue, AZStd::any_cast<DataType>(value));
                }
            }
        }
    }

    /**
    * Test ReadArrayValues() function against a particular data type.
    * @param jsonValue  A string that will appear in a sample json file, representing a single value of type DataType.
    * @param encodedValue  The actual value that is encoded in jsonValue
    */
    template<typename DataType>
    void TestReadValueArray(AZStd::string_view jsonValue, const DataType& encodedValue)
    {
        AZStd::string jsonTxt = AZStd::string::format("{ \"valueArray\": [%s] }", jsonValue.data());

        rapidjson::Document document;
        document.Parse(jsonTxt.data());
        EXPECT_FALSE(document.HasParseError());

        JsonReader reader(document);
        DataType value;
        EXPECT_TRUE(reader.ReadArrayValues<DataType>("valueArray", [&value](size_t, const DataType& item)
        {
            value = item;
        }));
        EXPECT_EQ(encodedValue, value);
    }

    /// Convenience function for calling both TestReadValue() and TestReadValueArray()
    template<typename DataType>
    void TestReadValueAndArray(AZStd::string_view jsonValue, const DataType& encodedValue, const DataType& defaultValue)
    {
        TestReadValue<DataType>(jsonValue, encodedValue, defaultValue);
        TestReadValueArray<DataType>(jsonValue, encodedValue);
    }

    /**
    * Test value-reading functions where the json text is invalid.
    * @param jsonValue  A string that will appear in a sample json file, having the wrong number of elements.
    * @param defaultValue  A default value that can be used when testing ReadOptionalValue().
    */
    template<typename DataType>
    void TestReadValue_InvalidData(AZStd::string_view jsonValue, const DataType& defaultValue)
    {
        AZStd::string jsonTxt = AZStd::string::format("{ \"value\": %s }", jsonValue.data());

        rapidjson::Document document;
        document.Parse(jsonTxt.data());
        EXPECT_FALSE(document.HasParseError());

        // ReadRequiredValue
        {
            JsonReader reader(document);
            DataType value;
            AZ_TEST_START_ASSERTTEST;
            EXPECT_FALSE(reader.ReadRequiredValue("value", value));
            AZ_TEST_STOP_ASSERTTEST(1);
            EXPECT_EQ(1, reader.GetErrorCount());
        }

        // ReadOptionalValue
        {
            JsonReader reader(document);
            DataType value;
            AZ_TEST_START_ASSERTTEST;
            EXPECT_FALSE(reader.ReadOptionalValue("value", value, defaultValue));
            AZ_TEST_STOP_ASSERTTEST(1);
            EXPECT_EQ(1, reader.GetErrorCount());
            EXPECT_EQ(defaultValue, value);
        }
    }
    
    TEST_F(JsonReaderTests, TestAllDataTypes)
    {
        // Basic data types (these support arrays of values)...

        TestReadValueAndArray<bool>("true", true, false);
        TestReadValueAndArray<bool>("false", false, true);

        TestReadValueAndArray<int32_t>("10", 10, -1);
        TestReadValueAndArray<int32_t>("-10", -10, -1);
        TestReadValueAndArray<int32_t>("2147483647", INT_MAX, -1);
        TestReadValueAndArray<int32_t>("-2147483648", INT_MIN, -1);

        TestReadValueAndArray<uint32_t>("42", 42, 1);
        TestReadValueAndArray<uint32_t>("2147483647", INT_MAX, 1);
        TestReadValueAndArray<uint32_t>("4294967295", UINT_MAX, 1);

        TestReadValueAndArray<float>("1.5", 1.5f, 1.0f);

        TestReadValueAndArray<AZStd::string>("\"Hello World!\"", "Hello World!", "<empty>");

        // Vector data types (these do not support arrays of values)...

        TestReadValue<AZ::Vector2>("[1.0, 1.5]", AZ::Vector2(1.0f, 1.5f), AZ::Vector2(0.0f, 1.0f));
        TestReadValue_InvalidData<AZ::Vector2>("[1.0]", AZ::Vector2(0.0f, 0.0f));           // Too few elements
        TestReadValue_InvalidData<AZ::Vector2>("[1.0, 1.0, 1.0]", AZ::Vector2(0.0f, 0.0f)); // Too many elements
        TestReadValue_InvalidData<AZ::Vector2>("[1.0, false]", AZ::Vector2(0.0f, 1.0f));    // wrong type for one element

        TestReadValue<AZ::Vector3>("[2.0, 2.5, 3.0]", AZ::Vector3(2.0f, 2.5f, 3.0f), AZ::Vector3(1.0f, 1.0f, 1.0f));
        TestReadValue_InvalidData<AZ::Vector3>("[2.0, 2.5]", AZ::Vector3(1.0f, 1.0f, 1.0f));            // Too few elements
        TestReadValue_InvalidData<AZ::Vector3>("[2.0, 2.5, 3.0, 0.0]", AZ::Vector3(1.0f, 1.0f, 1.0f));  // Too many elements
        TestReadValue_InvalidData<AZ::Vector3>("[2.0, {}, 3.0]", AZ::Vector3(1.0f, 1.0f, 1.0f));        // wrong type for one element

        TestReadValue<AZ::Vector4>("[1.5, 2.0, 2.5, 3.0]", AZ::Vector4(1.5, 2.0f, 2.5f, 3.0f), AZ::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        TestReadValue_InvalidData<AZ::Vector4>("[1.5, 2.0, 2.5]", AZ::Vector4(1.0f, 1.0f, 1.0f, 1.0f));             // Too few elements
        TestReadValue_InvalidData<AZ::Vector4>("[1.5, 2.0, 2.5, 3.0, 0.0]", AZ::Vector4(1.0f, 1.0f, 1.0f, 1.0f));   // Too many elements
        TestReadValue_InvalidData<AZ::Vector4>("[1.5, 2.0, [], 3.0]", AZ::Vector4(1.0f, 1.0f, 1.0f, 1.0f));         // wrong type for one element

        // For colors, we allow either 3 or 4 elements, with the alpha channel defaulting to 1.
        TestReadValue<AZ::Color>("[0.1, 0.2, 0.3, 0.5]", AZ::Color(0.1f, 0.2f, 0.3f, 0.5f), AZ::Color(0.0f, 0.0f, 0.0f, 1.0f));
        TestReadValue<AZ::Color>("[0.1, 0.2, 0.3]", AZ::Color(0.1f, 0.2f, 0.3f, 1.0f), AZ::Color(0.0f, 0.0f, 0.0f, 1.0f));
        TestReadValue_InvalidData<AZ::Color>("[0.1, 0.2]", AZ::Color(1.0f, 1.0f, 1.0f, 1.0f));                   // Too few elements
        TestReadValue_InvalidData<AZ::Color>("[0.1, 0.2, 0.3, 0.5, 1.0]", AZ::Color(1.0f, 1.0f, 1.0f, 1.0f));    // Too many elements
        TestReadValue_InvalidData<AZ::Color>("[0.1, true, 0.3, 0.5]", AZ::Color(0.0f, 0.0f, 0.0f, 1.0f));        // wrong type for one element
    }

    enum class Difficulty
    {
        Easy,
        Normal,
        Hard,
        Invalid
    };

    bool ParseDifficulty(AZStd::string_view str, Difficulty& value)
    {
        if (str == "Easy")
        {
            value = Difficulty::Easy;
        }
        else if (str == "Normal")
        {
            value = Difficulty::Normal;
        }
        else if (str == "Hard")
        {
            value = Difficulty::Hard;
        }
        else if (str == "Invalid")
        {
            value = Difficulty::Invalid;
        }
        else
        {
            value = Difficulty::Invalid;
            return false;
        }

        return true;
    }

    TEST_F(JsonReaderTests, ReadEnum)
    {
        AZStd::string jsonTxt =
            "{                                                                  "
            "    \"a\": \"Easy\",                                               "
            "    \"b\": \"Normal\",                                             "
            "    \"c\": \"Hard\",                                               "
            "    \"typo\": \"Eas\",                                             "
            "    \"wrongType\": {},                                             "
            "    \"array\": [ \"Invalid\", \"Easy\", \"Hard\", \"Normal\" ],    "
            "    \"typoArray\": [ \"Invalid\", \"Easy\", \"Hrd\", \"Normal\" ], "
            "    \"wrongTypeArray\": [ \"Invalid\", 0, \"Hard\", \"Normal\" ]   "
            "}                                                                  ";

        rapidjson::Document document;
        document.Parse(jsonTxt.data());
        EXPECT_FALSE(document.HasParseError());

        Difficulty difficulty;

        // Test ReadRequiredEnum
        {
            JsonReader reader(document);

            EXPECT_TRUE(reader.ReadRequiredEnum<Difficulty>("a", &ParseDifficulty, difficulty));
            EXPECT_EQ(Difficulty::Easy, difficulty);

            EXPECT_TRUE(reader.ReadRequiredEnum<Difficulty>("b", &ParseDifficulty, difficulty));
            EXPECT_EQ(Difficulty::Normal, difficulty);

            EXPECT_TRUE(reader.ReadRequiredEnum<Difficulty>("c", &ParseDifficulty, difficulty));
            EXPECT_EQ(Difficulty::Hard, difficulty);

            AZ_TEST_START_ASSERTTEST;
            EXPECT_FALSE(reader.ReadRequiredEnum<Difficulty>("missing", &ParseDifficulty, difficulty));
            EXPECT_FALSE(reader.ReadRequiredEnum<Difficulty>("typo", &ParseDifficulty, difficulty));
            EXPECT_FALSE(reader.ReadRequiredEnum<Difficulty>("wrongType", &ParseDifficulty, difficulty));
            AZ_TEST_STOP_ASSERTTEST(3);
            EXPECT_EQ(3, reader.GetErrorCount());
        }

        // Test ReadOptionalEnum
        {
            JsonReader reader(document);

            EXPECT_TRUE(reader.ReadOptionalEnum<Difficulty>("a", &ParseDifficulty, difficulty, Difficulty::Invalid));
            EXPECT_EQ(Difficulty::Easy, difficulty);

            EXPECT_TRUE(reader.ReadOptionalEnum<Difficulty>("b", &ParseDifficulty, difficulty, Difficulty::Invalid));
            EXPECT_EQ(Difficulty::Normal, difficulty);

            EXPECT_TRUE(reader.ReadOptionalEnum<Difficulty>("c", &ParseDifficulty, difficulty, Difficulty::Invalid));
            EXPECT_EQ(Difficulty::Hard, difficulty);

            EXPECT_FALSE(reader.ReadOptionalEnum<Difficulty>("missing", &ParseDifficulty, difficulty, Difficulty::Invalid));
            EXPECT_EQ(Difficulty::Invalid, difficulty);
            
            AZ_TEST_START_ASSERTTEST;

            difficulty = Difficulty::Easy;
            EXPECT_FALSE(reader.ReadOptionalEnum<Difficulty>("typo", &ParseDifficulty, difficulty, Difficulty::Invalid));
            EXPECT_EQ(Difficulty::Invalid, difficulty);

            difficulty = Difficulty::Easy;
            EXPECT_FALSE(reader.ReadOptionalEnum<Difficulty>("wrongType", &ParseDifficulty, difficulty, Difficulty::Invalid));
            EXPECT_EQ(Difficulty::Invalid, difficulty);

            AZ_TEST_STOP_ASSERTTEST(2);
            EXPECT_EQ(2, reader.GetErrorCount());
        }

        // Test ReadOptionalEnum
        {
            AZStd::vector<Difficulty> array;

            JsonReader reader(document);

            auto handleArrayElement = [&](size_t index, const Difficulty& value)
            {
                AZ_UNUSED(index);
                array.push_back(value);
            };

            array.clear();
            EXPECT_TRUE(reader.ReadArrayEnums<Difficulty>("array", &ParseDifficulty, handleArrayElement));
            EXPECT_EQ(4, array.size());
            EXPECT_EQ(Difficulty::Invalid, array[0]);
            EXPECT_EQ(Difficulty::Easy, array[1]);
            EXPECT_EQ(Difficulty::Hard, array[2]);
            EXPECT_EQ(Difficulty::Normal, array[3]);

            AZ_TEST_START_ASSERTTEST;
            EXPECT_FALSE(reader.ReadArrayEnums<Difficulty>("typoArray", &ParseDifficulty, handleArrayElement));
            EXPECT_FALSE(reader.ReadArrayEnums<Difficulty>("wrongTypeArray", &ParseDifficulty, handleArrayElement));
            AZ_TEST_STOP_ASSERTTEST(2);
            EXPECT_EQ(2, reader.GetErrorCount());
        }

    }

    TEST_F(JsonReaderTests, TestReportError)
    {
        rapidjson::Document d;
        JsonReader reader(d.Parse("{ \"object\": { \"array\": [ {} ] } }"));

        AZ_TEST_START_ASSERTTEST;
        reader.ReportError("Error %d", 1);
        reader.ReadRequiredObject("object", [](JsonReader& reader)
        {
            reader.ReportError("Error %d", 2);

            reader.ReadObjectArray("array", [](size_t index, JsonReader& reader)
            {
                AZ_UNUSED(index);
                reader.ReportError("Error %d", 3);
            });
        });
        AZ_TEST_STOP_ASSERTTEST(3);

        EXPECT_EQ(3, reader.GetErrorCount());
    }

    TEST_F(JsonReaderTests, TestReadMembers)
    {
        rapidjson::Document d;
        JsonReader reader(d.Parse("{\"a\": 1,\"b\" : 2,\"c\" : 3}"));

        int a, b, c;

        reader.ReadMembers([&](const char* key, JsonReader& reader)
        {
            AZ_UNUSED(key);
            reader.ReadRequiredValue("a", a);
            reader.ReadRequiredValue("b", b);
            reader.ReadRequiredValue("c", c);
        });

        EXPECT_EQ(1, a);
        EXPECT_EQ(2, b);
        EXPECT_EQ(3, c);
    }

} // namespace UnitTest

