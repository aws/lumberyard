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

// include the required headers
#include "DiskTextFile.h"
#include "LogManager.h"
#include "StringConversions.h"

namespace MCore
{
    // constructor
    DiskTextFile::DiskTextFile()
        : DiskFile()
    {
        mBuffer     = nullptr;
        mBufferSize = 0;
    }


    // destructor
    DiskTextFile::~DiskTextFile()
    {
        ClearBuffer();
    }


    // clear the temp buffer
    void DiskTextFile::ClearBuffer()
    {
        MCore::Free(mBuffer);
        mBuffer         = nullptr;
        mBufferSize     = 0;
    }


    // allocate space for the temp buffer
    void DiskTextFile::Allocate(size_t maxSize)
    {
        // allocate when we haven't allocated enough yet
        if (mBufferSize < maxSize)
        {
            if (mBuffer)
            {
                mBuffer = (char*)MCore::Realloc(mBuffer, maxSize, MCORE_MEMCATEGORY_DISKFILE);
            }
            else
            {
                mBuffer = (char*)MCore::Allocate(maxSize, MCORE_MEMCATEGORY_DISKFILE);
            }

            mBufferSize = maxSize;
        }
    }


    // return the unique type identification number
    uint32 DiskTextFile::GetType() const
    {
        return DiskTextFile::TYPE_ID;
    }


    // try to open the file
    bool DiskTextFile::Open(const char* fileName, EMode mode)
    {
        // if the file already is open, close it first
        if (mFile)
        {
            Close();
        }

        //String fileMode;
        char fileMode[16];
        uint32 numChars = 0;

        if (mode == READ)
        {
            fileMode[0] = 'r';
            numChars = 1;
        }                                                                                   // open for reading, file must exist
        if (mode == WRITE)
        {
            fileMode[0] = 'w';
            numChars = 1;
        }                                                                                   // open for writing, file will be overwritten if it already exists
        if (mode == READWRITE)
        {
            fileMode[0] = 'r';
            fileMode[1] = '+';
            numChars = 2;
        }                                                                                   // open for reading and writing, file must exist
        if (mode == READWRITECREATE)
        {
            fileMode[0] = 'w';
            fileMode[1] = '+';
            numChars = 2;
        }                                                                                   // open for reading and writing, file will be overwritten when already exists, or created when it doesn't
        if (mode == APPEND)
        {
            fileMode[0] = 'a';
            numChars = 1;
        }                                                                                   // open for writing at the end of the file, file will be created when it doesn't exist
        if (mode == READWRITEAPPEND)
        {
            fileMode[0] = 'a';
            fileMode[1] = '+';
            numChars = 2;
        }                                                                                   // open for reading and appending (writing), file will be created if it doesn't exist

        // construct the filemode string
        fileMode[numChars++] = 't'; // open in text mode
        fileMode[numChars++] = '\0';
        //strcat(fileMode, ",ccs=UTF-8");

        // try to open the file
        // set the file mode we used
        mFileMode = mode;

        // set the filename
        mFileName = fileName;

        mFile = nullptr;
        azfopen(&mFile, fileName, fileMode);

        // check on success
        return (mFile != nullptr);
    }


    // read the a line into the string
    bool DiskTextFile::ReadLine(AZStd::string& outResultString, uint32 maxLineLength)
    {
        // reserve space and clear the buffer
        Allocate(maxLineLength);

        // clear the result string
        outResultString.clear();

        // read the line into the buffer
        if (fgets(mBuffer, maxLineLength, mFile) == nullptr)
        {
            return false;
        }

        outResultString = mBuffer;
        AzFramework::StringFunc::TrimWhiteSpace(outResultString, false, true);
        outResultString += MCore::CharacterConstants::endLine;
        return true;
    }


    // read all the lines
    bool DiskTextFile::ReadAllLinesAsString(AZStd::string& outResultString, uint32 maxLineLength)
    {
        // reserve space for the line buffer
        AZStd::string lineBuffer;
        lineBuffer.reserve(maxLineLength);

        // read all the lines and attach them
        while (ReadLine(lineBuffer, maxLineLength))
        {
            outResultString += lineBuffer;
        }

        return true;
    }


    bool DiskTextFile::ReadAllLinesAsStringArray(MCore::Array<AZStd::string>& outResultArray, uint32 maxLineLength)
    {
        // clear the result array
        outResultArray.Clear();

        // reserve space for the line buffer
        AZStd::string lineBuffer;
        lineBuffer.reserve(maxLineLength);

        // read all the lines and attach them
        while (ReadLine(lineBuffer, maxLineLength))
        {
            outResultArray.Add(lineBuffer);
        }

        return true;
    }


    // write a string
    bool DiskTextFile::WriteString(const AZStd::string& stringToWrite)
    {
        MCORE_ASSERT(stringToWrite.c_str());
        return (fputs(stringToWrite.c_str(), mFile) >= 0);
    }


    // write a string from a const char pointer
    bool DiskTextFile::WriteString(const char* stringToWrite)
    {
        MCORE_ASSERT(stringToWrite);
        return (fputs(stringToWrite, mFile) >= 0);
    }
} // namespace MCore
