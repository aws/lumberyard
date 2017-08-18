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

// Description : Assert dialog box
#ifndef CRYINCLUDE_CRYCOMMON_CRYASSERT_H
#define CRYINCLUDE_CRYCOMMON_CRYASSERT_H
#pragma once

//-----------------------------------------------------------------------------------------------------
// Just undef this if you want to use the standard assert function
//-----------------------------------------------------------------------------------------------------

#if defined(_DEBUG) || defined(FORCE_ASSERTS_IN_PROFILE)
#undef USE_CRY_ASSERT
#define USE_CRY_ASSERT
#endif

#if defined(FORCE_STANDARD_ASSERT)
#undef USE_CRY_ASSERT
#endif


//-----------------------------------------------------------------------------------------------------
// Use like this:
// CRY_ASSERT(expression);
// CRY_ASSERT_MESSAGE(expression,"Useful message");
// CRY_ASSERT_TRACE(expression,("This should never happen because parameter n%d named %s is %f",iParameter,szParam,fValue));
//-----------------------------------------------------------------------------------------------------

#if defined(USE_CRY_ASSERT) && (defined(WIN32) || defined(DURANGO) || defined(APPLE) || defined(LINUX))

void CryAssertTrace(const char*, ...);
bool CryAssert(const char*, const char*, unsigned int, bool*);
void CryDebugBreak();

    #define CRY_ASSERT(condition) CRY_ASSERT_MESSAGE(condition, NULL)

    #define CRY_ASSERT_MESSAGE(condition, message) CRY_ASSERT_TRACE(condition, (message))

    #define CRY_ASSERT_TRACE(condition, parenthese_message)                  \
    do                                                                       \
    {                                                                        \
        static bool s_bIgnoreAssert = false;                                 \
        if (!s_bIgnoreAssert && !(condition))                                \
        {                                                                    \
            CryAssertTrace parenthese_message;                               \
            if (CryAssert(#condition, __FILE__, __LINE__, &s_bIgnoreAssert)) \
            {                                                                \
                DEBUG_BREAK;                                                 \
            }                                                                \
        }                                                                    \
    } while (0)

    #undef assert
    #define assert CRY_ASSERT

#else

        #include <assert.h>
        #define CRY_ASSERT(condition) assert(condition)
        #define CRY_ASSERT_MESSAGE(condition, message) assert(condition)
        #define CRY_ASSERT_TRACE(condition, parenthese_message) assert(condition)
#endif

// This forces boost to use CRY_ASSERT, regardless of what it is defined as
// See also: boost/assert.hpp
#define BOOST_ENABLE_ASSERT_HANDLER
namespace boost
{
    inline void assertion_failed_msg(const char* expr, const char* msg, const char* function, const char* file, long line)
    {
        CRY_ASSERT_TRACE(false, ("An assertion failed in boost: expr=%s, msg=%s, function=%s, file=%s, line=%d", expr, msg, function, file, (int)line));
    }
    inline void assertion_failed(const char* expr, const char* function, const char* file, long line)
    {
        assertion_failed_msg(expr, "BOOST_ASSERT", function, file, line);
    }
}

//-----------------------------------------------------------------------------------------------------


#endif // CRYINCLUDE_CRYCOMMON_CRYASSERT_H
