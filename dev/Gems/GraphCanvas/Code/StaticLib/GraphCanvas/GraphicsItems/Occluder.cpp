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
#include <QPainter>

#include <GraphCanvas/GraphicsItems/Occluder.h>

namespace GraphCanvas
{
    /////////////
    // Occluder
    /////////////
    
    Occluder::Occluder(const OccluderConfiguration& occluderConfiguration)
        : m_renderColor(occluderConfiguration.m_renderColor)
    {
        m_renderColor.setAlpha(255);
        
        setPos(occluderConfiguration.m_bounds.topLeft());
        setPreferredSize(occluderConfiguration.m_bounds.size());
        
        setOpacity(occluderConfiguration.m_opacity);

        setZValue(occluderConfiguration.m_zValue);
    }
    
    void Occluder::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
        painter->fillRect(boundingRect(), m_renderColor);
    }
}