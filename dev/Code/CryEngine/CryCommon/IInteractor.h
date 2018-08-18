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

// Description : Interactor interface.


#ifndef CRYINCLUDE_CRYACTION_IINTERACTOR_H
#define CRYINCLUDE_CRYACTION_IINTERACTOR_H
#pragma once


struct IInteractor
    : public IGameObjectExtension
{
    IInteractor() {}

    virtual bool IsUsable(EntityId entityId) const = 0;
    virtual bool IsLocked() const = 0;
    virtual int GetLockIdx() const = 0;
    virtual int GetLockedEntityId() const = 0;
    virtual void SetQueryMethods(char* pMethods) = 0;
    virtual int GetOverEntityId() const = 0;
    virtual int GetOverSlotIdx() const = 0;
};

#endif // CRYINCLUDE_CRYACTION_IINTERACTOR_H
