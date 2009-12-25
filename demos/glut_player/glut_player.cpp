/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2009 Kresimir Spes (kreso@cateia.com)

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
#include "../demo_basecode.h"
#include "TheoraPlayer.h"

unsigned int tex_id;
TheoraVideoManager* mgr;
TheoraVideoClip* clip;
std::string window_name="glut_player";
bool started=1;

void draw()
{
	glBindTexture(GL_TEXTURE_2D,tex_id);

	TheoraVideoFrame* f=clip->getNextFrame();
	if (f)
	{
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,f->getWidth(),f->getHeight(),GL_RGB,GL_UNSIGNED_BYTE,f->getBuffer());
		clip->popFrame();
	}

	
	float w=clip->getWidth(),h=clip->getHeight();
	float tw=nextPow2(w),th=nextPow2(h);
	
	glEnable(GL_TEXTURE_2D);
	drawTexturedQuad(0,0,800,600,w/tw,h/th);

	glDisable(GL_TEXTURE_2D);
	drawColoredQuad(0,570,800,30,0,0,0,1);
	drawWiredQuad(0,570,800,30,1,1,1,1);

	float x=clip->getTimePosition()/clip->getDuration();
	drawColoredQuad(3,573,794*x,24,1,1,1,1);
}

void update(float time_increase)
{
	if (started)
	{
		// let's wait until the system caches up a few frames on startup
		if (clip->getNumReadyFrames() < clip->getNumPrecachedFrames()*0.5f)
			return;
		started=0;
	}
	mgr->update(time_increase);
}

void setDebugTitle(char* out)
{
	int nDropped=clip->getNumDroppedFrames(),nDisplayed=clip->getNumDisplayedFrames();
	float percent=100*((float) nDropped/nDisplayed);
	sprintf(out," (%dx%d) %d precached, %d displayed, %d dropped (%.1f %%)",clip->getWidth(),
		                                                                    clip->getHeight(),
		                                                                    clip->getNumPrecachedFrames(),
		                                                                    nDisplayed,
		                                                                    nDropped,
		                                                                    percent);
}

void init()
{
	mgr=new TheoraVideoManager();
	clip=mgr->createVideoClip("../media/short.ogg",TH_YUV);
	clip->setAutoRestart(1);

	tex_id=createTexture(nextPow2(clip->getWidth()),nextPow2(clip->getHeight()));
}

void destroy()
{
	delete mgr;
}
