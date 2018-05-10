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

// include the Core headers
#include "StandardHeaders.h"
#include "DiskFile.h"
#include "MemoryManager.h"
#include "Array.h"

namespace MCore
{
    /**
     * A standard text file as we normally think of. In other words, a file stored on the harddisk or a CD or any other comparable medium.
     * This class is different from DiskFile because it has special functions for handling text based files, for example to read line by line.
     */
    class MCORE_API DiskTextFile
        : public DiskFile
    {
        MCORE_MEMORYOBJECTCATEGORY(DiskTextFile, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_DISKFILE)

    public:
        // the type returned by GetType()
        enum
        {
            TYPE_ID = 0x0000003
        };

        /**
         * The constructor.
         */
        DiskTextFile();

        /**
         * The destructor. Automatically closes the file.
         */
        virtual ~DiskTextFile();

        /**
         * Get the unique type ID.
         * @result The type identification number.
         */
        uint32 GetType() const override;

        /**
         * Try to open the file, given a filename and open mode.
         * The file is always opened in text mode. If you want to load binary files, use MCore::DiskFile.
         * @param fileName The filename on disk.
         * @param mode The file open mode. See enum EMode for more information. Do NOT use a combination of these modes.
         * @result Returns true when successfully opened the file, otherwise false is returned.
         */
        bool Open(const char* fileName, EMode mode) override;

        /**
         * Read a line from the text file.
         * This will include the 'enter' at the end of the string if there is any.
         * It will read a line until the next 'enter' character, or until the end of the file has been reached.
         * @param outResultString This will contain the read string, which represents the line. All its contents are overwritten.
         * @param maxLineLength The maximum length in characters, of a single line. This makes sure the read buffer inside this class is big enough to read this amount of data.
         * @result Returns true when the line has been successfully read, or false when the end of the file has been reached and there are no lines left.
         */
        bool ReadLine(AZStd::string& outResultString, uint32 maxLineLength = 4096);

        /**
         * Read the entire text file from its current position into a given string.
         * This concats all lines of the file together into the provided output string.
         * @param outResultString The string you want the result to be stored in. All existing string contents are overwritten.
         * @param maxLineLength The maximum length in characters, of a single line. This makes sure the read buffer inside this class is big enough to read this amount of data.
         * @result Returns true when there is no error, or false when an error occurred.
         */
        bool ReadAllLinesAsString(AZStd::string& outResultString, uint32 maxLineLength = 4096);

        /**
         * Reads the entire text file into an array of strings.
         * This will read all lines of the text file and put them inside the output array.
         * The contents of the array are cleared before starting this method.
         * @param outResultArray The array to put all the lines of text in.
         * @param maxLineLength The maximum length in characters, of a single line. This makes sure the read buffer inside this class is big enough to read this amount of data.
         * @result Returns true when there is no error, or false when an error occurred.
         */
        bool ReadAllLinesAsStringArray(MCore::Array<AZStd::string>& outResultArray, uint32 maxLineLength = 4096);

        /**
         * Write a string to the text file.
         * @param stringToWrite The string to write. Please keep in mind that it does NOT automatically add an 'enter' behind the string.
         * @result Returns true when succeeded or false in case of an error.
         */
        bool WriteString(const AZStd::string& stringToWrite);

        /**
         * Write a string to the text file.
         * @param stringToWrite The string to write. Please keep in mind that it does NOT automatically add an 'enter' behind the string.
         * @result Returns true when succeeded or false in case of an error.
         */
        bool WriteString(const char* stringToWrite);

        /**
         * Release the memory allocated by temp buffer that was used while reading lines from the file.
         */
        void ClearBuffer();

    private:
        char*   mBuffer;        /**< A temp string, which acts as read buffer. */
        size_t  mBufferSize;    /**< The buffer size, in number of characters. */

        /**
         * Allocate space for a given number of characters inside the read temp buffer.
         * This will only allocate data when it requests more data than there is already allocated.
         * In case smaller amounts of memory are allocated, nothing is done.
         * @param maxSize The maximum number of characters to allocate space for.
         */
        void Allocate(size_t maxSize);
    };
} // namespace MCore
