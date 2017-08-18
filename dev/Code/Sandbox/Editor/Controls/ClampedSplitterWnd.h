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

// Description : A splitter window which clamps at minimum size rather than
//               culling panes smaller than minimum (MFC default)


#ifndef __clampedsplitterwnd_h__
#define __clampedsplitterwnd_h__

#if _MSC_VER > 1000
#pragma once
#endif

// CClampedSplitterWnd

class CClampedSplitterWnd
    : public CXTSplitterWnd
{
    DECLARE_DYNAMIC(CClampedSplitterWnd)

public:
    CClampedSplitterWnd() {};
    virtual ~CClampedSplitterWnd() {};

protected:
    DECLARE_MESSAGE_MAP()

    // Overrides for standard CSplitterWnd behaviour
    virtual void TrackRowSize(int y, int row);
    virtual void TrackColumnSize(int x, int col);

private:
    // Attempts to resize panels but will not allow resizing smaller than min. Will bunch up multiple panels to try and meet resize requirements
    // offset: The position of the splitter relative to the entry at index
    // index: The index of the dataset which is to the left of the splitter
    // numEntries: The number of entries in this dataset
    // dataSet: The dataset (rows / cols)
    void ClampMovement(const int offset, const int index, const int numEntries, CRowColInfo* dataSet);

    // Attempts to shrink a panel, will not shrink smaller than minimum size
    // requestedShrink: The amount we want to reduce from the panel
    // accumulatedMovement: We add the amount we reduced from this panel to accumulatedMovement
    void RequestPanelShrink(const int index, int& requestedShrink, int& accumulatedMovement, CRowColInfo* dataSet);
};

#endif // __clampedsplitterwnd_h__