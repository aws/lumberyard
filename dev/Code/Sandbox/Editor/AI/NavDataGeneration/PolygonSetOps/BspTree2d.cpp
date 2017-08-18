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

#include "StdAfx.h"
#include "BspTree2d.h"

#include <list>
#include <limits>

/**
 * @brief Classifies position of the second ctor argument wrt the first argument.
 *
 * Basically just a glorified function call with somewhat complex output
 * information.  Plus, some added security and consistency enforcement
 * in output value handling.
 */
class MutualPosition
{
public:
    enum
    {
        /// Line segment's start is on the left side, the end is on the right.
        LEFT_TO_RIGHT,
        /// Line segment's start is on the right side, the end is on the left.
        RIGHT_TO_LEFT,
        RIGHT,
        LEFT,
        COINCIDENT,
        /// Just for assert()'s.
        NOT_INITIALIZED
    };

    MutualPosition (const LineSeg& cutter, const LineSeg& cuttee);
    int operator() () const;
    const Vector2d& Intersection () const;

private:
    int m_position;
    Vector2d m_intersection;
};

MutualPosition::MutualPosition (const LineSeg& cutter, const LineSeg& cuttee)
    : m_position (NOT_INITIALIZED)
{
    Vector2d cutter_normal = cutter.Normal ();

    double dot0 = cutter_normal.Dot (Vector2d (cuttee.Start() - cutter.Start()));
    double dot1 = cutter_normal.Dot (Vector2d (cuttee.End() - cutter.Start()));

    if (dot0 * dot1 < 0.0)
    {
        double t = dot0 / (dot0 - dot1);
        if (t < 0.99999999)
        {
            if (t > 0.00000001)
            {
                m_intersection = cuttee.Pt(t);
                if (dot1 > 0.0)
                {
                    m_position = RIGHT_TO_LEFT;
                }
                else
                {
                    m_position = LEFT_TO_RIGHT;
                }
                return;
            }
            else
            {
                dot0 = 0.0;
            }
        }
        else
        {
            assert (t <= 1.0);
            dot1 = 0.0;
        }
    }

    if (dot0 > 0.0 || dot1 > 0.0)
    {
        m_position = LEFT;
    }
    else if (dot0 < 0.0 || dot1 < 0.0)
    {
        m_position = RIGHT;
    }
    else
    {
        m_position = COINCIDENT;
    }
}

inline int MutualPosition::operator() () const
{
    assert (m_position != NOT_INITIALIZED);
    return m_position;
}

inline const Vector2d& MutualPosition::Intersection () const
{
    assert (m_position == LEFT_TO_RIGHT || m_position == RIGHT_TO_LEFT);
    return m_intersection;
}


// ---


bool IsClose (double d, double fixed, double tol)
{
    return abs (d - fixed) <= tol;
}

void SnapTo (double& d, double snap, double tol)
{
    if (IsClose (d, snap, tol))
    {
        d = snap;
    }
}

/**
* @brief 1D line segment.
*
* Basically just an interval, but it's useful to think about it as a 1D
* line segment in some contexts.
*/
typedef double Vector1d;
class LineSeg1d
{
    Vector1d m_start;
    Vector1d m_end;
public:
    static const double s_epsilon;

    LineSeg1d (Vector1d start, Vector1d end);
    Vector1d Start () const;
    Vector1d End () const;
    Vector1d Min () const;
    Vector1d Max () const;
    bool IsDirNegative () const;
    bool IsNonTrivial () const;
    void ClampTo (const LineSeg1d& rhs);

    bool operator< (const LineSeg1d& rhs) const;
};

const double LineSeg1d::s_epsilon = 0.000001;

inline LineSeg1d::LineSeg1d (Vector1d start, Vector1d end)
    : m_start (start)
    , m_end (end)
{
}

inline bool LineSeg1d::IsDirNegative () const
{
    return m_end < m_start;
}

inline bool LineSeg1d::IsNonTrivial () const
{
    return abs (m_end - m_start) > s_epsilon;
}

inline Vector1d LineSeg1d::Start () const
{
    return m_start;
}

inline Vector1d LineSeg1d::End () const
{
    return m_end;
}

inline Vector1d LineSeg1d::Min () const
{
    return std::min (m_start, m_end);
}

inline Vector1d LineSeg1d::Max () const
{
    return std::max (m_start, m_end);
}

