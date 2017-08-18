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

#ifndef CRYINCLUDE_EDITOR_INCLUDE_IPROPERTYTREEDIALOG_H
#define CRYINCLUDE_EDITOR_INCLUDE_IPROPERTYTREEDIALOG_H
#pragma once

struct HWND__;
typedef HWND__* HWND;

namespace serialization
{
    class IArchive;
    struct SStruct;
};

struct IPropertyTreeDialog
    : public CObject
{
    virtual void Release() = 0;
    virtual bool Edit(Serialization::SStruct& ser, const char* title, HWND parentWindow, const char* stateFilename = 0) = 0;
};

inline bool EditInPropertyTree(Serialization::SStruct& ser, const char* title, HWND parentWindow, const char* stateFilename = 0)
{
    IClassDesc* desc = GetIEditor()->GetClassFactory()->FindClass("PropertyTreeDialog");
    if (!desc)
    {
        return false;
    }

    CObject* object = desc->GetRuntimeClass()->CreateObject();
    if (!object)
    {
        return false;
    }

    IPropertyTreeDialog* propertyTreeDialog = static_cast<IPropertyTreeDialog*>(object);

    bool result = propertyTreeDialog->Edit(ser, title, parentWindow, stateFilename);

    propertyTreeDialog->Release();

    return result;
}


#endif // CRYINCLUDE_EDITOR_INCLUDE_IPROPERTYTREEDIALOG_H
