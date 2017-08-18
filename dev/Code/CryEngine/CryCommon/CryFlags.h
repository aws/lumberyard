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

#ifndef CRYINCLUDE_CRYCOMMON_CRYFLAGS_H
#define CRYINCLUDE_CRYCOMMON_CRYFLAGS_H
#pragma once

#include "platform.h"
#include <limits>  // std::numeric_limits
#include "CompileTimeAssert.h"

template< typename STORAGE >
class CCryFlags
{
    // the reason for the following assert: AreMultipleFlagsActive() works incorrect if STORAGE is signed
    COMPILE_TIME_ASSERT(!std::numeric_limits< STORAGE >::is_signed);
public:
    CCryFlags(STORAGE flags = 0)
        : m_bitStorage(flags) {}
    ILINE void AddFlags(STORAGE flags) { m_bitStorage |= flags; }
    ILINE void ClearFlags(STORAGE flags) { m_bitStorage &= ~flags; }
    ILINE bool AreAllFlagsActive(STORAGE flags) const { return((m_bitStorage & flags) == flags); }
    ILINE bool AreAnyFlagsActive(STORAGE flags) const { return((m_bitStorage & flags) != 0); }
    ILINE bool AreMultipleFlagsActive() const { return (m_bitStorage & (m_bitStorage - 1)) != 0; }
    ILINE bool IsOneFlagActive() const { return m_bitStorage != 0 && !AreMultipleFlagsActive(); }
    ILINE void ClearAllFlags() { m_bitStorage = 0; }
    ILINE void SetFlags(STORAGE flags, const bool bSet)
    {
        if (bSet)
        {
            m_bitStorage |= flags;
        }
        else
        {
            m_bitStorage &= ~flags;
        }
    }
    ILINE STORAGE GetRawFlags() const { return m_bitStorage; }
    ILINE bool operator==(const CCryFlags& other) const { return m_bitStorage == other.m_bitStorage; }
    ILINE bool operator!=(const CCryFlags& other) const { return !(*this == other); }

private:
    STORAGE m_bitStorage;
};

typedef CCryFlags<uint8> CCryFlags8;
typedef CCryFlags<uint16> CCryFlags16;
typedef CCryFlags<uint32> CCryFlags32;

#endif // CRYINCLUDE_CRYCOMMON_CRYFLAGS_H
