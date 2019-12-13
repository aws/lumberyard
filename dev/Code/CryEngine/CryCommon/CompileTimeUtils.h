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

#ifndef CRYINCLUDE_CRYCOMMON_COMPILETIMEUTILS_H
#define CRYINCLUDE_CRYCOMMON_COMPILETIMEUTILS_H
#pragma once


#include "MetaUtils.h"

// static assert
#define STATIC_ASSERT(condition, errMessage) static_assert(condition, errMessage)


// static_max
template<int left, int right>
struct static_max
{
    enum
    {
        value = (left > right ? left : right),
    };
};

// static_min
template<int left, int right>
struct static_min
{
    enum
    {
        value = (left < right ? left : right),
    };
};


// static_log2
template<unsigned num>
struct static_log2
{
    enum
    {
        value = static_log2<(num >> 1)>::value + 1,
    };
};

template<>
struct static_log2<1>
{
    enum
    {
        value = 0,
    };
};

template<>
struct static_log2<0>
{
};


// aligment_type - returns POD type with the desired alignment
template<size_t Alignment>
struct alignment_type
{
private:
    template<typename AlignmentType>
    struct SameAlignment
    {
        enum
        {
            value = Alignment == alignof(AlignmentType),
        };
    };

public:
    typedef int (alignment_type::* mptr);
    typedef int (alignment_type::* mfptr)();

    typedef typename metautils::select<SameAlignment<char>::value, char,
        typename metautils::select<SameAlignment<short>::value, short,
            typename metautils::select<SameAlignment<int>::value, int,
                typename metautils::select<SameAlignment<long>::value, long,
                    typename metautils::select<SameAlignment<long long>::value, long long,
                        typename metautils::select<SameAlignment<float>::value, float,
                            typename metautils::select<SameAlignment<double>::value, double,
                                typename metautils::select<SameAlignment<long double>::value, long double,
                                    typename metautils::select<SameAlignment<void*>::value, void*,
                                        typename metautils::select<SameAlignment<mptr>::value, mptr,
                                            typename metautils::select<SameAlignment<mfptr>::value, mfptr, char>::type
                                            >::type
                                        >::type
                                    >::type
                                >::type
                            >::type
                        >::type
                    >::type
                >::type
            >::type
        >::type type;
};


// aligned_buffer - declares a buffer with desired alignment and size
template<size_t Alignment, size_t Size>
struct aligned_buffer
{
private:
    struct storage
    {
        union
        {
            typename alignment_type<Alignment>::type _alignment;
            char _buffer[Size];
        } buffer;
    };

    storage buffer;
};

// aligned_storage - declares a buffer that can naturally hold the passed type
template<typename Ty>
struct aligned_storage
{
private:
    struct storage
    {
        union
        {
            typename alignment_type<alignof(Ty)>::type _alignment;
            char _buffer[sizeof(Ty)];
        } buffer;
    };

    storage buffer;
};

// test type equality
template <typename T1, typename T2>
struct SSameType
{
    enum
    {
        value = false
    };
};
template <typename T1>
struct SSameType<T1, T1>
{
    enum
    {
        value = true
    };
};

#endif // CRYINCLUDE_CRYCOMMON_COMPILETIMEUTILS_H
