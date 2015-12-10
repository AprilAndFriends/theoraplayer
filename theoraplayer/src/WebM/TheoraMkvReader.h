// Copyright (c) 2010 The WebM project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.

#ifndef THEORAMKVREADER_HPP
#define THEORAMKVREADER_HPP

#include <mkvparser.hpp>
#include "TheoraDataSource.h"
#include <cstdio>

class TheoraMkvReader : public mkvparser::IMkvReader {
public:
	explicit TheoraMkvReader(TheoraDataSource* dataSource);
	virtual ~TheoraMkvReader();

	int Open(const char*);
	void Close();

	virtual int Read(long long position, long length, unsigned char* buffer);
	virtual int Length(long long* total, long long* available);

private:
	TheoraMkvReader(const TheoraMkvReader&);
	TheoraMkvReader& operator=(const TheoraMkvReader&);	

	int m_length;
	TheoraDataSource* m_dataSource;

	bool GetFileSize();
};



#endif  // MKVREADER_HPP
