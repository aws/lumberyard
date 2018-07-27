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
#include "UserTypes.h"

#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/tokenize.h>
#include <AzCore/std/string/alphanum.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/std/string/wildcard.h>

// we need this for AZ_TEST_FLOAT compare
#include <float.h>

using namespace AZStd;

// Because of the SSO (small string optimization) we always shoule have capacity != 0 and data != 0
#define AZ_TEST_VALIDATE_EMPTY_STRING(_String)        \
    EXPECT_TRUE(_String.validate());               \
    EXPECT_EQ(0, _String.size());              \
    EXPECT_TRUE(_String.empty());                  \
    EXPECT_TRUE(_String.begin() == _String.end()); \
    EXPECT_NE(0, _String.capacity());          \
    EXPECT_NE(nullptr, _String.data());              \
    EXPECT_EQ(0, strlen(_String.c_str()));     \
    EXPECT_TRUE(_String.data() == _String.c_str())

#define AZ_TEST_VALIDATE_STRING(_String, _NumElements)                                                        \
    EXPECT_TRUE(_String.validate());                                                                       \
    EXPECT_EQ(_NumElements, _String.size());                                                           \
    EXPECT_TRUE((_String.size() > 0) ? !_String.empty() : _String.empty());                                \
    EXPECT_TRUE((_NumElements > 0) ? _String.capacity() >= _NumElements : true);                           \
    EXPECT_TRUE((_NumElements > 0) ? _String.begin() != _String.end() : _String.begin() == _String.end()); \
    EXPECT_FALSE(_String.empty());                                                                         \
    EXPECT_NE(nullptr, _String.data());                                                                      \
    EXPECT_EQ(_NumElements, strlen(_String.c_str()));

#define AZ_TEST_VALIDATE_WSTRING(_String, _NumElements)                                                       \
    EXPECT_TRUE(_String.validate());                                                                       \
    EXPECT_EQ(_NumElements, _String.size());                                                           \
    EXPECT_TRUE((_String.size() > 0) ? !_String.empty() : _String.empty());                                \
    EXPECT_TRUE((_NumElements > 0) ? _String.capacity() >= _NumElements : true);                           \
    EXPECT_TRUE((_NumElements > 0) ? _String.begin() != _String.end() : _String.begin() == _String.end()); \
    EXPECT_FALSE(_String.empty());                                                                         \
    EXPECT_NE(nullptr, _String.data());                                                                      \
    EXPECT_EQ(_NumElements, wcslen(_String.c_str()));

#if defined(AZ_COMPILER_MSVC) // just for a test purpose (we actully leak the memory)
# define strdup _strdup
#endif

namespace UnitTest
{
    void TestVSNPrintf(char* buffer, size_t bufferSize, const char* format, ...)
    {
        va_list mark;
        va_start(mark, format);
        azvsnprintf(buffer, bufferSize, format, mark);
        va_end(mark);
    }

    void TestVSWNPrintf(wchar_t* buffer, size_t bufferSize, const wchar_t* format, ...)
    {
        va_list mark;
        va_start(mark, format);
        azvsnwprintf(buffer, bufferSize, format, mark);
        va_end(mark);
    }

    TEST(StringC, VSNPrintf)
    {
        char buffer32[32];
        TestVSNPrintf(buffer32, AZ_ARRAY_SIZE(buffer32), "This is a buffer test %s", "Bla2");
        EXPECT_EQ(0, strcmp(buffer32, "This is a buffer test Bla2"));
    }

    TEST(StringC, VSWNPrintf)
    {
        wchar_t wbuffer32[32];
        TestVSWNPrintf(wbuffer32, AZ_ARRAY_SIZE(wbuffer32), L"This is a buffer test %ls", L"Bla3");
        AZ_TEST_ASSERT(wcscmp(wbuffer32, L"This is a buffer test Bla3") == 0);
    }

    TEST(StringC, AZSNPrintf)
    {
        char buffer32[32];
        azsnprintf(buffer32, AZ_ARRAY_SIZE(buffer32), "This is a buffer test %s", "Bla");
        EXPECT_EQ(0, strcmp(buffer32, "This is a buffer test Bla"));
    }

    TEST(StringC, AZSWNPrintf)
    {
        wchar_t wbuffer32[32];
        azsnwprintf(wbuffer32, AZ_ARRAY_SIZE(wbuffer32), L"This is a buffer test %ls", L"Bla1");
        EXPECT_EQ(0, wcscmp(wbuffer32, L"This is a buffer test Bla1"));
    }

    TEST(StringC, AZStrcat)
    {
        char buffer32[32];
        azsnprintf(buffer32, AZ_ARRAY_SIZE(buffer32), "This is a buffer test %s", "Bla2");
        azstrcat(buffer32, AZ_ARRAY_SIZE(buffer32), "_1");
        EXPECT_EQ(0, strcmp(buffer32, "This is a buffer test Bla2_1"));
    }

    TEST(StringC, AZStrncat)
    {
        char buffer32[32];
        azsnprintf(buffer32, AZ_ARRAY_SIZE(buffer32), "This is a buffer test %s", "Bla2");
        azstrncat(buffer32, AZ_ARRAY_SIZE(buffer32), "_23", 2);
        EXPECT_EQ(0, strcmp(buffer32, "This is a buffer test Bla2_2"));
    }

    TEST(StringC, AZStrcpy)
    {
        char buffer32[32];
        azstrcpy(buffer32, AZ_ARRAY_SIZE(buffer32), "Bla Bla 1");
        EXPECT_EQ(0, strcmp(buffer32, "Bla Bla 1"));
    }

    TEST(StringC, AZStrncpy)
    {
        char buffer32[32];
        azstrncpy(buffer32, AZ_ARRAY_SIZE(buffer32), "Gla Gla 1", 7);
        // azstrncpy note: if count is reached before the entire array src was copied, the resulting character array is not null-terminated.
        buffer32[7] = '\0';
        EXPECT_EQ(0, strcmp(buffer32, "Gla Gla"));
    }

    TEST(StringC, AZStricmp)
    {
        char buffer32[32];
        azstrncpy(buffer32, AZ_ARRAY_SIZE(buffer32), "Gla Gla 1", 7);
        // azstrncpy note: if count is reached before the entire array src was copied, the resulting character array is not null-terminated.
        buffer32[7] = '\0';
        EXPECT_EQ(0, azstricmp(buffer32, "gla gla"));
    }

    TEST(StringC, AZStrnicmp)
    {
        char buffer32[32];
        azstrncpy(buffer32, AZ_ARRAY_SIZE(buffer32), "Gla Gla 1", 7);
        EXPECT_EQ(0, azstrnicmp(buffer32, "gla", 3));
    }

    class String
        : public AllocatorsFixture
    {
    };

