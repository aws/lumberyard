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

// Description : Debug history interface

#ifndef CRYINCLUDE_CRYCOMMON_IDEBUGHISTORY_H
#define CRYINCLUDE_CRYCOMMON_IDEBUGHISTORY_H
#pragma once

struct IDebugHistory
{
    // <interfuscator:shuffle>
    virtual ~IDebugHistory(){}
    virtual void SetVisibility(bool show) = 0;

    virtual void SetName(const char* newName) = 0;

    virtual void SetupLayoutAbs(float leftx, float topy, float width, float height, float margin) = 0;
    virtual void SetupLayoutRel(float leftx, float topy, float width, float height, float margin) = 0;
    virtual void SetupScopeExtent(float outermin, float outermax, float innermin, float innermax) = 0;
    virtual void SetupScopeExtent(float outermin, float outermax) = 0;
    //    virtual void SetupGrid(int x, int y) = 0;
    virtual void SetupColors(ColorF curvenormal, ColorF curveclamped, ColorF box, ColorF gridline, ColorF gridnumber, ColorF name) = 0;
    virtual void SetGridlineCount(int nGridlinesX, int nGridlinesY) = 0;

    virtual void AddValue(float value) = 0;
    virtual void ClearHistory() = 0;
    // if i don't get a value in a frame, then i'll automatically add this value
    virtual void SetDefaultValue(float x) = 0;
    // </interfuscator:shuffle>
};

struct IDebugHistoryManager
{
    // <interfuscator:shuffle>
    virtual ~IDebugHistoryManager(){}
    virtual IDebugHistory* CreateHistory(const char* id, const char* name = 0) = 0;
    virtual void RemoveHistory(const char* name) = 0;
    virtual IDebugHistory* GetHistory(const char* name) = 0;
    virtual void Clear() = 0;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
    virtual void Release() = 0;

    virtual void LayoutHelper(const char* id, const char* name, bool visible, float minout, float maxout, float minin, float maxin, float x, float y, float w = 1.0f, float h = 1.0f) = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IDEBUGHISTORY_H
