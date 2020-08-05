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
#ifndef AZCORE_MATH_MATHUTILS_H
#define AZCORE_MATH_MATHUTILS_H

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/typetraits/is_signed.h>
#include <AzCore/std/typetraits/is_unsigned.h>
#include <AzCore/std/utils.h>
#include <limits>
#include <cmath>
#include <utility>

#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/is_integral.h>

namespace AZ
{
    /**
     * Constants!
     *
     * These are inside a namespace to try and prevent people messing them up with their PI macros... I've even seen
     * code like this before '\#define PI 2*3.14159f'...
     */
    namespace Constants
    {
        static constexpr float Pi = 3.14159265358979323846f;
        static constexpr float TwoPi = 6.28318530717958647692f;
        static constexpr float HalfPi = 1.57079632679489661923f;
        static constexpr float QuarterPi = 0.78539816339744830962f;
        static constexpr float MaxFloatBeforePrecisionLoss  = 100000.f;
    }

    /**
     * Enumerations to describe the sign and size relationship between two integer types.
     */
    enum class IntegralTypeDiff
    {
        //! Left integer is signed, right integer is signed, both are equal in size
        LSignedRSignedEqSize,

        //! Left integer is signed, right integer is unsigned, both are equal in size
        LSignedRUnsignedEqSize,

        //! Left integer is unsigned, right integer is signed, both are equal in size
        LUnsignedRSignedEqSize,

        //! Left integer is unsigned, right integer is unsigned, both are equal in size
        LUnsignedRUnsignedEqSize,

        //! Left integer is signed, right integer is signed, left integer is wider
        LSignedRSignedLWider,

        //! Left integer is signed, right integer is unsigned, left integer is wider
        LSignedRUnsignedLWider,

        //! Left integer is unsigned, right integer is signed, left integer is wider
        LUnsignedRSignedLWider,

        //! Left integer is unsigned, right integer is unsigned, left integer is wider
        LUnsignedRUnsignedLWider,

        //! Left integer is signed, right integer is signed, right integer is wider
        LSignedRSignedRWider,

        //! Left integer is signed, right integer is unsigned, right integer is wider
        LSignedRUnsignedRWider,

        //! Left integer is unsigned, right integer is signed, right integer is wider
        LUnsignedRSignedRWider,

        //! Left integer is unsigned, right integer is unsigned, right integer is wider
        LUnsignedRUnsignedRWider,
    };

    /**
     * Compares two integer types and returns their sign and size relationship.
     */
    template <typename LeftType, typename RightType>
    constexpr IntegralTypeDiff IntegralTypeCompare()
    {
        static_assert(AZStd::is_signed<LeftType>::value || AZStd::is_unsigned<LeftType>::value, "LeftType must either be a signed or unsigned integral type");
        static_assert(AZStd::is_signed<RightType>::value || AZStd::is_unsigned<RightType>::value, "RightType must either be a signed or unsigned integral type");

        if constexpr (sizeof(LeftType) < sizeof(RightType))
        {
            if constexpr(AZStd::is_signed<LeftType>::value && AZStd::is_signed<RightType>::value)
            {
                return IntegralTypeDiff::LSignedRSignedRWider;
            }
            else if constexpr(AZStd::is_signed<LeftType>::value && AZStd::is_unsigned<RightType>::value)
            {
                return IntegralTypeDiff::LSignedRUnsignedRWider;
            }
            else if constexpr(AZStd::is_unsigned<LeftType>::value && AZStd::is_signed<RightType>::value)
            {
                return IntegralTypeDiff::LUnsignedRSignedRWider;
            }
            else
            {
                return IntegralTypeDiff::LUnsignedRUnsignedRWider;
            }
        }
        else if constexpr (sizeof(LeftType) > sizeof(RightType))
        {
            if constexpr(AZStd::is_signed<LeftType>::value && AZStd::is_signed<RightType>::value)
            {
                return IntegralTypeDiff::LSignedRSignedLWider;
            }
            else if constexpr(AZStd::is_signed<LeftType>::value && AZStd::is_unsigned<RightType>::value)
            {
                return IntegralTypeDiff::LSignedRUnsignedLWider;
            }
            else if constexpr(AZStd::is_unsigned<LeftType>::value && AZStd::is_signed<RightType>::value)
            {
                return IntegralTypeDiff::LUnsignedRSignedLWider;
            }
            else
            {
                return IntegralTypeDiff::LUnsignedRUnsignedLWider;
            }
        }
        else
        {
            if constexpr(AZStd::is_signed<LeftType>::value && AZStd::is_signed<RightType>::value)
            {
                return IntegralTypeDiff::LSignedRSignedEqSize;
            }
            else if constexpr(AZStd::is_signed<LeftType>::value && AZStd::is_unsigned<RightType>::value)
            {
                return IntegralTypeDiff::LSignedRUnsignedEqSize;
            }
            else if constexpr(AZStd::is_unsigned<LeftType>::value && AZStd::is_signed<RightType>::value)
            {
                return IntegralTypeDiff::LUnsignedRSignedEqSize;
            }
            else
            {
                return IntegralTypeDiff::LUnsignedRUnsignedEqSize;
            }
        }
    }

