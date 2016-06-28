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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

// Heavily inspired by hoc
// Copyright (C) AT&T 1995
// All Rights Reserved
//
// Permission to use, copy, modify, and distribute this software and
// its documentation for any purpose and without fee is hereby
// granted, provided that the above copyright notice appear in all
// copies and that both that the copyright notice and this
// permission notice and warranty disclaimer appear in supporting
// documentation, and that the name of AT&T or any of its entities
// not be used in advertising or publicity pertaining to
// distribution of the software without specific, written prior
// permission.
//
// AT&T DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
// INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
// IN NO EVENT SHALL AT&T OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
// SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
// IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
// ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
// THIS SOFTWARE.

#include "engines/director/lingo/lingo.h"
#include "common/file.h"
#include "audio/decoders/wave.h"

#include "director/lingo/lingo-gr.h"

namespace Director {

void Lingo::execute(int pc) {
	for(_pc = pc; (*_currentScript)[_pc] != STOP && !_returning;) {

		for (int i = 0; i < _stack.size(); i++) {
			debugN(5, "%d ", _stack[i].val);
		}
		debug(5, "");

		_pc++;
		(*((*_currentScript)[_pc - 1]))();
	}
}

Symbol *Lingo::lookupVar(const char *name) {
	Symbol *sym;

	if (!_vars.contains(name)) { // Create variable if it was not defined
		sym = new Symbol;
		sym->name = (char *)calloc(strlen(name) + 1, 1);
		Common::strlcpy(sym->name, name, strlen(name) + 1);
		sym->type = VOID;
		sym->u.val = 0;

		_vars[name] = sym;
	} else {
		sym = g_lingo->_vars[name];
	}

	return sym;
}

void Lingo::define(Common::String &name, int start, int nargs) {
	debug(3, "define(\"%s\", %d, %d, %d)", name.c_str(), start, _currentScript->size() - 1, nargs);

	Symbol *sym;

	if (!_handlers.contains(name)) { // Create variable if it was not defined
		sym = new Symbol;

		sym->name = (char *)calloc(name.size() + 1, 1);
		Common::strlcpy(sym->name, name.c_str(), name.size() + 1);
		sym->type = HANDLER;

		_handlers[name] = sym;
	} else {
		sym = g_lingo->_handlers[name];

		warning("Redefining handler '%s'", name.c_str());
		delete sym->u.defn;
	}

	sym->u.defn = new ScriptData(&(*_currentScript)[start], _currentScript->size() - start + 1);
	sym->nargs = nargs;
}

void Lingo::codeArg(Common::String &s) {
	g_lingo->code1(g_lingo->c_varpush);
	g_lingo->codeString(s.c_str());
	g_lingo->code1(g_lingo->c_assign);
	g_lingo->code1(g_lingo->c_xpop);
}

int Lingo::codeId(Common::String &s) {
	return g_lingo->codeId_(s);
}

int Lingo::codeId_(Common::String &name) {
	int ret;

	if (_handlers.contains(name)) { // This is a call
		ret = code1(c_call);
		codeString(name.c_str());
		code1((inst)0);	// Zero arguments
	} else {
		ret = code1(c_varpush);

		codeString(name.c_str());
		code1(c_eval);
	}

	return ret;
}

}