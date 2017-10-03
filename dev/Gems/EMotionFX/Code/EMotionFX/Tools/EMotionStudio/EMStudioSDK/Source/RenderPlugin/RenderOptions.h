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

#ifndef __EMSTUDIO_RENDEROPTIONS_H
#define __EMSTUDIO_RENDEROPTIONS_H

//
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/Color.h>
#include "../EMStudioConfig.h"
#include <QSettings>
#include <QColor>


namespace EMStudio
{
    struct EMSTUDIO_API RenderOptions
    {
        MCORE_MEMORYOBJECTCATEGORY(RenderOptions, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE)

        MCore::RGBAColor        mBackgroundColor;
        MCore::RGBAColor        mWireframeColor;
        MCore::RGBAColor        mVertexNormalsColor;
        MCore::RGBAColor        mFaceNormalsColor;
        MCore::RGBAColor        mTangentsColor;
        MCore::RGBAColor        mMirroredBinormalsColor;
        MCore::RGBAColor        mBinormalsColor;
        MCore::RGBAColor        mNodeAABBColor;
        MCore::RGBAColor        mStaticAABBColor;
        MCore::RGBAColor        mMeshAABBColor;
        MCore::RGBAColor        mCollisionMeshAABBColor;
        MCore::RGBAColor        mCollisionMeshColor;
        MCore::RGBAColor        mOBBsColor;
        MCore::RGBAColor        mLineSkeletonColor;
        MCore::RGBAColor        mSkeletonColor;
        MCore::RGBAColor        mSelectionColor;
        MCore::RGBAColor        mSelectedObjectColor;

        MCore::RGBAColor        mGridColor;
        MCore::RGBAColor        mMainAxisColor;
        MCore::RGBAColor        mSubStepColor;

        MCore::RGBAColor        mLightSkyColor;
        MCore::RGBAColor        mLightGroundColor;
        MCore::RGBAColor        mNodeNameColor;

        float                   mGridUnitSize;
        float                   mFaceNormalsScale;
        float                   mVertexNormalsScale;
        float                   mTangentsScale;
        float                   mNearClipPlaneDistance;
        float                   mFarClipPlaneDistance;
        float                   mFOV;
        float                   mMainLightAngleA;
        float                   mMainLightAngleB;
        float                   mMainLightIntensity;
        float                   mSpecularIntensity;

        float                   mNodeOrientationScale;
        bool                    mScaleBonesOnLength;
        bool                    mRenderBonesOnly;

        bool                    mSkipLoadingTextures;
        int32                   mAspiredRenderFPS;

        // advanced render options
        bool                    mEnableAdvancedRendering;
        bool                    mBloomEnabled;
        float                   mBloomThreshold;
        float                   mBloomIntensity;
        float                   mBloomRadius;
        bool                    mDOFEnabled;
        float                   mDOFFocalPoint;
        float                   mDOFNear;
        float                   mDOFFar;
        float                   mDOFBlurRadius;
        float                   mRimAngle;
        float                   mRimWidth;
        float                   mRimIntensity;
        MCore::RGBAColor        mRimColor;
        bool                    mShowFPS;

        MCore::String           mTexturePath;
        bool                    mCreateMipMaps;
        MCore::RGBAColor        mGradientSourceColor;
        MCore::RGBAColor        mGradientTargetColor;
        MCore::RGBAColor        mTrajectoryArrowBorderColor;
        MCore::RGBAColor        mTrajectoryArrowInnerColor;
        MCore::String           mLastUsedLayout;
        bool                    mRenderSelectionBox;

        RenderOptions();

        void Save(QSettings* settings);
        void Load(QSettings* settings);

        static MCore::RGBAColor StringToColor(QString text)             { QColor color = QColor(text); return MCore::RGBAColor(color.redF(), color.greenF(), color.blueF(), color.alphaF()); }
        static QString ColorToString(const MCore::RGBAColor& color)     { QColor qColor; qColor.setRedF(color.r); qColor.setGreenF(color.g); qColor.setBlueF(color.b); qColor.setAlphaF(color.a); return qColor.name(); }
    };
} // namespace EMStudio


#endif
