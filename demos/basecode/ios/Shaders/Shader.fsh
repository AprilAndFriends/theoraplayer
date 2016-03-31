/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
varying lowp vec4 colorVarying;
uniform sampler2D diffuse_map;
varying mediump vec2 texCoord;

void main()
{
	if (colorVarying.a < 0.5) // hack to allow texture/notexture switching
	{
	    gl_FragColor = colorVarying;
	}
	else
	{
		gl_FragColor = texture2D(diffuse_map, texCoord);
	}
}
