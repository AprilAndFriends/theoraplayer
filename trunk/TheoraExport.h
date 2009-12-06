/*
-----------------------------------------------------------------------------
This source file is part of the ffmpegVideoSystem ExternalTextureSource PlugIn 
for OGRE (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

*****************************************************************************
				This PlugIn uses the following resources:

Ogre - see above
Ogg / Vorbis / Theora www.xiph.org
C++ Portable Types Library (PTypes - http://www.melikyan.com/ptypes/ )

*****************************************************************************

Copyright � 2000-2004 The OGRE Team
Also see acknowledgements in Readme.html

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
-----------------------------------------------------------------------------
/***************************************************************************
theoraExport.h  -  
	Dll export/import macros

-------------------
date                 : Jan 1 2004
email                : pjcast@yahoo.com
***************************************************************************/
#ifndef _theoraVideoExport_H
#define _theoraVideoExport_H

#ifndef OGRE_MAC_FRAMEWORK
  #include "OgrePrerequisites.h"
#else
  #include <Ogre/OgrePrerequisites.h>
#endif
//-----------------------------------------------------------------------
// Windows Settings
//-----------------------------------------------------------------------

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 
#   ifdef THEORAVIDEO_PLUGIN_EXPORTS 
#       define _OgreTheoraExport __declspec(dllexport) 
#   else 
#       define _OgreTheoraExport __declspec(dllimport) 
#   endif 
#else 
#   define _OgreTheoraExport 
#endif 

#endif

