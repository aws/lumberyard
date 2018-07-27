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
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/string.h>
#include <array>

using namespace UnitTestInternal;

namespace UnitTest
{
	TEST(CreateDestroy, UninitializedFill_StdArray_IntType_AllEight)
	{
		const int intArraySize = 5;
		const int fillValue = 8;
		std::array<int, intArraySize> intArray;
		AZStd::uninitialized_fill(intArray.begin(), intArray.end(), fillValue, std::false_type());
		for (auto itr : intArray)
		{
			EXPECT_EQ(itr, fillValue);
		}
	}

	TEST(CreateDestroy, UninitializedFill_AZStdArray_IntType_AllEight)
	{
		const int intArraySize = 5;
		const int fillValue = 8;
		AZStd::array<int, intArraySize> intArray;
		AZStd::uninitialized_fill(intArray.begin(), intArray.end(), fillValue, std::false_type());
		for (auto itr : intArray)
		{
			EXPECT_EQ(itr, fillValue);
		}
	}

	TEST(CreateDestroy, UninitializedFill_StdArray_StringType_AllEight)
	{
		const int stringArraySize = 5;
		const AZStd::string fillValue = "hello, world";
		std::array<AZStd::string, stringArraySize> stringArray;
		AZStd::uninitialized_fill(stringArray.begin(), stringArray.end(), fillValue, std::false_type());
		for (auto itr : stringArray)
		{
			EXPECT_EQ(0, strcmp(itr.c_str(), fillValue.c_str()));
		}
	}

	TEST(CreateDestroy, UninitializedFill_AZStdArray_StringType_AllEight)
	{
		const int stringArraySize = 5;
		const AZStd::string fillValue = "hello, world";
		AZStd::array<AZStd::string, stringArraySize> stringArray;
		AZStd::uninitialized_fill(stringArray.begin(), stringArray.end(), fillValue, std::false_type());
		for (auto itr : stringArray)
		{
			EXPECT_EQ(0, strcmp(itr.c_str(), fillValue.c_str()));
		}
	}
}