    TEST_F(String, Basic)
    {
        const char* sChar = "SSO string";  // 10 characters
        const char* sCharLong = "This is a long string test that will allocate";  // 45 characters
        array<char, 6> aChar = {
            { 'a', 'b', 'c', 'd', 'e', 'f' }
        };                                                 // short string (should use SSO)

        string str1;
        AZ_TEST_VALIDATE_EMPTY_STRING(str1);

        // short char*
        string str2(sChar);
        AZ_TEST_VALIDATE_STRING(str2, 10);

        string str2_1("");
        AZ_TEST_VALIDATE_EMPTY_STRING(str2_1);

        string str3(sChar, 5);
        AZ_TEST_VALIDATE_STRING(str3, 5);

        // long char*
        string str4(sCharLong);
        AZ_TEST_VALIDATE_STRING(str4, 45);

        string str5(sCharLong, 35);
        AZ_TEST_VALIDATE_STRING(str5, 35);

        // element
        string str6(13, 'a');
        AZ_TEST_VALIDATE_STRING(str6, 13);

        string str6_1(0, 'a');
        AZ_TEST_VALIDATE_EMPTY_STRING(str6_1);

        string str7(aChar.begin(), aChar.end());
        AZ_TEST_VALIDATE_STRING(str7, 6);

        string str7_1(aChar.begin(), aChar.begin());
        AZ_TEST_VALIDATE_EMPTY_STRING(str7_1);

        string str8(sChar, sChar + 3);
        AZ_TEST_VALIDATE_STRING(str8, 3);

        string str8_1(sChar, sChar);
        AZ_TEST_VALIDATE_EMPTY_STRING(str8_1);

        //
        string str9(str2);
        AZ_TEST_VALIDATE_STRING(str9, 10);

        string str9_1(str1);
        AZ_TEST_VALIDATE_EMPTY_STRING(str9_1);

        string str10(str2, 4);
        AZ_TEST_VALIDATE_STRING(str10, 6);

        string str11(str2, 4, 3);
        AZ_TEST_VALIDATE_STRING(str11, 3);

#if defined(AZ_HAS_RVALUE_REFS)
        string str12(sChar);
        string large = sCharLong;

        // move ctor
        string strSm = AZStd::move(str12);
        AZ_TEST_VALIDATE_STRING(strSm, 10);
        AZ_TEST_VALIDATE_EMPTY_STRING(str12);

        string strLg(AZStd::move(large));
        AZ_TEST_VALIDATE_STRING(strLg, 45);
        AZ_TEST_VALIDATE_EMPTY_STRING(large);

        string strEmpty(AZStd::move(str1));
        AZ_TEST_VALIDATE_EMPTY_STRING(strEmpty);
        AZ_TEST_VALIDATE_EMPTY_STRING(str1);

        // move assign
        str12 = sChar;      // reset
        large = sCharLong;
        // move small string into small string
        strSm = AZStd::move(str12);
        AZ_TEST_VALIDATE_STRING(strSm, 10);
        AZ_TEST_VALIDATE_EMPTY_STRING(str12);
        // move large string into large string
        strLg = AZStd::move(large);
        AZ_TEST_VALIDATE_STRING(strLg, 45);
        AZ_TEST_VALIDATE_EMPTY_STRING(large);

        str12 = sChar;      // reset
        large = sCharLong;
        // move large string into small string
        strSm = AZStd::move(large);
        AZ_TEST_VALIDATE_STRING(strSm, 45);
        AZ_TEST_VALIDATE_EMPTY_STRING(large);
        // move small string into large string
        strLg = AZStd::move(str12);
        AZ_TEST_VALIDATE_STRING(strLg, 10);
        AZ_TEST_VALIDATE_EMPTY_STRING(str12);

        // clear small and large, validate empty
        strSm.clear();
        AZ_TEST_VALIDATE_EMPTY_STRING(strSm);
        strLg.clear();
        AZ_TEST_VALIDATE_EMPTY_STRING(strLg);
#endif // AZ_HAS_RVALUE_REFS

        str2.append(str3);
        AZ_TEST_VALIDATE_STRING(str2, 15);
        AZ_TEST_ASSERT(str2[10] == 'S');

        str3.append(str4, 10, 4);
        AZ_TEST_VALIDATE_STRING(str3, 9);
        AZ_TEST_ASSERT(str3[5] == 'l');

        str4.append(sChar);
        AZ_TEST_VALIDATE_STRING(str4, 55);
        AZ_TEST_ASSERT(str4[45] == 'S');

        str5.append(10, 'b');
        AZ_TEST_VALIDATE_STRING(str5, 45);
        AZ_TEST_ASSERT(str5[35] == 'b');

        str6.append(aChar.begin(), aChar.end());
        AZ_TEST_VALIDATE_STRING(str6, 19);
        AZ_TEST_ASSERT(str6[14] == 'b');

        str7.append(sCharLong + 10, sCharLong + 14);
        AZ_TEST_VALIDATE_STRING(str7, 10);
        AZ_TEST_ASSERT(str7[6] == 'l');

        str2.assign(str9);
        AZ_TEST_VALIDATE_STRING(str2, 10);
        AZ_TEST_ASSERT(str2[0] == 'S');

        str3.assign(str9, 5, 5);
        AZ_TEST_VALIDATE_STRING(str3, 5);
        AZ_TEST_ASSERT(str3[0] == 't');

        str2.assign(sCharLong, 25);
        AZ_TEST_VALIDATE_STRING(str2, 25);
        AZ_TEST_ASSERT(str2[10] == 'l');

        str2.assign(sChar);
        AZ_TEST_VALIDATE_STRING(str2, 10);
        AZ_TEST_ASSERT(str2[0] == 'S');

        str2.assign(5, 'a');
        AZ_TEST_VALIDATE_STRING(str2, 5);
        AZ_TEST_ASSERT(str2[4] == 'a');

        str2.assign(aChar.begin(), aChar.end());
        AZ_TEST_VALIDATE_STRING(str2, 6);
        AZ_TEST_ASSERT(str2[1] == 'b');

        str2.assign(sChar, sChar + 5);
        AZ_TEST_VALIDATE_STRING(str2, 5);
        AZ_TEST_ASSERT(str2[0] == 'S');

        str1.clear();
        AZ_TEST_VALIDATE_EMPTY_STRING(str1);

        str1.assign(sCharLong);
        AZ_TEST_VALIDATE_STRING(str1, 45);
        AZ_TEST_ASSERT(str1[10] == 'l');

        str1.insert(10, str9, 4, 6);
        AZ_TEST_VALIDATE_STRING(str1, 51);
        AZ_TEST_ASSERT(str1[10] == 's');

        str2.insert(3, sChar, 5);
        AZ_TEST_VALIDATE_STRING(str2, 10);
        AZ_TEST_ASSERT(str2[0] == 'S');
        AZ_TEST_ASSERT(str2[3] == 'S');

        str2.insert(5, sChar);
        AZ_TEST_VALIDATE_STRING(str2, 20);
        AZ_TEST_ASSERT(str2[4] == 'S');

        str2.insert(10, 5, 'g');
        AZ_TEST_VALIDATE_STRING(str2, 25);
        AZ_TEST_ASSERT(str2[10] == 'g');
        AZ_TEST_ASSERT(str2[14] == 'g');

        str2.insert(str2.end(), 'b');
        AZ_TEST_VALIDATE_STRING(str2, 26);
        AZ_TEST_ASSERT(str2[25] == 'b');

        str2.insert(str2.end(), 2, 'c');
        AZ_TEST_VALIDATE_STRING(str2, 28);
        AZ_TEST_ASSERT(str2[26] == 'c');
        AZ_TEST_ASSERT(str2[27] == 'c');

        str2.insert(str2.begin(), aChar.begin(), aChar.end());
        AZ_TEST_VALIDATE_STRING(str2, 34);
        AZ_TEST_ASSERT(str2[0] == 'a');
        AZ_TEST_ASSERT(str2[1] == 'b');

        str2.erase(16, 5);
        AZ_TEST_VALIDATE_STRING(str2, 29);
        AZ_TEST_ASSERT(str2[10] != 'g');
        AZ_TEST_ASSERT(str2[14] != 'g');

        str2.erase(str2.begin());
        AZ_TEST_VALIDATE_STRING(str2, 28);
        AZ_TEST_ASSERT(str2[0] == 'b');

        str2.erase(str2.begin(), next(str2.begin(), 4));
        AZ_TEST_VALIDATE_STRING(str2, 24);
        AZ_TEST_ASSERT(str2[0] == 'f');

        str1.assign(aChar.begin(), aChar.end());

        str2.replace(1, 6, str1);
        AZ_TEST_VALIDATE_STRING(str2, 24);
        AZ_TEST_ASSERT(str2[0] == 'f');
        AZ_TEST_ASSERT(str2[1] == 'a');
        AZ_TEST_ASSERT(str2[5] == 'e');

        str2.replace(6, 1, str1, 5, 1);
        AZ_TEST_VALIDATE_STRING(str2, 24);
        AZ_TEST_ASSERT(str2[6] == 'f');

        str2.replace(3, 2, sCharLong, 2);
        AZ_TEST_VALIDATE_STRING(str2, 24);
        AZ_TEST_ASSERT(str2[3] == 'T');
        AZ_TEST_ASSERT(str2[4] == 'h');

        str2.replace(3, 10, sChar);
        AZ_TEST_VALIDATE_STRING(str2, 24);
        AZ_TEST_ASSERT(str2[3] == 'S');
        AZ_TEST_ASSERT(str2[4] == 'S');

        str2.replace(3, 2, 2, 'g');
        AZ_TEST_VALIDATE_STRING(str2, 24);
        AZ_TEST_ASSERT(str2[3] == 'g');
        AZ_TEST_ASSERT(str2[4] == 'g');

        str2.replace(str2.begin(), next(str2.begin(), str1.length()), str1);
        AZ_TEST_VALIDATE_STRING(str2, 24);
        AZ_TEST_ASSERT(str2[0] == 'a');
        AZ_TEST_ASSERT(str2[1] == 'b');

        str2.replace(str2.begin(), next(str2.begin(), 10), sChar);
        AZ_TEST_VALIDATE_STRING(str2, 24);
        AZ_TEST_ASSERT(str2[0] == 'S');
        AZ_TEST_ASSERT(str2[1] == 'S');

        str2.replace(str2.begin(), next(str2.begin(), 3), sChar, 3);
        AZ_TEST_VALIDATE_STRING(str2, 24);
        AZ_TEST_ASSERT(str2[0] == 'S');
        AZ_TEST_ASSERT(str2[1] == 'S');
        AZ_TEST_ASSERT(str2[2] == 'O');

        str2.replace(str2.begin(), next(str2.begin(), 2), 2, 'h');
        AZ_TEST_VALIDATE_STRING(str2, 24);
        AZ_TEST_ASSERT(str2[0] == 'h');
        AZ_TEST_ASSERT(str2[1] == 'h');

        str2.replace(str2.begin(), next(str2.begin(), 2), aChar.begin(), next(aChar.begin(), 2));
        AZ_TEST_VALIDATE_STRING(str2, 24);
        AZ_TEST_ASSERT(str2[0] == 'a');
        AZ_TEST_ASSERT(str2[1] == 'b');

        str2.replace(str2.begin(), next(str2.begin(), 2), sChar, sChar + 5);
        AZ_TEST_VALIDATE_STRING(str2, 27);
        AZ_TEST_ASSERT(str2[0] == 'S');
        AZ_TEST_ASSERT(str2[1] == 'S');

        AZ_TEST_ASSERT(str2.at(0) == 'S');
        str2.at(0) = 'E';
        AZ_TEST_ASSERT(str2.at(0) == 'E');
        str2[0] = 'G';
        AZ_TEST_ASSERT(str2.at(0) == 'G');

        AZ_TEST_ASSERT(str2.front() == 'G');
        str2.front() = 'X';
        AZ_TEST_ASSERT(str2.front() == 'X');
        AZ_TEST_ASSERT(str2.back() == 'c'); // From the insert of 2 'c's at the end() further up.
        str2.back() = 'p';
        AZ_TEST_ASSERT(str2.back() == 'p');

        AZ_TEST_ASSERT(str2.c_str() != 0);
        AZ_TEST_ASSERT(::strlen(str2.c_str()) == str2.length());

        str2.resize(30, 'm');
        AZ_TEST_VALIDATE_STRING(str2, 30);
        AZ_TEST_ASSERT(str2[29] == 'm');

        str2.reserve(100);
        AZ_TEST_VALIDATE_STRING(str2, 30);
        AZ_TEST_ASSERT(str2.capacity() >= 100);

        char myCharArray[128];  // make sure it's big enough or use make the copy safe
        AZ_TEST_ASSERT(str2.copy(myCharArray, AZ_ARRAY_SIZE(myCharArray), 1) == str2.length() - 1);

        str1.clear();
        str2.swap(str1);
        AZ_TEST_VALIDATE_STRING(str1, 30);
        AZ_TEST_VALIDATE_EMPTY_STRING(str2);
        str1.assign(sChar);
        str2.assign(sChar + 2, sChar + 5);

        str1.append(sChar);

        AZStd::size_t pos;

        pos = str1.find(str2);
        AZ_TEST_ASSERT(pos == 2);

        pos = str1.find(str2, 1);
        AZ_TEST_ASSERT(pos == 2);

        pos = str1.find(str2, 3);  // skip the first
        AZ_TEST_ASSERT(pos == 12);

        pos = str1.find(sChar, 1, 10);
        AZ_TEST_ASSERT(pos == 10);

        const char sStr[] = "string";
        pos = str1.find(sStr);
        AZ_TEST_ASSERT(pos == 4);

        pos = str1.find('O');
        AZ_TEST_ASSERT(pos == 2);

        pos = str1.find('Z');
        AZ_TEST_ASSERT(pos == string::npos);

        pos = str1.rfind(str2);
        AZ_TEST_ASSERT(pos == 12);

        pos = str1.rfind(str2);
        AZ_TEST_ASSERT(pos == 12);

        pos = str1.rfind(sChar, 0, 10);
        AZ_TEST_ASSERT(pos == 0);

        pos = str1.rfind(sChar);
        AZ_TEST_ASSERT(pos == 10);

        pos = str1.rfind(sChar, 11);
        AZ_TEST_ASSERT(pos == 10);

        pos = str1.rfind('O');
        AZ_TEST_ASSERT(pos == 12);

        pos = str1.rfind('Z');
        AZ_TEST_ASSERT(pos == string::npos);

        pos = str1.find_first_of(str2);
        AZ_TEST_ASSERT(pos == 2);

        pos = str1.find_first_of(str2, 10);
        AZ_TEST_ASSERT(pos == 12);

        pos = str1.find_first_of(sChar, 0, 15);
        AZ_TEST_ASSERT(pos == 0);

        pos = str1.find_first_of(sChar, 3, 10);
        AZ_TEST_ASSERT(pos == 3);

        pos = str1.find_first_of(sChar, 7);
        AZ_TEST_ASSERT(pos == 7);
        pos = str1.find_first_of(sChar);
        AZ_TEST_ASSERT(pos == 0);

        pos = str1.find_first_of('O');
        AZ_TEST_ASSERT(pos == 2);
        pos = str1.find_first_of('O', 10);
        AZ_TEST_ASSERT(pos == 12);

        pos = str1.find_first_of('Z');
        AZ_TEST_ASSERT(pos == string::npos);

        pos = str1.find_last_of(str2);
        AZ_TEST_ASSERT(pos == 14);

        pos = str1.find_last_of(str2, 3);
        AZ_TEST_ASSERT(pos == 3);

        pos = str1.find_last_of(sChar, 4);
        AZ_TEST_ASSERT(pos == 4);

        pos = str1.find_last_of('O');
        AZ_TEST_ASSERT(pos == 12);

        pos = str1.find_last_of('Z');
        AZ_TEST_ASSERT(pos == string::npos);

        pos = str1.find_first_not_of(str2, 3);
        AZ_TEST_ASSERT(pos == 5);

        pos = str1.find_first_not_of('Z');
        AZ_TEST_ASSERT(pos == 0);

        pos = str1.find_last_not_of(sChar);
        AZ_TEST_ASSERT(pos == string::npos);

        pos = str1.find_last_not_of('Z');
        AZ_TEST_ASSERT(pos == 19);


        string sub = str1.substr(0, 10);
        AZ_TEST_VALIDATE_STRING(sub, 10);
        AZ_TEST_ASSERT(sub[0] == 'S');
        AZ_TEST_ASSERT(sub[9] == 'g');

        int cmpRes;
        cmpRes = str1.compare(str1);
        AZ_TEST_ASSERT(cmpRes == 0);

        cmpRes = str1.compare(str2);
        AZ_TEST_ASSERT(cmpRes > 0);

        cmpRes = str1.compare(2, 3, str2);
        AZ_TEST_ASSERT(cmpRes == 0);

        cmpRes = str1.compare(12, 2, str2, 0, 2);
        AZ_TEST_ASSERT(cmpRes == 0);
        cmpRes = str1.compare(3, 1, str2, 0, 1);
        AZ_TEST_ASSERT(cmpRes < 0);

        cmpRes = str1.compare(sChar);
        AZ_TEST_ASSERT(cmpRes > 0);
        cmpRes = str1.compare(10, 10, sChar);
        AZ_TEST_ASSERT(cmpRes == 0);
        cmpRes = str1.compare(11, 3, sChar, 3);
        AZ_TEST_ASSERT(cmpRes < 0);

        using iteratorType = char;
        auto testValue = str4;
        reverse_iterator<iteratorType*> rend = testValue.rend();
        reverse_iterator<const iteratorType*> crend1 = testValue.rend();
        reverse_iterator<const iteratorType*> crend2 = testValue.crend();

        reverse_iterator<iteratorType*> rbegin = testValue.rbegin();
        reverse_iterator<const iteratorType*> crbegin1 = testValue.rbegin();
        reverse_iterator<const iteratorType*> crbegin2 = testValue.crbegin();

        AZ_TEST_ASSERT(rend == crend1);
        AZ_TEST_ASSERT(crend1 == crend2);

        AZ_TEST_ASSERT(rbegin == crbegin1);
        AZ_TEST_ASSERT(crbegin1 == crbegin2);
        
        AZ_TEST_ASSERT(rbegin != rend);

        str1.set_capacity(3);
        AZ_TEST_VALIDATE_STRING(str1, 3);
        AZ_TEST_ASSERT(str1[0] == 'S');
        AZ_TEST_ASSERT(str1[1] == 'S');
        AZ_TEST_ASSERT(str1[2] == 'O');

        str1.clear();
        for (int i = 0; i < 10000; ++i)
        {
            str1 += 'i';
        }
        AZ_TEST_ASSERT(str1.size() == 10000);
        for (int i = 0; i < 10000; ++i)
        {
            AZ_TEST_ASSERT(str1[i] == 'i');
        }
    }

#if defined(AZ_COMPILER_MSVC)
#   pragma warning(push)
#   pragma warning( disable: 4428 )   // universal-character-name encountered in source
#endif // AZ_COMPILER_MSVC
    TEST_F(String, Algorithms)
    {
        string str = string::format("%s %d", "BlaBla", 5);
        AZ_TEST_VALIDATE_STRING(str, 8);

        wstring wstr = wstring::format(L"%ls %d", L"BlaBla", 5);
        AZ_TEST_VALIDATE_WSTRING(wstr, 8);

        to_lower(str.begin(), str.end());
        AZ_TEST_ASSERT(str[0] == 'b');
        AZ_TEST_ASSERT(str[3] == 'b');

        to_upper(str.begin(), str.end());
        AZ_TEST_ASSERT(str[1] == 'L');
        AZ_TEST_ASSERT(str[2] == 'A');

        string intStr("10");
        int ival = AZStd::stoi(intStr);
        AZ_TEST_ASSERT(ival == 10);

        wstring wintStr(L"10");
        ival = AZStd::stoi(wintStr);
        AZ_TEST_ASSERT(ival == 10);

        string floatStr("2.32");
        float fval = AZStd::stof(floatStr);
        AZ_TEST_ASSERT_FLOAT_CLOSE(fval, 2.32f);

        wstring wfloatStr(L"2.32");
        fval = AZStd::stof(wfloatStr);
        AZ_TEST_ASSERT_FLOAT_CLOSE(fval, 2.32f);

        to_string(intStr, 20);
        AZ_TEST_ASSERT(intStr == "20");

        AZ_TEST_ASSERT(to_string(static_cast<int16_t>(20)) == "20");
        AZ_TEST_ASSERT(to_string(static_cast<uint16_t>(20)) == "20");
        AZ_TEST_ASSERT(to_string(static_cast<int32_t>(20)) == "20");
        AZ_TEST_ASSERT(to_string(static_cast<uint32_t>(20)) == "20");
        AZ_TEST_ASSERT(to_string(static_cast<int64_t>(20)) == "20");
        AZ_TEST_ASSERT(to_string(static_cast<uint64_t>(20)) == "20");

        // wstring to string
        string str1;
        to_string(str1, wstr);
        AZ_TEST_ASSERT(str1 == "BlaBla 5");

        str1 = string::format("%ls", wstr.c_str());
        AZ_TEST_ASSERT(str1 == "BlaBla 5");

        // string to wstring
        wstring wstr1;
        to_wstring(wstr1, str);
        AZ_TEST_ASSERT(wstr1 == L"BLABLA 5");

        wstr1 = wstring::format(L"%hs", str.c_str());
        AZ_TEST_ASSERT(wstr1 == L"BLABLA 5");

        // wchar UTF16/UTF32 to/from Utf8
        wstr1 = L"this is a \u20AC \u00A3 test"; // that's a euro and a pound sterling
        AZStd::to_string(str, wstr1);
        wstring wstr2;
        AZStd::to_wstring(wstr2, str);
        AZ_TEST_ASSERT(wstr1 == wstr2);

        // tokenize
        vector<string> tokens;
        tokenize(string("one, two, three"), string(", "), tokens);
        AZ_TEST_ASSERT(tokens.size() == 3);
        AZ_TEST_ASSERT(tokens[0] == "one");
        AZ_TEST_ASSERT(tokens[1] == "two");
        AZ_TEST_ASSERT(tokens[2] == "three");
        tokenize(string("one, ,, two, ,, three"), string(", "), tokens);
        AZ_TEST_ASSERT(tokens.size() == 3);
        AZ_TEST_ASSERT(tokens[0] == "one");
        AZ_TEST_ASSERT(tokens[1] == "two");
        AZ_TEST_ASSERT(tokens[2] == "three");
        tokenize(string("thequickbrownfox"), string("ABC"), tokens);
        AZ_TEST_ASSERT(tokens.size() == 1);
        AZ_TEST_ASSERT(tokens[0] == "thequickbrownfox");

        tokenize(string(""), string(""), tokens);
        AZ_TEST_ASSERT(tokens.empty());
        tokenize(string("ABC"), string("ABC"), tokens);
        AZ_TEST_ASSERT(tokens.empty());
        tokenize(string(" foo bar "), string(" "), tokens);
        AZ_TEST_ASSERT(tokens.size() == 2);
        AZ_TEST_ASSERT(tokens[0] == "foo");
        AZ_TEST_ASSERT(tokens[1] == "bar");

        tokenize_keep_empty(string(" foo ,  bar "), string(","), tokens);
        AZ_TEST_ASSERT(tokens.size() == 2);
        AZ_TEST_ASSERT(tokens[0] == " foo ");
        AZ_TEST_ASSERT(tokens[1] == "  bar ");

        // Sort
        AZStd::vector<string> toSort;
        toSort.push_back("z2");
        toSort.push_back("z100");
        toSort.push_back("z1");
        AZStd::sort(toSort.begin(), toSort.end());
        AZ_TEST_ASSERT(toSort[0] == "z1");
        AZ_TEST_ASSERT(toSort[1] == "z100");
        AZ_TEST_ASSERT(toSort[2] == "z2");

        // Natural sort
        AZ_TEST_ASSERT(alphanum_comp("", "") == 0);
        AZ_TEST_ASSERT(alphanum_comp("", "a") < 0);
        AZ_TEST_ASSERT(alphanum_comp("a", "") > 0);
        AZ_TEST_ASSERT(alphanum_comp("a", "a") == 0);
        AZ_TEST_ASSERT(alphanum_comp("", "9") < 0);
        AZ_TEST_ASSERT(alphanum_comp("9", "") > 0);
        AZ_TEST_ASSERT(alphanum_comp("1", "1") == 0);
        AZ_TEST_ASSERT(alphanum_comp("1", "2") < 0);
        AZ_TEST_ASSERT(alphanum_comp("3", "2") > 0);
        AZ_TEST_ASSERT(alphanum_comp("a1", "a1") == 0);
        AZ_TEST_ASSERT(alphanum_comp("a1", "a2") < 0);
        AZ_TEST_ASSERT(alphanum_comp("a2", "a1") > 0);
        AZ_TEST_ASSERT(alphanum_comp("a1a2", "a1a3") < 0);
        AZ_TEST_ASSERT(alphanum_comp("a1a2", "a1a0") > 0);
        AZ_TEST_ASSERT(alphanum_comp("134", "122") > 0);
        AZ_TEST_ASSERT(alphanum_comp("12a3", "12a3") == 0);
        AZ_TEST_ASSERT(alphanum_comp("12a1", "12a0") > 0);
        AZ_TEST_ASSERT(alphanum_comp("12a1", "12a2") < 0);
        AZ_TEST_ASSERT(alphanum_comp("a", "aa") < 0);
        AZ_TEST_ASSERT(alphanum_comp("aaa", "aa") > 0);
        AZ_TEST_ASSERT(alphanum_comp("Alpha 2", "Alpha 2") == 0);
        AZ_TEST_ASSERT(alphanum_comp("Alpha 2", "Alpha 2A") < 0);
        AZ_TEST_ASSERT(alphanum_comp("Alpha 2 B", "Alpha 2") > 0);
        string strA("Alpha 2");
        AZ_TEST_ASSERT(alphanum_comp(strA, "Alpha 2") == 0);
        AZ_TEST_ASSERT(alphanum_comp(strA, "Alpha 2A") < 0);
        AZ_TEST_ASSERT(alphanum_comp("Alpha 2 B", strA) > 0);
        AZ_TEST_ASSERT(alphanum_comp(strA, strdup("Alpha 2")) == 0);
        AZ_TEST_ASSERT(alphanum_comp(strA, strdup("Alpha 2A")) < 0);
        AZ_TEST_ASSERT(alphanum_comp(strdup("Alpha 2 B"), strA) > 0);

        // show usage of the comparison functor with a set
        typedef set<string, alphanum_less<string> > StringSetType;
        StringSetType s;
        s.insert("Xiph Xlater 58");
        s.insert("Xiph Xlater 5000");
        s.insert("Xiph Xlater 500");
        s.insert("Xiph Xlater 50");
        s.insert("Xiph Xlater 5");
        s.insert("Xiph Xlater 40");
        s.insert("Xiph Xlater 300");
        s.insert("Xiph Xlater 2000");
        s.insert("Xiph Xlater 10000");
        s.insert("QRS-62F Intrinsia Machine");
        s.insert("QRS-62 Intrinsia Machine");
        s.insert("QRS-60F Intrinsia Machine");
        s.insert("QRS-60 Intrinsia Machine");
        s.insert("Callisto Morphamax 7000 SE2");
        s.insert("Callisto Morphamax 7000 SE");
        s.insert("Callisto Morphamax 7000");
        s.insert("Callisto Morphamax 700");
        s.insert("Callisto Morphamax 600");
        s.insert("Callisto Morphamax 5000");
        s.insert("Callisto Morphamax 500");
        s.insert("Callisto Morphamax");
        s.insert("Alpha 2A-900");
        s.insert("Alpha 2A-8000");
        s.insert("Alpha 2A");
        s.insert("Alpha 200");
        s.insert("Alpha 2");
        s.insert("Alpha 100");
        s.insert("Allegia 60 Clasteron");
        s.insert("Allegia 52 Clasteron");
        s.insert("Allegia 51B Clasteron");
        s.insert("Allegia 51 Clasteron");
        s.insert("Allegia 500 Clasteron");
        s.insert("Allegia 50 Clasteron");
        s.insert("40X Radonius");
        s.insert("30X Radonius");
        s.insert("20X Radonius Prime");
        s.insert("20X Radonius");
        s.insert("200X Radonius");
        s.insert("10X Radonius");
        s.insert("1000X Radonius Maximus");
        // check sorting
        StringSetType::const_iterator setIt = s.begin();
        AZ_TEST_ASSERT(*setIt++ == "10X Radonius");
        AZ_TEST_ASSERT(*setIt++ == "20X Radonius");
        AZ_TEST_ASSERT(*setIt++ == "20X Radonius Prime");
        AZ_TEST_ASSERT(*setIt++ == "30X Radonius");
        AZ_TEST_ASSERT(*setIt++ == "40X Radonius");
        AZ_TEST_ASSERT(*setIt++ == "200X Radonius");
        AZ_TEST_ASSERT(*setIt++ == "1000X Radonius Maximus");
        AZ_TEST_ASSERT(*setIt++ == "Allegia 50 Clasteron");
        AZ_TEST_ASSERT(*setIt++ == "Allegia 51 Clasteron");
        AZ_TEST_ASSERT(*setIt++ == "Allegia 51B Clasteron");
        AZ_TEST_ASSERT(*setIt++ == "Allegia 52 Clasteron");
        AZ_TEST_ASSERT(*setIt++ == "Allegia 60 Clasteron");
        AZ_TEST_ASSERT(*setIt++ == "Allegia 500 Clasteron");
        AZ_TEST_ASSERT(*setIt++ == "Alpha 2");
        AZ_TEST_ASSERT(*setIt++ == "Alpha 2A");
        AZ_TEST_ASSERT(*setIt++ == "Alpha 2A-900");
        AZ_TEST_ASSERT(*setIt++ == "Alpha 2A-8000");
        AZ_TEST_ASSERT(*setIt++ == "Alpha 100");
        AZ_TEST_ASSERT(*setIt++ == "Alpha 200");
        AZ_TEST_ASSERT(*setIt++ == "Callisto Morphamax");
        AZ_TEST_ASSERT(*setIt++ == "Callisto Morphamax 500");
        AZ_TEST_ASSERT(*setIt++ == "Callisto Morphamax 600");
        AZ_TEST_ASSERT(*setIt++ == "Callisto Morphamax 700");
        AZ_TEST_ASSERT(*setIt++ == "Callisto Morphamax 5000");
        AZ_TEST_ASSERT(*setIt++ == "Callisto Morphamax 7000");
        AZ_TEST_ASSERT(*setIt++ == "Callisto Morphamax 7000 SE");
        AZ_TEST_ASSERT(*setIt++ == "Callisto Morphamax 7000 SE2");
        AZ_TEST_ASSERT(*setIt++ == "QRS-60 Intrinsia Machine");
        AZ_TEST_ASSERT(*setIt++ == "QRS-60F Intrinsia Machine");
        AZ_TEST_ASSERT(*setIt++ == "QRS-62 Intrinsia Machine");
        AZ_TEST_ASSERT(*setIt++ == "QRS-62F Intrinsia Machine");
        AZ_TEST_ASSERT(*setIt++ == "Xiph Xlater 5");
        AZ_TEST_ASSERT(*setIt++ == "Xiph Xlater 40");
        AZ_TEST_ASSERT(*setIt++ == "Xiph Xlater 50");
        AZ_TEST_ASSERT(*setIt++ == "Xiph Xlater 58");
        AZ_TEST_ASSERT(*setIt++ == "Xiph Xlater 300");
        AZ_TEST_ASSERT(*setIt++ == "Xiph Xlater 500");
        AZ_TEST_ASSERT(*setIt++ == "Xiph Xlater 2000");
        AZ_TEST_ASSERT(*setIt++ == "Xiph Xlater 5000");
        AZ_TEST_ASSERT(*setIt++ == "Xiph Xlater 10000");

        // show usage of comparison functor with a map
        typedef map<string, int, alphanum_less<string> > StringIntMapType;
        StringIntMapType m;
        m["z1.doc"] = 1;
        m["z10.doc"] = 2;
        m["z100.doc"] = 3;
        m["z101.doc"] = 4;
        m["z102.doc"] = 5;
        m["z11.doc"] = 6;
        m["z12.doc"] = 7;
        m["z13.doc"] = 8;
        m["z14.doc"] = 9;
        m["z15.doc"] = 10;
        m["z16.doc"] = 11;
        m["z17.doc"] = 12;
        m["z18.doc"] = 13;
        m["z19.doc"] = 14;
        m["z2.doc"] = 15;
        m["z20.doc"] = 16;
        m["z3.doc"] = 17;
        m["z4.doc"] = 18;
        m["z5.doc"] = 19;
        m["z6.doc"] = 20;
        m["z7.doc"] = 21;
        m["z8.doc"] = 22;
        m["z9.doc"] = 23;
        // check sorting
        StringIntMapType::const_iterator mapIt = m.begin();
        AZ_TEST_ASSERT((mapIt++)->second == 1);
        AZ_TEST_ASSERT((mapIt++)->second == 15);
        AZ_TEST_ASSERT((mapIt++)->second == 17);
        AZ_TEST_ASSERT((mapIt++)->second == 18);
        AZ_TEST_ASSERT((mapIt++)->second == 19);
        AZ_TEST_ASSERT((mapIt++)->second == 20);
        AZ_TEST_ASSERT((mapIt++)->second == 21);
        AZ_TEST_ASSERT((mapIt++)->second == 22);
        AZ_TEST_ASSERT((mapIt++)->second == 23);
        AZ_TEST_ASSERT((mapIt++)->second == 2);
        AZ_TEST_ASSERT((mapIt++)->second == 6);
        AZ_TEST_ASSERT((mapIt++)->second == 7);
        AZ_TEST_ASSERT((mapIt++)->second == 8);
        AZ_TEST_ASSERT((mapIt++)->second == 9);
        AZ_TEST_ASSERT((mapIt++)->second == 10);
        AZ_TEST_ASSERT((mapIt++)->second == 11);
        AZ_TEST_ASSERT((mapIt++)->second == 12);
        AZ_TEST_ASSERT((mapIt++)->second == 13);
        AZ_TEST_ASSERT((mapIt++)->second == 14);
        AZ_TEST_ASSERT((mapIt++)->second == 16);
        AZ_TEST_ASSERT((mapIt++)->second == 3);
        AZ_TEST_ASSERT((mapIt++)->second == 4);
        AZ_TEST_ASSERT((mapIt++)->second == 5);

        // show usage of comparison functor with an STL algorithm on a vector
        vector<string> v;
        // vector contents are reversed sorted contents of the old set
        AZStd::copy(s.rbegin(), s.rend(), back_inserter(v));
        // now sort the vector with the algorithm
        AZStd::sort(v.begin(), v.end(), alphanum_less<string>());
        // check values
        vector<string>::const_iterator vecIt = v.begin();
        AZ_TEST_ASSERT(*vecIt++ == "10X Radonius");
        AZ_TEST_ASSERT(*vecIt++ == "20X Radonius");
        AZ_TEST_ASSERT(*vecIt++ == "20X Radonius Prime");
        AZ_TEST_ASSERT(*vecIt++ == "30X Radonius");
        AZ_TEST_ASSERT(*vecIt++ == "40X Radonius");
        AZ_TEST_ASSERT(*vecIt++ == "200X Radonius");
        AZ_TEST_ASSERT(*vecIt++ == "1000X Radonius Maximus");
        AZ_TEST_ASSERT(*vecIt++ == "Allegia 50 Clasteron");
        AZ_TEST_ASSERT(*vecIt++ == "Allegia 51 Clasteron");
        AZ_TEST_ASSERT(*vecIt++ == "Allegia 51B Clasteron");
        AZ_TEST_ASSERT(*vecIt++ == "Allegia 52 Clasteron");
        AZ_TEST_ASSERT(*vecIt++ == "Allegia 60 Clasteron");
        AZ_TEST_ASSERT(*vecIt++ == "Allegia 500 Clasteron");
        AZ_TEST_ASSERT(*vecIt++ == "Alpha 2");
        AZ_TEST_ASSERT(*vecIt++ == "Alpha 2A");
        AZ_TEST_ASSERT(*vecIt++ == "Alpha 2A-900");
        AZ_TEST_ASSERT(*vecIt++ == "Alpha 2A-8000");
        AZ_TEST_ASSERT(*vecIt++ == "Alpha 100");
        AZ_TEST_ASSERT(*vecIt++ == "Alpha 200");
        AZ_TEST_ASSERT(*vecIt++ == "Callisto Morphamax");
        AZ_TEST_ASSERT(*vecIt++ == "Callisto Morphamax 500");
        AZ_TEST_ASSERT(*vecIt++ == "Callisto Morphamax 600");
        AZ_TEST_ASSERT(*vecIt++ == "Callisto Morphamax 700");
        AZ_TEST_ASSERT(*vecIt++ == "Callisto Morphamax 5000");
        AZ_TEST_ASSERT(*vecIt++ == "Callisto Morphamax 7000");
        AZ_TEST_ASSERT(*vecIt++ == "Callisto Morphamax 7000 SE");
        AZ_TEST_ASSERT(*vecIt++ == "Callisto Morphamax 7000 SE2");
        AZ_TEST_ASSERT(*vecIt++ == "QRS-60 Intrinsia Machine");
        AZ_TEST_ASSERT(*vecIt++ == "QRS-60F Intrinsia Machine");
        AZ_TEST_ASSERT(*vecIt++ == "QRS-62 Intrinsia Machine");
        AZ_TEST_ASSERT(*vecIt++ == "QRS-62F Intrinsia Machine");
        AZ_TEST_ASSERT(*vecIt++ == "Xiph Xlater 5");
        AZ_TEST_ASSERT(*vecIt++ == "Xiph Xlater 40");
        AZ_TEST_ASSERT(*vecIt++ == "Xiph Xlater 50");
        AZ_TEST_ASSERT(*vecIt++ == "Xiph Xlater 58");
        AZ_TEST_ASSERT(*vecIt++ == "Xiph Xlater 300");
        AZ_TEST_ASSERT(*vecIt++ == "Xiph Xlater 500");
        AZ_TEST_ASSERT(*vecIt++ == "Xiph Xlater 2000");
        AZ_TEST_ASSERT(*vecIt++ == "Xiph Xlater 5000");
        AZ_TEST_ASSERT(*vecIt++ == "Xiph Xlater 10000");
    }