inline void LineSeg1d::ClampTo (const LineSeg1d& rhs)
{
    Vector1d& minimum = m_start < m_end ? m_start : m_end;
    Vector1d& maximum = m_start > m_end ? m_start : m_end;

    if (minimum < rhs.Min ())
    {
        minimum = rhs.Min ();
    }
    if (maximum > rhs.Max ())
    {
        maximum = rhs.Max ();
    }
}

inline bool LineSeg1d::operator< (const LineSeg1d& rhs) const
{
    Vector1d min = Min ();
    Vector1d rhs_min = rhs.Min ();
    if (min < rhs_min)
    {
        return true;
    }
    else if (min > rhs_min)
    {
        return false;
    }

    if (Max () < rhs.Max ())
    {
        return true;
    }
    return false;
}

/**
* Both args are assumed coincident.  The first arg is interpreted as a 1D
* linear base and the second arg's endpoints are transformed to this base,
* which means that they are expressed as a parameter times 'seg'.
* The result is sanitized, clamped to (0,1) and returned as a 1D line segment
* (an interval on real axis).
*/
LineSeg1d OverlapCoincident (const LineSeg& seg, const LineSeg& coincident)
{
    Vector2d xform = seg.End () - seg.Start ();
    assert (xform.GetLength2 () > 0.0000001);
    xform /= xform.GetLength2 ();
    double coord0 = xform * (coincident.Start () - seg.Start ());
    double coord1 = xform * (coincident.End ()   - seg.Start ());

    SnapTo (coord0, 0.0, LineSeg1d::s_epsilon);
    SnapTo (coord0, 1.0, LineSeg1d::s_epsilon);
    SnapTo (coord1, 0.0, LineSeg1d::s_epsilon);
    SnapTo (coord1, 1.0, LineSeg1d::s_epsilon);

    LineSeg1d i (coord0, coord1);

    i.ClampTo (LineSeg1d (0.0, 1.0));
    return i;
}


// ---


BspTree2d::BspTree2d ()
    : m_leftSubtree (0)
    , m_rightSubtree (0)
{
}

BspTree2d::BspTree2d (const LineSegVec& linesegs)
    : m_leftSubtree (0)
    , m_rightSubtree (0)
{
    assert (!linesegs.empty ());

    m_kCoincident.push_back (linesegs[0]);

    LineSegVec right, left;
    int iMax = (int )linesegs.size();
    for (int i = 1; i < iMax; i++)
    {
        MutualPosition mutualPos (linesegs[0], linesegs[i]);

        switch (mutualPos ())
        {
        case MutualPosition::LEFT_TO_RIGHT:
            right.push_back (LineSeg (mutualPos.Intersection(), linesegs[i].End()));
            left.push_back (LineSeg (linesegs[i].Start(), mutualPos.Intersection()));
            break;
        case MutualPosition::RIGHT_TO_LEFT:
            right.push_back (LineSeg (linesegs[i].Start(), mutualPos.Intersection()));
            left.push_back (LineSeg (mutualPos.Intersection(), linesegs[i].End()));
            break;
        case MutualPosition::RIGHT:
            right.push_back (linesegs[i]);
            break;
        case MutualPosition::LEFT:
            left.push_back (linesegs[i]);
            break;
        default:
            m_kCoincident.push_back (linesegs[i]);
        }
    }

    if (right.size() > 0)
    {
        m_rightSubtree = new BspTree2d(right);
    }

    if (left.size() > 0)
    {
        m_leftSubtree = new BspTree2d(left);
    }
}

BspTree2d::~BspTree2d ()
{
    delete m_rightSubtree;
    delete m_leftSubtree;
}

BspTree2d* BspTree2d::Clone () const
{
    BspTree2d* clone = new BspTree2d;

    clone->m_rightSubtree = m_rightSubtree ? m_rightSubtree->Clone () : 0;
    clone->m_leftSubtree  = m_leftSubtree  ? m_leftSubtree->Clone ()  : 0;
    clone->m_kCoincident = m_kCoincident;
    return clone;
}

void BspTree2d::Invert ()
{
    std::swap (m_leftSubtree, m_rightSubtree);

    if (m_leftSubtree)
    {
        m_leftSubtree->Invert();
    }

    if (m_rightSubtree)
    {
        m_rightSubtree->Invert();
    }

    LineSegVec::iterator it = m_kCoincident.begin ();
    LineSegVec::iterator end = m_kCoincident.end ();
    for (; it != end; ++it)
    {
        (*it).Invert ();
    }
}

