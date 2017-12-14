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

namespace AZ
{
    AZ_MATH_FORCE_INLINE Color::Color(const VectorFloat& x)                                                                         { Set(x); }
    AZ_MATH_FORCE_INLINE Color::Color(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w)       { Set(x, y, z, w); }

    AZ_MATH_FORCE_INLINE const Color Color::CreateZero()            { return Color(0.0f); }
    AZ_MATH_FORCE_INLINE const Color Color::CreateOne()         { return Color(1.0f); }
    AZ_MATH_FORCE_INLINE const Color Color::CreateFromFloat4(const float* values)
    {
        Color result;
        result.Set(values);
        return result;
    }
    AZ_MATH_FORCE_INLINE const Color Color::CreateFromVector3(const Vector3& v)
    {
        Color result;
        result.Set(v);
        return result;
    }
    AZ_MATH_FORCE_INLINE const Color Color::CreateFromVector3AndFloat(const Vector3& v, const VectorFloat& w)
    {
        Color result;
        result.Set(v, w);
        return result;
    }

    AZ_MATH_FORCE_INLINE u32 Color::CreateU32(u8 r, u8 g, u8 b, u8 a)
    {
        return (a << 24) | (b << 16) | (g << 8) | r;
    }

    AZ_MATH_FORCE_INLINE void Color::StoreToFloat4(float* values) const
    {
        m_color.StoreToFloat4(values);
    }

    AZ_MATH_FORCE_INLINE u8 Color::GetR8() const    { return static_cast<u8>(m_color.GetX() * 255.0f); }
    AZ_MATH_FORCE_INLINE u8 Color::GetG8() const    { return static_cast<u8>(m_color.GetY() * 255.0f); }
    AZ_MATH_FORCE_INLINE u8 Color::GetB8() const    { return static_cast<u8>(m_color.GetZ() * 255.0f); }
    AZ_MATH_FORCE_INLINE u8 Color::GetA8() const    { return static_cast<u8>(m_color.GetW() * 255.0f); }

    AZ_MATH_FORCE_INLINE void Color::SetR8(u8 r)   { m_color.SetX(static_cast<float>(r) * (1.0f / 255.0f)); }
    AZ_MATH_FORCE_INLINE void Color::SetG8(u8 g)   { m_color.SetY(static_cast<float>(g) * (1.0f / 255.0f)); }
    AZ_MATH_FORCE_INLINE void Color::SetB8(u8 b)   { m_color.SetZ(static_cast<float>(b) * (1.0f / 255.0f)); }
    AZ_MATH_FORCE_INLINE void Color::SetA8(u8 a)   { m_color.SetW(static_cast<float>(a) * (1.0f / 255.0f)); }

