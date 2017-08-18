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
using System.Collections.Generic;
using System.Text;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Drawing.Imaging;

namespace Aga.Controls
{
	public static class BitmapHelper
	{
		[StructLayout(LayoutKind.Sequential)]
		private struct PixelData
		{
			public byte B;
			public byte G;
			public byte R;
			public byte A;
		}

		public static void SetAlphaChanelValue(Bitmap image, byte value)
		{
			if (image == null)
				throw new ArgumentNullException("image");
			if (image.PixelFormat != PixelFormat.Format32bppArgb)
				throw new ArgumentException("Wrong PixelFormat");

			BitmapData bitmapData = image.LockBits(new Rectangle(0, 0, image.Width, image.Height),
									 ImageLockMode.ReadWrite, PixelFormat.Format32bppArgb);
			unsafe
			{
				PixelData* pPixel = (PixelData*)bitmapData.Scan0;
				for (int i = 0; i < bitmapData.Height; i++)
				{
					for (int j = 0; j < bitmapData.Width; j++)
					{
						pPixel->A = value;
						pPixel++;
					}
					pPixel += bitmapData.Stride - (bitmapData.Width * 4);
				}
			}
			image.UnlockBits(bitmapData);
		}
	}
}
