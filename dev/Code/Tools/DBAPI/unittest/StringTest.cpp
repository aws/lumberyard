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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "stdafx.h"
#include "StringUtil.h"

TEST(STR_TRIM, trim_left)
{
    std::string str(" \t \n abcd");
    EXPECT_TRUE(str_trim_left(str) == "abcd");
    EXPECT_TRUE(str_trim_left(str, " ") == "\t \n abcd");
    EXPECT_TRUE(str_trim_left(str, " \t") == "\n abcd");
    EXPECT_TRUE(str_trim_left(str, " \n") == "\t \n abcd");
    EXPECT_TRUE(str == " \t \n abcd");
}

TEST(STR_TRIM, trim_left2)
{
    std::string str("   \t \n ");
    EXPECT_TRUE(str_trim_left(str).empty());
    EXPECT_FALSE(str.empty());
}

TEST(STR_TRIM, trim_left3)
{
    std::string str("ABCD  ");
    EXPECT_TRUE(str_trim_left(str) == str);
    EXPECT_TRUE(str_trim_left(str).c_str() != str.c_str());
}

TEST(STR_TRIM, trim_right)
{
    std::string str("abcd \t \n   ");
    EXPECT_TRUE(str_trim_right(str) == "abcd");
    EXPECT_TRUE(str_trim_right(str, " ") == "abcd \t \n");
    EXPECT_TRUE(str_trim_right(str, " \t") == "abcd \t \n");
    EXPECT_TRUE(str_trim_right(str, " \n") == "abcd \t");
    EXPECT_TRUE(str == "abcd \t \n   ");
}

TEST(STR_TRIM, trim_right2)
{
    std::string str("   \t \n ");
    EXPECT_TRUE(str_trim_right(str).empty());
    EXPECT_FALSE(str.empty());
}

TEST(STR_TRIM, trim_right3)
{
    std::string str("   ABCD");
    EXPECT_TRUE(str_trim_right(str) == str);
    EXPECT_TRUE(str_trim_right(str).c_str() != str.c_str());
}

TEST(STR_TRIM, trim)
{
    std::string str(" \t \r abcd \t \n   ");
    EXPECT_TRUE(str_trim(str) == "abcd");
    EXPECT_FALSE(str == "abcd");
    EXPECT_TRUE(str == " \t \r abcd \t \n   ");
}

TEST(STR_TRIM, trim2)
{
    std::string str(" \t \r  \t \n   ");
    EXPECT_TRUE(str_trim(str).empty());
    EXPECT_FALSE(str.empty());
}

TEST(STR_TRIM, trim3)
{
    std::string str("AB \t \r  \t \n  AB");
    EXPECT_TRUE(str_trim(str) == str);
    EXPECT_TRUE(str_trim(str).c_str() != str.c_str());
}

TEST(STR_INNER_TRIM, trim_left)
{
    std::string str(" \t \n abcd");
    EXPECT_TRUE(str_inner_trim_left(std::string(str)) == "abcd");
    EXPECT_TRUE(str_inner_trim_left(std::string(str), " ") == "\t \n abcd");
    EXPECT_TRUE(str_inner_trim_left(std::string(str), " \t") == "\n abcd");
    EXPECT_TRUE(str_inner_trim_left(std::string(str), " \n") == "\t \n abcd");
    EXPECT_TRUE(str == " \t \n abcd");
    EXPECT_TRUE(str_inner_trim_left(str) == "abcd");
    EXPECT_TRUE(str == "abcd");
}

TEST(STR_INNER_TRIM, trim_left2)
{
    std::string str("   \t \n ");
    EXPECT_TRUE(str_inner_trim_left(str).empty());
    EXPECT_TRUE(str.empty());
}

TEST(STR_INNER_TRIM, trim_right)
{
    std::string str("abcd \t \n   ");
    EXPECT_TRUE(str_inner_trim_right(std::string(str)) == "abcd");
    EXPECT_TRUE(str_inner_trim_right(std::string(str), " ") == "abcd \t \n");
    EXPECT_TRUE(str_inner_trim_right(std::string(str), " \t") == "abcd \t \n");
    EXPECT_TRUE(str_inner_trim_right(std::string(str), " \n") == "abcd \t");
    EXPECT_TRUE(str == "abcd \t \n   ");
    EXPECT_TRUE(str_inner_trim_right(str) == "abcd");
    EXPECT_TRUE(str == "abcd");
}

TEST(STR_INNER_TRIM, trim_right2)
{
    std::string str("   \t \n ");
    EXPECT_TRUE(str_inner_trim_right(str).empty());
    EXPECT_TRUE(str.empty());
}

TEST(STR_INNER_TRIM, str_trim)
{
    std::string str(" \t \r abcd \t \n   ");
    EXPECT_TRUE(str_inner_trim(std::string(str)) == "abcd");
    EXPECT_TRUE(str == " \t \r abcd \t \n   ");
    EXPECT_TRUE(str_inner_trim(str) == "abcd");
    EXPECT_TRUE(str == "abcd");
}

TEST(STR_INNER_TRIM, str_trim2)
{
    std::string str(" \t \r  \t \n   ");
    EXPECT_TRUE(str_inner_trim(str).empty());
    EXPECT_TRUE(str.empty());
}

TEST(FMSTRING, Create)
{
    fm_string str;
    ASSERT_TRUE(str.empty());
}

TEST(FMSTRING, Create2)
{
    fm_string str("Create");
    ASSERT_FALSE(str.empty());
    ASSERT_TRUE(str == "Create");
}

TEST(FMSTRING, Clear)
{
    fm_string str("Create");
    str.clear();
    ASSERT_TRUE(str.empty());
}

TEST(FMSTRING, trim_left)
{
    fm_string str1(" \t \n \bABCD");
    fm_string str2(str1);
    EXPECT_TRUE(str2.inner_trim_left() == "\bABCD");
    EXPECT_TRUE(str1.trim_left() == str2);
    EXPECT_TRUE(str1.inner_trim_left() == str2);
    EXPECT_TRUE(str1 == str2);
}


TEST(FMSTRING, trim_right)
{
    fm_string str1("ABCD\\c \n \t  \n");
    fm_string str2(str1);
    EXPECT_TRUE(str2.inner_trim_right() == "ABCD\\c");
    EXPECT_TRUE(str1.trim_right() == str2);
    EXPECT_TRUE(str1.inner_trim_right() == str2);
    EXPECT_TRUE(str1 == str2);
}

TEST(FMSTRING, trim)
{
    fm_string str1("  \t  \nABCD\\c \n \t  \n");
    fm_string str2(str1);
    EXPECT_TRUE(str2.inner_trim() == "ABCD\\c");
    EXPECT_TRUE(str1.trim() == str2);
    EXPECT_TRUE(str1.inner_trim() == str2);
    EXPECT_TRUE(str1 == str2);
}

TEST(FMSTRING, trim2)
{
    fm_string str1("  \t  \n \n \t  \n");
    EXPECT_TRUE(str1.trim().empty());
    EXPECT_TRUE(!str1.empty());
    EXPECT_TRUE(str1.inner_trim().empty());
    EXPECT_TRUE(str1.empty());
}
