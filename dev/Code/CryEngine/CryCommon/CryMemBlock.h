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

#ifndef CRYINCLUDE_CRYCOMMON_CRYMEMBLOCK_H
#define CRYINCLUDE_CRYCOMMON_CRYMEMBLOCK_H
#pragma once


class CryMemBlock
{
public:
    union
    {
        void* pv;
        char* pc;
    };

    uint32 size;
    uint32 ref;
    bool own;

    CryMemBlock()
    {
        pv = 0;
        size = 0;
        ref = 0;
        own = true;
    }

    CryMemBlock(uint32 _size)
    {
        ref = 0;
        pv = 0;
        size = 0;
        Alloc(_size);
    }

    CryMemBlock(void* _pv, uint32 _size)
    {
        ref = 0;
        pv = _pv;
        size = _size;
        own = false;
    }

    ~CryMemBlock()
    {
        if (own)
        {
            Free();
        }
        else
        {
            Detach();
        }
    }

    void AddRef()
    {
        ref++;
    }

    void Release()
    {
        ref--;
        if (!ref)
        {
            delete this;
        }
    }

    void Alloc(uint32 _size)
    {
        if (own)
        {
            Free();
        }
        else
        {
            Detach();
        }

        pv = new byte[_size];
        size = _size;
        own = true;
    }

    void Free()
    {
        if (!own)
        {
            return;
        }

        if (pv)
        {
            delete [] (byte*) pv;
        }

        pv = 0;
        size = 0;
        own = false;
    }

    void Attach(void* _pv, int _size)
    {
        if (own)
        {
            Free();
        }
        else
        {
            Detach();
        }

        pv = _pv;
        size = _size;
        own = false;
    }

    void Detach()
    {
        if (own)
        {
            return;
        }

        pv = 0;
        size = 0;
    }
};

#endif // CRYINCLUDE_CRYCOMMON_CRYMEMBLOCK_H
