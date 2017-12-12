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
#ifndef AZCORE_NUMERIC_CAST_H
#define AZCORE_NUMERIC_CAST_H

#include "AzCore/std/typetraits/is_enum.h"
#include "AzCore/std/typetraits/is_floating_point.h"
#include "AzCore/std/typetraits/is_integral.h"
#include "AzCore/std/typetraits/is_same.h"
#include "AzCore/std/typetraits/underlying_type.h"
#include "AzCore/std/utils.h"
#include <limits>

/*
// Numeric casts add range checking when casting from one numeric type to another. It adds run-time validation (if enabled for the
// particular build configuration) to ensure that no actual data loss happens. Assigning long long(17) to an unsigned char is allowed,
// but assigning char(-1) to unsigned long long variable is not, and will result in an assert or error if the validation has been
// enabled.
//
// Because we can't do partial function specialization, I'm using enable_if to chop up the implementation into one of these
// implementations. If none of these fit, then we will get a compile error because it is an unknown conversionr.
//
//--------------------------------------------
//              TYPE <- TYPE         DigitLoss
// (A)     Integer      Unsigned         N
// (A)      Signed        Signed         N
// (B)    Unsigned        Signed         N
// (C)     Integer      Unsigned         Y
// (D)     Integer        Signed         Y
//
// (E)     Integer          Enum         -
// (F)     Integer      Floating         -
//
// (G)        Enum       Integer         -
//
// (H)    Floating       Integer         -
//
// (I)        Enum          Enum         -
//
// (J)    Floating      Floating         N
// (K)    Floating      Floating         Y
*/

//#define AZ_NUMERICCAST_ENABLED 1

#if AZ_NUMERICCAST_ENABLED
#define AZ_NUMERIC_ASSERT(expr, ...) AZ_Assert(expr, __VA_ARGS__)
#else
#define AZ_NUMERIC_ASSERT(expr, ...) void(0)
#endif

#pragma push_macro("max")
#undef max

// INTEGER -> INTEGER
// (A) Not losing digits or risking sign loss
template <typename ToType, typename FromType>
inline typename AZStd::enable_if<
    AZStd::is_integral<FromType>::value&& AZStd::is_integral<ToType>::value
    && std::numeric_limits<FromType>::digits <= std::numeric_limits<ToType>::digits
    && (!std::numeric_limits<FromType>::is_signed || std::numeric_limits<ToType>::is_signed)
    , ToType > ::type aznumeric_cast(FromType value)
{
    return static_cast<ToType>(value);
}

// (B) Not losing digits, but we are losing sign, so make sure we aren't dealing with a negative number
template <typename ToType, typename FromType>
inline typename AZStd::enable_if<
    AZStd::is_integral<FromType>::value&& AZStd::is_integral<ToType>::value
    && std::numeric_limits<FromType>::digits <= std::numeric_limits<ToType>::digits
    && std::numeric_limits<FromType>::is_signed&& !std::numeric_limits<ToType>::is_signed
    , ToType > ::type aznumeric_cast(FromType value)
{
    AZ_NUMERIC_ASSERT(
        value >= static_cast<FromType>(std::numeric_limits<ToType>::lowest()),
        "Cannot assign a negative number to an unsigned integer!");
    return static_cast<ToType>(value);
}


// (C) Maybe losing digits from an unsigned type, so make sure we don't exceed the destination max value. No check against zero is necessary.
template <typename ToType, typename FromType>
inline typename AZStd::enable_if<
    AZStd::is_integral<FromType>::value&& AZStd::is_integral<ToType>::value
    && !(std::numeric_limits<FromType>::digits <= std::numeric_limits<ToType>::digits)
    && !std::numeric_limits<FromType>::is_signed
    , ToType > ::type aznumeric_cast(FromType value)
{
    AZ_NUMERIC_ASSERT(
        value <= static_cast<FromType>(std::numeric_limits<ToType>::max()),
        "Losing unsigned high bits due to type narrowing!");
    return static_cast<ToType>(value);
}

// (D) Maybe losing digits within signed types, we need to check both the min and max values.
template <typename ToType, typename FromType>
inline typename AZStd::enable_if<
    AZStd::is_integral<FromType>::value&& AZStd::is_integral<ToType>::value
    && !(std::numeric_limits<FromType>::digits <= std::numeric_limits<ToType>::digits)
    && std::numeric_limits<FromType>::is_signed
    , ToType > ::type aznumeric_cast(FromType value)
{
    AZ_NUMERIC_ASSERT(
        value <= static_cast<FromType>(std::numeric_limits<ToType>::max()) && value >= static_cast<FromType>(std::numeric_limits<ToType>::lowest()),
        "Losing signed high bits due to type narrowing!");
    return static_cast<ToType>(value);
}

