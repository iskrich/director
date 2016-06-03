/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "director/resource.h"

#include "common/debug.h"
#include "common/macresman.h"
#include "common/substream.h"
#include "common/util.h"
#include "common/textconsole.h"

namespace Director {

// Base Archive code

Archive::Archive() {
	_stream = 0;
}

Archive::~Archive() {
	close();
}

bool Archive::openFile(const Common::String &fileName) {
	Common::File *file = new Common::File();

	if (!file->open(fileName)) {
		delete file;
		return false;
	}

	if (!openStream(file)) {
		close();
		return false;
	}

	return true;
}

void Archive::close() {
	_types.clear();
	delete _stream; _stream = 0;
}

bool Archive::hasResource(uint32 tag, uint16 id) const {
	if (!_types.contains(tag))
		return false;

	return _types[tag].contains(id);
}

bool Archive::hasResource(uint32 tag, const Common::String &resName) const {
	if (!_types.contains(tag) || resName.empty())
		return false;

	const ResourceMap &resMap = _types[tag];

	for (ResourceMap::const_iterator it = resMap.begin(); it != resMap.end(); it++)
		if (it->_value.name.matchString(resName))
			return true;

	return false;
}

Common::SeekableReadStream *Archive::getResource(uint32 tag, uint16 id) {
	if (!_types.contains(tag))
		error("Archive does not contain '%s' %04x", tag2str(tag), id);

	const ResourceMap &resMap = _types[tag];

	if (!resMap.contains(id))
		error("Archive does not contain '%s' %04x", tag2str(tag), id);

	const Resource &res = resMap[id];

	return new Common::SeekableSubReadStream(_stream, res.offset, res.offset + res.size);
}

uint32 Archive::getOffset(uint32 tag, uint16 id) const {
	if (!_types.contains(tag))
		error("Archive does not contain '%s' %04x", tag2str(tag), id);

	const ResourceMap &resMap = _types[tag];

	if (!resMap.contains(id))
		error("Archive does not contain '%s' %04x", tag2str(tag), id);

	return resMap[id].offset;
}

uint16 Archive::findResourceID(uint32 tag, const Common::String &resName) const {
	if (!_types.contains(tag) || resName.empty())
		return 0xFFFF;

	const ResourceMap &resMap = _types[tag];

	for (ResourceMap::const_iterator it = resMap.begin(); it != resMap.end(); it++)
		if (it->_value.name.matchString(resName))
			return it->_key;

	return 0xFFFF;
}

Common::String Archive::getName(uint32 tag, uint16 id) const {
	if (!_types.contains(tag))
		error("Archive does not contain '%s' %04x", tag2str(tag), id);

	const ResourceMap &resMap = _types[tag];

	if (!resMap.contains(id))
		error("Archive does not contain '%s' %04x", tag2str(tag), id);

	return resMap[id].name;
}

Common::Array<uint32> Archive::getResourceTypeList() const {
	Common::Array<uint32> typeList;

	for (TypeMap::const_iterator it = _types.begin(); it != _types.end(); it++)
		typeList.push_back(it->_key);

	return typeList;
}

Common::Array<uint16> Archive::getResourceIDList(uint32 type) const {
	Common::Array<uint16> idList;

	if (!_types.contains(type))
		return idList;

	const ResourceMap &resMap = _types[type];

	for (ResourceMap::const_iterator it = resMap.begin(); it != resMap.end(); it++)
		idList.push_back(it->_key);

	return idList;
}

uint32 Archive::convertTagToUppercase(uint32 tag) {
	uint32 newTag = toupper(tag >> 24) << 24;
	newTag |= toupper((tag >> 16) & 0xFF) << 16;
	newTag |= toupper((tag >> 8) & 0xFF) << 8;
	return newTag | toupper(tag & 0xFF);
}

// Mac Archive code

MacArchive::MacArchive() : Archive(), _resFork(0) {
}

MacArchive::~MacArchive() {
	delete _resFork;
}

void MacArchive::close() {
	Archive::close();
	delete _resFork;
	_resFork = 0;
}

bool MacArchive::openFile(const Common::String &fileName) {
	close();

	_resFork = new Common::MacResManager();

	if (!_resFork->open(fileName) || !_resFork->hasResFork()) {
		close();
		return false;
	}

	Common::MacResTagArray tagArray = _resFork->getResTagArray();

	for (uint32 i = 0; i < tagArray.size(); i++) {
		ResourceMap &resMap = _types[tagArray[i]];
		Common::MacResIDArray idArray = _resFork->getResIDArray(tagArray[i]);

		for (uint32 j = 0; j < idArray.size(); j++) {
			Resource &res = resMap[idArray[j]];
			res.offset = res.size = 0; // unused
			res.name = _resFork->getResName(tagArray[i], idArray[j]);
		}
	}


	return true;
}

bool MacArchive::openStream(Common::SeekableReadStream *stream, uint32 startOffset) {
	// TODO: Add support for this (v4 Windows games)
	return false;
}

Common::SeekableReadStream *MacArchive::getResource(uint32 tag, uint16 id) {
	assert(_resFork);
	return _resFork->getResource(tag, id);
}

// RIFF Archive code

bool RIFFArchive::openStream(Common::SeekableReadStream *stream, uint32 startOffset) {
	close();

	stream->seek(startOffset);

	if (convertTagToUppercase(stream->readUint32BE()) != MKTAG('R', 'I', 'F', 'F'))
		return false;

	stream->readUint32LE(); // size

	if (convertTagToUppercase(stream->readUint32BE()) != MKTAG('R', 'M', 'M', 'P'))
		return false;

	if (convertTagToUppercase(stream->readUint32BE()) != MKTAG('C', 'F', 'T', 'C'))
		return false;

	uint32 cftcSize = stream->readUint32LE();
	uint32 startPos = stream->pos();
	stream->readUint32LE(); // unknown (always 0?)
	while ((uint32)stream->pos() < startPos + cftcSize) {
		uint32 tag = convertTagToUppercase(stream->readUint32BE());

		uint32 size = stream->readUint32LE();
		uint32 id = stream->readUint32LE();
		uint32 offset = stream->readUint32LE();

		if (tag == 0)
			break;
		uint16 startResPos = stream->pos();
		stream->seek(offset + 12);

		Common::String name = "";
		byte nameSize = stream->readByte();
		if (nameSize) {
			for (uint8 i = 0; i < nameSize; i++) {
				name += stream->readByte();
			}
		}

		stream->seek(startResPos);

		debug(0, "Found RIFF resource '%s' %d: %d @ 0x%08x", tag2str(tag), id, size, offset);

		ResourceMap &resMap = _types[tag];
		Resource &res = resMap[id];
		res.offset = offset;
		res.size = size;
		res.name = name;
	}

	_stream = stream;
	return true;
}

Common::SeekableReadStream *RIFFArchive::getResource(uint32 tag, uint16 id) {
	if (!_types.contains(tag))
		error("Archive does not contain '%s' %04x", tag2str(tag), id);

	const ResourceMap &resMap = _types[tag];

	if (!resMap.contains(id))
		error("Archive does not contain '%s' %04x", tag2str(tag), id);

	const Resource &res = resMap[id];

	// Adjust to skip the resource header
	uint32 offset = res.offset + 12;
	uint32 size = res.size - 4;
	// Skip the Pascal string
	_stream->seek(offset);
	byte stringSize = _stream->readByte(); // 1 for this byte

	offset += stringSize + 1;
	size -= stringSize + 1;

	// Align to nearest word boundary
	if (offset & 1) {
		offset++;
		size--;
	}

	return new Common::SeekableSubReadStream(_stream, offset, offset + size);
}

// RIFX Archive code

bool RIFXArchive::openStream(Common::SeekableReadStream *stream, uint32 startOffset) {
	close();

	stream->seek(startOffset);

	uint32 headerTag = stream->readUint32BE();

	if (headerTag == MKTAG('R', 'I', 'F', 'X'))
		_isBigEndian = true;
	else if (SWAP_BYTES_32(headerTag) == MKTAG('R', 'I', 'F', 'X'))
		_isBigEndian = false;
	else
		return false;

	Common::SeekableSubReadStreamEndian subStream(stream, startOffset + 4, stream->size(), _isBigEndian, DisposeAfterUse::NO);

	subStream.readUint32(); // size

	uint32 rifxType = subStream.readUint32();
	if (rifxType != MKTAG('M', 'V', '9', '3') && rifxType != MKTAG('A', 'P', 'P', 'L'))
		return false;

	if (subStream.readUint32() != MKTAG('i', 'm', 'a', 'p'))
		return false;

	subStream.readUint32(); // imap length
	subStream.readUint32(); // unknown
	uint32 mmapOffset = subStream.readUint32() - startOffset - 4;

	subStream.seek(mmapOffset);

	if (subStream.readUint32() != MKTAG('m', 'm', 'a', 'p'))
		return false;

	subStream.readUint32(); // mmap length
	subStream.readUint16(); // unknown
	subStream.readUint16(); // unknown
	subStream.readUint32(); // resCount + empty entries
	uint32 resCount = subStream.readUint32();
	subStream.skip(8); // all 0xFF
	subStream.readUint32(); // unknown

	Common::Array<Resource> resources;

	// Need to look for these two resources
	const Resource *keyRes = 0;
	const Resource *casRes = 0;

	for (uint32 i = 0; i < resCount; i++) {
		uint32 tag = subStream.readUint32();
		uint32 size = subStream.readUint32();
		uint32 offset = subStream.readUint32();
		/*uint16 flags = */ subStream.readUint16();
		/*uint16 unk1 = */ subStream.readUint16();
		/*uint32 unk2 = */ subStream.readUint32();

		debug(0, "Found RIFX resource index %d: '%s', %d @ 0x%08x", i, tag2str(tag), size, offset);

		Resource res;
		res.offset = offset;
		res.size = size;
		resources.push_back(res);

		// APPL is a special case; it has an embedded "normal" archive
		if (rifxType == MKTAG('A', 'P', 'P', 'L') && tag == MKTAG('F', 'i', 'l', 'e'))
			return openStream(stream, offset);

		// Looking for two types here
		if (tag == MKTAG('K', 'E', 'Y', '*'))
			keyRes = &resources[resources.size() - 1];
		else if (tag == MKTAG('C', 'A', 'S', '*'))
			casRes = &resources[resources.size() - 1];
	}

	// We need to have found the 'File' resource already
	if (rifxType == MKTAG('A', 'P', 'P', 'L')) {
		warning("No 'File' resource present in APPL archive");
		return false;
	}

	// A KEY* must be present
	if (!keyRes) {
		warning("No 'KEY*' resource present");
		return false;
	}

	// Parse the CAS*, if present
	Common::Array<uint32> casEntries;
	if (casRes) {
		Common::SeekableSubReadStreamEndian casStream(stream, casRes->offset + 8, casRes->offset + 8 + casRes->size, _isBigEndian, DisposeAfterUse::NO);
		casEntries.resize(casRes->size / 4);
		for (uint32 i = 0; i < casEntries.size(); i++)
			casEntries[i] = casStream.readUint32();
	}

	// Parse the KEY*
	Common::SeekableSubReadStreamEndian keyStream(stream, keyRes->offset + 8, keyRes->offset + 8 + keyRes->size, _isBigEndian, DisposeAfterUse::NO);
	/*uint16 unk1 = */ keyStream.readUint16();
	/*uint16 unk2 = */ keyStream.readUint16();
	/*uint32 unk3 = */ keyStream.readUint32();
	uint32 keyCount = keyStream.readUint32();

	for (uint32 i = 0; i < keyCount; i++) {
		uint32 index = keyStream.readUint32();
		uint32 id = keyStream.readUint32();
		uint32 resTag = keyStream.readUint32();

		// Handle CAS*/CASt nonsense
		if (resTag == MKTAG('C', 'A', 'S', 't')) {
			for (uint32 j = 0; j < casEntries.size(); j++) {
				if (casEntries[j] == index) {
					id += j + 1;
					break;
				}
			}
		}

		const Resource &res = resources[index];
		debug(0, "Found RIFX resource: '%s' 0x%04x, %d @ 0x%08x", tag2str(resTag), id, res.size, res.offset);
		_types[resTag][id] = res;
	}

	_stream = stream;
	return true;
}

} // End of namespace Director