    /** 
     * Comparison result returned by SafeIntegralCompare .
     */
    enum class IntegralCompare
    {
        LessThan,
        Equal,
        GreaterThan
    };

    /**
     * Safely compares two integers of any sign and width combination and returns an IntegralCompare result
     * that is the whether lhs is less than, equal to or greater than rhs.
     */
    template<typename LeftType, typename RightType>
    constexpr IntegralCompare SafeIntegralCompare(LeftType lhs, RightType rhs);

    /**
     * A collection of methods for clamping and constraining integer values and ranges
     * to that of a reference integer type.
     */
    template<typename SourceType, typename ClampType>
    struct ClampedIntegralLimits
    {
        /**
         * If SourceType and ClampType are different, returns the greater value of 
         * std::numeric_limits<SourceType>::lowest() and std::numeric_limits<ClampType>::lowest(),
         * otherwise returns std::numeric_limits<SourceType>::lowest().
         */
        static constexpr SourceType Min();

        /**
         * If SourceType and ClampType are different, returns the lesser value of 
         * std::numeric_limits<SourceType>::max() and std::numeric_limits<ClampType>::max(),
         * otherwise returns std::numeric_limits<SourceType>::max().
         */
        static constexpr SourceType Max();

        /**
         * Safely clamps a value of type ValueType to the [Min(), Max()] range as determined by the
         * numerical limit constraint rules described by Min() and Max() given SourceType and ClampType.
         * @param first component is the clamped value which may or may not equal the original value.
         * @param second is a Boolean flag signifying whether or not the the value was required to be clamped
         * in order to stay within ClampType's numerical range.
         */
        template<typename ValueType>
        static constexpr AZStd::pair<SourceType, bool> Clamp(ValueType value);        
    };

    /**
     * Degrees/radians conversion
     */
    /*@{*/
    constexpr float RadToDeg(float rad)      { return rad * 180.0f / Constants::Pi; }
    constexpr float DegToRad(float deg)      { return deg * Constants::Pi / 180.0f; }
    /*@}*/

    AZ_MATH_FORCE_INLINE bool IsClose(float a, float b, float tolerance) { return (fabsf(a - b) <= tolerance); }
    AZ_MATH_FORCE_INLINE bool IsClose(double a, double b, double tolerance) { return (fabs(a - b) <= tolerance); }

    AZ_MATH_FORCE_INLINE float GetSign(float x)
    {
        //return x >= 0.0f ? 1.0f : -1.0f;
        union FloatInt
        {
            float           f;
            unsigned int    u;
        } fi;
        fi.f = x;
        fi.u &= 0x80000000; // preserve sign
        fi.u |= 0x3f800000; // 1
        return fi.f;
    }

    /**
     * Return the clamped value.
     */
    template<typename T>
    constexpr T GetClamp(T value, T min, T max)
     {
        if (value < min)
        {
            return min;
        }
        else if (value > max)
        {
            return max;
        }
        else
        {
            return value;
        }
    }

    /**
     * Return the smaller of the 2 values.
     * \note we don't use names like clamp, min and max because many implementations define it as a macro.
     */
    template<typename T>
    constexpr T GetMin(T a, T b)     { return a < b ? a : b; }

    /**
     * Return the bigger of the 2 values.
     * \note we don't use names like clamp, min and max because many implementations define it as a macro.
     */
    template<typename T>
    constexpr T GetMax(T a, T b)     { return a > b ? a : b; }
    
    /**
     * Return a linear interpolation between 2 values.
     */
    constexpr float  Lerp( float a,  float b,  float t) { return a + (b - a) * t; }
    constexpr double Lerp(double a, double b, double t) { return a + (b - a) * t; }

