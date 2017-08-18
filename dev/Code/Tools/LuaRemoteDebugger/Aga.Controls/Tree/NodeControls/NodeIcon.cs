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
using System.Windows.Forms;
using Aga.Controls.Properties;
using System.ComponentModel;

namespace Aga.Controls.Tree.NodeControls
{
	public class NodeIcon : BindableControl
	{
		public NodeIcon()
		{
			LeftMargin = 1;
		}

		public override Size MeasureSize(TreeNodeAdv node, DrawContext context)
		{
			Image image = GetIcon(node);
			if (image != null)
				return image.Size;
			else
				return Size.Empty;
		}


		public override void Draw(TreeNodeAdv node, DrawContext context)
		{
			Image image = GetIcon(node);
			if (image != null)
			{
				Rectangle r = GetBounds(node, context);
				if ( image.Width > 0 && image.Height > 0 )
				{
					switch (_scaleMode)
					{
						case ImageScaleMode.Fit:
							context.Graphics.DrawImage(image, r);
							break;
						case ImageScaleMode.ScaleDown:
							{
								float factor = Math.Min((float)r.Width / (float)image.Width, (float)r.Height / (float)image.Height);
								if (factor < 1)
									context.Graphics.DrawImage(image, r.X, r.Y, image.Width * factor, image.Height * factor);
								else
									context.Graphics.DrawImage(image, r.X, r.Y, image.Width, image.Height);
							} break;
						case ImageScaleMode.ScaleUp:
							{
								float factor = Math.Max((float)r.Width / (float)image.Width, (float)r.Height / (float)image.Height);
								if (factor > 1)
									context.Graphics.DrawImage(image, r.X, r.Y, image.Width * factor, image.Height * factor);
								else
									context.Graphics.DrawImage(image, r.X, r.Y, image.Width, image.Height);
							} break;
						case ImageScaleMode.AlwaysScale:
							{
								float fx = (float)r.Width / (float)image.Width;
								float fy = (float)r.Height / (float)image.Height;
								if (Math.Min(fx, fy) < 1)
								{ //scale down
									float factor = Math.Min(fx, fy);
									context.Graphics.DrawImage(image, r.X, r.Y, image.Width * factor, image.Height * factor);
								}
								else if (Math.Max(fx, fy) > 1)
								{
									float factor = Math.Max(fx, fy);
									context.Graphics.DrawImage(image, r.X, r.Y, image.Width * factor, image.Height * factor);
								}
								else
									context.Graphics.DrawImage(image, r.X, r.Y, image.Width, image.Height);
							} break;
						case ImageScaleMode.Clip:
						default: 
							context.Graphics.DrawImage(image, r.X, r.Y, image.Width, image.Height);
							break;
					}
				}

			}
		}

		protected virtual Image GetIcon(TreeNodeAdv node)
		{
			return GetValue(node) as Image;
		}

        private ImageScaleMode _scaleMode = ImageScaleMode.Clip;
        [DefaultValue("Clip"), Category("Appearance")]
        public ImageScaleMode ScaleMode
        {
            get { return _scaleMode; }
            set { _scaleMode = value; }
        }


	}
}
