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

#ifndef CRYINCLUDE_CRYCOMMON_IAVI_READER_H
#define CRYINCLUDE_CRYCOMMON_IAVI_READER_H
#pragma once

//////////////////////////////////////////////////////////////////////////
struct IAVI_Reader
{
    virtual ~IAVI_Reader(){}
    virtual bool    OpenFile(const char* szFilename) = 0;
    virtual void    CloseFile() = 0;

    virtual int     GetWidth() = 0;
    virtual int     GetHeight() = 0;
    virtual int     GetFPS() = 0;

    // if no frame is passed, it will retrieve the current one and advance one frame.
    // If a frame is specified, it will get that one.
    // Notice the "const", don't override this memory!
    virtual const unsigned char* QueryFrame(int nFrame = -1) = 0;

    virtual int     GetFrameCount() = 0;
    virtual int     GetAVIPos() = 0;
    virtual void    SetAVIPos(int nFrame) = 0;
};

#endif // CRYINCLUDE_CRYCOMMON_IAVI_READER_H