    /**
     * Return a value t where Lerp(a,b,t)==value (or 0 if a==b)
     */
    inline float LerpInverse( float a,  float b,  float value) { return IsClose(a, b, std::numeric_limits< float>::epsilon()) ? 0 : (value - a) / (b - a); }
    inline double LerpInverse(double a, double b, double value) { return IsClose(a, b, std::numeric_limits<double>::epsilon()) ? 0 : (value - a) / (b - a); }

    /**
    * Return true if the number provided is even
    */
    template<typename T>
    constexpr AZStd::enable_if_t<AZStd::is_integral<T>::value, bool> IsEven(T a)
    {
        return a % 2 == 0;
    }

    /**
    * Return false if the number provided is odd
    */
    template<typename T>
    constexpr AZStd::enable_if_t<AZStd::is_integral<T>::value, bool> IsOdd(T a)
    {
        return a % 2 != 0;
    }

    AZ_MATH_FORCE_INLINE float GetAbs(float a) { return std::fabs(a); }
    AZ_MATH_FORCE_INLINE double GetAbs(double a) { return std::abs(a); }

    AZ_MATH_FORCE_INLINE float GetMod(float a, float b) { return fmod(a, b); }
    AZ_MATH_FORCE_INLINE double GetMod(double a, double b) { return fmod(a, b); }

    // Wraps [0,maxValue]
    AZ_MATH_FORCE_INLINE int  Wrap(int value, int maxValue)                     { return (value % maxValue) + ((value < 0) ? maxValue : 0); }
    // Wraps [minValue,maxValue]
    AZ_MATH_FORCE_INLINE int  Wrap(int value, int minValue, int maxValue)       { return Wrap(value - minValue, maxValue - minValue) + minValue; }
    // Wraps [0,maxValue]
    AZ_MATH_FORCE_INLINE float  Wrap(float value, float maxValue)                   { return fmodf(value, maxValue) + ((value < 0.0f) ? maxValue : 0.0f); }
    // Wraps [minValue,maxValue]
    AZ_MATH_FORCE_INLINE float  Wrap(float value, float minValue, float maxValue)   { return Wrap(value - minValue, maxValue - minValue) + minValue; }

    AZ_MATH_FORCE_INLINE float  GetFloatQNaN()                                  { return std::numeric_limits<float>::quiet_NaN(); }

    //! IsCloseMag(x, y, epsilon) returns true if y and x are sufficiently close, taking magnitude of x and y into account in the epsilon
    template<typename T>
    AZ_MATH_FORCE_INLINE bool IsCloseMag(T x, T y, T epsilonValue = std::numeric_limits<T>::epsilon())
    {
        return (std::fabs(x - y) <= epsilonValue * GetMax<T>(GetMax<T>(T(1.0), std::fabs(x)), std::fabs(y)));
    }

    //! ClampIfCloseMag(x, y, epsilon) returns y when x and y are within epsilon of each other (taking magnitude into account).  Otherwise returns x.
    template<typename T>
    AZ_MATH_FORCE_INLINE T ClampIfCloseMag(T x, T y, T epsilonValue = std::numeric_limits<T>::epsilon())
    {
        return IsCloseMag<T>(x, y, epsilonValue) ? y : x;
    }


    // This wrapper function exists here to ensure the correct version of isnormal is used on certain platforms (namely Android) because of the
    // math.h and cmath include order can be somewhat dangerous.  For example, cmath undefines isnormal (and a number of other C99 math macros 
    // defined in math.h) in favor of their std:: variants.
    AZ_MATH_FORCE_INLINE bool IsNormalDouble(double x)
    {
        return std::isnormal(x);
    }

    // Returns the maximum value for SourceType as constrained by the numerical range of ClampType
    template<typename SourceType, typename ClampType>    
    constexpr SourceType ClampedIntegralLimits<SourceType, ClampType>::Min()
    {
        if constexpr (AZStd::is_unsigned<ClampType>::value || AZStd::is_unsigned<SourceType>::value)
        {
            // If either SourceType or ClampType is unsigned, the lower limit will always be 0
            return 0;
        }
        else
        {
            // Both SourceType and ClampType are signed, take the greater of the lower limits of each type
            return sizeof(SourceType) < sizeof(ClampType) ? 
                (std::numeric_limits<SourceType>::lowest)() : 
                static_cast<SourceType>((std::numeric_limits<ClampType>::lowest)());
        }
    }

