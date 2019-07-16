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

#include "stdafx.h"

#include "Util.h"                                       // Util
#include "TGA.h"

namespace ImageTGA
{
    ///////////////////////////////////////////////////////////////////////////////////

    struct TgaFileHeader
    {
        uint8  idLength;
        uint8  colorMapType;
        uint8  imageType;
        uint8  colorMapInfo[5];
        uint16 imageOriginX;
        uint16 imageOriginY;
        uint16 imageWidth;
        uint16 imageHeight;
        uint8  pixelBitCount;
        struct
        {
            // FIXME: will these bitfields work with *both* little-endian and big-endian?
            unsigned char alphaBitCount        : 4;
            unsigned char pixelsAreRightToLeft : 1;
            unsigned char pixelsAreTopToBottom : 1;
            unsigned char _padding_            : 2;
        } imageDescription;
        // ubyte id[idLength];
        // ubyte colorMap[someFunction(colorMapInfo)];

        TgaFileHeader()
        {
        }

        void setTrueColor24(int wd, int hg)
        {
            setTrueColor(wd, hg);
            imageDescription.alphaBitCount = 0;
            pixelBitCount = 24;
        }

        void setTrueColor32(int wd, int hg)
        {
            setTrueColor(wd, hg);
            imageDescription.alphaBitCount = 8;
            pixelBitCount = 32;
        }

        void setBw8(int wd, int hg)
        {
            idLength = 0;
            colorMapType = 0;
            imageType = 3; // uncompressed black&white
            colorMapInfo[0] = 0;
            colorMapInfo[1] = 0;
            colorMapInfo[2] = 0;
            colorMapInfo[3] = 0;
            colorMapInfo[4] = 0;
            imageOriginX = 0;
            imageOriginY = 0;
            imageWidth = wd;
            imageHeight = hg;
            pixelBitCount = 8;
            imageDescription.alphaBitCount = 0;
            imageDescription.pixelsAreRightToLeft = 0;
            imageDescription.pixelsAreTopToBottom = 1;
            imageDescription._padding_ = 0;
        }

        uint32 getFileOffsetOfPixels() const
        {
            return idLength + sizeof(*this); // + colorMap[xxx]
        }

        bool isSimple24() const
        {
            return
                (imageWidth > 0) &&
                (imageHeight > 0) &&
                (colorMapType == 0) &&
                (imageType == 2) &&
                (pixelBitCount == 24) &&
                (imageDescription.pixelsAreRightToLeft == 0);
        }

        bool isSimple32() const
        {
            return
                (imageWidth > 0) &&
                (imageHeight > 0) &&
                (colorMapType == 0) &&
                (imageType == 2) &&
                (pixelBitCount == 32) &&
                (imageDescription.pixelsAreRightToLeft == 0);
        }

    private:
        void setTrueColor(int wd, int hg)
        {
            idLength = 0;
            colorMapType = 0;
            imageType = 2; // uncompressed true-color
            colorMapInfo[0] = 0;
            colorMapInfo[1] = 0;
            colorMapInfo[2] = 0;
            colorMapInfo[3] = 0;
            colorMapInfo[4] = 0;
            imageOriginX = 0;
            imageOriginY = 0;
            imageWidth = wd;
            imageHeight = hg;
            pixelBitCount = 0;
            imageDescription.alphaBitCount = 0;
            imageDescription.pixelsAreRightToLeft = 0;
            imageDescription.pixelsAreTopToBottom = 1;
            imageDescription._padding_ = 0;
        }
    };


    void WriteTga(const char* fname, int width, int height, const float* p, int step)
    {
        FILE* f = nullptr; 
        azfopen(&f, fname, "wb");

        TgaFileHeader h;
        h.setBw8(width, height);

        fwrite(&h, sizeof(h), 1, f);

        const int pixelCount = width * height;


        float minV, maxV;
        minV = p[0];
        maxV = p[0];
        for (int i = 0; i < pixelCount; ++i)
        {
            if (p[i * step] < minV)
            {
                minV = p[i * step];
            }
            if (p[i * step] > maxV)
            {
                maxV = p[i * step];
            }
        }

        for (int i = 0; i < pixelCount; ++i)
        {
            const float v = p[i * step];

            uint b = 0;

            if (minV >= maxV)
            {
                assert(0);
            }
            else
            {
                const float norm = (v - minV) / (maxV - minV);
                b = Util::getClamped(int(norm * 255.0f), 0, 255);
            }

            fwrite(&b, 1, 1, f);
        }
        fclose(f);
    }
    ///////////////////////////////////////////////////////////////////////////////////

    Type ReadTgaType(const char* fname)
    {
        FILE* f = nullptr;
        azfopen(&f, fname, "rb");
        
        if (!f)
        {
            return Type::Unknown;
        }

        TgaFileHeader h;
        size_t countRead = fread(&h, sizeof(h), 1, f);
        if (countRead != 1)
        {
            return Type::Unknown;
        }

        fclose(f);
        f = nullptr;

        Type mapping[] = {
            Type::NoImageData,                  // 0
            Type::UncompressedPalettized,       // 1
            Type::UncompressedTrueColor,        // 2
            Type::UncompressedGrayscale,        // 3
            Type::Unknown,                      // 4
            Type::Unknown,                      // 5
            Type::Unknown,                      // 6
            Type::Unknown,                      // 7
            Type::Unknown,                      // 8
            Type::RunLengthEncodedPalettized,   // 9
            Type::RunLengthEncodedTrueColor,    // 10
            Type::RunLengthEncodedGrayscale     // 11
        };

        size_t typeCount = sizeof(mapping) / sizeof(mapping[0]);
        if (h.imageType < typeCount)
        {
            return mapping[h.imageType];
        }

        return Type::Unknown;
    }
};
