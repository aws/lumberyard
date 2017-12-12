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

#ifndef CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_POLYGONSETOPS_BSPTREE2D_H
#define CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_POLYGONSETOPS_BSPTREE2D_H
#pragma once


#include "LineSeg.h"

/**
 * This BSP tree uses the left/right terminology, with 'left' meaning 'inside'.
 */
class BspTree2d
{
public:
    BspTree2d (const LineSegVec& edges);
    ~BspTree2d ();

    BspTree2d* Clone () const;

    void Invert ();

    const BspTree2d* LeftSubtree () const;
    const BspTree2d* RightSubtree () const;

    const LineSegVec& GetPartitionSegments () const;

private:
    BspTree2d ();
    /// Standard copying is forbidden.
    BspTree2d (const BspTree2d&);
    BspTree2d& operator= (const BspTree2d&);

    LineSegVec m_kCoincident;
    BspTree2d* m_rightSubtree;
    BspTree2d* m_leftSubtree;
};

inline const BspTree2d* BspTree2d::LeftSubtree () const
{
    return m_leftSubtree;
}

inline const BspTree2d* BspTree2d::RightSubtree () const
{
    return m_rightSubtree;
}

// ---

/**
 * @brief Splits LineSeg's by a BspTree2d.
 *
 * This slightly unusual device is useful for using a BSP tree to cut a line
 * segment into parts that are 'left' (or 'inside') the tree and those that
 * are 'right' (or 'outside').
 *
 * A splitter is primed with a BSP tree in constructor (the tree can be switched
 * later).  Then its Split() member function can be called (possibly repeatedly)
 * with line segments as arguments.  Pieces of line segments cut up by the BSP
 * tree are kept in the splitter instance and can be retrieved using LeftSegs()
 * and RightSegs() members.
 */
class BspLineSegSplitter
{
    const BspTree2d* m_bsp;
    LineSegVec m_leftSegs;
    LineSegVec m_rightSegs;

    void Split (const LineSeg& seg, const BspTree2d*);
    void SplitByLeftSubtree (const LineSeg&, const BspTree2d*);
    void SplitByRightSubtree (const LineSeg&, const BspTree2d*);
    void SplitByThisNode (const LineSeg&, const BspTree2d*);
public:
    BspLineSegSplitter (const BspTree2d*);

    /// Takes a line segment and splits it.
    void Split (const LineSeg&);

    /// Primes the splitter with a different BspTree2d.
    void ResetBspTree (const BspTree2d*);

    /// Retrieves vector of left/inside segments of the original line segment.
    const LineSegVec& LeftSegs () const;
    /// Retrieves vector of right/outside segments of the original line segment.
    const LineSegVec& RightSegs () const;
};

inline void BspLineSegSplitter::ResetBspTree (const BspTree2d* new_bsp)
{
    m_bsp = new_bsp;
}

inline const LineSegVec& BspLineSegSplitter::LeftSegs () const
{
    return m_leftSegs;
}

inline const LineSegVec& BspLineSegSplitter::RightSegs () const
{
    return m_rightSegs;
}

#endif // CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_POLYGONSETOPS_BSPTREE2D_H

