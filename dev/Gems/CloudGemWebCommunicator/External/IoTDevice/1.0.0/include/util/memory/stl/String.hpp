/*
 * Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file String.hpp
 * @brief
 *
 */

#pragma once

#include "util/Core_EXPORTS.hpp"
#include <functional>
#include <string>

namespace awsiotsdk {
	namespace util {
		using String = std::basic_string<char, std::char_traits<char>>;

#ifdef _WIN32
		using WString = std::basic_string<wchar_t, std::char_traits<wchar_t>>;
#endif
	} // namespace util
} // namespace awsiotsdk



