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
#include "TheoraTimer.h"

namespace Ogre
{

	TheoraTimer::TheoraTimer()
	{
		mTime=0;
		mPaused=0;
	}

	void TheoraTimer::update(float time_increase)
	{
		mTime+=time_increase;
	}
	
	float TheoraTimer::getTime()
	{
		return mTime;
	}
		
	void TheoraTimer::pause()
	{
		mPaused=true;
	}

	void TheoraTimer::play()
	{
		mPaused=false;
	}


	bool TheoraTimer::isPaused()
	{
		return mPaused;
	}

	void TheoraTimer::stop()
	{
	
	}
	
	void TheoraTimer::seek(float time)
	{
		mTime=time;
	}

} // end namespace Ogre


