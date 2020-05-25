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
#include <AzTest/AzTest.h>
#include <Util/EditorUtils.h>
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <Lib/Tests/IEditorMock.h>

#include <Terrain/LayerWeight.h>
#include <Terrain/Layer.h>
#include <Terrain/Heightmap.h>
#include <Util/ImagePainter.h>
#include <GameExporter.h>

namespace TerrainTests
{
    class TerrainTestFixture
        : public testing::Test
    {
    public:

        bool ImagesAreEqual(const CImageEx& srcImage, const CImageEx& destImage)
        {
            bool imagesEqual = true;

            imagesEqual = imagesEqual && (srcImage.GetHeight() == destImage.GetHeight());
            imagesEqual = imagesEqual && (srcImage.GetWidth() == destImage.GetWidth());

            if (imagesEqual)
            {
                for (int y = 0; y < srcImage.GetHeight(); y++)
                {
                    for (int x = 0; x < srcImage.GetWidth(); x++)
                    {
                        imagesEqual = imagesEqual && (srcImage.ValueAt(x, y) == destImage.ValueAt(x, y));
                    }
                }
            }

            return imagesEqual;
        }

        using PerPixelCallback = AZStd::function<uint8(int x, int y, int width, int height)>;
        void FillImage(CImageEx& image, int width, int height, PerPixelCallback fillPixel)
        {
            image.Allocate(width, height);
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    uint8 pixel = fillPixel(x, y, width, height);
                    image.ValueAt(x, y) = pixel | (pixel << 8) | (pixel << 16) | (0xFF << 24);
                }
            }
        }

        bool LayerWeightsAreEqual(LayerWeight weight, const uint8 (&testIds)[ITerrain::SurfaceWeight::WeightCount], const uint8 (&testWeights)[ITerrain::SurfaceWeight::WeightCount])
        {
            for (int weightIdx = 0; weightIdx < ITerrain::SurfaceWeight::WeightCount; weightIdx++)
            {
                if ((weight.Ids[weightIdx] != testIds[weightIdx]) || (weight.Weights[weightIdx] != testWeights[weightIdx]))
                {
                    return false;
                }
            }

            return true;
        }

    protected:
        void SetUp() override
        {
            SetIEditor(&m_editorMock);
            ON_CALL(m_editorMock, GetSystem()).WillByDefault(::testing::Return(nullptr));
            ON_CALL(m_editorMock, GetRenderer()).WillByDefault(::testing::Return(nullptr));

            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
        }

        void TearDown() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();

            SetIEditor(nullptr);
        }

        ::testing::NiceMock<CEditorMock> m_editorMock;
    };

    TEST_F(TerrainTestFixture, Test_LayerWeights_ZeroWeightShouldBeUndefined)
    {
        AZStd::vector<uint8> layerWeights{ 0, 0, 0 };
        AZStd::vector<uint8> layerIds{ 1, 2, 3 };

        // Test:  When initializing a LayerWeight with all 0 weights, the layer IDs
        // should all get set to "Undefined".
        LayerWeight layerWeight(layerIds, layerWeights);
        for (int layer = 0; layer < layerWeight.WeightCount; layer++)
        {
            EXPECT_TRUE(layerWeight.GetLayerId(layer) == LayerWeight::Undefined);
        }

        // Test:  Even when setting a specific layer ID to a 0 weight, its weight should
        // get set to "Undefined".
        layerWeight.SetWeight(1, 0);
        for (int layer = 0; layer < layerWeight.WeightCount; layer++)
        {
            EXPECT_TRUE(layerWeight.GetLayerId(layer) == LayerWeight::Undefined);
        }
    }

    TEST_F(TerrainTestFixture, Test_Heightmap_CreationAndResetTriviallyWork)
    {
        CHeightmap heightmap;
        heightmap.SetStandaloneMode(true);
        heightmap.Reset(2048, 1);

        EXPECT_TRUE(heightmap.GetWidth() == 2048);
        EXPECT_TRUE(heightmap.GetHeight() == 2048);
    }

    TEST_F(TerrainTestFixture, Test_Heightmap_NormalizeWorksOnFlatTerrain)
    {
        CHeightmap heightmap;
        heightmap.SetStandaloneMode(true);

        const float testHeight = 50.0f;

        // Set to a tiny heightmap for easier testing.
        heightmap.Reset(128, 1);
        heightmap.SetMaxHeight(100.0f);
        heightmap.InitHeight(testHeight);

        // Verify our heightmap is completely flat at a known value.
        for (int y = 0; y < heightmap.GetHeight(); y++)
        {
            for (int x = 0; x < heightmap.GetWidth(); x++)
            {
                EXPECT_TRUE(heightmap.GetXY(x, y) == testHeight);
            }
        }

        // Normalize the heightmap
        heightmap.Normalize();

        // Verify the heightmap is *still* completely flat at a known value.
        for (int y = 0; y < heightmap.GetHeight(); y++)
        {
            for (int x = 0; x < heightmap.GetWidth(); x++)
            {
                EXPECT_TRUE(heightmap.GetXY(x, y) == testHeight);
            }
        }
    }

    TEST_F(TerrainTestFixture, Test_Heightmap_NormalizeWorksOnNonFlatTerrain)
    {
        CHeightmap heightmap;
        heightmap.SetStandaloneMode(true);

        struct TestPoint
        {
            int   x;
            int   y;
            float preNormalizeHeight;
            float postNormalizeHeight;
        };

        // This is the max height that we'll scale to when we normalize.
        const float maxHeight = 100.0f;
        // The middle of the height range.
        const float avgHeight = maxHeight / 2.0f;

        TestPoint tests[] = 
        {
            { 0, 0, 25.0f, 0.0f },          // (0,0) - set to a pre-normalize min height of 25.  Normalize scales down to 0.
            { 0, 1, 50.0f, avgHeight },     // (0,1) - set to a pre-normalize average height of 50.  Normalize scales it to the midpoint, which is still 50.
            { 0, 2, 75.0f, maxHeight }      // (0,2) - set to a pre-normalize max height of 75.  Normalize scales up to 100.
        };

        // Set to a tiny heightmap for easier testing.
        heightmap.Reset(128, 1);
        heightmap.SetMaxHeight(maxHeight);
        heightmap.InitHeight(avgHeight);

        for (auto& test : tests)
        {
            heightmap.SetXY(test.x, test.y, test.preNormalizeHeight);
            EXPECT_TRUE(heightmap.GetXY(test.x, test.y) == test.preNormalizeHeight);
        }

        // Normalize the heightmap
        heightmap.Normalize();

        for (auto& test : tests)
        {
            EXPECT_TRUE(heightmap.GetXY(test.x, test.y) == test.postNormalizeHeight);
        }
    }

    TEST_F(TerrainTestFixture, Test_Heightmap_ImportExportLayerWeights)
    {
        // Trivial test of import/export: Create N non-overlapping splat maps,
        // import them, and export them.  Verify that the output exactly matches
        // the input.

        constexpr int worldSize = 32;
        constexpr int maxLayers = 5;

        // Create a small heightmap
        CHeightmap heightmap;
        heightmap.SetStandaloneMode(true);
        heightmap.Reset(worldSize, 1);

        CImageEx inputSplats[maxLayers];
        AZStd::vector<uint8> layerIds;

        // Fill our input splatmaps so that every N'th pixel is filled, where that pixel is exactly
        // different on every layer.
        for (int layer = 0; layer < maxLayers; layer++)
        {
            FillImage(inputSplats[layer], worldSize, worldSize, [layer, maxLayers](int x, int y, int width, int height)
            {
                return (((y * width) + x + layer) % maxLayers == 0) ? 0xFF : 0;
            });
            layerIds.push_back(layer);
        }

        // Pass the splatmaps to the heightmap
        heightmap.SetLayerWeights(layerIds, inputSplats, maxLayers);

        // Read back the splatmaps, we should get back exactly what we sent in.
        for (int layer = 0; layer < maxLayers; layer++)
        {
            CImageEx splatImage;
            heightmap.GetLayerWeights(layer, &splatImage);
            EXPECT_TRUE(ImagesAreEqual(inputSplats[layer], splatImage));
        }
    }

    TEST_F(TerrainTestFixture, Test_Heightmap_ExportNormalizedLayerWeights)
    {
        // Verify that when we send in inputs that need to get normalized, the output
        // exported splatmaps contain the expected normalized values, not the input values.

        constexpr int worldSize = 32;
        constexpr int maxLayers = 5;

        // Create a small heightmap
        CHeightmap heightmap;
        heightmap.SetStandaloneMode(true);
        heightmap.Reset(worldSize, 1);

        CImageEx inputSplats[maxLayers];
        AZStd::vector<uint8> layerIds;

        // Fill our input splatmaps equally with the same arbitrary value of 0x10 everywhere.
        for (int layer = 0; layer < maxLayers; layer++)
        {
            FillImage(inputSplats[layer], worldSize, worldSize, [](int x, int y, int width, int height)
            {
                return 0x10;
            });

            layerIds.push_back(layer);
        }

        // Pass the splatmaps to the heightmap.
        heightmap.SetLayerWeights(layerIds, inputSplats, maxLayers);

        // Build up the splatmaps that we expect to receive back.  Note that we expect these to be
        // different than the inputs due to the limitation of 3 layers and normalization.
        CImageEx outputSplats[maxLayers];
        for (int layer = 0; layer < maxLayers; layer++)
        {
            FillImage(outputSplats[layer], worldSize, worldSize, [layer, maxLayers](int x, int y, int width, int height)
            {
                switch (layer)
                {
                    // Splatmaps only support 3 layers.  If they're all weighted equally, the last two in the list
                    // (highest priority) will preserve their weights.  The third will get a normalized weight that
                    // contains the remainder.  All other layers will get 0.
                    case 2:
                        return (0xFF - 0x10 - 0x10);
                    case 3:
                    case 4:
                        return 0x10;
                }

                return 0x00;
            });
        }

        // Compare the results vs our expectations.
        for (int layer = 0; layer < maxLayers; layer++)
        {
            CImageEx splatImage;
            heightmap.GetLayerWeights(layer, &splatImage);
            EXPECT_TRUE(ImagesAreEqual(outputSplats[layer], splatImage));
        }
    }

    TEST_F(TerrainTestFixture, Test_ImagePainter_LargeValuesRoundCorrectly)
    {
        // LY-108871:  If surface weights already exist at a point, trying to paint over it
        // with an opaque brush fails to completely erase the previous surface.  Due to a
        // rounding error, the result is 2 layers of weights 254 and 1 unless the center point
        // of the brush *exactly* hits a 1-meter boundary.  Any fluctuation (like 1.001) will prevent
        // it from producing a weight of 255.

        // This unit test verifies that the rounding error is fixed, and painting slightly off the
        // pixel boundary still produces a correct result of 1 layer of weight 255.

        // Create a small, arbitrary heightmap.  It won't be used for anything in this
        // test, but needs to exist to create the paintbrush and the arbitrary layer.
        CHeightmap heightmap;
        constexpr int worldSize = 32;
        heightmap.SetStandaloneMode(true);
        heightmap.Reset(worldSize, 1);

        // Make GetIEditor()->GetHeightmap() return our heightmap created above.
        ON_CALL(m_editorMock, GetHeightmap()).WillByDefault(::testing::Return(&heightmap));

        // Create an arbitrary layer, needed to create the paintbrush.
        CLayer layer;

        // Create a paintbrush with no masking or flood fills, so that we can directly test using
        // the paintbrush's hardness, radius, and color settings.
        const bool floodFill = false;
        const bool maskByLayerSettings = false;
        const uint32 layerIdMask = 0xFFFFFFFF;
        SEditorPaintBrush brush(heightmap, layer, maskByLayerSettings, layerIdMask, floodFill);

        // Create a 3x3 image - the goal is to get a max weight in the center pixel where we paint,
        // and blended values in the surrounding 8 pixels.
        TImage<LayerWeight> testImage;
        const int imageWidth = 3;
        const int imageHeight = 3;
        testImage.Allocate(imageWidth, imageHeight);

        // Initially clear the image to only have layer 0 at weight 255.
        for (int y = 0; y < testImage.GetHeight(); ++y)
        {
            for (int x = 0; x < testImage.GetWidth(); ++x)
            {
                testImage.ValueAt(x, y) = LayerWeight(0);
            }
        }

        // Set the paintbrush hardness and "color" (i.e. layer ID):
        brush.hardness = 1.0f;  // full hardness at the center of the paintbrush, with falloff
        brush.color = 1;        // paint with layer ID 1

        // Create a 1 meter radius brush to make it easy to predict how the math will turn out.  By using 1 meter,
        // the distance from the center of the brush will be the exact amount we pass in as a fractional meter offset.
        // Note: PaintBrush() expects radius to be a fraction of image width, so 1 meter is represented as 1/imageWidth.
        brush.fRadius = 1.0f / aznumeric_cast<float>(imageWidth);   

        // Set our "center point" for painting to be just off of the pixel boundary for (1, 1).
        // The choice of offset is based on the knowledge that the calculations are in 0-1 space, and scaled
        // back up to 0-255, and that we have linear falloff based on distance from the paintbrush center.
        // To produce a weight result between 254.5 and 255 at our center pixel, we need the distance from the brush
        // center to be greater than 0, and less than half of 1/256 (i.e. less than 1/512).  Hence, the choice of 1/513.

        // We only apply the offset in one direction to simplify the choice of offset.  If we apply it in both directions,
        // we would need a choice where sqrt(offsetX^2 + offsetY^2) is < 1/512.  By setting offsetX or offsetY to 0, the offset
        // itself is the exact value that needs to be < 1/512.

        CImagePainter imagePainter;
        const float offsetToCauseRoundingError = 1.0f / 513.0f;
        const float xPct = (1.0f + offsetToCauseRoundingError) / aznumeric_cast<float>(imageWidth);
        const float yPct = 1.0f / aznumeric_cast<float>(imageHeight);
        imagePainter.PaintBrush(xPct, yPct, testImage, brush);

        // Center pixel should be max weight for layer 1
        EXPECT_TRUE(LayerWeightsAreEqual(testImage.ValueAt(1, 1), { 1, ITerrain::SurfaceWeight::Undefined, ITerrain::SurfaceWeight::Undefined }, { 255, 0, 0 }));
    }

    TEST(TerrainSetupWithOctree, TerrainNotSetupWithEmptyOctree)
    {
        const size_t octreeCompiledDataSize = 0;

        bool setupSuccessful = false;
        // function will not be called
        SetupTerrainInfo(
            octreeCompiledDataSize, [&setupSuccessful](size_t)
        {
            setupSuccessful = true;
        });

        EXPECT_FALSE(setupSuccessful);
    }

    TEST(TerrainSetupWithOctree, TerrainSetupWithNonEmptyOctree)
    {
        const size_t octreeCompiledDataSize = 8;

        bool setupSuccessful = false;
        // function will be called
        SetupTerrainInfo(
            octreeCompiledDataSize, [&setupSuccessful](size_t)
        {
            setupSuccessful = true;
        });

        EXPECT_TRUE(setupSuccessful);
    }
}