    class Regex
        : public AllocatorsFixture
    {
    };

    TEST_F(Regex, Regex_IPAddressSubnetPattern_Success)
    {
        // Error case for LY-43888
        regex txt_regex("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])(/([0-9]|[1-2][0-9]|3[0-2]))?$");
        string sample_input("10.85.22.92/24");
        bool match = regex_match(sample_input, txt_regex);
        AZ_TEST_ASSERT(match);
    }

    TEST_F(Regex, MatchConstChar)
    {
        //regex
        AZ_TEST_ASSERT(regex_match("subject", regex("(sub)(.*)")));
    }

    TEST_F(Regex, MatchString)
    {
        string reStr("subject");
        regex re("(sub)(.*)");
        AZ_TEST_ASSERT(regex_match(reStr, re));
        AZ_TEST_ASSERT(regex_match(reStr.begin(), reStr.end(), re))
    }

    TEST_F(Regex, CMatch)
    {
        regex re("(sub)(.*)");
        cmatch cm;    // same as match_results<const char*> cm;
        regex_match("subject", cm, re);
        AZ_TEST_ASSERT(cm.size() == 3);
    }

    TEST_F(Regex, SMatch)
    {
        string reStr("subject");
        regex re("(sub)(.*)");
        smatch sm;    // same as std::match_results<string::const_iterator> sm;
        regex_match(reStr, sm, re);
        AZ_TEST_ASSERT(sm.size() == 3);

        regex_match(reStr.cbegin(), reStr.cend(), sm, re);
        AZ_TEST_ASSERT(sm.size() == 3);
    }

