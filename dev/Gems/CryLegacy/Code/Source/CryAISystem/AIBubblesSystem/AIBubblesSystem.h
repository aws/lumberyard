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

// Description : Central class to Manage the Bubbles messages


#ifndef CRYINCLUDE_CRYAISYSTEM_AIBUBBLESSYSTEM_AIBUBBLESSYSTEM_H
#define CRYINCLUDE_CRYAISYSTEM_AIBUBBLESSYSTEM_AIBUBBLESSYSTEM_H
#pragma once

#include "IAIBubblesSystem.h"

#ifdef CRYAISYSTEM_DEBUG

#include "ISoftCodeMgr.h"

class CAIBubblesSystem
    : public IAIBubblesSystem
    , public ISoftCodeListener
{
public:
    CAIBubblesSystem()
        : m_pBubblesNotifier(NULL) {}
    virtual ~CAIBubblesSystem();

    virtual void QueueMessage(const char* messageName, const SAIBubbleRequest& request);
    virtual void Update();

    virtual void Reset();
    virtual void Init();
    virtual void Dispose();

    // Implements ISoftCodeListener
    virtual void InstanceReplaced(void* pOldInstance, void* pNewInstance);
private:

    void CreateBubblesNotifier();

    IAIBubblesNotifier* m_pBubblesNotifier;

    static CAIBubblesSystem* m_pInstance;
};
#endif // CRYAISYSTEM_DEBUG

#endif // CRYINCLUDE_CRYAISYSTEM_AIBUBBLESSYSTEM_AIBUBBLESSYSTEM_H
