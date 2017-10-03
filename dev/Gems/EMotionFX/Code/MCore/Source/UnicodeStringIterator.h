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
#include "UnicodeCharacter.h"
#include "UnicodeString.h"

namespace MCore
{
    class MCORE_API UnicodeStringIterator
    {
        friend class UnicodeString;
        friend class UnicodeCharacter;
    public:
        enum
        {
            UNICODE_SURHIGH_START       = (uint32)0xD800,
            UNICODE_SURHIGH_END         = (uint32)0xDBFF,
            UNICODE_SURLOW_START        = (uint32)0xDC00,
            UNICODE_SURLOW_END          = (uint32)0xDFFF,
            UNICODE_REPLACEMENT_CHAR    = (uint32)0x0000FFFD,
            UNICODE_MAX_BMP             = (uint32)0x0000FFFF,
            UNICODE_MAX_UTF16           = (uint32)0x0010FFFF,
            UNICODE_MAX_UTF32           = (uint32)0x7FFFFFFF,
            UNICODE_MAX_LEGAL_UTF32     = (uint32)0x0010FFFF,
            UNICODE_HALFSHIFT           = 10,
            UNICODE_HALFBASE            = 0x0010000UL,
            UNICODE_HALFMASK            = 0x3FFUL
        };

        MCORE_INLINE UnicodeStringIterator(const UnicodeString& str);
        MCORE_INLINE UnicodeStringIterator(const char* data, uint32 numCodeUnits);
        MCORE_INLINE UnicodeStringIterator(const char* data);
        MCORE_INLINE ~UnicodeStringIterator();

        MCORE_INLINE void InitFromString(const UnicodeString& str);
        MCORE_INLINE void InitFromCharBuffer(const char* data, uint32 numCodeUnits);

        MCORE_INLINE uint32 GetIndex() const;
        MCORE_INLINE uint32 GetLength() const;
        MCORE_INLINE bool GetHasReachedEnd() const;

        UnicodeCharacter GetNextCharacter();
        void GetNextCharacter(UnicodeCharacter* outCharacter);

        UnicodeCharacter GetPreviousCharacter();
        UnicodeCharacter GetCurrentCharacter() const;
        UnicodeCharacter GetCharacter(uint32 characterIndex) const;

        uint32 FindCharacterCodeUnitIndex(uint32 characterIndex) const;
        uint32 CalcNumCharacters() const;

        void ReverseToPreviousCharacter();
        void ForwardToNextCharacter();
        void SetIndexToEnd();
        void Rewind();
        void SetIndexToCharacter(uint32 characterIndex);
        void SetIndex(uint32 codeUnitIndex);

    private:
        char*       mData;
        uint32      mCodeUnitIndex;
        uint32      mNumCodeUnits;

        static uint32 CalcNumCharacters(const char* utf8String, uint32 numBytes);
        static uint32 CalcNumCharacters(const char* utf8String);

        static const char trailingBytesForUTF8[256];
        static const uint32 offsetsFromUTF8[6];
        static const uint8 firstByteMark[7];

        static bool CheckIfIsLegalUTF8Sequence(const uint8* source, const uint8* sourceEnd);
        static bool CheckIfIsLegalUTF8(const uint8* source, uint32 length);
        static bool ConvertUTF16toUTF32(const uint16** sourceStart, const uint16* sourceEnd, uint32** targetStart, uint32* targetEnd, bool strictConversion);
        static bool ConvertUTF32toUTF16(const uint32** sourceStart, const uint32* sourceEnd, uint16** targetStart, uint16* targetEnd, bool strictConversion);
        static bool ConvertUTF16toUTF8(const uint16** sourceStart, const uint16* sourceEnd, uint8** targetStart, uint8* targetEnd, bool strictConversion);
        static bool ConvertUTF8toUTF16(const uint8** sourceStart, const uint8* sourceEnd, uint16** targetStart, uint16* targetEnd, bool strictConversion);
        static bool ConvertUTF32toUTF8(const uint32** sourceStart, const uint32* sourceEnd, uint8** targetStart, uint8* targetEnd, bool strictConversion);
        static bool ConvertUTF8toUTF32(const uint8** sourceStart, const uint8* sourceEnd, uint32** targetStart, uint32* targetEnd, bool strictConversion);
    };


    // add the inline code
#include "UnicodeStringIterator.inl"
}   // namespace MCore
