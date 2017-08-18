#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# Original file Copyright Crytek GMBH or its affiliates, used under license.
#

#!/usr/bin/env python
# encoding: utf-8
# Chris Corliss

from waflib.TaskGen import feature, after_method,before_method, extension
from waflib.Task import Task,RUN_ME
from waflib.Tools import c_preproc, cxx
from waflib import TaskGen
from waflib import Task
from waflib import Logs
import os

@feature('wwise')
@before_method('apply_incpaths')
def apply_wwise_settings(self):
    if self.env['PLATFORM'] == 'project_generator':
        return

    platform = self.env['PLATFORM']
    if platform in ('android_armv7', 'android_armv7_gcc' ):
        platform = 'android-armeabi-v7a'
    if platform in ('win_x64_vs2015'):
        platform = 'x64/vs14'
    else:
        platform = 'x64/vs12'

    configuration =  self.env['CONFIGURATION'].title()

    if hasattr(self.env['CONFIG_OVERWRITES'], self.target):
        configuration = self.env['CONFIG_OVERWRITES'][self.target]

    includes = [ self.bld.CreateRootRelativePath('Code/SDKs/Wwise/include') ]
    libpath = [ self.bld.CreateRootRelativePath('Code/SDKs/Wwise/lib/' + platform + '/' + configuration) ]

    libs = [ 'AkAudioInputSource',
            'AkCompressorFX',
            'AkConvolutionReverbFX',
            'AkDelayFX',
            'AkExpanderFX',
            'AkFlangerFX',
            'AkGainFX',
            'AkGuitarDistortionFX',
            'AkHarmonizerFX',
            'AkMatrixReverbFX',
            'AkMemoryMgr',
            'AkMeterFX',
            'AkMusicEngine',
            'AkParametricEQFX',
            'AkPeakLimiterFX',
            'AkPitchShifterFX',
            'AkRoomVerbFX',
            'AkSilenceSource',
            'AkSineSource',
            'AkSoundEngine',
            'AkSoundSeedImpactFX',
            'AkSoundSeedWind',
            'AkSoundSeedWoosh',
            'AkStereoDelayFX',
            'AkStreamMgr',
            'AkSynthOne',
            'AkTimeStretchFX',
            'AkToneSource',
            'AkTremoloFX',
            'AkVorbisDecoder',
            'AstoundsoundExpanderFX',
            'AstoundsoundFolddownFX',
            'AstoundsoundRTIFX',
            'AstoundsoundShared',
            'McDSPFutzBoxFX',
            'McDSPLimiterFX' ]

    if 'ios' in platform or 'appletv' in platform:
        libs += [ 'AkAACDecoder' ]

    if configuration != 'Release':
        libs += [ 'CommunicationCentral' ]

    if not hasattr(self, 'includes'):
        self.includes = []
    self.includes = includes + self.includes

    self.env['LIB'] += libs
    self.env['LIBPATH'] += libpath