    TEST_F(Regex, CMatchWithFlags)
    {
        regex re("(sub)(.*)");
        cmatch cm;    // same as match_results<const char*> cm;
        // using explicit flags:
        regex_match("subject", cm, re, regex_constants::match_default);
        AZ_TEST_ASSERT(cm[0] == "subject");
        AZ_TEST_ASSERT(cm[1] == "sub");
        AZ_TEST_ASSERT(cm[2] == "ject");
    }

    TEST_F(Regex, PatternMatchFiles)
    {
        // Simple regular expression matching
        string fnames[] = { "foo.txt", "bar.txt", "baz.dat", "zoidberg" };
        regex txt_regex("[a-z]+\\.txt");

        for (size_t i = 0; i < AZ_ARRAY_SIZE(fnames); ++i)
        {
            if (i < 2)
            {
                AZ_TEST_ASSERT(regex_match(fnames[i], txt_regex) == true);
            }
            else
            {
                AZ_TEST_ASSERT(regex_match(fnames[i], txt_regex) == false);
            }
        }
    }

    TEST_F(Regex, PatternWithSingleCaptureGroup)
    {
        // Extraction of a sub-match
        string fnames[] = { "foo.txt", "bar.txt", "baz.dat", "zoidberg" };
        regex base_regex("([a-z]+)\\.txt");
        smatch base_match;

        for (size_t i = 0; i < AZ_ARRAY_SIZE(fnames); ++i)
        {
            if (regex_match(fnames[i], base_match, base_regex))
            {
                AZ_TEST_ASSERT(base_match.size() == 2);
                AZ_TEST_ASSERT(base_match[1] == "foo" || base_match[1] == "bar")
            }
        }
    }

