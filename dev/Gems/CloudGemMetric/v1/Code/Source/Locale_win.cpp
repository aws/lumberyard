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

#include "StdAfx.h"

#include <CloudGemMetric/Locale.h>
#include <Windows.h>

namespace CloudGemMetric
{
    AZStd::string Locale::GetLocale()
    {
        CHAR lang[5] = { 0 };
        CHAR country[5] = { 0 };

        ::GetLocaleInfoA(LOCALE_USER_DEFAULT,
            LOCALE_SISO639LANGNAME,
            lang,
            sizeof(lang) / sizeof(CHAR));

        ::GetLocaleInfoA(LOCALE_USER_DEFAULT,
            LOCALE_SISO3166CTRYNAME,
            country,
            sizeof(country) / sizeof(CHAR));

        return AZStd::string::format("%s_%s", lang, country);
    }
}