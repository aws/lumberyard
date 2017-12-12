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

#ifndef CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_POLYGONSETOPS_LINESEG_H
#define CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_POLYGONSETOPS_LINESEG_H
#pragma once


#include "Utils.h"

#include <vector>

/**
* @brief A simple oriented 2D line segment implementation.
*/
class LineSeg
{
    Vector2d m_start;
    Vector2d m_end;
public:
    LineSeg (const Vector2d& start, const Vector2d& end);
    const Vector2d& Start () const;
    const Vector2d& End () const;
    /// Inverts this line segment's direction.
    void Invert ();
    Vector2d Dir () const;
    /// Takes parameter and returns point corresponding to that parameter.
    Vector2d Pt (float t) const;
    /// Returns (unnormalized) normal vector to this lineseg.
    Vector2d Normal () const;
};

inline LineSeg::LineSeg (const Vector2d& start, const Vector2d& end)
    : m_start (start)
    , m_end (end)
{
}

inline const Vector2d& LineSeg::Start () const
{
    return m_start;
}

inline const Vector2d& LineSeg::End () const
{
    return m_end;
}

inline void LineSeg::Invert ()
{
    std::swap (m_start, m_end);
}

inline Vector2d LineSeg::Dir () const
{
    return m_end - m_start;
}

inline Vector2d LineSeg::Pt (float t) const
{
    return m_start + Dir() * t;
}

inline Vector2d LineSeg::Normal () const
{
    return Dir ().Perp ();
}

typedef std::vector <LineSeg> LineSegVec;

#endif // CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_POLYGONSETOPS_LINESEG_H
