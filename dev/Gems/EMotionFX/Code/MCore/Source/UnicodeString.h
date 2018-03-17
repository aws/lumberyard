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

// include required headers
#include <AzCore/std/functional.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector4.h>
#include "StandardHeaders.h"
#include "Array.h"
#include "Vector.h"
#include "AbstractData.h"
#include "UnicodeCharacter.h"

namespace MCore
{
    /**
     * The UTF8 compatible string class.
     * Please keep in mind that this means that the GetLength() function does not always return the number of characters in the string, but the number of code units.
     * You can use the UnicodeStringIterator and UnicodeCharacter classes to iterate over strings.
     */
    class MCORE_API UnicodeString
    {
        MCORE_MEMORYOBJECTCATEGORY(UnicodeString, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_STRING)

    public:
        /**
         * The memory block ID, used inside the memory manager.
         * This will make all strings remain in the same memory blocks, which is more efficient in a lot of cases.
         */
        enum
        {
            MEMORYBLOCK_ID = 1
        };

        UnicodeString()
            : mData(nullptr)
            , mLength(0)
            , mMaxLength(0) { }

        UnicodeString(const char* data)
            : mData(nullptr)
            , mLength(0)
            , mMaxLength(0)
        {
            if (data == nullptr)
            {
                return;
            }
            Alloc((uint32)strlen(data));
            if (mData)
            {
                MemCopy(mData, data, mLength * sizeof(char));
            }
        }

        UnicodeString(const UnicodeString& other)
            : mData(nullptr)
            , mLength(0)
            , mMaxLength(0)
        {
            if (other.mData == nullptr)
            {
                return;
            }
            Alloc(other.GetLength());
            if (mData)
            {
                MCore::MemCopy(mData, other.mData, (other.GetLength() + 1) * sizeof(char));
            }
        }

        UnicodeString(UnicodeString&& other)
        {
            mData = other.mData;
            mLength = other.mLength;
            mMaxLength = other.mMaxLength;
            other.mData = nullptr;
            other.mLength = 0;
            other.mMaxLength = 0;
        }

        /**
         * Creates a string from a single character.
         * @param c The character to put in the string.
         */
        UnicodeString(const char c)
            : mData(nullptr)
            , mLength(0)
            , mMaxLength(0)
        {
            Alloc(1);
            if (mData)
            {
                mData[0] = c;
            }
        }

        UnicodeString(const char* sA, uint32 lengthA, const char* sB, uint32 lengthB);

        /**
         * Creates a string from a given signed integer.
         * @param value The integer to convert into a string.
         */
        explicit UnicodeString(int32 value)
            : mData(nullptr)
            , mLength(0)
            , mMaxLength(0) { FromInt(value); }

        /**
         * Creates a string from a given unsigned integer.
         * @param value The integer to convert into a string.
         */
        explicit UnicodeString(uint32 value)
            : mData(nullptr)
            , mLength(0)
            , mMaxLength(0) { FromInt(value); }

        /**
         * Creates a string from a given float.
         * @param value The float to convert into a string.
         */
        explicit UnicodeString(float value)
            : mData(nullptr)
            , mLength(0)
            , mMaxLength(0) { FromFloat(value); }

        /**
         * Creates a string from a given Vector2.
         * @param[in] value The two component vector to convert into a string, in form of "value,value".
         */
        explicit UnicodeString(const AZ::Vector2& value)
            : mData(nullptr)
            , mLength(0)
            , mMaxLength(0) { FromVector2(value); }

        /**
         * Creates a string from a given Vector3.
         * @param[in] value The three component vector to convert into a string, in form of "value,value,value".
         */
        explicit UnicodeString(const AZ::Vector3& value)
            : mData(nullptr)
            , mLength(0)
            , mMaxLength(0) { FromVector3(value); }

        /**
         * Creates a string from a given Vector4.
         * @param[in] value The four component vector to convert into a string, in form of "value,value,value,value".
         */
        explicit UnicodeString(const AZ::Vector4& value)
            : mData(nullptr)
            , mLength(0)
            , mMaxLength(0) { FromVector4(value); }

        /**
         * Creates a string from a given Matrix.
         * @param[in] value The sixteen component vector to convert into a string, in form of "elem11,elem12,elem13,elem14,elem21,elem22,elem23,elem24,elem31,elem32,elem33,elem34,elem41,elem42,elem43,elem44".
         */
        explicit UnicodeString(const Matrix& value)
            : mData(nullptr)
            , mLength(0)
            , mMaxLength(0) { FromMatrix(value); }

        ~UnicodeString();

        MCORE_INLINE char* GetPtr() const                                               { return (mData != nullptr) ? mData : (char*)""; }
        MCORE_INLINE const char* AsChar() const                                         { return (mData != nullptr) ? mData : (const char*)""; }
        MCORE_INLINE uint32 GetLength() const                                           { return mLength; }

        /**
         * Returns the maximum length of the string character buffer. Note that this is NOT the actual number of characters inside the string.
         * A string can pre-allocate memory to store additional characters. This will reduce the number of allocations or re-allocations being made
         * which increases the performance on strings which are highly dynamic.
         * @result The maximum length of the character buffer excluding the null terminator. If you want to know how many characters the string contains do not use this method, but use the method GetLength()
         */
        MCORE_INLINE uint32 GetMaxLength() const                                        { return mMaxLength; }

        /**
         * Check if the string is empty or not.
         * @result Returns true if the string contains no characters at all. So if it's empty. Otherwise false will be returned.
         */
        MCORE_INLINE bool GetIsEmpty() const                                            { return (mLength == 0); }

