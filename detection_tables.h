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

#ifndef DIRECTOR_DETECTION_TABLES_H
#define DIRECTOR_DETECTION_TABLES_H

namespace Director {

static const DirectorGameDescription gameDescriptions[] = {
	{
		{
			"gundam0079",
			"",
			AD_ENTRY1("Gundam0079.exe", "1a7acbba10a7246ba58c1d53fc7203f5"),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GUIO_NOASPECT)
		},
		GID_GENERIC,
		5
	},

	{
		{
			"gundam0079",
			"",
			AD_ENTRY1("Gundam0079", "4c38a51a21a1ad231f218c4786ff771d"),
			Common::EN_ANY,
			Common::kPlatformMacintosh,
			ADGF_MACRESFORK,
			GUIO1(GUIO_NOASPECT)
		},
		GID_GENERIC,
		5
	},

	{
		{
			"jewels",
			"",
			AD_ENTRY1("JEWELS.EXE", "bb6d81471d166088260090472c6c3a87"),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GUIO_NOASPECT)
		},
		GID_GENERIC,
		4
	},

	{
		{
			"jewels",
			"",
			AD_ENTRY1("Jewels.exe", "c1a2e8b7e41fa204009324a9c7db1030"),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GUIO_NOASPECT)
		},
		GID_GENERIC,
		7
	},

	{
		{
			"jewels",
			"Two-Minute Demo",
			AD_ENTRY1("DEMO.EXE", "ebee52d3c4280674c600177df5b09da0"),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_DEMO,
			GUIO1(GUIO_NOASPECT)
		},
		GID_GENERIC,
		4
	},

	// Note: There are four versions of the binary included on the disc.
	// 5.6, 6, and 9 Meg variants all exist too.
	{
		{
			"jewels",
			"",
			AD_ENTRY1("Jewels 11 Meg", "339c89a148c4ff2c5c815c62ac006325"),
			Common::EN_ANY,
			Common::kPlatformMacintosh,
			ADGF_MACRESFORK,
			GUIO1(GUIO_NOASPECT)
		},
		GID_GENERIC,
		4
	},

	{
		{
			"jewels",
			"Two-Minute Demo",
			AD_ENTRY1("Two-Minute Demo", "01be45e7241194dad07938e7059b88e3"),
			Common::EN_ANY,
			Common::kPlatformMacintosh,
			ADGF_MACRESFORK | ADGF_DEMO,
			GUIO1(GUIO_NOASPECT)
		},
		GID_GENERIC,
		4
	},

	{
		{
			"jewels",
			"",
			AD_ENTRY1("Jewels of the Oracle", "fa52f0136cde568a46249ce74f01a324"),
			Common::EN_ANY,
			Common::kPlatformMacintosh,
			ADGF_MACRESFORK,
			GUIO1(GUIO_NOASPECT)
		},
		GID_GENERIC,
		7
	},

	{
		{
			"jewels",
			"Demo",
			AD_ENTRY1("JEWELS.EXE", "abcc448c035e88d4edb4a29034fd1e34"),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS | ADGF_DEMO,
			GUIO1(GUIO_NOASPECT)
		},
		GID_GENERIC,
		4
	},

	{
		{
			"jman",
			"",
			AD_ENTRY1("JOURNEY.EXE", "65d06b5fef155a2473434571aff5bc29"),
			Common::JA_JPN,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GUIO_NOASPECT)
		},
		GID_GENERIC,
		3
	},

	{
		{
			"jman",
			"Turbo!",
			AD_ENTRY1("JMP Turbo\xE2\x84\xA2", "cc3321069072b90f091f220bba16e4d4"), // Trademark symbol (UTF-8)
			Common::EN_ANY,
			Common::kPlatformMacintosh,
			ADGF_MACRESFORK,
			GUIO1(GUIO_NOASPECT)
		},
		GID_GENERIC,
		4
	},

	{
		{
			"majestic",
			"",
			AD_ENTRY1("MAJESTIC.EXE", "624267f70253e5327981003a6fc0aeba"),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GUIO_NOASPECT)
		},
		GID_GENERIC,
		4
	},

	{
		{
			"spyclub",
			"",
			AD_ENTRY1("SPYCLUB.EXE", "65d06b5fef155a2473434571aff5bc29"),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GUIO_NOASPECT)
		},
		GID_GENERIC,
		3
	},

	{ AD_TABLE_END_MARKER, GID_GENERIC, 0 }
};

} // End of Namespace Director

#endif
