/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2015 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
** Contact: http://www.qt.io/licensing/
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

/*!
    \qmltype MaskedBlur
    \inqmlmodule QtGraphicalEffects
    \since QtGraphicalEffects 1.0
    \inherits QtQuick2::Item
    \ingroup qtgraphicaleffects-blur
    \brief Applies a blur effect with a varying intesity.

    MaskedBlur effect softens the image by blurring it. The intensity of the
    blur can be controlled for each pixel using maskSource so that some parts of
    the source are blurred more than others.

    Performing blur live is a costly operation. Fullscreen gaussian blur
    with even a moderate number of samples will only run at 60 fps on highend
    graphics hardware.

    \table
    \header
        \li Source
        \li MaskSource
        \li Effect applied
    \row
        \li \image Original_bug.png
        \li \image MaskedBlur_mask.png
        \li \image MaskedBlur_bug.png
    \endtable

    \section1 Example

    The following example shows how to apply the effect.
    \snippet MaskedBlur-example.qml example

*/
Item {
    id: root

    /*!
        This property defines the source item that is going to be blurred.

        \note It is not supported to let the effect include itself, for
        instance by setting source to the effect's parent.
    */
    property alias source: blur.source

    /*!
        This property defines the item that is controlling the final intensity
        of the blur. The pixel alpha channel value from maskSource defines the
        actual blur radius that is going to be used for blurring the
        corresponding source pixel.

        Opaque maskSource pixels produce blur with specified
        \l{MaskedBlur::radius}{radius}, while transparent pixels suppress the
        blur completely. Semitransparent maskSource pixels produce blur with a
        radius that is interpolated according to the pixel transparency level.
    */
    property alias maskSource: maskProxy.input

    /*!
        This property defines the distance of the neighboring pixels which
        affect the blurring of an individual pixel. A larger radius increases
        the blur effect.

        Depending on the radius value, value of the
        \l{MaskedBlur::samples}{samples} should be set to sufficiently large to
        ensure the visual quality.

        The value ranges from 0.0 (no blur) to inf. By default, the property is
        set to \c 0.0 (no blur).

        \table
        \header
        \li Output examples with different radius values
        \li
        \li
        \row
            \li \image MaskedBlur_radius1.png
            \li \image MaskedBlur_radius2.png
            \li \image MaskedBlur_radius3.png
        \row
            \li \b { radius: 0 }
            \li \b { radius: 8 }
            \li \b { radius: 16 }
        \row
            \li \l samples: 25
            \li \l samples: 25
            \li \l samples: 25
        \endtable

    */
    property alias radius: blur.radius

    /*!
        This property defines how many samples are taken per pixel when blur
        calculation is done. Larger value produces better quality, but is slower
        to render.

        Ideally, this value should be twice as large as the highest required
        radius value plus 1, for example, if the radius is animated between 0.0
        and 4.0, samples should be set to 9.

        By default, the property is set to \c 9.

        This property is not intended to be animated. Changing this property may
        cause the underlying OpenGL shaders to be recompiled.
    */
    property alias samples: blur.samples

    /*!
        This property allows the effect output pixels to be cached in order to
        improve the rendering performance. Every time the source or effect
        properties are changed, the pixels in the cache must be updated. Memory
        consumption is increased, because an extra buffer of memory is required
        for storing the effect output.

        It is recommended to disable the cache when the source or the effect
        properties are animated.

        By default, the property is set to \c false.

    */
    property alias cached: cacheItem.visible

    /*!
        \internal

        Kept for source compatibility only. Removed in Qt 5.6
        ### Qt6: remove
    */
    property bool fast: false

    /*!
        \internal

        Kept for source compatibility only. Removed in Qt 5.6

        Doing transparent border on a masked source doesn't make any sense
        as the padded exterior will have a mask alpha value of 0 which means
        no blurring and as the padded exterior of the source is a transparent
        pixel, the result is no pixels at all.

        In Qt 5.6 and before, this worked based on that the mask source
        was scaled up to fit the padded blur target rect, which would lead
        to inconsistent and buggy results.

        ### Qt6: remove
    */
    property bool transparentBorder;

    GaussianBlur {
        id: blur

        source: root.source;
        anchors.fill: parent
        _maskSource: maskProxy.output;

        SourceProxy {
            id: maskProxy
        }
    }

    ShaderEffectSource {
        id: cacheItem
        x: -blur._kernelRadius
        y: -blur._kernelRadius
        width: blur.width + 2 * blur._kernelRadius
        height: blur.height + 2 * blur._kernelRadius
        visible: false
        smooth: true
        sourceRect: Qt.rect(-blur._kernelRadius, -blur._kernelRadius, width, height);
        sourceItem: blur
        hideSource: visible
    }
}
