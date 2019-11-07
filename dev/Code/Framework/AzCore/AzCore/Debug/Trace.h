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

#include <AzCore/PlatformDef.h>
#define AZ_VA_HAS_ARGS(...) ""#__VA_ARGS__[0] != 0


namespace AZ
{
    namespace Debug
    {
        /// Global instance to the tracer.
        extern class Trace      g_tracer;

        class Trace
        {
        public:
            static Trace& Instance()    { return g_tracer; }

            /**
            * Returns the default string used for a system window.
            * It can be useful for Trace message handlers to easily validate if the window they received is the fallback window used by this class,
            * or to force a Trace message bus handler to do special processing by using a known, consistent char*
            */
            static const char* GetDefaultSystemWindow();
            static bool IsDebuggerPresent();

            /// True or false if we want to handle system exceptions.
            static void HandleExceptions(bool isEnabled);

            /// Breaks program execution immediately.
            static void Break();

            /// Terminates the process with the specified exit code
            static void Terminate(int exitCode);

            static void Assert(const char* fileName, int line, const char* funcName, const char* format, ...);
            static void Error(const char* fileName, int line, const char* funcName, const char* window, const char* format, ...);
            static void Warning(const char* fileName, int line, const char* funcName, const char* window, const char* format, ...);
            static void Printf(const char* window, const char* format, ...);

            static void Output(const char* window, const char* message);

            static void PrintCallstack(const char* window, unsigned int suppressCount = 0, void* nativeContext = 0);

            /// PEXCEPTION_POINTERS on Windows, always NULL on other platforms
            static void* GetNativeExceptionInfo();
        };
    }
}

#ifdef AZ_ENABLE_TRACING

/**
* AZ tracing macros provide debug information reporting for assert, errors, warnings, and informational messages.
* The syntax allows printf style formatting for the message, e.g DBG_Error("System | MyWindow", true/false, "message %d", ...).
* Asserts are always sent to the "System" window, since they cannot be ignored.
*
* The four different types of macro to use, depending on the situation:
*    - Asserts should be used for critical errors, where the program cannot continue. They print the message together
*      with the file and line number. They then break program execution.
*    - Errors should be used where something is clearly wrong, but the program can continue safely. They print the message
*      together with the file and line number. Depending on platform they will notify the user that
*      an error has occurred, e.g. with a message box or an on-screen message.
*    - Warnings should be used when something could be wrong. They print the message together with the file and line number, and
*      take no other action.
*    - Printfs are purely informational. They print the message unadorned.
*    - Traces which have "Once" at the end will display the message only once for the life of the application instance.
*
* \note AZCore implements the AZStd::Default_Assert so if you want to customize it, use the Trace Listeners...
*/