        /**
         * Clears all the character data out of the string. This can be done on two ways: by keeping the memory but making the length of the string 0, or deleting all memory of the string as well.
         * @param keepMemory If this parameter is set to 'true', the memory won't be released. However, when 'false' is specified, all allocated memory will be released and the methods GetReadPtr() and GetPtr() will return nullptr.
         */
        MCORE_INLINE void Clear(bool keepMemory = true)
        {
            if (keepMemory)
            {
                mLength = 0;
                if (mData)
                {
                    mData[0] = '\0';
                }
            }
            else
            {
                if (mData)
                {
                    MCore::Free(mData);
                }
                mData = nullptr;
                mLength = 0;
                mMaxLength = 0;
            }
        }

        void FromWChar(const wchar_t* input);
        void FromUTF16(const uint16* input);
        void FromUTF32(const uint32* input);

        AbstractData AsWChar() const;
        AbstractData AsUTF16() const;
        AbstractData AsUTF32() const;
        void AsWChar(AbstractData* output) const;
        void AsUTF16(AbstractData* output) const;
        void AsUTF32(AbstractData* output) const;

        /**
         * Construct the string from the given integer value.
         * @param[in] value The boolean value which will be converted to a text version.
         */
        void FromInt(int32 value);

        /**
         * Construct the string from the given float value.
         * @param[in] value The boolean value which will be converted to a text version.
         */
        void FromFloat(float value);

        /**
         * Construct the string from the given boolean.
         * @param[in] value The boolean value which will be converted to a text version.
         */
        void FromBool(bool value);

        /**
         * Construct the string from the given Vector2.
         * @param[in] value The two component vector which will be converted to a text version.
         */
        void FromVector2(const AZ::Vector2& value);

        /**
         * Construct the string from the given Vector3.
         * @param[in] value The three component vector which will be converted to a text version.
         */
        void FromVector3(const AZ::Vector3& value);

        /**
         * Construct the string from the given Vector4.
         * @param[in] value The four component vector which will be converted to a text version.
         */
        void FromVector4(const AZ::Vector4& value);

        /**
         * Construct the string from the given Matrix.
         * @param[in] value The 16 component matrix which will be converted to a text version.
         */
        void FromMatrix(const Matrix& value);

        /**
         * Convert the string to an integer.
         * Can return weird values when the string cannot be converted correctly!
         * @result The integer value.
         */
        int32 ToInt() const;

        /**
         * Convert the string to a float.
         * Can return weird values when the string cannot be converted correctly!
         * @result The float value.
         */
        float ToFloat() const;

        /**
         * Convert a string to a Vector2.
         * It is assumed that we are dealing with a valid Vector2 string here though. You can verify this with IsValidVector2().
         * The syntax is "value1,value2", for example "-1.5,3.1415". Spaces are allowed as well.
         * @result The Vector2 object.
         */
        AZ::Vector2 ToVector2() const;

        /**
         * Convert a string to a Vector3.
         * It is assumed that we are dealing with a valid Vector3 string here though. You can verify this with IsValidVector3().
         * The syntax is "value1,value2,value3", for example "-1.5,3.1415,+56.7". Spaces are allowed as well.
         * @result The Vector3 object.
         */
        AZ::PackedVector3f ToVector3() const;

        /**
         * Convert a string to a Vector4.
         * It is assumed that we are dealing with a valid Vector3 string here though. You can verify this with IsValidVector4().
         * The syntax is "value1,value,value3,value4", for example "-1.5,3.1415,+14.5,13456.3242". Spaces are allowed as well.
         * @result The Vector4 object.
         */
        AZ::Vector4 ToVector4() const;

        /**
         * Convert a string to a Matrix.
         * It is assumed that we are dealing with a valid Matrix string here though. You can verify this with IsValidMatrix().
         * The syntax is "value1,value2", for example "-1.5,3.1415,+14.5,13456.3242". Spaces are allowed as well.
         * @result The Vector3 object.
         */
        Matrix ToMatrix() const;

        /**
         * Convert the string to a boolean.
         * In case the text in the string cannot be converted correctly, false will be the returned value.
         * Valid strings can be any of the following (non case sensitive): "0", "1", "true", "false", "yes", "no"
         * @result The boolean value.
         */
        bool ToBool() const;

        /**
         * Check if the string can be converted to a valid integer or not.
         * The only requirement is that the string has no empty spaces in it, so the string must be trimmed before conversion.
         * @result Returns true when the string can be converted to an integer, otherwise false is returned.
         */
        bool CheckIfIsValidInt() const;

        /**
         * Check if the string can be converted to a valid float or not.
         * The only requirement is that the string has no empty spaces in it, so the string must be trimmed before conversion.
         * @result Returns true when the string can be converted to a float, otherwise false is returned.
         */
        bool CheckIfIsValidFloat() const;

        /**
         * Check if this string contains a valid boolean.
         * This can be either "0", "1", "true", "false", "yes", "no" (non case sensitive).
         * You can use ToBool() to convert into a boolean.
         * @result Returns true when this is a valid boolean string, otherwise false is returned.
         */
        bool CheckIfIsValidBool() const;

        /**
         * Check if this string contains a valid Vector2 (2 floats).
         * The syntax is "value1,value2", for example "-1.5,3.1415". Spaces are allowed as well.
         * You can use ToVector2() to convert into a Vector2.
         * @result Returns true when this is a valid Vector2 string, otherwise false is returned.
         */
        bool CheckIfIsValidVector2() const;

        /**
         * Check if this string contains a valid Vector3 (3 floats).
         * The syntax is "value1,value2,value3", for example "-1.5,3.1415,+2.0". Spaces are allowed as well.
         * You can use ToVector3() to convert into a Vector3.
         * @result Returns true when this is a valid Vector3 string, otherwise false is returned.
         */
        bool CheckIfIsValidVector3() const;

