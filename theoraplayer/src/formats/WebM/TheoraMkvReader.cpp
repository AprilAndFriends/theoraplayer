// Copyright (c) 2010 The WebM project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.

#include "TheoraMkvReader.h"

#include <cassert>

TheoraMkvReader::TheoraMkvReader(TheoraDataSource* dataSource) : m_dataSource(dataSource)
{
	m_length = (int)dataSource->getSize();
}

TheoraMkvReader::~TheoraMkvReader() {	
	m_dataSource = NULL;
}

int TheoraMkvReader::Open(const char* fileName) {
	return 0;
}

void TheoraMkvReader::Close() {
	
}

bool TheoraMkvReader::GetFileSize() 
{
	if (m_dataSource == NULL)
		return false;

	m_length = (int)m_dataSource->getSize();

	assert(m_length >= 0);

	if (m_length < 0)
		return false;

	return true;
}

int TheoraMkvReader::Length(long long* total, long long* available) 
{
	if (m_dataSource == NULL)
		return 1;

	if (total)
		*total = m_length;

	if (available)
		*available = m_length;

	return 0;
}

int TheoraMkvReader::Read(long long offset, long len, unsigned char* buffer) {
	if (m_dataSource == NULL)
		return -1;

	if (offset < 0)
		return -1;

	if (len < 0)
		return -1;

	if (len == 0)
		return 0;

	if (offset >= m_length)
		return -1;

	m_dataSource->seek(offset);

	m_dataSource->read(buffer, len);

	return 0;  // success
}


