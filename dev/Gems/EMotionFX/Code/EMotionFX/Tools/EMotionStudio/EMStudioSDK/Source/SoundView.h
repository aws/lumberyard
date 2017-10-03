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

#ifndef __EMSTUDIO_SOUNDVIEW_H
#define __EMSTUDIO_SOUNDVIEW_H

#ifdef INCLUDE_SOUNDVIEW

//
#include <MCore/Source/StandardHeaders.h>
#include "EMStudioConfig.h"
#include "../../Plugins/StandardPlugins/Source/Lipsync/mp3_audio_source.h"
#include <QWidget>
#include <QPainter>
#include <QPixmap>


namespace EMStudio
{
    class EMSTUDIO_API SoundView
        : public QWidget
    {
        Q_OBJECT
                       MCORE_MEMORYOBJECTCATEGORY(SoundView, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_CUSTOMWIDGETS)

    public:
        SoundView(QWidget* parent);
        SoundView();
        ~SoundView();

        void Init();
        bool LoadWavData(const MCore::String& waveFile);
        void SetSoundData(char* wavData, uint32 wavLength, bool copySoundData = false);
        void SetDataFromMP3Source(CMp3LipsyncAudioSource* source);
        void SetProcessingPercentage(const uint32 percentage);

        void paintEvent(QPaintEvent* event);
    protected:
        QPixmap*    mPixmap;
        char*       mWavBuffer;
        uint32      mProcessingPercentage;
        bool        mCopySoundData;
        uint32      mWavLength;
        uint32      mChannels;
        uint32      mSamplesPerSec;
    };
} // namespace EMStudio

#endif

#endif