    TEST_F(Regex, PatternWithMultipleCaptureGroups)
    {
        // Extraction of several sub-matches
        string fnames[] = { "foo.txt", "bar.txt", "baz.dat", "zoidberg" };
        regex pieces_regex("([a-z]+)\\.([a-z]+)");
        smatch pieces_match;
        for (size_t i = 0; i < AZ_ARRAY_SIZE(fnames); ++i)
        {
            if (regex_match(fnames[i], pieces_match, pieces_regex))
            {
                AZ_TEST_ASSERT(pieces_match.size() == 3);
                AZ_TEST_ASSERT(pieces_match[0] == "foo.txt" || pieces_match[0] == "bar.txt" || pieces_match[0] == "baz.dat");
                AZ_TEST_ASSERT(pieces_match[1] == "foo" || pieces_match[1] == "bar" || pieces_match[1] == "baz");
                AZ_TEST_ASSERT(pieces_match[2] == "txt" || pieces_match[2] == "dat");
            }
        }
    }

    TEST_F(Regex, WideCharTests)
    {
        //wchar_t
        AZ_TEST_ASSERT(regex_match(L"subject", wregex(L"(sub)(.*)")));
        wstring reWStr(L"subject");
        wregex reW(L"(sub)(.*)");
        AZ_TEST_ASSERT(regex_match(reWStr, reW));
        AZ_TEST_ASSERT(regex_match(reWStr.begin(), reWStr.end(), reW))
    }

