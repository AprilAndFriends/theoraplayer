/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifdef __AVFOUNDATION
#define AVFOUNDATION_CLASSES_DEFINED
#import <AVFoundation/AVFoundation.h>
#include "TheoraDataSource.h"
#include "TheoraException.h"
#include "TheoraFrameQueue.h"
#include "TheoraVideoFrame.h"
#include "TheoraVideoClip_AVFoundation.h"

//AVAssetReader* mReader;
//AVAssetReaderTrackOutput* mOutput;

TheoraVideoClip_AVFoundation::TheoraVideoClip_AVFoundation(TheoraDataSource* data_source,
											   TheoraOutputMode output_mode,
											   int nPrecachedFrames,
											   bool usePower2Stride): TheoraVideoClip(data_source, output_mode, nPrecachedFrames, usePower2Stride)
{
	mReader = NULL;
	mOutput = NULL;
}

TheoraVideoClip_AVFoundation::~TheoraVideoClip_AVFoundation()
{
	unload();
}

void TheoraVideoClip_AVFoundation::unload()
{
	if (mOutput != NULL)
	{
		[mOutput release];
		mOutput = NULL;
	}
	if (mReader != NULL)
	{
		[mReader release];
		mReader = NULL;
	}
}

bool TheoraVideoClip_AVFoundation::_readData()
{
	return 1;
}

void TheoraVideoClip_AVFoundation::decodeNextFrame()
{
	if (mReader == NULL || mEndOfFile) return;
	TheoraVideoFrame* frame = mFrameQueue->requestEmptyFrame();
	if (!frame) return;

	CMSampleBufferRef sampleBuffer = NULL;
	NSAutoreleasePool* pool = NULL;
	if ([mReader status] == AVAssetReaderStatusReading)
	{
		pool = [[NSAutoreleasePool alloc] init];
		if ((sampleBuffer = [mOutput copyNextSampleBuffer]))
		{
			frame->mTimeToDisplay = mFrameNumber / mFPS;
			frame->mIteration = mIteration;
			frame->_setFrameNumber(mFrameNumber);
			mFrameNumber++;
			
			CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
			CVPixelBufferLockBaseAddress(imageBuffer, 0);
			void *baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);
			mStride = CVPixelBufferGetBytesPerRow(imageBuffer);
			size_t width = CVPixelBufferGetWidth(imageBuffer);
			size_t height = CVPixelBufferGetHeight(imageBuffer);
			frame->decodeBGRX(baseAddress);
			
			CVPixelBufferUnlockBaseAddress(imageBuffer,0);
			CFRelease(sampleBuffer);
		}
	}
	if (pool) [pool release];

	if (sampleBuffer == NULL)
	{
		frame->mInUse = 0;
		[mOutput release];
		[mReader release];
		mReader = NULL;
		mOutput = NULL;
		_restart();
	}
}

void TheoraVideoClip_AVFoundation::_restart()
{
	unload();
	load(mStream);
	mRestarted = 1;
}

void TheoraVideoClip_AVFoundation::load(TheoraDataSource* source)
{
	mStream = source;
	mFrameNumber = 0;
	TheoraFileDataSource* fileDataSource = dynamic_cast<TheoraFileDataSource*>(source);

	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	NSString* path = [NSString stringWithUTF8String:fileDataSource->getFilename().c_str()];
	NSError* err;
	NSURL *url = [NSURL fileURLWithPath:path];
	AVAsset* asset = [[AVURLAsset alloc] initWithURL:url options:nil];
	mReader = [[AVAssetReader alloc] initWithAsset:asset error:&err];
	AVAssetTrack *videoTrack = [[asset tracksWithMediaType:AVMediaTypeVideo] objectAtIndex:0];
	NSDictionary *videoOptions = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:kCVPixelFormatType_32BGRA], kCVPixelBufferPixelFormatTypeKey, nil];
	
	mOutput = [[AVAssetReaderTrackOutput alloc] initWithTrack:videoTrack outputSettings:videoOptions];
	[mReader addOutput:mOutput];
	if ([mOutput respondsToSelector:@selector(setAlwaysCopiesSampleData:)]) // Not supported on < iOS5
	{
		mOutput.alwaysCopiesSampleData = NO;
	}

	mFPS = videoTrack.nominalFrameRate;
	mWidth = mStride = videoTrack.naturalSize.width;
	mHeight = videoTrack.naturalSize.height;
	mFrameDuration = 1.0f / mFPS;
	mDuration = (float) CMTimeGetSeconds(asset.duration);
	if (mFrameQueue == NULL) mFrameQueue = new TheoraFrameQueue(mNumPrecachedFrames, this);

	[mReader startReading];
	[pool release];
}
 
void TheoraVideoClip_AVFoundation::decodedAudioCheck()
{

}

float TheoraVideoClip_AVFoundation::decodeAudio()
{
	return -1;
}

void TheoraVideoClip_AVFoundation::doSeek()
{
	
}
#endif