        /**
         * Check if this string contains a valid Vector4 (4 floats).
         * The syntax is "value1,value2,value3,value4", for example "-1.5,3.1415,1.11,5.6789". Spaces are allowed as well.
         * You can use ToVector4() to convert into a Vector4.
         * @result Returns true when this is a valid Vector4 string, otherwise false is returned.
         */
        bool CheckIfIsValidVector4() const;

        /**
         * Check if this string contains a valid 4x4 Matrix (16 floats).
         * The syntax is "value1,value2,value3,...value16". Spaces are allowed as well.
         * You can use ToMatrix() to convert into a Matrix.
         * @result Returns true when this is a valid Matrix string, otherwise false is returned.
         */
        bool CheckIfIsValidMatrix() const;

        /**
         * Copies a restricted area of one string to another one paying attention to the maximum length.
         * If the string to copy is longer than the maximum length allows it will be cut to the right length.
         * This can make your model not work anymore.
         * @param to The place where the cut string will be copied to.
         * @param from The string which we try to copy.
         * @param maxLength The maximum length the copied string is allowed to be.
         */
        static void RestrictedCopy(char* to, const char* from, uint32 maxLength);

        /**
         * Fills the string on the way printf works.
         * Example: String text; text.Format("And the winning number is: %d !!!", winningNumber);
         * @param text The printf style arguments.
         * @result The current string, containing the text you specified.
         */
        UnicodeString& Format(const char* text, ...);

        /**
         * Adds a given formatted string to the current string data. The parameters of this method work exactly like printf.
         * Example: String text("The winning number "); text.FormatAdd("is: %d", winningNumber);
         * @param text The printf style arguments.
         * @result The current string, with the specified text added to it.
         */
        UnicodeString& FormatAdd(const char* text, ...);

        /**
         * Split the string in seperated parts. The splitting will be done on a given character. The splitting character itself will not be included in any of the resulting parts.
         * Example: String s("Mystic;Game;Development"); s.Split(';'); will result in an array of 3 strings, being: "Mystic", "Game", "Development"
         * @param splitChar The character to split on.
         * @result An array with strings containing the seperated parts. This will be an empty array in case no splitting has been performed (so an array where Array::GetLength() returns 0).
         */
        Array<UnicodeString> Split(const UnicodeCharacter& splitChar = UnicodeCharacter::semiColon) const;

        /**
         * Checks if this string is exactly the same as a given other string. The check is case sensitive!
         * @param other The string to compare with.
         * @result Returns true when both strings are identical, otherwise false is returned.
         */
        bool CheckIfIsEqual(const char* other) const;

        /**
         * Checks if this string is the same as a given other string. The check is NOT case sensitive.
         * @param other The string to compare with.
         * @result Returns true when the lowercase version of both strings are identical, otherwise false is returned.
         */
        bool CheckIfIsEqualNoCase(const char* other) const;

        /**
         * Compare two strings on a case sensitive way. This method works the same way as strcmp of standard C.
         * @param other The other string to compare with.
         * @result This method will return 0 when both strings are equal, -1 when this string is less than the other string, and +1 when this string is greater than the other string. With less and greater we mean the alphabetic order.
         */
        int32 Compare(const char* other) const;

        /**
         * Count the number of given characters.
         * @param character The character to scan for.
         * @result The number of characters as specified as the parameter.
         */
        uint32 CountNumChars(const UnicodeCharacter& character) const;

        /**
         * Compare two strings on a NON case sensitive way. This method works the same way as strcmp of standard C. But then NON case sensitive!
         * @param other The other string to compare with.
         * @result This method will return 0 when both strings are equal, -1 when this string is less than the other string, and +1 when this string is greater than the other string. With less and greater we mean the alphabetic order.
         */
        int32 CompareNoCase(const char* other) const;

        /**
         * Find a substring inside the current string and return it's position (starting at 0). The search is case sensitive!
         * Example: Searching for "Game" in "Mystic Game Development" will result in a return value of 7, because M=0 +"ystic "=6, which means
         * that the G of game starts at position 7.
         * @param subString The substring to search for.
         * @result Returns the offset in the string (starting at 0) of the subString inside the string. If the subString cannot be found, MCORE_INVALIDINDEX32 is returned.
         */
        uint32 Find(const char* subString) const;

        /**
        * Find a substring inside the current string and return it's position (starting at the end of the string, moving its way to the start). The search is case sensitive!
        * Example: Searching for "Game" in "Game of Mystic Game Development" will result in a return value of 15, pointing to the Game word after Mystic.
        * @param subString The substring to search for.
        * @result Returns the offset in the string of the subString inside the string. If the subString cannot be found, MCORE_INVALIDINDEX32 is returned.
        */
        uint32 FindRight(const char* subString) const;

        /**
         * Search for a given character, starting at the right (end) of the string, going back to the beginning of the string.
         * When the character has been found, its index will be returned. In case no such character can be located in the string a value of MCORE_INVALIDINDEX32 will be return.
         * @param character The character to search for.
         * @result The zero based index (character number) in the string that contains this character. A value of MCORE_INVALIDINDEX32 will be returned when the character isn't inside the string.
         */
        uint32 FindRight(const UnicodeCharacter& character) const;

        /**
         * Checks if this string contains a given substring. The search is case sensitive!
         * @param subString The substring you want to look for inside this string.
         * @result Returns true when the substring is located inside the string, otherwise false us returned.
         */
        MCORE_INLINE bool Contains(const char* subString) const                     { return (Find(subString) != MCORE_INVALIDINDEX32); }

        /**
         * Removes the line ends from the string.
         * This essentially removes any \n and \r at the end.
         */
        void RemoveLineEnd();

