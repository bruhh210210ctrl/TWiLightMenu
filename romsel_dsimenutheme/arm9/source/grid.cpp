#include "grid.h"

#include <algorithm>

#include "common/twlmenusettings.h"

static const GridLayout largeLayout = {3, 2, 24, 34, 72, 68, 64, 16, 12};
// 40 pixel boxes (a 0.625 scale of the 64 pixel graphics) with the icon at its normal size inside
static const GridLayout smallLayout = {6, 3, 8, 34, 40, 44, 40, 4, 3};

const GridLayout &gridLayout(bool small) { return small ? smallLayout : largeLayout; }
const GridLayout &gridCurrentLayout() { return gridLayout(gridIsSmall()); }
int gridBoxScale(const GridLayout &l) { return (l.box << 12) / 64; }

bool gridEnabled() {
	return ms().multiRow && ms().theme == TWLSettings::ETheme3DS && !ms().macroMode;
}

bool gridIsSmall() { return ms().multiRowSmall; }

int gridPerPage() {
	const GridLayout &l = gridCurrentLayout();
	return l.cols * l.rows;
}

int gridPageOf(int slot) { return std::max(slot, 0) / gridPerPage(); }

int gridNeighbour(int cur, int dir, int last, bool small) {
	const GridLayout &l = gridLayout(small);
	const int perPage = l.cols * l.rows;
	const int page = cur / perPage;
	const int inPage = cur % perPage;
	const int row = inPage / l.cols;
	const int col = inPage % l.cols;
	int target = -1;

	switch (dir) {
	case 0: // Left
		if (col > 0) {
			target = cur - 1;
		} else if (page > 0) {
			// Last box of the same row on the page before
			target = ((page - 1) * perPage) + (row * l.cols) + (l.cols - 1);
		}
		break;
	case 1: // Right
		if (col < l.cols - 1) {
			target = cur + 1;
		} else {
			// First box of the same row on the next page
			target = ((page + 1) * perPage) + (row * l.cols);
			if (target > last) {
				// A short last page: land on its last box if it has any
				target = ((page + 1) * perPage <= last) ? last : -1;
			}
		}
		break;
	case 2: // Up
		if (row > 0) {
			target = cur - l.cols;
		}
		break;
	case 3: // Down
		if (row < l.rows - 1) {
			target = cur + l.cols;
			if (target > last) {
				// Row below is not full: its last box, if that is in a lower row of this page
				target = ((last / perPage) == page && ((last % perPage) / l.cols) > row) ? last : -1;
			}
		}
		break;
	}
	if (target > last) {
		target = -1;
	}
	return target;
}

// ---- Scrolling ----

int gridScroll = 0;
int gridScrollTarget = 0;

void gridShowSlot(int slot, bool snap) {
	gridScrollTarget = gridPageOf(slot) * (256 << 8);
	if (snap) {
		gridScroll = gridScrollTarget;
	}
}

void gridVisibleSlots(int *first, int *last) {
	const int perPage = gridPerPage();
	const int pageSize = 256 << 8;
	const int lo = std::max(gridScroll, 0) / pageSize;
	const int hi = (std::max(gridScroll, 0) + pageSize - 1) / pageSize;
	*first = lo * perPage;
	*last = std::min(((hi + 1) * perPage) - 1, 39);
}

bool gridSlotVisible(int slot) {
	int first, last;
	gridVisibleSlots(&first, &last);
	return slot >= first && slot <= last;
}

bool gridCellPos(int slot, int *x, int *y) {
	if (slot < 0) {
		return false;
	}
	const GridLayout &l = gridCurrentLayout();
	const int perPage = l.cols * l.rows;
	const int page = slot / perPage;
	const int inPage = slot % perPage;
	const int pageX = ((page * (256 << 8)) - gridScroll + 128) >> 8;
	const int bx = pageX + l.x0 + ((inPage % l.cols) * l.xstep);
	if (bx + l.box <= 0 || bx >= 256) {
		return false;
	}
	*x = bx;
	*y = l.y0 + ((inPage / l.cols) * l.ystep);
	return true;
}

int gridSlotAt(int px, int py) {
	const GridLayout &l = gridCurrentLayout();
	int first, last;
	gridVisibleSlots(&first, &last);
	for (int slot = first; slot <= last; slot++) {
		int x, y;
		if (gridCellPos(slot, &x, &y) && px >= x && px < x + l.box && py >= y && py < y + l.box) {
			return slot;
		}
	}
	return -1;
}

// ---- Large / small switch ----

// Frames the zoom takes: out for the first half, in for the second. The layout changes in between.
#define TOGGLE_FRAMES 16
#define TOGGLE_MIN_ZOOM 2300	// Smallest the grid gets (20.12), a little over half size

static int toggleFrame = 0;		// 0 = not running
static int toggleCursor = 0;

void gridToggleSize(int cursorSlot) {
	if (toggleFrame == 0) {
		toggleCursor = cursorSlot;
		toggleFrame = 1;
	}
}

bool gridToggling() { return toggleFrame != 0; }

int gridZoom() {
	if (toggleFrame == 0) {
		return 1 << 12;
	}
	const int half = TOGGLE_FRAMES / 2;
	const int span = (1 << 12) - TOGGLE_MIN_ZOOM;
	if (toggleFrame <= half) {
		// Shrinking: slow start, quick finish
		const int u = (toggleFrame << 12) / half;			// 0..1 (20.12)
		return (1 << 12) - ((span * ((u * u) >> 12)) >> 12);
	}
	// Growing: quick start, slow finish
	const int u = ((toggleFrame - half) << 12) / half;			// 0..1
	const int inv = (1 << 12) - u;
	const int eased = (1 << 12) - ((inv * inv) >> 12);
	return TOGGLE_MIN_ZOOM + ((span * eased) >> 12);
}

bool gridButtonHit(int px, int py) {
	return px >= GRID_BUTTON_X && px < GRID_BUTTON_X + GRID_BUTTON_W && py >= GRID_BUTTON_Y &&
	       py < GRID_BUTTON_Y + GRID_BUTTON_H;
}

void gridStep() {
	// Scroll: close a fixed fraction of the remaining distance every frame, which eases out
	const int d = gridScrollTarget - gridScroll;
	if (d != 0) {
		if (d > -96 && d < 96) {
			gridScroll = gridScrollTarget;
		} else {
			int step = (d * 5) / 16;
			if (step == 0) {
				step = (d > 0) ? 1 : -1;
			}
			gridScroll += step;
		}
	}

	if (toggleFrame != 0) {
		if (toggleFrame == TOGGLE_FRAMES / 2) {
			// Smallest point: swap the layout and put the cursor's page in view
			ms().multiRowSmall = !ms().multiRowSmall;
			gridShowSlot(toggleCursor, true);
		}
		toggleFrame++;
		if (toggleFrame > TOGGLE_FRAMES) {
			toggleFrame = 0;
		}
	}
}
