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
#include <ctype.h>

#include <AzCore/std/string/conversions.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/IO/SystemFile.h> // AZ_MAX_PATH_LEN
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/AzCore_Traits_Platform.h>

#if !AZ_TRAIT_USE_WINDOWS_FILE_API

//Have to declare this typedef for Android & Linux to minimize changes elsewhere in this file
#if AZ_TRAIT_USE_ERRNO_T_TYPEDEF
typedef int errno_t;
#endif

void ClearToEmptyStr(char* buffer)
{
    if (buffer != nullptr)
    {
        *buffer = '\0';
    }
}

// Microsoft defines _splitpath_s but no one else so define it ourselves
errno_t _splitpath_s (const char* path,
    char* drive, size_t driveBufferSize,
    char* dir, size_t dirBufferSize,
    char* fname, size_t nameBufferSize,
    char* ext, size_t extBufferSize)
{
    // Defined the same value error values as windows...
#undef EINVAL
#undef ERANGE
    static const errno_t EINVAL = 22;
    static const errno_t ERANGE = 34;

    // Error checking first
    if ((path == nullptr) ||
        (drive == nullptr && driveBufferSize > 0) || (drive != nullptr && driveBufferSize == 0) ||
        (dir == nullptr && dirBufferSize > 0) || (dir != nullptr && dirBufferSize == 0) ||
        (fname == nullptr && nameBufferSize > 0) || (fname != nullptr && nameBufferSize == 0) ||
        (ext == nullptr && extBufferSize > 0) || (ext != nullptr && extBufferSize == 0))
    {
        return EINVAL;
    }

    // Clear all output buffers
    ClearToEmptyStr(drive);
    ClearToEmptyStr(dir);
    ClearToEmptyStr(fname);
    ClearToEmptyStr(ext);

    const char* lastBackSlashLocation = strrchr(path, '\\');
    if (lastBackSlashLocation == nullptr)
    {
        lastBackSlashLocation = path;
    }

    const char* lastForwardSlashLocation = strrchr(path, '/');
    if (lastForwardSlashLocation == nullptr)
    {
        lastForwardSlashLocation = path;
    }

    const char* lastPathSeparatorLocation = AZStd::max(lastBackSlashLocation, lastForwardSlashLocation);
    const char* fileNameLocation = lastPathSeparatorLocation != path ?
        lastPathSeparatorLocation + 1 :
        lastPathSeparatorLocation;

    const char* pathEndLocation = path + strlen(path);
    const char* extensionLocation = strrchr(path, '.');
    if (extensionLocation == nullptr ||
        extensionLocation < lastPathSeparatorLocation)
    {
        // No extension
        extensionLocation = pathEndLocation;
    }

    if (ext != nullptr)
    {
        const size_t extLength = pathEndLocation - extensionLocation;
        if (extLength >= extBufferSize) // account for null terminator
        {
            return ERANGE;
        }
        azstrcpy(ext, extBufferSize, extensionLocation);
    }

    if (extensionLocation == path)
    {
        // The entire path is the extension
        return 0;
    }

    if (fname != nullptr)
    {
        const size_t fileNameLength = extensionLocation - fileNameLocation;
        if (fileNameLength >= nameBufferSize) // account for null terminator
        {
            // Clear buffers set previously to maintain consistency with msvc implementation:
            // https://msdn.microsoft.com/en-us/library/8e46eyt7.aspx
            ClearToEmptyStr(ext);
            return ERANGE;
        }
        azstrncpy(fname, nameBufferSize, fileNameLocation, fileNameLength);
        fname[fileNameLength] = '\0';
    }

    if (fileNameLocation == path)
    {
        // The entire path is the filename (+ possible extension handled above)
        return 0;
    }

    if (dir != nullptr)
    {
        const size_t dirLength = fileNameLocation - path;
        if (dirLength >= dirBufferSize) // account for null terminator
        {
            // Clear buffers set previously to maintain consistency with msvc implementation:
            // https://msdn.microsoft.com/en-us/library/8e46eyt7.aspx
            ClearToEmptyStr(ext);
            ClearToEmptyStr(fname);
            return ERANGE;
        }
        azstrncpy(dir, dirBufferSize, path, dirLength);
        dir[dirLength] = '\0';
    }

    // 'drive' was already set to an empty string above.
    // While windows is the only supported platform with
    // the concept of drives, not sure whether we expect
    // input of the form C:\dir\filename.ext to function
    // the same as it would on windows?

    return 0;
}
#endif

// Some platforms define NAME_MAX but Windows doesn't and the AZ headers provide no equivalent
#define MAX_NAME_LEN 255

namespace AZ
{
    namespace StringFunc
    {
        bool Equal(const char* inA, const char* inB, bool bCaseSensitive /*= false*/, size_t n /*= 0*/)
        {
            if (!inA || !inB)
            {
                return false;
            }

            if (inA == inB)
            {
                return true;
            }

            if (bCaseSensitive)
            {
                if (n)
                {
                    return !strncmp(inA, inB, n);
                }
                else
                {
                    return !strcmp(inA, inB);
                }
            }
            else
            {
                if (n)
                {
                    return !azstrnicmp(inA, inB, n);
                }
                else
                {
                    return !azstricmp(inA, inB);
                }
            }
        }

        bool StartsWith(AZStd::string_view sourceValue, AZStd::string_view prefixValue, bool bCaseSensitive)
        {
            return sourceValue.size() >= prefixValue.size()
                && Equal(sourceValue.data(), prefixValue.data(), bCaseSensitive, prefixValue.size());
        }

        bool EndsWith(AZStd::string_view sourceValue, AZStd::string_view suffixValue, bool bCaseSensitive)
        {
            return sourceValue.size() >= suffixValue.size()
                && Equal(sourceValue.substr(sourceValue.size() - suffixValue.size(), AZStd::string_view::npos).data(), suffixValue.data(), bCaseSensitive, suffixValue.size());
        }

        size_t Find(const char* in, char c, size_t pos /*= 0*/, bool bReverse /*= false*/, bool bCaseSensitive /*= false*/)
        {
            if (!in)
            {
                return AZStd::string::npos;
            }

            if (pos == AZStd::string::npos)
            {
                pos = 0;
            }

            size_t inLen = strlen(in);
            if (inLen < pos)
            {
                return AZStd::string::npos;
            }

            if (!bCaseSensitive)
            {
                c = (char)tolower(c);
            }

            if (bReverse)
            {
                pos = inLen - pos - 1;
            }

            char character;

            do
            {
                if (!bCaseSensitive)
                {
                    character = (char)tolower(in[pos]);
                }
                else
                {
                    character = in[pos];
                }

                if (character == c)
                {
                    return pos;
                }

                if (bReverse)
                {
                    pos = pos > 0 ? pos-1 : pos;
                }
                else
                {
                    pos++;
                }
            } while (bReverse ? pos : character != '\0');

            return AZStd::string::npos;
        }

        size_t Find(AZStd::string_view in, AZStd::string_view s, size_t offset /*= 0*/, bool bReverse /*= false*/, bool bCaseSensitive /*= false*/)
        {
            if (in.empty() || s.empty())
            {
                return AZStd::string::npos;
            }

            const size_t inlen = in.size();
            const size_t slen = s.size();

            if (offset == AZStd::string::npos)
            {
                offset = 0;
            }

            if (offset + slen > inlen)
            {
                return AZStd::string::npos;
            }

            const char* pCur;

            if (bReverse)
            {
                // Start at the end (- pos)
                pCur = in.data() + inlen - slen - offset;
            }
            else
            {
                // Start at the beginning (+ pos)
                pCur = in.data() + offset;
            }

            do
            {
                if (bCaseSensitive)
                {
                    if (!strncmp(pCur, s.data(), slen))
                    {
                        return static_cast<size_t>(pCur - in.data());
                    }
                }
                else
                {
                    if (!azstrnicmp(pCur, s.data(), slen))
                    {
                        return static_cast<size_t>(pCur - in.data());
                    }
                }

                if (bReverse)
                {
                    pCur--;
                }
                else
                {
                    pCur++;
                }
            } while (bReverse ? pCur >= in.data() : pCur - in.data() <= static_cast<ptrdiff_t>(inlen));

            return AZStd::string::npos;
        }

        char FirstCharacter(const char* in)
        {
            if (!in)
            {
                return '\0';
            }
            if (in[0] == '\n')
            {
                return '\0';
            }
            return in[0];
        }

        char LastCharacter(const char* in)
        {
            if (!in)
            {
                return '\0';
            }
            size_t len = strlen(in);
            if (!len)
            {
                return '\0';
            }
            return in[len - 1];
        }

        AZStd::string& Append(AZStd::string& inout, const char s)
        {
            return inout.append(1, s);
        }

        AZStd::string& Append(AZStd::string& inout, const char* str)
        {
            if (!str)
            {
                return inout;
            }
            return inout.append(str);
        }

        AZStd::string& Prepend(AZStd::string& inout, const char s)
        {
            return inout.insert((size_t)0, 1, s);
        }

        AZStd::string& Prepend(AZStd::string& inout, const char* str)
        {
            if (!str)
            {
                return inout;
            }
            return inout.insert(0, str);
        }

        AZStd::string& LChop(AZStd::string& inout, size_t num /* = 1 */)
        {
            size_t len = inout.length();
            if (num == AZStd::string::npos)
            {
                num = 0;
            }
            if (num > len)
            {
                inout.clear();
                return inout;
            }
            return inout.erase(0, num);
        }

        AZStd::string& RChop(AZStd::string& inout, size_t num /* = 1 */)
        {
            size_t len = inout.length();
            if (num == AZStd::string::npos)
            {
                num = 0;
            }
            if (num > len)
            {
                inout.clear();
                return inout;
            }
            return inout.erase(len - num, len);
        }

        AZStd::string& LKeep(AZStd::string& inout, size_t pos, bool bKeepPosCharacter /* = false */)
        {
            size_t len = inout.length();
            if (pos == AZStd::string::npos)
            {
                pos = 0;
            }
            if (pos > len)
            {
                return inout;
            }
            if (bKeepPosCharacter)
            {
                return inout.erase(pos + 1, len);
            }
            else
            {
                return inout.erase(pos, len);
            }
        }

        AZStd::string& RKeep(AZStd::string& inout, size_t pos, bool bKeepPosCharacter /* = false */)
        {
            size_t len = inout.length();
            if (pos == AZStd::string::npos)
            {
                pos = 0;
            }
            if (pos > len)
            {
                inout.clear();
                return inout;
            }
            if (bKeepPosCharacter)
            {
                return inout.erase(0, pos);
            }
            else
            {
                return inout.erase(0, pos + 1);
            }
        }

        bool Replace(AZStd::string& inout, const char replaceA, const char withB, bool bCaseSensitive /*= false*/, bool bReplaceFirst /*= false*/, bool bReplaceLast /*= false*/)
        {
            bool bSomethingWasReplaced = false;
            size_t pos = 0;

            if (!bReplaceFirst && !bReplaceLast)
            {
                //replace all
                if (bCaseSensitive)
                {
                    bSomethingWasReplaced = false;
                    while ((pos = inout.find(replaceA, pos)) != AZStd::string::npos)
                    {
                        bSomethingWasReplaced = true;
                        inout.replace(pos, 1, 1, withB);
                        pos++;
                    }
                }
                else
                {
                    AZStd::string lowercaseIn(inout);
                    AZStd::to_lower(lowercaseIn.begin(), lowercaseIn.end());

                    char lowercaseReplaceA = (char)tolower(replaceA);

                    while ((pos = lowercaseIn.find(lowercaseReplaceA, pos)) != AZStd::string::npos)
                    {
                        bSomethingWasReplaced = true;
                        inout.replace(pos, 1, 1, withB);
                        pos++;
                    }
                }
            }
            else
            {
                if (bCaseSensitive)
                {
                    if (bReplaceFirst)
                    {
                        //replace first
                        if ((pos = inout.find_first_of(replaceA)) != AZStd::string::npos)
                        {
                            bSomethingWasReplaced = true;
                            inout.replace(pos, 1, 1, withB);
                        }
                    }

                    if (bReplaceLast)
                    {
                        //replace last
                        if ((pos = inout.find_last_of(replaceA)) != AZStd::string::npos)
                        {
                            bSomethingWasReplaced = true;
                            inout.replace(pos, 1, 1, withB);
                        }
                    }
                }
                else
                {
                    AZStd::string lowercaseIn(inout);
                    AZStd::to_lower(lowercaseIn.begin(), lowercaseIn.end());

                    char lowercaseReplaceA = (char)tolower(replaceA);

                    if (bReplaceFirst)
                    {
                        //replace first
                        if ((pos = lowercaseIn.find_first_of(lowercaseReplaceA)) != AZStd::string::npos)
                        {
                            bSomethingWasReplaced = true;
                            inout.replace(pos, 1, 1, withB);
                            if (bReplaceLast)
                            {
                                lowercaseIn.replace(pos, 1, 1, withB);
                            }
                        }
                    }

                    if (bReplaceLast)
                    {
                        //replace last
                        if ((pos = lowercaseIn.find_last_of(lowercaseReplaceA)) != AZStd::string::npos)
                        {
                            bSomethingWasReplaced = true;
                            inout.replace(pos, 1, 1, withB);
                        }
                    }
                }
            }

            return bSomethingWasReplaced;
        }

        bool Replace(AZStd::string& inout, const char* replaceA, const char* withB, bool bCaseSensitive /*= false*/, bool bReplaceFirst /*= false*/, bool bReplaceLast /*= false*/)
        {
            if (!replaceA) //withB can be nullptr
            {
                return false;
            }

            size_t lenA = strlen(replaceA);
            if (!lenA)
            {
                return false;
            }

            const char* emptystring = "";
            if (!withB)
            {
                withB = emptystring;
            }

            size_t lenB = strlen(withB);

            bool bSomethingWasReplaced = false;

            size_t pos = 0;

            if (!bReplaceFirst && !bReplaceLast)
            {
                //replace all
                if (bCaseSensitive)
                {
                    while ((pos = inout.find(replaceA, pos)) != AZStd::string::npos)
                    {
                        bSomethingWasReplaced = true;
                        inout.replace(pos, lenA, withB, lenB);
                        pos += lenB;
                    }
                }
                else
                {
                    AZStd::string lowercaseIn(inout);
                    AZStd::to_lower(lowercaseIn.begin(), lowercaseIn.end());

                    AZStd::string lowercaseReplaceA(replaceA);
                    AZStd::to_lower(lowercaseReplaceA.begin(), lowercaseReplaceA.end());

                    while ((pos = lowercaseIn.find(lowercaseReplaceA, pos)) != AZStd::string::npos)
                    {
                        bSomethingWasReplaced = true;
                        lowercaseIn.replace(pos, lenA, withB, lenB);
                        inout.replace(pos, lenA, withB, lenB);
                        pos += lenB;
                    }
                }
            }
            else
            {
                if (bCaseSensitive)
                {
                    if (bReplaceFirst)
                    {
                        //replace first
                        if ((pos = inout.find(replaceA)) != AZStd::string::npos)
                        {
                            bSomethingWasReplaced = true;
                            inout.replace(pos, lenA, withB, lenB);
                        }
                    }

                    if (bReplaceLast)
                    {
                        //replace last
                        if ((pos = inout.rfind(replaceA)) != AZStd::string::npos)
                        {
                            bSomethingWasReplaced = true;
                            inout.replace(pos, lenA, withB, lenB);
                        }
                    }
                }
                else
                {
                    AZStd::string lowercaseIn(inout);
                    AZStd::to_lower(lowercaseIn.begin(), lowercaseIn.end());

                    AZStd::string lowercaseReplaceA(replaceA);
                    AZStd::to_lower(lowercaseReplaceA.begin(), lowercaseReplaceA.end());

                    if (bReplaceFirst)
                    {
                        //replace first
                        if ((pos = lowercaseIn.find(lowercaseReplaceA)) != AZStd::string::npos)
                        {
                            bSomethingWasReplaced = true;
                            inout.replace(pos, lenA, withB, lenB);
                            if (bReplaceLast)
                            {
                                lowercaseIn.replace(pos, lenA, withB, lenB);
                            }
                        }
                    }

                    if (bReplaceLast)
                    {
                        //replace last
                        if ((pos = lowercaseIn.rfind(lowercaseReplaceA)) != AZStd::string::npos)
                        {
                            bSomethingWasReplaced = true;
                            inout.replace(pos, lenA, withB, lenB);
                        }
                    }
                }
            }

            return bSomethingWasReplaced;
        }

