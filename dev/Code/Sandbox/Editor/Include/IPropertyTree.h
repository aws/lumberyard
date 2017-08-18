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

#ifndef CRYINCLUDE_EDITOR_INCLUDE_IPROPERTYTREE_H
#define CRYINCLUDE_EDITOR_INCLUDE_IPROPERTYTREE_H
#pragma once

#include <functor.h>

#include "Include/IEditorClassFactory.h"

namespace Serialization
{
    class IArchive;
    struct SStruct;
    struct SContextLink;
}

struct IPropertyTree
    : public CWnd
{
    virtual BOOL Create(CWnd* parent, const RECT& rect) = 0;
    virtual void Release() = 0;

    virtual void Attach(Serialization::SStruct& ser) = 0;
    virtual void Detach() = 0;
    virtual void Revert() = 0;
    virtual void SetExpandLevels(int expandLevels) = 0;
    virtual void SetCompact(bool compact) = 0;
    virtual void SetArchiveContext(Serialization::SContextLink* context) = 0;
    virtual void SetHideSelection(bool hideSelection) = 0;
    virtual void SetSizeChangeHandler(const Functor0& callback) = 0;
    virtual void SetPropertyChangeHandler(const Functor0& callback) = 0;
    virtual SIZE ContentSize() const = 0;

    virtual void Serialize(Serialization::IArchive& ar) = 0;
};


#endif // CRYINCLUDE_EDITOR_INCLUDE_IPROPERTYTREE_H
