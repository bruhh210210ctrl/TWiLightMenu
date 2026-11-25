#include "gift.h"

#include <limits.h>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nds.h>

#include "common/systemdetails.h"
#include "common/twlmenusettings.h"
#include "graphics/color.h"
#include "graphics/graphics.h"
#include "graphics/ThemeTextures.h"

// Defined in ndsheaderbanner.cpp
extern bool isDirectory[40];
extern bool bnrSysSettings[41];

// Defined in fileBrowse.cpp and ThemeTextures.cpp
extern bool boxArtLoaded;
extern unsigned int boxArtWidth, boxArtHeight;

bool giftWrapped[40] = {false};
int giftAnimFrame[40] = {0};
bool giftMovingApp = false;

// Full paths of every app that has been unwrapped
static std::set<std::string> seenApps;
static bool seenListLoaded = false;

static const char *seenListPath(void) {
	return sys().isRunFromSD() ? "sd:/_nds/TWiLightMenu/giftseen.txt" : "fat:/_nds/TWiLightMenu/giftseen.txt";
}

// Apps are remembered by folder + file name, so the same file name in two
// different folders is two different apps.
static std::string entryKey(const std::string &name) {
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		return name;
	}
	std::string key(cwd);
	if (key.empty() || key[key.size() - 1] != '/') {
		key += '/';
	}
	return key + name;
}

void giftLoadSeenList() {
	if (seenListLoaded) {
		return;
	}
	seenListLoaded = true;

	FILE *file = fopen(seenListPath(), "r");
	if (!file) {
		return; // No list yet: every app is new
	}
	char line[PATH_MAX + 2];
	while (fgets(line, sizeof(line), file)) {
		size_t len = strlen(line);
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
			line[--len] = '\0';
		}
		if (len > 0) {
			seenApps.insert(std::string(line));
		}
	}
	fclose(file);
}

void giftClearSlots() {
	for (int i = 0; i < 40; i++) {
		giftWrapped[i] = false;
		giftAnimFrame[i] = 0;
	}
}

void giftUpdateSlot(int slot, bool isDir, const std::string &name) {
	giftAnimFrame[slot] = 0;
	if (ms().theme != TWLSettings::ETheme3DS || isDir) {
		giftWrapped[slot] = false;
		return;
	}
	giftLoadSeenList();
	giftWrapped[slot] = (seenApps.find(entryKey(name)) == seenApps.end());
}

bool giftActive(int slot) {
	return giftWrapped[slot] && !isDirectory[slot] && !bnrSysSettings[slot];
}

int giftSpriteFrame(int slot) {
	const int t = giftAnimFrame[slot];
	if (t < 24) {
		return 0; // Closed (shaking)
	} else if (t < 30) {
		return 1; // Lid starts to lift
	} else if (t < 36) {
		return 2; // Lid up, light shining out
	}
	return 3; // Burst of light
}

int giftShakeX(int slot) {
	const int t = giftAnimFrame[slot];
	if (t <= 0 || t >= 24) {
		return 0;
	}
	const int amplitude = (t < 12) ? 2 : 3;
	return ((t / 2) % 2) ? amplitude : -amplitude;
}

void giftFinishUnwrap(int slot, const std::string &name) {
	const std::string key = entryKey(name);
	if (seenApps.insert(key).second) {
		FILE *file = fopen(seenListPath(), "a");
		if (file) {
			fprintf(file, "%s\n", key.c_str());
			fclose(file);
		}
	}
	giftWrapped[slot] = false;
	giftAnimFrame[slot] = 0;
}

// ---------------------------------------------------------------------------
// Spinning gift on the top screen
// ---------------------------------------------------------------------------

static unsigned short spinColors[256];	// RGB555 | BIT(15)
static unsigned char spinAlphas[256];

void giftComposite(unsigned short *dst, const unsigned short *bg, const unsigned char *frame,
		int w, int h, const unsigned short *colorTable) {
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			const int o = (y * 256) + x;
			const unsigned char index = frame[(y * w) + x];
			if (index == 0) {
				dst[o] = bg[o];
				continue;
			}
			const unsigned char a = spinAlphas[index];
			unsigned short color = spinColors[index];
			if (colorTable) {
				color = colorTable[color % 0x8000] | BIT(15);
			}
			dst[o] = (a == 255) ? color : alphablend(color, bg[o], a);
		}
	}
}