        bool Strip(AZStd::string& inout, const char stripCharacter /*= ' '*/, bool bCaseSensitive /*= false*/, bool bStripBeginning /*= false*/, bool bStripEnding /*= false*/)
        {
            bool bSomethingWasStripped = false;
            size_t pos = 0;
            if (!bStripBeginning && !bStripEnding)
            {
                //strip all
                if (bCaseSensitive)
                {
                    while ((pos = inout.find_first_of(stripCharacter, pos)) != AZStd::string::npos)
                    {
                        bSomethingWasStripped = true;
                        inout.erase(pos, 1);
                    }
                }
                else
                {
                    char lowerStripCharacter = (char)tolower(stripCharacter);
                    char upperStripCharacter = (char)toupper(stripCharacter);
                    while ((pos = inout.find_first_of(lowerStripCharacter, pos)) != AZStd::string::npos)
                    {
                        bSomethingWasStripped = true;
                        inout.erase(pos, 1);
                    }
                    pos = 0;
                    while ((pos = inout.find_first_of(upperStripCharacter, pos)) != AZStd::string::npos)
                    {
                        bSomethingWasStripped = true;
                        inout.erase(pos, 1);
                    }
                }
            }
            else
            {
                if (bCaseSensitive)
                {
                    if (bStripBeginning)
                    {
                        //strip beginning
                        if ((pos = inout.find_first_not_of(stripCharacter)) != AZStd::string::npos)
                        {
                            if (pos != 0)
                            {
                                bSomethingWasStripped = true;
                                RKeep(inout, pos, true);
                            }
                        }
                        else
                        {
                            // strip first
                            inout.erase(0, 1);
                        }
                    }

                    if (bStripEnding)
                    {
                        //strip ending
                        if ((pos = inout.find_last_not_of(stripCharacter)) != AZStd::string::npos)
                        {
                            if (pos != inout.length())
                            {
                                bSomethingWasStripped = true;
                                LKeep(inout, pos, true);
                            }
                        }
                        else
                        {
                            // strip last
                            const size_t length = inout.length();
                            if (length > 0)
                            {
                                inout.erase(length - 1, 1);
                            }
                        }
                    }
                }
                else
                {
                    AZStd::string combinedStripCharacters;
                    combinedStripCharacters += (char)tolower(stripCharacter);
                    combinedStripCharacters += (char)toupper(stripCharacter);

                    if (bStripBeginning)
                    {
                        //strip beginning
                        if ((pos = inout.find_first_not_of(combinedStripCharacters)) != AZStd::string::npos)
                        {
                            if (pos != 0)
                            {
                                bSomethingWasStripped = true;
                                RKeep(inout, pos, true);
                            }
                        }
                        else
                        {
                            // strip first
                            inout.erase(0, 1);
                        }
                    }

                    if (bStripEnding)
                    {
                        //strip ending
                        if ((pos = inout.find_last_not_of(combinedStripCharacters)) != AZStd::string::npos)
                        {
                            if (pos != inout.length())
                            {
                                bSomethingWasStripped = true;
                                LKeep(inout, pos, true);
                            }
                        }
                        else
                        {
                            // strip last
                            const size_t length = inout.length();
                            if (length > 0)
                            {
                                inout.erase(length - 1, 1);
                            }
                        }
                    }
                }
            }
            return bSomethingWasStripped;
        }

        bool Strip(AZStd::string& inout, const char* stripCharacters /*= " "*/, bool bCaseSensitive /*= false*/, bool bStripBeginning /*= false*/, bool bStripEnding /*= false*/)
        {
            bool bSomethingWasStripped = false;
            size_t pos = 0;
            if (!bStripBeginning && !bStripEnding)
            {
                //strip all
                if (bCaseSensitive)
                {
                    while ((pos = inout.find_first_of(stripCharacters, pos)) != AZStd::string::npos)
                    {
                        bSomethingWasStripped = true;
                        inout.erase(pos, 1);
                    }
                }
                else
                {
                    AZStd::string lowercaseStripCharacters(stripCharacters);
                    AZStd::to_lower(lowercaseStripCharacters.begin(), lowercaseStripCharacters.end());
                    AZStd::string combinedStripCharacters(stripCharacters);
                    AZStd::to_upper(combinedStripCharacters.begin(), combinedStripCharacters.end());
                    combinedStripCharacters += lowercaseStripCharacters;

                    while ((pos = inout.find_first_of(combinedStripCharacters, pos)) != AZStd::string::npos)
                    {
                        bSomethingWasStripped = true;
                        inout.erase(pos, 1);
                    }
                }
            }
            else
            {
                if (bCaseSensitive)
                {
                    if (bStripBeginning)
                    {
                        //strip beginning
                        if ((pos = inout.find_first_not_of(stripCharacters)) != AZStd::string::npos)
                        {
                            if (pos != 0)
                            {
                                bSomethingWasStripped = true;
                                RKeep(inout, pos, true);
                            }
                        }
                    }

                    if (bStripEnding)
                    {
                        //strip ending
                        if ((pos = inout.find_last_not_of(stripCharacters)) != AZStd::string::npos)
                        {
                            if (pos != inout.length())
                            {
                                bSomethingWasStripped = true;
                                LKeep(inout, pos, true);
                            }
                        }
                    }
                }
                else
                {
                    AZStd::string lowercaseStripCharacters(stripCharacters);
                    AZStd::to_lower(lowercaseStripCharacters.begin(), lowercaseStripCharacters.end());
                    AZStd::string combinedStripCharacters(stripCharacters);
                    AZStd::to_upper(combinedStripCharacters.begin(), combinedStripCharacters.end());
                    combinedStripCharacters += lowercaseStripCharacters;

                    if (bStripBeginning)
                    {
                        //strip beginning
                        if ((pos = inout.find_first_not_of(combinedStripCharacters)) != AZStd::string::npos)
                        {
                            if (pos != 0)
                            {
                                bSomethingWasStripped = true;
                                RKeep(inout, pos, true);
                            }
                        }
                    }

                    if (bStripEnding)
                    {
                        //strip ending
                        if ((pos = inout.find_last_not_of(combinedStripCharacters)) != AZStd::string::npos)
                        {
                            if (pos != inout.length())
                            {
                                bSomethingWasStripped = true;
                                LKeep(inout, pos, true);
                            }
                        }
                    }
                }
            }
            return bSomethingWasStripped;
        }

        AZStd::string& TrimWhiteSpace(AZStd::string& value, bool leading, bool trailing)
        {
            static const char* trimmable = " \t\r\n";
            if (value.length() > 0)
            {
                if (leading)
                {
                    value.erase(0, value.find_first_not_of(trimmable));
                }
                if (trailing)
                {
                    value.erase(value.find_last_not_of(trimmable) + 1);
                }
            }
            return value;
        }

        void Tokenize(AZStd::string_view in, AZStd::vector<AZStd::string>& tokens, const char delimiter /* = ','*/, bool keepEmptyStrings /*= false*/, bool keepSpaceStrings /*= false*/)
        {
            if (in.empty())
            {
                return;
            }

            size_t pos, lastPos = 0;
            bool bDone = false;
            while (!bDone)
            {
                if ((pos = in.find_first_of(delimiter, lastPos)) == AZStd::string::npos)
                {
                    bDone = true;
                    pos = in.length();
                }

                AZStd::string newElement(AZStd::string(in.data() + lastPos, pos - lastPos));
                bool bIsEmpty = newElement.empty();
                bool bIsSpaces = false;
                if (!bIsEmpty)
                {
                    bIsSpaces = Strip(newElement, ' ') && newElement.empty();
                }

                if ((!bIsEmpty && !bIsSpaces) ||
                    (bIsEmpty && keepEmptyStrings) ||
                    (bIsSpaces && keepSpaceStrings))
                {
                    tokens.push_back(AZStd::string(in.data() + lastPos, pos - lastPos));
                }

                lastPos = pos + 1;
            }
        }

        void Tokenize(AZStd::string_view in, AZStd::vector<AZStd::string>& tokens, const char* delimiters /* = ", "*/, bool keepEmptyStrings /*= false*/, bool keepSpaceStrings /*= false*/)
        {
            if (!delimiters)
            {
                return;
            }

            if (in.empty())
            {
                return;
            }

            size_t pos, lastPos = 0;
            bool bDone = false;
            while (!bDone)
            {
                if ((pos = in.find_first_of(delimiters, lastPos)) == AZStd::string::npos)
                {
                    bDone = true;
                    pos = in.length();
                }

                AZStd::string newElement(AZStd::string(in.data() + lastPos, pos - lastPos));
                bool bIsEmpty = newElement.empty();
                bool bIsSpaces = false;
                if (!bIsEmpty)
                {
                    bIsSpaces = Strip(newElement, ' ') && newElement.empty();
                }

                if ((bIsEmpty && keepEmptyStrings) ||
                    (bIsSpaces && keepSpaceStrings) ||
                    (!bIsSpaces && !bIsEmpty))
                {
                    tokens.push_back(AZStd::string(in.data() + lastPos, pos - lastPos));
                }

                lastPos = pos + 1;
            }
        }

        bool FindFirstOf(AZStd::string_view inString, size_t offset, const AZStd::vector<AZStd::string_view>& searchStrings, uint32_t& outIndex, size_t& outOffset)
        {
            bool found = false;

            outIndex = 0;
            outOffset = AZStd::string::npos;
            for (int32_t i = 0; i < searchStrings.size(); ++i)
            {
                const AZStd::string& search = searchStrings[i];

                size_t entry = inString.find(search, offset);
                if (entry != AZStd::string::npos)
                {
                    if (!found || (entry < outOffset))
                    {
                        found = true;
                        outIndex = i;
                        outOffset = entry;
                    }
                }
            }

            return found;
        }

        void Tokenize(AZStd::string_view input, AZStd::vector<AZStd::string>& tokens, const AZStd::vector<AZStd::string_view>& delimiters, bool keepEmptyStrings /*= false*/, bool keepSpaceStrings /*= false*/)
        {
            if (input.empty())
            {
                return;
            }

            size_t offset = 0;
            for (;;)
            {
                uint32_t nextMatch = 0;
                size_t nextOffset = offset;
                if (!FindFirstOf(input, offset, delimiters, nextMatch, nextOffset))
                {
                    // No more occurrences of a separator, consume whatever is left and exit
                    tokens.push_back(input.substr(offset));
                    break;
                }

                // Take the substring, not including the separator, and increment our offset
                AZStd::string nextSubstring = input.substr(offset, nextOffset - offset);
                if (keepEmptyStrings || keepSpaceStrings || !nextSubstring.empty())
                {
                    tokens.push_back(nextSubstring);
                }

                offset = nextOffset + delimiters[nextMatch].size();
            }
        }

        size_t UniqueCharacters(const char* in, bool bCaseSensitive /* = false*/)
        {
            if (!in)
            {
                return 0;
            }

            AZStd::set<char> norepeats;

            size_t len = strlen(in);
            for (size_t i = 0; i < len; ++i)
            {
                norepeats.insert(bCaseSensitive ? in[i] : (char)tolower(in[i]));
            }

            return norepeats.size();
        }

        size_t CountCharacters(const char* in, char c, bool bCaseSensitive /* = false*/)
        {
            if (!in)
            {
                return 0;
            }

            size_t len = strlen(in);
            if (!len)
            {
                return 0;
            }

            size_t count = 0;
            for (size_t i = 0; i < len; ++i)
            {
                if (bCaseSensitive)
                {
                    if (in[i] == c)
                    {
                        count++;
                    }
                }
                else
                {
                    char lowerc = (char)tolower(c);
                    if (tolower(in[i]) == lowerc)
                    {
                        count++;
                    }
                }
            }

            return count;
        }

        int ToInt(const char* in)
        {
            if (!in)
            {
                return 0;
            }
            return atoi(in);
        }

        bool LooksLikeInt(const char* in, int* pInt /*=nullptr*/)
        {
            if (!in)
            {
                return false;
            }

            //if pos is past then end of the string false
            size_t len = strlen(in);
            if (!len)//must at least 1 characters to work with "1"
            {
                return false;
            }

            const char* pStr = in;

            size_t countNeg = 0;
            while (*pStr != '\0' &&
                   (isdigit(*pStr) ||
                    *pStr == '-'))
            {
                if (*pStr == '-')
                {
                    countNeg++;
                }
                pStr++;
            }

            if (*pStr == '\0' &&
                countNeg < 2)
            {
                if (pInt)
                {
                    *pInt = ToInt(in);
                }

                return true;
            }
            return false;
        }

        double ToDouble(const char* in)
        {
            if (!in)
            {
                return 0.;
            }
            return atof(in);
        }

        bool LooksLikeDouble(const char* in, double* pDouble)
        {
            if (!in)
            {
                return false;
            }

            size_t len = strlen(in);
            if (len < 2)//must have at least 2 characters to work with "1."
            {
                return false;
            }

            const char* pStr = in;

            size_t countDot = 0;
            size_t countNeg = 0;
            while (*pStr != '\0' &&
                (isdigit(*pStr) ||
                (*pStr == '-' ||
                    *pStr == '.')))
            {
                if (*pStr == '.')
                {
                    countDot++;
                }
                if (*pStr == '-')
                {
                    countNeg++;
                }
                pStr++;
            }

            if (*pStr == '\0' &&
                countDot == 1 &&
                countNeg < 2)
            {
                if (pDouble)
                {
                    *pDouble = ToDouble(in);
                }

                return true;
            }

            return false;
        }

        float ToFloat(const char* in)
        {
            if (!in)
            {
                return 0.f;
            }
            return (float)atof(in);
        }

        bool LooksLikeFloat(const char* in, float* pFloat /* = nullptr */)
        {
            bool result = false;

            if (pFloat)
            {
                double doubleValue = 0.0;
                result = LooksLikeDouble(in, &doubleValue);

                (*pFloat) = aznumeric_cast<float>(doubleValue);
            }
            else
            {
                result = LooksLikeDouble(in);
            }

            return result;
        }

        bool ToBool(const char* in)
        {
            bool boolValue = false;
            if (LooksLikeBool(in, &boolValue))
            {
                return boolValue;
            }
            return false;
        }

        bool LooksLikeBool(const char* in, bool* pBool /* = nullptr */)
        {
            if (!in)
            {
                return false;
            }

            if (!azstricmp(in, "true") || !azstricmp(in, "1"))
            {
                if (pBool)
                {
                    *pBool = true;
                }
                return true;
            }

            if (!azstricmp(in, "false") || !azstricmp(in, "0"))
            {
                if (pBool)
                {
                    *pBool = false;
                }
                return true;
            }

            return false;
        }

        template <typename VECTOR_TYPE, uint32_t ELEMENT_COUNT>
        bool LooksLikeVectorHelper(const char* in, VECTOR_TYPE* outVector)
        {
            AZStd::vector<AZStd::string> tokens;
            Tokenize(in, tokens, ',', false, true);
            if (tokens.size() == ELEMENT_COUNT)
            {
                float vectorValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

                for (uint32_t element = 0; element < ELEMENT_COUNT; ++element)
                {
                    if (!LooksLikeFloat(tokens[element].c_str(), outVector ? &vectorValues[element] : nullptr))
                    {
                        return false;
                    }
                }

                if (outVector)
                {
                    for (uint32_t element = 0; element < ELEMENT_COUNT; ++element)
                    {
                        outVector->SetElement(element, vectorValues[element]);
                    }
                }

                return true;
            }

            return false;
        }

        bool LooksLikeVector2(const char* in, AZ::Vector2* outVector)
        {
            return LooksLikeVectorHelper<AZ::Vector2, 2>(in, outVector);
        }

        AZ::Vector2 ToVector2(const char* in)
        {
            AZ::Vector2 vector;
            LooksLikeVector2(in, &vector);
            return vector;
        }

        bool LooksLikeVector3(const char* in, AZ::Vector3* outVector)
        {
            return LooksLikeVectorHelper<AZ::Vector3, 3>(in, outVector);
        }

        AZ::Vector3 ToVector3(const char* in)
        {
            AZ::Vector3 vector;
            LooksLikeVector3(in, &vector);
            return vector;
        }

        bool LooksLikeVector4(const char* in, AZ::Vector4* outVector)
        {
            return LooksLikeVectorHelper<AZ::Vector4, 4>(in, outVector);
        }

        AZ::Vector4 ToVector4(const char* in)
        {
            AZ::Vector4 vector;
            LooksLikeVector4(in, &vector);
            return vector;
        }

        bool ToHexDump(const char* in, AZStd::string& out)
        {
            struct TInline
            {
                static void ByteToHex(char* pszHex, unsigned char bValue)
                {
                    pszHex[0] = bValue / 16;

                    if (pszHex[0] < 10)
                    {
                        pszHex[0] += '0';
                    }
                    else
                    {
                        pszHex[0] -= 10;
                        pszHex[0] += 'A';
                    }

                    pszHex[1] = bValue % 16;

                    if (pszHex[1] < 10)
                    {
                        pszHex[1] += '0';
                    }
                    else
                    {
                        pszHex[1] -= 10;
                        pszHex[1] += 'A';
                    }
                }
            };

            size_t len = strlen(in);
            if (len < 1) //must be at least 1 character to work with
            {
                return false;
            }

            size_t nBytes = len;

            char* pszData = reinterpret_cast<char*>(azmalloc((nBytes * 2) + 1));

            for (size_t ii = 0; ii < nBytes; ++ii)
            {
                TInline::ByteToHex(&pszData[ii * 2], in[ii]);
            }

            pszData[nBytes * 2] = 0x00;
            out = pszData;
            azfree(pszData);

            return true;
        }

