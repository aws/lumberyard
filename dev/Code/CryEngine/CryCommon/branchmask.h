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

#ifndef CRYINCLUDE_CRYCOMMON_BRANCHMASK_H
#define CRYINCLUDE_CRYCOMMON_BRANCHMASK_H
#pragma once

///////////////////////////////////////////
// helper functions for branch elimination
//
// msb/lsb - most/less significant byte
//
// mask - 0xFFFFFFFF
// nz   - not zero
// zr   - is zero

ILINE const uint32 nz2msb(const uint32 x)
{
    return -(int32)x | x;
}

ILINE const uint32 msb2mask(const uint32 x)
{
    return (int32)(x) >> 31;
}

ILINE const uint32 nz2one(const uint32 x)
{
    return nz2msb(x) >> 31; // int((bool)x);
}

ILINE const uint32 nz2mask(const uint32 x)
{
    return (int32)msb2mask(nz2msb(x)); // -(int32)(bool)x;
}


ILINE const uint32 iselmask(const uint32 mask, uint32 x, const uint32 y)// select integer with mask (0xFFFFFFFF or 0x0 only!!!)
{
    return (x & mask) | (y & ~mask);
}


ILINE const uint32 mask_nz_nz(const uint32 x, const uint32 y)// mask if( x != 0 && y != 0)
{
    return msb2mask(nz2msb(x) & nz2msb(y));
}

ILINE const uint32 mask_nz_zr(const uint32 x, const uint32 y)// mask if( x != 0 && y == 0)
{
    return msb2mask(nz2msb(x) & ~nz2msb(y));
}


ILINE const uint32 mask_zr_zr(const uint32 x, const uint32 y)// mask if( x == 0 && y == 0)
{
    return ~nz2mask(x | y);
}

#endif // CRYINCLUDE_CRYCOMMON_BRANCHMASK_H
