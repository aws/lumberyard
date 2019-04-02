    /*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <ImageProcessing_precompiled.h>

#include <AzTest/AzTest.h>
#include <Processing/PixelFormatInfo.h>
#include <ImageLoader/ImageLoaders.h>
#include <Processing/ImageObject.h>
#include <Editor/EditorCommon.h>

#include <AzQtComponents/Utilities/QtPluginPaths.h>

#include <AzCore/Memory/Memory.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <Processing/PixelFormatInfo.h>
#include <BuilderSettings/BuilderSettingManager.h>
#include <BuilderSettings/PresetSettings.h>
#include <BuilderSettings/CubemapSettings.h>
#include <BuilderSettings/ImageProcessingDefines.h>

#include <Processing/ImageConvert.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/DataPatch.h>
#include <Processing/ImageToProcess.h>
#include <Compressors/Compressor.h>
#include <Converters/Cubemap.h>

#include <QFileInfo>
#include <qdir.h>
#include <QDirIterator>
#include <QIODevice>

#include <array>
#include <utility>

//Enable generate image files for result of some tests. 
//This is slow and only useful for debugging. This should be disabled for unit test
//#define DEBUG_OUTPUT_IMAGES

//There are some test functions in this test which are DISABLED. They were mainly for programming tests. 
//It's only recommanded to enable them for programming test purpose.

#include <Tests/TestTypes.h>
#include "../Source/ImageBuilderComponent.h"

using namespace ImageProcessing;

namespace UnitTest
{

class ImageProcessingTest
    : public AllocatorsFixture
{
protected:

    AZStd::unique_ptr<AZ::SerializeContext> m_context;
    
    void SetUp() override
    {
        AllocatorsFixture::SetUp();
        BuilderSettingManager::CreateInstance();

        //prepare reflection
        m_context = AZStd::make_unique<AZ::SerializeContext>();
        BuilderPluginComponent::Reflect(m_context.get());
        AZ::DataPatch::Reflect(m_context.get());
        
        // Startup default local FileIO (hits OSAllocator) if not already setup.
        if (AZ::IO::FileIOBase::GetInstance() == nullptr)
        {
            AZ::IO::FileIOBase::SetInstance(aznew AZ::IO::LocalFileIO());
        }

        //load qt plugins for some image file formats support
        AzQtComponents::PrepareQtPaths();

        InitialImageFilenames();

        ImageProcessingEditor::EditorHelper::InitPixelFormatString();
    }


    void TearDown() override
    {
        delete AZ::IO::FileIOBase::GetInstance();
        AZ::IO::FileIOBase::SetInstance(nullptr);

        m_context.release();
        BuilderSettingManager::DestroyInstance();
        CPixelFormats::DestroyInstance();
        AllocatorsFixture::TearDown();
    }
    
    //enum names for Images with specific identification
    enum ImageFeature
    {
        Image_20X16_RGBA8_Png = 0,
        Image_32X32_16bit_F_Tif,
        Image_32X32_32bit_F_Tif,
        Image_200X200_RGB8_Jpg,
        Image_512X288_RGB8_Tga,
        Image_1024X1024_RGB8_Tif,
        Image_UpperCase_Tga,
        Image_512x512_Normal_Tga,
        Image_128x128_Transparent_Tga,
        Image_237x177_RGB_Jpg,
        Image_GreyScale_Png,
        Image_BlackWhite_Png
    };

    //image file names for testing
    AZStd::map<ImageFeature, AZStd::string> m_imagFileNameMap;

    //intialial image file names for testing
    void InitialImageFilenames()
    {
        const char* fileFolder = "../Gems/ImageProcessing/Code/Tests/TestAssets/";

        m_imagFileNameMap[Image_20X16_RGBA8_Png] = fileFolder + AZStd::string("20x16_32bit.png");
        m_imagFileNameMap[Image_32X32_16bit_F_Tif] = fileFolder + AZStd::string("32x32_16bit_f.tif");
        m_imagFileNameMap[Image_32X32_32bit_F_Tif] = fileFolder + AZStd::string("32x32_32bit_f.tif");
        m_imagFileNameMap[Image_200X200_RGB8_Jpg] = fileFolder + AZStd::string("200x200_24bit.jpg");
        m_imagFileNameMap[Image_512X288_RGB8_Tga] = fileFolder + AZStd::string("512x288_24bit.tga");
        m_imagFileNameMap[Image_1024X1024_RGB8_Tif] = fileFolder + AZStd::string("1024x1024_24bit.tif");
        m_imagFileNameMap[Image_UpperCase_Tga] = fileFolder + AZStd::string("uppercase.TGA");
        m_imagFileNameMap[Image_512x512_Normal_Tga] = fileFolder + AZStd::string("512x512_RGB_N.tga");
        m_imagFileNameMap[Image_128x128_Transparent_Tga] = fileFolder + AZStd::string("128x128_RGBA8.tga");
        m_imagFileNameMap[Image_237x177_RGB_Jpg] = fileFolder + AZStd::string("237x177_RGB.jpg");
        m_imagFileNameMap[Image_GreyScale_Png] = fileFolder + AZStd::string("greyscale.png");
        m_imagFileNameMap[Image_BlackWhite_Png] = fileFolder + AZStd::string("BlackWhite.png");
    }

public:
    //helper function to save an image object to a file through QtImage
    static void SaveImageToFile(const IImageObjectPtr imageObject, const AZStd::string imageName, AZ::u32 maxMipCnt = 100)
    {
#ifndef DEBUG_OUTPUT_IMAGES
        return;
#endif
        if (imageObject == nullptr)
        {
            return;
        }

        //create the directory if it's not exist
        const char* outputDir = "../Gems/ImageProcessing/Code/Tests/TestAssets/Output/";
        QDir dir(outputDir);
        if (!dir.exists()) 
        {
            dir.mkpath(".");
        }

        //save origin file pixel format so we could use it to generate name later
        EPixelFormat originPixelFormat = imageObject->GetPixelFormat();

        //convert to RGBA8 before can be exported.
        ImageToProcess imageToProcess(imageObject);
        imageToProcess.ConvertFormat(ePixelFormat_R8G8B8A8);

        IImageObjectPtr finalImage = imageToProcess.Get();

        //for each mipmap 
        for (uint32 mip = 0; mip < finalImage->GetMipCount() && mip < maxMipCnt; mip++)
        {
            uint8* imageBuf;
            uint32 pitch;
            finalImage->GetImagePointer(mip, imageBuf, pitch);
            uint32 width = finalImage->GetWidth(mip);
            uint32 height = finalImage->GetHeight(mip);

            //generate file name
            char filePath[2048];
            azsprintf(filePath, "%s%s_%s_mip%d_%dx%d.png", outputDir, imageName.c_str()
                , CPixelFormats::GetInstance().GetPixelFormatInfo(originPixelFormat)->szName
                , mip, width, height);

            QImage qimage(imageBuf, width, height, pitch, QImage::Format_RGBA8888);
            qimage.save(filePath);
        }
    }

    static bool GetComparisonResult(IImageObjectPtr image1, IImageObjectPtr image2, QString& output)
    {
        bool isImageLoaded = true;
        bool isDifferent = false;
        
        if (image1 == nullptr)
        {
            isImageLoaded = false;
            output += ",Image 1 does not exist. ";
        }

        if (image2 == nullptr)
        {
            isImageLoaded = false;
            output += ",Image 2 does not exist. ";
        }

        if (!isImageLoaded)
        {
            return (!image1 && !image2) ? false: true;
        }
        
        // Mip
        int mip1 = image1->GetMipCount();
        int mip2 = image2->GetMipCount();
        int mipDiff = abs(mip1 - mip2);

        isDifferent |= mipDiff != 0;

        // Format
        EPixelFormat format1 = image1->GetPixelFormat();
        EPixelFormat format2 = image2->GetPixelFormat();

        isDifferent |= (format1 != format2);

        // Flag
        AZ::u32 flag1 = image1->GetImageFlags();
        AZ::u32 flag2 = image2->GetImageFlags();
        
        isDifferent |= (flag1 != flag2);

        // Size
        int memSize1 = image1->GetTextureMemory();
        int memSize2 = image2->GetTextureMemory();
        int memDiff = abs(memSize1 - memSize2);

        isDifferent |= memDiff != 0;

        // Error
        float error = GetErrorBetweenImages(image1, image2);

        static float EPSILON = 0.000001f; 
        isDifferent |= abs(error) >= EPSILON;
        
        output += QString(",%1/%2,%3,%4/%5,%6/%7,").arg(QString::number(mip1,'f',1), QString::number(mip2,'f',1), QString::number(mipDiff),
            QString(ImageProcessingEditor::EditorHelper::s_PixelFormatString[format1]),
            QString(ImageProcessingEditor::EditorHelper::s_PixelFormatString[format2]),
            QString::number(flag1, 16), QString::number(flag2, 16));

        output += QString("%1/%2,%3,%4").arg(QString(ImageProcessingEditor::EditorHelper::GetFileSizeString(memSize1).c_str()),
            QString(ImageProcessingEditor::EditorHelper::GetFileSizeString(memSize2).c_str()),
            QString(ImageProcessingEditor::EditorHelper::GetFileSizeString(memDiff).c_str()),
            QString::number(error, 'f', 8));


        return isDifferent;
    }


    static bool CompareDDSImage(const QString& imagePath1, const QString& imagePath2, QString& output)
    {
        IImageObjectPtr image1, alphaImage1, image2, alphaImage2;
        
        
        image1 = IImageObjectPtr(LoadImageFromDdsFile(imagePath1.toUtf8().constData()));
        if (image1 && image1->HasImageFlags(EIF_AttachedAlpha))
        {
            if (image1->HasImageFlags(EIF_Splitted))
            {
                alphaImage1 = IImageObjectPtr(LoadImageFromDdsFile(QString(imagePath1 + ".a").toUtf8().constData()));
            }
            else
            {
                alphaImage1 = IImageObjectPtr(LoadAttachedImageFromDdsFile(imagePath1.toUtf8().constData(), image1));
            }
        }

        image2 = IImageObjectPtr(LoadImageFromDdsFile(imagePath2.toUtf8().constData()));
        if (image2 && image2->HasImageFlags(EIF_AttachedAlpha))
        {
            if (image2->HasImageFlags(EIF_Splitted))
            {
                alphaImage2 = IImageObjectPtr(LoadImageFromDdsFile(QString(imagePath2 + ".a").toUtf8().constData()));
            }
            else
            {
                alphaImage2 = IImageObjectPtr(LoadAttachedImageFromDdsFile(imagePath2.toUtf8().constData(), image2));
            }
        }

        if (!image1 && !image2)
        {
            output += "Cannot load both image file! ";
            return false;
        }
        bool isDifferent = false;
       
        isDifferent = GetComparisonResult(image1, image2, output);


        QFileInfo fi(imagePath1);
        AZStd::string imageName = fi.baseName().toUtf8().constData();
        SaveImageToFile(image1, imageName + "_new");
        SaveImageToFile(image2, imageName + "_old");

        if (alphaImage1 || alphaImage2)
        {
            isDifferent |= GetComparisonResult(alphaImage1, alphaImage2, output);
        }

        return isDifferent;
    }
};

// test CPixelFormats related functions
TEST_F(ImageProcessingTest, TestPixelFormats)
{
    CPixelFormats& pixelFormats = CPixelFormats::GetInstance();

    //verify names which was used for legacy rc.ini
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("BC7t") == ePixelFormat_BC7t);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("ETC2A") == ePixelFormat_ETC2a);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("PVRTC4") == ePixelFormat_PVRTC4);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("BC1") == ePixelFormat_BC1);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("ETC2") == ePixelFormat_ETC2);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("BC1a") == ePixelFormat_BC1a);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("BC3") == ePixelFormat_BC3);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("BC7") == ePixelFormat_BC7);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("BC5s") == ePixelFormat_BC5s);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("EAC_RG11") == ePixelFormat_EAC_RG11);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("BC4") == ePixelFormat_BC4);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("EAC_R11") == ePixelFormat_EAC_R11);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("A8R8G8B8") == ePixelFormat_R8G8B8A8);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("BC6UH") == ePixelFormat_BC6UH);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("R9G9B9E5") == ePixelFormat_R9G9B9E5);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("X8R8G8B8") == ePixelFormat_R8G8B8X8);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("A16B16G16R16F") == ePixelFormat_R16G16B16A16F);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("G8R8") == ePixelFormat_R8G8);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("G16R16") == ePixelFormat_R16G16);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("G16R16F") == ePixelFormat_R16G16F);

    //some legacy format need to be mapping to new format. 
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("DXT1") == ePixelFormat_BC1);
    ASSERT_TRUE(pixelFormats.FindPixelFormatByLegacyName("DXT5") == ePixelFormat_BC3);

    //calculate mipmap count. no cubemap support at this moment

    //for all the non-compressed textures, if there minimum required texture size is 1x1
    for (uint32 i = 0; i < ePixelFormat_Count; i++)
    {
        EPixelFormat pixelFormat = (EPixelFormat)i;
        if (pixelFormats.IsPixelFormatUncompressed(pixelFormat))
        {
            //square, power of 2 sizes for uncompressed format which minimum required size is 1x1
            ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 128) == 8);
            ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 64, 64) == 7);
            ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 4, 4) == 3);
            ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 2, 2) == 2);
            ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 1, 1) == 1);

            //non-square, power of 2 sizes for uncompressed format which minimum required size is 1x1
            ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 64) == 8);
            ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 32) == 8);
            ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 32, 2) == 6);
            ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 2, 1) == 2);

            //Non power of 2 sizes for uncompressed format which minimum required size is 1x1
            ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 64) == 8);
            ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 32) == 8);
            ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 32, 2) == 6);
            ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 2, 1) == 2);
        }
    }

    //check function IsImageSizeValid && EvaluateImageDataSize function
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_PVRTC4, 2, 1, false) == false);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_PVRTC4, 4, 4, false) == false);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_PVRTC4, 16, 16, false) == true);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_PVRTC4, 16, 32, false) == false);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_PVRTC4, 34, 34, false) == false);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_PVRTC4, 256, 256, false) == true);

    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 2, 1, false) == false);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 16, 16, false) == true);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 16, 32, false) == true);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 34, 34, false) == false);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 256, 256, false) == true);

    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 2, 1, false) == false);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 16, 16, false) == true);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 16, 32, false) == true);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 34, 34, false) == true);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 256, 256, false) == true);

    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 2, 1, false) == true);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 16, 16, false) == true);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 16, 32, false) == true);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 34, 34, false) == true);
    ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 256, 256, false) == true);
}

// test image file loading
TEST_F(ImageProcessingTest, TestImageLoaders)
{
    //file extention support for different loader
    ASSERT_TRUE(IsExtensionSupported("jpg") == true);
    ASSERT_TRUE(IsExtensionSupported("JPG") == true);
    ASSERT_TRUE(IsExtensionSupported(".JPG") == false);
    ASSERT_TRUE(IsExtensionSupported("tga") == true);
    ASSERT_TRUE(IsExtensionSupported("TGA") == true);
    ASSERT_TRUE(IsExtensionSupported("tif") == true);
    ASSERT_TRUE(IsExtensionSupported("tiff") == true);

    IImageObjectPtr img;
    img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_1024X1024_RGB8_Tif]));

    ASSERT_TRUE(img != nullptr);
    ASSERT_TRUE(img->GetWidth(0) == 1024);
    ASSERT_TRUE(img->GetHeight(0) == 1024);
    ASSERT_TRUE(img->GetMipCount() == 1);
    ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8X8);

    //load png
    img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_20X16_RGBA8_Png]));
    ASSERT_TRUE(img != nullptr);
    ASSERT_TRUE(img->GetWidth(0) == 20);
    ASSERT_TRUE(img->GetHeight(0) == 16);
    ASSERT_TRUE(img->GetMipCount() == 1);
    ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8A8);

    //load jpg
    img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_200X200_RGB8_Jpg]));
    ASSERT_TRUE(img->GetWidth(0) == 200);
    ASSERT_TRUE(img->GetHeight(0) == 200);
    ASSERT_TRUE(img->GetMipCount() == 1);
    ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8A8);

    //tga
    img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_512X288_RGB8_Tga]));
    ASSERT_TRUE(img->GetWidth(0) == 512);
    ASSERT_TRUE(img->GetHeight(0) == 288);
    ASSERT_TRUE(img->GetMipCount() == 1);
    ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8A8);

    //image with upper case extension
    img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_UpperCase_Tga]));
    ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8A8);

    //16bits float tif
    img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_32X32_16bit_F_Tif]));
    ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R16G16B16A16F);

    //32bits float tif
    img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_32X32_32bit_F_Tif]));
    ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R32G32B32A32F);

}

TEST_F(ImageProcessingTest, PresetSettingCopyAssignmentOperatorOverload_WithDynamicallyAllocatedSettings_ReturnsTwoSeparateAllocations)
{
    PresetSettings presetSetting;
    presetSetting.m_mipmapSetting = AZStd::unique_ptr<MipmapSettings>(new MipmapSettings());
    presetSetting.m_cubemapSetting = AZStd::unique_ptr<CubemapSettings>(new CubemapSettings());

    // Explicit invoke assignment operator by splitting the operation into two lines.
    PresetSettings otherPresetSetting;
    otherPresetSetting = presetSetting;

    EXPECT_NE(otherPresetSetting.m_cubemapSetting, presetSetting.m_cubemapSetting);
    EXPECT_NE(otherPresetSetting.m_mipmapSetting, presetSetting.m_mipmapSetting);
}

TEST_F(ImageProcessingTest, PresetSettingCopyConstructor_WithDynamicallyAllocatedSettings_ReturnsTwoSeparateAllocations)
{
    PresetSettings presetSetting;
    presetSetting.m_mipmapSetting = AZStd::unique_ptr<MipmapSettings>(new MipmapSettings());
    presetSetting.m_cubemapSetting = AZStd::unique_ptr<CubemapSettings>(new CubemapSettings());

    PresetSettings otherPresetSetting(presetSetting);

    EXPECT_NE(otherPresetSetting.m_cubemapSetting, presetSetting.m_cubemapSetting);
    EXPECT_NE(otherPresetSetting.m_mipmapSetting, presetSetting.m_mipmapSetting);
}

TEST_F(ImageProcessingTest, PresetSettingEqualityOperatorOverload_WithIdenticalSettings_ReturnsEquivalent)
{
    PresetSettings presetSetting;
    PresetSettings otherPresetSetting(presetSetting);

    EXPECT_TRUE(otherPresetSetting == presetSetting);
}

TEST_F(ImageProcessingTest, PresetSettingEqualityOperatorOverload_WithDifferingDynamicallyAllocatedSettings_ReturnsUnequivalent)
{
    PresetSettings presetSetting;
    presetSetting.m_mipmapSetting = AZStd::unique_ptr<MipmapSettings>(new MipmapSettings());
    presetSetting.m_mipmapSetting->m_type = MipGenType::gaussian;

    PresetSettings otherPresetSetting(presetSetting);
    otherPresetSetting.m_mipmapSetting = AZStd::unique_ptr<MipmapSettings>(new MipmapSettings());
    otherPresetSetting.m_mipmapSetting->m_type = MipGenType::blackmanHarris;

    EXPECT_FALSE(otherPresetSetting == presetSetting);

}

//this test is to test image data won't be lost between uncompressed formats (for low to high precision or same precision)
TEST_F(ImageProcessingTest, TestConvertFormatUncompressed)
{    
    //source image
    IImageObjectPtr srcImage(LoadImageFromFile(m_imagFileNameMap[Image_200X200_RGB8_Jpg]));
    ImageToProcess imageToProcess(srcImage);

    //image pointers to hold precessed images for comparison
    IImageObjectPtr dstImage1, dstImage2, dstImage3, dstImage4, dstImage5;

    //compare four channels pixel formats
    //we will convert to target format then convert back to RGBX8 so they can compare to easy other
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8A8);
    dstImage1 = imageToProcess.Get();
    
    imageToProcess.Set(srcImage);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16G16B16A16);
    ASSERT_FALSE(srcImage->CompareImage(imageToProcess.Get())); //this is different than source image
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8A8);
    dstImage2 = imageToProcess.Get();

    imageToProcess.Set(srcImage);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16G16B16A16F);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8A8);
    dstImage3 = imageToProcess.Get();

    imageToProcess.Set(srcImage);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R32G32B32A32F);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8A8);
    dstImage4 = imageToProcess.Get();

    ASSERT_TRUE(dstImage2->CompareImage(dstImage1));
    ASSERT_TRUE(dstImage3->CompareImage(dstImage1));
    ASSERT_TRUE(dstImage4->CompareImage(dstImage1));

    // three channels formats
    imageToProcess.Set(srcImage);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
    dstImage1 = imageToProcess.Get();

    imageToProcess.Set(srcImage);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R9G9B9E5);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
    dstImage2 = imageToProcess.Get();

    ASSERT_TRUE(dstImage2->CompareImage(dstImage1));

    //convert image to all one channel formats then convert them back to RGBX8 for comparasion
    imageToProcess.Set(srcImage);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
    dstImage1 = imageToProcess.Get();

    imageToProcess.Set(srcImage);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
    dstImage2 = imageToProcess.Get();

    imageToProcess.Set(srcImage);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16F);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
    dstImage3 = imageToProcess.Get();

    imageToProcess.Set(srcImage);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R32F);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
    dstImage4 = imageToProcess.Get();

    ASSERT_TRUE(dstImage2->CompareImage(dstImage1));
    ASSERT_TRUE(dstImage3->CompareImage(dstImage1));
    ASSERT_TRUE(dstImage4->CompareImage(dstImage1));

    //convert image to all two channels formats then convert them back to RGBX8 for comparasion
    imageToProcess.Set(srcImage);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
    dstImage1 = imageToProcess.Get();

    imageToProcess.Set(srcImage);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16G16);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
    dstImage2 = imageToProcess.Get();

    imageToProcess.Set(srcImage);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16G16F);
    imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
    dstImage3 = imageToProcess.Get();

    ASSERT_TRUE(dstImage2->CompareImage(dstImage1));
    ASSERT_TRUE(dstImage3->CompareImage(dstImage1));
}

TEST_F(ImageProcessingTest, DISABLED_TestConvertPVRTC)
{
    //load builder presets
    AZStd::string buiderSetting("../Gems/ImageProcessing/Code/Source/ImageBuilderDefaultPresets.settings");
    auto outcome = BuilderSettingManager::Instance()->LoadBuilderSettings(buiderSetting, m_context.get());
    
    AZStd::vector<AZStd::string> outPaths;
    AZStd::string inputFile = "../Gems/ImageProcessing/Code/Tests/TestAssets/normalSmoothness_ddna.tif";     
    const char* outputFolder = "../Gems/ImageProcessing/Code/Tests/TestAssets/temp/";
    ImageConvertProcess* process = CreateImageConvertProcess(inputFile, outputFolder, "ios", m_context.get());
    if (process != nullptr)
    {
        //the process can be stopped if the job is cancelled or the worker is shutting down
        int step = 0;
        while (!process->IsFinished())
        {
            process->UpdateProcess();
            step++;
        }

        //get process result
        ASSERT_TRUE(process->IsSucceed());

        SaveImageToFile(process->GetOutputImage(), "rgb", 10);
        SaveImageToFile(process->GetOutputAlphaImage(), "alpha", 10);

        process->GetAppendOutputFilePaths(outPaths);
        delete process;
    }

    //ASSERT_TRUE(ConvertImageFile(inputFile, outputFolder, outPaths, "ios", m_context.get()));
    
}

TEST_F(ImageProcessingTest, DISABLED_TestConvertFormat)
{
    EPixelFormat pixelFormat;
    IImageObjectPtr srcImage;

    //images to be tested
    static const int imageCount = 5;
    ImageFeature images[imageCount] = {
        Image_20X16_RGBA8_Png, 
        Image_32X32_16bit_F_Tif, 
        Image_32X32_32bit_F_Tif , 
        Image_512x512_Normal_Tga , 
        Image_128x128_Transparent_Tga };

    for (int imageIdx = 0; imageIdx < imageCount; imageIdx++)
    {
        //get image's name and it will be used for output file name
        QFileInfo fi(m_imagFileNameMap[images[imageIdx]].c_str());
        AZStd::string imageName = fi.baseName().toUtf8().constData();

        srcImage = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[images[imageIdx]]));
        ImageToProcess imageToProcess(srcImage);

        //test ConvertFormat functions againest all the pixel formats
        for (pixelFormat = ePixelFormat_R8G8B8A8; pixelFormat < ePixelFormat_Unknown;)
        {
            imageToProcess.Set(srcImage);
            imageToProcess.ConvertFormat(pixelFormat);

            ASSERT_TRUE(imageToProcess.Get());

            //if the format is compressed and there is no compressor for it, it won't be converted to the expected format
            if (ICompressor::FindCompressor(pixelFormat, true) == nullptr
                && !CPixelFormats::GetInstance().IsPixelFormatUncompressed(pixelFormat))
            {
                ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() != pixelFormat);
            }
            else
            {
                //validate the size and it may not working for some uncompressed format
                if (!CPixelFormats::GetInstance().IsImageSizeValid(
                    pixelFormat, srcImage->GetWidth(0), srcImage->GetHeight(0), false))
                {
                    ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() != pixelFormat);
                }
                else
                {
                    ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() == pixelFormat);

                    //save the image to a file so we can check the visual result
                    SaveImageToFile(imageToProcess.Get(), imageName, 1);

                    //convert back to an uncompressed format and expect it will be successful
                    imageToProcess.ConvertFormat(ePixelFormat_R8G8B8A8);
                    ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() == ePixelFormat_R8G8B8A8);

                }
            }

            //next pixel format
            pixelFormat = EPixelFormat(pixelFormat + 1);
        }
    }
}

TEST_F(ImageProcessingTest, DISABLED_TestImageFilter)
{
    AZStd::string testImageFile = m_imagFileNameMap[Image_1024X1024_RGB8_Tif];
    IImageObjectPtr srcImage, dstImage;

    QFileInfo fi(testImageFile.c_str());
    AZStd::string imageName = fi.baseName().toUtf8().constData();

    //load src image and convert it to RGBA32F
    srcImage = IImageObjectPtr(LoadImageFromFile(testImageFile));
    ImageToProcess imageToProcess(srcImage);
    imageToProcess.ConvertFormat(ePixelFormat_R32G32B32A32F);
    srcImage = imageToProcess.Get();

    //create dst image with same size and mipmaps
    dstImage = IImageObjectPtr(
        IImageObject::CreateImage(srcImage->GetWidth(0), srcImage->GetHeight(0), 3, 
            ePixelFormat_R32G32B32A32F));

    //for each filters
    const std::array<std::pair<MipGenType, AZStd::string>, 7> allFilters =
    { 
        {
            {MipGenType::point, "point"},
            {MipGenType::box, "box" },
            { MipGenType::triangle, "triangle" },
            { MipGenType::quadratic, "Quadratic" },
            { MipGenType::blackmanHarris, "blackmanHarris" },
            { MipGenType::kaiserSinc, "kaiserSinc" }
        } 
    };

    for (std::pair<MipGenType, AZStd::string> filter : allFilters)
    {
        for (uint mip = 0; mip < dstImage->GetMipCount(); mip++)
        {
            FilterImage(filter.first, MipGenEvalType::sum,
                0, 0, imageToProcess.Get(), 0, dstImage, mip, nullptr, nullptr);
        }
        SaveImageToFile(dstImage, imageName + "_" + filter.second);
    }
}

TEST_F(ImageProcessingTest, TestColorSpaceConversion)
{
    IImageObjectPtr srcImage(LoadImageFromFile(m_imagFileNameMap[Image_GreyScale_Png]));

    ImageToProcess imageToProcess(srcImage);
    imageToProcess.GammaToLinearRGBA32F(true);
    SaveImageToFile(imageToProcess.Get(), "GammaTolinear_DeGamma", 1);
    imageToProcess.LinearToGamma();
    SaveImageToFile(imageToProcess.Get(), "LinearToGamma_DeGamma", 1);     
}

//This function can be used to modify some value in the builder setting and keep all presets uuid then save back to setting file
//It will only change the file if the file was checked out
TEST_F(ImageProcessingTest, DISABLED_ModifyBuilderSetting)
{
    AZStd::string buiderSetting("../Gems/ImageProcessing/Code/Source/ImageBuilderDefaultPresets.settings");
    QFileInfo fileInfo(buiderSetting.c_str());
    if (fileInfo.isWritable())
    {
        auto outcome = BuilderSettingManager::Instance()->LoadBuilderSettings(buiderSetting, m_context.get());
        BuilderSettingManager::Instance()->WriteBuilderSettings(buiderSetting, m_context.get());
    }
}

TEST_F(ImageProcessingTest, VerifyRestrictedPlatform)
{
    AZStd::string buiderSetting("../Gems/ImageProcessing/Code/Source/ImageBuilderDefaultPresets.settings");
    auto outcome = BuilderSettingManager::Instance()->LoadBuilderSettings(buiderSetting, m_context.get());
    PlatformNameList platforms = BuilderSettingManager::Instance()->GetPlatformList();

#ifndef AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
    ASSERT_TRUE(platforms.size() == 4);
#endif //AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
}

TEST_F(ImageProcessingTest, DISABLED_TestCubemap)
{    
    //load builder presets
    AZStd::string buiderSetting("../Gems/ImageProcessing/Code/Source/ImageBuilderDefaultPresets.settings");
    auto outcome = BuilderSettingManager::Instance()->LoadBuilderSettings(buiderSetting, m_context.get());

    const char* outputFolder = "../Gems/ImageProcessing/Code/Tests/TestAssets/temp/";
    AZStd::string inputFile;
    AZStd::vector<AZStd::string> outPaths;

    inputFile = "../Engine/EngineAssets/Shading/defaultProbe_cm.tif";

    IImageObjectPtr srcImage(LoadImageFromFile(inputFile));
    ImageToProcess imageToProcess(srcImage);
    imageToProcess.ConvertCubemapLayout(CubemapLayoutVertical);
    SaveImageToFile(imageToProcess.Get(), "Vertical", 100);
    imageToProcess.ConvertCubemapLayout(CubemapLayoutHorizontalCross);
    SaveImageToFile(imageToProcess.Get(), "HorizontalCross", 100);
    imageToProcess.ConvertCubemapLayout(CubemapLayoutVerticalCross);
    SaveImageToFile(imageToProcess.Get(), "VerticalCross", 100);
    imageToProcess.ConvertCubemapLayout(CubemapLayoutHorizontal);
    SaveImageToFile(imageToProcess.Get(), "VerticalHorizontal", 100);

    ImageConvertProcess* process = CreateImageConvertProcess(inputFile, outputFolder, "pc");

    if (process != nullptr)
    {
        int step = 0;
        while (!process->IsFinished())
        {
            process->UpdateProcess();
            step++;
            char name[100];
            azsprintf(name, "cubemap_%d", step);
            //SaveImageToFile(process->GetOutputImage(), name, 1);
        }

        //get process result
        ASSERT_TRUE(process->IsSucceed());

        SaveImageToFile(process->GetOutputImage(), "cubemap", 100);
        SaveImageToFile(process->GetOutputDiffCubemap(), "diffCubemap", 100);
        SaveImageToFile(process->GetOutputAlphaImage(), "alpha", 1);
        process->GetAppendOutputFilePaths(outPaths);

        delete process;
    }
}

//test image conversion for builder
TEST_F(ImageProcessingTest, DISABLED_TestBuilderImageConvertor)
{  
    AZStd::string oldCacheFolder = "E:/Javelin_old_tex_cache/textures";
    AZStd::string srcFolder = "E:/Javelin_NWLYDev/dev/Assets/Textures";

    //load builder presets
    AZStd::string buiderSetting("../Gems/ImageProcessing/Code/Source/ImageBuilderDefaultPresets.settings");
    auto outcome = BuilderSettingManager::Instance()->LoadBuilderSettings(buiderSetting, m_context.get());

    const char* outputFolder = "../Gems/ImageProcessing/Code/Tests/TestAssets/temp/";
    AZStd::string inputFile;
    AZStd::vector<AZStd::string> outPaths;
    
    inputFile = srcFolder + "/terrain/cry/detail/grass_with_stones_displ.tif";
    inputFile = m_imagFileNameMap[Image_128x128_Transparent_Tga];
    AZStd::string oldFile = oldCacheFolder + "/terrain/cry/detail/grass_with_stones_displ.dds";
    ImageConvertProcess* process = CreateImageConvertProcess(inputFile, outputFolder, "pc", m_context.get());
    
    if (process != nullptr)
    {
        //the process can be stopped if the job is cancelled or the worker is shutting down
        int step = 0;
        while (!process->IsFinished() )
        {
            process->UpdateProcess();
            step++;
        }

        //get process result
        ASSERT_TRUE(process->IsSucceed());

        SaveImageToFile(process->GetOutputImage(), "rgb", 10);
        SaveImageToFile(process->GetOutputAlphaImage(), "alpha", 10);

        process->GetAppendOutputFilePaths(outPaths);
                
        QString output;
        //CompareDDSImage(outPaths[0].c_str(), oldFile.c_str(), output);
        delete process;
    }



/*  //test cases for different presets
    //ddna
    inputFile = "../SamplesProject/Objects/ParticleAssets/ShowRoom/showroom_pipe_blue_001_m_ddna.tif";
    ASSERT_TRUE(ConvertImageFile(inputFile, outputFolder, outPaths, m_context.get()));
    //cubemap
    inputFile = "../SamplesProject/Levels/Samples/Camera_Sample/Cubemaps/noon_cm.tif";
    ASSERT_TRUE(ConvertImageFile(inputFile, outputFolder, outPaths, m_context.get()));
    //albedo
    inputFile = "../SamplesProject/Objects/ParticleAssets/ShowRoom/showroom_steel_brushed_001_diff.tif";
    ASSERT_TRUE(ConvertImageFile(inputFile, outputFolder, outPaths, m_context.get()));
    inputFile = "../SamplesProject/materials/pbs_reference/light_leather_diff.tif";
    ASSERT_TRUE(ConvertImageFile(inputFile, outputFolder, outPaths, m_context.get()));
    inputFile = "../Gems/PBSreferenceMaterials/Assets/materials/pbs_reference/brushed_steel.tif";
    ASSERT_TRUE(ConvertImageFile(inputFile, outputFolder, outPaths, m_context.get()));
    //ui ReferenceImage auto preset
    inputFile = "../Bems/UiBasics/Assets/UI/Textures/Prefab/textinput_normal.tif";
    ASSERT_TRUE(ConvertImageFile(inputFile, outputFolder, outPaths, m_context.get()));
    //albedo with generic alpha auto preset
    inputFile = "../SamplesProject/textures/GettingStartedTextures/LY_Logo_Beaver.tif";
    ASSERT_TRUE(ConvertImageFile(inputFile, outputFolder, outPaths, m_context.get()));
    //color chart
    inputFile = "../Gems/PBSreferenceMaterials/Assets/materials/pbs_reference/colorcharts/debug_contrast_low_cch.tif";
    ASSERT_TRUE(ConvertImageFile(inputFile, outputFolder, outPaths, m_context.get()));
*/
}