        /**
         * Makes sure the line ends at \n and not \n\r for example.
         */
        void FixLineEnd();

        /**
         * Fixes invalid characters inside filenames.
         * This changes the characters "|^&*{}:?+"#%" in an underscore character "_".
         * The file name is allowed to have backslashes or slashes inside it, so it can be a full path.
         */
        void MakeValidFileName();

        /**
         * Calculates the number of words inside this string. Both spaces and tabs can be the seperators.
         * @result The number of words this string contains.
         */
        uint32 CalcNumWords() const;

        /**
         * Extracts a given word number.
         * @param wordNr The word number to extract. This must be in range of [0..CalcNumWords()-1]
         * @result Returns the extracted word.
         */
        UnicodeString ExtractWord(uint32 wordNr) const;

        /**
         * Removes the first occurrence of a given substring. This can be used if you want to remove for example "super" from "Very supercool thing".
         * In that case, the remaining string after executing this method, would be "Very cool thing".
         * @param part The part of the string you want to remove. Note that it will try to remove only the first occurrence of this substring.
         * @result Returns true when successfully removed the part from the string, otherwise returns false (will happen when the given substring wasn't found, so nothing could be removed).
         */
        bool RemoveFirstPart(const char* part);

        /**
         * Removes all occurring substrings from the string. This works the same as the method RemoveFirstPart, but with the difference that it will not
         * only remove the first occurrence, but all occurrences. Implemented as iterative algorithm, which keeps trying to remove the given substring until
         * no occurrences are found anymore.
         * For an example:
         * @see RemoveFirstPart
         * @param part The substring you want to remove. Note, that ALL occurrences will be removed!
         * @result Returns true when all substrings have been removed, otherwise false is returned. This will happen when the given substring cannot be found in the string.
         */
        bool RemoveAllParts(const char* part);

        /**
         * Removes a given set of characters from the string.
         * Example: a charSet containing the characters 'y', 'c' and 'e' performed on the string "Mystic Game Development" will result in "Msti Gam Dvlopmnt"
         * @param charSet the set of characters to remove from the string. All occurrences will be removed.
         * @result Returns true when successfully performed the opertion, otherwise false is returned. This happens when none of the characters in the set could be found.
         */
        bool RemoveChars(const Array<UnicodeCharacter>& charSet);

        /**
         * Remove the given collection of characters from the string.
         * If this string contains "FooBar" and we pass as parameter to this function the string "oa" it will remove all the 'o' and 'a' characters from the string.
         * In that case the resulting string will be "FBr".
         * @param characterSet The string containing the set of characters to be removed.
         * @result Returns true when successfully performed the opertion, otherwise false is returned. This happens when none of the characters in the set could be found.
         */
        bool RemoveChars(const char* characterSet);

        /**
         * Generate a unique string using a given prefix.
         * After executing this method the current string will contain the unique resulting string.
         * You specify a validation function, which is used to test if the string is unique or not. The function should return true when the value passed as parameter is unique, and false if it is not unique.
         * Internally the function will continue to generate new strings until a unique one has been found. It does this by adding a number behind the prefix string, such as: "String0", "String1", "String2".
         * @param prefix The prefix of the string part, for example "String".
         * @param validationFunction The function which tests if a given string is unique or not. Return true when the string is unique (and should be the one used) and return false when the passed string is not a valid (unique) one.
         */
        void GenerateUniqueString(const char* prefix, const AZStd::function<bool(const MCore::UnicodeString& value)>& validationFunction);

        /**
         * Remove the extension from a string containing a filename.
         * This removes all contents after the last dot inside the string.
         * For example the string "TopSecret.pdf" would result in "TopSecret", while the string "TopSecret.Info.doc" would result
         * in "TopSecret.Info".
         */
        void RemoveFileExtension();

        /**
         * Extract the file name from the string that contains some file.
         * For example calling this method on a string containing: "c:\somepath\another\cool.bmp" will return a string
         * containing "cool.bmp".
         * It is also possible to use paths like "c:/somepath/another/cool.bmp".
         * When the string contains no path but for example the text "hello" then the return value will return a copy of itself, so "hello" in this case.
         * The same is returned when there is no dot inside the file name.
         * @result The file name, for example "cool.bmp" when the string contains "c:\somepath\another\cool.bmp".
         */
        UnicodeString ExtractFileName() const;

        /**
         * Extract the path from the string that contains some file.
         * For example calling this method on a string containing: "c:\somepath\another\cool.bmp" will return a string
         * containing "c:\somepath\another\".
         * It is also possible to use paths like "c:/somepath/another/cool.bmp".
         * When the string contains no path but for example the text "hello" then the return value will be a copy of itself, so "hello" in this case.
         * @param includeSlashAtEnd When set to true, the a backslash or forward slash will be added to the returned path.
         * @result The file name, for example "c:\somepath\another\" when the string contains "c:\somepath\another\cool.bmp".
         */
        UnicodeString ExtractPath(bool includeSlashAtEnd = true) const;

        /**
         * Extract the extension from a file name.
         * For example when the string contains "SomeFile.doc" it will return "doc".
         * When no extension is there, an empty string is returned.
         * In case of a string like "FileName.temp.doc" the string "doc" is returned as well.
         * @result The file extension or an empty string when no extension is there.
         */
        UnicodeString ExtractFileExtension() const;

