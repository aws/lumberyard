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
 * @file LogLevel.hpp
 * @brief
 *
 */

#pragma once

#include "util/Core_EXPORTS.hpp"
#include "util/memory/stl/String.hpp"

namespace awsiotsdk {
	namespace util {
		namespace Logging {
			/**
			 * LogLevel used to control verbosity of logging system.
			 */
			enum class LogLevel : int {
				Off = 0,
				Fatal = 1,
				Error = 2,
				Warn = 3,
				Info = 4,
				Debug = 5,
				Trace = 6
			};

			AWS_API_EXPORT util::String GetLogLevelName(LogLevel logLevel);
		} // namespace Logging
	} // namespace util
} // namespace awsiotsdk
