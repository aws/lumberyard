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

#ifndef CRYINCLUDE_CRYCOMMON_IGAMEREF_H
#define CRYINCLUDE_CRYCOMMON_IGAMEREF_H
#pragma once



struct IGame;
struct IGameRef
{
    IGameRef()
        : m_ptr(0) {}
    IGameRef(IGame** ptr)
        : m_ptr(ptr) {};
    ~IGameRef() {};

    IGame* operator ->() const { return m_ptr ? *m_ptr : 0; };
    operator IGame*() const {
        return m_ptr ? *m_ptr : 0;
    };
    IGameRef& operator =(IGame** ptr) { m_ptr = ptr; return *this; };

private:
    IGame** m_ptr;
};

#endif // CRYINCLUDE_CRYCOMMON_IGAMEREF_H
