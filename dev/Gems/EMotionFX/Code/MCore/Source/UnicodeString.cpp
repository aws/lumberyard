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

// include required headers
#include "Macros.h"
#include "UnicodeString.h"
#include "LogManager.h"
#include "Array.h"
#include "Algorithms.h"
#include "MemoryManager.h"
#include "LogManager.h"
#include "FileSystem.h"
#include "Matrix4.h"
#include "UnicodeStringIterator.h"

namespace MCore
{
    // construct this string from two strings
    UnicodeString::UnicodeString(const char* sA, uint32 lengthA, const char* sB, uint32 lengthB)
    {
        MCORE_ASSERT(sA);
        MCORE_ASSERT(sB);

        mData       = nullptr;
        mMaxLength  = 0;
        mLength     = 0;

        Alloc(lengthA + lengthB, 0);
        MCore::MemCopy(mData, sA, lengthA * sizeof(char));
        MCore::MemCopy(mData + lengthA, sB, lengthB * sizeof(char));
    }


    // destructor
    UnicodeString::~UnicodeString()
    {
        if (mData)
        {
            MCore::Free(mData);
        }
    }


    // format a string, returns itself
    UnicodeString& UnicodeString::Format(const char* text, ...)
    {
        if (mMaxLength < 1024)
        {
            Alloc(1024, 0);
        }

        va_list args;
        va_start(args, text);
    #if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC)
        const int32 len = vsprintf_s(mData, mMaxLength, text, args);
    #else
        const int32 len = vsprintf(mData, text, args);
    #endif
        //  MCORE_ASSERT(len >= 0);
        va_end(args);

        Alloc((uint32)len, 0);
        return *this;
    }


    // add a formatted string to the current string
    UnicodeString& UnicodeString::FormatAdd(const char* text, ...)
    {
        const uint32 oldLength = mLength;

        if (mMaxLength < 1024)
        {
            Alloc(1024, 0);
        }

        va_list args;
        va_start(args, text);
    #if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC)
        const int32 len = vsprintf_s(mData + oldLength, mMaxLength - oldLength, text, args);
    #else
        const int32 len = vsprintf(mData + oldLength, text, args);
    #endif
        //  MCORE_ASSERT(len >= 0);
        va_end(args);

        Alloc(oldLength + len, 0);
        return *this;
    }




    // copy some data
    UnicodeString& UnicodeString::Copy(const char* what, uint32 length)
    {
        if (what == nullptr)
        {
            if (mData)
            {
                MCore::Free(mData);
                mData       = nullptr;
                mLength     = 0;
                mMaxLength  = 0;
            }
            return *this;
        }

        // remove existing data
        if (mData)
        {
            //MCore::Free(mData);
            mLength     = 0;
            mData[0]    = 0;
        }

        //mData = nullptr;

        // alloc new
        Alloc(length, 0);

        // copy new data
        if (mData)
        {
            MCore::MemCopy(mData, what, length * sizeof(char));
        }

        return *this;
    }


    // concatenate two strings
    UnicodeString& UnicodeString::Concat(const char* what, uint32 length)
    {
        if (what == nullptr || length == 0)
        {
            return *this;
        }

        const uint32 oldLength = mLength;

        // realloc new
        if (mMaxLength <= length + mLength)
        {
            Alloc(mLength + length, 0);
        }
        else
        {
            mLength += length;
        }

        MCore::MemCopy(mData + oldLength, what, length * sizeof(char));
        mData[mLength] = 0;

        return *this;
    }


    void UnicodeString::Alloc(uint32 numCodeUnits, uint32 extraUnits)
    {
        // if we have a zero length, do nothing if we don't have to
        if (numCodeUnits + extraUnits == 0)
        {
            if (mData)
            {
                mData[0] = 0;
            }

            mLength = 0;
            return;
        }

        if ((numCodeUnits + extraUnits > mMaxLength) || mData == nullptr)
        {
            mLength     = numCodeUnits;
            mMaxLength  = mLength + extraUnits;

            if (mData)
            {
                mData   = (char*)MCore::Realloc(mData, (mMaxLength + 1) * sizeof(char), MCORE_MEMCATEGORY_STRING, UnicodeString::MEMORYBLOCK_ID, MCORE_FILE, MCORE_LINE);
            }
            else
            {
                mData   = (char*)MCore::Allocate((mMaxLength + 1) * sizeof(char), MCORE_MEMCATEGORY_STRING, UnicodeString::MEMORYBLOCK_ID, MCORE_FILE, MCORE_LINE);
            }

            if (mData == nullptr)
            {
                return;
            }

            mData[numCodeUnits] = 0;
        }
        else
        {
            mLength = numCodeUnits;
            mData[numCodeUnits] = 0;
        }
    }



    // convert the string to a boolean. string may be "1", "0", "true", "false", "yes", "no") (non case sensitive)
    bool UnicodeString::ToBool() const
    {
        if (mData == nullptr || mLength == 0)
        {
            return false;
        }

        // check if it's true, else return false
        return (CheckIfIsEqualNoCase("1") || CheckIfIsEqualNoCase("true") || CheckIfIsEqualNoCase("yes"));
    }


    // convert the string to an int32
    int32 UnicodeString::ToInt() const
    {
        if (mData == nullptr || mLength == 0)
        {
            return 0;
        }

        return atoi(mData);
    }


    // convert the string to a float
    float UnicodeString::ToFloat() const
    {
        if (mData == nullptr || mLength == 0)
        {
            return 0.0f;
        }

        return static_cast<float>(atof(mData));
    }



    // convert the string into a Vector2
    AZ::Vector2 UnicodeString::ToVector2() const
    {
        // split the string into different floats
        Array<UnicodeString> parts = Split(UnicodeCharacter::comma);
        if (parts.GetLength() != 2)
        {
            return AZ::Vector2(0.0f, 0.0f);
        }

        // remove spaces
        for (uint32 i = 0; i < 2; ++i)
        {
            parts[i].Trim();
        }

        return AZ::Vector2(parts[0].ToFloat(), parts[1].ToFloat());
    }


    // convert the string into a Vector3
    AZ::PackedVector3f UnicodeString::ToVector3() const
    {
        // split the string into different floats
        Array<UnicodeString> parts = Split(UnicodeCharacter::comma);
        if (parts.GetLength() != 3)
        {
            return AZ::PackedVector3f(0.0f, 0.0f, 0.0f);
        }

        // remove spaces
        for (uint32 i = 0; i < 3; ++i)
        {
            parts[i].Trim();
        }

        return AZ::PackedVector3f(parts[0].ToFloat(), parts[1].ToFloat(), parts[2].ToFloat());
    }


    // convert the string into a Vector4
    AZ::Vector4 UnicodeString::ToVector4() const
    {
        // split the string into different floats
        Array<UnicodeString> parts = Split(UnicodeCharacter::comma);
        if (parts.GetLength() != 4)
        {
            return AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
        }

        // remove spaces
        for (uint32 i = 0; i < 4; ++i)
        {
            parts[i].Trim();
        }

        return AZ::Vector4(parts[0].ToFloat(), parts[1].ToFloat(), parts[2].ToFloat(), parts[3].ToFloat());
    }


    // convert the string into a Matrix
    Matrix UnicodeString::ToMatrix() const
    {
        // split the string into different floats
        Array<UnicodeString> parts = Split(UnicodeCharacter::comma);
        if (parts.GetLength() != 16)
        {
            Matrix result;
            result.Identity();
            return result;
        }

        // remove spaces
        for (uint32 i = 0; i < 16; ++i)
        {
            parts[i].Trim();
        }

        // convert the elements
        float data[16];
        for (uint32 i = 0; i < 16; ++i)
        {
            data[i] = parts[i].ToFloat();
        }

        return Matrix(data);
    }


    bool UnicodeString::CheckIfIsValidFloat() const
    {
        if (mData == nullptr || mLength == 0)
        {
            return false;
        }

        char validChars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', 'e', '+', '-' };

        // check if all characters are valid
        for (uint32 i = 0; i < mLength; ++i)
        {
            char c = mData[i];

            // if the + or - is not the first character
            if (c == '+' || c == '-')
            {
                if (i != 0)
                {
                    return false;
                }
            }

            // check if the characters are correct values
            bool found = false;
            for (uint32 a = 0; a < 14 && !found; ++a)
            {
                if (c == validChars[a])
                {
                    found = true;
                }
            }

            // if we found a non-valid character, we cannot convert this to a float
            if (!found)
            {
                return false;
            }
        }

        return true;
    }