    TEST_F(Regex, LongPatterns)
    {
        // test construction and destruction of a regex with a pattern long enough to require reallocation of buffers
        regex longerThan16(".*\\/Presets\\/GeomCache\\/.*", regex::flag_type::icase | regex::flag_type::ECMAScript);
        regex longerThan32(".*\\/Presets\\/GeomCache\\/Whatever\\/Much\\/Test\\/Very\\/Memory\\/.*", regex::flag_type::icase);
    }
    
    TEST_F(Regex, SmileyFaceParseRegression)
    {
        regex smiley(":)");

        EXPECT_TRUE(smiley.Empty());
        EXPECT_TRUE(smiley.GetError() != nullptr);
        EXPECT_FALSE(regex_match("wut", smiley));
        EXPECT_FALSE(regex_match(":)", smiley));
    }

    TEST_F(Regex, ParseFailure)
    {
        regex failed(")))/?!\\$");

        EXPECT_FALSE(failed.Valid());

        regex other = AZStd::move(failed);
        EXPECT_FALSE(other.Valid());

        regex other2;
        other2.swap(other);
        EXPECT_TRUE(other.Empty());
        EXPECT_TRUE(other.GetError() == nullptr);
        EXPECT_FALSE(other.Valid());
        EXPECT_FALSE(other2.Valid());
    }

