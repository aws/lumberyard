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

// Description : AVI files reader.


#ifndef CRYINCLUDE_CRYSYSTEM_AVI_READER_H
#define CRYINCLUDE_CRYSYSTEM_AVI_READER_H
#pragma once

#include "IAVI_Reader.h"

struct tCaptureAVI_VFW;

//////////////////////////////////////////////////////////////////////////
// AVI file reader.
//////////////////////////////////////////////////////////////////////////
class CAVI_Reader
    : public IAVI_Reader
{
public:
    CAVI_Reader();
    ~CAVI_Reader();

    bool    OpenFile(const char* szFilename);
    void    CloseFile();

    int     GetWidth();
    int     GetHeight();
    int     GetFPS();

    // if no frame is passed, it will retrieve the current one and advance one frame.
    // If a frame is specified, it will get that one.
    // Notice the "const", don't override this memory!
    const unsigned char* QueryFrame(int nFrame = -1);

    int     GetFrameCount();
    int     GetAVIPos();
    void    SetAVIPos(int nFrame);


private:

    void    PrintAVIError(int hr);

    void        InitCapture_VFW();
    int         OpenAVI_VFW(const char* filename);

    tCaptureAVI_VFW* m_capture;
};

#endif // CRYINCLUDE_CRYSYSTEM_AVI_READER_H



