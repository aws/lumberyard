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

#include "CloudGemSpeechRecognition_precompiled.h"

#import "CocoaAudioRecorder.h"
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreAudio/CoreAudioTypes.h>

static const char* sSampleFileAlias = "@user@";
static const char* sSampleFileName = "cocoaaudiorecorder.wav";


@interface CocoaAudioRecorder_Internal : NSObject <AVAudioRecorderDelegate>
{
    AVAudioRecorder* recorder;
    NSURL* soundFileURL;
}

@property (nonatomic) BOOL isRecording;

-(void)initialize;
-(void)startRecording;
-(void)stopRecording;
-(void)shutdown;
-(void)cleanUpFile;
-(NSURL*)getFileName;
@end

@implementation CocoaAudioRecorder_Internal
-(void)initialize
{
    _isRecording = false;
}

-(void)startRecording
{
    if(_isRecording)
    {
        [self stopRecording];
        [self cleanUpFile];
    }
#if defined(AZ_PLATFORM_APPLE_IOS)
    // AVAudioSession is a iOS/AppleTV specific API that helps negotiate audio use between apps.
    // The API doesn't exist on MacOS
    AVAudioSession* session = [AVAudioSession sharedInstance];
    [session setCategory:AVAudioSessionCategoryPlayAndRecord error:nil];
    [session setActive:YES error:nil];
#endif

    NSDictionary* recordSettings =
    [[NSDictionary alloc] initWithObjectsAndKeys:
     [NSNumber numberWithFloat: 16000.0], AVSampleRateKey,
     [NSNumber numberWithInt: kAudioFormatLinearPCM], AVFormatIDKey,
     [NSNumber numberWithInt: 1], AVNumberOfChannelsKey,
     [NSNumber numberWithInt: 16], AVLinearPCMBitDepthKey,
     [NSNumber numberWithBool: NO], AVLinearPCMIsFloatKey,
     [NSNumber numberWithBool: NO], AVLinearPCMIsBigEndianKey,
     [NSNumber numberWithBool: NO], AVLinearPCMIsNonInterleavedKey,
     nil];
    NSError* err = nil;
    recorder = [[AVAudioRecorder alloc] initWithURL:[self getFileName] settings: recordSettings error:&err];
    [recordSettings release];
    
    if(recorder == nil)
    {
        NSLog(@"%@", err);
        return;
    }
    
    [recorder prepareToRecord];
    [recorder record];
    
    _isRecording = true;
}

-(void)stopRecording
{
    [recorder stop];
    _isRecording = false;
}

-(void)shutdown
{
#if defined(AZ_PLATFORM_APPLE_IOS)
    AVAudioSession* session = [AVAudioSession sharedInstance];
    [session setActive:NO error:nil];
#endif

    [self cleanUpFile];
    _isRecording = false;
}

-(void)cleanUpFile
{
    [[NSFileManager defaultManager] removeItemAtURL:[self getFileName] error:nil];
}

-(NSURL*)getFileName
{
    if(soundFileURL.absoluteString.length == 0)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZStd::string cFilePath;
        if(fileIO)
        {
            cFilePath = fileIO->GetAlias(sSampleFileAlias);
        }
        cFilePath += "/";
        cFilePath += sSampleFileName;
        soundFileURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:cFilePath.c_str()] isDirectory:FALSE];
    }
    return soundFileURL;
}


@end

static CocoaAudioRecorder_Internal *s_recorder = nullptr;

void CocoaAudioRecorder::initialize()
{
    [s_recorder release];
    s_recorder = [[CocoaAudioRecorder_Internal alloc] init];
    [s_recorder initialize];
}

void CocoaAudioRecorder::startRecording()
{
    [s_recorder startRecording];
}

void CocoaAudioRecorder::stopRecording()
{
    [s_recorder stopRecording];
}

void CocoaAudioRecorder::shutdown()
{
    [s_recorder shutdown];
    [s_recorder release];
    s_recorder = nullptr;
}

bool CocoaAudioRecorder::isRecording()
{
    return [s_recorder isRecording];
}

AZStd::string CocoaAudioRecorder::getFileName()
{
    AZStd::string cFilePath(sSampleFileAlias);
    cFilePath += "/";
    cFilePath += sSampleFileName;

    return cFilePath;
}

