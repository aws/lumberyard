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

#ifndef CRYINCLUDE_CRYCOMMON_ISERIALIZE_INFO_H
#define CRYINCLUDE_CRYCOMMON_ISERIALIZE_INFO_H
#pragma once

#include <ISerialize.h> // <> required for Interfuscator

STRUCT_INFO_BEGIN(SNetObjectID)
STRUCT_VAR_INFO(id, TYPE_INFO(uint16))
STRUCT_VAR_INFO(salt, TYPE_INFO(uint16))
STRUCT_INFO_END(SNetObjectID)

STRUCT_INFO_EMPTY(SSerializeString)


#endif // CRYINCLUDE_CRYCOMMON_ISERIALIZE_INFO_H