    // StringAlgorithmTest-End
#if defined(AZ_COMPILER_MSVC)
#   pragma warning(pop)
#endif // AZ_COMPILER_MSVC

    TEST_F(String, ConstString)
    {
        string_view cstr1;
        AZ_TEST_ASSERT(cstr1.data()==nullptr);
        AZ_TEST_ASSERT(cstr1.size() == 0);
        AZ_TEST_ASSERT(cstr1.length() == 0);
        AZ_TEST_ASSERT(cstr1.begin() == cstr1.end());
        AZ_TEST_ASSERT(cstr1 == string_view());
        AZ_TEST_ASSERT(cstr1.empty());

        string_view cstr2("Test");
        AZ_TEST_ASSERT(cstr2.data() != nullptr);
        AZ_TEST_ASSERT(cstr2.size() == 4);
        AZ_TEST_ASSERT(cstr2.length() == 4);
        AZ_TEST_ASSERT(cstr2.begin() != cstr2.end());
        AZ_TEST_ASSERT(cstr2 != cstr1);
        AZ_TEST_ASSERT(cstr2 == string_view("Test"));
        AZ_TEST_ASSERT(cstr2 == "Test");
        AZ_TEST_ASSERT(cstr2 != "test");
        AZ_TEST_ASSERT(cstr2[2] == 's');
        AZ_TEST_ASSERT(cstr2.at(2) == 's');
        AZ_TEST_START_ASSERTTEST;
        AZ_TEST_ASSERT(cstr2.at(7) == 0);
        AZ_TEST_STOP_ASSERTTEST(1);
        AZ_TEST_ASSERT(!cstr2.empty());
        AZ_TEST_ASSERT(cstr2.to_string() == string("Test"));
        AZ_TEST_ASSERT((string)cstr2 == string("Test"));

        string_view cstr3 = cstr2;
        AZ_TEST_ASSERT(cstr3 == cstr2);

        cstr3.swap(cstr1);
        AZ_TEST_ASSERT(cstr3 == string_view());
        AZ_TEST_ASSERT(cstr1 == cstr2);

        cstr1.erase();
        AZ_TEST_ASSERT(cstr1 == string_view());
        AZ_TEST_ASSERT(cstr1.size() == 0);
        AZ_TEST_ASSERT(cstr1.length() == 0);

        AZStd::string str1("Test");
        AZ_TEST_ASSERT(cstr2 == str1);
        cstr1 = str1;
        AZ_TEST_ASSERT(cstr1 == cstr2);

        // check hashing
        AZStd::hash<string_view> h;
        AZStd::size_t value = h(cstr1);
        AZ_TEST_ASSERT(value != 0);
    }