        /**
         * Extract a version of this string, which has to represent a full path to a folder or filename, but made relative to another given folder.
         * For example if this string contains the value "c:/mygame/assets/dragon/dragon.txt" and the relativeToFolder specified is "c:/mygame", then the
         * function will return "assets/dragon/dragon.txt". Another example would be if this string contains "c:/mygame/info.txt" and the relativeToFolder
         * is set to "c:/mygame/assets/dragon", in which case the resulting string will be "../../info.txt".
         * @param relativeToFolder The folder we want the resulting string to be made relative to.
         * @param removeLeadingFolderSeparators When set to true it makes sure the resulting string will not start with a forwardslash or backslash character.
         * @param removeLastFolderSeparators When set to true it makes sure the resulting string does not end with a forwardslash or backslash.
         * @result The version of the current string that represents a file or folder path, but made relative to the specified folder.
         */
        UnicodeString ExtractPathRelativeTo(const UnicodeString& relativeToFolder, bool removeLeadingFolderSeparators = true, bool removeLastFolderSeparators = true) const;

        /**
         * Fix double forward slashes and double backslashes inside file paths.
         * This will turn something like "c:\foo\\bar" into "c:\foo\bar".
         * If you set allowDoubleAtStart to true it will allow a double slash or backslash at the start of the string.
         * In that case "\\computername\foo\\bar" will be turned into "\\computername\foo\bar". If it is set to false it would become "\computername\foo\bar".
         * @param allowDoubleAtStart Set to true if a double backslash or double slash is allowed at the start of the string.
         */
        void FixPathDoubleSlashes(bool allowDoubleAtStart = true);

        /**
         * Remove the last character if it is equal to a given character.
         * If the last character in the string is not equal to the specified character nothing will happen.
         * @param lastCharacter The character to remove if this is also the last one in the string.
         */
        void RemoveLastCharacterIfEqualTo(const UnicodeCharacter& lastCharacter);

        /**
         * Remove the first character if it is equal to a given character.
         * If the first character in the string is not equal to the specified character nothing will happen.
         * @param firstCharacter The character to remove if this is also the first one in the string.
         */
        void RemoveFirstCharacterIfEqualTo(const UnicodeCharacter& firstCharacter);

        /**
         * Removes all the first spaces (unless another character is specified) on the left of the string.
         * Example: Performing this operation on the string "     Mystic     " would result in the string "Mystic     ".
         * @param trimChar The character to trim. The default is an empty space.
         */
        void TrimLeft(const UnicodeCharacter& trimChar = UnicodeCharacter::space);

        /**
         * Removes all the first spaces (unless another character is specified) on the right of the string.
         * Example: Performing this operation on the string "     Mystic     " would result in the string "     Mystic".
         * @param trimChar The character to trim. The default is an empty space.
         */
        void TrimRight(const UnicodeCharacter& trimChar = UnicodeCharacter::space);

        /**
         * Removes all the first spaces (unless another character is specified) on both the left and right side of the string.
         * Example: Performing this operation on the string "     Mystic     " would result in the string "Mystic".
         * @param trimChar The character to trim. The default is an empty space.
         */
        void Trim(const UnicodeCharacter& trimChar = UnicodeCharacter::space);

        /**
         * Replace a given piece of text inside the string with another piece of text. This replaces all occurrences.
         * Imagine you have a string that contains: "Hello %NAME% how are you?" and you wish to replace the %NAME% part with
         * for example "John". You can do this by calling: string.Replace("%NAME%", "John").
         * Please note that this method replaces all occurrences of the given key value (%NAME% in our example).
         * @param what The piece of text to replace.
         * @param with The piece of text to replace it with.
         */
        void Replace(const char* what, const char* with);

        /**
        * Replace a given piece of text inside the string with another piece of text, but only replace the first occurrence.
        * Imagine you have a string that contains: "Hello %NAME% how are you?" and you wish to replace the %NAME% part with
        * for example "John". You can do this by calling: string.Replace("%NAME%", "John").
        * Please note that this method replaces all occurrences of the given key value (%NAME% in our example).
        * @param what The piece of text to replace.
        * @param with The piece of text to replace it with.
        */
        void ReplaceFirst(const char* what, const char* with);

        /**
        * Replace a given piece of text inside the string with another piece of text, but only replace the last occurrence.
        * Imagine you have a string that contains: "Hello %NAME% how are you?" and you wish to replace the %NAME% part with
        * for example "John". You can do this by calling: string.Replace("%NAME%", "John").
        * Please note that this method replaces all occurrences of the given key value (%NAME% in our example).
        * @param what The piece of text to replace.
        * @param with The piece of text to replace it with.
        */
        void ReplaceLast(const char* what, const char* with);

        /**
         * Replace a given character inside the string with another character.
         * Imagine you have a string that contains: "abc" and you wish to replace the 'a' with for example a 'c'
         * You can do this by calling: string.Replace('a', 'c'). The result of that will be "cbc" then.
         * Please note that this method replaces all occurrences of the given character.
         * @param c The character to be replaced.
         * @param with The character to replace it with.
         */
        void Replace(const UnicodeCharacter& c, const UnicodeCharacter& with);

        /**
         * Replace spaces with non-breaking spaces in html syntax to avoid unwanted line breaks.
         */
        void ConvertToNonBreakingHTMLSpaces();

        /**
         * Insert a given string inside this string.
         * @param index The zero-based index where to insert the text string. This must be in range of [0..GetLength()-1].
         * @param text The text to insert inside this string.
         */
        void Insert(uint32 index, const char* text);

        /**
         * Resize the string to a given length.
         * @param length The new string length.
         * @param fillChar In case we are growing the string, init the new characters to this character.
         * @param doFill Set to true if you want to fill the newly allocated space with the character, or set to false when not
         */
        void Resize(uint32 length, char fillChar = ' ', bool doFill = true);

        /**
         * Pre-alloc space in the string, up to a given length.
         * If the preserve-length passed as parameter to this method is smaller than the maximum length that is already allocated space for
         * then nothing happens. So the string is never being made smaller to prevent memory reallocs.
         * @param length The number of characters to reserve space for.
         */
        void Reserve(uint32 length);