        bool FromHexDump(const char* in, AZStd::string& out)
        {
            struct TInline
            {
                static unsigned char HexToByte(const char* pszHex)
                {
                    unsigned char bHigh = 0;
                    unsigned char bLow = 0;

                    if ((pszHex[0] >= '0') && (pszHex[0] <= '9'))
                    {
                        bHigh = pszHex[0] - '0';
                    }
                    else if ((pszHex[0] >= 'A') && (pszHex[0] <= 'F'))
                    {
                        bHigh = (pszHex[0] - 'A') + 10;
                    }

                    bHigh = bHigh << 4;

                    if ((pszHex[1] >= '0') && (pszHex[1] <= '9'))
                    {
                        bLow = pszHex[1] - '0';
                    }
                    else if ((pszHex[1] >= 'A') && (pszHex[1] <= 'F'))
                    {
                        bLow = (pszHex[1] - 'A') + 10;
                    }

                    return bHigh | bLow;
                }
            };

            size_t len = strlen(in);
            if (len < 2) //must be at least 2 characters to work with
            {
                return false;
            }

            size_t nBytes = len / 2;
            char* pszData = reinterpret_cast<char*>(azmalloc(nBytes + 1));

            for (size_t ii = 0; ii < nBytes; ++ii)
            {
                pszData[ii] = TInline::HexToByte(&in[ii * 2]);
            }

            pszData[nBytes] = 0x00;
            out = pszData;
            azfree(pszData);

            return true;
        }

        namespace NumberFormatting
        {
            int GroupDigits(char* buffer, size_t bufferSize, size_t decimalPosHint, char digitSeparator, char decimalSeparator, int groupingSize, int firstGroupingSize)
            {
                static const int MAX_SEPARATORS = 16;

                AZ_Assert(buffer, "Null string buffer");
                AZ_Assert(bufferSize > decimalPosHint, "Decimal position %lu cannot be located beyond bufferSize %lu", decimalPosHint, bufferSize);
                AZ_Assert(groupingSize > 0, "Grouping size must be a positive integer");

                int numberEndPos = 0;
                int stringEndPos = 0;
                
                if (decimalPosHint > 0 && decimalPosHint < (bufferSize - 1) && buffer[decimalPosHint] == decimalSeparator)
                {
                    // Assume the number ends at the supplied location
                    numberEndPos = (int)decimalPosHint;
                    stringEndPos = numberEndPos + (int)strnlen(buffer + numberEndPos, bufferSize - numberEndPos);
                }
                else
                {
                    // Search for the final digit or separator while obtaining the string length
                    int lastDigitSeenPos = 0;

                    while (stringEndPos < bufferSize)
                    {
                        char c = buffer[stringEndPos];

                        if (!c)
                        {
                            break;
                        }
                        else if (c == decimalSeparator)
                        {
                            // End the number if there's a decimal
                            numberEndPos = stringEndPos;
                        }
                        else if (numberEndPos <= 0 && c >= '0' && c <= '9')
                        {
                            // Otherwise keep track of where the last digit we've seen is
                            lastDigitSeenPos = stringEndPos;
                        }

                        stringEndPos++;
                    }

                    if (numberEndPos <= 0)
                    {
                        if (lastDigitSeenPos > 0)
                        {
                            // No decimal, so use the last seen digit as the end of the number
                            numberEndPos = lastDigitSeenPos + 1;
                        }
                        else
                        {
                            // No digits, no decimals, therefore no change in the string
                            return stringEndPos;
                        }
                    }
                }

                if (firstGroupingSize <= 0)
                {
                    firstGroupingSize = groupingSize;
                }

                // Determine where to place the separators
                int groupingSizes[] = { firstGroupingSize + 1, groupingSize };  // First group gets +1 since we begin all subsequent groups at the second digit
                int groupingOffsetsToNext[] = { 1, 0 };  // We will offset from the first entry to the second, then stay at the second for remaining iterations
                const int* currentGroupingSize = groupingSizes;
                const int* currentGroupingOffsetToNext = groupingOffsetsToNext;
                AZStd::fixed_vector<char*, MAX_SEPARATORS> separatorLocations;
                int groupCounter = 0;
                int digitPosition = numberEndPos - 1;

                while (digitPosition >= 0)
                {
                    // Walk backwards in the string from the least significant digit to the most significant, demarcating consecutive groups of digits
                    char c = buffer[digitPosition];

                    if (c >= '0' && c <= '9')
                    {
                        if (++groupCounter == *currentGroupingSize)
                        {
                            // Demarcate a new group of digits at this location
                            separatorLocations.push_back(buffer + digitPosition);
                            currentGroupingSize += *currentGroupingOffsetToNext;
                            currentGroupingOffsetToNext += *currentGroupingOffsetToNext;
                            groupCounter = 0;
                        }

                        digitPosition--;
                    }
                    else
                    {
                        break;
                    }
                }

                if (stringEndPos + separatorLocations.size() >= bufferSize)
                {
                    // Won't fit into buffer, so return unchanged
                    return stringEndPos;
                }

                // Insert the separators by shifting characters forward in the string, starting at the end and working backwards
                const char* src = buffer + stringEndPos;
                char* dest = buffer + stringEndPos + separatorLocations.size();
                auto separatorItr = separatorLocations.begin();

                while (separatorItr != separatorLocations.end())
                {
                    while (src > *separatorItr)
                    {
                        *dest-- = *src--;
                    }

                    // Insert the separator and reduce the distance between our destination and source by one
                    *dest-- = digitSeparator;
                    ++separatorItr;
                }

                return (int)(stringEndPos + separatorLocations.size());
            }
        }

        namespace AssetPath
        {
            void CalculateBranchToken(const AZStd::string& appRootPath, AZStd::string& token)
            {
                // Normalize the token to prepare for CRC32 calculation
                AZStd::string normalized = appRootPath;

                // Strip out any trailing path separators
                AZ::StringFunc::Strip(normalized, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING  AZ_WRONG_FILESYSTEM_SEPARATOR_STRING,false, false, true);

                // Lower case always
                AZStd::to_lower(normalized.begin(), normalized.end());

                // Substitute path separators with '_'
                AZStd::replace(normalized.begin(), normalized.end(), '\\', '_');
                AZStd::replace(normalized.begin(), normalized.end(), '/', '_');

                // Perform the CRC32 calculation
                const AZ::Crc32 branchTokenCrc(normalized.c_str(), normalized.size(), true);
                char branchToken[12];
                azsnprintf(branchToken, AZ_ARRAY_SIZE(branchToken), "0x%08X", static_cast<AZ::u32>(branchTokenCrc));
                token = AZStd::string(branchToken);
            }
        }

        namespace AssetDatabasePath
        {
            bool Normalize(AZStd::string& inout)
            {
                Strip(inout, AZ_DATABASE_INVALID_CHARACTERS);

    #ifndef AZ_FILENAME_ALLOW_SPACES
                Strip(inout, AZ_SPACE_CHARACTERS);
    #endif // AZ_FILENAME_ALLOW_SPACES

                //too small to be a path
                AZStd::size_t len = inout.length();
                if (!len)
                {
                    return false;
                }

                //too big to be a path fail
                if (len > AZ_MAX_PATH_LEN)
                {
                    return false;
                }

                AZStd::replace(inout.begin(), inout.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);
                Replace(inout, AZ_DOUBLE_CORRECT_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR_STRING);

                return IsValid(inout.c_str());
            }

            bool IsValid(const char* in)
            {
                if (!in)
                {
                    return false;
                }

                if (!strlen(in))
                {
                    return false;
                }

                if (Find(in, AZ_DATABASE_INVALID_CHARACTERS) != AZStd::string::npos)
                {
                    return false;
                }

                if (Find(in, AZ_WRONG_DATABASE_SEPARATOR) != AZStd::string::npos)
                {
                    return false;
                }

    #ifndef AZ_FILENAME_ALLOW_SPACES
                if (Find(in, AZ_SPACE_CHARACTERS) != AZStd::string::npos)
                {
                    return false;
                }
    #endif // AZ_FILENAME_ALLOW_SPACES

                if (LastCharacter(in) == AZ_CORRECT_DATABASE_SEPARATOR)
                {
                    return false;
                }

                return true;
            }

            bool ConstructFull(const char* pProjectRoot, const char* pDatabaseRoot, const char* pDatabasePath, const char* pDatabaseFile, const char* pFileExtension, AZStd::string& out)
            {
                if (!pProjectRoot)
                {
                    return false;
                }

                if (!pDatabaseRoot)
                {
                    return false;
                }

                if (!pDatabasePath)
                {
                    return false;
                }

                if (!pDatabaseFile)
                {
                    return false;
                }

                if (!strlen(pProjectRoot))
                {
                    return false;
                }

                if (!strlen(pDatabaseRoot))
                {
                    return false;
                }

                if (!strlen(pDatabaseFile))
                {
                    return false;
                }

                if (pFileExtension && strlen(pFileExtension))
                {
                    if (Find(pFileExtension, AZ_CORRECT_DATABASE_SEPARATOR) != AZStd::string::npos)
                    {
                        return false;
                    }

                    if (Find(pFileExtension, AZ_WRONG_DATABASE_SEPARATOR) != AZStd::string::npos)
                    {
                        return false;
                    }
                }

                AZStd::string tempProjectRoot;
                AZStd::string tempDatabaseRoot;
                AZStd::string tempDatabasePath;
                AZStd::string tempDatabaseFile;
                AZStd::string tempFileExtension;
                if (pProjectRoot == out.c_str() || pDatabaseRoot == out.c_str() || pDatabasePath == out.c_str() || pDatabaseFile == out.c_str() || pFileExtension == out.c_str())
                {
                    tempProjectRoot = pProjectRoot;
                    tempDatabaseRoot = pDatabaseRoot;
                    tempDatabasePath = pDatabasePath;
                    tempDatabaseFile = pDatabaseFile;
                    tempFileExtension = pFileExtension;
                    pProjectRoot = tempProjectRoot.c_str();
                    pDatabaseRoot = tempDatabaseRoot.c_str();
                    pDatabasePath = tempDatabasePath.c_str();
                    pDatabaseFile = tempDatabaseFile.c_str();
                    pFileExtension = tempFileExtension.c_str();
                }

                AZStd::string projectRoot = pProjectRoot;
                if (!Root::Normalize(projectRoot))
                {
                    return false;
                }

                AZStd::string databasePath = pDatabasePath;
                if (!RelativePath::Normalize(databasePath))
                {
                    return false;
                }

                if (!Path::Join(projectRoot.c_str(), pDatabaseRoot, out))
                {
                    return false;
                }

                if (!databasePath.empty())
                {
                    if (!Path::Join(out.c_str(), databasePath.c_str(), out))
                    {
                        return false;
                    }
                }

                if (!Path::Join(out.c_str(), pDatabaseFile, out))
                {
                    return false;
                }

                if (pFileExtension)
                {
                    Path::ReplaceExtension(out, pFileExtension);
                }

                return Path::IsValid(out.c_str());
            }

            bool Split(const char* in, AZStd::string* pDstProjectRootOut /*= nullptr*/, AZStd::string* pDstDatabaseRootOut /*= nullptr*/, AZStd::string* pDstDatabasePathOut /*= nullptr*/, AZStd::string* pDstFileOut /*= nullptr*/, AZStd::string* pDstFileExtensionOut /*= nullptr*/)
            {
                if (!in)
                {
                    return false;
                }

                if (!strlen(in))
                {
                    return false;
                }

                AZStd::string temp(in);
                if (!Normalize(temp))
                {
                    return false;
                }

                if (temp.empty())
                {
                    return false;
                }

                if (pDstProjectRootOut)
                {
                    pDstProjectRootOut->clear();
                }

                if (pDstDatabaseRootOut)
                {
                    pDstDatabaseRootOut->clear();
                }

                if (pDstDatabasePathOut)
                {
                    pDstDatabasePathOut->clear();
                }

                if (pDstFileOut)
                {
                    pDstFileOut->clear();
                }

                if (pDstFileExtensionOut)
                {
                    pDstFileExtensionOut->clear();
                }
                //////////////////////////////////////////////////////////////////////////
                AZStd::size_t lastExt = AZStd::string::npos;
                if ((lastExt = Find(temp.c_str(), AZ_DATABASE_EXTENSION_SEPARATOR, AZStd::string::npos, true)) != AZStd::string::npos)
                {
                    AZStd::size_t lastSep = AZStd::string::npos;
                    if ((lastSep = Find(temp.c_str(), AZ_CORRECT_DATABASE_SEPARATOR, AZStd::string::npos, true)) != AZStd::string::npos)
                    {
                        if (lastSep > lastExt)
                        {
                            lastExt = AZStd::string::npos;
                        }
                    }

                    if (lastExt != AZStd::string::npos)
                    {
                        //we found a file extension
                        if (pDstFileExtensionOut)
                        {
                            *pDstFileExtensionOut = temp;
                            RKeep(*pDstFileExtensionOut, lastExt, true);
                        }

                        LKeep(temp, lastExt);
                    }
                }

                AZStd::size_t lastSep = AZStd::string::npos;
                if ((lastSep = Find(temp.c_str(), AZ_CORRECT_DATABASE_SEPARATOR, AZStd::string::npos, true)) != AZStd::string::npos)
                {
                    if (pDstFileOut)
                    {
                        *pDstFileOut = temp;
                        RKeep(*pDstFileOut, lastSep);
                    }
                    LKeep(temp, lastSep, true);
                }
                else if (!temp.empty())
                {
                    if (pDstFileOut)
                    {
                        *pDstFileOut = temp;
                    }
                    temp.clear();
                }

                if (pDstDatabasePathOut)
                {
                    *pDstDatabasePathOut = temp;
                }

                return true;
            }

            bool Join(const char* pFirstPart, const char* pSecondPart, AZStd::string& out, bool bJoinOverlapping /*= false*/, bool bCaseInsenitive /*= true*/, bool bNormalize /*= true*/)
            {
                //null && null
                if (!pFirstPart && !pSecondPart)
                {
                    return false;
                }

                size_t firstPartLen = 0;
                size_t secondPartLen = 0;
                if (pFirstPart)
                {
                    firstPartLen = strlen(pFirstPart);
                }
                if (pSecondPart)
                {
                    secondPartLen = strlen(pSecondPart);
                }

                //0 && 0
                if (!firstPartLen && !secondPartLen)
                {
                    return false;
                }

                //null good
                if (!pFirstPart && pSecondPart)
                {
                    //null 0
                    if (!secondPartLen)
                    {
                        return false;
                    }

                    out = pSecondPart;
                    if (bNormalize)
                    {
                        return Normalize(out);
                    }

                    return true;
                }

                //good null
                if (!pSecondPart && pFirstPart)
                {
                    //0 null
                    if (!firstPartLen)
                    {
                        return false;
                    }

                    out = pFirstPart;
                    if (bNormalize)
                    {
                        return Normalize(out);
                    }

                    return true;
                }

                //0 good
                if (!firstPartLen && pSecondPart)
                {
                    out = pSecondPart;
                    if (bNormalize)
                    {
                        return Normalize(out);
                    }

                    return true;
                }

                //good 0
                if (pFirstPart && !secondPartLen)
                {
                    out = pFirstPart;
                    if (bNormalize)
                    {
                        return Normalize(out);
                    }

                    return true;
                }

                if (Path::HasDrive(pSecondPart))
                {
                    return false;
                }

                AZStd::string tempFirst;
                AZStd::string tempSecond;
                if (pFirstPart == out.c_str() || pSecondPart == out.c_str())
                {
                    tempFirst = pFirstPart;
                    tempSecond = pSecondPart;
                    pFirstPart = tempFirst.c_str();
                    pSecondPart = tempSecond.c_str();
                }

                out = pFirstPart;

                if (bJoinOverlapping)
                {
                    AZStd::string firstPart(pFirstPart);
                    AZStd::string secondPart(pSecondPart);
                    if (bCaseInsenitive)
                    {
                        AZStd::to_lower(firstPart.begin(), firstPart.end());
                        AZStd::to_lower(secondPart.begin(), secondPart.end());
                    }

                    AZStd::vector<AZStd::string> firstPartDelimited;
                    Strip(firstPart, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);
                    Tokenize(firstPart.c_str(), firstPartDelimited, AZ_CORRECT_DATABASE_SEPARATOR, true);
                    AZStd::vector<AZStd::string> secondPartDelimited;
                    Strip(secondPart, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);
                    Tokenize(secondPart.c_str(), secondPartDelimited, AZ_CORRECT_DATABASE_SEPARATOR, true);
                    AZStd::vector<AZStd::string> secondPartNormalDelimited;

                    for (size_t i = 0; i < firstPartDelimited.size(); ++i)
                    {
                        if (firstPartDelimited[i].length() == secondPartDelimited[0].length())
                        {
                            if (!strncmp(firstPartDelimited[i].c_str(), secondPartDelimited[0].c_str(), firstPartDelimited[i].length()))
                            {
                                //we found the first component that is equal
                                //now every component for the rest of firstPart Must Be equal or we fail
                                bool bFailed = false;
                                size_t jj = 1;
                                for (size_t ii = i + 1; !bFailed && ii < firstPartDelimited.size(); ++ii)
                                {
                                    if (firstPartDelimited[ii].length() != secondPartDelimited[jj].length())
                                    {
                                        bFailed = true;
                                    }
                                    else if (strncmp(firstPartDelimited[ii].c_str(), secondPartDelimited[jj].c_str(), firstPartDelimited[ii].length()))
                                    {
                                        bFailed = true;
                                    }

                                    jj++;
                                }

                                if (!bFailed)
                                {
                                    if (LastCharacter(out.c_str()) != AZ_CORRECT_DATABASE_SEPARATOR)
                                    {
                                        out += AZ_CORRECT_DATABASE_SEPARATOR;
                                    }

                                    AZStd::string secondPartNormal(pSecondPart);
                                    Strip(secondPartNormal, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);
                                    Tokenize(secondPartNormal.c_str(), secondPartNormalDelimited, AZ_CORRECT_DATABASE_SEPARATOR, true);

                                    for (; jj < secondPartNormalDelimited.size(); ++jj)
                                    {
                                        out += secondPartNormalDelimited[jj];
                                        out += AZ_CORRECT_DATABASE_SEPARATOR;
                                    }

                                    if (LastCharacter(pSecondPart) != AZ_CORRECT_DATABASE_SEPARATOR)
                                    {
                                        RChop(out);
                                    }

                                    return true;
                                }
                            }
                        }
                    }
                }

                if (LastCharacter(pFirstPart) == AZ_CORRECT_DATABASE_SEPARATOR && FirstCharacter(pSecondPart) == AZ_CORRECT_DATABASE_SEPARATOR)
                {
                    Strip(out, AZ_CORRECT_DATABASE_SEPARATOR, false, false, true);
                }

                if (LastCharacter(pFirstPart) != AZ_CORRECT_DATABASE_SEPARATOR && FirstCharacter(pSecondPart) != AZ_CORRECT_DATABASE_SEPARATOR)
                {
                    Append(out, AZ_CORRECT_DATABASE_SEPARATOR);
                }

                out += pSecondPart;

                if (bNormalize)
                {
                    return Normalize(out);
                }

                return true;
            }

