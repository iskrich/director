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

#include "director/score.h"
#include "common/stream.h"
#include "common/debug.h"
#include "common/file.h"
#include "common/config-manager.h"

#include "common/system.h"
#include "director/dib.h"
#include "director/resource.h"
#include "director/lingo/lingo.h"

#include "graphics/palette.h"
#include "common/events.h"
#include "engines/util.h"
#include "graphics/managed_surface.h"

namespace Director {

Score::Score(Archive &movie, Lingo &lingo) {

	_surface = new Graphics::ManagedSurface;
	_movieArchive = &movie;
	_lingo = &lingo;

	_lingo->processEvent(kEventPrepareMovie, 0);

	assert(_movieArchive->hasResource(MKTAG('V','W','S','C'), 1024));
	assert(_movieArchive->hasResource(MKTAG('V','W','C','F'), 1024));
	assert(_movieArchive->hasResource(MKTAG('V','W','C','R'), 1024));

	loadFrames(*_movieArchive->getResource(MKTAG('V','W','S','C'), 1024));
	loadConfig(*_movieArchive->getResource(MKTAG('V','W','C','F'), 1024));
	loadCastData(*_movieArchive->getResource(MKTAG('V','W','C','R'), 1024));

	if (_movieArchive->hasResource(MKTAG('M','C','N','M'), 0)) {
		_macName = _movieArchive->getName(MKTAG('M','C','N','M'), 0).c_str();
	}

	if (_movieArchive->hasResource(MKTAG('V','W','L','B'), 1024)) {
		loadLabels(*_movieArchive->getResource(MKTAG('V','W','L','B'), 1024));
	}

	if (_movieArchive->hasResource(MKTAG('V','W','A','C'), 1024)) {
		loadActions(*_movieArchive->getResource(MKTAG('V','W','A','C'), 1024));
	}

	if (_movieArchive->hasResource(MKTAG('V','W','F','I'), 1024)) {
		loadFileInfo(*_movieArchive->getResource(MKTAG('V','W','F','I'), 1024));
	}

	if (_movieArchive->hasResource(MKTAG('V','W','F','M'), 1024)) {
		loadFontMap(*_movieArchive->getResource(MKTAG('V','W','F','M'), 1024));
	}

	Common::Array<uint16> vwci = _movieArchive->getResourceIDList(MKTAG('V','W','C','I'));
	if (vwci.size() > 0) {
		Common::Array<uint16>::iterator iterator;
		for (iterator = vwci.begin(); iterator != vwci.end(); ++iterator)
			loadCastInfo(*_movieArchive->getResource(MKTAG('V','W','C','I'), *iterator), *iterator);
	}

	DIBDecoder palette;
	Common::Array<uint16> clutList = _movieArchive->getResourceIDList(MKTAG('C','L','U','T'));

	if (clutList.size() > 1)
		error("More than one palette was found");
	if (clutList.size() == 0)
		warning("CLUT not found");

	//Common::SeekableReadStream *pal = _movieArchive->getResource(MKTAG('C', 'L', 'U', 'T'), clutList[0]);
	//palette.loadPalette(*pal);
	//g_system->getPaletteManager()->setPalette(palette.getPalette(), 0, palette.getPaletteColorCount());

}

Score::~Score() {
	_surface->free();
	delete _surface;

	_movieArchive->close();
	delete _movieArchive;
}

void Score::loadFrames(Common::SeekableReadStream &stream) {
	uint32 size = stream.readUint32BE();
	size -= 4;
	uint16 channelSize;
	uint16 channelOffset;

	Frame *initial = new Frame();
	_frames.push_back(initial);

	while (size != 0) {
		uint16 frameSize = stream.readUint16BE();
		size -= frameSize;
		frameSize -= 2;
		Frame *frame = new Frame(*_frames.back());
		while (frameSize != 0) {
			channelSize = stream.readByte() * 2;
			channelOffset = stream.readByte() * 2;
			frame->readChannel(stream, channelOffset, channelSize);
			frameSize -= channelSize + 2;
		}
		_frames.push_back(frame);
	}
	//remove initial frame
	_frames.remove_at(0);
}

void Score::loadConfig(Common::SeekableReadStream &stream) {
	/*uint16 unk1 = */ stream.readUint16BE();
	/*ver1 = */ stream.readUint16BE();
	_movieRect = Score::readRect(stream);

	_castArrayStart = stream.readUint16BE();
	_castArrayEnd = stream.readUint16BE();
	_currentFrameRate = stream.readByte();
	stream.skip(9);
	_stageColor = stream.readUint16BE();
}

void Score::readVersion(uint32 rid) {
	_versionMinor = rid & 0xffff;
	_versionMajor = rid >> 16;
	debug("%d.%d", _versionMajor, _versionMinor);
}

void Score::loadCastData(Common::SeekableReadStream &stream) {
	for (uint16 id = _castArrayStart; id <= _castArrayEnd; id++) {
		byte size = stream.readByte();
		if (size == 0)
			continue;
		uint8 castType = stream.readByte();
		switch (castType) {
		case kCastBitmap:
			_casts[id] = new BitmapCast(stream);
			_casts[id]->type = kCastBitmap;
			break;
		case kCastText:
			_casts[id] = new TextCast(stream);
			_casts[id]->type = kCastText;
			break;
		case kCastShape:
			_casts[id] = new ShapeCast(stream);
			_casts[id]->type = kCastShape;
			break;
		case kCastButton:
			_casts[id] = new ButtonCast(stream);
			_casts[id]->type = kCastButton;
			break;
		default:
			warning("Unhandled cast type: %d", castType);
			stream.skip(size - 1);
			break;
		}
	}
	//Set cast pointers to sprites
	for (uint16 i = 0; i < _frames.size(); i++) {
		for (uint16 j = 0; j < _frames[i]->_sprites.size(); j++) {
			byte castId = _frames[i]->_sprites[j]->_castId;
			if (_casts.contains(castId))
				_frames[i]->_sprites[j]->_cast = _casts.find(castId)->_value;
		}
	}
}

void Score::loadLabels(Common::SeekableReadStream &stream) {
	uint16 count = stream.readUint16BE() + 1;
	uint16 offset = count * 4 + 2;

	uint16 frame = stream.readUint16BE();
	uint16 stringPos = stream.readUint16BE() + offset;

	for (uint16 i = 0; i < count; i++) {

		uint16 nextFrame = stream.readUint16BE();
		uint16 nextStringPos = stream.readUint16BE() + offset;
		uint16 streamPos = stream.pos();

		stream.seek(stringPos);

		for (uint16 j = stringPos; j < nextStringPos; j++) {
			_labels[frame] += stream.readByte();
		}

		stream.seek(streamPos);

		frame = nextFrame;
		stringPos = nextStringPos;
	}

	Common::HashMap<uint16, Common::String>::iterator j;
	for (j = _labels.begin(); j != _labels.end(); ++j) {
		debug("Frame %d, Label %s", j->_key, j->_value.c_str());
	}
}

void Score::loadActions(Common::SeekableReadStream &stream) {

	uint16 count = stream.readUint16BE() + 1;
	uint16 offset = count * 4 + 2;

	byte id = stream.readByte();
	/*byte subId = */ stream.readByte(); //I couldn't find how it used in continuity (except print). Frame actionId = 1 byte.
	uint16 stringPos = stream.readUint16BE() + offset;

	for (uint16 i = 0; i < count; i++) {

		uint16 nextId = stream.readByte();
		/*byte subId = */ stream.readByte();
		uint16 nextStringPos = stream.readUint16BE() + offset;
		uint16 streamPos = stream.pos();

		stream.seek(stringPos);

		for (uint16 j = stringPos; j < nextStringPos; j++) {
			_actions[id] += stream.readByte();
		}

		stream.seek(streamPos);

		id = nextId;
		stringPos = nextStringPos;
		if (stringPos == stream.size())
			break;
	}

	Common::HashMap<uint16, Common::String>::iterator j;

	if (!ConfMan.getBool("dump_scripts"))
		return;

	for (j = _actions.begin(); j != _actions.end(); ++j) {
		dumpScript(j->_key, kFrameScript, j->_value);
	}
}

void Score::dumpScript(uint16 id, scriptType type, Common::String script) {
	Common::DumpFile out;
	Common::String typeName;
	char buf[256];

	switch (type) {
	case kFrameScript:
		typeName = "frame";
		break;
	case kMovieScript:
		typeName = "movie";
		break;
	case kSpriteScript:
		typeName = "sprite";
		break;
	}

	sprintf(buf, "./dumps/%s-%s-%d.txt", _macName.c_str(), typeName.c_str(), id);

	if (!out.open(buf)) {
		warning("Can not open dump file %s", buf);
		return;
	}
	out.writeString(script);

	out.flush();
	out.close();
}

void Score::loadCastInfo(Common::SeekableReadStream &stream, uint16 id) {
	uint32 entryType = 0;
	Common::Array<Common::String> castStrings = loadStrings(stream, entryType);
	CastInfo *ci = new CastInfo();
	ci->script = castStrings[0];
	ci->name = getString(castStrings[1]);
	ci->directory = getString(castStrings[2]);
	ci->fileName = getString(castStrings[3]);
	ci->type = castStrings[4];
	_castsInfo[id] = ci;
}

Common::String Score::getString(Common::String str) {
	if (str.size() == 0) {
		return str;
	}
	uint8 f = static_cast<uint8>(str.firstChar());

	if (f == 0) {
		return "";
	}
	str.deleteChar(0);
	if (str.lastChar() == '\x00') {
		str.deleteLastChar();
	}
	return str;
}

void Score::loadFileInfo(Common::SeekableReadStream &stream) {
	Common::Array<Common::String> fileInfoStrings = loadStrings(stream, _flags);
	_script = fileInfoStrings[0];
	_changedBy = fileInfoStrings[1];
	_createdBy = fileInfoStrings[2];
	_directory = fileInfoStrings[3];
}

Common::Array<Common::String> Score::loadStrings(Common::SeekableReadStream &stream, uint32 &entryType, bool hasHeader) {
	Common::Array<Common::String> strings;
	uint32 offset = 0;
	if (hasHeader) {
		offset = stream.readUint32BE();
		/*uint32 unk1 = */ stream.readUint32BE();
		/*uint32 unk2 = */ stream.readUint32BE();
		entryType = stream.readUint32BE();
		stream.seek(offset);
	}

	uint16 count = stream.readUint16BE();
	offset += (count + 1) * 4 + 2; //positions info + uint16 count
	uint32 startPos = stream.readUint32BE() + offset;
	for (uint16 i = 0; i < count; i++) {
		Common::String entryString;
		uint32 nextPos = stream.readUint32BE() + offset;
		uint32 streamPos = stream.pos();

		stream.seek(startPos);

		while (startPos != nextPos) {
			entryString += stream.readByte();
			++startPos;
		}

		strings.push_back(entryString);

		stream.seek(streamPos);
		startPos = nextPos;
	}
	return strings;
}

void Score::loadFontMap(Common::SeekableReadStream &stream) {
	uint16 count = stream.readUint16BE();
	uint32 offset = (count * 2) + 2;
	uint16 currentRawPosition = offset;

	for (uint16 i = 0; i < count; i++) {
		uint16 id = stream.readUint16BE();
		uint32 positionInfo = stream.pos();

		stream.seek(currentRawPosition);

		uint16 size = stream.readByte();
		Common::String font;

		for (uint16 k = 0; k < size; k++) {
			font += stream.readByte();
		}

		_fontMap[id] = font;
		currentRawPosition = stream.pos();
		stream.seek(positionInfo);
	}
}

BitmapCast::BitmapCast(Common::SeekableReadStream &stream) {
	/*byte flags = */ stream.readByte();
	uint16 someFlaggyThing = stream.readUint16BE();
	initialRect = Score::readRect(stream);
	boundingRect = Score::readRect(stream);
	regY = stream.readUint16BE();
	regX = stream.readUint16BE();
	if (someFlaggyThing & 0x8000) {
		/*uint16 unk1 =*/ stream.readUint16BE();
		/*uint16 unk2 =*/ stream.readUint16BE();
	}
}

TextCast::TextCast(Common::SeekableReadStream &stream) {
	/*byte flags =*/ stream.readByte();
	borderSize = stream.readByte();
	gutterSize = stream.readByte();
	boxShadow = stream.readByte();
	textType = stream.readByte();
	textAlign = stream.readUint16BE();
	stream.skip(6); //palinfo
	/*uint32 unk1 = */ stream.readUint32BE();
	initialRect = Score::readRect(stream);
	textShadow = stream.readByte();
	textFlags = stream.readByte();
	/*uint16 unk2 =*/ stream.readUint16BE();
}

ShapeCast::ShapeCast(Common::SeekableReadStream &stream) {
	/*byte flags = */ stream.readByte();
	/*unk1 = */ stream.readByte();
	shapeType = stream.readByte();
	initialRect = Score::readRect(stream);
	pattern = stream.readUint16BE();
	fgCol = stream.readByte();
	bgCol = stream.readByte();
	fillType = stream.readByte();
	lineThickness = stream.readByte();
	lineDirection = stream.readByte();
}

Common::Rect Score::readRect(Common::SeekableReadStream &stream) {
	Common::Rect *rect = new Common::Rect();
	rect->top = stream.readUint16BE();
	rect->left = stream.readUint16BE();
	rect->bottom = stream.readUint16BE();
	rect->right = stream.readUint16BE();
	return *rect;
}

void Score::startLoop() {
	initGraphics(_movieRect.width(), _movieRect.height(), true);
	_surface->create(_movieRect.width(), _movieRect.height());
	_currentFrame = 0;
	_stopPlay = false;
	_nextFrameTime = 0;

	_lingo->processEvent(kEventStartMovie, 0);
	_frames[_currentFrame]->prepareFrame(*_movieArchive, *_surface, _movieRect);
	while (!_stopPlay && _currentFrame < _frames.size() - 2) {
		update();
		processEvents();
		g_system->updateScreen();
		g_system->delayMillis(10);
	}
}

void Score::update() {
	if (g_system->getMillis() < _nextFrameTime)
		return;
	//FIXME  0 - default white? (Still believe that last color in palette everywhere white)
	if (_stageColor == 0)
		_surface->clear(15);

	//Enter and exit from previous frame (Director 4)
	_lingo->processEvent(kEventEnterFrame, _currentFrame);
	_lingo->processEvent(kEventExitFrame, _currentFrame);
	//TODO Director 6 - another order


	//TODO Director 6 step: send beginSprite event to any sprites whose span begin in the upcoming frame
	//for (uint16 i = 0; i < CHANNEL_COUNT; i++) {
	//	if (_frames[_currentFrame]->_sprites[i]->_enabled)
	//		_lingo->processEvent(kEventBeginSprite, i);
	//}

	//TODO Director 6 step: send prepareFrame event to all sprites and the script channel in upcoming frame
	//_lingo->processEvent(kEventPrepareFrame, _currentFrame);
	_currentFrame++;
	_frames[_currentFrame]->prepareFrame(*_movieArchive, *_surface, _movieRect);
	//Stage is drawn between the prepareFrame and enterFrame events (Lingo in a Nutshell)

	byte tempo = _frames[_currentFrame]->_tempo;

	if (tempo) {
		if (tempo > 161) {
			//Delay
			_nextFrameTime = g_system->getMillis() + (256 - tempo) * 1000;
			return;
		} else if (tempo <= 60) {
			//FPS
			_nextFrameTime = g_system->getMillis() + (float)tempo / 60 * 1000;
			_currentFrameRate = tempo;
		} else if (tempo >= 136) {
			//TODO Wait for channel tempo - 135
		} else if (tempo == 128) {
			//TODO Wait for Click/Key
		} else if (tempo == 135) {
			//TODO Wait for sound channel 1
		} else if (tempo == 134) {
			//TODO Wait for sound channel 2
		}
	}
	_nextFrameTime = g_system->getMillis() + (float)_currentFrameRate / 60 * 1000;
}

void Score::processEvents() {
	if (_currentFrame > 0)
		_lingo->processEvent(kEventIdle, _currentFrame - 1);

	Common::Event event;
	while (g_system->getEventManager()->pollEvent(event)) {
		if (event.type == Common::EVENT_QUIT)
			_stopPlay = true;

		if (event.type == Common::EVENT_LBUTTONDOWN) {
			Common::Point pos = g_system->getEventManager()->getMousePos();
			//TODO there is dont send frame id
			_lingo->processEvent(kEventMouseDown, _frames[_currentFrame]->getSpriteIDFromPos(pos));
		}

		if (event.type == Common::EVENT_LBUTTONUP) {
			Common::Point pos = g_system->getEventManager()->getMousePos();
			_lingo->processEvent(kEventMouseUp, _frames[_currentFrame]->getSpriteIDFromPos(pos));
		}
	}
}

Frame::Frame() {
	_transFlags = 0;
	_transChunkSize = 0;
	_tempo = 0;

	_sound1 = 0;
	_sound2 = 0;
	_soundType1 = 0;
	_soundType2 = 0;

	_actionId = 0;
	_skipFrameFlag = 0;
	_blend = 0;

	_sprites.resize(CHANNEL_COUNT);
	for (uint16 i = 0; i < _sprites.size(); i++) {
		Sprite *sp = new Sprite();
		_sprites[i] = sp;
	}
}

Frame::Frame(const Frame &frame) {
	_actionId = frame._actionId;
	_transFlags = frame._transFlags;
	_transType = frame._transType;
	_transChunkSize = frame._transChunkSize;
	_tempo = frame._tempo;
	_sound1 = frame._sound1;
	_sound2 = frame._sound2;
	_soundType1 = frame._soundType1;
	_soundType2 = frame._soundType2;
	_skipFrameFlag = frame._skipFrameFlag;
	_blend = frame._blend;

	_sprites.resize(CHANNEL_COUNT);
	for (uint16 i = 0; i < CHANNEL_COUNT; i++) {
		_sprites[i] = new Sprite(*frame._sprites[i]);
	}
}

Frame::~Frame() {
	for (uint16 i = 0; i < _sprites.size(); i++) {
		delete _sprites[i];
	}
}

void Frame::readChannel(Common::SeekableReadStream &stream, uint16 offset, uint16 size) {
	if (offset >= 32) {
		if (size <= 16)
			readSprite(stream, offset, size);
		else {
			//read > 1 sprites channel
			while (size > 16) {
				byte spritePosition = (offset - 32) / 16;
				uint16 nextStart = (spritePosition + 1) * 16 + 32;
				uint16 needSize = nextStart - offset;
				readSprite(stream, offset, needSize);
				offset += needSize;
				size -= needSize;
			}
			readSprite(stream, offset, size);
		}
	} else {
		readMainChannels(stream, offset, size);
	}
}

void Frame::readMainChannels(Common::SeekableReadStream &stream, uint16 offset, uint16 size) {
	uint16 finishPosition = offset + size;

	while (offset < finishPosition) {
		switch(offset) {
		case kScriptIdPosition:
			_actionId = stream.readByte();
			offset++;
			break;
		case kSoundType1Position:
			_soundType1 = stream.readByte();
			offset++;
			break;
		case kTransFlagsPosition:
			_transFlags = stream.readByte();
			offset++;
			break;
		case kTransChunkSizePosition:
			_transChunkSize = stream.readByte();
			offset++;
			break;
		case kTempoPosition:
			_tempo = stream.readByte();
			offset++;
			break;
		case kTransTypePosition:
			_transType = static_cast<transitionType>(stream.readByte());
			offset++;
			break;
		case kSound1Position:
			_sound1 = stream.readUint16LE();
			offset+=2;
			break;
		case kSkipFrameFlagsPosition:
			_skipFrameFlag = stream.readByte();
			offset++;
			break;
		case kBlendPosition:
			_blend = stream.readByte();
			offset++;
			break;
		case kSound2Position:
			_sound2 = stream.readUint16LE();
			offset += 2;
			break;
		case kSound2TypePosition:
			_soundType2 = stream.readByte();
			offset += 1;
			break;
		case kPaletePosition:
			//TODO palette channel
			stream.skip(16);
			offset += 16;
		default:
			offset++;
			stream.readByte();
			debug("Field Position %d, Finish Position %d", offset, finishPosition);
			break;
		}
	}
}

void Frame::readSprite(Common::SeekableReadStream &stream, uint16 offset, uint16 size) {
	uint16 spritePosition = (offset - 32) / 16;
	uint16 spriteStart = spritePosition * 16 + 32;

	uint16 fieldPosition = offset - spriteStart;
	uint16 finishPosition = fieldPosition + size;

	Sprite &sprite = *_sprites[spritePosition];

	while (fieldPosition < finishPosition) {
		switch (fieldPosition) {
		case kSpritePositionUnk1:
			/*byte x1 = */ stream.readByte();
			fieldPosition++;
			break;
		case kSpritePositionEnabled:
			sprite._enabled = (stream.readByte() != 0);
			fieldPosition++;
			break;
		case kSpritePositionUnk2:
			/*byte x2 = */ stream.readUint16BE();
			fieldPosition += 2;
			break;
		case kSpritePositionFlags:
			sprite._flags = stream.readUint16BE();
			sprite._ink = static_cast<inkType>(sprite._flags & 0x3f); //TODO more flags?
			sprite._trails = sprite._flags & 0x40;
			fieldPosition += 2;
			break;
		case kSpritePositionCastId:
			sprite._castId = stream.readUint16BE();
			fieldPosition += 2;
			break;
		case kSpritePositionY:
			sprite._startPoint.y = stream.readUint16BE();
			fieldPosition += 2;
			break;
		case kSpritePositionX:
			sprite._startPoint.x = stream.readUint16BE();
			fieldPosition += 2;
			break;
		case kSpritePositionWidth:
			sprite._width = stream.readUint16BE();
			fieldPosition += 2;
			break;
		case kSpritePositionHeight:
			sprite._height = stream.readUint16BE();
			fieldPosition += 2;
			break;
		default:
			//end cycle, go to next sprite channel
			readSprite(stream, spriteStart + 16, finishPosition - fieldPosition);
			fieldPosition = finishPosition;
			break;
		}
	}
}

void Frame::prepareFrame(Archive &_movie, Graphics::ManagedSurface &surface, Common::Rect movieRect) {
	renderSprites(_movie, surface, movieRect);
	renderTrailSprites(_movie, surface, movieRect);
	if (_transType != 0)
		playTranisition();
	if (_sound1 != 0 || _sound2 != 0) {
		playSoundChannel();
	}
}

void Frame::playSoundChannel() {
	debug(0, "Sound2 %d", _sound2);
	debug(0, "Sound1 %d", _sound1);
}

void Frame::playTranisition() {
	warning("STUB: playTranisition(%d, %d, %d)", _transType, _transFlags, _transChunkSize);
}

void Frame::renderSprites(Archive &_movie, Graphics::ManagedSurface &surface, Common::Rect movieRect) {
	for (uint16 i = 0; i < CHANNEL_COUNT; i++) {
		//TODO if trails == 0 -> render as trail sprite
		if (_sprites[i]->_enabled) {
			DIBDecoder img;
			uint32 imgId = 1024 + _sprites[i]->_castId;
			if (!_movie.hasResource(MKTAG('D', 'I', 'B', ' '), imgId)) {
				continue;
			}

			img.loadStream(*_movie.getResource(MKTAG('D', 'I', 'B', ' '), imgId));

			uint32 regX = static_cast<BitmapCast *>(_sprites[i]->_cast)->regX;
			uint32 regY = static_cast<BitmapCast *>(_sprites[i]->_cast)->regY;
			uint32 rectLeft = static_cast<BitmapCast *>(_sprites[i]->_cast)->initialRect.left;
			uint32 rectTop = static_cast<BitmapCast *>(_sprites[i]->_cast)->initialRect.top;

			int x = _sprites[i]->_startPoint.x - regX + rectLeft;
			int y = _sprites[i]->_startPoint.y - regY + rectTop;
			int height = _sprites[i]->_height;
			int width = _sprites[i]->_width;
			if (x < 0) {
				width += x;
				x = 0;
			}
			if (y < 0) {
				height += y;
				y = 0;
			}
			Common::Rect drawRect = Common::Rect(x, y, x + width, y + height);
			_drawRects.push_back(drawRect);

			switch (_sprites[i]->_ink) {
			case kInkTypeCopy:
				surface.blitFrom(*img.getSurface(), Common::Point(x, y));
				break;
			case kInkTypeBackgndTrans:
				drawBackgndTransSprite(surface, *img.getSurface(), drawRect);
				break;
			case kInkTypeMatte:
				drawMatteSprite(surface, *img.getSurface(), drawRect);
				break;
			default:
				warning("Unhandled ink type %d", _sprites[i]->_ink);
				surface.blitFrom(*img.getSurface(), Common::Point(x, y));
				break;
			}
		}
	}
	g_system->copyRectToScreen(surface.getPixels(), surface.pitch, 0, 0, surface.getBounds().width(), surface.getBounds().height());
}

void Frame::renderTrailSprites(Archive &_movie, Graphics::ManagedSurface &surface, Common::Rect movieRect) {
	for (uint16 i = 0; i < CHANNEL_COUNT; i++) {
		if (_sprites[i]->_enabled && _sprites[i]->_trails != 0)
			warning("STUB: renderTrailSprites(%d)", i);
	}
}

void Frame::drawBackgndTransSprite(Graphics::ManagedSurface &target, const Graphics::Surface &sprite, Common::Rect &drawRect) {
	uint8 skipColor = 15; //FIXME is it always white (last entry in pallette) ?
	for (int ii = 0; ii < sprite.h; ii++) {
		const byte *src = (const byte *)sprite.getBasePtr(0, ii);
		byte *dst = (byte *)target.getBasePtr(drawRect.left, drawRect.top + ii);
		for (int j = 0; j < drawRect.width(); j++) {
			if (*src != skipColor)
				*dst = *src;
			src++;
			dst++;
		}
	}
}

void Frame::drawMatteSprite(Graphics::ManagedSurface &target, const Graphics::Surface &sprite, Common::Rect &drawRect) {
	//Like background trans, but all white pixels NOT ENCLOSED by coloured pixels are transparent
	Graphics::Surface tmp;
	tmp.copyFrom(sprite);

	Graphics::FloodFill ff(&tmp, *(byte *)tmp.getBasePtr(0, 0), 0);

	for (int yy = 0; yy < tmp.h; yy++) {
		ff.addSeed(0, yy);
		ff.addSeed(tmp.w - 1, yy);
	}
	for (int xx = 0; xx < tmp.w; xx++) {
		ff.addSeed(xx, 0);
		ff.addSeed(xx, tmp.h - 1);
	}
	ff.fillMask();

	for (int yy = 0; yy < tmp.h; yy++) {
		const byte *src = (const byte *)tmp.getBasePtr(0, yy);
		const byte *mask = (const byte *)ff.getMask()->getBasePtr(0, yy);
		byte *dst = (byte *)target.getBasePtr(drawRect.left, drawRect.top + yy);

		for (int xx = 0; xx < drawRect.width(); xx++, src++, dst++, mask++)
			if (*mask == 0)
				*dst = *src;
	}

	tmp.free();
}

uint16 Frame::getSpriteIDFromPos(Common::Point pos) {
	//Find first from top to bottom
	for (uint16 i = _drawRects.size() - 1; i > 0; i--) {
		if (_drawRects[i].contains(pos))
			return i;
	}
	return 0;
}

Sprite::Sprite() {
	_enabled = false;
	_width = 0;
	_ink = kInkTypeCopy;
	_flags = 0;
	_height = 0;
	_castId = 0;
	_castId = 0;
}

Sprite::Sprite(const Sprite &sprite) {
	_enabled = sprite._enabled;
	_castId = sprite._castId;
	_flags = sprite._flags;
	_ink = sprite._ink;
	_width = sprite._width;
	_height = sprite._height;
	_startPoint.x = sprite._startPoint.x;
	_startPoint.y = sprite._startPoint.y;
}

} //End of namespace Director
