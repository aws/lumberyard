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

// Description : Implementation of path drawing object


#ifndef CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYPATH_H
#define CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYPATH_H
#pragma once


//////////////////////////////////////////////////////////////////////////
#include "TelemetryObjects.h"
#include "Util/XmlArchive.h"
#include "Util/Variable.h"
#include "Objects/BaseObject.h"
//////////////////////////////////////////////////////////////////////////

class CTelemetryPathObject
    : public CBaseObject
{
public:
    DECLARE_DYNCREATE(CTelemetryPathObject)

    // BaseObject
    virtual void Display(DisplayContext& dc);
    virtual void GetBoundBox(AABB& box);
    virtual void GetLocalBounds(AABB& box);
    virtual bool HitTest(HitContext& hc);
    virtual bool HitTestRect(HitContext& hc);

    // Own methods
    void FinalConstruct(CTelemetryRepository* repo, CTelemetryTimeline* timeline);
    CTelemetryTimeline& getPath();

protected:
    CTelemetryPathObject();
    ~CTelemetryPathObject();
    virtual void Done();
    virtual void DeleteThis();

private:
    CTelemetryRepository* m_repository;
    CTelemetryTimeline* m_path;

    typedef std::vector<CSmartVariable<CString> > TVarVector;
    TVarVector m_variables;
};

//////////////////////////////////////////////////////////////////////////

class CTelemetryPathObjectClassDesc
    : public CObjectClassDesc
{
public:
    REFGUID ClassID()
    {
        // {dd4eef86-7a7f-4d29-b402-74844fb26a20}
        static const GUID guid = {
            0xdd4eef86, 0x7a7f, 0x4d29, { 0xb4, 0x02, 0x74, 0x84, 0x4f, 0xb2, 0x6a, 0x20 }
        };
        return guid;
    }
    ObjectType GetObjectType() { return OBJTYPE_TELEMETRY; };
    const char* ClassName() { return "TelemetryPath"; };
    const char* Category() { return "Misc"; };
    CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CTelemetryPathObject); };
    int GameCreationOrder() { return 500; };
};

//////////////////////////////////////////////////////////////////////////

#endif // CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYPATH_H