    // Returns the minimum value for SourceType as constrained by the numerical range of ClampType
    template<typename SourceType, typename ClampType>
    constexpr SourceType ClampedIntegralLimits<SourceType, ClampType>::Max()
    {
        if constexpr (sizeof(SourceType) < sizeof(ClampType))
        {
            // If SourceType is narrower than ClampType, the upper limit will be SourceType's
            return (std::numeric_limits<SourceType>::max)();
        }
        else if constexpr (sizeof(SourceType) > sizeof(ClampType))
        {
            // If SourceType is wider than ClampType, the upper limit will be ClampType's
            return static_cast<SourceType>((std::numeric_limits<ClampType>::max)());
        }
        else
        {
            if constexpr (AZStd::is_signed<ClampType>::value)
            {
                // SourceType and ClampType are the same width, ClampType is signed
                // so our upper limit will be ClampType
                return static_cast<SourceType>((std::numeric_limits<ClampType>::max)());
            }       
            else
            {
                // SourceType and ClampType are the same width, ClampType is unsigned
                // then our upper limit will be SourceType
                return (std::numeric_limits<SourceType>::max)();
            }
        }
    }

    template<typename SourceType, typename ClampType>
    template<typename ValueType>
    constexpr AZStd::pair<SourceType, bool> ClampedIntegralLimits<SourceType, ClampType>::Clamp(ValueType value)
    {
        if (SafeIntegralCompare(value, Min()) == IntegralCompare::LessThan)
        {
            return { Min(), true };
        }
        if (SafeIntegralCompare(value, Max()) == IntegralCompare::GreaterThan)
        {
            return { Max(), true };
        }
        else
        {
            return { static_cast<SourceType>(value), false };
        }
    }
    
    template<typename LeftType, typename RightType>
    constexpr IntegralCompare SafeIntegralCompare(LeftType lhs, RightType rhs)
    {
        constexpr bool LeftSigned = AZStd::is_signed<LeftType>::value;
        constexpr bool RightSigned = AZStd::is_signed<RightType>::value;
        constexpr std::size_t LeftTypeSize = sizeof(LeftType);
        constexpr std::size_t RightTypeSize = sizeof(RightType);

        static_assert(AZStd::is_integral<LeftType>::value && AZStd::is_integral<RightType>::value,
                      "Template parameters must be integrals");

        auto comp = [lhs](auto value) -> IntegralCompare
        {
            if (lhs < value)
            {
                return IntegralCompare::LessThan;
            }
            else if (lhs > value)
            {
                return IntegralCompare::GreaterThan;
            }
            else
            {
                return IntegralCompare::Equal;
            }
        };

        if constexpr (LeftSigned == RightSigned)
        {
            return comp(rhs);
        }
        else if constexpr (LeftTypeSize > RightTypeSize)
        {
            if constexpr (LeftSigned)
            {
                // LeftTypeSize > RightTypeSize
                // LeftType is signed
                // RightType is unsigned
                return comp(static_cast<LeftType>(rhs));
            }
            else
            {
                // LeftTypeSize > RightTypeSize
                // LeftType is unsigned
                // RightType is signed
                if (rhs < 0)
                {
                    return IntegralCompare::GreaterThan;
                }
                else
                {
                    return comp(static_cast<LeftType>(rhs));
                }
            }
        }
        else if constexpr (LeftSigned)
        {
            // LeftTypeSize <= RightTypeSize
            // LeftType is signed
            // RightType is unsigned
            RightType max = static_cast<RightType>((std::numeric_limits<LeftType>::max)());

            if (rhs > max)
            {
                return IntegralCompare::LessThan;
            }
            else
            {
                return comp(static_cast<LeftType>(rhs));
            }
        }
        else if constexpr (LeftTypeSize < RightTypeSize)
        {
            // LeftType < RightType
            // LeftType is unsigned
            // RightType is signed
            RightType max = static_cast<RightType>((std::numeric_limits<LeftType>::max)());

            if (rhs < 0)
            {
                return IntegralCompare::GreaterThan;
            }
            else if (rhs > max)
            {
                return IntegralCompare::LessThan;
            }
            else
            {
                return comp(static_cast<LeftType>(rhs));
            }
        }
        else
        {
            // LeftType == RightType
            // LeftType is unsigned
            // RightType is signed
            if (rhs < 0)
            {
                return IntegralCompare::GreaterThan;
            }
            else
            {
                return comp(static_cast<LeftType>(rhs));
            }
        }
    }
}

#endif
