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

#ifdef INCLUDE_SOUNDVIEW

// include the required headers
#include "SoundView.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/DiskFile.h>

#define WAVE_FORMAT_PCM 0x0001


namespace EMStudio
{
    // the constructor
    SoundView::SoundView(QWidget* parent)
        : QWidget(parent)
    {
        Init();
    }

    // default constructor
    SoundView::SoundView()
        : QWidget(nullptr)
    {
        Init();
    }


    // destructor
    SoundView::~SoundView()
    {
        if (mWavBuffer && mCopySoundData)
        {
            delete[] mWavBuffer;
        }
        mWavBuffer = nullptr;
    }


    // init the sound view widget
    void SoundView::Init()
    {
        setMinimumHeight(30);
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        mWavBuffer = nullptr;
        mWavLength = 0;
        mProcessingPercentage = 0;
    }


    // overload the paintevent
    void SoundView::paintEvent(QPaintEvent* event)
    {
        // init the painter
        QPainter painter(this);
        QColor fontColor = QColor(190, 190, 190);
        QPen pen(fontColor);
        painter.setPen(pen);

        // draw progress bar
        if (mProcessingPercentage > 0 && mProcessingPercentage != 100)
        {
            QLinearGradient gradient(0, 0, 0, height());
            gradient.setColorAt(0.0, QColor(94, 102, 110));
            gradient.setColorAt(0.5, QColor(56, 65, 72));
            gradient.setColorAt(1.0, QColor(94, 102, 110));

            painter.setPen(QColor(94, 102, 110));
            painter.setBrush(QBrush(gradient));
            painter.drawRoundedRect(3, 3, (width() - 6) * mProcessingPercentage * 0.01f, height() - 6, 10.0, 10.0);
        }
        else
        {
            QLinearGradient gradient(0, 0, 0, height());
            gradient.setColorAt(0.0, QColor(66, 66, 66));
            gradient.setColorAt(0.5, QColor(66, 150, 66));
            gradient.setColorAt(1.0, QColor(66, 66, 66));

            painter.setPen(QColor(66, 100, 66));
            painter.setBrush(QBrush(gradient));
            if (mProcessingPercentage > 0)
            {
                painter.drawRoundedRect(3, 3, (width() - 6) * mProcessingPercentage * 0.01f, height() - 6, 10.0, 10.0);
            }
        }

        // draw bounding rect
        painter.setPen(QColor(111, 111, 111));
        painter.setBrush(QBrush());
        painter.drawRoundedRect(3, 3, width() - 6, height() - 6, 10.0, 10.0);

        // display error text or add line within the center
        if (mProcessingPercentage == 0)
        {
            painter.drawText((width() / 2) - 60, (height() / 2) + 3, "No sound file analyzed yet.");
        }

        // check if wav buffer is filled with data
        if (mWavBuffer == nullptr)
        {
            return;
        }

        // draw center line
        painter.drawLine(3, ((height()) * 0.5f), width() - 3, ((height()) * 0.5f));

        // change color for the wave view
        pen.setColor(fontColor);
        painter.setPen(pen);

        // calculate some border variables for limiting the wave into the box
        float width1 = ((float)(width() - 4) / (float)mWavLength);
        int step = MCore::Max<int>((mWavLength / width()), 1);
        float heightScale = (256 / (height() - 20));
        int halfHeight = height() * 0.5;

        // loop trough the wave data and display it
        for (uint32 i = 1; i < mWavLength - (6 * step); i += step)
        {
            QPoint point1 = QPoint(5 + (width1 * i), (mWavBuffer[i] / heightScale) + halfHeight);
            QPoint point2 = QPoint(5 + (width1 * (i + 1)), (mWavBuffer[i + 1] / heightScale) + halfHeight);
            painter.drawLine(point1, point2);
        }
    }


    // sets the data from a CMp3LipsyncAudioSource
    void SoundView::SetDataFromMP3Source(CMp3LipsyncAudioSource* source)
    {
        if (mWavBuffer && mCopySoundData)
        {
            delete[] mWavBuffer;
        }
        mWavBuffer = nullptr;
        mCopySoundData = true;

        // set the length
        mWavLength = source->GetNumBytes();

        mWavBuffer = new char[mWavLength];
        source->ReadBytes(mWavBuffer, mWavLength);

        update();
    }