//test image loading function for output dds files
TEST_F(ImageProcessingTest, DISABLED_TestLoadDdsImage)
{
    IImageObjectPtr originImage, alphaImage;
    AZStd::string inputFolder = "../Cache/SamplesProject/pc/samplesproject/engineassets/texturemsg/";
    AZStd::string inputFile;
    
    inputFile = "E:/Javelin_NWLYDev/dev/Cache/Assets/pc/assets/textures/blend_maps/moss/jav_moss_ddn.dds";
 
    IImageObjectPtr newImage = IImageObjectPtr(LoadImageFromDdsFile(inputFile));
    if (newImage->HasImageFlags(EIF_AttachedAlpha))
    {
        if (newImage->HasImageFlags(EIF_Splitted))
        {
            alphaImage = IImageObjectPtr(LoadImageFromDdsFile(inputFile+".a"));

        }
        else
        {
            alphaImage = IImageObjectPtr(LoadAttachedImageFromDdsFile(inputFile, newImage));
        }
    }

    SaveImageToFile(newImage, "jav_moss_ddn", 10);
}

TEST_F(ImageProcessingTest, DISABLED_CompareOutputImage)
{
    AZStd::string curretTextureFolder = "../TestAssets/TextureAssets/assets_new/textures";
    AZStd::string oldTextureFolder = "../TestAssets/TextureAssets/assets_old/textures";
    bool outputOnlyDifferent = false;
    QDirIterator it(curretTextureFolder.c_str(), QStringList() << "*.dds", QDir::Files, QDirIterator::Subdirectories);
    QFile f("../texture_comparison_output.csv");
    f.open(QIODevice::ReadWrite | QIODevice::Truncate);
    // Write a header for csv file
    f.write("Texture Name, Path, Mip new/old, MipDiff, Format new/old, Flag new/old, MemSize new/old, MemDiff, Error, AlphaMip new/old, AlphaMipDiff, AlphaFormat new/old, AlphaFlag new/old, AlphaMemSize new/old, AlphaMemDiff, AlphaError\r\n");
    int i = 0;
    while (it.hasNext())
    {
        i++;
        it.next();

        QString fileName = it.fileName();
        QString newFilePath = it.filePath();
        QString sharedPath = QString(newFilePath).remove(curretTextureFolder.c_str());
        QString oldFilePath = QString(oldTextureFolder.c_str()) + sharedPath;
        QString output;
        if (QFile::exists(oldFilePath))
        {
            bool isDifferent = CompareDDSImage(newFilePath, oldFilePath, output);
            if (outputOnlyDifferent && !isDifferent)
            {
                continue;
            }
            else
            {
                f.write(fileName.toUtf8().constData());
                f.write(",");
                f.write(sharedPath.toUtf8().constData());
                f.write(output.toUtf8().constData());
            }
        }
        else
        {
            f.write(fileName.toUtf8().constData());
            f.write(",");
            f.write(sharedPath.toUtf8().constData());
            output += ",No old file for comparison!";
            f.write(output.toUtf8().constData());
        }
        f.write("\r\n");
    }
    f.close();
}

