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

// include the required headers
#include "StandardHeaders.h"
#include "MemoryManager.h"

namespace MCore
{
    /**
     * A Unicode character.
     * This class internally represents a character in UTF32 format and allows conversion from and to UTF8 and UTF16.
     */
    class MCORE_API UnicodeCharacter
    {
    public:
        MCORE_INLINE UnicodeCharacter()                                                                             { mCodeUnit = 0; mIndex = MCORE_INVALIDINDEX32; }
        MCORE_INLINE explicit UnicodeCharacter(char character, uint32 index = MCORE_INVALIDINDEX32)                   { InitFromUTF8((char*)&character, index); }
        MCORE_INLINE explicit UnicodeCharacter(uint16 firstUTF16, uint16 secondUTF16 = 0, uint32 index = MCORE_INVALIDINDEX32)  { InitFromUTF16(firstUTF16, secondUTF16, index); }
        MCORE_INLINE explicit UnicodeCharacter(const char* utf8Data, uint32 index = MCORE_INVALIDINDEX32)             { InitFromUTF8(utf8Data, index);}
        MCORE_INLINE explicit UnicodeCharacter(uint32 utf32Value, uint32 index = MCORE_INVALIDINDEX32)                { InitFromUTF32(utf32Value, index); }
        MCORE_INLINE UnicodeCharacter(const UnicodeCharacter& other)                                                { mCodeUnit = other.mCodeUnit; mIndex = other.mIndex; }
        MCORE_INLINE ~UnicodeCharacter() {}

        MCORE_INLINE void SetIndex(uint32 index)                            { mIndex = index; }
        MCORE_INLINE uint32 GetIndex() const                                { return mIndex; }

        uint32 CalcNumRequiredUTF8Bytes() const;

        bool InitFromUTF8(const char* utf8Data, uint32 index = MCORE_INVALIDINDEX32);
        void InitFromUTF16(uint16 first, uint16 second = 0, uint32 index = MCORE_INVALIDINDEX32);
        MCORE_INLINE void InitFromUTF32(uint32 utf32Value, uint32 index = MCORE_INVALIDINDEX32)                       { mCodeUnit = utf32Value; mIndex = index; }

        char* AsUTF8(char* output, uint32* outNumBytes, bool addNullTerminator = false) const;    // outNumBytes does NOT include the null terminator
        uint16* AsUTF16(uint16* output) const;
        MCORE_INLINE uint32 AsUTF32() const                                 { return mCodeUnit; }

        static MCORE_INLINE UnicodeCharacter FromUTF16(uint16 first, uint16 second = 0, uint32 index = MCORE_INVALIDINDEX32)        { return UnicodeCharacter(first, second, index); }
        static MCORE_INLINE UnicodeCharacter FromUTF32(uint32 utf32Value, uint32 index = MCORE_INVALIDINDEX32)                    { return UnicodeCharacter((uint32)utf32Value, index); }
        static MCORE_INLINE UnicodeCharacter FromUTF8(const char* characterData, uint32 index = MCORE_INVALIDINDEX32)             { return UnicodeCharacter(characterData, index); }

        MCORE_INLINE bool operator==(const UnicodeCharacter& other)         { return (mCodeUnit == other.mCodeUnit); }
        MCORE_INLINE bool operator!=(const UnicodeCharacter& other)         { return (mCodeUnit != other.mCodeUnit); }

        static const UnicodeCharacter space;
        static const UnicodeCharacter tab;
        static const UnicodeCharacter endLine;
        static const UnicodeCharacter comma;
        static const UnicodeCharacter dot;
        static const UnicodeCharacter backSlash;
        static const UnicodeCharacter forwardSlash;
        static const UnicodeCharacter semiColon;
        static const UnicodeCharacter colon;
        static const UnicodeCharacter doubleQuote;
        static const UnicodeCharacter dash;

    private:
        uint32  mCodeUnit;  /**< UTF32 code unit. */
        uint32  mIndex;     /**< The start index inside the string buffer. */
    };

    MCORE_INLINE bool operator==(const UnicodeCharacter& charA, const UnicodeCharacter& charB)  { return (charA.AsUTF32() == charB.AsUTF32()); }
    MCORE_INLINE bool operator!=(const UnicodeCharacter& charA, const UnicodeCharacter& charB)  { return (charA.AsUTF32() != charB.AsUTF32()); }
}   // namespace MCore
