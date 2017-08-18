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

// Description : Undo for Python function (PySetCVar)


#ifndef CRYINCLUDE_EDITOR_UNDOCVAR_H
#define CRYINCLUDE_EDITOR_UNDOCVAR_H
#pragma once


#include "Util/BoostPythonHelpers.h"

class CUndoCVar
    : public IUndoObject
{
public:
    CUndoCVar(const char* pCVarName, const QString& pUndoDescription = "Set CVar");

protected:
    int GetSize();
    QString GetDescription();
    void Undo(bool bUndo);
    void Redo();

private:
    SPyWrappedProperty m_undo;
    SPyWrappedProperty m_redo;
    const char* m_CVarName;
    QString m_undoDescription;
};

#endif // CRYINCLUDE_EDITOR_UNDOCVAR_H
