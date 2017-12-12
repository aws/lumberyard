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
#pragma once

#include <SerializeFwd.h>
#include <AzCore/Math/Vector2.h>

// forward declarations
class ITexture;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! A sprite is a texture with extra information about how it behaves for 2D drawing
//! Currently a sprite exists on disk as a side car file next to the texture file.
class ISprite
    : public _i_reference_target<int>
{
public: // types

    // The borders define the areas of the sprite that stretch
    // Border's members are always in range 0-1, they are normalized positions within the texture bounds
    struct Borders
    {
        Borders()
            : m_left(0.0f)
            , m_right(1.0f)
            , m_top(0.0f)
            , m_bottom(1.0f) {}

        Borders(float left, float right, float top, float bottom)
            : m_left(left)
            , m_right(right)
            , m_top(top)
            , m_bottom(bottom) {}

        float   m_left;
        float   m_right;
        float   m_top;
        float   m_bottom;
    };

public: // member functions

    //! Deleting a sprite will release its texture
    virtual ~ISprite() {}

    //! Get the pathname of this sprite
    virtual const string& GetPathname() const = 0;

    //! Get the pathname of the texture of this sprite
    virtual const string& GetTexturePathname() const = 0;

    //! Get the borders of this sprite
    virtual Borders GetBorders() const = 0;

    //! Set the borders of this sprite
    virtual void SetBorders(Borders borders) = 0;

    //! Get the texture for this sprite
    virtual ITexture* GetTexture() const = 0;

    //! Serialize this object for save/load
    virtual void Serialize(TSerialize ser) = 0;

    //! Save this sprite data to disk
    virtual bool SaveToXml(const string& pathname) = 0;

    //! Test if this sprite has any borders
    virtual bool AreBordersZeroWidth() const = 0;

    //! Get the dimensions of the sprite
    virtual AZ::Vector2 GetSize() const = 0;
};

