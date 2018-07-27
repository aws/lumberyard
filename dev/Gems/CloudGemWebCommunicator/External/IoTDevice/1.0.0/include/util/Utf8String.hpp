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
 * @file Utf8String.hpp
 * @brief
 *
 */

#pragma once

#include <memory>
#include "util/memory/stl/String.hpp"

namespace awsiotsdk {
	class Utf8String {
	protected:
		util::String data;
		std::size_t length;

		Utf8String(util::String str);

		Utf8String(const char *str, std::size_t length);

		static bool IsValidInput(util::String str);

		static bool IsValidInput(const char *str, std::size_t length);

	public:
		// Rule of 5 stuff
		// Disabling default constructor while keeping defaults for the rest
		Utf8String() = delete;									// Delete Default constructor
		Utf8String(const Utf8String&) = default;				// Copy constructor
        // LY 2013 Fixes
		Utf8String& operator=(const Utf8String&) = default;	// Copy assignment operator
		~Utf8String() = default;								// Default destructor

		static std::unique_ptr<Utf8String> Create(util::String str);

		static std::unique_ptr<Utf8String> Create(const char *str, std::size_t length);

		std::size_t Length();

		util::String ToStdString();
	};
}
