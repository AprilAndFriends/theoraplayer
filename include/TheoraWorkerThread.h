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
#ifndef _TheoraWorkerThread_h
#define _TheoraWorkerThread_h

#include <ptypes/pasync.h>

namespace Ogre
{
	/**
		
	*/

	class TheoraVideoClip;

	class TheoraWorkerThread : public pt::thread
	{
		TheoraVideoClip* mClip;

		volatile bool mThreadRunning;

	public:
		TheoraWorkerThread();
		~TheoraWorkerThread();

	    //! Main Thread Body - do not call directly!
		void execute();
	};
}
#endif
