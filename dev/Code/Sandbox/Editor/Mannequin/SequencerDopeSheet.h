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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERDOPESHEET_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERDOPESHEET_H
#pragma once


#include "SequencerDopeSheetBase.h"

/** List of tracks.
*/
class CSequencerDopeSheet
    : public CSequencerDopeSheetBase
{
    Q_OBJECT
public:
    // public stuff.

    CSequencerDopeSheet(QWidget* parent = nullptr);
    ~CSequencerDopeSheet();


protected:
    void DrawTrack(int item, QPainter* painter, const QRect& rcItem) override;
    void DrawKeys(CSequencerTrack* track, QPainter* painter, const QRect& rc, const Range& timeRange, EDSRenderFlags renderFlags) override;
    void DrawNodeItem(CSequencerNode* pAnimNode, QPainter* dc, const QRect& rcItem);

    // Overrides from CSequencerKeys.
    int FirstKeyFromPoint(const QPoint& point, bool exact = false) const override;
    int LastKeyFromPoint(const QPoint& point, bool exact = false) const override;
    void SelectKeys(const QRect& rc) override;

    int NumKeysFromPoint(const QPoint& point) const;
};


#endif // CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERDOPESHEET_H
