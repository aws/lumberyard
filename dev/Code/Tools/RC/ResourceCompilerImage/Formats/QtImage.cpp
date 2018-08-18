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
#include "stdafx.h"

#include "../ImageObject.h"  // ImageToProcess

#include "IConfig.h"

#include <stdio.h>
#include <math.h>
#include <platform.h>
#include <string.h>
#include <ctype.h>

#include <QImage>
#include <QPainter>
#include <QFile>
#include <QTextStream>

#include "QtImage.h"
#include "ExportSettings.h"

///////////////////////////////////////////////////////////////////////////////////

namespace ImageQImage
{
    ImageObject* LoadByUsingQImageLoader(const char* filenameRead, const char* filenameWrite, CImageProperties* pProps, string& res_specialInstructions)
    {
        //try to open the image
        QImage qimage(filenameRead);
        if (qimage.isNull())
        {
            return NULL;
        }

        //make sure its in a valid format
        if (qimage.format() != QImage::Format_ARGB32)
        {
            qimage = qimage.convertToFormat(QImage::Format_ARGB32);
        }

        //create a new image object
        ImageObject* pImage(new ImageObject(qimage.width(), qimage.height(), 1, ePixelFormat_A8R8G8B8, ImageObject::eCubemap_UnknownYet));

        //get a pointer to the image objects pixel data
        char* pDst;
        uint32 dwPitch;
        pImage->GetImagePointer(0, pDst, dwPitch);

        //copy the qImage into the image object
        for (uint32 dwY = 0; dwY < qimage.height(); ++dwY)
        {
            char* dstLine = &pDst[dwPitch * dwY];
            uchar* srcLine = qimage.scanLine(dwY);
            memcpy(dstLine, srcLine, dwPitch);
        }

        // Load settings
        ImageExportSettings::LoadSettings(filenameWrite, res_specialInstructions);

        // Set props
        if (pProps != 0)
        {
            pProps->ClearInputImageFlags();

            if (qimage.hasAlphaChannel() /*has non-opaque alpha - might need attention*/)
            {
                pProps->m_bPreserveAlpha = true;
            }
        }

        return pImage;
    }

    bool IsExtensionSupported(const char* extension)
    {
        // This is the list of file extensions supported by this loader/saver
        return !azstricmp(extension, "bmp") ||
               !azstricmp(extension, "gif") ||
               !azstricmp(extension, "jpg") ||
               !azstricmp(extension, "jpeg") ||
               !azstricmp(extension, "jpe") ||
               !azstricmp(extension, "tga") ||
               !azstricmp(extension, "png");
    }

    bool UpdateAndSaveSettings(const char* settingsFilename, const string& settings)
    {
        return ImageExportSettings::SaveSettings(settingsFilename, settings);
    }

    namespace
    {
        class QImageConfigSink
            : public IConfigSink
        {
        public:
            QImageConfigSink(){}
            ~QImageConfigSink(){}

            // interface IConfigSink -------------------------------------

            // Copied from ImageTIF.cpp to match the format
            virtual void SetKeyValue(EConfigPriority ePri, const char* key, const char* value)
            {
                assert(ePri == eCP_PriorityFile);

                // We skip deprecated/obsolete keys (This should match ImageTIF.cpp)
                static const char* const keysBlackList[] =
                {
                    "dither",       // obsolete
                    "m0",           // obsolete
                    "m1",           // obsolete
                    "m2",           // obsolete
                    "m3",           // obsolete
                    "m4",           // obsolete
                    "m5",           // obsolete
                    "mipgamma",     // obsolete
                    "pixelformat",  // no need to store it in file, it's read from preset only
                };
                for (size_t i = 0; i < sizeof(keysBlackList) / sizeof(keysBlackList[0]); ++i)
                {
                    if (azstricmp(key, keysBlackList[i]) == 0)
                    {
                        return;
                    }
                }

                // determine if we need to use quotes around the value
                bool bQuotes = false;
                {
                    const char* p = value;
                    while (*p)
                    {
                        if (!IConfig::IsValidNameChar(*p))
                        {
                            bQuotes = true;
                            break;
                        }
                        ++p;
                    }
                }

                const string keyValue =
                    bQuotes
                    ? string("/") + key + "=\"" + value + "\""
                    : string("/") + key + "=" + value;

                // We skip keys with default values to avoid having too long
                // config strings.
                for (int i = 0;; ++i)
                {
                    const char* const defaultProp = CImageProperties::GetDefaultProperty(i);
                    if (!defaultProp)
                    {
                        break;
                    }
                    if (azstricmp(keyValue.c_str() + 1, defaultProp) == 0)  // '+ 1' because defaultProp doesn't have "/" in the beginning
                    {
                        return;
                    }
                }

                if (!m_result.empty())
                {
                    m_result += " ";
                }
                m_result += keyValue;
            }
            //------------------------------------------------------------

            const string& GetAsString() const
            {
                return m_result;
            }

        private:
            string m_result;
        };

        string SerializePropertiesToSettings(const CImageProperties* pProps)
        {
            QImageConfigSink sink;
            pProps->GetMultiConfig().CopyToConfig(eCP_PriorityFile, &sink);

            return sink.GetAsString();
        }
    }

    bool UpdateAndSaveSettings(const char* settingsFilename, const CImageProperties* pProps, const string* pOriginalSettings, bool bLogSettings)
    {
        const string settings = SerializePropertiesToSettings(pProps);

        if (bLogSettings)
        {
            RCLog("Saving settings:");
            RCLog("  '%s'", settings.c_str());
        }

        if (pOriginalSettings && StringHelpers::Equals(*pOriginalSettings, settings))
        {
            RCLog("New settings are equal to existing settings - file writing is skipped.");
            return true;
        }

        return UpdateAndSaveSettings(settingsFilename, settings);
    }
}
