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

using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Statoscope
{
	class ImageProcessor
	{
    public static MemoryStream CreateImageStreamFromScreenshotBytes(byte[] data)
    {
      int scaleFactor = (data[2] == 0) ? 1 : data[2];
      int width = data[0] * scaleFactor;
      int height = data[1] * scaleFactor;

      Bitmap bmp = new Bitmap(width, height, System.Drawing.Imaging.PixelFormat.Format24bppRgb);

      System.Drawing.Rectangle rect = new System.Drawing.Rectangle(0, 0, width, height);
      System.Drawing.Imaging.BitmapData bmpData = bmp.LockBits(rect, System.Drawing.Imaging.ImageLockMode.WriteOnly, bmp.PixelFormat);
      System.Runtime.InteropServices.Marshal.Copy(data, 3, bmpData.Scan0, bmpData.Stride * height);
      bmp.UnlockBits(bmpData);

			// jpeg compress the image to save memory
			MemoryStream memStrm = new MemoryStream();
			bmp.Save(memStrm, JpegEncoder, EncoderParams);

      return memStrm;
    }

		static ImageCodecInfo s_jpegEncoder = null;
		static ImageCodecInfo JpegEncoder
		{
			get
			{
				if (s_jpegEncoder == null)
				{
					ImageCodecInfo[] codecs = ImageCodecInfo.GetImageDecoders();

					foreach (ImageCodecInfo codec in codecs)
					{
						if (codec.FormatID == ImageFormat.Jpeg.Guid)
						{
							s_jpegEncoder = codec;
							break;
						}
					}
				}

				return s_jpegEncoder;
			}
		}

		static EncoderParameters s_encoderParams = null;
		static EncoderParameters EncoderParams
		{
			get
			{
				if (s_encoderParams == null)
				{
					s_encoderParams = new EncoderParameters(1);
					EncoderParameter encoderParam = new EncoderParameter(System.Drawing.Imaging.Encoder.Quality, 75L);
					s_encoderParams.Param[0] = encoderParam;
				}

				return s_encoderParams;
			}
		}
	}
}
