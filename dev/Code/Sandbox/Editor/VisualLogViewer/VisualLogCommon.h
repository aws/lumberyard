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

#ifndef CRYINCLUDE_EDITOR_VISUALLOGVIEWER_VISUALLOGCOMMON_H
#define CRYINCLUDE_EDITOR_VISUALLOGVIEWER_VISUALLOGCOMMON_H
#pragma once

#include <QFont>
#include <QMap>


enum EVLogPreviewState
{
    eVLPS_Uninitialized,
    eVLPS_Active,
    eVLPS_Playing,
};

enum EVLogDlgMsgLparam
{
    eVLDML_CheckValidFolder = 1,
    eVLDML_FrameChanged,
    eVLDML_Invalidate,
    eVLDML_ForceInvalidate,
    eVLDML_ForceDword = 0xFFFFFFFF,
};

enum EVLogDlgMsgWparam
{
    eVLDMW_View = 0x01,
    eVLDMW_Text = 0x02,
    eVLDMW_Both = 0x0F,
};

struct SVLogFrameTxtData
{
    SVLogFrameTxtData(QString str = "", int nCol = 0, const QColor& clr = QColor(0, 0, 0), int nSize = 0, bool bSync = false)
    {
        this->str       = str;
        this->nCol  = nCol;
        this->clr       = clr;
        this->nSize = nSize;
        this->bSync = bSync;
    }

    QString     str;        // One line of text
    int             nCol;       // The column to display the text in
    QColor    clr;        // Color of the display text
    int             nSize;  // The font size (identifier)
    bool            bSync;  // Sync column heights after displaying this element
};


struct SVLogFrameTxtDataEx
{
    SVLogFrameTxtDataEx()
        : strFileName("")
        , fFrameTime(0.f)
        , lst(NULL)
    {
    }

    ~SVLogFrameTxtDataEx()
    {
        delete lst;
    }

    void Set(const QString& str, float f, QList<SVLogFrameTxtData>* l)
    {
        strFileName = str;
        fFrameTime  = f;
        lst                 = l;
    }

    QList<SVLogFrameTxtData>* lst;      // list of the strings to display for this frame
    QString strFileName;
    float       fFrameTime;
};


struct SVLogCommonData
{
    static const float fFONT_MUL;
    static const QString strFONT;

    // Construction & destruction
    SVLogCommonData(void)
    {
        eState              = eVLPS_Uninitialized;
        strFolder           = "";
        nCurFrame           = -1;
        nLastFrame      = -1;
        bUpdateTxt      = true;
        bUpdateImages   = true;
        bKeepAspect     = true;
        clrBack             = QColor(0, 0, 0);
        strLog              = "";
        strLogParams    = "";
        nMaxCol             = 0;
        bHasText            = false;
        bHasImages      = false;

        defVals.str = "defaults";
    }

    ~SVLogCommonData(void)
    {
        DestroyFonts();
    }

    // Operations
    void DestroyFonts(void)
    {
        bHasText = false;
        mapFonts.clear();
        mapHeights.clear();
    }

    // Member data
    EVLogPreviewState                       eState;                 // State of the visual log viewer
    QString                                         strFolder;          // Working folder
    int                                                 nCurFrame;          // Current frame
    int                                                 nLastFrame;         // Last frame
    QStringList                vecFiles;               // Image files in the directory
    bool                                                bUpdateTxt;         // Indicates whether text output should be updated as frames change
    bool                                                bUpdateImages;  // Indicates whether image view should be updated as frames change
    bool                                                bKeepAspect;        // Indicates whether frames should be displayed using their original aspect ratio
    QImage                                 img;                        // The image being displayed currently
    QColor                                        clrBack;                // The background color of the view
    // Logs
    QString                                         strLog;                 // The log file
    QString                                         strLogParams;       // The log_params file
    int                                                 nMaxCol;                // The maximum column number

    SVLogFrameTxtData                       defVals;                // Default values for text display

    bool                                                                bHasImages;     // Images have been successfully read
    bool                                                                bHasText;           // Text has been successfully read
    QMap<int, QFont>        mapFonts;           // Fonts of various sizes (precreated for use)
    QMap<int, int>          mapHeights;         // Heights of the fonts
    QVector<SVLogFrameTxtDataEx>  arrFrameTxt;     // <SVLogFrameTxtDataEx-text_data_for_the_frame>
};

#endif // CRYINCLUDE_EDITOR_VISUALLOGVIEWER_VISUALLOGCOMMON_H
