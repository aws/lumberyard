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

#ifndef CRYINCLUDE_CRYACTION_NETWORK_SERIALIZEDIRHELPER_H
#define CRYINCLUDE_CRYACTION_NETWORK_SERIALIZEDIRHELPER_H
#pragma once

#include "SerializeBits.h"

// Ideaally this could go into the network layer as a policy

static inline void SerializeDirHelper(TSerialize ser, Vec3& dir, int policyYaw, int policyElev)
{
    // Serialise look direction as yaw and elev. Gives better compression
    float yaw, elev;

    if (!ser.IsReading())
    {
        float xy = dir.GetLengthSquared2D();
        if (xy > 0.001f)
        {
            yaw = atan2_tpl(dir.y, dir.x);
            elev = asin_tpl(clamp_tpl(dir.z, -1.f, +1.f));
        }
        else
        {
            yaw = 0.f;
            elev = (float)fsel(dir.z, +1.f, -1.f) * (gf_PI * 0.5f);
        }
    }

    ser.Value("playerLookYaw", yaw, policyYaw);
    ser.Value("playerLookElev", elev, policyElev);

    if (ser.IsReading())
    {
        float sinYaw, cosYaw;
        float sinElev, cosElev;

        sincos_tpl(yaw, &sinYaw, &cosYaw);
        sincos_tpl(elev, &sinElev, &cosElev);

        dir.x = cosYaw * cosElev;
        dir.y = sinYaw * cosElev;
        dir.z = sinElev;
    }
}

static inline void SerializeDirHelper(CBitArray& array, Vec3& dir, int nYawBits, int nElevBits)
{
    // Serialise look direction as yaw and elev. Gives better compression
    float yaw, elev;

    if (array.IsReading() == 0)
    {
        float xy = dir.GetLengthSquared2D();
        if (xy > 0.001f)
        {
            yaw = atan2_tpl(dir.y, dir.x);
            elev = asin_tpl(clamp_tpl(dir.z, -1.f, +1.f));
        }
        else
        {
            yaw = 0.f;
            elev = (float)fsel(dir.z, +1.f, -1.f) * (gf_PI * 0.5f);
        }
    }

    array.Serialize(yaw, -gf_PI, +gf_PI, nYawBits, 1);      // NB: reduce range by 1 so that zero is correctly replicated
    array.Serialize(elev, -gf_PI * 0.5f, +gf_PI * 0.5f, nElevBits, 1);

    if (array.IsReading())
    {
        float sinYaw, cosYaw;
        float sinElev, cosElev;

        sincos_tpl(yaw, &sinYaw, &cosYaw);
        sincos_tpl(elev, &sinElev, &cosElev);

        dir.x = cosYaw * cosElev;
        dir.y = sinYaw * cosElev;
        dir.z = sinElev;
    }
}

static inline void SerializeDirVector(CBitArray& array, Vec3& dir, float maxLength, int nScaleBits, int nYawBits, int nElevBits)
{
    float length;
    if (array.IsReading())
    {
        SerializeDirHelper(array, dir, nYawBits, nElevBits);
        array.Serialize(length, 0.f, maxLength, nScaleBits);
        dir = dir * length;
    }
    else
    {
        length = dir.GetLength();
        Vec3 tmp = (length > 0.f) ? dir / length : dir;
        SerializeDirHelper(array, tmp, nYawBits, nElevBits);
        array.Serialize(length, 0.f, maxLength, nScaleBits);
    }
}

#endif // CRYINCLUDE_CRYACTION_NETWORK_SERIALIZEDIRHELPER_H
