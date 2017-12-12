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

// Description : Main dialog of telemetry plug-in


#ifndef CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYDIALOG_H
#define CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYDIALOG_H
#pragma once


//////////////////////////////////////////////////////////////////////////

#include "TelemetryRepository.h"
#include "Controls/RollupCtrl.h"
#include "ConnectionPanel.h"

//////////////////////////////////////////////////////////////////////////

class CTelemetryDialog
    : public CWnd
{
    DECLARE_DYNCREATE(CTelemetryDialog)

public:

    static void RegisterViewClass();

    CTelemetryDialog();

    ~CTelemetryDialog();

protected:

    DECLARE_MESSAGE_MAP()

    afx_msg void OnSize(UINT nType, int cx, int cy);

private:
    CTelemetryRepository m_repository;

    CRollupCtrl m_rollupCtrl;
    CConnectionPanel m_connectionPanel;
};

//////////////////////////////////////////////////////////////////////////

#endif // CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYDIALOG_H
