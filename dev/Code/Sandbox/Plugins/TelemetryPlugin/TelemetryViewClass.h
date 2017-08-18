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

#ifndef CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYVIEWCLASS_H
#define CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYVIEWCLASS_H
#pragma once


#include "TelemetryDialog.h"
#include "Include/IViewPane.h"
#include "Util/RefCountBase.h"


class CTelemetryViewClass
    : public TRefCountBase<IViewPaneClass>
{
    virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };

    virtual REFGUID ClassID()
    {
        // {50087104-84c4-44a8-a733-2d23809f453d}
        static const GUID guid =
        {
            0x50087104, 0x84c4, 0x44a8, { 0xa7, 0x33, 0x2d, 0x23, 0x80, 0x9f, 0x45, 0x3d }
        };
        return guid;
    }

    virtual const char* ClassName() { return "Telemetry"; }

    virtual const char* Category() { return "Telemetry"; }

    virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CTelemetryDialog); }

    virtual const char* GetPaneTitle() { return "Telemetry"; }

    virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; }

    virtual CRect GetPaneRect() { return CRect(200, 200, 600, 500); }

    virtual bool SinglePane() { return false; }

    virtual bool WantIdleUpdate() { return true; }
};

#endif // CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYVIEWCLASS_H
