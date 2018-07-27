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

#ifndef CRYINCLUDE_CRYACTION_RANGESIGNALINGSYSTEM_RANGESIGNALING_H
#define CRYINCLUDE_CRYACTION_RANGESIGNALINGSYSTEM_RANGESIGNALING_H
#pragma once

#include "IRangeSignaling.h"

class CPersonalRangeSignaling;

class CRangeSignaling
    : public IRangeSignaling
{
public:

    /*$1- Singleton Stuff ----------------------------------------------------*/
    static CRangeSignaling&   ref();
    static bool               Create();
    static void               Shutdown();
    void                      Reset();

    /*$1- IEditorSetGameModeListener -----------------------------------------*/
    void  OnEditorSetGameMode(bool bGameMode);
    void  OnProxyReset(EntityId IdEntity);

    /*$1- Basics -------------------------------------------------------------*/
    void  Init();
    bool  Update(float fElapsedTime);

    /*$1- Utils --------------------------------------------------------------*/
    bool  AddRangeSignal(EntityId IdEntity, float fRadius, float fBoundary, const char* sSignal, IAISignalExtraData* pData = NULL);
    bool  AddTargetRangeSignal(EntityId IdEntity, EntityId IdTarget, float fRadius, float fBoundary, const char* sSignal, IAISignalExtraData* pData = NULL);
    bool  AddAngleSignal(EntityId IdEntity, float fAngle, float fBoundary, const char* sSignal, IAISignalExtraData* pData = NULL);
    bool  DestroyPersonalRangeSignaling(EntityId IdEntity);
    void  ResetPersonalRangeSignaling(EntityId IdEntity);
    void  EnablePersonalRangeSignaling(EntityId IdEntity, bool bEnable);
    void  SetDebug(bool bDebug);
    bool  GetDebug() const;

protected:

    /*$1- Creation and destruction via singleton -----------------------------*/
    CRangeSignaling();
    virtual ~CRangeSignaling();

    /*$1- Utils --------------------------------------------------------------*/
    CPersonalRangeSignaling*  GetPersonalRangeSignaling(EntityId IdEntity) const;
    CPersonalRangeSignaling*  CreatePersonalRangeSignaling(EntityId IdEntity);

private:
    typedef std::map<EntityId, CPersonalRangeSignaling*> MapPersonals;

    /*$1- Members ------------------------------------------------------------*/
    bool m_bInit;
    bool m_bDebug;
    static CRangeSignaling* m_pInstance;
    MapPersonals m_Personals;
};
#endif // CRYINCLUDE_CRYACTION_RANGESIGNALINGSYSTEM_RANGESIGNALING_H
