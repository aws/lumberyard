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

// Description : This interface provides access to the particle editor functionality.
//               from the particle editor plugin

#ifndef CRYINCLUDE_EDITOR_INCLUDE_IPARTICLEEDITOR_H
#define CRYINCLUDE_EDITOR_INCLUDE_IPARTICLEEDITOR_H
#pragma once
struct IParticleEditor
    : public IUnknown
{
    DEFINE_UUID(0x12FF6D98, 0x253B, 0x4F43, 0x85, 0xBA, 0xFF, 0x9F, 0x22, 0x51, 0x26, 0x6A)

    virtual void Open() = 0;
    virtual void Select(QString libraryName, CBaseLibraryItem* pItem) = 0;

    //////////////////////////////////////////////////////////////////////////
    // IUnknown
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) { return E_NOINTERFACE; };
    virtual ULONG STDMETHODCALLTYPE AddRef() { return 0; };
    virtual ULONG STDMETHODCALLTYPE Release() { return 0; };
    //////////////////////////////////////////////////////////////////////////
};
#endif // CRYINCLUDE_EDITOR_INCLUDE_IPARTICLEEDITOR_H
