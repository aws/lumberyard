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

#include "stdafx.h"

#include "ImageProperties.h"
#include "CompileTimeAssert.h"


static const char* s_mipDownsizingMethodNames[][2] =
{
    { "average", "1" },    // "1" obsolete setting, to keep compatibility with old files
    { "nearest", "" },
    { "gauss", "0" },      // "0" obsolete setting, to keep compatibility with old files
    { "kaiser", "" },
    { "lanczos", "" }
};


inline int CImageProperties::GetMipDownsizingMethodDefault()
{
    return 2;
}


inline int CImageProperties::GetMipDownsizingMethodCount()
{
    return sizeof(s_mipDownsizingMethodNames) / sizeof(s_mipDownsizingMethodNames[0]);
}


const char* CImageProperties::GetMipDownsizingMethodName(int idx)
{
    if (idx < 0 || idx >= GetMipDownsizingMethodCount())
    {
        idx = GetMipDownsizingMethodDefault();
    }
    return s_mipDownsizingMethodNames[idx][0];
}


const int CImageProperties::GetMipDownsizingMethodIndex(int platform) const
{
    if (platform < 0)
    {
        platform = m_pCC->platform;
    }

    const int defaultIndex = GetMipDownsizingMethodDefault();
    const char* const pDefaultName = GetMipDownsizingMethodName(defaultIndex);

    const string name = m_pCC->multiConfig->getConfig(platform).GetAsString("mipgentype", pDefaultName, pDefaultName);

    const int n = GetMipDownsizingMethodCount();
    for (int i = 0; i < n; ++i)
    {
        const char* const p0 = s_mipDownsizingMethodNames[i][0];
        const char* const p1 = s_mipDownsizingMethodNames[i][1];
        if (_stricmp(name.c_str(), p0) == 0 || _stricmp(name.c_str(), p1) == 0)
        {
            return i;
        }
    }

    RCLogWarning("Unknown downsizing method: '%s'. Using '%s'.", name.c_str(), s_mipDownsizingMethodNames[defaultIndex][0]);

    return defaultIndex;
}

// eof