const LineSegVec& BspTree2d::GetPartitionSegments () const
{
    assert (!m_kCoincident.empty ());
    return m_kCoincident;
}


// ---


BspLineSegSplitter::BspLineSegSplitter (const BspTree2d* bsp)
    : m_bsp (bsp)
{
}

void BspLineSegSplitter::SplitByLeftSubtree (const LineSeg& seg, const BspTree2d* bsp)
{
    if (bsp->LeftSubtree ())
    {
        Split (seg, bsp->LeftSubtree ());
    }
    else
    {
        m_leftSegs.push_back (seg);
    }
}

void BspLineSegSplitter::SplitByRightSubtree (const LineSeg& seg, const BspTree2d* bsp)
{
    if (bsp->RightSubtree ())
    {
        Split (seg, bsp->RightSubtree ());
    }
    else
    {
        m_rightSegs.push_back (seg);
    }
}

/**
 * The 'seg' param is supposed to be coincident with the root node of the
 * 'bsp' param.  Any overlap of 'seg' with a bsp node line segment is
 * classified as left/inside if directions of both line segments agree,
 * right/outside otherwise.  Parts of 'seg' (if any) that don't overlap
 * with any of the bsp node line segments are passed to both BSP subtrees for
 * further evaluation.
 */
void BspLineSegSplitter::SplitByThisNode (const LineSeg& seg, const BspTree2d* bsp)
{
    LineSegVec segsToGo;
    segsToGo.push_back (seg);

    while (!segsToGo.empty ())
    {
        LineSeg currSeg = segsToGo.front ();

        LineSegVec::const_iterator co_it = bsp->GetPartitionSegments ().begin ();
        LineSegVec::const_iterator co_end = bsp->GetPartitionSegments ().end ();
        for (; co_it != co_end; ++co_it)
        {
            LineSeg1d o = OverlapCoincident (currSeg, *co_it);

            if (!o.IsNonTrivial ())
            {
                continue;
            }

            if (o.IsDirNegative ())
            {
                m_rightSegs.push_back (LineSeg (currSeg.Pt (o.End()), currSeg.Pt (o.Start())));
            }
            else
            {
                m_leftSegs.push_back (LineSeg (currSeg.Pt (o.Start()), currSeg.Pt (o.End())));
            }

            if (abs(o.Min()) > LineSeg1d::s_epsilon)
            {
                segsToGo.push_back (LineSeg (currSeg.Pt(0.0), currSeg.Pt(o.Min())));
            }
            if (abs(1.0 - o.Max()) > LineSeg1d::s_epsilon)
            {
                segsToGo.push_back (LineSeg (currSeg.Pt(o.Max()), currSeg.Pt(1.0)));
            }

            break;
        }

        if (co_it == co_end)
        {
            SplitByLeftSubtree (currSeg, bsp);
            SplitByRightSubtree (currSeg, bsp);
        }

        segsToGo.erase (segsToGo.begin ());
    }
}

void BspLineSegSplitter::Split (const LineSeg& seg, const BspTree2d* bsp)
{
    MutualPosition mutualPos (bsp->GetPartitionSegments ().front (), seg);

    switch (mutualPos ())
    {
    case MutualPosition::LEFT_TO_RIGHT:
        SplitByRightSubtree (LineSeg (mutualPos.Intersection (), seg.End()), bsp);
        SplitByLeftSubtree  (LineSeg (seg.Start(), mutualPos.Intersection ()), bsp);
        break;
    case MutualPosition::RIGHT_TO_LEFT:
        SplitByRightSubtree (LineSeg(seg.Start(), mutualPos.Intersection ()), bsp);
        SplitByLeftSubtree  (LineSeg(mutualPos.Intersection (), seg.End()), bsp);
        break;
    case MutualPosition::RIGHT:
        SplitByRightSubtree (seg, bsp);
        break;
    case MutualPosition::LEFT:
        SplitByLeftSubtree (seg, bsp);
        break;
    default:
        SplitByThisNode (seg, bsp);
        break;
    }
}

void BspLineSegSplitter::Split (const LineSeg& seg)
{
    Split (seg, m_bsp);
}