        /**
         * Returns an uppercase version of this string.
         * Example: performing this operation on the string "Mystic Game Development" would return a string containing "MYSTIC GAME DEVELOPMENT".
         * @result The uppercase version of the string.
         */
        UnicodeString Uppered() const;

        /**
         * Returns a lowercase version of this string.
         * Example: performing this operation on the string "Mystic Game Development" would return a string containing "mystic game development".
         * @result The lowercase version of the string.
         */
        UnicodeString Lowered() const;

        /**
         * Convert this string to uppercase.
         * @result The same string, but now in uppercase.
         */
        UnicodeString& ToUpper();

        /**
         * Convert this string to lowercase.
         * @result The same string, but now in lowercase.
         */
        UnicodeString& ToLower();

        /**
         * Align this string to a given length.
         * This makes sure that the length of the string will be the specified stringLength number of characters.
         * If the string is already longer or equal to the specified string length, then nothing will happen.
         * If you have a string named "Hi" and you call Align(5, '.') on this string, the result would be: "Hi...".
         * @param newNumCharacters The minimum length of the string, so the number of characters to align to.
         * @param character The character to fill the characters to be added with.
         * @result Returns a reference to the same string, but now after alignment has been applied to it.
         */
        UnicodeString& Align(uint32 newNumCharacters, const UnicodeCharacter& character = UnicodeCharacter::space);

        /**
         * Elide a string on the left. This shortens the string if it is too long, and makes the string start with a given fill text.
         * For example the string "verylongstring" could be turned into something like "...ngstring" when calling this function.
         * The maximum number of characters can be specified. In the above example this would have a value of 11, as it shows 11 characters, including the fill text.
         * @param maxNumChars The maximum number of characters in the elided string, this includes the fill text characters.
         * @param fillText The characters to use at the beginning of the string, in case the string will be elided.
         */
        void ElideLeft(uint32 maxNumChars, const char* fillText = "...");

        /**
         * Returns a left elided copy of this string.
         * Left eliding shortens the string if it is too long, and makes the string start with a given fill text.
         * For example the string "verylongstring" could be turned into something like "...ngstring" when calling this function.
         * The maximum number of characters can be specified. In the above example this would have a value of 11, as it shows 11 characters, including the fill text.
         * @param maxNumChars The maximum number of characters in the elided string, this includes the fill text characters.
         * @param fillText The characters to use at the beginning of the string, in case the string will be elided.
         */
        UnicodeString ElidedLeft(uint32 maxNumChars, const char* fillText = "...") const;

        /**
         * Elide a string on the right. This shortens the string if it is too long, and makes the string end with a given fill text.
         * For example the string "verylongstring" could be turned into something like "verylong..." when calling this function.
         * The maximum number of characters can be specified. In the above example this would have a value of 11, as it shows 11 characters, including the fill text characters.
         * @param maxNumChars The maximum number of characters in the elided string, this includes the fill text characters.
         * @param fillText The characters to use at the end of the string, in case the string will be elided.
         */
        void ElideRight(uint32 maxNumChars, const char* fillText = "...");

        /**
         * Returns a right elided copy of this string. Eliding shortens the string if it is too long, and makes the string end with a given fill text.
         * For example the string "verylongstring" could be turned into something like "verylong..." when calling this function.
         * The maximum number of characters can be specified. In the above example this would have a value of 11, as it shows 11 characters, including the fill text characters.
         * @param maxNumChars The maximum number of characters in the elided string, this includes the fill text characters.
         * @param fillText The characters to use at the end of the string, in case the string will be elided.
         */
        UnicodeString ElidedRight(uint32 maxNumChars, const char* fillText = "...") const;

        /**
         * Elide a string in the middle. This shortens the string if it is too long, and makes the center of the string filled with a given fill text.
         * For example the string "verylongstring" could be turned into something like "very...ring" when calling this function.
         * The maximum number of characters can be specified. In the above example this would have a value of 11, as it shows 11 characters, including the fill text characters.
         * @param maxNumChars The maximum number of characters in the elided string, this includes the fill text characters.
         * @param fillText The characters to use in the center of the string, in case the string will be elided.
         */
        void ElideMiddle(uint32 maxNumChars, const char* fillText = "...");

        /**
         * Returns a middle elided copy of this string. Eliding shortens the string if it is too long, and makes the center of the string filled with a given fill text.
         * For example the string "verylongstring" could be turned into something like "very...ring" when calling this function.
         * The maximum number of characters can be specified. In the above example this would have a value of 11, as it shows 11 characters, including the fill text characters.
         * @param maxNumChars The maximum number of characters in the elided string, this includes the fill text characters.
         * @param fillText The characters to use in the center of the string, in case the string will be elided.
         */
        UnicodeString ElidedMiddle(uint32 maxNumChars, const char* fillText = "...") const;

        UnicodeString& Copy(const char* what, uint32 length);

        // element access operators
        //MCORE_INLINE wchar_t& operator[](int32 nr)                                        { return mData[nr]; }
        //MCORE_INLINE const wchar_t& operator[](int32 nr) const                            { return mData[nr]; }
        //MCORE_INLINE wchar_t* operator()(int32 nr)                                        { return (wchar_t*)(mData + nr); }

        MCORE_INLINE operator const char*() const                                       { return (mData != nullptr) ? mData : (const char*)""; }

