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
#include "precompiled.h"
#include "Output.h"
#include "Writer.h"
#include "JSONWriter.h"
#include "Configuration.h"
#ifdef AZCG_PLATFORM_WINDOWS
#include <io.h>
// Underscore ISO C++ versions are only supported on windows right now
#define dup _dup
#define dup2 _dup2
#define close _close
#else
#include <unistd.h>
#endif

namespace CodeGenerator
{
    namespace Configuration
    {
        enum OutputRedirection
        {
            NoRedirect,
            NullRedirect,
            FileRedirect
        };

        using namespace llvm;

        static cl::OptionCategory OutputSettingsCategory("Output Settings");
        static cl::opt<bool> s_outputUsingJSON("output-using-json", cl::desc("Output using json objects instead of plain text, easier for calling applications to parse"), cl::cat(OutputSettingsCategory), cl::Optional);
        static cl::opt<OutputRedirection> s_outputRedirection("output-redirection", cl::desc("Redirect output and error messages from internal utilities (e.g. clang and python)"),
            cl::values(
                clEnumValN(NoRedirect, "none", "No output redirection, clang and python will output to stdout and stderr"),
                clEnumValN(NullRedirect, "null", "Redirect clang and python to null, effectively suppressing that output"),
                clEnumValN(FileRedirect, "file", "Redirect clang and python to disk, specify path with redirect-output-file"),
                clEnumValEnd
                ),
            cl::cat(OutputSettingsCategory),
            cl::Optional,
            cl::init(NullRedirect)
            );
        static cl::opt<std::string> s_redirectOutputFile("redirect-output-file", cl::desc("File path to write redirect output, used in combination with -output-redirection=file (defaults to output.log)"), cl::cat(OutputSettingsCategory), cl::Optional, cl::init("output.log"));
    }

    OutputWriter::OutputWriter()
        : m_outputWriter("")
    {
    }

    void OutputWriter::StartOutput()
    {
        if (Configuration::s_outputRedirection != Configuration::NoRedirect)
        {
            // Save me a copy of these fine handles right here
            m_stdout = dup(1);
            m_stderr = dup(2);

            // Figure out where to redirect to
            std::string redirectFile;
            if (Configuration::s_outputRedirection == Configuration::NullRedirect)
            {
#if defined(AZCG_PLATFORM_WINDOWS)
                redirectFile = "nul";
#elif defined(AZCG_PLATFORM_DARWIN) || defined(AZCG_PLATFORM_LINUX)
                redirectFile = "/dev/null";
#endif
            }
            else if (Configuration::s_outputRedirection == Configuration::FileRedirect)
            {
                redirectFile = Configuration::s_redirectOutputFile;
            }

            // Redirect stdout
            freopen(redirectFile.c_str(), "w", stdout);
            // Duplicate it over to stderr
            dup2(1, 2);
        }

        if (Configuration::s_outputUsingJSON)
        {
            m_outputWriter.BeginArray();
        }
    }

    void OutputWriter::EndOutput()
    {
        if (Configuration::s_outputUsingJSON)
        {
            m_outputWriter.EndArray();

            if (Configuration::s_outputRedirection != Configuration::NoRedirect)
            {
                // Reset stdout and stderr to default
                if (m_stdout != -1)
                {
                    dup2(m_stdout, 1);
                    close(m_stdout);
                }
                if (m_stderr != -1)
                {
                    dup2(m_stderr, 2);
                    close(m_stderr);
                }
            }
            // Print our stored json returns
            printf("%s\n", m_outputWriter.GetStringBuffer().GetString());
        }
    }

    void OutputWriter::Print(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        Print(format, args);
        va_end(args);
    }