            bool IsASuperFolderOfB(const char* pathA, const char* pathB, bool bCaseInsenitive /*= true*/, bool bIgnoreStartingPath /*= true*/)
            {
                if (!pathA || !pathB)
                {
                    return false;
                }

                if (pathA == pathB)
                {
                    return false;
                }

                AZStd::string strPathA(pathA);
                if (!strPathA.length())
                {
                    return false;
                }

                AZStd::string strPathB(pathB);
                if (!strPathB.length())
                {
                    return false;
                }

                if (bIgnoreStartingPath)
                {
                    Path::StripDrive(strPathA);
                    Path::StripDrive(strPathB);

                    Strip(strPathA, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);
                    Strip(strPathB, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);
                }
                else
                {
                    Strip(strPathA, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);
                    Strip(strPathB, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);

                    size_t lenA = strPathA.length();
                    size_t lenB = strPathB.length();
                    if (lenA >= lenB)
                    {
                        return false;
                    }
                    else if (bCaseInsenitive)
                    {
                        if (azstrnicmp(strPathA.c_str(), strPathB.c_str(), lenA))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (strncmp(strPathA.c_str(), strPathB.c_str(), lenA))
                        {
                            return false;
                        }
                    }
                }

                AZStd::vector<AZStd::string> pathADelimited;
                Tokenize(strPathA.c_str(), pathADelimited, AZ_CORRECT_DATABASE_SEPARATOR, true);
                AZStd::vector<AZStd::string> pathBDelimited;
                Tokenize(strPathB.c_str(), pathBDelimited, AZ_CORRECT_DATABASE_SEPARATOR, true);

                //EX: A= p4/Main/Source/GameAssets/gameinfo
                //    B= p4/Main/Source/GameAssets/gameinfo/Characters
                if (bIgnoreStartingPath)
                {
                    for (size_t i = 0; i < pathADelimited.size(); ++i)
                    {
                        if (pathADelimited[i].length() != pathBDelimited[0].length())
                        {
                        }
                        else if (bCaseInsenitive)
                        {
                            if (!azstrnicmp(pathADelimited[i].c_str(), pathBDelimited[0].c_str(), pathADelimited[i].length()))
                            {
                                //we found the first component that is equal
                                //now every component for the rest of A Must Be equal or we fail

                                //if the number of components after this match in A are more than
                                //the remaining in B then fail as it can not be a super folder
                                if (pathADelimited.size() - i >= pathBDelimited.size())
                                {
                                    return false;
                                }

                                size_t jj = 1;
                                for (size_t ii = i + 1; ii < pathADelimited.size(); ++ii)
                                {
                                    if (pathADelimited[ii].length() != pathBDelimited[jj].length())
                                    {
                                        return false;
                                    }
                                    else if (azstrnicmp(pathADelimited[ii].c_str(), pathBDelimited[jj].c_str(), pathADelimited[ii].length()))
                                    {
                                        return false;
                                    }
                                    jj++;
                                }
                                return true;
                            }
                        }
                        else
                        {
                            if (!strncmp(pathADelimited[i].c_str(), pathBDelimited[0].c_str(), pathADelimited[i].length()))
                            {
                                //we found the first component that is equal
                                //now every component for the rest of A Must Be equal or we fail

                                //if the number of components after this match in A are more than
                                //the remaining in B then fail as it can not be a super folder
                                if (pathADelimited.size() - i >= pathBDelimited.size())
                                {
                                    return false;
                                }

                                size_t jj = 1;
                                for (size_t ii = i + 1; ii < pathADelimited.size(); ++ii)
                                {
                                    if (pathADelimited[ii].length() != pathBDelimited[jj].length())
                                    {
                                        return false;
                                    }
                                    if (strncmp(pathADelimited[ii].c_str(), pathBDelimited[jj].c_str(), pathADelimited[ii].length()))
                                    {
                                        return false;
                                    }
                                    jj++;
                                }
                                return true;
                            }
                        }
                    }
                    return false;
                }
                else
                {
                    //if the number of components after this match in A are more than
                    //the remaining in B then fail as it can not be a super folder
                    if (pathADelimited.size() >= pathBDelimited.size())
                    {
                        return false;
                    }

                    for (size_t i = 0; i < pathADelimited.size(); ++i)
                    {
                        if (pathADelimited[i].length() != pathBDelimited[i].length())
                        {
                            return false;
                        }
                        else if (bCaseInsenitive)
                        {
                            if (azstrnicmp(pathADelimited[i].c_str(), pathBDelimited[i].c_str(), pathADelimited[i].length()))
                            {
                                return false;
                            }
                        }
                        else
                        {
                            if (strncmp(pathADelimited[i].c_str(), pathBDelimited[i].c_str(), pathADelimited[i].length()))
                            {
                                return false;
                            }
                        }
                    }
                    return true;
                }
            }

            bool IsASubFolderOfB(const char* pathA, const char* pathB, bool bCaseInsenitive /*= true*/, bool bIgnoreStartingPath /*= true*/)
            {
                if (!pathA || !pathB)
                {
                    return false;
                }

                if (pathA == pathB)
                {
                    return false;
                }

                AZStd::string strPathA(pathA);
                if (!strPathA.length())
                {
                    return false;
                }

                AZStd::string strPathB(pathB);
                if (!strPathB.length())
                {
                    return false;
                }

                if (bIgnoreStartingPath)
                {
                    Path::StripDrive(strPathA);
                    Path::StripDrive(strPathB);

                    Strip(strPathA, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);
                    Strip(strPathB, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);
                }
                else
                {
                    Strip(strPathA, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);
                    Strip(strPathB, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);

                    size_t lenA = strPathA.length();
                    size_t lenB = strPathB.length();
                    if (lenA < lenB)
                    {
                        return false;
                    }
                    else if (bCaseInsenitive)
                    {
                        if (azstrnicmp(strPathA.c_str(), strPathB.c_str(), lenB))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (strncmp(strPathA.c_str(), strPathB.c_str(), lenB))
                        {
                            return false;
                        }
                    }
                }

                AZStd::vector<AZStd::string> pathADelimited;
                Tokenize(strPathA.c_str(), pathADelimited, AZ_CORRECT_DATABASE_SEPARATOR, true);
                AZStd::vector<AZStd::string> pathBDelimited;
                Tokenize(strPathB.c_str(), pathBDelimited, AZ_CORRECT_DATABASE_SEPARATOR, true);

                //EX: A= p4/Main/Source/GameAssets/gameinfo/Characters
                //    B= p4/Main/Source/GameAssets/gameinfo
                if (bIgnoreStartingPath)
                {
                    for (size_t i = 0; i < pathADelimited.size(); ++i)
                    {
                        if (pathADelimited[i].length() != pathBDelimited[0].length())
                        {
                        }
                        else if (bCaseInsenitive)
                        {
                            if (!azstrnicmp(pathADelimited[i].c_str(), pathBDelimited[0].c_str(), pathADelimited[i].length()))
                            {
                                //we found the first component that is equal
                                //now every component for the rest of B Must Be equal or we fail

                                //if the number of components after this match in A has to be greater
                                //then B or it can not be a sub folder
                                if (pathADelimited.size() - i <= pathBDelimited.size())
                                {
                                    return false;
                                }

                                size_t ii = i + 1;
                                for (size_t jj = 1; jj < pathBDelimited.size(); ++jj)
                                {
                                    if (pathADelimited[ii].length() != pathBDelimited[jj].length())
                                    {
                                        return false;
                                    }
                                    else if (azstrnicmp(pathADelimited[ii].c_str(), pathBDelimited[jj].c_str(), pathADelimited[ii].length()))
                                    {
                                        return false;
                                    }
                                    ii++;
                                }
                                return true;
                            }
                        }
                        else
                        {
                            if (!strncmp(pathADelimited[i].c_str(), pathBDelimited[0].c_str(), pathADelimited[i].length()))
                            {
                                //we found the first component that is equal
                                //now every component for the rest of A Must Be equal or we fail

                                //if the number of components after this match in A has to be greater
                                //then B or it can not be a sub folder
                                if (pathADelimited.size() - i <= pathBDelimited.size())
                                {
                                    return false;
                                }

                                size_t ii = i + 1;
                                for (size_t jj = 1; jj < pathBDelimited.size(); ++jj)
                                {
                                    if (pathADelimited[ii].length() != pathBDelimited[jj].length())
                                    {
                                        return false;
                                    }
                                    else if (azstrnicmp(pathADelimited[ii].c_str(), pathBDelimited[jj].c_str(), pathADelimited[ii].length()))
                                    {
                                        return false;
                                    }
                                    ii++;
                                }
                                return true;
                            }
                        }
                    }
                    return false;
                }
                else
                {
                    if (pathADelimited.size() <= pathBDelimited.size())
                    {
                        return false;
                    }

                    for (size_t i = 0; i < pathBDelimited.size(); ++i)
                    {
                        if (pathADelimited[i].length() != pathBDelimited[i].length())
                        {
                            return false;
                        }
                        else if (bCaseInsenitive)
                        {
                            if (azstrnicmp(pathADelimited[i].c_str(), pathBDelimited[i].c_str(), pathADelimited[i].length()))
                            {
                                return false;
                            }
                        }
                        else
                        {
                            if (strncmp(pathADelimited[i].c_str(), pathBDelimited[i].c_str(), pathADelimited[i].length()))
                            {
                                return false;
                            }
                        }
                    }
                    return true;
                }
            }

            bool IsFileInFolder(const char* pFilePath, const char* pFolder, bool bIncludeSubTree /*= false*/, bool bCaseInsenitive /*= true*/, bool bIgnoreStartingPath /*= true*/)
            {
                (void)bIncludeSubTree;

                if (!pFilePath || !pFolder)
                {
                    return false;
                }

                AZStd::string strFilePath(pFilePath);
                if (!strFilePath.length())
                {
                    return false;
                }

                AZStd::string strFolder(pFolder);
                if (!strFolder.length())
                {
                    return false;
                }

                Path::StripFullName(strFilePath);

                if (bIgnoreStartingPath)
                {
                    Path::StripDrive(strFilePath);
                    Path::StripDrive(strFolder);

                    Strip(strFilePath, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);
                    Strip(strFolder, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);
                }
                else
                {
                    Strip(strFilePath, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);
                    Strip(strFolder, AZ_CORRECT_DATABASE_SEPARATOR, false, true, true);

                    size_t lenFilePath = strFilePath.length();
                    size_t lenFolder = strFolder.length();
                    if (lenFilePath < lenFolder)
                    {
                        return false;
                    }
                    else if (bCaseInsenitive)
                    {
                        if (azstrnicmp(strFilePath.c_str(), strFolder.c_str(), lenFolder))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (strncmp(strFilePath.c_str(), strFolder.c_str(), lenFolder))
                        {
                            return false;
                        }
                    }
                }

                AZStd::vector<AZStd::string> strFilePathDelimited;
                Tokenize(strFilePath.c_str(), strFilePathDelimited, AZ_CORRECT_DATABASE_SEPARATOR, true);
                AZStd::vector<AZStd::string> strFolderDelimited;
                Tokenize(strFolder.c_str(), strFolderDelimited, AZ_CORRECT_DATABASE_SEPARATOR, true);

                //EX: strFilePath= "p4/Main/Source/GameAssets/gameinfo/character"
                //    strFolder= "Main/Source/GameAssets/gameinfo"
                //    = true
                if (bIgnoreStartingPath)
                {
                    bool bFound = false;
                    size_t i = 0;
                    for (i = 0; !bFound && i < strFilePathDelimited.size(); ++i)
                    {
                        if (strFilePathDelimited[i].length() != strFolderDelimited[0].length())
                        {
                        }
                        else if (bCaseInsenitive)
                        {
                            if (!azstrnicmp(strFilePathDelimited[i].c_str(), strFolderDelimited[0].c_str(), strFilePathDelimited[i].length()))
                            {
                                //we found the first component that is equal
                                bFound = true;
                            }
                        }
                        else
                        {
                            if (!strncmp(strFilePathDelimited[i].c_str(), strFolderDelimited[0].c_str(), strFilePathDelimited[i].length()))
                            {
                                //we found the first component that is equal
                                bFound = true;
                            }
                        }
                    }

                    if (bFound)
                    {
                        //we found a match, modify the file path delimited
                        if (i)
                        {
                            strFilePathDelimited.erase(strFilePathDelimited.begin(), strFilePathDelimited.begin() + (i - 1));
                        }
                    }
                    else
                    {
                        for (i = 0; !bFound && i < strFolderDelimited.size(); ++i)
                        {
                            if (strFolderDelimited[i].length() != strFilePathDelimited[0].length())
                            {
                            }
                            else if (bCaseInsenitive)
                            {
                                if (!azstrnicmp(strFolderDelimited[i].c_str(), strFilePathDelimited[0].c_str(), strFolderDelimited[i].length()))
                                {
                                    //we found the first component that is equal
                                    bFound = true;
                                }
                            }
                            else
                            {
                                if (!strncmp(strFolderDelimited[i].c_str(), strFilePathDelimited[0].c_str(), strFolderDelimited[i].length()))
                                {
                                    //we found the first component that is equal
                                    bFound = true;
                                }
                            }
                        }

                        if (bFound)
                        {
                            //we found a match, modify the folder delimited
                            if (i)
                            {
                                strFolderDelimited.erase(strFolderDelimited.begin(), strFolderDelimited.begin() + (i - 1));
                            }
                        }
                        else
                        {
                            return false;
                        }
                    }
                }

                //EX: strFilePath= "Main/Source/GameAssets/gameinfo/character"
                //    strFolder= "Main/Source/GameAssets/gameinfo"
                //    = true

                if (!bIncludeSubTree && strFilePathDelimited.size() != strFolderDelimited.size())
                {
                    return false;
                }

                if (strFilePathDelimited.size() < strFolderDelimited.size())
                {
                    return false;
                }

                for (size_t i = 0; i < strFolderDelimited.size(); ++i)
                {
                    if (strFilePathDelimited[i].length() != strFolderDelimited[i].length())
                    {
                        return false;
                    }
                    else if (bCaseInsenitive)
                    {
                        if (azstrnicmp(strFilePathDelimited[i].c_str(), strFolderDelimited[i].c_str(), strFilePathDelimited[i].length()))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (strncmp(strFilePathDelimited[i].c_str(), strFolderDelimited[i].c_str(), strFilePathDelimited[i].length()))
                        {
                            return false;
                        }
                    }
                }
                return true;
            }
        }//namespace AssetDatabasePath

