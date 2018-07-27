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
 * @file JsonParser.hpp
 * @brief
 *
 */

#pragma once

#include <cstdint>
#include <rapidjson/document.h>

#include "util/memory/stl/String.hpp"
#include "ResponseCode.hpp"

#define MAX_CONFIG_FILE_SIZE_BYTES 4096 //Increase if adding more configuration options

namespace awsiotsdk {
	namespace util {
		using JsonDocument = rapidjson::Document;
		using JsonValue = rapidjson::Value;

		class JsonParser {
		public:
			static ResponseCode InitializeFromJsonFile(JsonDocument &json_document, const util::String &config_file_path);

			static ResponseCode InitializeFromJsonString(JsonDocument &json_document,
														 const util::String &config_json_string);

			static ResponseCode GetBoolValue(const JsonDocument &json_document, const char *key, bool &value);

			static ResponseCode GetIntValue(const JsonDocument &json_document, const char *key, int &value);

			static ResponseCode GetUint16Value(const JsonDocument &json_document, const char *key,
											   uint16_t &value);

			static ResponseCode GetUint32Value(const JsonDocument &json_document, const char *key,
											   uint32_t &value);

			static ResponseCode GetSizeTValue(const JsonDocument &json_document, const char *key, size_t &value);

			static ResponseCode GetCStringValue(const JsonDocument &json_document, const char *key, char *value,
												uint16_t max_string_len);

			static ResponseCode GetStringValue(const JsonDocument &json_document, const char *key,
											   util::String &value);

			static rapidjson::ParseErrorCode GetParseErrorCode(const JsonDocument &json_document);

			static size_t GetParseErrorOffset(const JsonDocument &json_document);

			static ResponseCode MergeValues(JsonValue &target, JsonValue &source, JsonValue::AllocatorType& allocator);

			static ResponseCode DiffValues(JsonValue &target_doc, JsonValue &old_doc, JsonValue &new_doc, JsonValue::AllocatorType& allocator);

			static util::String ToString(JsonDocument &json_document);

			static util::String ToString(JsonValue &json_value);
		};
	}
}