class ImageProcessingSerializationTest
    : public AllocatorsFixture
{
protected:
    AZStd::unique_ptr<AZ::SerializeContext> m_context;

    void SetUp() override
    {
        AllocatorsFixture::SetUp();
        BuilderSettingManager::CreateInstance();

        m_context = AZStd::make_unique<AZ::SerializeContext>();
        BuilderPluginComponent::Reflect(m_context.get());
        AZ::DataPatch::Reflect(m_context.get());

        // Startup default local FileIO (hits OSAllocator) if not already setup.
        if (AZ::IO::FileIOBase::GetInstance() == nullptr)
        {
            AZ::IO::FileIOBase::SetInstance(aznew AZ::IO::LocalFileIO());
        }
    }

    void TearDown() override
    {
        delete AZ::IO::FileIOBase::GetInstance();
        AZ::IO::FileIOBase::SetInstance(nullptr);

        m_context.release();
        BuilderSettingManager::DestroyInstance();
        CPixelFormats::DestroyInstance();
        AllocatorsFixture::TearDown();
    }
};

TEST_F(ImageProcessingSerializationTest, LoadBuilderSettingsFromRC_SerializingLegacyDataIn_InvalidFiles)
{
    AZStd::string filepath("../Gems/ImageProcessing/Code/Tests/TestAssets/rc.ini_Missing");
    ASSERT_FALSE(BuilderSettingManager::Instance()->LoadBuilderSettingsFromRC(filepath).IsSuccess());

    filepath = "../Bin64/rc/rc.ini";
    auto outcome = BuilderSettingManager::Instance()->LoadBuilderSettingsFromRC(filepath);
    ASSERT_TRUE(outcome.IsSuccess());

    AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;

    // Load legacy texture settings from file that not exists
    TextureSettings legacyTextureSetting;
    AZStd::string notExistingFile = "../Gems/ImageProcessing/Code/Tests/TestAssets/NotExistingFile";
    auto legacyLoadOutcome = TextureSettings::LoadLegacyTextureSettingFromFile("", notExistingFile, legacyTextureSetting, m_context.get());
    EXPECT_FALSE(legacyLoadOutcome.IsSuccess());

    // Load legacy texture settings from file whose format is wrong

    // Wrong override data
    AZStd::string wrongFormatFile = "../Gems/ImageProcessing/Code/Tests/TestAssets/invalid.exportsettings";
    AZStd::string wrongFormatContent = "/autooptimizefile=0 /preset=Diffuse_highQ /reduce=\"es3:0,randomdata,ios:3,osx_gl:0,pc:4\" /ser=0";
    if (AZ::IO::FileIOBase::GetInstance()->Open(wrongFormatFile.c_str(), AZ::IO::OpenMode::ModeWrite, fileHandle))
    {
        AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, wrongFormatContent.c_str(), wrongFormatContent.size());
        AZ::IO::FileIOBase::GetInstance()->Close(fileHandle);
    }
    else
    {
        EXPECT_TRUE(false);
    }
    legacyLoadOutcome = TextureSettings::LoadLegacyTextureSettingFromFile("", wrongFormatFile, legacyTextureSetting, m_context.get());
    EXPECT_FALSE(legacyLoadOutcome.IsSuccess());

    // Wrong format data
    wrongFormatContent = "//// ,&*&#$@#/preset=Diffuse_highQ / //reduce=0 /ser=0";
    if (AZ::IO::FileIOBase::GetInstance()->Open(wrongFormatFile.c_str(), AZ::IO::OpenMode::ModeWrite, fileHandle))
    {
        AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, wrongFormatContent.c_str(), wrongFormatContent.size());
        AZ::IO::FileIOBase::GetInstance()->Close(fileHandle);
    }
    legacyLoadOutcome = TextureSettings::LoadLegacyTextureSettingFromFile("", wrongFormatFile, legacyTextureSetting, m_context.get());
    EXPECT_FALSE(legacyLoadOutcome.IsSuccess());

    AZ::IO::FileIOBase::GetInstance()->Remove(wrongFormatFile.c_str());
}