    TEST_F(String, StringViewModifierTest)
    {
        string_view emptyView1;
        string_view view2("Needle in Haystack");

        // front
        EXPECT_EQ('N', view2.front());
        // back
        EXPECT_EQ('k', view2.back());

        AZStd::string findStr("Hay");
        string_view view3(findStr);
        string_view nullptrView4(nullptr);

        EXPECT_EQ(emptyView1, nullptrView4);

        // copy
        const size_t destBufferSize = 32;
        char dest[destBufferSize] = { 0 };
        AZStd::size_t copyResult = view2.copy(dest, destBufferSize, 1);
        EXPECT_EQ(view2.size() - 1, copyResult);

        char assertDest[destBufferSize] = { 0 };
        AZ_TEST_START_ASSERTTEST;
        view2.copy(assertDest, destBufferSize, view2.size() + 1);
        AZ_TEST_STOP_ASSERTTEST(1);

        // substr
        string_view subView2 = view2.substr(10);
        EXPECT_EQ("Haystack", subView2);
        AZ_TEST_START_ASSERTTEST;
        string_view assertSubView = view2.substr(view2.size() + 1);
        AZ_TEST_STOP_ASSERTTEST(1);

        // compare
        AZStd::size_t compareResult = view2.compare(1, view2.size() - 1, dest, copyResult);
        EXPECT_EQ(0, compareResult);
        
        string_view compareView = "Stackhay in Needle";
        compareResult = compareView.compare(view2);
        EXPECT_NE(0, compareResult);
        
        compareResult = compareView.compare(12, 6, view2, 0, 6);
        EXPECT_EQ(0, compareResult);

        compareResult = compareView.compare(9, 2, view2, 7, 2);
        EXPECT_EQ(0, compareResult);

        compareResult = compareView.compare("Stackhay in Needle");
        EXPECT_EQ(0, compareResult);

        // find
        AZStd::size_t findResult = view2.find(view3, 0);
        EXPECT_NE(0, findResult);
        EXPECT_EQ(10, findResult);

        findResult = compareView.find("Random String");
        EXPECT_EQ(string_view::npos, findResult);

        findResult = view3.find('y', 2);
        EXPECT_EQ(2, findResult);

        // rfind
        AZStd::size_t rfindResult = view3.rfind('a', 2);
        EXPECT_EQ(1, rfindResult);

        rfindResult = nullptrView4.rfind("");
        EXPECT_EQ(0, rfindResult);

        rfindResult = emptyView1.rfind("");
        EXPECT_EQ(0, rfindResult);

        rfindResult = view2.rfind("z");
        EXPECT_EQ(string_view::npos, rfindResult);

        // find_first_of
        string_view repeatString = "abcdefabcfedghiabcdef";
        AZStd::size_t findFirstOfResult = repeatString.find_first_of('f');
        EXPECT_EQ(5, findFirstOfResult);

        findFirstOfResult = repeatString.find_first_of("def");
        EXPECT_EQ(3, findFirstOfResult);

        findFirstOfResult = repeatString.find_first_of("def", 6);
        EXPECT_EQ(9, findFirstOfResult);

        AZStd::string notFoundStr = "zzz";
        AZStd::string foundStr = "ghi";
        findFirstOfResult = repeatString.find_first_of(notFoundStr);
        EXPECT_EQ(string_view::npos, findFirstOfResult);

        findFirstOfResult = repeatString.find_first_of(foundStr);
        EXPECT_EQ(12, findFirstOfResult);

        // find_last_of
        AZStd::size_t findLastOfResult = repeatString.find_last_of('f');
        EXPECT_EQ(20, findLastOfResult);

        findLastOfResult = repeatString.find_last_of("bcd");
        EXPECT_EQ(18, findLastOfResult);

        findLastOfResult = repeatString.find_last_of("bcd", 3);
        EXPECT_EQ(3, findLastOfResult);

        findLastOfResult = repeatString.find_last_of(notFoundStr);
        EXPECT_EQ(string_view::npos, findLastOfResult);

        findLastOfResult = repeatString.find_last_of(foundStr);
        EXPECT_EQ(14, findLastOfResult);

        // find_first_not_of
        AZStd::size_t findFirstNotOfResult = repeatString.find_first_not_of('a');
        EXPECT_EQ(1, findFirstNotOfResult);

        findFirstNotOfResult = repeatString.find_first_not_of("abcdef");
        EXPECT_EQ(12, findFirstNotOfResult);

        findFirstNotOfResult = repeatString.find_first_not_of("abc", 6);
        EXPECT_EQ(9, findFirstNotOfResult);

        findFirstNotOfResult = repeatString.find_first_not_of(notFoundStr);
        EXPECT_EQ(0, findFirstNotOfResult);

        findFirstNotOfResult = repeatString.find_first_not_of(foundStr, 12);
        EXPECT_EQ(15, findFirstNotOfResult);

        // find_last_not_of
        AZStd::size_t findLastNotOfResult = repeatString.find_last_not_of('a');
        EXPECT_EQ(20, findLastNotOfResult);

        findLastNotOfResult = repeatString.find_last_not_of("abcdef");
        EXPECT_EQ(14, findLastNotOfResult);

        findLastNotOfResult = repeatString.find_last_not_of("abcf", 9);
        EXPECT_EQ(4, findLastNotOfResult);

        findLastNotOfResult = repeatString.find_last_not_of(notFoundStr);
        EXPECT_EQ(20, findLastNotOfResult);

        findLastNotOfResult = repeatString.find_last_not_of(foundStr, 14);
        EXPECT_EQ(11, findLastNotOfResult);

        // remove_prefix
        string_view prefixRemovalView = view2;
        prefixRemovalView.remove_prefix(6);
        EXPECT_EQ(" in Haystack", prefixRemovalView);
        
        // remove_suffix
        string_view suffixRemovalView = view2;
        suffixRemovalView.remove_suffix(8);
        EXPECT_EQ("Needle in ", suffixRemovalView);
    }

    TEST_F(String, StringViewCmpOperatorTest)
    {
        string_view view1("The quick brown fox jumped over the lazy dog");
        string_view view2("Needle in Haystack");
        string_view nullBeaverView(nullptr);
        string_view emptyBeaverView;
        string_view superEmptyBeaverView("");
        
        EXPECT_EQ(nullBeaverView, emptyBeaverView);
        EXPECT_EQ(superEmptyBeaverView, nullBeaverView);
        EXPECT_EQ(emptyBeaverView, superEmptyBeaverView);
        EXPECT_EQ(nullBeaverView, "");
        EXPECT_EQ(nullBeaverView, nullptr);
        EXPECT_EQ("", emptyBeaverView);
        EXPECT_EQ(nullptr, superEmptyBeaverView);

        EXPECT_EQ("The quick brown fox jumped over the lazy dog", view1);
        EXPECT_NE("The slow brown fox jumped over the lazy dog", view1);
        EXPECT_EQ(view2, "Needle in Haystack");
        EXPECT_NE(view2, "Needle in Hayqueue");

        string_view compareView(view2);
        EXPECT_EQ(view2, compareView);
        EXPECT_NE(view2, view1);

        AZStd::string compareStr("Busy Beaver");
        string_view notBeaverView("Lumber Beaver");
        string_view beaverView("Busy Beaver");
        EXPECT_EQ(compareStr, beaverView);
        EXPECT_NE(compareStr, notBeaverView);

        AZStd::string microBeaverStr("Micro Beaver");

        EXPECT_LT(view2, view1);
        EXPECT_LT(notBeaverView, "Super Lumber Beaver");
        EXPECT_LT("Disgruntled Beaver", notBeaverView);
        EXPECT_LT(notBeaverView, microBeaverStr);
        EXPECT_LT(compareStr, notBeaverView);

        EXPECT_GT(view1, view2);
        EXPECT_GT(notBeaverView, "Disgruntled Beaver");
        EXPECT_GT("Super Lumber Beaver", notBeaverView);
        EXPECT_GT(microBeaverStr, notBeaverView);
        EXPECT_GT(notBeaverView, compareStr);

        AZStd::string lowerBeaverStr("busy Beaver");
        EXPECT_LE(view2, view1);
        EXPECT_LE(compareView, compareView);
        EXPECT_LE(beaverView, "Rocket Beaver");
        EXPECT_LE(beaverView, "Busy Beaver");
        EXPECT_LE("Likable Beaver", notBeaverView);
        EXPECT_LE("Busy Beaver", beaverView);
        EXPECT_LE(nullBeaverView, nullBeaverView);
        EXPECT_LE(nullBeaverView, lowerBeaverStr);
        EXPECT_LE(microBeaverStr, view1);
        EXPECT_LE(compareStr, beaverView);
        
        AZStd::string bigBeaver("Big Beaver");
        EXPECT_GE(view1, view2);
        EXPECT_GE(view1, view1);
        EXPECT_GE(beaverView, "Busy Beave");
        EXPECT_GE(beaverView, "Busy Beaver");
        EXPECT_GE("Busy Beaver", beaverView);
        EXPECT_GE("Busy Beaver1", beaverView);
        EXPECT_GE(beaverView, compareStr);
        EXPECT_GE(beaverView, bigBeaver);
        EXPECT_GE(compareStr, beaverView);
        EXPECT_GE(microBeaverStr, beaverView);
    }
}
