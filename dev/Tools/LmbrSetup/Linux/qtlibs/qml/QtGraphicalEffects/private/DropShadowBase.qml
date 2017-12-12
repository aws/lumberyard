/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Graphical Effects module.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtGraphicalEffects.private 1.0
import QtGraphicalEffects 1.0

Item {
    id: root

    property variant source
    property real radius: Math.floor(samples / 2)
    property int samples: 9
    property color color: "black"
    property real horizontalOffset: 0
    property real verticalOffset: 0
    property real spread: 0.0
    property bool cached: false
    property bool transparentBorder: true

    GaussianBlur {
        id: blur
        width: parent.width
        height: parent.height
        x: Math.round(horizontalOffset)
        y: Math.round(verticalOffset)
        source: root.source
        radius: root.radius
        samples: root.samples
        _thickness: root.spread
        transparentBorder: root.transparentBorder


        _color: root.color;
        _alphaOnly: true
        // ignoreDevicePixelRatio: root.ignoreDevicePixelRatio

        ShaderEffect {
            x: blur._outputRect.x - parent.x
            y: blur._outputRect.y - parent.y
            width: transparentBorder ? blur._outputRect.width : blur.width
            height: transparentBorder ? blur._outputRect.height : blur.height
            property variant source: blur._output;
        }

    }

    ShaderEffectSource {
        id: cacheItem
        x: -blur._kernelRadius + horizontalOffset
        y: -blur._kernelRadius + verticalOffset
        width: blur.width + 2 * blur._kernelRadius
        height: blur.height + 2 * blur._kernelRadius
        visible: root.cached
        smooth: true
        sourceRect: Qt.rect(-blur._kernelRadius, -blur._kernelRadius, width, height);
        sourceItem: blur
        hideSource: visible
    }


}
