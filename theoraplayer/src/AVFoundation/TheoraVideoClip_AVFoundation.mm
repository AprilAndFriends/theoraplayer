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
#include "TheoraTimer.h"
#include "TheoraUtil.h"
#include "TheoraVideoFrame_AVFoundation.h"
#include "TheoraVideoManager.h"
#include "TheoraVideoClip_AVFoundation.h"

//AVAssetReader* mReader;
//AVAssetReaderTrackOutput* mOutput;

TheoraVideoClip_AVFoundation::TheoraVideoClip_AVFoundation(TheoraDataSource* data_source,
											   TheoraOutputMode output_mode,
											   int nPrecachedFrames,
											   bool usePower2Stride): TheoraVideoClip(data_source, output_mode, nPrecachedFrames, usePower2Stride)
{
	mLoaded = 0;
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

bool TheoraVideoClip_AVFoundation::decodeNextFrame()
{
	if (mReader == NULL || mEndOfFile) return 0;
	TheoraVideoFrame* frame = mFrameQueue->requestEmptyFrame();
	if (!frame) return 0;

	CMSampleBufferRef sampleBuffer = NULL;
	NSAutoreleasePool* pool = NULL;
	if ([mReader status] == AVAssetReaderStatusReading)
	{
		pool = [[NSAutoreleasePool alloc] init];
		while ((sampleBuffer = [mOutput copyNextSampleBuffer]))
		{
			frame->mTimeToDisplay = mFrameNumber / mFPS;
			frame->mIteration = mIteration;
			frame->_setFrameNumber(mFrameNumber);
			mFrameNumber++;
			if (frame->mTimeToDisplay < mTimer->getTime() && !mRestarted && mFrameNumber % 16 != 0)
			{
				// %16 operation is here to prevent a playback halt during video playback if the decoder can't keep up with demand.
#ifdef _DEBUG
				th_writelog(mName + ": pre-dropped frame " + str(mFrameNumber - 1));
#endif
				mNumDisplayedFrames++;
				mNumDroppedFrames++;
				CFRelease(sampleBuffer);
				sampleBuffer = NULL;
				continue; // drop frame
			}
			
			CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
			CVPixelBufferLockBaseAddress(imageBuffer, 0);
			void *baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);
			mStride = CVPixelBufferGetBytesPerRow(imageBuffer);
			size_t width = CVPixelBufferGetWidth(imageBuffer);
			size_t height = CVPixelBufferGetHeight(imageBuffer);
			frame->decode(baseAddress, TH_BGRX);
			
			CVPixelBufferUnlockBaseAddress(imageBuffer,0);
			CFRelease(sampleBuffer);
			break;
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
		if (mAutoRestart)
			_restart();
		else
			mEndOfFile = 1;
		return 0;
	}
	return 1;
}

void TheoraVideoClip_AVFoundation::_restart()
{
	mEndOfFile = 0;
	unload();
	load(mStream);
	mRestarted = 1;
}

void TheoraVideoClip_AVFoundation::load(TheoraDataSource* source)
{
	mStream = source;
	mFrameNumber = 0;
	mEndOfFile = 0;
	TheoraFileDataSource* fileDataSource = dynamic_cast<TheoraFileDataSource*>(source);
	std::string filename;
	if (fileDataSource != NULL) filename = fileDataSource->getFilename();
	else
	{
		TheoraMemoryFileDataSource* memoryDataSource = dynamic_cast<TheoraMemoryFileDataSource*>(source);
		if (memoryDataSource != NULL) filename = memoryDataSource->getFilename();
		else throw TheoraGenericException("Unable to load MP4 file");
	}
	
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	NSString* path = [NSString stringWithUTF8String:filename.c_str()];
	NSError* err;
	NSURL *url = [NSURL fileURLWithPath:path];
	AVAsset* asset = [[AVURLAsset alloc] initWithURL:url options:nil];
	mReader = [[AVAssetReader alloc] initWithAsset:asset error:&err];
	AVAssetTrack *videoTrack = [[asset tracksWithMediaType:AVMediaTypeVideo] objectAtIndex:0];
	bool yuv_output = (mOutputMode == TH_YUV);
	NSDictionary *videoOptions = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:(yuv_output) ? kCVPixelFormatType_420YpCbCr8Planar : kCVPixelFormatType_32BGRA], kCVPixelBufferPixelFormatTypeKey, nil];
	
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
	if (mFrameQueue == NULL)
	{
		mFrameQueue = new TheoraFrameQueue_AVFoundation(this);
		mFrameQueue->setSize(mNumPrecachedFrames);
	}

	if (mSeekFrame != -1)
	{
		mFrameNumber = mSeekFrame;
		[mReader setTimeRange: CMTimeRangeMake(CMTimeMakeWithSeconds(mSeekFrame / mFPS, 1), kCMTimePositiveInfinity)];
	}
#ifdef _DEBUG
	else if (!mLoaded)
	{
		th_writelog("-----\nwidth: " + str(mWidth) + ", height: " + str(mHeight) + ", fps: " + str((int) getFPS()));
		th_writelog("duration: " + strf(mDuration) + " seconds\n-----");
	}
#endif
	[mReader startReading];
	[pool release];
	mLoaded = 1;
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
#if _DEBUG
	th_writelog(mName + " [seek]: seeking to frame " + str(mSeekFrame));
#endif
	int frame;
	float time = mSeekFrame / getFPS();
	mTimer->seek(time);
	bool paused = mTimer->isPaused();
	if (!paused) mTimer->pause(); // pause until seeking is done
	
	mEndOfFile = 0;
	mRestarted = 0;
	
	mFrameQueue->clear();
	unload();
	load(mStream);

	if (!paused) mTimer->play();
	mSeekFrame = -1;
}
#endif
