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
#pragma once
#include <AzTest/AzTest.h>

// Type Printing

void PrintTo(const Vec3& v, std::ostream* os);
void PrintTo(const Quat& v, std::ostream* os);

void PrintTo(const Vec3& v, ::testing::AssertionResult& result);
void PrintTo(const Quat& q, ::testing::AssertionResult& result);

// Custom Assertions

::testing::AssertionResult AreEqual(const Vec3& expected, const Vec3& actual, float epsilon=1.0e-5f);
::testing::AssertionResult AreEqual(const Quat& expected, const Quat& actual, float epsilon=1.0e-5f);