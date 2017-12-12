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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include <CryStructPack.h>
#include <AzCore/Casting/lossy_cast.h>

#if NEED_STRUCT_PACK
uint32 StructSize(const CTypeInfo& typeInfo, uint32 limit)
{
    uint32 size = 0;

    for AllSubVars (pVar, typeInfo)
    {
        if (pVar->Offset >= limit)
        {
            break;
        }
        if (pVar->Type.IsType<void*>())
        {
            size += 4 * pVar->ArrayDim;
        }
        else if (pVar->Type.HasSubVars())
        {
            uint32 typeSize = StructSize(pVar->Type);
            if (pVar->ArrayDim * typeSize > limit - pVar->Offset)
            {
                size = limit;
            }
            else
            {
                size += pVar->ArrayDim * typeSize;
            }
        }
        else
        {
            size += azlossy_cast<uint32>(pVar->GetSize());
        }
    }
    return size;
}

uint32 StructUnpack(const CTypeInfo& typeInfo, uint8* ptr, const uint8* buffer,
    uint32 limit, bool expandPointers)
{
    uint32 offset = 0;
    for AllSubVars (pVar, typeInfo)
    {
        if (pVar->Offset >= limit)
        {
            break;
        }
        if (pVar->Type.IsType<void*>())
        {
            if (expandPointers)
            {
                for (uint32 j = 0; j < pVar->ArrayDim; ++j)
                {
                    INT_PTR v = buffer[offset] | (buffer[offset + 1] << 8)
                        | (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
                    memcpy(
                        (uint8*)ptr + pVar->Offset + j * sizeof(void*),
                        &v,
                        sizeof(void*));
                    offset += 4;
                }
            }
            else
            {
                memset((uint8*)ptr + pVar->Offset, 0, pVar->GetSize());
                offset += 4 * pVar->ArrayDim;
            }
        }
        else if (pVar->Type.HasSubVars())
        {
            uint32 varOffset = pVar->Offset;
            for (uint32 j = 0; j < pVar->ArrayDim; ++j)
            {
                offset += StructUnpack(pVar->Type, ptr + varOffset, buffer + offset,
                        limit - varOffset, expandPointers);
                varOffset += azlossy_cast<uint32>(pVar->Type.Size);
                if (varOffset >= limit)
                {
                    break;
                }
            }
        }
        else
        {
            uint32 varSize = azlossy_caster(pVar->GetSize());
            memcpy((uint8*)ptr + pVar->Offset, buffer + offset, varSize);
#ifdef NEED_ENDIAN_SWAP
            SwapEndian((uint8*)ptr + pVar->Offset,
                pVar->ArrayDim, pVar->Type, pVar->Type.Size);
#endif
            offset += varSize;
        }
    }
    return offset;
}

uint32 StructPack(const CTypeInfo& typeInfo, const uint8* ptr, uint8* buffer,
    uint32 limit, bool packPointers)
{
    uint32 offset = 0;
    for AllSubVars (pVar, typeInfo)
    {
        if (pVar->Offset >= limit)
        {
            break;
        }
        if (pVar->Type.IsType<void*>())
        {
            if (packPointers)
            {
                // Note that only the least significant 32 bits of the pointer are
                // packed for storage.
                for (uint32 i = 0; i < pVar->ArrayDim; ++i)
                {
                    INT_PTR v = 0;
                    memcpy(
                        &v,
                        (const uint8*)ptr + pVar->Offset + i * sizeof(void*),
                        sizeof(void*));
                    buffer[offset] = (uint8)(v & 0xff);
                    buffer[offset + 1] = (uint8)((v >> 8) & 0xff);
                    buffer[offset + 2] = (uint8)((v >> 16) & 0xff);
                    buffer[offset + 3] = (uint8)((v >> 24) & 0xff);
                    offset += 4;
                }
            }
            else
            {
                memset(buffer + offset, 0, 4 * pVar->ArrayDim);
                offset += 4 * pVar->ArrayDim;
            }
        }
        else if (pVar->Type.HasSubVars())
        {
            uint32 varOffset = pVar->Offset;
            for (uint32 j = 0; j < pVar->ArrayDim; ++j)
            {
                offset += StructPack(pVar->Type, ptr + varOffset, buffer + offset,
                        limit - varOffset, packPointers);
                varOffset += azlossy_cast<uint32>(pVar->Type.Size);
                if (varOffset >= limit)
                {
                    break;
                }
            }
        }
        else
        {
            uint32 varSize = azlossy_caster(pVar->GetSize());
            memcpy(buffer + offset, (const uint8*)ptr + pVar->Offset, varSize);
#ifdef NEED_ENDIAN_SWAP
            SwapEndian(buffer + offset,
                pVar->ArrayDim, pVar->Type, pVar->Type.Size);
#endif
            offset += varSize;
        }
    }
    return offset;
}
#endif