/**
 * This is used to prevent incorrect usage if the user forgets to write the boolean expression to assert.
 * Example:
 *     AZ_Assert("Fail always"); // <- assertion will never fail
 * Correct usage:
 *     AZ_Assert(false, "Fail always");
 */
    
    namespace AZ
    {
        namespace TraceInternal
        {
            enum class ExpressionValidResult { Valid, Invalid_ConstCharPtr, Invalid_ConstCharArray };
            template<typename T>
            struct ExpressionIsValid
            {
                static constexpr ExpressionValidResult value = ExpressionValidResult::Valid;
            };
            template<>
            struct ExpressionIsValid<const char*&> 
            {
                static constexpr ExpressionValidResult value = ExpressionValidResult::Valid;
            };
            template<unsigned int N>
            struct ExpressionIsValid<const char(&)[N]>
            {
                static constexpr ExpressionValidResult value = ExpressionValidResult::Invalid_ConstCharArray;
            };
        } // namespace TraceInternal
    } // namespace AZ

    #if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1900
        // For VS2015 and backwards we don't support this check since it relies on relaxed constexpr c++14 feature: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3652.html
        #define AZ_TraceFmtCompileTimeCheck(...)
    #else
        #define AZ_TraceFmtCompileTimeCheck(expression, isVaArgs, baseMsg, msg, msgVargs)                                                                                                 \
        {                                                                                                                                                                                 \
            using namespace AZ::TraceInternal;                                                                                                                                            \
            auto&& rTraceFmtCompileTimeCheckExpressionHelper = (expression); /* This is needed for edge cases for expressions containing lambdas, that were unsupported before C++20 */   \
            constexpr ExpressionValidResult isValidTraceFmtResult = ExpressionIsValid<decltype(rTraceFmtCompileTimeCheckExpressionHelper)>::value;                                        \
            /* Assert different message depending whether it's const char array or if we have extra arguments */                                                                          \
            static_assert(!isVaArgs ? isValidTraceFmtResult != ExpressionValidResult::Invalid_ConstCharArray : true, baseMsg " " msg);                                                    \
            static_assert(isVaArgs  ? isValidTraceFmtResult != ExpressionValidResult::Invalid_ConstCharArray : true, baseMsg " " msgVargs);                                               \
            static_assert(!isVaArgs ? isValidTraceFmtResult != ExpressionValidResult::Invalid_ConstCharPtr   : true, baseMsg " " msg " or "#expression" != nullptr");                     \
            static_assert(isVaArgs  ? isValidTraceFmtResult != ExpressionValidResult::Invalid_ConstCharPtr   : true, baseMsg " " msgVargs " or "#expression" != nullptr");                \
        }
    #endif

    #define AZ_Assert(expression, ...)                                                                                  \
    AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                           \
    if (!(expression))                                                                                                  \
    {                                                                                                                   \
        AZ_TraceFmtCompileTimeCheck(expression, AZ_VA_HAS_ARGS(__VA_ARGS__),                                            \
            "String used in place of boolean expression for AZ_Assert.",                                                \
            "Did you mean AZ_Assert(false, \"%s\", "#expression"); ?",                                                  \
            "Did you mean AZ_Assert(false, "#expression", "#__VA_ARGS__"); ?");                                         \
        AZ::Debug::Trace::Instance().Assert(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, /*format,*/ __VA_ARGS__);        \
    }                                                                                                                   \
    AZ_POP_DISABLE_WARNING

    #define AZ_Error(window, expression, ...)                                                                              \
    AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                              \
    if (!(expression))                                                                                                     \
    {                                                                                                                      \
        AZ_TraceFmtCompileTimeCheck(expression, AZ_VA_HAS_ARGS(__VA_ARGS__),                                               \
            "String used in place of boolean expression for AZ_Error.",                                                    \
            "Did you mean AZ_Error("#window", false, \"%s\", "#expression"); ?",                                           \
            "Did you mean AZ_Error("#window", false, "#expression", "#__VA_ARGS__"); ?");                                  \
        AZ::Debug::Trace::Instance().Error(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, window, /*format,*/ __VA_ARGS__);    \
    }                                                                                                                      \
    AZ_POP_DISABLE_WARNING

    #define AZ_ErrorOnce(window, expression, ...)                                                                           \
    AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                               \
    if (!(expression))                                                                                                      \
    {                                                                                                                       \
        AZ_TraceFmtCompileTimeCheck(expression, AZ_VA_HAS_ARGS(__VA_ARGS__),                                                \
            "String used in place of boolean expression for AZ_ErrorOnce.",                                                 \
            "Did you mean AZ_ErrorOnce("#window", false, \"%s\", "#expression"); ?",                                        \
            "Did you mean AZ_ErrorOnce("#window", false, "#expression", "#__VA_ARGS__"); ?");                               \
        static bool isDisplayed = false;                                                                                    \
        if (!isDisplayed)                                                                                                   \
        {                                                                                                                   \
            isDisplayed = true;                                                                                             \
            AZ::Debug::Trace::Instance().Error(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, window, /*format,*/ __VA_ARGS__); \
        }                                                                                                                   \
    }                                                                                                                       \
    AZ_POP_DISABLE_WARNING

    #define AZ_Warning(window, expression, ...)                                                                             \
    AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                               \
    if (!(expression))                                                                                                      \
    {                                                                                                                       \
        AZ_TraceFmtCompileTimeCheck(expression, AZ_VA_HAS_ARGS(__VA_ARGS__),                                                \
            "String used in place of boolean expression for AZ_Warning.",                                                   \
            "Did you mean AZ_Warning("#window", false, \"%s\", "#expression"); ?",                                          \
            "Did you mean AZ_Warning("#window", false, "#expression", "#__VA_ARGS__"); ?");                                 \
        AZ::Debug::Trace::Instance().Warning(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, window, /*format,*/ __VA_ARGS__);   \
    }                                                                                                                       \
    AZ_POP_DISABLE_WARNING


    #define AZ_WarningOnce(window, expression, ...)                                                                             \
    AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                                   \
    if (!(expression))                                                                                                          \
    {                                                                                                                           \
        AZ_TraceFmtCompileTimeCheck(expression, AZ_VA_HAS_ARGS(__VA_ARGS__),                                                    \
            "String used in place of boolean expression for AZ_WarningOnce.",                                                   \
            "Did you mean AZ_WarningOnce("#window", false, \"%s\", "#expression"); ?",                                          \
            "Did you mean AZ_WarningOnce("#window", false, "#expression", "#__VA_ARGS__"); ?");                                 \
        static bool isDisplayed = false;                                                                                        \
        if (!isDisplayed)                                                                                                       \
        {                                                                                                                       \
            AZ::Debug::Trace::Instance().Warning(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, window, /*format,*/ __VA_ARGS__);   \
            isDisplayed = true;                                                                                                 \
        }                                                                                                                       \
    }                                                                                                                           \
    AZ_POP_DISABLE_WARNING

    #define AZ_TracePrintf(window, ...)                      AZ::Debug::Trace::Instance().Printf(window, __VA_ARGS__);

/*!
 * Verify version of the trace checks evaluates the expression even in release.
 *
 * i.e.
 *   // with assert
 *   buffer = azmalloc(size,alignment);
 *   AZ_Assert( buffer != nullptr,"Assert Message");
 *   ...
 *
 *   // with verify
 *   AZ_Verify((buffer = azmalloc(size,alignment)) != nullptr,"Assert Message")
 *   ...
 */
    #define AZ_Verify(expression, ...) AZ_Assert(0 != (expression), __VA_ARGS__)
    #define AZ_VerifyError(window, expression, ...) AZ_Error(window, 0 != (expression), __VA_ARGS__)
    #define AZ_VerifyWarning(window, expression, ...) AZ_Warning(window, 0 != (expression), __VA_ARGS__)

#else // !AZ_ENABLE_TRACING
    #define AZ_Assert(expression, ...)
    #define AZ_Error(window, expression, ...)
    #define AZ_ErrorOnce(window, expression, ...)
    #define AZ_Warning(window, expression, ...)
    #define AZ_WarningOnce(window, expression, ...)
    #define AZ_TracePrintf(window, ...)

    #define AZ_Verify(expression, ...) (void)(expression)
    #define AZ_VerifyError(window, expression, ...) (void)(expression)
    #define AZ_VerifyWarning(window, expression, ...) (void)(expression)

#endif  // AZ_ENABLE_TRACING

#define AZ_Printf(window, ...)       AZ::Debug::Trace::Instance().Printf(window, __VA_ARGS__);

#if defined(AZ_DEBUG_BUILD)
#   define AZ_DbgIf(expression) \
        AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") \
        if (expression) \
        AZ_POP_DISABLE_WARNING
#   define AZ_DbgElseIf(expression) \
        AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") \
        else if (expression) \
        AZ_POP_DISABLE_WARNING
#else
#   define AZ_DbgIf(expression)         if (false)
#   define AZ_DbgElseIf(expression)     else if (false)
#endif
