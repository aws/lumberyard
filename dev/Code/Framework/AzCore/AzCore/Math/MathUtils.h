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
#include <math.h>
#include <limits>
#include <cmath>

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
        static const float Pi       = 3.14159265358979323846f;
        static const float TwoPi    = 6.28318530717958623200f;
        static const float HalfPi   = 1.57079632679489655800f;
        static const float MaxFloatBeforePrecisionLoss  = 100000.f;
    };

    /**
     * Degrees/radians conversion
     */
    /*@{*/
    AZ_MATH_FORCE_INLINE float RadToDeg(float rad)      { return rad * 180.0f / Constants::Pi; }
    AZ_MATH_FORCE_INLINE float DegToRad(float deg)      { return deg * Constants::Pi / 180.0f; }
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
    AZ_MATH_FORCE_INLINE T GetClamp(T value, T min, T max)
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
    AZ_MATH_FORCE_INLINE T GetMin(T a, T b)     { return a < b ? a : b; }

    /**
     * Return the bigger of the 2 values.
     * \note we don't use names like clamp, min and max because many implementations define it as a macro.
     */
    template<typename T>
    AZ_MATH_FORCE_INLINE T GetMax(T a, T b)     { return a > b ? a : b; }
    
    /**
     * Return a linear interpolation between 2 values.
     */
    AZ_MATH_FORCE_INLINE float  Lerp( float a,  float b,  float t) { return a + (b - a) * t; }
    AZ_MATH_FORCE_INLINE double Lerp(double a, double b, double t) { return a + (b - a) * t; }

    /**
     * Return a value t where Lerp(a,b,t)==value (or 0 if a==b)
     */
    AZ_MATH_FORCE_INLINE  float LerpInverse( float a,  float b,  float value) { return IsClose(a, b, std::numeric_limits< float>::epsilon()) ? 0 : (value - a) / (b - a); }
    AZ_MATH_FORCE_INLINE double LerpInverse(double a, double b, double value) { return IsClose(a, b, std::numeric_limits<double>::epsilon()) ? 0 : (value - a) / (b - a); }

    /**
    * Return true if the number provided is even
    */
    template<typename T>
    AZ_MATH_FORCE_INLINE AZStd::enable_if_t<AZStd::is_integral<T>::value, bool> IsEven(T a)
    {
        return a % 2 == 0;
    }

    /**
    * Return false if the number provided is odd
    */
    template<typename T>
    AZ_MATH_FORCE_INLINE AZStd::enable_if_t<AZStd::is_integral<T>::value, bool> IsOdd(T a)
    {
        return a % 2 != 0;
    }

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
        return (std::fabs(x - y) <= epsilonValue * GetMax(GetMax(T(1.0), std::fabs(x)), std::fabs(y)));
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
}



#endif