        // copy operators
        MCORE_INLINE const UnicodeString&   operator=(const UnicodeString& str)
        {
            MCORE_ASSERT(&str != this);
            if (&str == this)
            {
                return *this;
            }
            return Copy(str.AsChar(), str.GetLength());
        }
        MCORE_INLINE const UnicodeString&   operator=(UnicodeString&& str)
        {
            MCORE_ASSERT(&str != this);
            if (mData)
            {
                Free(mData);
            }
            mData = str.mData;
            mLength = str.mLength;
            mMaxLength = str.mMaxLength;
            str.mMaxLength = 0;
            str.mLength = 0;
            str.mData = nullptr;
            return *this;
        }
        MCORE_INLINE const UnicodeString&   operator=(char c)                           { return Copy(&c, 1); }
        MCORE_INLINE const UnicodeString&   operator=(const char* str)
        {
            if (mData == str)
            {
                return *this;
            }
            if (str != nullptr)
            {
                return Copy(str, (uint32)strlen(str));
            }
            else
            {
                Clear();
            } return *this;
        }

        // append operators
        MCORE_INLINE UnicodeString  operator+(const char* str)
        {
            if (str == nullptr)
            {
                return *this;
            }
            UnicodeString s(*this);
            return s.Concat(str, (uint32)strlen(str));
        }
        MCORE_INLINE UnicodeString  operator+(const UnicodeString& str)                 { UnicodeString s(*this); return s.Concat(str.AsChar(), str.GetLength()); }
        MCORE_INLINE UnicodeString  operator+(int32 val)                                { UnicodeString s(*this); UnicodeString str(val); return s.Concat(str.AsChar(), str.GetLength()); }
        MCORE_INLINE UnicodeString  operator+(float val)                                { UnicodeString s(*this); UnicodeString str(val); return s.Concat(str.AsChar(), str.GetLength()); }
        //MCORE_INLINE UnicodeString    operator+(double val)                               { UnicodeString s(*this); UnicodeString str(val); return s.Concat(str.AsChar(), str.GetLength()); }
        MCORE_INLINE UnicodeString  operator+(char c)                                   { UnicodeString s(*this); return s.Concat(&c, 1); }

        MCORE_INLINE const UnicodeString& operator+=(const char* str)
        {
            if (str == nullptr)
            {
                return *this;
            }
            return Concat(str, (uint32)strlen(str));
        }
        MCORE_INLINE const UnicodeString& operator+=(const UnicodeString& str)          { return Concat(str.AsChar(), str.GetLength()); }
        MCORE_INLINE const UnicodeString& operator+=(char c)                            { return Concat(&c, 1); }
        MCORE_INLINE const UnicodeString& operator+=(const UnicodeCharacter& c)         { uint32 numBytes; char buffer[8]; c.AsUTF8(buffer, &numBytes, false); return Concat(buffer, numBytes); }
        MCORE_INLINE const UnicodeString& operator+=(int32 val)                         { UnicodeString str(val); return Concat(str.AsChar(), str.GetLength()); }
        MCORE_INLINE const UnicodeString& operator+=(float val)                         { UnicodeString str(val); return Concat(str.AsChar(), str.GetLength()); }
        //      MCORE_INLINE const UnicodeString& operator+=(double val)                        { UnicodeString str(val); return Concat(str.AsChar(), str.GetLength()); }

        // compare operators
        MCORE_INLINE bool   operator< (const UnicodeString& str)                        { return (SafeCompare(mData, str.AsChar())      <  0); }
        MCORE_INLINE bool   operator< (const char*          str)                        { return (SafeCompare(mData, str)               <  0); }
        MCORE_INLINE bool   operator> (const UnicodeString& str)                        { return (SafeCompare(mData, str.AsChar())      >  0); }
        MCORE_INLINE bool   operator> (const char*          str)                        { return (SafeCompare(mData, str)               >  0); }
        MCORE_INLINE bool   operator<=(const UnicodeString& str)                        { return (SafeCompare(mData, str.AsChar())      <= 0); }
        MCORE_INLINE bool   operator<=(const char*          str)                        { return (SafeCompare(mData, str)               <= 0); }
        MCORE_INLINE bool   operator>=(const UnicodeString& str)                        { return (SafeCompare(mData, str.AsChar())      >= 0); }
        MCORE_INLINE bool   operator>=(const char*          str)                        { return (SafeCompare(mData, str)               >= 0); }
        MCORE_INLINE bool   operator==(const UnicodeString& str)                        { return (SafeCompare(mData, str.AsChar())      == 0); }
        MCORE_INLINE bool   operator==(const char*          str)                        { return (SafeCompare(mData, str)               == 0); }
        MCORE_INLINE bool   operator!=(const UnicodeString& str)                        { return (SafeCompare(mData, str.AsChar())      != 0); }
        MCORE_INLINE bool   operator!=(const char*          str)                        { return (SafeCompare(mData, str)               != 0); }