TEST_F(ImageProcessingSerializationTest, TextureSettingReflect_SerializingLegacyDataIn_EmbeddedSetting)
{
    AZStd::string buiderSetting("../Gems/ImageProcessing/Code/Source/ImageBuilderDefaultPresets.settings");
    auto outcome = BuilderSettingManager::Instance()->LoadBuilderSettings(buiderSetting, m_context.get());

    // Load legacy texture settings
    TextureSettings legacyTextureSetting;
    AZStd::string textureFilepath = "../Gems/ImageProcessing/Code/Tests/TestAssets/Lenstexture_dirtyglass.tif";
    AZStd::string textureSetting = LoadEmbeddedSettingFromFile(textureFilepath);
    EXPECT_FALSE(textureSetting.empty());

    auto legacyLoadOutcome = TextureSettings::LoadLegacyTextureSetting(textureFilepath, textureSetting, legacyTextureSetting, m_context.get());
    // Ensure we loaded and parsed the texture settings correctly.
    EXPECT_TRUE(legacyLoadOutcome.IsSuccess());
    EXPECT_EQ(legacyTextureSetting.m_preset, BuilderSettingManager::Instance()->GetPresetIdFromName("LensOptics"));
}

TEST_F(ImageProcessingSerializationTest, TextureSettingReflect_SerializingLegacyDataIn_CommonAndPlatformSpecificSettingsAreSerializedCorrectly)
{
    AZStd::string buiderSetting("../Gems/ImageProcessing/Code/Source/ImageBuilderDefaultPresets.settings");
    auto outcome = BuilderSettingManager::Instance()->LoadBuilderSettings(buiderSetting, m_context.get());

    // Load legacy texture settings
    TextureSettings legacyTextureSetting;
    AZStd::string textureFilepath = "../Gems/ImageProcessing/Code/Tests/TestAssets/1024x1024_24bit.tif";
    auto legacyLoadOutcome = TextureSettings::LoadLegacyTextureSettingFromFile(textureFilepath, 
        textureFilepath + TextureSettings::legacyExtensionName, legacyTextureSetting, m_context.get());

    // Ensure we loaded and parsed the texture settings correctly.
    EXPECT_TRUE(legacyLoadOutcome.IsSuccess());
    EXPECT_EQ(legacyTextureSetting.m_mipGenType, MipGenType::kaiserSinc);
    EXPECT_EQ(legacyTextureSetting.m_preset, BuilderSettingManager::Instance()->GetPresetIdFromName("Albedo"));
    EXPECT_EQ(legacyTextureSetting.m_mipAlphaAdjust[0], 62);
    EXPECT_EQ(legacyTextureSetting.m_suppressEngineReduce, false);

    // Ensure overrides are properly parsed as well.
    {
        TextureSettings iosTextureSettings;
        auto iosOutcome = TextureSettings::GetPlatformSpecificTextureSetting("ios", legacyTextureSetting, iosTextureSettings, m_context.get());
        EXPECT_TRUE(iosOutcome.IsSuccess());
        EXPECT_EQ(iosTextureSettings.m_sizeReduceLevel, 3);
    }
}

