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

#ifndef CRYINCLUDE_EDITOR_MEASUREMENTSYSTEM_MEASUREMENTSYSTEM_H
#define CRYINCLUDE_EDITOR_MEASUREMENTSYSTEM_MEASUREMENTSYSTEM_H
#pragma once


#define MEASUREMENT_SYSTEM_WINDOW_NAME "Measurement System Tool"

#include "Objects/RoadObject.h"

#include <QWidget>
#include <QScopedPointer>

class CRoadObjectEnhanced;
class CMeasurementSystem;

namespace Ui {
    class CMeasurementSystemDialog;
}

//////////////////////////////////////////////////////////////////////////
// Measurement System Dialog
class CMeasurementSystemDialog
    : public QWidget
{
    Q_OBJECT
public:
    explicit CMeasurementSystemDialog(QWidget* parent = nullptr);
    ~CMeasurementSystemDialog();

    void UpdateSelectedLength(const float& fNewValue);
    void UpdateTotalLength(const float& fNewValue);

    // you are required to implement this to satisfy the unregister/registerclass requirements on "AzToolsFramework::RegisterViewPane"
    // make sure you pick a unique GUID
    static const GUID& GetClassID()
    {
        // {ae734780-5224-4209-88bf-979e6ce707bc}
        static const GUID guid =
        {
            0xae734780, 0x5224, 0x4209, { 0x88, 0xbf, 0x97, 0x9e, 0x6c, 0xe7, 0x7, 0xbe }
        };
        return guid;
    }

    static void RegisterViewClass();
public slots:
    void UpdateObjectLength();
    void Optimize();

private:
    QScopedPointer<Ui::CMeasurementSystemDialog> ui;
};

//////////////////////////////////////////////////////////////////////////
class CMeasurementSystem
{
public:
    CMeasurementSystem();

    void SetStartPos(const int& iStartPosIndex){ m_startPosIndex = iStartPosIndex; SwitchMeasuringPoint(); };
    void SetEndPos(const int& iEndPosIndex) { m_endPosIndex = iEndPosIndex; SwitchMeasuringPoint(); };
    int GetStartPointIndex(){ return m_startPosIndex; };
    int GetEndPointIndex(){ return m_endPosIndex; };
    // Should "Start" point be set in this click?
    bool ShouldStartPointBecomeSelected(){ return m_firstMeasurePointClicked; };
    bool ProcessedLButtonClick(int endPoint);
    bool ProcessedDblButtonClick(int clickedSegmentPointIndex);
    void ResetSelectedDistance();

    float GetSelectionLength(){ return m_selectionLength; };
    void SetTotalDistance(const float& newSelectionLength){ m_selectionLength = newSelectionLength; };


    void ShutdownMeasurementSystem();
    bool IsMeasurementToolActivated(){ return m_bMeasurementToolActivated; };
    void SetMeasurementToolActivation(bool bActivated) { m_bMeasurementToolActivated = bActivated; };

    // Changes points of distance selection
    void SwitchMeasuringPoint(){ m_firstMeasurePointClicked = !m_firstMeasurePointClicked; };

    CMeasurementSystemDialog* GetMeasurementDialog();

    static CMeasurementSystem& GetMeasurementSystem(){ static CMeasurementSystem oMeasurementSystem; return oMeasurementSystem; }
protected:
    int m_startPosIndex;
    int m_endPosIndex;
    bool m_firstMeasurePointClicked;
    float m_selectionLength;
    bool m_bMeasurementToolActivated;
};
//////////////////////////////////////////////////////////////////////////
class CRoadObjectEnhanced
    : public CRoadObject
{
public:
    void DrawMeasurementSystemInfo(DisplayContext& dc, const QColor& col);
    void DrawCenterLine(const int& startPosIndex, const int& endPosIndex, float& segmentDist, float& totalDist, DisplayContext& dc, bool bDrawMeasurementInfo);
    void DrawSegmentBox(DisplayContext& dc, const int& startPosIndex, const int& endPosIndex, const QColor& col);
    void GetSectorLength(const int& sectorIndex, const int& endIndex, float& sectorLength);
    void GetObjectLength(float& objectLength);
};
//////////////////////////////////////////////////////////////////////////

#endif // CRYINCLUDE_EDITOR_MEASUREMENTSYSTEM_MEASUREMENTSYSTEM_H