        namespace Root
        {
            bool Normalize(AZStd::string& inout)
            {
                Strip(inout, AZ_FILESYSTEM_INVALID_CHARACTERS);

    #ifndef AZ_FILENAME_ALLOW_SPACES
                Strip(inout, AZ_SPACE_CHARACTERS);
    #endif // AZ_FILENAME_ALLOW_SPACES

                AZStd::replace(inout.begin(), inout.end(), AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);

                size_t pos = AZStd::string::npos;
                if ((pos = inout.find(AZ_DOUBLE_CORRECT_FILESYSTEM_SEPARATOR)) == 0)
                {
                    AZStd::string temp = inout;
                    if (!Path::GetDrive(temp.c_str(), inout))
                    {
                        return false;
                    }
                    if (!Path::StripDrive(temp))
                    {
                        return false;
                    }

                    Replace(temp, AZ_DOUBLE_CORRECT_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
                    inout += temp;
                }
                else if (pos != AZStd::string::npos)
                {
                    Replace(inout, AZ_DOUBLE_CORRECT_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
                }

                if (LastCharacter(inout.c_str()) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                {
                    Append(inout, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                }

                return IsValid(inout.c_str());
            }

            bool IsValid(const char* in)
            {
                if (!in)
                {
                    return false;
                }

                if (!strlen(in))
                {
                    return false;
                }

                if (Find(in, AZ_FILESYSTEM_INVALID_CHARACTERS) != AZStd::string::npos)
                {
                    return false;
                }

                if (Find(in, AZ_WRONG_FILESYSTEM_SEPARATOR) != AZStd::string::npos)
                {
                    return false;
                }

    #ifndef AZ_FILENAME_ALLOW_SPACES
                if (Find(in, AZ_SPACE_CHARACTERS) != AZStd::string::npos)
                {
                    return false;
                }
    #endif // AZ_FILENAME_ALLOW_SPACES

                if (!Path::HasDrive(in))
                {
                    return false;
                }

                if (LastCharacter(in) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                {
                    return false;
                }

                return true;
            }
        }//namespace Root

        namespace RelativePath
        {
            bool Normalize(AZStd::string& inout)
            {
                Strip(inout, AZ_FILESYSTEM_INVALID_CHARACTERS);

    #ifndef AZ_FILENAME_ALLOW_SPACES
                Strip(inout, AZ_SPACE_CHARACTERS);
    #endif // AZ_FILENAME_ALLOW_SPACES

                AZStd::replace(inout.begin(), inout.end(), AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);

                size_t pos = AZStd::string::npos;
                if ((pos = inout.find(AZ_DOUBLE_CORRECT_FILESYSTEM_SEPARATOR)) == 0)
                {
                    AZStd::string temp = inout;
                    if (!Path::GetDrive(temp.c_str(), inout))
                    {
                        return false;
                    }
                    if (!Path::StripDrive(temp))
                    {
                        return false;
                    }

                    Replace(temp, AZ_DOUBLE_CORRECT_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
                    inout += temp;
                }
                else if (pos != AZStd::string::npos)
                {
                    Replace(inout, AZ_DOUBLE_CORRECT_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
                }

                if (LastCharacter(inout.c_str()) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                {
                    Append(inout, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                }

                if (FirstCharacter(inout.c_str()) == AZ_CORRECT_FILESYSTEM_SEPARATOR)
                {
                    LChop(inout);
                }

                return IsValid(inout.c_str());
            }

            bool IsValid(const char* in)
            {
                if (!in)
                {
                    return false;
                }

                if (!strlen(in))
                {
                    return true;
                }

                if (Find(in, AZ_FILESYSTEM_INVALID_CHARACTERS) != AZStd::string::npos)
                {
                    return false;
                }

                if (Find(in, AZ_WRONG_FILESYSTEM_SEPARATOR) != AZStd::string::npos)
                {
                    return false;
                }

    #ifndef AZ_FILENAME_ALLOW_SPACES
                if (Find(in, AZ_SPACE_CHARACTERS) != AZStd::string::npos)
                {
                    return false;
                }
    #endif // AZ_FILENAME_ALLOW_SPACES

                if (Path::HasDrive(in))
                {
                    return false;
                }

                if (FirstCharacter(in) == AZ_CORRECT_FILESYSTEM_SEPARATOR)
                {
                    return false;
                }

                if (LastCharacter(in) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                {
                    return false;
                }

                return true;
            }
        }//namespace RelativePath

        namespace Path
        {
            bool Normalize(AZStd::string& inout)
            {
                Strip(inout, AZ_FILESYSTEM_INVALID_CHARACTERS);

    #ifndef AZ_FILENAME_ALLOW_SPACES
                Strip(inout, AZ_SPACE_CHARACTERS);
    #endif // AZ_FILENAME_ALLOW_SPACES

                //too big to be a path fail
                if (inout.length() > AZ_MAX_PATH_LEN)
                {
                    return false;
                }

                AZStd::replace(inout.begin(), inout.end(), AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);

                size_t pos = AZStd::string::npos;
                if ((pos = inout.find(AZ_DOUBLE_CORRECT_FILESYSTEM_SEPARATOR)) == 0)
                {
                    AZStd::string temp = inout;
                    if (!Path::GetDrive(temp.c_str(), inout))
                    {
                        return false;
                    }
                    if (!Path::StripDrive(temp))
                    {
                        return false;
                    }

                    Replace(temp, AZ_DOUBLE_CORRECT_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
                    inout += temp;
                }
                else if (pos != AZStd::string::npos)
                {
                    Replace(inout, AZ_DOUBLE_CORRECT_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
                }

                const char* const curDirToken = AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "." AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING;
                const char* const parentDirToken = AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING ".." AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING;

                // Clear out any '/./' references as '.' represents the current directory
                size_t curPos = 0;
                while ((pos = inout.find(curDirToken, curPos)) != AZStd::string::npos)
                {
                    inout.replace(pos, 3, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
                    curPos = pos;
                }

                // If there are any '/../'  patterns, they need to be collapsed against any previous directory that precedes it.
                // If the '..' attempts to collapse beyond the root, then just absorb it. We are following how python handles its
                // normalize operation (os.path.normalize)
                // 
                // e.g.
                //
                // C:\\One\\Two\\..\\Three\\  -> C:\\One\\Three\\
                // C:\\One\\..\\..\\Two\\     -> C:\\Two\\
                // C:\\One\\Two\\Three\\..\\  -> C:\\One\\Two\\
                // 
                if ((pos = inout.find(parentDirToken)) != AZStd::string::npos)
                {
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
                    // Attempt to get a drive letter from the path. If this is the root on a win based file system, then 
                    // a drive letter will be returned, and we wont collapse beyond this point. Otherwise, we wont collapse beyond
                    // the beginning of the path.
                    AZStd::string driveLetter;
                    GetDrive(inout.c_str(), driveLetter);
                    size_t startingPos = driveLetter.length();
#else
                    // For posix-based systems, starting with a '/' will represent an absolute path. Also, 
                    size_t startingPos = inout.find_first_not_of(AZ_CORRECT_FILESYSTEM_SEPARATOR);
                    startingPos = (startingPos != AZStd::string::npos && startingPos != 0) ? startingPos - 1 : 0;
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

                    while ((pos != AZStd::string::npos) && (pos >= startingPos))
                    {
                        // If we found a '/../' pattern, look for the previous '/' before it to start a nullifying replacement
                        size_t startingCollapseIndex = inout.rfind(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING, pos - 1);
                        size_t collapseSubstrSize;

                        if ((startingCollapseIndex == AZStd::string::npos) || (startingCollapseIndex < startingPos))
                        {
                            // There are no matching parent folder to collapse or we've reached the start of the path, so just eat up the current
                            // '../'
                            startingCollapseIndex = startingPos;
                            collapseSubstrSize = 3; // Length of '../'
                        }
                        else
                        {
                            // Based on the previous path separator, we will calculate the size of the path + '../' (ie /a/path/../b -> a/b)
                            // and attempt to replace it with an empty string (ie remove the chunk)
                            collapseSubstrSize = pos - startingCollapseIndex + 3;
                            AZ_Assert(startingCollapseIndex + collapseSubstrSize < inout.length(), "Nullifying substring length greater than length of the original string.");
                        }
                        // Replace the substring (nullifier) with a blank string
                        inout.replace(startingCollapseIndex, collapseSubstrSize, "");
                        pos = inout.find(parentDirToken);
                    }
                }


                return IsValid(inout.c_str());
            }

            bool IsValid(const char* in, bool bHasDrive /*= false*/, bool bHasExtension /*= false*/, AZStd::string* errors /*= nullptr*/)
            {
                //if they gave us a error reporting string empty it.
                if (errors)
                {
                    errors->clear();
                }

                //empty is not a valid path
                if (!in)
                {
                    if (errors)
                    {
                        *errors += "The path is Empty.";
                    }
                    return false;
                }

                //empty is not a valid path
                size_t length = strlen(in);
                if (!length)
                {
                    if (errors)
                    {
                        *errors += "The path is Empty.";
                    }
                    return false;
                }

                //invalid characters
                const char* inEnd = in + length;
                const char* invalidCharactersBegin = AZ_FILESYSTEM_INVALID_CHARACTERS;
                const char* invalidCharactersEnd = invalidCharactersBegin + AZ_ARRAY_SIZE(AZ_FILESYSTEM_INVALID_CHARACTERS);
                if (AZStd::find_first_of(in, inEnd, invalidCharactersBegin, invalidCharactersEnd) != inEnd)
                {
                    if (errors)
                    {
                        *errors += "The path has invalid characters.";
                    }
                    return false;
                }

                //invalid characters
                if (Find(in, AZ_WRONG_FILESYSTEM_SEPARATOR) != AZStd::string::npos)
                {
                    if (errors)
                    {
                        *errors += "The path has wrong separator.";
                    }
                    return false;
                }

    #ifndef AZ_FILENAME_ALLOW_SPACES
                const char* spaceCharactersBegin = AZ_SPACE_CHARACTERS;
                const char* spaceCharactersEnd = spaceCharactersBegin + AZ_ARRAY_SIZE(AZ_SPACE_CHARACTERS);
                if (AZStd::find_first_of(in, inEnd, spaceCharactersBegin, spaceCharactersEnd) != inEnd)
                {
                    if (errors)
                    {
                        *errors += "The path has space characters.";
                    }
                    return false;
                }
    #endif // AZ_FILENAME_ALLOW_SPACES

                //does it have a drive if specified
                if (bHasDrive && !HasDrive(in))
                {
                    if (errors)
                    {
                        *errors += "The path should have a drive. The path [";
                        *errors += in;
                        *errors += "] is invalid.";
                    }
                    return false;
                }

                //does it have and extension if specified
                if (bHasExtension && !HasExtension(in))
                {
                    if (errors)
                    {
                        *errors += "The path should have the a file extension. The path [";
                        *errors += in;
                        *errors += "] is invalid.";
                    }
                    return false;
                }

                //start at the beginning and walk down the characters of the path
                const char* elementStart = in;
                const char* walk = elementStart;
                while (*walk)
                {
                    if (*walk == AZ_CORRECT_FILESYSTEM_SEPARATOR) //is this the correct separator
                    {
                        elementStart = walk;
                    }
    #if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
                    else if (*walk == AZ_FILESYSTEM_DRIVE_SEPARATOR) //is this the drive separator
                    {
                        //A AZ_FILESYSTEM_DRIVE_SEPARATOR character con only occur in the first
                        //component of a valid path. If the elementStart is not GetBufferPtr()
                        //then we have past the first component
                        if (elementStart != in)
                        {
                            if (errors)
                            {
                                *errors += "There is a stray AZ_FILESYSTEM_DRIVE_SEPARATOR = ";
                                *errors += AZ_FILESYSTEM_DRIVE_SEPARATOR;
                                *errors += " found after the first component. The path [";
                                *errors += in;
                                *errors += "] is invalid.";
                            }
                            return false;
                        }
                    }
    #endif
    #ifndef AZ_FILENAME_ALLOW_SPACES
                    else if (*walk == ' ') //is this a space
                    {
                        if (errors)
                        {
                            *errors += "The component [";
                            for (const char* c = elementStart + 1; c != walk; ++c)
                            {
                                *errors += *c;
                            }
                            *errors += "] has a SPACE character. The path [";
                            *errors += in;
                            *errors += "] is invalid.";
                        }
                        return false;
                    }
    #endif

                    ++walk;
                }

    #if !AZ_TRAIT_OS_ALLOW_UNLIMITED_PATH_COMPONENT_LENGTH
                //is this full path longer than AZ_MAX_PATH_LEN (The longest a path with all components can possibly be)?
                if (walk - in > AZ_MAX_PATH_LEN)
                {
                    if (errors != 0)
                    {
                        *errors += "The path [";
                        *errors += in;
                        *errors += "] is over the AZ_MAX_PATH_LEN = ";
                        char buf[64];
                        _itoa_s(AZ_MAX_PATH_LEN, buf, 10);
                        *errors += buf;
                        *errors += " characters total length limit.";
                    }
                    return false;
                }
    #endif

                return true;
            }

            bool ConstructFull(const char* pRootPath, const char* pFileName, AZStd::string& out, bool bNormalize /* = false*/)
            {
                if (!pRootPath)
                {
                    return false;
                }

                if (!pFileName)
                {
                    return false;
                }

                if (!strlen(pRootPath))
                {
                    return false;
                }

                if (!strlen(pFileName))
                {
                    return false;
                }

                if (!HasDrive(pRootPath))
                {
                    return false;
                }

                if (HasDrive(pFileName))
                {
                    return false;
                }

                AZStd::string tempRoot;
                AZStd::string tempFileName;
                if (pRootPath == out.c_str() || pFileName == out.c_str())
                {
                    tempRoot = pRootPath;
                    tempFileName = pFileName;
                    pRootPath = tempRoot.c_str();
                    pFileName = tempFileName.c_str();
                }

                if (bNormalize)
                {
                    AZStd::string rootPath = pRootPath;
                    Root::Normalize(rootPath);

                    AZStd::string fileName = pFileName;
                    Path::Normalize(fileName);

                    Strip(fileName, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);

                    if (!IsRelative(fileName.c_str()))
                    {
                        return false;
                    }

                    out = rootPath;
                    if (LastCharacter(out.c_str()) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                    {
                        Append(out, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                    }
                    out += fileName;
                }
                else
                {
                    if (!IsRelative(pFileName))
                    {
                        return false;
                    }

                    out = pRootPath;
                    if (LastCharacter(out.c_str()) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                    {
                        Append(out, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                    }
                    Append(out, pFileName);
                }

                if (bNormalize)
                {
                    return Normalize(out);
                }
                else
                {
                    return IsValid(out.c_str());
                }
            }

            bool ConstructFull(const char* pRootPath, const char* pFileName, const char* pFileExtension, AZStd::string& out, bool bNormalize /* = false*/)
            {
                if (!pRootPath)
                {
                    return false;
                }

                if (!pFileName)
                {
                    return false;
                }

                if (!strlen(pRootPath))
                {
                    return false;
                }

                if (!strlen(pFileName))
                {
                    return false;
                }

                if (!HasDrive(pRootPath))
                {
                    return false;
                }

                if (HasDrive(pFileName))
                {
                    return false;
                }

                AZStd::string tempRoot;
                AZStd::string tempFileName;
                AZStd::string tempExtension;
                if (pRootPath == out.c_str() || pFileName == out.c_str() || pFileExtension == out.c_str())
                {
                    tempRoot = pRootPath;
                    tempFileName = pFileName;
                    tempExtension = pFileExtension;
                    pRootPath = tempRoot.c_str();
                    pFileName = tempFileName.c_str();
                    pFileExtension = tempExtension.c_str();
                }

                if (pFileExtension && strlen(pFileExtension))
                {
                    if (Find(pFileExtension, AZ_CORRECT_FILESYSTEM_SEPARATOR) != AZStd::string::npos)
                    {
                        return false;
                    }

                    if (Find(pFileExtension, AZ_WRONG_FILESYSTEM_SEPARATOR) != AZStd::string::npos)
                    {
                        return false;
                    }
                }

                if (bNormalize)
                {
                    AZStd::string rootPath = pRootPath;
                    Root::Normalize(rootPath);

                    AZStd::string fileName = pFileName;
                    Path::Normalize(fileName);
                    Strip(fileName, AZ_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR, false, true, true);

                    if (!IsRelative(fileName.c_str()))
                    {
                        return false;
                    }

                    out = rootPath;
                    if (LastCharacter(out.c_str()) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                    {
                        Append(out, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                    }
                    out += fileName;
                }
                else
                {
                    if (!IsRelative(pFileName))
                    {
                        return false;
                    }

                    out = pRootPath;
                    if (LastCharacter(out.c_str()) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                    {
                        Append(out, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                    }
                    Append(out, pFileName);
                }

                if (pFileExtension)
                {
                    ReplaceExtension(out, pFileExtension);
                }

                if (bNormalize)
                {
                    return Normalize(out);
                }
                else
                {
                    return IsValid(out.c_str());
                }
            }

            bool ConstructFull(const char* pRoot, const char* pRelativePath, const char* pFileName, const char* pFileExtension, AZStd::string& out, bool bNormalize /* = false*/)
            {
                if (!pRoot)
                {
                    return false;
                }

                if (!pRelativePath)
                {
                    return false;
                }

                if (!pFileName)
                {
                    return false;
                }

                if (!strlen(pRoot))
                {
                    return false;
                }

                if (!strlen(pFileName))
                {
                    return false;
                }

                if (!HasDrive(pRoot))
                {
                    return false;
                }

                if (HasDrive(pRelativePath))
                {
                    return false;
                }

                if (HasDrive(pFileName))
                {
                    return false;
                }

                AZStd::string tempRoot;
                AZStd::string tempRelativePath;
                AZStd::string tempFileName;
                AZStd::string tempExtension;
                if (pRoot == out.c_str() || pFileName == out.c_str() || pFileExtension == out.c_str())
                {
                    tempRoot = pRoot;
                    tempRelativePath = pRelativePath;
                    tempFileName = pFileName;
                    tempExtension = pFileExtension;
                    pRoot = tempRoot.c_str();
                    pFileName = tempFileName.c_str();
                    pFileExtension = tempExtension.c_str();
                    pRelativePath = tempRelativePath.c_str();
                }

                if (pFileExtension && strlen(pFileExtension))
                {
                    if (Find(pFileExtension, AZ_CORRECT_FILESYSTEM_SEPARATOR) != AZStd::string::npos)
                    {
                        return false;
                    }

                    if (Find(pFileExtension, AZ_WRONG_FILESYSTEM_SEPARATOR) != AZStd::string::npos)
                    {
                        return false;
                    }
                }

                if (bNormalize)
                {
                    AZStd::string root = pRoot;
                    Root::Normalize(root);

                    AZStd::string relativePath = pRelativePath;
                    RelativePath::Normalize(relativePath);

                    if (!IsRelative(relativePath.c_str()))
                    {
                        return false;
                    }

                    AZStd::string fileName = pFileName;
                    Path::Normalize(fileName);

                    Strip(fileName, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);

                    if (!IsRelative(fileName.c_str()))
                    {
                        return false;
                    }

                    out = root;
                    if (LastCharacter(out.c_str()) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                    {
                        Append(out, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                    }
                    out += relativePath;
                    if (LastCharacter(out.c_str()) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                    {
                        Append(out, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                    }
                    out += fileName;
                }
                else
                {
                    if (!IsRelative(pRelativePath))
                    {
                        return false;
                    }

                    if (!IsRelative(pFileName))
                    {
                        return false;
                    }

                    out = pRoot;
                    if (LastCharacter(out.c_str()) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                    {
                        Append(out, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                    }
                    out += pRelativePath;
                    if (LastCharacter(out.c_str()) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                    {
                        Append(out, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                    }
                    out += pFileName;
                }

                if (pFileExtension)
                {
                    ReplaceExtension(out, pFileExtension);
                }

                if (bNormalize)
                {
                    return Normalize(out);
                }
                else
                {
                    return IsValid(out.c_str());
                }
            }

            bool Split(const char* in, AZStd::string* pDstDrive /*= nullptr*/, AZStd::string* pDstPath /*= nullptr*/, AZStd::string* pDstName /*= nullptr*/, AZStd::string* pDstExtension /*= nullptr*/)
            {
                if (!in)
                {
                    return false;
                }

                if (!strlen(in))
                {
                    return false;
                }

                AZStd::string temp(in);
                if (HasDrive(temp.c_str()))
                {
                    StripDrive(temp);
                    if (pDstDrive)
                    {
                        GetDrive(in, *pDstDrive);
                    }
                }
                else
                {
                    if (pDstDrive)
                    {
                        pDstDrive->clear();
                    }
                }

                char b2[256], b3[256], b4[256];
                _splitpath_s(temp.c_str(), nullptr, 0, b2, 256, b3, 256, b4, 256);
                if (pDstPath)
                {
                    *pDstPath = b2;
                }
                if (pDstName)
                {
                    *pDstName = b3;
                }
                if (pDstExtension)
                {
                    *pDstExtension = b4;
                }

                return true;
            }

            bool Join(const char* pFirstPart, const char* pSecondPart, AZStd::string& out, bool bJoinOverlapping /*= false*/, bool bCaseInsenitive /*= true*/, bool bNormalize /*= true*/)
            {
                //null && null
                if (!pFirstPart && !pSecondPart)
                {
                    return false;
                }

                size_t firstPartLen = 0;
                size_t secondPartLen = 0;
                if (pFirstPart)
                {
                    firstPartLen = strlen(pFirstPart);
                }
                if (pSecondPart)
                {
                    secondPartLen = strlen(pSecondPart);
                }

                //0 && 0
                if (!firstPartLen && !secondPartLen)
                {
                    return false;
                }

                //null good
                if (!pFirstPart && pSecondPart)
                {
                    //null 0
                    if (!secondPartLen)
                    {
                        return false;
                    }

                    out = pSecondPart;
                    if (bNormalize)
                    {
                        return Normalize(out);
                    }

                    return true;
                }

                //good null
                if (!pSecondPart && pFirstPart)
                {
                    //0 null
                    if (!firstPartLen)
                    {
                        return false;
                    }

                    out = pFirstPart;
                    if (bNormalize)
                    {
                        return Normalize(out);
                    }

                    return true;
                }

                //0 good
                if (!firstPartLen && pSecondPart)
                {
                    out = pSecondPart;
                    if (bNormalize)
                    {
                        return Normalize(out);
                    }

                    return true;
                }

                //good 0
                if (pFirstPart && !secondPartLen)
                {
                    out = pFirstPart;
                    if (bNormalize)
                    {
                        return Normalize(out);
                    }

                    return true;
                }

                if (HasDrive(pSecondPart))
                {
                    return false;
                }

                AZStd::string tempFirst;
                AZStd::string tempSecond;
                if (pFirstPart == out.c_str() || pSecondPart == out.c_str())
                {
                    tempFirst = pFirstPart;
                    tempSecond = pSecondPart;
                    pFirstPart = tempFirst.c_str();
                    pSecondPart = tempSecond.c_str();
                }

                out = pFirstPart;

                if (bJoinOverlapping)
                {
                    AZStd::string firstPart(pFirstPart);
                    AZStd::string secondPart(pSecondPart);
                    if (bCaseInsenitive)
                    {
                        AZStd::to_lower(firstPart.begin(), firstPart.end());
                        AZStd::to_lower(secondPart.begin(), secondPart.end());
                    }

                    AZStd::vector<AZStd::string> firstPartDelimited;
                    Strip(firstPart, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);
                    Tokenize(firstPart.c_str(), firstPartDelimited, AZ_CORRECT_FILESYSTEM_SEPARATOR, true);
                    AZStd::vector<AZStd::string> secondPartDelimited;
                    Strip(secondPart, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);
                    Tokenize(secondPart.c_str(), secondPartDelimited, AZ_CORRECT_FILESYSTEM_SEPARATOR, true);
                    AZStd::vector<AZStd::string> secondPartNormalDelimited;

                    for (size_t i = 0; i < firstPartDelimited.size(); ++i)
                    {
                        if (firstPartDelimited[i].length() == secondPartDelimited[0].length())
                        {
                            if (!strncmp(firstPartDelimited[i].c_str(), secondPartDelimited[0].c_str(), firstPartDelimited[i].length()))
                            {
                                //we found the first component that is equal
                                //now every component for the rest of firstPart Must Be equal or we fail
                                bool bFailed = false;
                                size_t jj = 1;
                                for (size_t ii = i + 1; !bFailed && ii < firstPartDelimited.size(); ++ii)
                                {
                                    if (jj >= secondPartDelimited.size() || firstPartDelimited[ii].length() != secondPartDelimited[jj].length())
                                    {
                                        bFailed = true;
                                    }
                                    else if (strncmp(firstPartDelimited[ii].c_str(), secondPartDelimited[jj].c_str(), firstPartDelimited[ii].length()))
                                    {
                                        bFailed = true;
                                    }

                                    jj++;
                                }

                                if (!bFailed)
                                {
                                    if (LastCharacter(out.c_str()) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                                    {
                                        out += AZ_CORRECT_FILESYSTEM_SEPARATOR;
                                    }

                                    AZStd::string secondPartNormal(pSecondPart);
                                    Strip(secondPartNormal, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);
                                    Tokenize(secondPartNormal.c_str(), secondPartNormalDelimited, AZ_CORRECT_FILESYSTEM_SEPARATOR, true);

                                    for (; jj < secondPartNormalDelimited.size(); ++jj)
                                    {
                                        out += secondPartNormalDelimited[jj];
                                        out += AZ_CORRECT_FILESYSTEM_SEPARATOR;
                                    }

                                    if (LastCharacter(pSecondPart) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                                    {
                                        RChop(out);
                                    }

                                    return true;
                                }
                            }
                        }
                    }
                }

                if (LastCharacter(pFirstPart) == AZ_CORRECT_FILESYSTEM_SEPARATOR && FirstCharacter(pSecondPart) == AZ_CORRECT_FILESYSTEM_SEPARATOR)
                {
                    Strip(out, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, false, true);
                }

                if (LastCharacter(pFirstPart) != AZ_CORRECT_FILESYSTEM_SEPARATOR && FirstCharacter(pSecondPart) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                {
                    Append(out, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                }

                out += pSecondPart;

                if (bNormalize)
                {
                    return Path::Normalize(out);
                }

                return true;
            }

            bool HasDrive(const char* in, bool bCheckAllFileSystemFormats /*= false*/)
            {
                //no drive if empty
                if (!in)
                {
                    return false;
                }

                if (!strlen(in))
                {
                    return false;
                }

                bool useWinFilePaths = AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS;

                if (useWinFilePaths || bCheckAllFileSystemFormats)
                {
                    //find the first AZ_FILESYSTEM_DRIVE_SEPARATOR
                    if (const char* pFirstDriveSep = strchr(in, AZ_FILESYSTEM_DRIVE_SEPARATOR))
                    {
                        //fail if the drive separator is not the second character
                        if (pFirstDriveSep != in + 1)
                        {
                            return false;
                        }

                        //fail if the first character, the drive letter, is not a letter
                        if (!isalpha(in[0]))
                        {
                            return false;
                        }

                        //find the first Win filesystem separator
                        if (const char* pFirstSep = strchr(in, '\\'))
                        {
                            //fail if the first AZ_FILESYSTEM_DRIVE_SEPARATOR occurs after the first Win filesystem separator
                            if (pFirstDriveSep > pFirstSep)
                            {
                                return false;
                            }
                        }
                        return true;
                    }
                    else if (!strncmp(in, AZ_NETWORK_PATH_START, AZ_NETWORK_PATH_START_SIZE))//see if it has a network start
                    {
                        //find the first Win filesystem separator
                        if (const char* pFirstSep = strchr(in + AZ_NETWORK_PATH_START_SIZE, '\\'))
                        {
                            //fail if the first Win filesystem separator is first character
                            if (pFirstSep == in + AZ_NETWORK_PATH_START_SIZE)
                            {
                                return false;
                            }

                            //fail if the first character after the NETWORK_START isn't alphanumeric
                            if (!isalnum(*(in + AZ_NETWORK_PATH_START_SIZE + 1)))
                            {
                                return false;
                            }
                        }
                        return true;
                    }
                }

                if (!useWinFilePaths || bCheckAllFileSystemFormats)
                {
                    // on other platforms, its got a root if it starts with '/'
                    if (in[0] == '/')
                    {
                        return true;
                    }
                }

                return false;
            }

            bool HasPath(const char* in)
            {
                //no path to strip
                if (!in)
                {
                    return false;
                }

                size_t len = strlen(in);
                if (!len)
                {
                    return false;
                }

                //find the last AZ_CORRECT_FILESYSTEM_SEPARATOR
                if (const char* pLastSep = strrchr(in, AZ_CORRECT_FILESYSTEM_SEPARATOR))
                {
                    if (*(pLastSep + 1) != '\0')
                    {
                        return true;
                    }
                }

                return false;
            }


            bool HasExtension(const char* in)
            {
                //it doesn't have an extension if it's empty
                if (!in)
                {
                    return false;
                }

                size_t len = strlen(in);
                if (!len)
                {
                    return false;
                }

                char b1[256];
                _splitpath_s(in, nullptr, 0, nullptr, 0, nullptr, 0, b1, 256);

                size_t lenExt = strlen(b1);
                if (lenExt == 0)
                {
                    return false;
                }

                return true;
            }

            bool IsExtension(const char* in, const char* pExtension, bool bCaseInsenitive /*= false*/)
            {
                //it doesn't have an extension if it's empty
                if (!in)
                {
                    return false;
                }

                char b1[256] = "";
                _splitpath_s(in, nullptr, 0, nullptr, 0, nullptr, 0, b1, 256);
                char* pExt = b1;

                if (FirstCharacter(pExt) == AZ_FILESYSTEM_EXTENSION_SEPARATOR)
                {
                    pExt++;
                }

                size_t lenExt = strlen(pExt);

                if (pExtension)
                {
                    if (FirstCharacter(pExtension) == AZ_FILESYSTEM_EXTENSION_SEPARATOR)
                    {
                        pExtension++;
                    }

                    size_t lenExtCmp = strlen(pExtension);
                    if (lenExt != lenExtCmp)
                    {
                        return false;
                    }
                }
                else
                {
                    return lenExt == 0;
                }

                if (bCaseInsenitive)
                {
                    return !azstrnicmp(pExt, pExtension, lenExt);
                }
                else
                {
                    return !strncmp(pExt, pExtension, lenExt);
                }
            }

            bool IsRelative(const char* in)
            {
                //not relative if empty
                if (!in)
                {
                    return false;
                }

                size_t len = strlen(in);
                if (!len)
                {
                    return true;
                }

                //not relative if it has a drive
                if (HasDrive(in))
                {
                    return false;
                }

                //not relative if it starts with a AZ_CORRECT_FILESYSTEM_SEPARATOR
                if (FirstCharacter(in) == AZ_CORRECT_FILESYSTEM_SEPARATOR)
                {
                    return false;
                }

                return true;
            }

            bool IsASuperFolderOfB(const char* pathA, const char* pathB, bool bCaseInsenitive /*= true*/, bool bIgnoreStartingPath /*= true*/)
            {
                if (!pathA || !pathB)
                {
                    return false;
                }

                if (pathA == pathB)
                {
                    return false;
                }

                AZStd::string strPathA(pathA);
                if (!strPathA.length())
                {
                    return false;
                }

                AZStd::string strPathB(pathB);
                if (!strPathB.length())
                {
                    return false;
                }

                if (bIgnoreStartingPath)
                {
                    StripDrive(strPathA);
                    StripDrive(strPathB);

                    Strip(strPathA, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);
                    Strip(strPathB, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);
                }
                else
                {
                    Strip(strPathA, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);
                    Strip(strPathB, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);

                    size_t lenA = strPathA.length();
                    size_t lenB = strPathB.length();
                    if (lenA >= lenB)
                    {
                        return false;
                    }
                    else if (bCaseInsenitive)
                    {
                        if (azstrnicmp(strPathA.c_str(), strPathB.c_str(), lenA))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (strncmp(strPathA.c_str(), strPathB.c_str(), lenA))
                        {
                            return false;
                        }
                    }
                }

                AZStd::vector<AZStd::string> pathADelimited;
                Tokenize(strPathA.c_str(), pathADelimited, AZ_CORRECT_FILESYSTEM_SEPARATOR, true);
                AZStd::vector<AZStd::string> pathBDelimited;
                Tokenize(strPathB.c_str(), pathBDelimited, AZ_CORRECT_FILESYSTEM_SEPARATOR, true);

                //EX: A= p4\\Main\\Source\\GameAssets\\gameinfo
                //    B= p4\\Main\\Source\\GameAssets\\gameinfo\\Characters
                if (bIgnoreStartingPath)
                {
                    for (size_t i = 0; i < pathADelimited.size(); ++i)
                    {
                        if (pathADelimited[i].length() != pathBDelimited[0].length())
                        {
                        }
                        else if (bCaseInsenitive)
                        {
                            if (!azstrnicmp(pathADelimited[i].c_str(), pathBDelimited[0].c_str(), pathADelimited[i].length()))
                            {
                                //we found the first component that is equal
                                //now every component for the rest of A Must Be equal or we fail

                                //if the number of components after this match in A are more than
                                //the remaining in B then fail as it can not be a super folder
                                if (pathADelimited.size() - i >= pathBDelimited.size())
                                {
                                    return false;
                                }

                                size_t jj = 1;
                                for (size_t ii = i + 1; ii < pathADelimited.size(); ++ii)
                                {
                                    if (pathADelimited[ii].length() != pathBDelimited[jj].length())
                                    {
                                        return false;
                                    }
                                    else if (azstrnicmp(pathADelimited[ii].c_str(), pathBDelimited[jj].c_str(), pathADelimited[ii].length()))
                                    {
                                        return false;
                                    }
                                    jj++;
                                }
                                return true;
                            }
                        }
                        else
                        {
                            if (!strncmp(pathADelimited[i].c_str(), pathBDelimited[0].c_str(), pathADelimited[i].length()))
                            {
                                //we found the first component that is equal
                                //now every component for the rest of A Must Be equal or we fail

                                //if the number of components after this match in A are more than
                                //the remaining in B then fail as it can not be a super folder
                                if (pathADelimited.size() - i >= pathBDelimited.size())
                                {
                                    return false;
                                }

                                size_t jj = 1;
                                for (size_t ii = i + 1; ii < pathADelimited.size(); ++ii)
                                {
                                    if (pathADelimited[ii].length() != pathBDelimited[jj].length())
                                    {
                                        return false;
                                    }
                                    if (strncmp(pathADelimited[ii].c_str(), pathBDelimited[jj].c_str(), pathADelimited[ii].length()))
                                    {
                                        return false;
                                    }
                                    jj++;
                                }
                                return true;
                            }
                        }
                    }
                    return false;
                }
                else
                {
                    //if the number of components after this match in A are more than
                    //the remaining in B then fail as it can not be a super folder
                    if (pathADelimited.size() >= pathBDelimited.size())
                    {
                        return false;
                    }

                    for (size_t i = 0; i < pathADelimited.size(); ++i)
                    {
                        if (pathADelimited[i].length() != pathBDelimited[i].length())
                        {
                            return false;
                        }
                        else if (bCaseInsenitive)
                        {
                            if (azstrnicmp(pathADelimited[i].c_str(), pathBDelimited[i].c_str(), pathADelimited[i].length()))
                            {
                                return false;
                            }
                        }
                        else
                        {
                            if (strncmp(pathADelimited[i].c_str(), pathBDelimited[i].c_str(), pathADelimited[i].length()))
                            {
                                return false;
                            }
                        }
                    }
                    return true;
                }
            }

            bool IsASubFolderOfB(const char* pathA, const char* pathB, bool bCaseInsenitive /*= true*/, bool bIgnoreStartingPath /*= true*/)
            {
                if (!pathA || !pathB)
                {
                    return false;
                }

                if (pathA == pathB)
                {
                    return false;
                }

                AZStd::string strPathA(pathA);
                if (!strPathA.length())
                {
                    return false;
                }

                AZStd::string strPathB(pathB);
                if (!strPathB.length())
                {
                    return false;
                }

                if (bIgnoreStartingPath)
                {
                    StripDrive(strPathA);
                    StripDrive(strPathB);

                    Strip(strPathA, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);
                    Strip(strPathB, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);
                }
                else
                {
                    Strip(strPathA, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);
                    Strip(strPathB, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);

                    size_t lenA = strPathA.length();
                    size_t lenB = strPathB.length();
                    if (lenA < lenB)
                    {
                        return false;
                    }
                    else if (bCaseInsenitive)
                    {
                        if (azstrnicmp(strPathA.c_str(), strPathB.c_str(), lenB))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (strncmp(strPathA.c_str(), strPathB.c_str(), lenB))
                        {
                            return false;
                        }
                    }
                }

                AZStd::vector<AZStd::string> pathADelimited;
                Tokenize(strPathA.c_str(), pathADelimited, AZ_CORRECT_FILESYSTEM_SEPARATOR, true);
                AZStd::vector<AZStd::string> pathBDelimited;
                Tokenize(strPathB.c_str(), pathBDelimited, AZ_CORRECT_FILESYSTEM_SEPARATOR, true);

                //EX: A= p4\\Main\\Source\\GameAssets\\gameinfo\\Characters
                //    B= p4\\Main\\Source\\GameAssets\\gameinfo
                if (bIgnoreStartingPath)
                {
                    for (size_t i = 0; i < pathADelimited.size(); ++i)
                    {
                        if (pathADelimited[i].length() != pathBDelimited[0].length())
                        {
                        }
                        else if (bCaseInsenitive)
                        {
                            if (!azstrnicmp(pathADelimited[i].c_str(), pathBDelimited[0].c_str(), pathADelimited[i].length()))
                            {
                                //we found the first component that is equal
                                //now every component for the rest of B Must Be equal or we fail

                                //if the number of components after this match in A has to be greater
                                //then B or it can not be a sub folder
                                if (pathADelimited.size() - i <= pathBDelimited.size())
                                {
                                    return false;
                                }

                                size_t ii = i + 1;
                                for (size_t jj = 1; jj < pathBDelimited.size(); ++jj)
                                {
                                    if (pathADelimited[ii].length() != pathBDelimited[jj].length())
                                    {
                                        return false;
                                    }
                                    else if (azstrnicmp(pathADelimited[ii].c_str(), pathBDelimited[jj].c_str(), pathADelimited[ii].length()))
                                    {
                                        return false;
                                    }
                                    ii++;
                                }
                                return true;
                            }
                        }
                        else
                        {
                            if (!strncmp(pathADelimited[i].c_str(), pathBDelimited[0].c_str(), pathADelimited[i].length()))
                            {
                                //we found the first component that is equal
                                //now every component for the rest of A Must Be equal or we fail

                                //if the number of components after this match in A has to be greater
                                //then B or it can not be a sub folder
                                if (pathADelimited.size() - i <= pathBDelimited.size())
                                {
                                    return false;
                                }

                                size_t ii = i + 1;
                                for (size_t jj = 1; jj < pathBDelimited.size(); ++jj)
                                {
                                    if (pathADelimited[ii].length() != pathBDelimited[jj].length())
                                    {
                                        return false;
                                    }
                                    else if (azstrnicmp(pathADelimited[ii].c_str(), pathBDelimited[jj].c_str(), pathADelimited[ii].length()))
                                    {
                                        return false;
                                    }
                                    ii++;
                                }
                                return true;
                            }
                        }
                    }
                    return false;
                }
                else
                {
                    if (pathADelimited.size() <= pathBDelimited.size())
                    {
                        return false;
                    }

                    for (size_t i = 0; i < pathBDelimited.size(); ++i)
                    {
                        if (pathADelimited[i].length() != pathBDelimited[i].length())
                        {
                            return false;
                        }
                        else if (bCaseInsenitive)
                        {
                            if (azstrnicmp(pathADelimited[i].c_str(), pathBDelimited[i].c_str(), pathADelimited[i].length()))
                            {
                                return false;
                            }
                        }
                        else
                        {
                            if (strncmp(pathADelimited[i].c_str(), pathBDelimited[i].c_str(), pathADelimited[i].length()))
                            {
                                return false;
                            }
                        }
                    }
                    return true;
                }
            }

            bool IsFileInFolder(const char* pFilePath, const char* pFolder, bool bIncludeSubTree /*= false*/, bool bCaseInsenitive /*= true*/, bool bIgnoreStartingPath /*= true*/)
            {
                (void)bIncludeSubTree;

                if (!pFilePath || !pFolder)
                {
                    return false;
                }

                AZStd::string strFilePath(pFilePath);
                if (!strFilePath.length())
                {
                    return false;
                }

                AZStd::string strFolder(pFolder);
                if (!strFolder.length())
                {
                    return false;
                }

                StripFullName(strFilePath);

                if (bIgnoreStartingPath)
                {
                    StripDrive(strFilePath);
                    StripDrive(strFolder);

                    Strip(strFilePath, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);
                    Strip(strFolder, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);
                }
                else
                {
                    Strip(strFilePath, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);
                    Strip(strFolder, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);

                    size_t lenFilePath = strFilePath.length();
                    size_t lenFolder = strFolder.length();
                    if (lenFilePath < lenFolder)
                    {
                        return false;
                    }
                    else if (bCaseInsenitive)
                    {
                        if (azstrnicmp(strFilePath.c_str(), strFolder.c_str(), lenFolder))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (strncmp(strFilePath.c_str(), strFolder.c_str(), lenFolder))
                        {
                            return false;
                        }
                    }
                }

                AZStd::vector<AZStd::string> strFilePathDelimited;
                Tokenize(strFilePath.c_str(), strFilePathDelimited, AZ_CORRECT_FILESYSTEM_SEPARATOR, true);
                AZStd::vector<AZStd::string> strFolderDelimited;
                Tokenize(strFolder.c_str(), strFolderDelimited, AZ_CORRECT_FILESYSTEM_SEPARATOR, true);

                //EX: strFilePath= "p4\\Main\\Source\\GameAssets\\gameinfo\\character"
                //    strFolder= "Main\\Source\\GameAssets\\gameinfo"
                //    = true
                if (bIgnoreStartingPath)
                {
                    bool bFound = false;
                    size_t i = 0;
                    for (i = 0; !bFound && i < strFilePathDelimited.size(); ++i)
                    {
                        if (strFilePathDelimited[i].length() != strFolderDelimited[0].length())
                        {
                        }
                        else if (bCaseInsenitive)
                        {
                            if (!azstrnicmp(strFilePathDelimited[i].c_str(), strFolderDelimited[0].c_str(), strFilePathDelimited[i].length()))
                            {
                                //we found the first component that is equal
                                bFound = true;
                            }
                        }
                        else
                        {
                            if (!strncmp(strFilePathDelimited[i].c_str(), strFolderDelimited[0].c_str(), strFilePathDelimited[i].length()))
                            {
                                //we found the first component that is equal
                                bFound = true;
                            }
                        }
                    }

                    if (bFound)
                    {
                        //we found a match, modify the file path delimited
                        if (i)
                        {
                            strFilePathDelimited.erase(strFilePathDelimited.begin(), strFilePathDelimited.begin() + (i - 1));
                        }
                    }
                    else
                    {
                        for (i = 0; !bFound && i < strFolderDelimited.size(); ++i)
                        {
                            if (strFolderDelimited[i].length() != strFilePathDelimited[0].length())
                            {
                            }
                            else if (bCaseInsenitive)
                            {
                                if (!azstrnicmp(strFolderDelimited[i].c_str(), strFilePathDelimited[0].c_str(), strFolderDelimited[i].length()))
                                {
                                    //we found the first component that is equal
                                    bFound = true;
                                }
                            }
                            else
                            {
                                if (!strncmp(strFolderDelimited[i].c_str(), strFilePathDelimited[0].c_str(), strFolderDelimited[i].length()))
                                {
                                    //we found the first component that is equal
                                    bFound = true;
                                }
                            }
                        }

                        if (bFound)
                        {
                            //we found a match, modify the folder delimited
                            if (i)
                            {
                                strFolderDelimited.erase(strFolderDelimited.begin(), strFolderDelimited.begin() + (i - 1));
                            }
                        }
                        else
                        {
                            return false;
                        }
                    }
                }

                //EX: strFilePath= "Main\\Source\\GameAssets\\gameinfo\\character"
                //    strFolder= "Main\\Source\\GameAssets\\gameinfo"
                //    = true

                if (!bIncludeSubTree && strFilePathDelimited.size() != strFolderDelimited.size())
                {
                    return false;
                }

                if (strFilePathDelimited.size() < strFolderDelimited.size())
                {
                    return false;
                }

                for (size_t i = 0; i < strFolderDelimited.size(); ++i)
                {
                    if (strFilePathDelimited[i].length() != strFolderDelimited[i].length())
                    {
                        return false;
                    }
                    else if (bCaseInsenitive)
                    {
                        if (azstrnicmp(strFilePathDelimited[i].c_str(), strFolderDelimited[i].c_str(), strFilePathDelimited[i].length()))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (strncmp(strFilePathDelimited[i].c_str(), strFolderDelimited[i].c_str(), strFilePathDelimited[i].length()))
                        {
                            return false;
                        }
                    }
                }
                return true;
            }

            bool StripDrive(AZStd::string& inout)
            {
                //no drive to strip
                if (!HasDrive(inout.c_str()))
                {
                    return false;
                }

    #if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

                //find the first AZ_FILESYSTEM_DRIVE_SEPARATOR
                size_t pos = inout.find_first_of(AZ_FILESYSTEM_DRIVE_SEPARATOR);
                if (pos != AZStd::string::npos)
                {
                    //make a new string that starts after the first AZ_FILESYSTEM_DRIVE_SEPARATOR
                    RKeep(inout, pos);
                }
                else
                {
                    pos = inout.find_first_of(AZ_CORRECT_FILESYSTEM_SEPARATOR, AZ_NETWORK_PATH_START_SIZE);
                    if (pos != AZStd::string::npos)
                    {
                        //make a new string that starts at the first AZ_CORRECT_FILESYSTEM_SEPARATOR after the NETWORK_START
                        //note we don't have to check the pointer as it has already past the HasDrive() check
                        RKeep(inout, pos, true);
                    }
                    else
                    {
                        AZ_Assert(false, "Passed HasDrive() test but found no drive...");
                    }
                }
    #endif
                return true;
            }

            void StripPath(AZStd::string& inout)
            {
                char b1[MAX_NAME_LEN], b2[MAX_NAME_LEN];
                _splitpath_s(inout.c_str(), nullptr, 0, nullptr, 0, b1, MAX_NAME_LEN, b2, MAX_NAME_LEN);
                inout = AZStd::string::format("%s%s", b1, b2);
            }

            void StripFullName(AZStd::string& inout)
            {
                char b1[MAX_NAME_LEN], b2[AZ_MAX_PATH_LEN];
                _splitpath_s(inout.c_str(), b1, MAX_NAME_LEN, b2, AZ_MAX_PATH_LEN, nullptr, 0, nullptr, 0);
                inout = AZStd::string::format("%s%s", b1, b2);
            }

            void StripExtension(AZStd::string& inout)
            {
                char b1[MAX_NAME_LEN], b2[AZ_MAX_PATH_LEN], b3[MAX_NAME_LEN];
                _splitpath_s(inout.c_str(), b1, MAX_NAME_LEN, b2, AZ_MAX_PATH_LEN, b3, MAX_NAME_LEN, nullptr, 0);
                inout = AZStd::string::format("%s%s%s", b1, b2, b3);
            }

            bool StripComponent(AZStd::string& inout, bool bLastComponent /* = false*/)
            {
                if (!bLastComponent)
                {
                    //Note: Directories can have any legal filename character including
                    //AZ_FILESYSTEM_EXTENSION_SEPARATOR 's in their names
                    //we define the first component of a path as anything
                    //before and including the first AZ_CORRECT_FILESYSTEM_SEPARATOR
                    //i.e. "c:\\root\\parent\\child\\name<.ext>" => "c:\\"

                    //strip starting separators
                    size_t pos = inout.find_first_not_of(AZ_CORRECT_FILESYSTEM_SEPARATOR);
                    pos = inout.find_first_of(AZ_CORRECT_FILESYSTEM_SEPARATOR, pos);
                    if (pos != AZStd::string::npos)
                    {
                        //the next component starts at the next AZ_CORRECT_FILESYSTEM_SEPARATOR
                        RKeep(inout, pos);

                        //take care of the case when only a AZ_CORRECT_FILESYSTEM_SEPARATOR remains, it should just clear
                        if (inout.length() == 1 && FirstCharacter(inout.c_str()) == AZ_CORRECT_FILESYSTEM_SEPARATOR)
                        {
                            inout.clear();
                        }

                        return true;
                    }

                    if (inout.length())
                    {
                        inout.clear();
                        return true;
                    }

                    return false;
                }
                else
                {
                    //Note: Directories can have any legal filename character including
                    //AZ_FILESYSTEM_EXTENSION_SEPARATOR 's in their names
                    //we define the last component of a path as the Name
                    //so anything after and including the last AZ_CORRECT_FILESYSTEM_SEPARATOR
                    //i.e. root\\parent\\child\\name<.ext> => name<.ext>

                    //strip ending separators
                    Strip(inout, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, false, true);

                    //the next component starts after the next last AZ_CORRECT_FILESYSTEM_SEPARATOR
                    size_t pos = inout.find_last_of(AZ_CORRECT_FILESYSTEM_SEPARATOR);
                    if (pos != AZStd::string::npos)
                    {
                        LKeep(inout, pos);
                        //take care of the case when only a AZ_CORRECT_FILESYSTEM_SEPARATOR remains, it should just clear
                        if (inout.length() == 1 && FirstCharacter(inout.c_str()) == AZ_CORRECT_FILESYSTEM_SEPARATOR)
                        {
                            inout.clear();
                        }

                        return true;
                    }

                    //it doesn't have a AZ_CORRECT_FILESYSTEM_SEPARATOR, empty the string
                    if (inout.length())
                    {
                        inout.clear();
                        return true;
                    }

                    return false;
                }
            }

            bool StripQuotes(const char* input, char* outputBuffer, size_t outputBufferSize)
            {
                size_t inputLen = strlen(input); 

                if (inputLen + 1 >= outputBufferSize) // +1 for null-term
                {
                    return false;
                }

                size_t copyLen = inputLen;
                size_t copyOffset = 0;

                while (input[copyOffset] == '"' && copyOffset < inputLen)
                {
                    ++copyOffset;
                }

                while (input[copyLen - 1] == '"' && copyLen > copyOffset) // -1 to generate the correct index offset from the count
                {
                    --copyLen;
                }
                copyLen -= copyOffset;

                auto ret = azstrncpy(outputBuffer, outputBufferSize, input + copyOffset, copyLen);
                bool copyResult = false;
            #if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
                copyResult = (ret == 0);
            #else
                copyResult = (ret != nullptr);
            #endif

                if (!copyResult)
                {
                    return false;
                }

                outputBuffer[copyLen] = '\0';

                return true;
            }

            bool GetDrive(const char* in, AZStd::string& out)
            {
                if (!in)
                {
                    out.clear();
                    return false;
                }

                if (!strlen(in))
                {
                    out.clear();
                    return false;
                }

                // If caller used the same AZStd:string for in and out parameters
                // copy it to another variable and use its memory for in.
                AZStd::string tempIn;
                if (in == out.c_str())
                {
                    tempIn = in;
                    in = tempIn.c_str();
                }
                out.clear();

    #if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

                //find the first AZ_FILESYSTEM_DRIVE_SEPARATOR
                if (const char* pFirstDriveSep = strchr(in, AZ_FILESYSTEM_DRIVE_SEPARATOR))
                {
                    //find the first AZ_CORRECT_FILESYSTEM_SEPARATOR
                    if (const char* pFirstSep = strchr(in, AZ_CORRECT_FILESYSTEM_SEPARATOR))
                    {
                        //fail if the first AZ_FILESYSTEM_DRIVE_SEPARATOR occurs after the first AZ_CORRECT_FILESYSTEM_SEPARATOR
                        if (pFirstDriveSep > pFirstSep)
                        {
                            return false;
                        }

                        //resize to the drive
                        out = in;
                        out.resize(pFirstSep - in);
                    }
                    return true;
                }
                else if (!strncmp(in, AZ_NETWORK_PATH_START, AZ_NETWORK_PATH_START_SIZE))//see if it has a network start
                {
                    //find the first AZ_CORRECT_FILESYSTEM_SEPARATOR
                    if (const char* pFirstSep = strchr(in + AZ_NETWORK_PATH_START_SIZE, AZ_CORRECT_FILESYSTEM_SEPARATOR))
                    {
                        //fail if the first AZ_CORRECT_FILESYSTEM_SEPARATOR is first character
                        if (pFirstSep == in + AZ_NETWORK_PATH_START_SIZE)
                        {
                            return false;
                        }

                        //fail if the first character after the NETWORK_START isn't alphanumeric
                        if (!isalnum(*(in + AZ_NETWORK_PATH_START_SIZE + 1)))
                        {
                            return false;
                        }

                        //resize to the drive
                        out = in;
                        out.resize(pFirstSep - in);
                    }
                    return true;
                }
    #else
                // on other platforms, its got a root if it starts with '/'
                if (in[0] == '/')
                {
                    return true;
                }
    #endif
                return false;
            }

            bool GetFullPath(const char* in, AZStd::string& out)
            {
                if (!in)
                {
                    return false;
                }

                if (!strlen(in))
                {
                    return false;
                }

                AZStd::string tempIn;
                if (in == out.c_str())
                {
                    tempIn = in;
                    in = tempIn.c_str();
                }

                char b1[256], b2[256];
                _splitpath_s(in, b1, 256, b2, 256, nullptr, 0, nullptr, 0);
                out = AZStd::string::format("%s%s", b1, b2);
                return !out.empty();
            }

            bool GetFolderPath(const char* in, AZStd::string& out)
            {
                if (!in)
                {
                    return false;
                }

                if (!strlen(in))
                {
                    return false;
                }

                AZStd::string tempIn;
                if (in == out.c_str())
                {
                    tempIn = in;
                    in = tempIn.c_str();
                }

                char b1[256];
                _splitpath_s(in, nullptr, 0, b1, 256, nullptr, 0, nullptr, 0);
                out = b1;
                return !out.empty();
            }

            bool GetFolder(const char* in, AZStd::string& out, bool bFirst /* = false*/)
            {
                if (!in)
                {
                    return false;
                }

                if (!strlen(in))
                {
                    return false;
                }

                AZStd::string tempIn;
                if (in == out.c_str())
                {
                    tempIn = in;
                    in = tempIn.c_str();
                }

                if (!bFirst)
                {
                    out = in;
                    StripFullName(out);
                    Strip(out, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, false, true);
                    RKeep(out, out.find_last_of(AZ_CORRECT_FILESYSTEM_SEPARATOR));
                    return !out.empty();
                }
                else
                {
                    //EX: "C:\\p4\\game\\info\\some.file"
                    out = in;
                    StripDrive(out);

                    //EX: "\\p4\\game\\info\\some.file"
                    //EX: "p4\\game\\info\\some.file"
                    size_t posFirst = out.find_first_not_of(AZ_CORRECT_FILESYSTEM_SEPARATOR);
                    if (posFirst != AZStd::string::npos)
                    {
                        if (posFirst)
                        {
                            RKeep(out, posFirst, true);
                        }
                        //EX: "p4\\game\\info\\some.file"

                        size_t posSecond = out.find_first_of(AZ_CORRECT_FILESYSTEM_SEPARATOR, posFirst);
                        if (posSecond != AZStd::string::npos)
                        {
                            LKeep(out, posSecond);
                        }

                        return !out.empty();
                    }

                    //no characters except perhaps AZ_CORRECT_FILESYSTEM_SEPARATOR ia a fail clear it and return
                    out.clear();
                    return false;
                }
            }

            bool GetFullFileName(const char* in, AZStd::string& out)
            {
                if (!in)
                {
                    return false;
                }

                if (!strlen(in))
                {
                    return false;
                }

                AZStd::string tempIn;
                if (in == out.c_str())
                {
                    tempIn = in;
                    in = tempIn.c_str();
                }

                char b1[256], b2[256];
                _splitpath_s(in, nullptr, 0, nullptr, 0, b1, 256, b2, 256);
                out = AZStd::string::format("%s%s", b1, b2);
                return !out.empty();
            }

            bool GetFileName(const char* in, AZStd::string& out)
            {
                if (!in)
                {
                    return false;
                }

                if (!strlen(in))
                {
                    return false;
                }

                AZStd::string tempIn;
                if (in == out.c_str())
                {
                    tempIn = in;
                    in = tempIn.c_str();
                }

                char b1[256];
                _splitpath_s(in, nullptr, 0, nullptr, 0, b1, 256, nullptr, 0);
                out = b1;
                return !out.empty();
            }

            bool GetExtension(const char* in, AZStd::string& out, bool includeDot)
            {
                if (!in)
                {
                    return false;
                }

                if (!strlen(in))
                {
                    return false;
                }

                AZStd::string tempIn;
                if (in == out.c_str())
                {
                    tempIn = in;
                    in = tempIn.c_str();
                }

                char b1[256];
                _splitpath_s(in, nullptr, 0, nullptr, 0, nullptr, 0, b1, 256);
                if (includeDot)
                {
                    out = b1;
                }
                else if (b1[0] != '\0')
                {
                    out = b1 + 1; // skip dot
                }
                return !out.empty();
            }

            void ReplaceDrive(AZStd::string& inout, const char* newDrive)
            {
                StripDrive(inout);
                if (!newDrive)
                {
                    return;
                }

                if (!strlen(newDrive))
                {
                    return;
                }

                if (FirstCharacter(inout.c_str()) == AZ_CORRECT_FILESYSTEM_SEPARATOR && LastCharacter(newDrive) == AZ_CORRECT_FILESYSTEM_SEPARATOR)
                {
                    Strip(inout, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, false);
                }

                if (FirstCharacter(inout.c_str()) != AZ_CORRECT_FILESYSTEM_SEPARATOR && LastCharacter(newDrive) != AZ_CORRECT_FILESYSTEM_SEPARATOR)
                {
                    Prepend(inout, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                }

                Prepend(inout, newDrive);
            }

            void ReplaceFullName(AZStd::string& inout, const char* pFileName /* = nullptr*/, const char* pFileExtension /* = nullptr*/)
            {
                //strip the full file name if it has one
                StripFullName(inout);
                if (pFileName)
                {
                    Append(inout, pFileName);
                }
                if (pFileExtension)
                {
                    ReplaceExtension(inout, pFileExtension);
                }
            }

            void ReplaceExtension(AZStd::string& inout, const char* newExtension /* = nullptr*/)
            {
                //strip the extension if it has one
                StripExtension(inout);

                //treat this as a strip
                if (!newExtension)
                {
                    return;
                }

                if (!strlen(newExtension))
                {
                    return;
                }

                //tolerate not having a AZ_FILESYSTEM_EXTENSION_SEPARATOR
                if (FirstCharacter(newExtension) != AZ_FILESYSTEM_EXTENSION_SEPARATOR)
                {
                    Append(inout, AZ_FILESYSTEM_EXTENSION_SEPARATOR);
                }

                //append the new extension
                Append(inout, newExtension);
            }

            size_t NumComponents(const char* in)
            {
                //0 components if its empty
                if (!in)
                {
                    return 0;
                }

                AZStd::string temp = in;
                if (temp.empty())
                {
                    return 0;
                }

                //strip AZ_CORRECT_FILESYSTEM_SEPARATOR(s) from the end
                Strip(temp, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true, true);

                //pass starting separators
                //if there is nothing but separators there are 0 components
                size_t pos = temp.find_first_not_of(AZ_CORRECT_FILESYSTEM_SEPARATOR);

                //inc every time we hit a AZ_CORRECT_FILESYSTEM_SEPARATOR
                size_t componentCount = 0;
                while (pos != AZStd::string::npos)
                {
                    componentCount++;
                    pos = temp.find_first_of(AZ_CORRECT_FILESYSTEM_SEPARATOR, pos);
                    if (pos != AZStd::string::npos)
                    {
                        pos++;
                    }
                }

                return componentCount;
            }

            bool GetComponent(const char* in, AZStd::string& out, size_t nthComponent /*= 1*/, bool bReverse /*= false*/)
            {
                if (!in)
                {
                    return false;
                }

                if (!nthComponent)
                {
                    return false;
                }

                if (!strlen(in))
                {
                    return false;
                }

                out = in;

                if (!bReverse)
                {
                    //pass starting separators to the first character
                    //if it doesn't have anything but separators then fail
                    size_t startPos = out.find_first_not_of(AZ_CORRECT_FILESYSTEM_SEPARATOR);
                    if (startPos == AZStd::string::npos)
                    {
                        return false;
                    }

                    //find the next separator after the first non separator
                    //if it doesn't have one then its a file name
                    //don't alter anything and return true
                    size_t endPos = out.find_first_of(AZ_CORRECT_FILESYSTEM_SEPARATOR, startPos);
                    if (endPos == AZStd::string::npos)
                    {
                        return true;
                    }

                    //start and end represent the start and end of the first component
                    size_t componentCount = 1;
                    while (componentCount < nthComponent)
                    {
                        startPos = endPos + 1;

                        //inc every time we hit a AZ_CORRECT_FILESYSTEM_SEPARATOR
                        endPos = out.find_first_of(AZ_CORRECT_FILESYSTEM_SEPARATOR, startPos);
                        if (endPos == AZStd::string::npos)
                        {
                            if (componentCount == nthComponent - 1)
                            {
                                RKeep(out, startPos, true);
                                return true;
                            }
                            else
                            {
                                return false; //nth component does not exists
                            }
                        }

                        componentCount++;
                    }

                    out = out.substr(startPos, endPos - startPos + 1);
                }
                else
                {
                    //pass starting separators
                    //if it doesn't have anything but separators then fail
                    size_t endPos = out.find_last_not_of(AZ_CORRECT_FILESYSTEM_SEPARATOR);
                    if (endPos == AZStd::string::npos)
                    {
                        return false;
                    }

                    //find the next separator before the last non separator
                    //if it doesn't have one then its a file name
                    //don't alter anything and return true
                    size_t startPos = out.find_last_of(AZ_CORRECT_FILESYSTEM_SEPARATOR, endPos);
                    if (startPos == AZStd::string::npos)
                    {
                        return true;
                    }

                    //start and end represent the start and end of the last component
                    size_t componentCount = 1;
                    while (componentCount < nthComponent)
                    {
                        endPos = startPos - 1;

                        //inc every time we hit a AZ_CORRECT_FILESYSTEM_SEPARATOR
                        startPos = out.find_last_of(AZ_CORRECT_FILESYSTEM_SEPARATOR, endPos);
                        if (startPos == AZStd::string::npos)
                        {
                            if (componentCount == nthComponent - 1)
                            {
                                LKeep(out, endPos + 1, true);
                                return true;
                            }
                            else
                            {
                                return false; //nth component does not exists
                            }
                        }

                        componentCount++;
                    }

                    out = out.substr(startPos + 1, endPos - startPos + 1);
                }
                return true;
            }
        } // namespace Path

        namespace Json
        {
            /*
            According to http://www.ecma-international.org/publications/files/ECMA-ST/ECMA-404.pdf:
            A string is a sequence of Unicode code points wrapped with quotation marks (U+0022). All characters may be
            placed within the quotation marks except for the characters that must be escaped: quotation mark (U+0022),
            reverse solidus (U+005C), and the control characters U+0000 to U+001F.
            */
            AZStd::string& ToEscapedString(AZStd::string& inout)
            {
                size_t strSize = inout.size();

                for (size_t i = 0; i < strSize; ++i)
                {
                    char character = inout[i];

                    // defaults to 1 if it hits any cases except default
                    size_t jumpChar = 1;
                    switch (character)
                    {
                    case '"':
                        inout.insert(i, "\\");
                        break;

                    case '\\':
                        inout.insert(i, "\\");
                        break;

                    case '/':
                        inout.insert(i, "\\");
                        break;

                    case '\b':
                        inout.replace(i, i + 1, "\\b");
                        break;

                    case '\f':
                        inout.replace(i, i + 1, "\\f");
                        break;

                    case '\n':
                        inout.replace(i, i + 1, "\\n");
                        break;

                    case '\r':
                        inout.replace(i, i + 1, "\\r");
                        break;

                    case '\t':
                        inout.replace(i, i + 1, "\\t");
                        break;

                    default:
                        /*
                        Control characters U+0000 to U+001F may be represented as a six - character sequence : a reverse solidus,
                        followed by the lowercase letter u, followed by four hexadecimal digits that encode the code point.
                        */
                        if (character >= '\x0000' && character <= '\x001f')
                        {
                            // jumping "\uXXXX" characters
                            jumpChar = 6;

                            AZStd::string hexStr = AZStd::string::format("\\u%04x", static_cast<int>(character));
                            inout.replace(i, i + 1, hexStr);
                        }
                        else
                        {
                            jumpChar = 0;
                        }
                    }

                    i += jumpChar;
                    strSize += jumpChar;
                }

                return inout;
            }
        } // namespace Json

        namespace Base64
        {
            static const char base64pad = '=';

            static const char c_base64Table[] =
            {
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz"
                "0123456789+/"
            };

            static const AZ::u8 c_inverseBase64Table[] =
            {
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
                0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
                0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
                0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
                0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff,
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
            };

            bool IsValidEncodedChar(const char encodedChar)
            {
                return c_inverseBase64Table[static_cast<size_t>(encodedChar)] != 0xff;
            }

            AZStd::string Encode(const AZ::u8* in, const size_t size)
            {
                /*
                figure retrieved from the Base encoding rfc https://tools.ietf.org/html/rfc4648
                +--first octet--+-second octet--+--third octet--+
                |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
                +-----------+---+-------+-------+---+-----------+
                |5 4 3 2 1 0|5 4 3 2 1 0|5 4 3 2 1 0|5 4 3 2 1 0|
                +--1.index--+--2.index--+--3.index--+--4.index--+
                */
                AZStd::string result;

                const size_t remainder = size % 3;
                const size_t alignEndSize = size - remainder;
                const AZ::u8* encodeBuf = in;
                size_t encodeIndex = 0;
                for (; encodeIndex < alignEndSize; encodeIndex += 3)
                {
                    encodeBuf = &in[encodeIndex];

                    result.push_back(c_base64Table[(encodeBuf[0] & 0xfc) >> 2]);
                    result.push_back(c_base64Table[((encodeBuf[0] & 0x03) << 4) | ((encodeBuf[1] & 0xf0) >> 4)]);
                    result.push_back(c_base64Table[((encodeBuf[1] & 0x0f) << 2) | ((encodeBuf[2] & 0xc0) >> 6)]);
                    result.push_back(c_base64Table[encodeBuf[2] & 0x3f]);
                }

                encodeBuf = &in[encodeIndex];
                if (remainder == 2)
                {
                    result.push_back(c_base64Table[(encodeBuf[0] & 0xfc) >> 2]);
                    result.push_back(c_base64Table[((encodeBuf[0] & 0x03) << 4) | ((encodeBuf[1] & 0xf0) >> 4)]);
                    result.push_back(c_base64Table[((encodeBuf[1] & 0x0f) << 2)]);
                    result.push_back(base64pad);
                }
                else if (remainder == 1)
                {
                    result.push_back(c_base64Table[(encodeBuf[0] & 0xfc) >> 2]);
                    result.push_back(c_base64Table[(encodeBuf[0] & 0x03) << 4]);
                    result.push_back(base64pad);
                    result.push_back(base64pad);
                }

                return result;
            }

            bool Decode(AZStd::vector<AZ::u8>& out, const char* in, const size_t size)
            {
                if (size % 4 != 0)
                {
                    AZ_Warning("StringFunc", size % 4 == 0, "Base 64 encoded data length must be multiple of 4");
                    return false;
                }

                AZStd::vector<AZ::u8> result;
                result.reserve(size * 3 / 4);
                const char* decodeBuf = in;
                size_t decodeIndex = 0;
                for (; decodeIndex < size; decodeIndex += 4)
                {
                    decodeBuf = &in[decodeIndex];
                    //Check if each character is a valid Base64 encoded character 
                    {
                        // First Octet
                        if (!IsValidEncodedChar(decodeBuf[0]))
                        {
                            AZ_Warning("StringFunc", false, "Invalid Base64 encoded text at offset %tu", AZStd::distance(in, &decodeBuf[0]));
                            return false;
                        }
                        if (!IsValidEncodedChar(decodeBuf[1]))
                        {
                            AZ_Warning("StringFunc", false, "Invalid Base64 encoded text at offset %tu", AZStd::distance(in, &decodeBuf[1]));
                            return false;
                        }

                        result.push_back((c_inverseBase64Table[static_cast<size_t>(decodeBuf[0])] << 2) | ((c_inverseBase64Table[static_cast<size_t>(decodeBuf[1])] & 0x30) >> 4));
                    }

                    {
                        // Second Octet
                        if (decodeBuf[2] == base64pad)
                        {
                            break;
                        }

                        if (!IsValidEncodedChar(decodeBuf[2]))
                        {
                            AZ_Warning("StringFunc", false, "Invalid Base64 encoded text at offset %tu", AZStd::distance(in, &decodeBuf[2]));
                            return false;
                        }

                        result.push_back(((c_inverseBase64Table[static_cast<size_t>(decodeBuf[1])] & 0x0f) << 4) | ((c_inverseBase64Table[static_cast<size_t>(decodeBuf[2])] & 0x3c) >> 2));
                    }

                    {
                        // Third Octet
                        if (decodeBuf[3] == base64pad)
                        {
                            break;
                        }

                        if (!IsValidEncodedChar(decodeBuf[3]))
                        {
                            AZ_Warning("StringFunc", false, "Invalid Base64 encoded text at offset %tu", AZStd::distance(in, &decodeBuf[3]));
                            return false;
                        }

                        result.push_back(((c_inverseBase64Table[static_cast<size_t>(decodeBuf[2])] & 0x03) << 6) | (c_inverseBase64Table[static_cast<size_t>(decodeBuf[3])] & 0x3f));
                    }
                }

                out = AZStd::move(result);
                return true;
            }
        }

        namespace Utf8
        {
            bool CheckNonAsciiChar(const AZStd::string& in)
            {
                for (int i = 0; i < in.length(); ++i)
                {
                    char byte = in[i];
                    if (byte & 0x80)
                    {
                        return true;
                    }
                }
                return false;
            }
        }
    } // namespace StringFunc
} // namespace AZ