TEST_F(ImageProcessingSerializationTest, TextureSettingReflect_SerializingModernDataOutThenIn_PreSerializedAndPostSerializedDataIsEquivalent)
{
    AZStd::string buiderSetting("../Gems/ImageProcessing/Code/Source/ImageBuilderDefaultPresets.settings");
    auto outcome = BuilderSettingManager::Instance()->LoadBuilderSettings(buiderSetting, m_context.get());

    // Load legacy texture settings
    TextureSettings legacyTextureSetting;
    AZStd::string textureFilepath = "../Gems/ImageProcessing/Code/Tests/TestAssets/1024x1024_24bit.tif";
    auto legacyLoadOutcome = TextureSettings::LoadLegacyTextureSettingFromFile(textureFilepath, 
        textureFilepath+TextureSettings::legacyExtensionName, legacyTextureSetting, m_context.get());

    // Let's make modifications to the loaded texture setting
    // Modification1: Set reduce level for common settings.
    // Modification2: Set reduce level for iOS-override settings.
    legacyTextureSetting.m_sizeReduceLevel = 1337;
    TextureSettings iosOverride = legacyTextureSetting;
    iosOverride.m_sizeReduceLevel = 0xDAD;
    legacyTextureSetting.ApplySettings(iosOverride, "ios", m_context.get());

    // Write the modified texture settings to file, using AZ::Serialization.
    AZStd::string modernMetafilePath = textureFilepath + TextureSettings::modernExtensionName;
    auto writeOutcome = TextureSettings::WriteTextureSetting(modernMetafilePath, legacyTextureSetting, m_context.get());
    EXPECT_TRUE(writeOutcome.IsSuccess());

    // Load the modified settings back to memory, using AZ::Serialization
    TextureSettings modernTextureSetting;
    auto modernLoadOutcome = TextureSettings::LoadTextureSetting(modernMetafilePath, modernTextureSetting, m_context.get());
    
    // Ensure what we just serialized-in is identical to what we serialized-out.
    // The comparison operator also compares overrides.
    EXPECT_TRUE(modernLoadOutcome.IsSuccess());
    EXPECT_TRUE(modernTextureSetting.Equals(legacyTextureSetting, m_context.get()));

    // Remove the temp file that was written out.
    AZ::IO::FileIOBase::GetInstance()->Remove(modernMetafilePath.c_str());
}

