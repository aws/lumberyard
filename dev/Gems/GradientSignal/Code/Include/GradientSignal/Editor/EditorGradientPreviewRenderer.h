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

// AZ
#include <AzCore/Math/MathUtils.h>

// Qt
#include <QImage>
#include <QObject>
#include <QSize>
#include <QTimer>

// GradientSignal
#include <GradientSignal/Ebuses/GradientPreviewContextRequestBus.h>
#include <GradientSignal/GradientSampler.h>

namespace GradientSignal
{
    class EditorGradientPreviewRenderer
        : public QObject
    {
    public:
        using SampleFilterFunc = AZStd::function<float(float)>;

        EditorGradientPreviewRenderer()
            : QObject()
            , m_updateTimer(new QTimer(this))
        {
            // Defer updates by one tick to avoid sampling gradients before they're ready
            m_updateTimer->setSingleShot(true);
            m_updateTimer->setInterval(0);
            QObject::connect(m_updateTimer, &QTimer::timeout, this, [this]() { OnUpdate(); });
        }

        void SetGradientSampler(const GradientSampler& sampler)
        {
            m_sampler = sampler;
            QueueUpdate();
        }

        void SetGradientSampleFilter(SampleFilterFunc filterFunc)
        {
            m_filterFunc = filterFunc;
            QueueUpdate();
        }

        void QueueUpdate()
        {
            m_dirty = true;
            m_updateTimer->start();
        }

    protected:
        /**
         * Since this base class is shared between a QWidget and a QGraphicsItem, we need to abstract
         * the actual update() call so that they can invoke the proper one
         */
        virtual void OnUpdate() = 0;

        void Render(const QSize& imageResolution)
        {
            if (m_previewImage.size() != imageResolution)
            {
                m_previewImage = QImage(imageResolution, QImage::Format_Grayscale8);
            }

            m_previewImage.fill(QColor(0, 0, 0));
            if (!m_sampler.m_gradientId.IsValid())
            {
                return;
            }

            bool constrainToShape = false;
            GradientPreviewContextRequestBus::EventResult(constrainToShape, m_sampler.m_ownerEntityId, &GradientPreviewContextRequestBus::Events::GetConstrainToShape);

            AZ::Aabb previewBounds = AZ::Aabb::CreateNull();
            GradientPreviewContextRequestBus::EventResult(previewBounds, m_sampler.m_ownerEntityId, &GradientPreviewContextRequestBus::Events::GetPreviewBounds);

            AZ::EntityId previewEntityId;
            GradientPreviewContextRequestBus::EventResult(previewEntityId, m_sampler.m_ownerEntityId, &GradientPreviewContextRequestBus::Events::GetPreviewEntity);

            constrainToShape = constrainToShape && previewEntityId.IsValid();

            if (!previewBounds.IsValid())
            {
                return;
            }

            const AZ::Vector3 previewBoundsCenter = previewBounds.GetCenter();
            const AZ::Vector3 previewBoundsExtentsOld = previewBounds.GetExtents();
            previewBounds = AZ::Aabb::CreateCenterRadius(previewBoundsCenter, AZ::GetMax(previewBoundsExtentsOld.GetX(), previewBoundsExtentsOld.GetY()) / 2.0f);

            const AZ::Vector3 previewBoundsStart = AZ::Vector3(previewBounds.GetMin().GetX(), previewBounds.GetMin().GetY(), previewBoundsCenter.GetZ());
            const AZ::Vector3 previewBoundsExtents = previewBounds.GetExtents();
            const float previewBoundsExtentsX = previewBoundsExtents.GetX();
            const float previewBoundsExtentsY = previewBoundsExtents.GetY();

            // Get the actual resolution of our preview image.  Note that this might be non-square, depending on how the window is sized.
            const int imageResolutionX = imageResolution.width();
            const int imageResolutionY = imageResolution.height();

            // Get the largest square size that fits into our window bounds.
            const int imageBoundsX = AZStd::min(imageResolutionX, imageResolutionY);
            const int imageBoundsY = AZStd::min(imageResolutionX, imageResolutionY);

            // Get how many pixels we need to offset in x and y to center our square in the window.  Because we've made our square
            // as large as possible, one of these two values should always be 0.  
            // i.e. we'll end up with black bars on the sides or on top, but it should never be both.
            const int centeringOffsetX = (imageResolutionX - imageBoundsX) / 2;
            const int centeringOffsetY = (imageResolutionY - imageBoundsY) / 2;

            AZ::u8* buffer = static_cast<AZ::u8*>(m_previewImage.bits());

            // This is the "striding value".  When walking directly through our preview image bits() buffer, there might be
            // extra pad bytes for each line due to alignment.  We use this to make sure we start writing each line at the right byte offset.
            const int imageBytesPerLine = m_previewImage.bytesPerLine();

            // When sampling the gradient, we can choose to either do it at the corners of each texel area we're sampling, or at the center.
            // They're both correct choices in different ways.  We're currently choosing to do the corners, which makes scaledTexelOffset = 0,
            // but the math is here to make it easy to change later if we ever decide sampling from the center provides a more intuitive preview.
            constexpr float texelOffset = 0.0f;  // Use 0.5f to sample from the center of the texel.
            const AZ::Vector3 scaledTexelOffset(texelOffset * previewBoundsExtentsX / static_cast<float>(imageBoundsX),
                                                texelOffset * previewBoundsExtentsY / static_cast<float>(imageBoundsY), 0.0f);

            // Scale from our preview image size space (ex: 256 pixels) to our preview bounds space (ex: 16 meters)
            const AZ::Vector3 pixelToBoundsScale(previewBoundsExtentsX / static_cast<float>(imageBoundsX), 
                                                 previewBoundsExtentsY / static_cast<float>(imageBoundsY), 0.0f);

            for (int y = 0; y < imageBoundsY; ++y)
            {
                for (int x = 0; x < imageBoundsX; ++x)
                {
                    // Invert world y to match axis.  (We use "imageBoundsY- 1" to invert because our loop doesn't go all the way to imageBoundsY)
                    AZ::Vector3 uvw(static_cast<float>(x), static_cast<float>((imageBoundsY - 1) - y), 0.0f);

                    GradientSampleParams sampleParams;
                    sampleParams.m_position = previewBoundsStart + (uvw * pixelToBoundsScale) + scaledTexelOffset;

                    bool inBounds = true;
                    if (constrainToShape)
                    {
                        LmbrCentral::ShapeComponentRequestsBus::EventResult(inBounds, previewEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, sampleParams.m_position);
                    }

                    float sample = inBounds ? m_sampler.GetValue(sampleParams) : 0.0f;

                    if (m_filterFunc)
                    {
                        sample = m_filterFunc(sample);
                    }

                    buffer[((centeringOffsetY + y) * imageBytesPerLine) + (centeringOffsetX + x)] = static_cast<AZ::u8>(sample * 255);
                }
            }
        }

        GradientSampler m_sampler;
        SampleFilterFunc m_filterFunc;
        QImage m_previewImage;
        QTimer* m_updateTimer;
        bool m_dirty = true;
    };
} //namespace GradientSignal
