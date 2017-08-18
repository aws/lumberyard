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

// Description : Implements platform abstracted, programmatically triggered
//               data and code breakpoints


#ifndef CRYINCLUDE_CRYCOMMON_HARDWAREBREAKPOINT_H
#define CRYINCLUDE_CRYCOMMON_HARDWAREBREAKPOINT_H
#pragma once


// needs platform.h included first!
#include <assert.h>


struct HardwareBreakpointBase
{
public:
    enum Type
    {
        TYPE_CODE,
        TYPE_WRITE,
        TYPE_READWRITE
    };

    enum Size
    {
        SIZE_1,
        SIZE_2,
        SIZE_4,
#if defined(WIN32) || defined(WIN64)
        SIZE_8
#endif
    };
};


template <class HardwareBreakpointImpl>
class HardwareBreakpoint
    : public HardwareBreakpointImpl
{
public:
    HardwareBreakpoint()
        : HardwareBreakpointImpl() {}
    ~HardwareBreakpoint() {}

    bool Set(void* thread, void* address, HardwareBreakpointBase::Size size, HardwareBreakpointBase::Type type)
    {
        return HardwareBreakpointImpl::Set(thread, address, size, type);
    }

    bool Clear()
    {
        return HardwareBreakpointImpl::Clear();
    }
};


#if defined(WIN32) || defined(WIN64)

class HardwareBreakpoint_Windows
    : public HardwareBreakpointBase
{
public:
    HardwareBreakpoint_Windows()
        : m_thread(0)
        , m_idx((unsigned char) -1)
    {
    }

    ~HardwareBreakpoint_Windows()
    {
        Clear();
    }

    bool Set(void* thread, void* address, Size size, Type type)
    {
        if (m_thread)
        {
            assert(0);
            return false;
        }

        assert(m_idx == (unsigned char) -1);

        CONTEXT cxt;
        cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS;

        if (GetThreadContext(thread, &cxt))
        {
            for (m_idx = 0; m_idx < 4; ++m_idx)
            {
                if ((cxt.Dr7 & (ULONG_PTR) (1 << (m_idx * 2))) == 0)
                {
                    break;
                }
            }

            if (m_idx < 4)
            {
                switch (m_idx)
                {
                case 0:
                    cxt.Dr0 = (ULONG_PTR) address;
                    break;
                case 1:
                    cxt.Dr1 = (ULONG_PTR) address;
                    break;
                case 2:
                    cxt.Dr2 = (ULONG_PTR) address;
                    break;
                case 3:
                    cxt.Dr3 = (ULONG_PTR) address;
                    break;
                default:
                    assert(0);
                    break;
                }

                SetBits(cxt.Dr7, m_idx * 2, 1, 1);
                SetBits(cxt.Dr7, 16 + m_idx * 4, 2, RealType(type));
                SetBits(cxt.Dr7, 18 + m_idx * 4, 2, RealSize(type != TYPE_CODE ? size : SIZE_1));

                if (SetThreadContext(thread, &cxt))
                {
                    m_thread = thread;
                }
                else
                {
                    m_idx = (unsigned char) -1;
                }
            }
            else
            {
                m_idx = (unsigned char) -1;
                m_thread = 0;
            }
        }

        return m_thread != 0;
    }

    bool Clear()
    {
        bool ret = true;

        if (m_thread)
        {
            assert(m_idx < 4);

            CONTEXT cxt;
            cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS;

            if (GetThreadContext(m_thread, &cxt))
            {
                SetBits(cxt.Dr7, m_idx * 2, 1, 0);

                ret = SetThreadContext(m_thread, &cxt) == TRUE;
            }

            m_idx = (unsigned char) -1;
            m_thread = 0;
        }

        return ret;
    }

private:
    static void SetBits(ULONG_PTR& v, ULONG_PTR lowBit, ULONG_PTR numBits, ULONG_PTR newValue)
    {
        ULONG_PTR mask = ((1 << numBits) - 1) << lowBit;
        v = (v & ~mask) | ((newValue << lowBit) & mask);
    }

    static ULONG_PTR RealType(Type type)
    {
        switch (type)
        {
        case TYPE_CODE:
            return 0;
        case TYPE_WRITE:
            return 1;
        case TYPE_READWRITE:
            return 3;
        default:
            assert(0);
            return 1;
        }
        ;
    }

    static ULONG_PTR RealSize(Size size)
    {
        switch (size)
        {
        case SIZE_1:
            return 0;
        case SIZE_2:
            return 1;
        case SIZE_4:
            return 3;
        case SIZE_8:
            return 2;
        default:
            assert(0);
            return 3;
        }
        ;
    }

private:
    void* m_thread;
    unsigned char m_idx;
};

typedef HardwareBreakpoint<HardwareBreakpoint_Windows> CHardwareBreakpoint;

#else // #if defined(WIN32) || defined(WIN64)

class HardwareBreakpoint_Null
    : public HardwareBreakpointBase
{
public:
    HardwareBreakpoint_Null() {}
    ~HardwareBreakpoint_Null() {}

    bool Set(void* /*thread*/, void* /*address*/, Size /*size*/, Type /*type*/)
    {
        return true;
    }

    bool Clear()
    {
        return true;
    }
};

typedef HardwareBreakpoint<HardwareBreakpoint_Null> CHardwareBreakpoint;

#endif


#endif // CRYINCLUDE_CRYCOMMON_HARDWAREBREAKPOINT_H