TEST_F(ImageProcessingSerializationTest, TextureSettingReflect_SerializingModernDataInAndOut_WritesAndParsesFileAccurately)
{
    AZStd::string filepath = "test.xml";

    // Fill-in structure with test data  
    TextureSettings fakeTextureSettings;
    fakeTextureSettings.m_preset = AZ::Uuid::CreateRandom();
    fakeTextureSettings.m_sizeReduceLevel = 0;
    fakeTextureSettings.m_suppressEngineReduce = true;
    fakeTextureSettings.m_enableMipmap = false;
    fakeTextureSettings.m_maintainAlphaCoverage = true;
    fakeTextureSettings.m_mipAlphaAdjust = { 0xDEAD, 0xBADBEEF, 0xBADC0DE, 0xFEEFEE, 0xBADF00D, 0xC0FFEE };
    fakeTextureSettings.m_mipGenEval = MipGenEvalType::max;
    fakeTextureSettings.m_mipGenType = MipGenType::quadratic;

    // Write test data to file
    auto writeOutcome = TextureSettings::WriteTextureSetting(filepath, fakeTextureSettings, m_context.get());
    EXPECT_TRUE(writeOutcome.IsSuccess());

    // Parse test data to file
    TextureSettings parsedFakeTextureSettings;
    auto readOutcome = TextureSettings::LoadTextureSetting(filepath, parsedFakeTextureSettings, m_context.get());
    EXPECT_TRUE(readOutcome.IsSuccess());
    EXPECT_TRUE(parsedFakeTextureSettings.Equals(fakeTextureSettings, m_context.get()));

    // Delete temp data
    AZ::IO::FileIOBase::GetInstance()->Remove(filepath.c_str());
}