    void OutputWriter::Print(const char* format, va_list args)
    {
        // Only print out if verbose output is on
        if (!GlobalConfiguration::g_enableVerboseOutput)
        {
            return;
        }

        // va_list can only be used once on some platforms, must make a copy since we will iterate on it twice here
        va_list targs;
        va_copy(targs,args);
        // Manually handle the null terminator because some version of VC++ do not actually print the null terminator when the buffer size is >= input size
        size_t bufferSize = vsnprintf(nullptr, 0, format, targs) + 1;
        va_end(targs);
        std::vector<char> buffer;
        buffer.resize(bufferSize);
        vsnprintf(buffer.data(), bufferSize, format, args);
        // Manual null terminator
        buffer[bufferSize - 1] = 0;

        if (Configuration::s_outputUsingJSON)
        {
            // {
            //      "type" : "info",
            //      "info" : "<formatted string>"
            // }
            m_outputWriter.Begin();

            m_outputWriter.WriteString("type");
            m_outputWriter.WriteString("info");

            m_outputWriter.WriteString("info");
            m_outputWriter.WriteString(buffer.data());

            m_outputWriter.End();
        }
        else
        {
            printf("Info: %s\n", buffer.data());
        }
    }

    void OutputWriter::Error(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        Error(format, args);
        va_end(args);
    }

    void OutputWriter::Error(const char* format, va_list args)
    {
        // Manually handle the null terminator because some version of VC++ do not actually print the null terminator when the buffer size is >= input size
        size_t bufferSize = vsnprintf(nullptr, 0, format, args) + 1;
        std::vector<char> buffer;
        buffer.resize(bufferSize);
        vsnprintf(buffer.data(), bufferSize, format, args);
        // Manual null terminator
        buffer[bufferSize - 1] = 0;

        if (Configuration::s_outputUsingJSON)
        {
            // {
            //      "type" : "error",
            //      "error" : "<formatted string>"
            // }
            m_outputWriter.Begin();

            m_outputWriter.WriteString("type");
            m_outputWriter.WriteString("error");

            m_outputWriter.WriteString("error");
            m_outputWriter.WriteString(buffer.data());

            m_outputWriter.End();
        }
        else
        {
            printf("Error: %s\n", buffer.data());
        }
    }

    void OutputWriter::GeneratedFile(const std::string& fileName, bool shouldBeAddedToBuild /*= false*/)
    {
        if (Configuration::s_outputUsingJSON)
        {
            // {
            //      "type" : "generated_file",
            //      "file_name" : "<fileName>",
            //      should_be_added_to_build : <shouldBeAddedToBuild>
            // }
            m_outputWriter.Begin();

            m_outputWriter.WriteString("type");
            m_outputWriter.WriteString("generated_file");

            m_outputWriter.WriteString("file_name");
            m_outputWriter.WriteString(fileName);

            m_outputWriter.WriteString("should_be_added_to_build");
            m_outputWriter.WriteBool(shouldBeAddedToBuild);

            m_outputWriter.End();
        }
        else
        {
            printf("Generated File: %s (%s be added to the build)", fileName.c_str(), shouldBeAddedToBuild ? "should" : "should not");
        }
    }

    void OutputWriter::DependencyFile(const std::string& fileName)
    {
        if (Configuration::s_outputUsingJSON)
        {
            // {
            //      "type" : "dependency_file",
            //      "file_name" : "<fileName>"
            // }
            m_outputWriter.Begin();

            m_outputWriter.WriteString("type");
            m_outputWriter.WriteString("dependency_file");

            m_outputWriter.WriteString("file_name");
            m_outputWriter.WriteString(fileName);

            m_outputWriter.End();
        }
    }

    namespace Output
    {

        // Todo: Refactor this to have less global-variable.
        // https://issues.labcollab.net/browse/LMBR-23575
        static OutputWriter g_outputWriter;

        void StartCapturing()
        {
            g_outputWriter.StartOutput();
        }

        void FinishCapturingAndPrint()
        {
            g_outputWriter.EndOutput();
        }

        void Print(const char* format, ...)
        {
            va_list args;
            va_start(args, format);
            g_outputWriter.Print(format, args);
            va_end(args);
        }
        void Error(const char* format, ...)
        {
            va_list args;
            va_start(args, format);
            g_outputWriter.Error(format, args);
            va_end(args);
        }
        void GeneratedFile(const std::string& fileName, bool shouldBeAddedToBuild)
        {
            g_outputWriter.GeneratedFile(fileName, shouldBeAddedToBuild);
        }

        void DependencyFile(const std::string& fileName)
        {
            g_outputWriter.DependencyFile(fileName);
        }

        ScopedCapture::ScopedCapture()
        {
            StartCapturing();
        }

        ScopedCapture::~ScopedCapture()
        {
            FinishCapturingAndPrint();
        }
    }
}
