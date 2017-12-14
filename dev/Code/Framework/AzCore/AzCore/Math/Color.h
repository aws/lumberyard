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

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace AZ
{
    /**
     * A color class with 4 components, you probably only want to use this if you're
     * dealing with any sort of color manipulation.
     */
    class Color
    {
    public:
        AZ_TYPE_INFO(Color, "{7894072A-9050-4F0F-901B-34B1A0D29417}")

        //===============================================================
        // Constructors
        //===============================================================

        ///Default constructor, components are uninitialized
        Color()   { }
        Color(const Vector4& v)   { m_color = v; }

        ///Constructs vector with all components set to the same specified value
        explicit Color(const VectorFloat& rgba);

        explicit Color(const VectorFloat& r, const VectorFloat& g, const VectorFloat& b, const VectorFloat& a);

        explicit Color(float r, float g, float b, float a) { SetR(static_cast<const VectorFloat&>(r)); SetG(static_cast<const VectorFloat&>(g)); SetB(static_cast<const VectorFloat&>(b)); SetA(static_cast<const VectorFloat&>(a)); }

        explicit Color(u8 r, u8 g, u8 b, u8 a) { SetR8(r); SetG8(g); SetB8(b); SetA8(a); }

        ///Creates a vector with all components set to zero, more efficient than calling Color(0.0f)
        static const Color CreateZero();

        ///Creates a vector with all components set to one, more efficient than calling Color(1.0f)
        static const Color CreateOne();

        ///Sets components from an array of 4 floats, stored in xyzw order
        static const Color CreateFromFloat4(const float* values);

        ///Copies r,g,b components from a Vector3, sets w to 1.0
        static const Color CreateFromVector3(const Vector3& v);

        ///Copies r,g,b components from a Vector3, specify w separately
        static const Color CreateFromVector3AndFloat(const Vector3& v, const VectorFloat& w);

        /// r,g,b,a to u32 => 0xAABBGGRR (COLREF format)
        static u32 CreateU32(u8 r, u8 g, u8 b, u8 a);

        //===============================================================
        // Store
        //===============================================================

        ///Stores the vector to an array of 4 floats. The floats need only be 4 byte aligned, 16 byte alignment is NOT
        ///required.
        void StoreToFloat4(float* values) const;

        //===============================================================
        // Component access
        //===============================================================

        u8 GetR8() const;
        u8 GetG8() const;
        u8 GetB8() const;
        u8 GetA8() const;

        void SetR8(u8 r);
        void SetG8(u8 g);
        void SetB8(u8 b);
        void SetA8(u8 a);

        const VectorFloat GetR() const;
        const VectorFloat GetG() const;
        const VectorFloat GetB() const;
        const VectorFloat GetA() const;

        void SetR(const VectorFloat& r);
        void SetG(const VectorFloat& g);
        void SetB(const VectorFloat& b);
        void SetA(const VectorFloat& a);

        // We recommend using GetR,G,B,A. GetElement can be slower.
        const VectorFloat GetElement(int index) const;
        // We recommend using SetR,G,B,A. SetElement can be slower.
        void SetElement(int index, const VectorFloat& v);

        Vector3 GetAsVector3() const;

        Vector4 GetAsVector4() const;

        ///Sets all components to the same specified value
        void Set(const VectorFloat& x);

        void Set(const VectorFloat& r, const VectorFloat& g, const VectorFloat& b, const VectorFloat& a);

        ///Sets components from an array of 4 floats, stored in rgba order
        void Set(const float values[4]);

        ///Sets r,g,b components from a Vector3, sets a to 1.0
        void Set(const Vector3& v);

        ///Sets r,g,b components from a Vector3, specify a separately
        void Set(const Vector3& v, const VectorFloat& a);

        //===============================================================
        // Comparisons
        //===============================================================

        ///Checks the color is equal to another within a floating point tolerance
        bool IsClose(const Color& v, const VectorFloat& tolerance = g_simdTolerance) const;

        bool IsZero(const VectorFloat& tolerance = g_fltEps) const;

        bool operator==(const Color& rhs) const;
        bool operator!=(const Color& rhs) const;

        explicit operator Vector3() const { return m_color.GetAsVector3(); }
        explicit operator Vector4() const { return m_color; }

        Color& operator=(const Vector3& rhs)
        {
            Set(rhs);
            return *this;
        }

        // Color to u32 => 0xAABBGGRR
        u32 ToU32() const;

        // Color to u32 => 0xAABBGGRR, RGB convert from Linear to Gamma corrected values.
        u32 ToU32LinearToGamma() const;

        // Color from u32 => 0xAABBGGRR
        void FromU32(u32 c);

        // Color from u32 => 0xAABBGGRR, RGB convert from Gamma corrected to Linear values.
        void FromU32GammaToLinear(u32 c);

        // Convert color from linear to gamma corrected space.
        Color LinearToGamma() const;

        // Convert color from gamma corrected to linear space.
        Color GammaToLinear() const;

        /**
        * Comparison functions, not implemented as operators since that would probably be a little dangerous. These
        * functions return true only if all components pass the comparison test.
        */
        /*@{*/
        bool IsLessThan(const Color& rhs) const;
        bool IsLessEqualThan(const Color& rhs) const;
        bool IsGreaterThan(const Color& rhs) const;
        bool IsGreaterEqualThan(const Color& rhs) const;
        /*@}*/

        //===============================================================
        // Dot products
        //===============================================================

        ///Dot product of two colors, uses all 4 components.
        const VectorFloat Dot(const Color& rhs) const;

        //Dot product of two colors, using only the r,g,b components.
        const VectorFloat Dot3(const Color& rhs) const;

        //===============================================================
        // Standard operators
        //===============================================================

        const Color operator-() const;
        const Color operator+(const Color& rhs) const;
        const Color operator-(const Color& rhs) const;
        const Color operator*(const Color& rhs) const;
        const Color operator/(const Color& rhs) const;
        const Color operator*(const VectorFloat& multiplier) const;
        const Color operator/(const VectorFloat& divisor) const;

        AZ_MATH_FORCE_INLINE Color& operator+=(const Color& rhs)
        {
            *this = (*this) + rhs;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Color& operator-=(const Color& rhs)
        {
            *this = (*this) - rhs;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Color& operator*=(const Color& rhs)
        {
            *this = (*this) * rhs;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Color& operator/=(const Color& rhs)
        {
            *this = (*this) / rhs;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Color& operator*=(const VectorFloat& multiplier)
        {
            *this = (*this) * multiplier;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Color& operator/=(const VectorFloat& divisor)
        {
            *this = (*this) / divisor;
            return *this;
        }
    private:
        Vector4 m_color;
    };
}

#ifndef AZ_PLATFORM_WINDOWS // Remove this once all compilers support POD (MSVC already does)
#   include <AzCore/std/typetraits/is_pod.h>
AZSTD_DECLARE_POD_TYPE(AZ::Color);
#endif

#include <AzCore/Math/Internal/Color.inl>
