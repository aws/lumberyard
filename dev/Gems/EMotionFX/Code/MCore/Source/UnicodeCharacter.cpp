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
#include "UnicodeCharacter.h"
#include "UnicodeStringIterator.h"
#include "LogManager.h"

namespace MCore
{
    // statics
    const UnicodeCharacter UnicodeCharacter::space(' ');
    const UnicodeCharacter UnicodeCharacter::tab('\t');
    const UnicodeCharacter UnicodeCharacter::endLine('\n');
    const UnicodeCharacter UnicodeCharacter::comma(',');
    const UnicodeCharacter UnicodeCharacter::dot('.');
    const UnicodeCharacter UnicodeCharacter::colon(':');
    const UnicodeCharacter UnicodeCharacter::backSlash('\\');
    const UnicodeCharacter UnicodeCharacter::forwardSlash('/');
    const UnicodeCharacter UnicodeCharacter::semiColon(';');
    const UnicodeCharacter UnicodeCharacter::doubleQuote('\"');
    const UnicodeCharacter UnicodeCharacter::dash('-');


    // init from a UTF16 code point
    void UnicodeCharacter::InitFromUTF16(uint16 first, uint16 second, uint32 index)
    {
        if (first >= UnicodeStringIterator::UNICODE_SURHIGH_START && first <= UnicodeStringIterator::UNICODE_SURHIGH_END)
        {
            mCodeUnit = ((first - UnicodeStringIterator::UNICODE_SURHIGH_START) << UnicodeStringIterator::UNICODE_HALFSHIFT) + (second - UnicodeStringIterator::UNICODE_SURLOW_START) + UnicodeStringIterator::UNICODE_HALFBASE;
        }
        else
        {
            mCodeUnit = first;
        }

        mIndex = index;
    }


    // convert to UTF16
    uint16* UnicodeCharacter::AsUTF16(uint16* output) const
    {
        if (mCodeUnit <= UnicodeStringIterator::UNICODE_MAX_BMP)
        {
            output[0] = static_cast<uint16>(mCodeUnit);
            output[1] = 0;
        }
        else
        {
            const uint32 value = mCodeUnit - UnicodeStringIterator::UNICODE_HALFBASE;
            output[0] = (uint16)((value >> UnicodeStringIterator::UNICODE_HALFSHIFT) + UnicodeStringIterator::UNICODE_SURHIGH_START);
            output[1] = (uint16)((value& UnicodeStringIterator::UNICODE_HALFMASK) +UnicodeStringIterator::UNICODE_SURLOW_START);
        }

        return output;
    }


    // convert to UTF8
    char* UnicodeCharacter::AsUTF8(char* output, uint32* outNumBytes, bool addNullTerminator) const
    {
        // get the UTF32 character code
        uint32 ch = AsUTF32();

        // convert that into UTF8
        unsigned char* target = (unsigned char*)output;

        uint16 bytesToWrite = 0;
        const uint32 byteMask = 0xBF;
        const uint32 byteMark = 0x80;

        // figure out how many bytes the result will require
        // turn any illegally large UTF32 things (> Plane 17) into replacement chars
        if (ch < (uint32)0x80)
        {
            bytesToWrite = 1;
        }
        else if (ch < (uint32)0x800)
        {
            bytesToWrite = 2;
        }
        else if (ch < (uint32)0x10000)
        {
            bytesToWrite = 3;
        }
        else if (ch <= UnicodeStringIterator::UNICODE_MAX_LEGAL_UTF32)
        {
            bytesToWrite = 4;
        }
        else
        {
            bytesToWrite = 3;
            ch = UnicodeStringIterator::UNICODE_REPLACEMENT_CHAR;
        }

        target += bytesToWrite;
        switch (bytesToWrite)
        {
        case 4:
            *--target = (uint8)((ch | byteMark) & byteMask);
            ch >>= 6;
        case 3:
            *--target = (uint8)((ch | byteMark) & byteMask);
            ch >>= 6;
        case 2:
            *--target = (uint8)((ch | byteMark) & byteMask);
            ch >>= 6;
        case 1:
            *--target = (uint8) (ch | UnicodeStringIterator::firstByteMark[bytesToWrite]);
        }
        target += bytesToWrite;

        if (addNullTerminator)
        {
            *target = 0;
        }

        *outNumBytes = bytesToWrite;
        return output;
    }