    // loads data from a wav file
    bool SoundView::LoadWavData(const MCore::String& waveFile)
    {
        // delete previous wav data
        if (mWavBuffer && mCopySoundData)
        {
            delete[] mWavBuffer;
        }
        mWavBuffer = nullptr;
        mCopySoundData = true;

        // open the wav file
        MCore::DiskFile f;
        if (f.Open(waveFile.AsChar(), MCore::DiskFile::READ) == false)
        {
            return false;
        }

        // struct for the riffheader
        struct RiffHeader
        {
            char riff[4];
            long size;
            char wave[4];
        };

        // read the riffheader
        RiffHeader riffHeader;
        f.Read(&riffHeader, sizeof(RiffHeader));

        // check for right wav format
        if ((riffHeader.riff[0] != 'R' || riffHeader.riff[1] != 'I' || riffHeader.riff[2] != 'F' || riffHeader.riff[3] != 'F') ||
            (riffHeader.wave[0] != 'W' || riffHeader.wave[1] != 'A' || riffHeader.wave[2] != 'V' || riffHeader.wave[3] != 'E'))
        {
            f.Close();
            MCore::LogError("File '%s' is an invalid WAV file!", waveFile.AsChar());
            return false;
        }

        // struct for one chunk
        struct  FormatChunk
        {
            short          wFormatTag;
            unsigned short wChannels;
            unsigned long  dwSamplesPerSec;
            unsigned long  dwAvgBytesPerSec;
            unsigned short wBlockAlign;
            unsigned short wBitsPerSample;
        };

        // struct for a wav chunk
        struct WavChunk
        {
            char            chunkID[4];
            unsigned int    chunkSize;
        };

        // loop trough the wav data
        do
        {
            WavChunk chunk;
            if (f.Read(&chunk, sizeof(WavChunk)) == 0)
            {
                break;
            }

            if (chunk.chunkID[0] == 'f' && chunk.chunkID[1] == 'm' && chunk.chunkID[2] == 't' && chunk.chunkID[3] == ' ')
            {
                FormatChunk fmt;
                f.Read(&fmt, sizeof(FormatChunk));

                if (fmt.wFormatTag != WAVE_FORMAT_PCM)
                {
                    f.Close();
                    MCore::LogError("Not an uncompressed PCM wave!");
                    return false;
                }

                MCore::LogDebug("WAVE INFO for '%s'", waveFile.AsChar());
                MCore::LogDebug("channels          = %d", fmt.wChannels);
                MCore::LogDebug("samples per sec   = %d", fmt.dwSamplesPerSec);
                MCore::LogDebug("avg bytes per sec = %d", fmt.dwAvgBytesPerSec);
                MCore::LogDebug("block align       = %d", fmt.wBlockAlign);
                MCore::LogDebug("bits per sample   = %d", fmt.wBitsPerSample);

                mChannels = fmt.wChannels;
                if (fmt.wBitsPerSample != 16)
                {
                    f.Close();
                    MCore::LogError("WAVE file '%s' must be 16 bits!", waveFile.AsChar());
                    return false;
                }

                mSamplesPerSec = fmt.dwSamplesPerSec;
            }
            else
            if (chunk.chunkID[0] == 'd' && chunk.chunkID[1] == 'a' && chunk.chunkID[2] == 't' && chunk.chunkID[3] == 'a')
            {
                MCore::LogDebug("WAV data size = %d", chunk.chunkSize);
                delete[] mWavBuffer;
                mWavBuffer = new char[chunk.chunkSize];
                mWavLength = chunk.chunkSize;
                f.Read(mWavBuffer, chunk.chunkSize);
            }
            else    // skip the chunk
            {
                f.Forward(chunk.chunkSize);
            }
        } while (f.GetIsEOF() == false);

        f.Close();
        return true;
    }


    // sets the sound view loading progress
    void SoundView::SetProcessingPercentage(const uint32 percentage)
    {
        mProcessingPercentage = percentage;
        repaint();
    }


    // set the sound data from existing data
    void SoundView::SetSoundData(char* wavData, uint32 wavLength, bool copySoundData)
    {
        // delete previous wav data
        if (mWavBuffer)
        {
            delete[] mWavBuffer;
            mWavBuffer = nullptr;
        }

        // set the new wav data
        mCopySoundData = copySoundData;
        mWavLength = wavLength;

        // copy data if necessary
        if (mCopySoundData)
        {
            mWavBuffer = new char[mWavLength];
            memcpy(mWavBuffer, wavData, sizeof(char) * mWavLength);
        }
        else
        {
            mWavBuffer = wavData;
        }
    }
} // namespace EMStudio

#endif

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/SoundView.moc>