    AZ_MATH_FORCE_INLINE const VectorFloat Color::GetR() const    { return m_color.GetX(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Color::GetG() const    { return m_color.GetY(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Color::GetB() const    { return m_color.GetZ(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Color::GetA() const    { return m_color.GetW(); }

    AZ_MATH_FORCE_INLINE const VectorFloat Color::GetElement(int index) const
    {
        return m_color.GetElement(index);
    }

    AZ_MATH_FORCE_INLINE void Color::SetR(const VectorFloat& r)   { m_color.SetX(r); }
    AZ_MATH_FORCE_INLINE void Color::SetG(const VectorFloat& g)   { m_color.SetY(g); }
    AZ_MATH_FORCE_INLINE void Color::SetB(const VectorFloat& b)   { m_color.SetZ(b); }
    AZ_MATH_FORCE_INLINE void Color::SetA(const VectorFloat& a)   { m_color.SetW(a); }

    AZ_MATH_FORCE_INLINE void Color::Set(const VectorFloat& x)    { m_color.Set(x); }
    AZ_MATH_FORCE_INLINE void Color::Set(const VectorFloat& r, const VectorFloat& g, const VectorFloat& b, const VectorFloat& a)  { m_color.Set(r, g, b, a); }
    AZ_MATH_FORCE_INLINE void Color::Set(const float values[4])    { m_color.Set(values); }
    AZ_MATH_FORCE_INLINE void Color::Set(const Vector3& v)        { m_color.Set(v); }
    AZ_MATH_FORCE_INLINE void Color::Set(const Vector3& color, const VectorFloat& a)  { m_color.Set(color, a); }
    AZ_MATH_FORCE_INLINE void Color::SetElement(int index, const VectorFloat& v)  { m_color.SetElement(index, v); }

    AZ_MATH_FORCE_INLINE Vector3 Color::GetAsVector3() const { return m_color.GetAsVector3(); }
    AZ_MATH_FORCE_INLINE Vector4 Color::GetAsVector4() const { return m_color; }


    AZ_MATH_FORCE_INLINE bool Color::IsClose(const Color& v, const VectorFloat& tolerance) const
    {
        return m_color.IsClose(v.GetAsVector4(), tolerance);
    }

    AZ_MATH_FORCE_INLINE bool Color::IsZero(const VectorFloat& tolerance) const
    { 
        return IsClose(CreateZero(), tolerance); 
    }

    AZ_MATH_FORCE_INLINE bool Color::operator==(const Color& rhs) const
    {
        return m_color == rhs.m_color;
    }

    AZ_MATH_FORCE_INLINE bool Color::operator!=(const Color& rhs) const
    {
        return m_color != rhs.m_color;
    }

    // Color to u32 => 0xAABBGGRR (COLREF format)
    AZ_MATH_FORCE_INLINE u32 Color::ToU32()  const { return CreateU32(GetR8(), GetG8(), GetB8(), GetA8()); }

    // Color from u32 => 0xAABBGGRR (COLREF format)
    AZ_MATH_FORCE_INLINE void Color::FromU32(u32 c) 
    {
        SetA(static_cast<VectorFloat>(static_cast<float>(c >> 24) * (1.0f / 255.0f)));
        SetB(static_cast<VectorFloat>(static_cast<float>((c >> 16) & 0xff) * (1.0f / 255.0f)));
        SetG(static_cast<VectorFloat>(static_cast<float>((c >> 8) & 0xff) * (1.0f / 255.0f)));
        SetR(static_cast<VectorFloat>(static_cast<float>(c & 0xff) * (1.0f / 255.0f)));
    }
    AZ_MATH_FORCE_INLINE u32 Color::ToU32LinearToGamma() const
    {
        return LinearToGamma().ToU32();
    }

    AZ_MATH_FORCE_INLINE void Color::FromU32GammaToLinear(u32 c)
    {
        FromU32(c);
        *this = GammaToLinear();
    }

    AZ_MATH_FORCE_INLINE Color Color::LinearToGamma() const
    {
        VectorFloat r = GetR();
        VectorFloat g = GetG();
        VectorFloat b = GetB();

        r = (r <= 0.0031308 ? VectorFloat(12.92f * r) : VectorFloat(static_cast<float>(1.055 * pow(static_cast<double>(r), 1.0 / 2.4) - 0.055)));
        g = (g <= 0.0031308 ? VectorFloat(12.92f * g) : VectorFloat(static_cast<float>(1.055 * pow(static_cast<double>(g), 1.0 / 2.4) - 0.055)));
        b = (b <= 0.0031308 ? VectorFloat(12.92f * b) : VectorFloat(static_cast<float>(1.055 * pow(static_cast<double>(b), 1.0 / 2.4) - 0.055)));

        return Color(r,g,b,GetA());
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_MATH_FORCE_INLINE Color Color::GammaToLinear() const
    {
        float r = GetR();
        float g = GetG();
        float b = GetB();

        return Color(static_cast<VectorFloat>(r <= 0.04045 ? (r / 12.92f) : static_cast<float>(pow((static_cast<double>(r) + 0.055) / 1.055, 2.4))),
            static_cast<VectorFloat>(g <= 0.04045 ? (g / 12.92f) : static_cast<float>(pow((static_cast<double>(g) + 0.055) / 1.055, 2.4))),
            static_cast<VectorFloat>(b <= 0.04045 ? (b / 12.92f) : static_cast<float>(pow((static_cast<double>(b) + 0.055) / 1.055, 2.4))), GetA());
    }

    AZ_MATH_FORCE_INLINE bool Color::IsLessThan(const Color& rhs) const         { return (GetR() < rhs.GetR()) && (GetG() < rhs.GetG()) && (GetB() < rhs.GetB()) && (GetA() < rhs.GetA()); }
    AZ_MATH_FORCE_INLINE bool Color::IsLessEqualThan(const Color& rhs) const    { return (GetR() <= rhs.GetR()) && (GetG() <= rhs.GetG()) && (GetB() <= rhs.GetB()) && (GetA() <= rhs.GetA()); }
    AZ_MATH_FORCE_INLINE bool Color::IsGreaterThan(const Color& rhs) const      { return (GetR() > rhs.GetR()) && (GetG() > rhs.GetG()) && (GetB() > rhs.GetB()) && (GetA() > rhs.GetA()); }
    AZ_MATH_FORCE_INLINE bool Color::IsGreaterEqualThan(const Color& rhs) const { return (GetR() >= rhs.GetR()) && (GetG() >= rhs.GetG()) && (GetB() >= rhs.GetB()) && (GetA() >= rhs.GetA()); }

    AZ_MATH_FORCE_INLINE const VectorFloat Color::Dot(const Color& rhs) const
    {
        return (m_color.Dot(rhs.m_color));
    }
    AZ_MATH_FORCE_INLINE const VectorFloat Color::Dot3(const Color& rhs) const
    {
        return (m_color.Dot3(rhs.m_color.GetAsVector3()));
    }

    AZ_MATH_FORCE_INLINE const Color Color::operator-() const
    {
        return Color(-m_color);
    }
    AZ_MATH_FORCE_INLINE const Color Color::operator+(const Color& rhs) const
    {
        return Color(m_color + rhs.m_color);
    }
    AZ_MATH_FORCE_INLINE const Color Color::operator-(const Color& rhs) const
    {
        return Color(m_color - rhs.m_color);
    }
    AZ_MATH_FORCE_INLINE const Color Color::operator*(const Color& rhs) const
    {
        return Color(m_color * rhs.m_color);
    }
    AZ_MATH_FORCE_INLINE const Color Color::operator/(const Color& rhs) const
    {
        return Color(m_color / rhs.m_color);
    }
    AZ_MATH_FORCE_INLINE const Color Color::operator*(const VectorFloat& multiplier) const
    {
        return Color(m_color * multiplier);
    }
    AZ_MATH_FORCE_INLINE const Color Color::operator/(const VectorFloat& divisor) const
    {
        return Color(m_color / divisor);
    }

    AZ_MATH_FORCE_INLINE const Color operator*(const VectorFloat& multiplier, const Color& rhs)
    {
        return rhs * multiplier;
    }
}
