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

#ifndef DIRECTOR_RESOURCE_H
#define DIRECTOR_RESOURCE_H

#include "common/scummsys.h"
#include "common/endian.h"
#include "common/func.h"
#include "common/hashmap.h"
#include "common/file.h"
#include "common/str.h"

namespace Common {
class MacResManager;
}

namespace Director {

// Completely ripped off of Mohawk's Archive code

class Archive {
public:
	Archive();
	virtual ~Archive();

	virtual bool openFile(const Common::String &fileName);
	virtual bool openStream(Common::SeekableReadStream *stream, uint32 offset = 0) = 0;
	virtual void close();

	bool isOpen() const { return _stream != 0; }

	bool hasResource(uint32 tag, uint16 id) const;
	bool hasResource(uint32 tag, const Common::String &resName) const;
	virtual Common::SeekableReadStream *getResource(uint32 tag, uint16 id);
	uint32 getOffset(uint32 tag, uint16 id) const;
	uint16 findResourceID(uint32 tag, const Common::String &resName) const;
	Common::String getName(uint32 tag, uint16 id) const;

	Common::Array<uint32> getResourceTypeList() const;
	Common::Array<uint16> getResourceIDList(uint32 type) const;

	static uint32 convertTagToUppercase(uint32 tag);

protected:
	Common::SeekableReadStream *_stream;

	struct Resource {
		uint32 offset;
		uint32 size;
		Common::String name;
	};

	typedef Common::HashMap<uint16, Resource> ResourceMap;
	typedef Common::HashMap<uint32, ResourceMap> TypeMap;
	TypeMap _types;
};

class MacArchive : public Archive {
public:
	MacArchive();
	~MacArchive();

	void close();
	bool openFile(const Common::String &fileName);
	bool openStream(Common::SeekableReadStream *stream, uint32 startOffset = 0);
	Common::SeekableReadStream *getResource(uint32 tag, uint16 id);

private:
	Common::MacResManager *_resFork;
};

class RIFFArchive : public Archive {
public:
	RIFFArchive() : Archive() {}
	~RIFFArchive() {}

	bool openStream(Common::SeekableReadStream *stream, uint32 startOffset = 0);
	Common::SeekableReadStream *getResource(uint32 tag, uint16 id);
};

class RIFXArchive : public Archive {
public:
	RIFXArchive() : Archive(), _isBigEndian(true) {}
	~RIFXArchive() {}

	bool openStream(Common::SeekableReadStream *stream, uint32 startOffset = 0);

private:
	bool _isBigEndian;
};

} // End of namespace Director

#endif