// ENUMS -> INTEGER
// (E) handled by changing the enum to its underlying type
template <typename ToType, typename FromType>
inline typename AZStd::enable_if<
    AZStd::is_enum<FromType>::value&& AZStd::is_integral<ToType>::value
    , ToType > ::type aznumeric_cast(FromType value)
{
    using UnderlyingFromType = typename AZStd::underlying_type<FromType>::type;
    return aznumeric_cast<ToType>(static_cast<UnderlyingFromType>(value));
}

// FLOATING -> INTEGER
// (E) We'll accept precision loss as long as it stays in range
template <typename ToType, typename FromType>
inline typename AZStd::enable_if<
    AZStd::is_floating_point<FromType>::value&& AZStd::is_integral<ToType>::value
    , ToType > ::type aznumeric_cast(FromType value)
{
    AZ_NUMERIC_ASSERT(
        value <= static_cast<FromType>(std::numeric_limits<ToType>::max()) && value >= static_cast<FromType>(std::numeric_limits<ToType>::lowest()),
        "Floating point value is too big!");
    return static_cast<ToType>(value);
}

// INTEGER -> ENUM
// (G) We must cast to an enum so go through the backing type
template <typename ToType, typename FromType>
inline typename AZStd::enable_if<
    AZStd::is_integral<FromType>::value&& AZStd::is_enum<ToType>::value
    , ToType > ::type aznumeric_cast(FromType value)
{
    using UnderlyingToType = typename AZStd::underlying_type<ToType>::type;
    return static_cast<ToType>(aznumeric_cast<UnderlyingToType>(value));
}

// INTEGER -> FLOATING POINT
// (H) Perhaps some faster code substitutions could be done here instead of the standard int->float calls
template <typename ToType, typename FromType>
inline typename AZStd::enable_if<
    AZStd::is_integral<FromType>::value&& AZStd::is_floating_point<ToType>::value
    , ToType > ::type aznumeric_cast(FromType value)
{
    return static_cast<ToType>(value);
}

// ENUM -> ENUM
// (I) crossing enums using the underlying type as the transport
template <typename ToType, typename FromType>
inline typename AZStd::enable_if<
    AZStd::is_enum<FromType>::value&& AZStd::is_enum<ToType>::value
    , ToType > ::type aznumeric_cast(FromType value)
{
    using UnderlyingFromType = typename AZStd::underlying_type<FromType>::type;
    using UnderlyingToType = typename AZStd::underlying_type<ToType>::type;
    return static_cast<ToType>(aznumeric_cast<UnderlyingToType>(static_cast<UnderlyingFromType>(value)));
}

// FLOATING POINT -> FLOATING POINT
// (J) crossing floats with no digit loss
template <typename ToType, typename FromType>
inline typename AZStd::enable_if<
    AZStd::is_floating_point<FromType>::value&& AZStd::is_floating_point<ToType>::value
    && std::numeric_limits<FromType>::digits <= std::numeric_limits<ToType>::digits
    , ToType > ::type aznumeric_cast(FromType value)
{
    return static_cast<ToType>(value);
}

// (K) crossing floats with digit loss
template <typename ToType, typename FromType>
inline typename AZStd::enable_if<
    AZStd::is_floating_point<FromType>::value&& AZStd::is_floating_point<ToType>::value
    && !(std::numeric_limits<FromType>::digits <= std::numeric_limits<ToType>::digits)
    , ToType > ::type aznumeric_cast(FromType value)
{
    AZ_NUMERIC_ASSERT(
        value <= static_cast<FromType>(std::numeric_limits<ToType>::max()) && value >= static_cast<FromType>(std::numeric_limits<ToType>::lowest()),
        "Floating point value is too big!");
    return static_cast<ToType>(value);
}

// This is a helper class that lets us induce the destination type of a numeric cast
// It should never be directly used by anything other than azlossy_caster.
namespace AZ
{
    template <typename FromType>
    class NumericCasted
    {
    public:
        explicit NumericCasted(FromType value)
            : m_value(value) { }
        template <typename ToType>
        operator ToType() const { return aznumeric_cast<ToType>(m_value); }

    private:
        NumericCasted() = delete;
        void operator=(NumericCasted const&) = delete;

        FromType m_value;
    };
}

// This is the primary function we should use when doing numeric casting, since it induces the
// type we need to cast to from the code rather than requiring an explicit coupling in the source.
template <typename FromType>
inline AZ::NumericCasted<FromType> aznumeric_caster(FromType value)
{
    return AZ::NumericCasted<FromType>(value);
}

#endif // AZCORE_NUMERIC_CAST_H

#pragma pop_macro("max")
#pragma once