    // init from UTF8
    bool UnicodeCharacter::InitFromUTF8(const char* utf8Data, uint32 index)
    {
        uint32 ch = 0;
        const unsigned char* source = (unsigned char*)utf8Data;
        const unsigned char firstByteValue = *source;
        uint32 extraBytesToRead = (uint32)UnicodeStringIterator::trailingBytesForUTF8[firstByteValue];

        // do this check whether lenient or strict
    #ifdef MCORE_DEBUG
        if (UnicodeStringIterator::CheckIfIsLegalUTF8((uint8*)source, extraBytesToRead + 1) == false)
        {
            MCore::LogWarning("MCore::UnicodeCharacter::InitFromUTF8() - The input data is not valid UTF8!");
            return false;
        }
    #endif

        switch (extraBytesToRead)
        {
        case 0:
            ch += *source++;
            break;
        case 1:
            ch += *source++;
            ch <<= 6;
            ch += *source++;
            break;
        case 2:
            ch += *source++;
            ch <<= 6;
            ch += *source++;
            ch <<= 6;
            ch += *source++;
            break;
        case 3:
            ch += *source++;
            ch <<= 6;
            ch += *source++;
            ch <<= 6;
            ch += *source++;
            ch <<= 6;
            ch += *source++;
            break;
        case 4:
            ch += *source++;
            ch <<= 6;                  // remember, illegal UTF-8
            ch += *source++;
            ch <<= 6;
            ch += *source++;
            ch <<= 6;
            ch += *source++;
            ch <<= 6;
            ch += *source++;
            break;
        case 5:
            ch += *source++;
            ch <<= 6;                  // remember, illegal UTF-8
            ch += *source++;
            ch <<= 6;                  // remember, illegal UTF-8
            ch += *source++;
            ch <<= 6;
            ch += *source++;
            ch <<= 6;
            ch += *source++;
            ch <<= 6;
            ch += *source++;
            break;
        }
        ;

        /*
                switch (extraBytesToRead)
                {
                    case 5: ch += *source++; ch <<= 6; // remember, illegal UTF-8
                    case 4: ch += *source++; ch <<= 6; // remember, illegal UTF-8
                    case 3: ch += *source++; ch <<= 6;
                    case 2: ch += *source++; ch <<= 6;
                    case 1: ch += *source++; ch <<= 6;
                    case 0: ch += *source++;
                    default:;
                }
        */

        ch -= UnicodeStringIterator::offsetsFromUTF8[extraBytesToRead];
        InitFromUTF32(ch, index);

        return true;
    }


    // calculate the number of required bytes for this character when writing to UTF8
    uint32 UnicodeCharacter::CalcNumRequiredUTF8Bytes() const
    {
        const uint32 ch = AsUTF32();

        // figure out how many bytes the result will require
        uint32 bytesToWrite = 0;
        if (ch < (uint32)0x80)
        {
            bytesToWrite = 1;
        }
        else if (ch < (uint32)0x800)
        {
            bytesToWrite = 2;
        }
        else if (ch < (uint32)0x10000)
        {
            bytesToWrite = 3;
        }
        else if (ch < (uint32)0x110000)
        {
            bytesToWrite = 4;
        }
        else
        {
            bytesToWrite = 3; /* UnicodeStringIterator::UNICODE_REPLACEMENT_CHAR */
        }

        return bytesToWrite;
    }
}   // namespace MCore

