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
#include <cstdarg>
#include <stdio.h>
#include <AzCore/IO/FileIO.h>
#include "FileOperations.h"

namespace AZ
{
    namespace IO
    {
        int64_t Print(HandleType fileHandle, const char* format, ...)
        {
            va_list arglist;
            va_start(arglist, format);
            int64_t result = PrintV(fileHandle, format, arglist);
            va_end(arglist);
            return result;
        }

        int64_t PrintV(HandleType fileHandle, const char* format, va_list arglist)
        {
            const int bufferSize = 1024;
            char buffer[bufferSize];

            FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::PrintV: No FileIO instance");

            int printCount = azvsnprintf(buffer, bufferSize, format, arglist);

            if (printCount > 0)
            {
                AZ_Assert(printCount < bufferSize, "AZ::IO::PrintV: required buffer size for vsnprintf is larger than working buffer");
                if (!fileIO->Write(fileHandle, buffer, printCount))
                {
                    return -1;
                }
                return printCount;
            }

            return printCount;
        }

        Result Move(const char* sourceFilePath, const char* destinationFilePath)
        {
            FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::Move: No FileIO instance");

            if (!fileIO->Copy(sourceFilePath, destinationFilePath))
            {
                return ResultCode::Error;
            }

            if (!fileIO->Remove(sourceFilePath))
            {
                return ResultCode::Error;
            }

            return ResultCode::Success;
        }



        int GetC(HandleType fileHandle)
        {
            FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::GetC: No FileIO instance");
            char character;
            if (!fileIO->Read(fileHandle, &character, 1, true))
            {
                return EOF;
            }
            return static_cast<int>(character);
        }

        int UnGetC(int character, HandleType fileHandle)
        {
            FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::UnGetC: No FileIO instance");

            // EOF does nothing
            if (character == EOF)
            {
                return EOF;
            }
            char characterToRestore = static_cast<char>(character);
            // Just move back a byte and write it in
            // Still not identical behavior since ungetc normally gets lost if you seek, but it works for our case
            if (!fileIO->Seek(fileHandle, -1, SeekType::SeekFromCurrent))
            {
                return EOF;
            }

            if (!fileIO->Write(fileHandle, &characterToRestore, sizeof(characterToRestore)))
            {
                return EOF;
            }
            return character;
        }

        char* FGetS(char* buffer, uint64_t bufferSize, HandleType fileHandle)
        {
            AZ_Assert(buffer && bufferSize, "AZ::IO::FGetS: Invalid buffer supplied");

            FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::FGetS: No FileIO instance");

            memset(buffer, 0, bufferSize);

            AZ::u64 bytesRead = 0;
            fileIO->Read(fileHandle, buffer, bufferSize - 1, false, &bytesRead);
            if (!bytesRead)
            {
                return 0;
            }

            char* currentPosition = buffer;
            int64_t len = 0;

            bool done = false;
            int64_t slashRPosition = -1;
            do
            {
                if (*currentPosition != '\r' && *currentPosition != '\n' && static_cast<uint64_t>(len) < bufferSize - 1)
                {
                    len++;
                    currentPosition++;
                }
                else
                {
                    done = true;
                    if (*currentPosition == '\r')
                    {
                        slashRPosition = len;
                    }
                }
            } while (!done);

            // null terminate string
            buffer[len] = '\0';

            //////////////////////////////////////////
            //seek back to the end of the string
            AZ::s64 seekback = bytesRead - len - 1;

            // handle CR/LF for file coming from different platforms
            if (slashRPosition > -1 && bytesRead > static_cast<uint64_t>(slashRPosition) && buffer[slashRPosition + 1] == '\n')
            {
                seekback--;
            }
            fileIO->Seek(fileHandle, -seekback, AZ::IO::SeekType::SeekFromCurrent);
            ///////////////////////////////////////////

            return buffer;
        }

        int FPutS(const char* buffer, HandleType fileHandle)
        {
            FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::FPutS: No FileIO instance");

            uint64_t writeAmount = strlen(buffer);
            uint64_t bytesWritten = 0;
            if (!fileIO->Write(fileHandle, buffer, writeAmount))
            {
                return EOF;
            }
            return static_cast<int>(bytesWritten);
        }
    } // namespace IO
} // namespace AZ