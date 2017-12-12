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

// Description : Signal entities based on ranges from other entities

#ifndef CRYINCLUDE_CRYACTION_IRANGESIGNALING_H
#define CRYINCLUDE_CRYACTION_IRANGESIGNALING_H
#pragma once

struct IAISignalExtraData;

struct IRangeSignaling
{
    virtual ~IRangeSignaling() {}

    virtual bool  AddRangeSignal(EntityId IdEntity, float fRadius, float fBoundary, const char* sSignal, IAISignalExtraData* pData = NULL) = 0;
    virtual bool  AddTargetRangeSignal(EntityId IdEntity, EntityId IdTarget, float fRadius, float fBoundary, const char* sSignal, IAISignalExtraData* pData = NULL) = 0;
    virtual bool  AddAngleSignal(EntityId IdEntity, float fAngle, float fBoundary, const char* sSignal, IAISignalExtraData* pData = NULL) = 0;
    virtual bool  DestroyPersonalRangeSignaling(EntityId IdEntity) = 0;
    virtual void  ResetPersonalRangeSignaling(EntityId IdEntity) = 0;
    virtual void  EnablePersonalRangeSignaling(EntityId IdEntity, bool bEnable) = 0;
};

#endif // CRYINCLUDE_CRYACTION_IRANGESIGNALING_H