        static MCORE_INLINE int32 SafeCompare(const char* strA, const char* strB)
        {
            // if both strings exist
            if (strA && strB)
            {
                return strcmp(strA, strB);
            }

            // if one of them is nullptr
            if (strA == nullptr)
            {
                if (strB)
                {
                    if (strB[0] == 0)
                    {
                        return 0;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else // both strA and strB are nullptr
                {
                    return 0;
                }
            }
            else    // strA not nullptr, strB must be nullptr
            {
                if (strA[0] == 0)
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
        }


    private:
        char*       mData;
        uint32      mLength;        // num code units
        uint32      mMaxLength;

        UnicodeString& Concat(const char* what, uint32 length);         // append two strings
        void Alloc(uint32 numCodeUnits, uint32 extraUnits = 0);

        /**
         * Convert slashes of a file path to the natively used ones on the used platform.
         * For example "C:/MyFolder/myFile.txt" will be converted to "C:\MyFolder\myFile.txt" when running windows.
         */
        void ConvertToNativePath();

        /**
         * Copy the string, convert slashes of a file path to the natively used ones on the used platform and return the copy.
         * For example "C:/MyFolder/myFile.txt" will be converted to "C:\MyFolder\myFile.txt" when running windows.
         * @return The converted file path.
         */
        UnicodeString ConvertedToNativePath() const;
    };


    // external operators
    MCORE_INLINE UnicodeString operator+ (const UnicodeString&  strA, const UnicodeString&  strB)           { return UnicodeString(strA.AsChar(), strA.GetLength(), strB.AsChar(), strB.GetLength()); }
    MCORE_INLINE UnicodeString operator+ (const UnicodeString&  strA, char                  strB)           { return UnicodeString(strA.AsChar(), strA.GetLength(), &strB, 1); }
    MCORE_INLINE UnicodeString operator+ (const char            strA, const UnicodeString&  strB)           { return UnicodeString(&strA, 1, strB.AsChar(), strB.GetLength()); }
    MCORE_INLINE UnicodeString operator+ (const UnicodeString&  strA, const char*           strB)
    {
        if (strB)
        {
            return UnicodeString(strA.AsChar(), strA.GetLength(), strB, (uint32)strlen(strB));
        }
        else
        {
            return strA;
        }
    }
    MCORE_INLINE UnicodeString operator+ (const char*           strA, const UnicodeString&  strB)
    {
        if (strA)
        {
            return UnicodeString(strA, (uint32)strlen(strA), strB.AsChar(), strB.GetLength());
        }
        else
        {
            return strB;
        }
    }

    MCORE_INLINE bool   operator< (const UnicodeString& strA, const UnicodeString&  strB)                   { return (UnicodeString::SafeCompare(strA.AsChar(), strB.AsChar()) < 0); }
    MCORE_INLINE bool   operator< (const UnicodeString& strA, const char*           strB)                   { return (UnicodeString::SafeCompare(strA.AsChar(), strB) < 0); }
    MCORE_INLINE bool   operator< (const char*          strA, const UnicodeString&  strB)                   { return (UnicodeString::SafeCompare(strA, strB.AsChar()) < 0); }
    MCORE_INLINE bool   operator> (const UnicodeString& strA, const UnicodeString&  strB)                   { return (UnicodeString::SafeCompare(strA.AsChar(), strB.AsChar()) > 0); }
    MCORE_INLINE bool   operator> (const UnicodeString& strA, const char*           strB)                   { return (UnicodeString::SafeCompare(strA.AsChar(), strB) > 0); }
    MCORE_INLINE bool   operator> (const char*          strA, const UnicodeString&  strB)                   { return (UnicodeString::SafeCompare(strA, strB.AsChar()) > 0); }
    MCORE_INLINE bool   operator<=(const UnicodeString& strA, const UnicodeString&  strB)                   { return (UnicodeString::SafeCompare(strA.AsChar(), strB.AsChar()) <= 0); }
    MCORE_INLINE bool   operator<=(const UnicodeString& strA, const char*           strB)                   { return (UnicodeString::SafeCompare(strA.AsChar(), strB) <= 0); }
    MCORE_INLINE bool   operator<=(const char*          strA, const UnicodeString&  strB)                   { return (UnicodeString::SafeCompare(strA, strB.AsChar()) <= 0); }
    MCORE_INLINE bool   operator>=(const UnicodeString& strA, const UnicodeString&  strB)                   { return (UnicodeString::SafeCompare(strA.AsChar(), strB.AsChar()) >= 0); }
    MCORE_INLINE bool   operator>=(const UnicodeString& strA, const char*           strB)                   { return (UnicodeString::SafeCompare(strA.AsChar(), strB) >= 0); }
    MCORE_INLINE bool   operator>=(const char*          strA, const UnicodeString&  strB)                   { return (UnicodeString::SafeCompare(strA, strB.AsChar()) >= 0); }
    MCORE_INLINE bool   operator==(const UnicodeString& strA, const UnicodeString&  strB)                   { return (UnicodeString::SafeCompare(strA.AsChar(), strB.AsChar()) == 0); }
    MCORE_INLINE bool   operator==(const UnicodeString& strA, const char*           strB)                   { return (UnicodeString::SafeCompare(strA.AsChar(), strB) == 0); }
    MCORE_INLINE bool   operator==(const char*          strA, const UnicodeString&  strB)                   { return (UnicodeString::SafeCompare(strA, strB.AsChar()) == 0); }
    MCORE_INLINE bool   operator!=(const UnicodeString& strA, const UnicodeString&  strB)                   { return ((strA.GetLength() != strB.GetLength()) || (UnicodeString::SafeCompare(strA.AsChar(), strB.AsChar()) != 0)); }
    MCORE_INLINE bool   operator!=(const UnicodeString& strA, const char*           strB)                   { return (UnicodeString::SafeCompare(strA.AsChar(), strB) != 0); }
    MCORE_INLINE bool   operator!=(const char*          strA, const UnicodeString&  strB)                   { return (UnicodeString::SafeCompare(strA, strB.AsChar()) != 0); }

    // some shortcut
    typedef UnicodeString String;   /**< The string type, which is a UnicodeString. */


    class MCORE_API StringRef
    {
    public:
        MCORE_INLINE StringRef(const char* text)                    { mData = const_cast<char*>(text); }
        MCORE_INLINE const char* AsChar() const                     { return mData; }
        MCORE_INLINE uint32 GetLength() const                       { return (uint32)strlen(mData); }

        MCORE_INLINE bool operator==(const StringRef& other)        { return (strcmp(mData, other.mData) == 0); }
        MCORE_INLINE bool operator!=(const StringRef& other)        { return (strcmp(mData, other.mData) != 0); }

    private:
        char* mData;
    };
}   // namespace MCore
