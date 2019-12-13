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

#pragma once
#include <platform.h>

#if defined(RESOURCE_COMPILER)
typedef CryStringLocalT<char> string;
typedef CryStringLocalT<wchar_t> wstring;
#else
typedef CryStringT<char> string;
typedef CryStringT<wchar_t> wstring;
#endif

template <class Type>
struct less_stricmp
{
    bool operator()(const string& left, const string& right) const
    {
        return _stricmp(left.c_str(), right.c_str()) < 0;
    }
    bool operator()(const char* left, const char* right) const
    {
        return _stricmp(left, right) < 0;
    }
};
