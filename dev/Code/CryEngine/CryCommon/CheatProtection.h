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

#ifndef CRYINCLUDE_CRYCOMMON_CHEATPROTECTION_H
#define CRYINCLUDE_CRYCOMMON_CHEATPROTECTION_H
#pragma once

// Exports methods for encryption. Do not remove.
// In doubt, ask Marco Corbetta.
#ifdef ENABLE_CHEAT_PROTECTION
#define CHEAT_PROTECTION_EXPORT __declspec(dllexport)
#include "SolidShield/SolidSDK.h"
#else
#define CHEAT_PROTECTION_EXPORT
#endif

#endif // CRYINCLUDE_CRYCOMMON_CHEATPROTECTION_H
