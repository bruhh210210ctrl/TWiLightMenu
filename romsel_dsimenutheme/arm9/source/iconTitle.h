/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2013
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy
	Claudio "sverx"

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/
#pragma once

#include <string_view>

void getGameInfo(bool isDir, const char* name, int num, bool fromArgv = false);
void iconUpdate(bool isDir, const char* name, int num);

// Multiple rows view: forget which slot each icon bank holds (call when the list changes), and
// whether a slot's icon is currently loaded in its bank.
void iconViewForgetAll();

// Which bank pool iconUpdate() loads into: -1 = the one in use, 0 = single row, 1 = paged views.
extern volatile int iconPoolOverride;
bool iconViewReady(int slot);
void titleUpdate(bool isDir, std::string_view name, int num);
void drawIcon(int Xpos, int Ypos, int num);
// Multiple rows view: the slot's icon at a scale (20.12, 1 << 12 = normal)
void drawIconScaled(int Xpos, int Ypos, int num, int scale);
void execDeferredIconUpdates();
void loadDeferredIconPalettes();
void writeBannerText(std::string_view name, std::string_view text);
void writeBannerText(std::string_view name, std::u16string text);
void writeBannerText(std::u16string name, std::u16string text);
