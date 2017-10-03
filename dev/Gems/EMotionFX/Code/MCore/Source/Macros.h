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

namespace MCore
{
    // forward declaration
    class UnicodeString;

    /**
     * Forward declare an MCore class.
     */
#define MCORE_FORWARD_DECLARE(className) \
    namespace MCore { class className; }

    /**
     * Exposes the current class to the runtime.
     * @param name The class name.
     */
#define MCORE_DECLARE_CLASS(name)                \
    static char* MCORE_InternalGetCurrentClass() \
    {                                            \
        return (char*)#name;                     \
    }

    /**
     * Exposes the current function to the runtime.
     * @param name The function name.
     */
#ifdef MCORE_PLATFORM_WINDOWS
    #define MCORE_DECLARE_FUNCTION(name) \
    static char* MCORE_InternalGetCurrentFunction = #name ## "()";
#else
    #define MCORE_DECLARE_FUNCTION(name) \
    static char* MCORE_InternalGetCurrentFunction = #name "()";
#endif

    /**
     * Returns the name of the current function (including class name).
     * @result The name of the current function (including class name).
     */
#define MCORE_THIS_FUNCTION MCORE_InternalGetCurrentFunction

    /**
     * Returns the name of the current class.
     * @result The name of the current class.
     */
#define MCORE_THIS_CLASS MCORE_InternalGetCurrentClass()

    /**
     * Macro that returns the current function name excluding namespace, class name.
     * ::FunctionName().
     */
#define MCORE_NOCLASS_HERE \
    MCore::UnicodeString(MCore::UnicodeString("Global::") + MCore::String(MCORE_THIS_FUNCTION))

    /**
     * Macro that returns the current location including namespace, class name
     * and function name. ClassName::FunctionName().
     */
#if (MCORE_COMPILER == MCORE_COMPILER_MSVC)
    #define MCORE_HERE \
    MCore::UnicodeString(MCore::UnicodeString(MCORE_THIS_CLASS) + MCore::UnicodeString("::") + MCore::String(MCORE_THIS_FUNCTION))
#else
    #define MCORE_HERE ""
#endif

    /**
     * Macro that returns the current location. Global::.
     */
#define MCORE_GLOBAL_HERE \
    MCore::UnicodeString("Global::")
}   // namespace MCore
