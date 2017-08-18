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

#ifndef CRYINCLUDE_CRYTEXTURESQUISHER_COLORTYPES_H
#define CRYINCLUDE_CRYTEXTURESQUISHER_COLORTYPES_H
#pragma once


#include "platform.h"           // uint32, uint16 etc.
#include "CompileTimeAssert.h"  // COMPILE_TIME_ASSERT()


// 16 bit 565 BGR color
struct ColorB5G6R5
{
    ColorB5G6R5()
    {
    }

    ColorB5G6R5(const ColorB5G6R5& a_c)
        : u(a_c.u)
    {
    }

    explicit ColorB5G6R5(uint16 a_u)
        : u(a_u)
    {
    }

    union
    {
        struct
        {
            uint16 b : 5;
            uint16 g : 6;
            uint16 r : 5;
        };
        uint16 u;
    };
};

COMPILE_TIME_ASSERT(sizeof(ColorB5G6R5) == 2);


// 32 bit 8888 BGRA color
struct ColorBGRA8
{
    ColorBGRA8()
    {
    }

    ColorBGRA8(const ColorBGRA8& a_c)
        : u(a_c.u)
    {
    }

    ColorBGRA8(uint8 a_b, uint8 a_g, uint8 a_r, uint8 a_a)
        : b(a_b)
        , g(a_g)
        , r(a_r)
        , a(a_a)
    {
    }

    explicit ColorBGRA8(uint32 a_u)
        : u(a_u)
    {
    }

    void setBGRA(uint8 a_b, uint8 a_g, uint8 a_r, uint8 a_a)
    {
        b = a_b;
        g = a_g;
        r = a_r;
        a = a_a;
    }

    void setBGRA(const uint8* a_pBGRA8)
    {
        b = a_pBGRA8[0];
        g = a_pBGRA8[1];
        r = a_pBGRA8[2];
        a = a_pBGRA8[3];
    }

    union
    {
        struct
        {
            uint8 b;
            uint8 g;
            uint8 r;
            uint8 a;
        };
        uint32 u;
    };
};

// 64 bit 16161616 BGRA color
struct ColorBGRA16
{
    ColorBGRA16()
    {
    }

    ColorBGRA16(const ColorBGRA16& a_c)
        : u(a_c.u)
    {
    }

    ColorBGRA16(uint16 a_b, uint16 a_g, uint16 a_r, uint16 a_a)
        : b(a_b)
        , g(a_g)
        , r(a_r)
        , a(a_a)
    {
    }

    explicit ColorBGRA16(uint64 a_u)
        : u(a_u)
    {
    }

    void setBGRA(uint16 a_b, uint16 a_g, uint16 a_r, uint16 a_a)
    {
        b = a_b;
        g = a_g;
        r = a_r;
        a = a_a;
    }

    void setBGRA(const uint16* a_pBGRA16)
    {
        b = a_pBGRA16[0];
        g = a_pBGRA16[1];
        r = a_pBGRA16[2];
        a = a_pBGRA16[3];
    }

    union
    {
        struct
        {
            uint16 b;
            uint16 g;
            uint16 r;
            uint16 a;
        };
        uint64 u;
    };
};

// 32 bit 8888 RGBA color
struct ColorRGBA8
{
    ColorRGBA8()
    {
    }

    ColorRGBA8(const ColorRGBA8& a_c)
        : u(a_c.u)
    {
    }

    ColorRGBA8(uint8 a_r, uint8 a_g, uint8 a_b, uint8 a_a)
        : r(a_r)
        , g(a_g)
        , b(a_b)
        , a(a_a)
    {
    }

    explicit ColorRGBA8(uint32 a_u)
        : u(a_u)
    {
    }

    void setRGBA(uint8 a_r, uint8 a_g, uint8 a_b, uint8 a_a)
    {
        r = a_r;
        g = a_g;
        b = a_b;
        a = a_a;
    }

    void setRGBA(const uint8* a_pRGBA8)
    {
        r = a_pRGBA8[0];
        g = a_pRGBA8[1];
        b = a_pRGBA8[2];
        a = a_pRGBA8[3];
    }

    void setBGRA(const uint8* a_pBGRA8)
    {
        b = a_pBGRA8[0];
        g = a_pBGRA8[1];
        r = a_pBGRA8[2];
        a = a_pBGRA8[3];
    }

    void getRGBA(uint8& a_r, uint8& a_g, uint8& a_b, uint8& a_a) const
    {
        a_r = r;
        a_g = g;
        a_b = b;
        a_a = a;
    }

