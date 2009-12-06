/************************************************************************************
This source file is part of the TheoraVideoPlugin ExternalTextureSource PlugIn 
for OGRE3D (Object-oriented Graphics Rendering Engine)
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright © 2008-2009 Kresimir Spes (kreso@cateia.com)

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License (LGPL) as published by the 
Free Software Foundation; either version 2 of the License, or (at your option) 
any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
*************************************************************************************/
#include "TheoraFrameQueue.h"
#include "TheoraVideoFrame.h"

namespace Ogre
{
	TheoraFrameQueue::TheoraFrameQueue(int n,TheoraVideoClip* parent):
		mSize(0),
		mQueue(0)
	{
		mParent=parent;
		setSize(n);
	}
	
	TheoraFrameQueue::~TheoraFrameQueue()
	{
		if (mQueue)
		{
			for (int i=0;i<mSize;i++)
				delete mQueue[i];
			delete mQueue;
		}
	}

	void TheoraFrameQueue::setSize(int n)
	{
		mMutex.lock();
		if (mQueue)
		{
			// todo: copy frames
			//       and delete each frame
			delete mQueue;
		}
		mQueue=new TheoraVideoFrame*[n];
		for (int i=0;i<n;i++)
			mQueue[i]=new TheoraVideoFrame(mParent);

		mSize=n;
		mMutex.unlock();
	}

	int TheoraFrameQueue::getSize()
	{
		return mSize;
	}

	TheoraVideoFrame* TheoraFrameQueue::getFirstAvailableFrame()
	{
		TheoraVideoFrame* frame=0;
		mMutex.lock();
		if (mQueue[0]->mReady) frame=mQueue[0];
		mMutex.unlock();
		return frame;
	}

	void TheoraFrameQueue::clear()
	{
		mMutex.lock();
		for (int i=0;i<mSize;i++)
		{
			mQueue[i]->mInUse=false;
			mQueue[i]->mReady=false;
		}
		mMutex.unlock();
	}

	void TheoraFrameQueue::pop()
	{
		mMutex.lock();
		TheoraVideoFrame* first=mQueue[0];

		for (int i=0;i<mSize-1;i++)
		{
			mQueue[i]=mQueue[i+1];
		}
		mQueue[mSize-1]=first;

		first->mInUse=false;
		first->mReady=false;
		mMutex.unlock();
	}
		
	TheoraVideoFrame* TheoraFrameQueue::requestEmptyFrame()
	{
		TheoraVideoFrame* frame=0;
		mMutex.lock();
		for (int i=0;i<mSize;i++)
		{
			if (!mQueue[i]->mInUse)
			{
				mQueue[i]->mInUse=true;
				mQueue[i]->mReady=false;
				frame=mQueue[i];
				break;
			}
		}
		mMutex.unlock();
		return frame;
	}

	void TheoraFrameQueue::fillBackColour(unsigned int colour)
	{
		mMutex.lock();
		mBackColour=colour;
		for (int i=0;i<mSize;i++) mQueue[i]->fillBackColour(colour);
		mMutex.unlock();
	}

	unsigned int TheoraFrameQueue::getBackColour()
	{
		return mBackColour;
	}

	int TheoraFrameQueue::getUsedCount()
	{
		mMutex.lock();
		int i,n=0;
		for (i=0;i<mSize;i++)
			if (mQueue[i]->mInUse) n++;
		mMutex.unlock();
		return n;
	}
} // end namespace Ogre
