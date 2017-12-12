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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _WIN32
#include <WinSock2.h>
#else
#define UNREFERENCED_PARAMETER(P) (P)
#include <stddef.h>
#include <time.h>
#include <string.h>

static int strnicmp(char const *s1, char const *s2, size_t n)
{
    return strncasecmp(s1, s2, n);
}

#endif
#include <set>
#include <map>
#include <vector>
#include <algorithm>

#include "DBTypes.h"
#include "IDBConnection.h"
#include "DBAPI.h"