    // check if this is a valid int
    bool UnicodeString::CheckIfIsValidInt() const
    {
        if (mData == nullptr || mLength == 0)
        {
            return false;
        }

        char validChars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '-' };

        // check if all characters are valid
        for (uint32 i = 0; i < mLength; ++i)
        {
            char c = mData[i];

            // if the + or - is not the first character
            if (c == '+' || c == '-')
            {
                if (i != 0)
                {
                    return false;
                }
            }

            // check if the characters are correct values
            bool found = false;
            for (uint32 a = 0; a < 12 && !found; ++a)
            {
                if (c == validChars[a])
                {
                    found = true;
                }
            }

            // if we found a non-valid character, we cannot convert this to an int32
            if (!found)
            {
                return false;
            }
        }

        return true;
    }



    // check if this is a valid Vector2
    bool UnicodeString::CheckIfIsValidVector2() const
    {
        if (mData == nullptr || mLength == 0)
        {
            return false;
        }

        // split the string into different floats
        Array<UnicodeString> parts = Split(UnicodeCharacter::comma);
        if (parts.GetLength() != 2)
        {
            return false;
        }

        // remove spaces
        for (uint32 i = 0; i < 2; ++i)
        {
            parts[i].Trim();
        }

        return (parts[0].CheckIfIsValidFloat() && parts[1].CheckIfIsValidFloat());
    }


    // check if this is a valid Vector3
    bool UnicodeString::CheckIfIsValidVector3() const
    {
        if (mData == nullptr || mLength == 0)
        {
            return false;
        }

        // split the string into different floats
        Array<UnicodeString> parts = Split(UnicodeCharacter::comma);
        if (parts.GetLength() != 3)
        {
            return false;
        }

        // remove spaces
        for (uint32 i = 0; i < 3; ++i)
        {
            parts[i].Trim();
        }

        return (parts[0].CheckIfIsValidFloat() && parts[1].CheckIfIsValidFloat() && parts[2].CheckIfIsValidFloat());
    }


    // check if this is a valid Vector4
    bool UnicodeString::CheckIfIsValidVector4() const
    {
        if (mData == nullptr || mLength == 0)
        {
            return false;
        }

        // split the string into different floats
        Array<UnicodeString> parts = Split(UnicodeCharacter::comma);
        if (parts.GetLength() != 4)
        {
            return false;
        }

        // remove spaces
        for (uint32 i = 0; i < 4; ++i)
        {
            parts[i].Trim();
        }

        return (parts[0].CheckIfIsValidFloat() && parts[1].CheckIfIsValidFloat() && parts[2].CheckIfIsValidFloat() && parts[3].CheckIfIsValidFloat());
    }


    // check if this is a valid Matrix
    bool UnicodeString::CheckIfIsValidMatrix() const
    {
        if (mData == nullptr || mLength == 0)
        {
            return false;
        }

        // split the string into different floats
        Array<UnicodeString> parts = Split(UnicodeCharacter::comma);
        if (parts.GetLength() != 16)
        {
            return false;
        }

        // remove spaces
        for (uint32 i = 0; i < 16; ++i)
        {
            parts[i].Trim();
        }

        // verify the elements, to see if they are valid floats
        for (uint32 i = 0; i < 16; ++i)
        {
            if (parts[i].CheckIfIsValidFloat() == false)
            {
                return false;
            }
        }

        return true;
    }



    // check if this is a valid bool
    bool UnicodeString::CheckIfIsValidBool() const
    {
        if (mData == nullptr || mLength == 0)
        {
            return false;
        }

        if (CheckIfIsEqualNoCase("1") || CheckIfIsEqualNoCase("0") || CheckIfIsEqualNoCase("false") || CheckIfIsEqualNoCase("true") || CheckIfIsEqualNoCase("yes") || CheckIfIsEqualNoCase("no"))
        {
            return true;
        }

        return false;
    }


    // restricted string copy
    void UnicodeString::RestrictedCopy(char* to, const char* from, uint32 maxLength)
    {
        // check length, if it is smaller than our space just copy it over
        if (strlen(from) <= maxLength)
        {
            azstrcpy(to, maxLength, from);
        }
        else // string is too long, only copy it till the maximum length
        {
            for (uint32 i = 0; i < maxLength - 1; i++)
            {
                to[i] = from[i];
            }

            to[maxLength - 1] = 0;
        }
    }


    // init from a boolean
    void UnicodeString::FromBool(bool value)
    {
        if (value)
        {
            Copy("true", 4);
        }
        else
        {
            Copy("false", 5);
        }
    }


    // init from an integer
    void UnicodeString::FromInt(int32 value)
    {
        Alloc(32);
    #if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC)
        sprintf_s(mData, mMaxLength, "%d", value);
    #else
        sprintf(mData, "%d", value);
    #endif
        Alloc((uint32)strlen(mData));
    }


    // init from a float
    void UnicodeString::FromFloat(float value)
    {
        Alloc(32);
    #if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC)
        sprintf_s(mData, mMaxLength, "%.8f", value);
    #else
        sprintf(mData, "%.8f", value);
    #endif
        Alloc((uint32)strlen(mData));
    }


    // init from a Vector2
    void UnicodeString::FromVector2(const AZ::Vector2& value)
    {
        Format("%.8f,%.8f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()));
    }


    // init from a Vector3
    void UnicodeString::FromVector3(const AZ::Vector3& value)
    {
        Format("%.8f,%.8f,%.8f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()));
    }


    // init from a Vector4
    void UnicodeString::FromVector4(const AZ::Vector4& value)
    {
        // Needs explicit conversion from VectorFloat to float.
        Format("%.8f,%.8f,%.8f,%.8f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()), static_cast<float>(value.GetW()));
    }


    // init from a Matrix
    void UnicodeString::FromMatrix(const Matrix& value)
    {
        Format("%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f",   value.m16[0], value.m16[1], value.m16[2], value.m16[3],
            value.m16[4], value.m16[5], value.m16[6], value.m16[7],
            value.m16[8], value.m16[9], value.m16[10], value.m16[11],
            value.m16[12], value.m16[13], value.m16[14], value.m16[15]);
    }

    //---------------------------


    // count the number of characters in a string
    uint32 UnicodeString::CountNumChars(const UnicodeCharacter& character) const
    {
        if (mData == nullptr || mLength == 0)
        {
            return 0;
        }

        uint32 count = 0;
        UnicodeStringIterator iterator(*this);
        while (iterator.GetHasReachedEnd() == false)
        {
            UnicodeCharacter c = iterator.GetNextCharacter();
            if (c == character)
            {
                count++;
            }
        }

        return count;
    }


    // find a given substring, returns the position, or MCORE_INVALIDINDEX32 when not found
    uint32 UnicodeString::Find(const char* subString) const
    {
        if (mData == nullptr || mLength == 0 || subString == nullptr || subString[0] == 0)
        {
            return MCORE_INVALIDINDEX32;
        }

        // try to locate the string
        const char* strResult = strstr(mData, subString);

        // if the string has not been found
        if (strResult == nullptr)
        {
            return MCORE_INVALIDINDEX32;
        }

        // else return the position
        return static_cast<uint32>(strResult - mData);
    }


    // find a given substring, returns the position, or MCORE_INVALIDINDEX32 when not found
    // this starts searching at the end of the string, moving its way to the beginning
    uint32 UnicodeString::FindRight(const char* subString) const
    {
        if (mData == nullptr || mLength == 0 || subString == nullptr || subString[0] == 0)
        {
            return MCORE_INVALIDINDEX32;
        }

        // try to locate the string
        // search the string from end to start
        UnicodeStringIterator iterator(*this);
        iterator.SetIndexToEnd();
        do
        {
            UnicodeCharacter c = iterator.GetPreviousCharacter();
            const uint32 index = c.GetIndex();
            const char* strResult = strstr(&mData[index], subString);
            if (strResult)
            {
                return index;
            }
        } while (iterator.GetIndex() != 0);

        return MCORE_INVALIDINDEX32;
    }


    // search for a given character, starting from the end of the string to the start, and return the code unit index, or MCORE_INVALIDINDEX32 when not found
    uint32 UnicodeString::FindRight(const UnicodeCharacter& character) const
    {
        if (mData == nullptr || mLength == 0)
        {
            return MCORE_INVALIDINDEX32;
        }

        // search the character from end to start
        UnicodeStringIterator iterator(*this);
        iterator.SetIndexToEnd();
        do
        {
            UnicodeCharacter c = iterator.GetPreviousCharacter();
            if (c == character)
            {
                return iterator.GetIndex();
            }
        } while (iterator.GetIndex() != 0);

        // character hasn't been found
        return MCORE_INVALIDINDEX32;
    }


    // split the string into substrings. "a;b;c" with splitchar ';' would result in an array of 3 strings ("a", "b", "c")
    Array<UnicodeString> UnicodeString::Split(const UnicodeCharacter& splitChar) const
    {
        Array<UnicodeString> result;
        result.SetMemoryCategory(MCORE_MEMCATEGORY_STRINGOPS);
        if (mData == nullptr || mLength == 0)
        {
            return result;
        }

        // count the number of split characters
        const uint32 numSplitChars = CountNumChars(splitChar);

        // reserve array memory for our strings, to prevent reallocs
        result.Reserve(numSplitChars + 1);

        const uint32 numSplitCharBytes = splitChar.CalcNumRequiredUTF8Bytes();

        uint32 startOffset = 0;
        UnicodeStringIterator iterator(*this);
        while (iterator.GetHasReachedEnd() == false)
        {
            const UnicodeCharacter curChar = iterator.GetNextCharacter();

            // if we reached the end of the string
            if (iterator.GetHasReachedEnd())
            {
                //if (startOffset - iterator.GetIndex() > 0)
                if (startOffset != iterator.GetIndex())
                {
                    result.AddEmpty();
                    if (curChar != splitChar)
                    {
                        result.GetLast().Copy(&mData[startOffset], iterator.GetIndex() - startOffset);
                    }
                    else
                    {
                        result.GetLast().Copy(&mData[startOffset], iterator.GetIndex() - startOffset - numSplitCharBytes);
                    }
                }
                break;
            }

            // if the current character is the split character
            if (curChar == splitChar)
            {
                result.AddEmpty();
                result.GetLast().Copy(&mData[startOffset], iterator.GetIndex() - startOffset - numSplitCharBytes);
                startOffset = iterator.GetIndex();
            }
        }

        // if there are no substrings, return itself
        if (result.GetLength() == 0)
        {
            if (FindRight(splitChar) != MCORE_INVALIDINDEX32)
            {
                result.Add(*this);
            }
        }

        return result;
    }


    // returns true if this string is equal to the given other string (case sensitive)
    bool UnicodeString::CheckIfIsEqual(const char* other) const
    {
        // if the length of the strings isn't equal, they are not equal ofcourse
        //if (mLength != (uint32)strlen(other))
        //return false;

        // compare
        return (SafeCompare(mData, other) == 0);
    }


    // returns true if this string is equal to the given other string (not case sensitive)
    bool UnicodeString::CheckIfIsEqualNoCase(const char* other) const
    {
        // length not equal
        //if (mLength != (uint32)strlen(other))
        //return false;

        // compare
        return (CompareNoCase(other) == 0);
    }



    // compares two strings (case sensitive) returns 0 when equal, -1 when this string is bigger and 1 when other is bigger
    int32 UnicodeString::Compare(const char* other) const
    {
        return SafeCompare(mData, other);
    }


    // compares two strings (non case sensitive) returns 0 when equal, -1 when this string is bigger and 1 when other is bigger
    int32 UnicodeString::CompareNoCase(const char* other) const
    {
        const char* with = AsChar();
        const char* what = (other) ? other : "";

        return azstricmp(with, what);
    }


    // returns the number of words inside this string
    uint32 UnicodeString::CalcNumWords() const
    {
        if (mData == nullptr || mLength == 0)
        {
            return 0;
        }

        uint32 numWords = 0;

        // read away all spaces in the beginning
        UnicodeStringIterator iterator(*this);
        while (iterator.GetHasReachedEnd() == false)
        {
            const uint32 curPos = iterator.GetIndex();
            UnicodeCharacter c = iterator.GetNextCharacter();
            if (c != UnicodeCharacter::space && c != UnicodeCharacter::endLine && c != UnicodeCharacter::tab)
            {
                iterator.SetIndex(curPos);
                break;
            }
        }

        // count the number of words
        while (iterator.GetHasReachedEnd() == false)
        {
            // move cursor to next space
            while (iterator.GetHasReachedEnd() == false)
            {
                UnicodeCharacter c = iterator.GetNextCharacter();
                if (c == UnicodeCharacter::space || c == UnicodeCharacter::endLine || c == UnicodeCharacter::tab)
                {
                    break;
                }
            }


            numWords++;

            // move cursor to next word
            bool lastIsSpace = true;
            while (iterator.GetHasReachedEnd() == false)
            {
                UnicodeCharacter c = iterator.GetNextCharacter();
                if (c != UnicodeCharacter::space && c != UnicodeCharacter::endLine && c != UnicodeCharacter::tab)
                {
                    lastIsSpace = false;
                    break;
                }
            }

            if (iterator.GetHasReachedEnd() && lastIsSpace == false)
            {
                numWords++;
            }
        }

        return numWords;
    }


    // returns word number <wordNr>
    UnicodeString UnicodeString::ExtractWord(uint32 wordNr) const
    {
        MCORE_ASSERT(wordNr < CalcNumWords()); // slow in debugmode, but more safe

        if (mData == nullptr || mLength == 0)
        {
            return UnicodeString();
        }

        UnicodeString result;
        uint32 numWords = 0;

        // read away all spaces in the beginning
        UnicodeStringIterator iterator(*this);
        while (iterator.GetHasReachedEnd() == false)
        {
            const uint32 curPos = iterator.GetIndex();
            UnicodeCharacter c = iterator.GetNextCharacter();
            if (c != UnicodeCharacter::space && c != UnicodeCharacter::endLine && c != UnicodeCharacter::tab)
            {
                iterator.SetIndex(curPos);
                break;
            }
        }

        // count the number of words
        uint32 curPos = iterator.GetIndex();
        while (iterator.GetHasReachedEnd() == false)
        {
            const uint32 startOffset = curPos;

            // move cursor to next space
            while (iterator.GetHasReachedEnd() == false)
            {
                curPos = iterator.GetIndex();
                UnicodeCharacter c = iterator.GetNextCharacter();
                if (c == UnicodeCharacter::space || c == UnicodeCharacter::endLine || c == UnicodeCharacter::tab)
                {
                    break;
                }
            }


            // if this is the word number we are interested in
            if (numWords == wordNr)
            {
                if (iterator.GetHasReachedEnd() == false)
                {
                    result.Copy(&mData[startOffset], curPos - startOffset);
                }
                else
                {
                    result.Copy(&mData[startOffset], curPos - startOffset + 1);
                }

                return result;
            }

            numWords++;

            // move cursor to next word
            bool lastIsSpace = true;
            while (iterator.GetHasReachedEnd() == false)
            {
                curPos = iterator.GetIndex();
                UnicodeCharacter c = iterator.GetNextCharacter();
                if (c != UnicodeCharacter::space && c != UnicodeCharacter::endLine && c != UnicodeCharacter::tab)
                {
                    lastIsSpace = false;
                    break;
                }
            }

            if (iterator.GetHasReachedEnd() && lastIsSpace == false)
            {
                if (numWords == wordNr)
                {
                    result.Copy(&mData[curPos], mLength - curPos);
                    return result;
                }

                numWords++;
            }
        }

        return result;
    }


    // removes a given part from the string (the first found one)
    bool UnicodeString::RemoveFirstPart(const char* part)
    {
        const uint32 pos = Find(part);

        // substring not found, so exit
        if (pos == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        const uint32 partLength = (uint32)strlen(part);

        // remove the part
        MCore::MemMove(mData + pos, mData + pos + partLength, (mLength - (pos + partLength)) * sizeof(char));

        // resize the memory amount
        Alloc(mLength - partLength, 0);

        return true;
    }


    // removes all given parts from a string (all occurences)
    bool UnicodeString::RemoveAllParts(const char* part)
    {
        if (mData == nullptr || mLength == 0)
        {
            return false;
        }

        bool result = false;
        while (RemoveFirstPart(part))
        {
            result = true;
        }

        return result;
    }


    // removes a given set of characters from the string
    bool UnicodeString::RemoveChars(const Array<UnicodeCharacter>& charSet)
    {
        if (mData == nullptr || mLength == 0)
        {
            return false;
        }

        bool result = false;

        // for all characters in the set
        char partToRemove[8];
        const uint32 numChars = charSet.GetLength();
        for (uint32 i = 0; i < numChars; ++i)
        {
            uint32 numBytes = 0;
            charSet[i].AsUTF8(partToRemove, &numBytes, true);
            MCORE_ASSERT(numBytes < 8);
            if (RemoveAllParts(partToRemove))
            {
                result = true;
            }
        }

        return result;
    }


    // removes a given set of characters from the string
    bool UnicodeString::RemoveChars(const char* characterSet)
    {
        if (mData == nullptr || mLength == 0)
        {
            return false;
        }

        bool result = false;

        UnicodeString s = characterSet;

        // for all characters in the set
        char partToRemove[8];
        //  UnicodeStringIterator iterator(characterSet);
        UnicodeStringIterator iterator(s);
        while (iterator.GetHasReachedEnd() == false)
        {
            UnicodeCharacter c = iterator.GetNextCharacter();

            uint32 numBytes = 0;
            c.AsUTF8(partToRemove, &numBytes, true);
            MCORE_ASSERT(numBytes < 8);
            if (RemoveAllParts(partToRemove))
            {
                result = true;
            }
        }

        return result;
    }


    // remove the extension from a given string containing a filename
    // this removes everything after the last encountered dot, so "filename.bla" would result in "filename"
    void UnicodeString::RemoveFileExtension()
    {
        if (mData == nullptr || mLength == 0)
        {
            return;
        }

        // search for the dot, starting from the end of the string, searching towards the first character
        const uint32 dotPos = FindRight(UnicodeCharacter::dot);

        // search for any slash, starting from the end of the string, searching towards the first character
        uint32 slashPos = MCORE_INVALIDINDEX32;
        UnicodeStringIterator iterator(*this);
        iterator.SetIndexToEnd();
        do
        {
            UnicodeCharacter c = iterator.GetPreviousCharacter();
            if (c == UnicodeCharacter::backSlash || c == UnicodeCharacter::forwardSlash)
            {
                slashPos = iterator.GetIndex();
                break;
            }
        } while (iterator.GetIndex() != 0);

        // if the dot has not been found, there is nothing to remove, since the string contains no extension
        if (dotPos == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // if there is a slash present
        if (slashPos != MCORE_INVALIDINDEX32)
        {
            // if the slash comes after the dot, like in "../SomeFileNameWithoutExtension", then do nothing
            if (slashPos > dotPos)
            {
                return;
            }
        }

        // if the string ends with a dot, then that's no extension
        if (dotPos == mLength - 1)
        {
            return;
        }

        // simply terminate the string at the dot location
        mLength = dotPos;
        mData[dotPos] = 0;
    }


    // extract the file name
    UnicodeString UnicodeString::ExtractFileName() const
    {
        if (mData == nullptr || mLength == 0)
        {
            return UnicodeString();
        }

        // find the forward slash and/or back slash
        const uint32 forwardSlashIndex = FindRight(UnicodeCharacter::forwardSlash);
        const uint32 backSlashIndex    = FindRight(UnicodeCharacter::backSlash);

        // if no slash or backslash found, simply return itself
        if (forwardSlashIndex == MCORE_INVALIDINDEX32 && backSlashIndex == MCORE_INVALIDINDEX32)
        {
            return *this;
        }

        // pick the one at the end
        uint32 index = 0;
        if (forwardSlashIndex != MCORE_INVALIDINDEX32)
        {
            index = forwardSlashIndex;
        }

        if (backSlashIndex != MCORE_INVALIDINDEX32)
        {
            index = Max<uint32>(index, backSlashIndex);
        }

        // copy the filename into the result string
        UnicodeString result;
        result.Copy(mData + index + 1, mLength - index - 1);

        // if there is no dot in the filename, then there is no filename, so return an empty string
        if (result.FindRight(UnicodeCharacter::dot) == MCORE_INVALIDINDEX32)
        {
            return *this;
        }

        // return the result
        return result;
    }


    // extract the path
    UnicodeString UnicodeString::ExtractPath(bool includeSlashAtEnd) const
    {
        if (mData == nullptr || mLength == 0)
        {
            return UnicodeString();
        }

        // first extract the file name
        UnicodeString filename = ExtractFileName();

        // since the filename is stored at the end, everything in front of the filename is considered as path
        UnicodeString result;
        result.Copy(mData, mLength - filename.GetLength());

        // find the forward slash and/or back slash
        const uint32 forwardSlashIndex = result.FindRight(UnicodeCharacter::forwardSlash);
        const uint32 backSlashIndex    = result.FindRight(UnicodeCharacter::backSlash);

        // if no slash or backslash found, simply return the same string
        if (forwardSlashIndex == MCORE_INVALIDINDEX32 && backSlashIndex == MCORE_INVALIDINDEX32)
        {
            return *this;
        }

        // decide to use a forward or backslash when we have to end with a slash
        bool useForward = true;
        if (forwardSlashIndex != MCORE_INVALIDINDEX32)
        {
            useForward = true;
        }

        if (backSlashIndex != MCORE_INVALIDINDEX32)
        {
            if (forwardSlashIndex < backSlashIndex)
            {
                useForward = false;
            }
        }

        // make sure there are no slashes at the end when we don't want it
        if (!includeSlashAtEnd)
        {
            result.TrimRight(UnicodeCharacter::backSlash);
            result.TrimRight(UnicodeCharacter::forwardSlash);
        }
        else // when we do want a slash at the end, make sure it's there
        {
            // when the last character isn't a forward slash or backslash, add it
            UnicodeStringIterator iterator(result);
            iterator.SetIndexToEnd();
            UnicodeCharacter lastChar = iterator.GetPreviousCharacter();
            if (lastChar != UnicodeCharacter::forwardSlash && lastChar != UnicodeCharacter::backSlash)
            {
                // check if we need to add a forward slash or backslash
                if (useForward)
                {
                    result += '/';
                }
                else
                {
                    result += '\\';
                }
            }
        }

        // return the result
        return result;
    }


    // remove all given trimChars on the left of the string
    void UnicodeString::TrimLeft(const UnicodeCharacter& trimChar)
    {
        if (mData == nullptr || mLength == 0)
        {
            return;
        }

        // for all characters in the string
        UnicodeStringIterator iterator(*this);
        while (iterator.GetHasReachedEnd() == false)
        {
            UnicodeCharacter c = iterator.GetNextCharacter();

            // return when another character found
            uint32 index = c.GetIndex();
            if (c != trimChar)
            {
                if (index == 0)
                {
                    return;
                }

                const uint32 numUnits = (mLength - index) + 1; // +1 because of the '\0'
                MCore::MemMove(mData, mData + index, numUnits * sizeof(char));
                mLength -= index;
                return;
            }
        }

        // in case we have for example a string of only spaces
        Clear(true);
    }



    // remove all given trimChars on the right of the string
    void UnicodeString::TrimRight(const UnicodeCharacter& trimChar)
    {
        if (mData == nullptr || mLength == 0)
        {
            return;
        }

        UnicodeStringIterator iterator(*this);
        iterator.SetIndexToEnd();
        do
        {
            UnicodeCharacter c = iterator.GetPreviousCharacter();
            if (c != trimChar)
            {
                uint32 index = c.GetIndex();

                // remove the last chars
                mLength = index + c.CalcNumRequiredUTF8Bytes();
                mData[mLength] = 0;

                return;
            }
        } while (iterator.GetIndex() != 0);

        // seems like we have a string of only spaces for example
        Clear(true);
    }



    // remove all given trimChars on both left and right side of the string
    void UnicodeString::Trim(const UnicodeCharacter& trimChar)
    {
        TrimLeft(trimChar);
        TrimRight(trimChar);
    }


    // remove the last character if it is equal to a given character
    void UnicodeString::RemoveLastCharacterIfEqualTo(const UnicodeCharacter& lastCharacter)
    {
        if (mData == nullptr || mLength == 0)
        {
            return;
        }

        UnicodeStringIterator iterator(*this);
        iterator.SetIndexToEnd();
        UnicodeCharacter c = iterator.GetPreviousCharacter();
        if (c == lastCharacter)
        {
            // remove the last char
            const uint32 index = c.GetIndex();
            mLength = index;
            mData[index] = 0;
        }
    }


    // remove the first character if it is equal to a given character
    void UnicodeString::RemoveFirstCharacterIfEqualTo(const UnicodeCharacter& firstCharacter)
    {
        if (mData == nullptr || mLength == 0)
        {
            return;
        }

        UnicodeStringIterator iterator(*this);
        UnicodeCharacter c = iterator.GetNextCharacter();
        if (c == firstCharacter)
        {
            const uint32 numCharBytes = c.CalcNumRequiredUTF8Bytes();
            MemMove((uint8*)mData, (uint8*)mData + (sizeof(char) * numCharBytes), (mLength - (numCharBytes * sizeof(char))) * sizeof(char));
            mLength -= numCharBytes;
            mData[mLength] = 0;
        }
    }


    // extract the file extension
    UnicodeString UnicodeString::ExtractFileExtension() const
    {
        if (mData == nullptr || mLength == 0)
        {
            return UnicodeString();
        }

        // search for the dot, starting from the end of the string, searching towards the first character
        const uint32 dotPos = FindRight(UnicodeCharacter::dot);

        // if the dot has not been found, there is nothing to remove, since the string contains no extension
        if (dotPos == MCORE_INVALIDINDEX32)
        {
            return UnicodeString(); // an empty string
        }
        // search for any slash, starting from the end of the string, searching towards the first character
        uint32 slashPos = MCORE_INVALIDINDEX32;
        UnicodeStringIterator iterator(*this);
        iterator.SetIndexToEnd();
        do
        {
            UnicodeCharacter c = iterator.GetPreviousCharacter();
            if (c == UnicodeCharacter::backSlash || c == UnicodeCharacter::forwardSlash)
            {
                slashPos = iterator.GetIndex();
                break;
            }
        } while (iterator.GetIndex() != 0);

        // if there is a slash present
        if (slashPos != MCORE_INVALIDINDEX32)
        {
            // if the slash comes after the dot, like in "../SomeFileNameWithoutExtension", then do nothing
            if (slashPos > dotPos)
            {
                return UnicodeString();
            }
        }

        // if the string ends with a dot, then that's no extension
        if (dotPos == mLength - 1)
        {
            return UnicodeString();
        }

        // return the extension
        return UnicodeString((const char*)(&mData[dotPos + 1]));
    }


    // convert slashes of a file path to the natively used ones on the used platform
    void UnicodeString::ConvertToNativePath()
    {
        Replace(UnicodeCharacter::forwardSlash,    UnicodeCharacter((char)FileSystem::mFolderSeparatorChar));
        Replace(UnicodeCharacter::backSlash,       UnicodeCharacter((char)FileSystem::mFolderSeparatorChar));
    }


    // convert slashes of a copied file path to the natively used ones on the used platform and return the result
    UnicodeString UnicodeString::ConvertedToNativePath() const
    {
        UnicodeString result(*this);
        result.ConvertToNativePath();
        return result;
    }


    // replace a given string with another one
    // for example replace all %NAME% parts of the string with a given name
    void UnicodeString::Replace(const char* what, const char* with)
    {
        if (mData == nullptr || mLength == 0)
        {
            return;
        }

        const uint32 withLen = (uint32)strlen(with);
        const uint32 whatLen = (uint32)strlen(what);

        if (whatLen == 0)
        {
            return;
        }

        uint32 location = Find(what);
        UnicodeString temp;
        while (location != MCORE_INVALIDINDEX32)
        {
            //numPasses++;
            temp.Alloc(mLength + (withLen - whatLen), 0);
            MCore::MemCopy(temp.mData, mData, location * sizeof(char));
            MCore::MemCopy(temp.mData + location, with, withLen * sizeof(char));
            MCore::MemCopy(temp.mData + location + withLen, mData + location + whatLen, (mLength - location - whatLen) * sizeof(char));
            *this = temp;

            // try to locate the string again
            const char* strResult = strstr(mData + location + withLen, what);
            if (strResult == nullptr)
            {
                location = MCORE_INVALIDINDEX32; // if the string has not been found break the loop
            }
            else
            {
                location = static_cast<uint32>(strResult - mData); // in case the string has been found go again
            }
        }
    }


    // replace a given string with another one, but only the first occurrence
    // for example replace all %NAME% parts of the string with a given name
    void UnicodeString::ReplaceFirst(const char* what, const char* with)
    {
        if (mData == nullptr || mLength == 0)
        {
            return;
        }

        const uint32 withLen = (uint32)strlen(with);
        const uint32 whatLen = (uint32)strlen(what);
        if (whatLen == 0)
        {
            return;
        }

        uint32 location = Find(what);
        UnicodeString temp;
        if (location != MCORE_INVALIDINDEX32)
        {
            //numPasses++;
            temp.Alloc(mLength + (withLen - whatLen), 0);
            MCore::MemCopy(temp.mData, mData, location * sizeof(char));
            MCore::MemCopy(temp.mData + location, with, withLen * sizeof(char));
            MCore::MemCopy(temp.mData + location + withLen, mData + location + whatLen, (mLength - location - whatLen) * sizeof(char));
            *this = temp;

            // try to locate the string again
            const char* strResult = strstr(mData + location + withLen, what);
            if (strResult == nullptr)
            {
                location = MCORE_INVALIDINDEX32; // if the string has not been found break the loop
            }
            else
            {
                location = static_cast<uint32>(strResult - mData); // in case the string has been found go again
            }
        }
    }


    // replace a given string with another one, but only the last occurrence
    // for example replace all %NAME% parts of the string with a given name
    void UnicodeString::ReplaceLast(const char* what, const char* with)
    {
        if (mData == nullptr || mLength == 0)
        {
            return;
        }

        const uint32 withLen = (uint32)strlen(with);
        const uint32 whatLen = (uint32)strlen(what);
        if (whatLen == 0)
        {
            return;
        }

        uint32 location = FindRight(what);
        UnicodeString temp;
        if (location != MCORE_INVALIDINDEX32)
        {
            //numPasses++;
            temp.Alloc(mLength + (withLen - whatLen), 0);
            MCore::MemCopy(temp.mData, mData, location * sizeof(char));
            MCore::MemCopy(temp.mData + location, with, withLen * sizeof(char));
            MCore::MemCopy(temp.mData + location + withLen, mData + location + whatLen, (mLength - location - whatLen) * sizeof(char));
            *this = temp;

            // try to locate the string again
            const char* strResult = strstr(mData + location + withLen, what);
            if (strResult == nullptr)
            {
                location = MCORE_INVALIDINDEX32; // if the string has not been found break the loop
            }
            else
            {
                location = static_cast<uint32>(strResult - mData); // in case the string has been found go again
            }
        }
    }




    // replace a given character with another one
    void UnicodeString::Replace(const UnicodeCharacter& c, const UnicodeCharacter& with)
    {
        // if no data has been allocated yet skip directly
        if (c == with || mData == nullptr || mLength == 0)
        {
            return;
        }

        const uint32 numCBytes = c.CalcNumRequiredUTF8Bytes();

        // iterate through all characters and replace them if needed
        UnicodeStringIterator iterator(*this);
        while (iterator.GetHasReachedEnd() == false)
        {
            UnicodeCharacter character = iterator.GetNextCharacter();

            // check if the current character is the one we want to replace
            if (character == c)
            {
                uint32 numWithBytes = with.CalcNumRequiredUTF8Bytes();
                if (numCBytes == numWithBytes)
                {
                    with.AsUTF8(&mData[character.GetIndex()], &numWithBytes, false);
                }
                else
                {
                    const int32 numExtraChars = numWithBytes - numCBytes;
                    if (numExtraChars > 0)
                    {
                        Resize(mLength + numExtraChars, ' ', false);
                    }

                    const uint32 numBytesToMove = mLength - character.GetIndex() - numExtraChars - 1;
                    MemMove(&mData[character.GetIndex() + numWithBytes], &mData[iterator.GetIndex()], numBytesToMove);
                    with.AsUTF8(&mData[character.GetIndex()], &numWithBytes, false);

                    if (numExtraChars < 0)
                    {
                        Resize((uint32)strlen(mData));  // adjust the length if we did shrink down
                    }
                    iterator.InitFromString(*this);
                    iterator.SetIndex(character.GetIndex() + numWithBytes);
                }
            }
        }
    }


    // replace spaces with non-breaking html spaces
    void UnicodeString::ConvertToNonBreakingHTMLSpaces()
    {
        // count the spaces to be able reserve enough memory so that we can perform the replace without any allocation
        const uint32 numSpaces = CountNumChars(UnicodeCharacter::space);

        // this is the html non-breaking space
        const char* with = "&nbsp;";
        const uint32 withLen = 6;
        //const char* with = "<p style=\"color:rgb(50,50,50)\">_</p>";
        //const uint32 withLen = (strlen(with);

        // temporary string where we copy to
        MCore::UnicodeString temp;
        temp.Resize(mLength + numSpaces * (withLen - 1));

        // iterate through the characters in the string
        uint32 tempLocation = 0;
        uint32 orgLocation  = 0;
        UnicodeStringIterator iterator(*this);
        while (iterator.GetHasReachedEnd() == false)
        {
            UnicodeCharacter c = iterator.GetNextCharacter();
            if (c == UnicodeCharacter::space)
            {
                // we found a space to convert
                MCore::MemCopy(temp.mData + tempLocation, with, withLen);
                tempLocation += withLen;
            }
            else
            {
                uint32 numBytes;
                c.AsUTF8(&mData[orgLocation], &numBytes, false);
                MCore::MemCopy(temp.mData + tempLocation, &mData[orgLocation], numBytes);
                tempLocation += numBytes;
            }

            orgLocation++;
        }

        // copy the result back
        *this = temp;
    }


    // resize the string
    void UnicodeString::Resize(uint32 length, char fillChar, bool doFill)
    {
        if (length == mLength)
        {
            return;
        }

        // clear the string
        if (length == 0)
        {
            Clear(true);
            return;
        }

        // make the string smaller
        if (length < mLength)
        {
            mData[length] = 0;
            mLength = length;
            return;
        }

        // make it bigger
        const uint32 oldLength = mLength;
        Alloc(length, 0);
        if (doFill)
        {
            for (uint32 i = oldLength; i < length; ++i)
            {
                mData[i] = fillChar;
            }
        }
    }


    // pre-alloc space
    void UnicodeString::Reserve(uint32 length)
    {
        if (length == 0)
        {
            return;
        }

        if ((length > mMaxLength) || mData == nullptr)
        {
            mMaxLength  = length;

            if (mData)
            {
                mData   = (char*)MCore::Realloc(mData, (mMaxLength + 1) * sizeof(char), MCORE_MEMCATEGORY_STRING, UnicodeString::MEMORYBLOCK_ID, MCORE_FILE, MCORE_LINE);
            }
            else
            {
                mData   = (char*)MCore::Allocate((mMaxLength + 1) * sizeof(char), MCORE_MEMCATEGORY_STRING, UnicodeString::MEMORYBLOCK_ID, MCORE_FILE, MCORE_LINE);
                mData[0] = 0;
            }
        }
    }


    // returns an uppercase version of this string
    UnicodeString UnicodeString::Uppered() const
    {
        UnicodeString newString(*this);
        newString.ToUpper();
        return newString;
    }


    // returns a lowercase version of this string
    UnicodeString UnicodeString::Lowered() const
    {
        UnicodeString newString(*this);
        newString.ToLower();
        return newString;
    }


    // uppercase this string
    UnicodeString& UnicodeString::ToUpper()
    {
        if (mData == nullptr || mLength == 0)
        {
            return *this;
        }

        UnicodeStringIterator iterator(*this);
        while (iterator.GetHasReachedEnd() == false)
        {
            UnicodeCharacter c = iterator.GetNextCharacter();
            if (c.CalcNumRequiredUTF8Bytes() == 1)
            {
                mData[c.GetIndex()] = static_cast<uint8>(toupper(mData[c.GetIndex()]));
            }
        }

        return *this;
    }


    // lowercase this string
    UnicodeString& UnicodeString::ToLower()
    {
        if (mData == nullptr || mLength == 0)
        {
            return *this;
        }

        UnicodeStringIterator iterator(*this);
        while (iterator.GetHasReachedEnd() == false)
        {
            UnicodeCharacter c = iterator.GetNextCharacter();
            if (c.CalcNumRequiredUTF8Bytes() == 1)
            {
                mData[c.GetIndex()] = static_cast<uint8>(tolower(mData[c.GetIndex()]));
            }
        }

        return *this;
    }


    // align a string to a certain length
    UnicodeString& UnicodeString::Align(uint32 newNumCharacters, const UnicodeCharacter& character)
    {
        UnicodeStringIterator iterator(*this);
        const uint32 numCharacters = iterator.CalcNumCharacters(mData, mLength);

        // if the string is already long enough, there is nothing to do
        if (numCharacters >= newNumCharacters)
        {
            return *this;
        }

        //
        const uint32 numExtraCharacters = newNumCharacters - numCharacters;
        const uint32 numExtraBytes = character.CalcNumRequiredUTF8Bytes() * numExtraCharacters;

        // allocate space
        const uint32 oldLength = mLength;
        Alloc(mLength + numExtraBytes, 0);

        // add the extra characters
        uint32 numCharBytes;
        uint32 offset = oldLength;
        for (uint32 i = 0; i < numExtraCharacters; ++i)
        {
            character.AsUTF8(&mData[offset], &numCharBytes, false);
            offset += numCharBytes;
        }

        return *this;
    }


    // left elide
    void UnicodeString::ElideLeft(uint32 maxNumChars, const char* fillText)
    {
        MCORE_ASSERT(maxNumChars > 0);

        // nothing to do if the string length is already withing limits
        if (mLength <= maxNumChars)
        {
            return;
        }

        // get the number of characters in the fill text
        const uint32 numFillTextChars = (uint32)strlen(fillText);
        MCORE_ASSERT(maxNumChars >= numFillTextChars);  // max length of say 1, while using "..." as fill text
        if (maxNumChars <= numFillTextChars)
        {
            return;
        }

        // check how many characters we have to remove
        const int32 numToRemove = mLength - maxNumChars + numFillTextChars;
        if (numToRemove <= 0)
        {
            return;
        }

        // remove the characters from the left
        MCore::MemMove(mData + numFillTextChars, mData + numToRemove, (mLength - numToRemove) * sizeof(char));

        // insert the fill text
        MCore::MemCopy(mData, fillText, numFillTextChars * sizeof(char));

        // adjust string length
        Resize(maxNumChars);
    }


    // right elide
    void UnicodeString::ElideRight(uint32 maxNumChars, const char* fillText)
    {
        MCORE_ASSERT(maxNumChars > 0);

        // nothing to do if the string length is already withing limits
        if (mLength <= maxNumChars)
        {
            return;
        }

        // get the number of characters in the fill text
        const uint32 numFillTextChars = (uint32)strlen(fillText);
        MCORE_ASSERT(maxNumChars >= numFillTextChars);  // max length of say 1, while using "..." as fill text
        if (maxNumChars <= numFillTextChars)
        {
            return;
        }

        // check how many characters we have to remove
        const int32 numToRemove = mLength - maxNumChars + numFillTextChars;
        if (numToRemove <= 0)
        {
            return;
        }

        // insert the fill text
        MCore::MemCopy(mData + maxNumChars - numFillTextChars, fillText, numFillTextChars * sizeof(char));

        // remove the characters from the left
        Resize(maxNumChars);
    }


    // middle elide
    void UnicodeString::ElideMiddle(uint32 maxNumChars, const char* fillText)
    {
        MCORE_ASSERT(maxNumChars > 0);

        // nothing to do if the string length is already withing limits
        if (mLength <= maxNumChars)
        {
            return;
        }

        // get the number of characters in the fill text
        const uint32 numFillTextChars = (uint32)strlen(fillText);
        MCORE_ASSERT(maxNumChars >= numFillTextChars);  // max length of say 1, while using "..." as fill text
        if (maxNumChars <= numFillTextChars)
        {
            return;
        }

        // check how many characters we have to remove
        const int32 numToRemove = mLength - maxNumChars + numFillTextChars;
        if (numToRemove <= 0)
        {
            return;
        }

        // calculate how many characters we will have on the left, and how many on the right
        const uint32 numCharsLeft   = (maxNumChars - numFillTextChars) >> 1;
        const uint32 numCharsRight  = maxNumChars - numCharsLeft - numFillTextChars;

        // insert the fill text
        MCore::MemCopy(mData + numCharsLeft, fillText, numFillTextChars * sizeof(char));

        // copy the right part
        MCore::MemMove(mData + numCharsLeft + numFillTextChars, mData + mLength - numCharsRight, numCharsRight * sizeof(char));

        // remove the characters from the left
        Resize(maxNumChars);
    }


    // return a left elided version
    UnicodeString UnicodeString::ElidedLeft(uint32 maxNumChars, const char* fillText) const
    {
        UnicodeString result(*this);
        result.ElideLeft(maxNumChars, fillText);
        return result;
    }


    // return a right elided version
    UnicodeString UnicodeString::ElidedRight(uint32 maxNumChars, const char* fillText) const
    {
        UnicodeString result(*this);
        result.ElideRight(maxNumChars, fillText);
        return result;
    }


    // return a middle elided version
    MCore::UnicodeString UnicodeString::ElidedMiddle(uint32 maxNumChars, const char* fillText) const
    {
        UnicodeString result(*this);
        result.ElideMiddle(maxNumChars, fillText);
        return result;
    }


    // init from UTF16
    void UnicodeString::FromUTF16(const uint16* input)
    {
        Clear();
        if (input == nullptr)
        {
            return;
        }

        // count the number of uint16 code units and calculate the number of required UTF8 bytes
        uint32 numRequiredUTF8Bytes = 0;
        uint32 numInputCodeUnits = 0;
        uint16 ch = input[0];
        uint16 ch2 = 0;
        while (ch != 0)
        {
            ch = input[numInputCodeUnits++];
            if (ch >= UnicodeStringIterator::UNICODE_SURHIGH_START && ch <= UnicodeStringIterator::UNICODE_SURHIGH_END) // if we need 2 uint16's to represent one character
            {
                ch2 = input[numInputCodeUnits++];
            }
            else
            {
                ch2 = 0;
            }

            // check how many byts are needed in UTF8 to represent this character
            UnicodeCharacter unicodeChar(ch, ch2);
            numRequiredUTF8Bytes += unicodeChar.CalcNumRequiredUTF8Bytes();
        }

        if (numRequiredUTF8Bytes == 0)
        {
            return;
        }

        // resize the string buffer
        Resize(numRequiredUTF8Bytes - 1);

        // get some pointers for the source and dest locations of the conversion
        const uint16* sourceStart = input;
        const uint16* sourceEnd = reinterpret_cast<const uint16*>(&input[numInputCodeUnits - 1]);
        uint8* targetStart = reinterpret_cast<uint8*>(mData);
        uint8* targetEnd = reinterpret_cast<uint8*>(&mData[mLength]);

        // convert
    #ifdef MCORE_DEBUG
        UnicodeStringIterator::ConvertUTF16toUTF8(&sourceStart, sourceEnd, &targetStart, targetEnd, true);
    #else
        UnicodeStringIterator::ConvertUTF16toUTF8(&sourceStart, sourceEnd, &targetStart, targetEnd, false);
    #endif
    }


    // init from UTF32
    void UnicodeString::FromUTF32(const uint32* input)
    {
        Clear();
        if (input == nullptr)
        {
            return;
        }

        // count the number of uint16 code units and calculate the number of required UTF8 bytes
        uint32 numRequiredUTF8Bytes = 0;
        uint32 numInputCodeUnits = 0;
        uint32 ch = input[0];
        while (ch != 0)
        {
            ch = input[numInputCodeUnits++];

            // check how many bytes are needed in UTF8 to represent this character
            UnicodeCharacter unicodeChar((uint32)ch);
            numRequiredUTF8Bytes += unicodeChar.CalcNumRequiredUTF8Bytes();
        }

        // resize the string buffer
        Resize(numRequiredUTF8Bytes - 1);

        // get some pointers for the source and dest locations of the conversion
        const uint32* sourceStart = input;
        const uint32* sourceEnd = reinterpret_cast<const uint32*>(&input[numInputCodeUnits - 1]);
        uint8* targetStart = reinterpret_cast<uint8*>(mData);
        uint8* targetEnd = reinterpret_cast<uint8*>(&mData[mLength]);

        // convert
    #ifdef MCORE_DEBUG
        UnicodeStringIterator::ConvertUTF32toUTF8(&sourceStart, sourceEnd, &targetStart, targetEnd, true);
    #else
        UnicodeStringIterator::ConvertUTF32toUTF8(&sourceStart, sourceEnd, &targetStart, targetEnd, false);
    #endif
    }


    // convert from wchar_t into utf8
    void UnicodeString::FromWChar(const wchar_t* input)
    {
    #ifdef MCORE_UTF16
        FromUTF16((uint16*)input);
    #else
        FromUTF32((uint32*)input);
    #endif
    }


    // get the wchar_t representation
    void UnicodeString::AsWChar(AbstractData* output) const
    {
    #ifdef MCORE_UTF16
        AsUTF16(output);
    #else
        AsUTF32(output);
    #endif
    }



    // get the UTF16 representation
    void UnicodeString::AsUTF16(AbstractData* output) const
    {
        // if the string length is zero
        if (mLength == 0)
        {
            output->Resize(sizeof(uint16));
            ((uint16*)output->GetPointer())[0] = 0;
            return;
        }

        // count the number of uint16 code units and calculate the number of required UTF8 bytes
        uint32 numUTF16Units = 1; // 1 for null terminator
        uint16 utf16[2];
        UnicodeStringIterator iterator(*this);
        while (iterator.GetHasReachedEnd() == false)
        {
            UnicodeCharacter c = iterator.GetNextCharacter();
            c.AsUTF16(utf16);

            if (utf16[1] == 0)
            {
                numUTF16Units++;
            }
            else
            {
                numUTF16Units += 2;
            }
        }

        output->Resize(sizeof(uint16) * numUTF16Units); // +1 for null terminator
        const uint8* sourceStart    = reinterpret_cast<uint8*>(mData);
        const uint8* sourceEnd      = reinterpret_cast<uint8*>((uint8*)mData + (mLength + 1) * sizeof(uint8));
        uint16* targetStart         = reinterpret_cast<uint16*>(output->GetPointer());
        uint16* targetEnd           = reinterpret_cast<uint16*>((uint8*)output->GetPointer() + numUTF16Units * sizeof(uint16));

    #ifdef MCORE_DEBUG
        UnicodeStringIterator::ConvertUTF8toUTF16(&sourceStart, sourceEnd, &targetStart, targetEnd, true);      // true enables strictness checks
    #else
        UnicodeStringIterator::ConvertUTF8toUTF16(&sourceStart, sourceEnd, &targetStart, targetEnd, false);
    #endif
    }


    // get the UTF32 representation
    void UnicodeString::AsUTF32(AbstractData* output) const
    {
        // if the string length is zero
        if (mLength == 0)
        {
            output->Resize(sizeof(uint16));
            ((uint16*)output->GetPointer())[0] = 0;
            return;
        }

        // count the number of characters
        UnicodeStringIterator iterator(*this);
        uint32 numUTF32Units = iterator.CalcNumCharacters(mData) + 1; // +1 for the null terminator

        output->Resize(sizeof(uint32) * numUTF32Units); // +1 for null terminator
        const uint8* sourceStart    = reinterpret_cast<uint8*>(mData);
        const uint8* sourceEnd      = reinterpret_cast<uint8*>((uint8*)mData + (mLength + 1) * sizeof(uint8));
        uint32* targetStart         = reinterpret_cast<uint32*>(output->GetPointer());
        uint32* targetEnd           = reinterpret_cast<uint32*>((uint8*)output->GetPointer() + numUTF32Units * sizeof(uint32));

    #ifdef MCORE_DEBUG
        UnicodeStringIterator::ConvertUTF8toUTF32(&sourceStart, sourceEnd, &targetStart, targetEnd, true);      // true enables strictness checks
    #else
        UnicodeStringIterator::ConvertUTF8toUTF32(&sourceStart, sourceEnd, &targetStart, targetEnd, false);
    #endif
    }


    // convert to wchar_t buffer
    AbstractData UnicodeString::AsWChar() const
    {
        AbstractData result;
        AsWChar(&result);
        return result;
    }


    // convert to UTF16
    AbstractData UnicodeString::AsUTF16() const
    {
        AbstractData result;
        AsUTF16(&result);
        return result;
    }


    // convert to UTF32
    AbstractData UnicodeString::AsUTF32() const
    {
        AbstractData result;
        AsUTF32(&result);
        return result;
    }


    // remove the line end
    void UnicodeString::RemoveLineEnd()
    {
        RemoveLastCharacterIfEqualTo(UnicodeCharacter('\n'));
        RemoveLastCharacterIfEqualTo(UnicodeCharacter('\r'));
    }


    // fix the line end
    void UnicodeString::FixLineEnd()
    {
        // remove any existing line end
        RemoveLineEnd();

        // add the line end
        const uint32 oldLength = mLength;
        Resize(mLength + 1, ' ', false);
        mData[oldLength] = '\n';
    }


    // insert a string inside this string
    void UnicodeString::Insert(uint32 index, const char* text)
    {
        size_t textLength = strlen(text);
        if (textLength == 0)
        {
            return;
        }

        size_t oldLength = mLength;
        Resize((uint32)(mLength + textLength), ' ', false);

        size_t numBytesToMove = oldLength - index;
        MemMove(&mData[index + textLength], &mData[index], numBytesToMove);
        MemCopy(&mData[index], text, textLength);
    }


    // extract the filename or folder that is relative to a given other folder
    UnicodeString UnicodeString::ExtractPathRelativeTo(const UnicodeString& relativeToFolder, bool removeLeadingFolderSeparators, bool removeLastFolderSeparators) const
    {
        if (mLength == 0 || relativeToFolder.GetLength() == 0)
        {
            return *this;
        }

        UnicodeString result;

        // if it is down the hierarchy
        if (Contains(relativeToFolder.AsChar()))
        {
            result = *this;
            result.RemoveFirstPart(relativeToFolder.AsChar());

            if (removeLeadingFolderSeparators)
            {
                result.RemoveFirstCharacterIfEqualTo(UnicodeCharacter::forwardSlash);
                result.RemoveFirstCharacterIfEqualTo(UnicodeCharacter::backSlash);
            }

            if (removeLastFolderSeparators)
            {
                result.RemoveLastCharacterIfEqualTo(UnicodeCharacter::forwardSlash);
                result.RemoveLastCharacterIfEqualTo(UnicodeCharacter::backSlash);
            }

            result.ConvertToNativePath();
            return result;
        }

        // find the equal first part
        uint32 numEqualChars = 0;
        UnicodeStringIterator pathIterator(*this);
        UnicodeStringIterator relIterator(relativeToFolder);
        while (pathIterator.GetHasReachedEnd() == false)
        {
            if (relIterator.GetHasReachedEnd())
            {
                break;
            }

            UnicodeCharacter pathChar = pathIterator.GetNextCharacter();
            UnicodeCharacter relChar  = relIterator.GetNextCharacter();

            if (pathChar == relChar)
            {
                numEqualChars++;
            }
            else
            {
                break;
            }
        }

        //
        UnicodeString equalPart;
        UnicodeString differentPart;
        const uint32 pathIndex = pathIterator.GetIndex();
        if (GetLength() >= pathIndex)
        {
            if (GetLength() == pathIndex)
            {
                equalPart.Copy(AsChar(), pathIndex);
                differentPart = &mData[pathIndex];
            }
            else
            {
                equalPart.Copy(AsChar(), pathIndex - 1);
                differentPart = &mData[pathIndex - 1];
            }
        }

        // just return the input part
        if (equalPart.GetLength() == 0)
        {
            return (*this).ConvertedToNativePath();
        }

        UnicodeStringIterator equalIt(equalPart);
        equalIt.SetIndexToEnd();
        UnicodeCharacter equalChar = equalIt.GetPreviousCharacter();
        while ((equalChar != UnicodeCharacter::backSlash && equalChar != UnicodeCharacter::forwardSlash) && equalIt.GetIndex() != 0)
        {
            equalChar = equalIt.GetPreviousCharacter();
        }

        MCore::String correction = &equalPart[equalChar.GetIndex()];
        differentPart = correction + differentPart;
        equalPart.GetPtr()[equalChar.GetIndex()] = '\0';

        //MCore::LogInfo( equalPart );
        //MCore::LogInfo( differentPart );

        // the result must be up the hierarchy, containing one or more ".."
        UnicodeString temp = relativeToFolder;
        temp.RemoveFirstPart(equalPart.AsChar());

        uint32 numSubFolders = 0;
        UnicodeStringIterator iterator(temp);
        while (iterator.GetHasReachedEnd() == false)
        {
            const UnicodeCharacter c = iterator.GetNextCharacter();
            if (c == UnicodeCharacter::backSlash || c == UnicodeCharacter::forwardSlash)
            {
                numSubFolders++;
            }
        }

        UnicodeStringIterator lastRelIterator(relativeToFolder);
        UnicodeStringIterator lastEqualIterator(equalPart);
        lastRelIterator.SetIndexToEnd();
        lastEqualIterator.SetIndexToEnd();
        const UnicodeCharacter lastRelChar      = lastRelIterator.GetPreviousCharacter();
        const UnicodeCharacter lastEqualChar    = lastEqualIterator.GetPreviousCharacter();

        if (lastEqualChar == UnicodeCharacter::backSlash || lastEqualChar == UnicodeCharacter::forwardSlash)
        {
            numSubFolders++;
        }

        if (lastRelChar == UnicodeCharacter::backSlash || lastRelChar == UnicodeCharacter::forwardSlash)
        {
            numSubFolders--;
        }

        for (uint32 i = 0; i < numSubFolders; ++i)
        {
            result += "..";
            result += FileSystem::mFolderSeparatorChar;
        }

        result += differentPart;

        if (removeLastFolderSeparators)
        {
            result.RemoveLastCharacterIfEqualTo(UnicodeCharacter::forwardSlash);
            result.RemoveLastCharacterIfEqualTo(UnicodeCharacter::backSlash);
        }

        if (result.FindRight("//") != 0 && result.FindRight("\\") != 0)
        {
            result.Replace("//", "/");
            result.Replace("\\\\", "\\");
        }

        result.Replace("\\/", "/");
        result.Replace("/\\", "/");
        result.ConvertToNativePath();

        return result;
    }


    // generate a unique string
    void UnicodeString::GenerateUniqueString(const char* prefix, const AZStd::function<bool(const MCore::UnicodeString& value)>& validationFunction)
    {
        MCORE_ASSERT(validationFunction);

        // get the anim graph name
        const MCore::UnicodeString prefixString = prefix;

        // create the string iterator and set it at the end
        MCore::UnicodeStringIterator stringIterator(prefixString);
        stringIterator.SetIndexToEnd();

        // find the last letter index from the right
        uint32 lastIndex = MCORE_INVALIDINDEX32;
        const uint32 numCharacters = stringIterator.CalcNumCharacters();
        for (int32 i = numCharacters - 1; i >= 0; --i)
        {
            const uint32 utf32Value = stringIterator.GetPreviousCharacter().AsUTF32();
            const bool isDigit = (utf32Value >= '0' && utf32Value <= '9');
            if (isDigit == false)
            {
                lastIndex = stringIterator.GetIndex();
                break;
            }
        }

        // copy the string
        MCore::UnicodeString nameWithoutLastDigits;
        nameWithoutLastDigits.Copy(prefixString, lastIndex + 1);

        // remove all space on the right
        nameWithoutLastDigits.TrimRight();

        // generate the unique name
        uint32 nameIndex = 0;
        MCore::UnicodeString uniqueName = nameWithoutLastDigits + "0";
        while (validationFunction(uniqueName) == false)
        {
            uniqueName = nameWithoutLastDigits + MCore::UnicodeString(++nameIndex);
        }

        *this = uniqueName;
    }


    // fix double slashes in paths
    void UnicodeString::FixPathDoubleSlashes(bool allowDoubleAtStart)
    {
        if (allowDoubleAtStart)
        {
            uint32 index = FindRight("//");
            while (index != MCORE_INVALIDINDEX32 && index != 0)
            {
                ReplaceLast("//", "/");
                index = FindRight("//");
            }

            index = FindRight("\\\\");
            while (index != MCORE_INVALIDINDEX32 && index != 0)
            {
                ReplaceLast("\\\\", "\\");
                index = FindRight("\\\\");
            }
        }
        else
        {
            uint32 index = FindRight("//");
            while (index != MCORE_INVALIDINDEX32)
            {
                ReplaceLast("//", "/");
                index = FindRight("//");
            }

            index = FindRight("\\\\");
            while (index != MCORE_INVALIDINDEX32)
            {
                ReplaceLast("\\\\", "\\");
                index = FindRight("\\\\");
            }
        }
    }


    // replace invalid characters with an underscore
    // TODO: optimize?
    void UnicodeString::MakeValidFileName()
    {
        UnicodeCharacter underScore('_');
        Replace(UnicodeCharacter('|'), underScore);
        Replace(UnicodeCharacter('^'), underScore);
        Replace(UnicodeCharacter('&'), underScore);
        Replace(UnicodeCharacter('*'), underScore);
        Replace(UnicodeCharacter('{'), underScore);
        Replace(UnicodeCharacter('}'), underScore);
        Replace(UnicodeCharacter(':'), underScore);
        Replace(UnicodeCharacter('?'), underScore);
        Replace(UnicodeCharacter('+'), underScore);
        Replace(UnicodeCharacter('\"'), underScore);
        Replace(UnicodeCharacter('#'), underScore);
        Replace(UnicodeCharacter('%'), underScore);
    }



} // namespace MCore