static FILE *spinFile = NULL;
static int spinFrame = 0;
static int spinTick = 0;
static int spinIdle = 0;			// Calls since the gift was last wanted

// Every frame, once read. NULL if there was not enough memory, in which case frames are
// streamed from the card through spinSingle instead.
static unsigned char *spinCache = NULL;
static unsigned int spinCached = 0;		// Bit per frame in spinCache
static bool spinCacheTried = false;
static unsigned char spinSingle[GIFT_SPIN_FRAME_BYTES] __attribute__((aligned(4)));

// After this many calls without a wanted gift the cache is given back (5 seconds)
#define GIFT_SPIN_KEEP_CALLS 300

static void spinClose(void) {
	if (spinFile) {
		fclose(spinFile);
		spinFile = NULL;
	}
}

static void spinFreeCache(void) {
	if (spinCache) {
		free(spinCache);
		spinCache = NULL;
	}
	spinCached = 0;
	spinCacheTried = false;
}

// Reads the colour table; returns false if the file is not usable
static bool spinReadHeader(void) {
	unsigned char header[GIFT_SPIN_HEADER_BYTES];
	if (fseek(spinFile, 0, SEEK_SET) != 0 || fread(header, 1, sizeof(header), spinFile) != sizeof(header)) {
		return false;
	}
	for (int i = 0; i < 256; i++) {
		spinColors[i] = (unsigned short)(header[i * 2] | (header[(i * 2) + 1] << 8)) | BIT(15);
		spinAlphas[i] = header[512 + i];
	}
	return true;
}

// Returns a pointer to the pixels of frame k, reading it from the card if it is not cached.
// NULL if the card could not be read.
static const unsigned char *spinGetFrame(int k) {
	unsigned char *dst;
	if (spinCache) {
		dst = spinCache + ((size_t)k * GIFT_SPIN_FRAME_BYTES);
		if (spinCached & (1u << k)) {
			return dst;
		}
	} else {
		dst = spinSingle;
	}
	if (fseek(spinFile, (long)GIFT_SPIN_HEADER_BYTES + ((long)k * GIFT_SPIN_FRAME_BYTES), SEEK_SET) != 0 ||
	    fread(dst, 1, GIFT_SPIN_FRAME_BYTES, spinFile) != (size_t)GIFT_SPIN_FRAME_BYTES) {
		return NULL;
	}
	if (spinCache) {
		spinCached |= (1u << k);
	}
	return dst;
}

bool giftSpinUpdate(bool wanted) {
	if (!wanted) {
		if (spinFile) {
			spinClose();
			// If nothing else has cleared the top screen yet, put the background back
			if (boxArtWidth == GIFT_SPIN_W && boxArtHeight == GIFT_SPIN_H) {
				clearBoxArt();
				resumeRotatingCubesVideo();
			}
			boxArtLoaded = false;
		}
		if (spinCache && ++spinIdle > GIFT_SPIN_KEEP_CALLS) {
			spinFreeCache();
		}
		return false;
	}
	spinIdle = 0;

	// (Re)start when we have no file open, or the top screen was cleared under us
	// (for example the cursor moved away and back)
	const bool restart = (spinFile == NULL) || !boxArtLoaded || (boxArtWidth != GIFT_SPIN_W);
	if (restart) {
		spinClose();
		spinFile = fopen(GIFT_SPIN_PATH, "rb");
		if (!spinFile) {
			return false; // No animation data: fall back to normal box art
		}
		if (!spinReadHeader()) {
			spinClose();
			return false;
		}
		if (!spinCache && !spinCacheTried) {
			spinCacheTried = true;
			spinCache = (unsigned char *)malloc((size_t)GIFT_SPIN_FRAMES * GIFT_SPIN_FRAME_BYTES);
			spinCached = 0;
		}
		spinFrame = 0;
		spinTick = 0;
	} else {
		if (++spinTick < GIFT_SPIN_TICKS) {
			// Between frames: read the next one ahead of time so that showing it costs nothing
			if (spinCache && spinTick == 1) {
				spinGetFrame((spinFrame + 1) % GIFT_SPIN_FRAMES);
			}
			return true;
		}
		spinTick = 0;
		spinFrame = (spinFrame + 1) % GIFT_SPIN_FRAMES;
	}

	const unsigned char *pixels = spinGetFrame(spinFrame);
	if (!pixels) {
		spinClose();
		return false;
	}
	tex().drawGiftSpinFrame(pixels, restart);
	boxArtLoaded = true;
	return true;
}