TEST_F(ImageProcessingSerializationTest, BuilderSettingsReflect_SerializingDataInAndOut_WritesAndParsesFileAccurately)
{    
    AZStd::string buildSettingsFilepath = "../Gems/ImageProcessing/Code/Tests/TestAssets/tempPresets.settings";
    AZStd::string rcFilePath = "../Bin64/rc/rc.ini";
    
    auto loadOutcome = BuilderSettingManager::Instance()->LoadBuilderSettingsFromRC(rcFilePath);
    ASSERT_TRUE(loadOutcome.IsSuccess());

    //Save the preset loaded from rc.ini for later comparison
    AZ::Uuid oldPresetSettingsUuid = BuilderSettingManager::Instance()->GetPresetIdFromName("NormalsFromDisplacement");
    const PresetSettings oldPresetSetting = *BuilderSettingManager::Instance()->GetPreset(oldPresetSettingsUuid, "pc");

    //Save builder settings to new file format
    auto writeOutcome = BuilderSettingManager::Instance()->WriteBuilderSettings(buildSettingsFilepath, m_context.get());
    ASSERT_TRUE(writeOutcome.IsSuccess());

    //Re-load Builder Settings    
    auto reloadOutcome = BuilderSettingManager::Instance()->LoadBuilderSettings(buildSettingsFilepath, m_context.get());
    ASSERT_TRUE(reloadOutcome.IsSuccess());

    //Find the same preset
    AZ::Uuid newPresetSettingsUuid = BuilderSettingManager::Instance()->GetPresetIdFromName("NormalsFromDisplacement");
    const PresetSettings newPresetSetting = *BuilderSettingManager::Instance()->GetPreset(newPresetSettingsUuid, "pc");

    // Delete temp data
    AZ::IO::FileIOBase::GetInstance()->Remove(buildSettingsFilepath.c_str());

    //make sure the preset loaded from RC.ini is same as the preset loaded from builder setting
    ASSERT_EQ(oldPresetSetting, newPresetSetting);
}

}

AZ_UNIT_TEST_HOOK();
