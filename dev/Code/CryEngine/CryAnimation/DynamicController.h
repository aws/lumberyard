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
#ifndef CRYINCLUDE_CRYANIMATION_DYNAMICCONTROLLER_H
#define CRYINCLUDE_CRYANIMATION_DYNAMICCONTROLLER_H
#pragma once

struct IDynamicController;
struct ICharacterInstance;

class CDynamicControllerJoint
    : public IDynamicController
{
public:
    CDynamicControllerJoint(const CDefaultSkeleton* parentSkeleton, uint32 dynamicControllerName);
    virtual ~CDynamicControllerJoint()
    {}

    virtual IDynamicController::e_dynamicControllerType GetType() const { return e_dynamicControllerType_Joint; }
    virtual float GetValue(const ICharacterInstance& charInstance) const;

private:
    uint32 m_nJointIndex;
};

#endif // CRYINCLUDE_CRYANIMATION_DYNAMICCONTROLLER_H