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

// Description : Interface of the VisualLog
//               The VisualLog system captures frames at runtime and associates
//               it with logging data for subsequent offline playback


#ifndef CRYINCLUDE_CRYCOMMON_IVISUALLOG_H
#define CRYINCLUDE_CRYCOMMON_IVISUALLOG_H
#pragma once

struct SVisualLogParams
{
    ColorF color;
    float size;
    int column;
    bool alignColumnsToThis;

    SVisualLogParams()  { Init(); }
    SVisualLogParams(ColorF _color)
    {
        Init();
        this->color = _color;
    }
    SVisualLogParams(ColorF _color, float _size)
    {
        Init();
        this->color = _color;
        this->size = _size;
    }
    SVisualLogParams(ColorF _color, float _size, int _column, bool _align)
    {
        Init();
        this->color = _color;
        this->size = _size;
        this->column = _column;
        this->alignColumnsToThis = _align;
    }

private:
    void Init()
    {
        color = ColorF(1.f, 1.f, 1.f, 1.f);
        size = 2.f;
        column = 1;
        alignColumnsToThis = false;
    }
};

struct IVisualLog
{
    // <interfuscator:shuffle>
    virtual ~IVisualLog(){}
    virtual void Log (const char* format, ...)  PRINTF_PARAMS(2, 3) = 0;
    virtual void Log (const SVisualLogParams& params, const char* format, ...)  PRINTF_PARAMS(3, 4) = 0;

    virtual void Reset() = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IVISUALLOG_H
