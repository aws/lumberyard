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

#include "Config.h"

namespace MCore
{
    class MCORE_API StringRef
    {
    public:
        MCORE_INLINE StringRef(const char* text)                    { mData = const_cast<char*>(text); }
        MCORE_INLINE const char* AsChar() const                     { return mData; }
        MCORE_INLINE uint32 GetLength() const                       { return (uint32)strlen(mData); }

        MCORE_INLINE bool operator==(const StringRef& other)        { return (strcmp(mData, other.mData) == 0); }
        MCORE_INLINE bool operator!=(const StringRef& other)        { return (strcmp(mData, other.mData) != 0); }

    private:
        char* mData;
    };
}   // namespace MCore