    void getRGBA(uint8* a_pRGBA8) const
    {
        a_pRGBA8[0] = r;
        a_pRGBA8[1] = g;
        a_pRGBA8[2] = b;
        a_pRGBA8[3] = a;
    }

    void getBGRA(uint8* a_pBGRA8) const
    {
        a_pBGRA8[0] = b;
        a_pBGRA8[1] = g;
        a_pBGRA8[2] = r;
        a_pBGRA8[3] = a;
    }

    union
    {
        struct
        {
            uint8 r;
            uint8 g;
            uint8 b;
            uint8 a;
        };
        uint32 u;
    };
};

// 64 bit 16161616 RGBA color
struct ColorRGBA16
{
    ColorRGBA16()
    {
    }

    ColorRGBA16(const ColorRGBA16& a_c)
        : u(a_c.u)
    {
    }

    ColorRGBA16(uint16 a_r, uint16 a_g, uint16 a_b, uint16 a_a)
        : r(a_r)
        , g(a_g)
        , b(a_b)
        , a(a_a)
    {
    }

    explicit ColorRGBA16(uint64 a_u)
        : u(a_u)
    {
    }

    void setRGBA(uint16 a_r, uint16 a_g, uint16 a_b, uint16 a_a)
    {
        r = a_r;
        g = a_g;
        b = a_b;
        a = a_a;
    }

    void setRGBA(const uint16* a_pRGBA16)
    {
        r = a_pRGBA16[0];
        g = a_pRGBA16[1];
        b = a_pRGBA16[2];
        a = a_pRGBA16[3];
    }

    void setBGRA(const uint16* a_pBGRA16)
    {
        b = a_pBGRA16[0];
        g = a_pBGRA16[1];
        r = a_pBGRA16[2];
        a = a_pBGRA16[3];
    }

    void getRGBA(uint16& a_r, uint16& a_g, uint16& a_b, uint16& a_a) const
    {
        a_r = r;
        a_g = g;
        a_b = b;
        a_a = a;
    }

    void getRGBA(uint16* a_pRGBA16) const
    {
        a_pRGBA16[0] = r;
        a_pRGBA16[1] = g;
        a_pRGBA16[2] = b;
        a_pRGBA16[3] = a;
    }

    void getBGRA(uint16* a_pBGRA16) const
    {
        a_pBGRA16[0] = b;
        a_pBGRA16[1] = g;
        a_pBGRA16[2] = r;
        a_pBGRA16[3] = a;
    }

    union
    {
        struct
        {
            uint16 r;
            uint16 g;
            uint16 b;
            uint16 a;
        };
        uint64 u;
    };
};

// 128 bit (4 floats) RGBA color
struct ColorRGBAf
{
    ColorRGBAf()
    {
    }

    ColorRGBAf(const ColorRGBAf& a_c)
        : r(a_c.r)
        , g(a_c.g)
        , b(a_c.b)
        , a(a_c.a)
    {
    }

    ColorRGBAf(float a_r, float a_g, float a_b, float a_a)
        : r(a_r)
        , g(a_g)
        , b(a_b)
        , a(a_a)
    {
    }

    void setRGBA(float a_r, float a_g, float a_b, float a_a)
    {
        r = a_r;
        g = a_g;
        b = a_b;
        a = a_a;
    }

    void setRGBA(const float* a_pRGBAf)
    {
        r = a_pRGBAf[0];
        g = a_pRGBAf[1];
        b = a_pRGBAf[2];
        a = a_pRGBAf[3];
    }

    void getRGBA(float& a_r, float& a_g, float& a_b, float& a_a) const
    {
        a_r = r;
        a_g = g;
        a_b = b;
        a_a = a;
    }

    void getRGBA(float* a_pRGBAf) const
    {
        a_pRGBAf[0] = r;
        a_pRGBAf[1] = g;
        a_pRGBAf[2] = b;
        a_pRGBAf[3] = a;
    }

    union
    {
        struct
        {
            float r;
            float g;
            float b;
            float a;
        };
    };
};

COMPILE_TIME_ASSERT(sizeof(ColorBGRA8) == 4);
COMPILE_TIME_ASSERT(sizeof(ColorRGBA8) == 4);
COMPILE_TIME_ASSERT(sizeof(ColorBGRA16) == 8);
COMPILE_TIME_ASSERT(sizeof(ColorRGBA16) == 8);
COMPILE_TIME_ASSERT(sizeof(ColorRGBAf) == 16);

#endif // CRYINCLUDE_CRYTEXTURESQUISHER_COLORTYPES_H
