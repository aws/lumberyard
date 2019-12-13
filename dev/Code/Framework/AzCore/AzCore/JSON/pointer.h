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

#include <AzCore/JSON/rapidjson.h>

// Make you have available rapidjson/include folder. Currently 3rdParty\rapidjson\rapidjson-1.0.2\include
#pragma push_macro("RAPIDJSON_NOMEMBERITERATORCLASS")
#ifndef RAPIDJSON_NOMEMBERITERATORCLASS
#define RAPIDJSON_NOMEMBERITERATORCLASS
#endif
#include <rapidjson/pointer.h>
#pragma pop_macro("RAPIDJSON_NOMEMBERITERATORCLASS")
