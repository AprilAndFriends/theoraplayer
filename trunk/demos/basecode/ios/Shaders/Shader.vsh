/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
attribute vec4 position;
attribute vec2 tex;

varying lowp vec4 colorVarying;
varying mediump vec2 texCoord;

uniform vec4 color;
uniform mat4 modelViewProjectionMatrix;

void main()
{
    vec4 diffuseColor = vec4(1.0, 1.0, 1.0, 1.0);

    colorVarying = color * diffuseColor;
    texCoord = tex;

    gl_Position = modelViewProjectionMatrix * position;
}
