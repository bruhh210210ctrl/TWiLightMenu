#ifndef TWLMENU_GRID_H
#define TWLMENU_GRID_H

// "Multiple rows" view for the 3DS theme.
//
// The bottom screen shows pages of boxes, like the 3DS HOME Menu. There are two sizes, switched
// with the button at the bottom of the screen (or SELECT):
//   large: 3 x 2 boxes per page
//   small: 6 x 3 boxes per page
// Left/Right move one box and carry on onto the next page in the same row (the pages slide
// across with an eased scroll), Up/Down change row. Tapping a box selects it, tapping the
// selected box opens it. It is switched on in the settings ("Multiple rows"); with it off
// nothing here is used and the menu behaves as before.
//
// Everything that moves is stepped once per rendered frame by gridStep(), which the renderer calls,
// so it stays smooth even while the main thread is busy reading the card.

struct GridLayout {
	int cols, rows;
	int x0, y0;		// Top left of the first box on a page
	int xstep, ystep;	// Distance from one box to the next
	int box;		// Size of a box in pixels (the box graphics are 64)
	int iconX, iconY;	// Where the 32x32 icon goes inside a box
};

// Number of icon banks the grid views use (see iconHandler.h); a slot owns bank slot % this
#define GRID_BANKS 36

// Box scale in 20.12 fixed point for a layout (1 << 12 = 64 pixel boxes)
int gridBoxScale(const GridLayout &l);

const GridLayout &gridLayout(bool small);
const GridLayout &gridCurrentLayout();

// True while the grid view is in use
bool gridEnabled();
// True if the small layout is the one being shown (the layout the next frame will draw)
bool gridIsSmall();

// Slots per page, and the page of a slot in the current layout
int gridPerPage();
int gridPageOf(int slot);

// Neighbour in a direction (0 left, 1 right, 2 up, 3 down) of slot cur, for a list whose last
// usable slot is last. Returns -1 at an edge. Right on the last column goes to the next page,
// same row; Left on the first column goes back a page.
int gridNeighbour(int cur, int dir, int last, bool small);

// ---- Scrolling ----
// Horizontal scroll in 1/256 pixel, and the position it is easing towards. A page is 256 pixels wide.
extern int gridScroll;
extern int gridScrollTarget;

// Aims the scroll at the page holding slot. snap = jump there without easing.
void gridShowSlot(int slot, bool snap);

// Slot range currently on screen (first..last), including a page that is only partly scrolled in
void gridVisibleSlots(int *first, int *last);
bool gridSlotVisible(int slot);

// Screen position of a slot's box (top left, includes scroll). Returns false if off screen.
bool gridCellPos(int slot, int *x, int *y);

// Slot under a screen position (a touch), or -1 if the point is not on a box.
int gridSlotAt(int px, int py);

// ---- Large / small switch ----
// Starts the zoom between layouts; cursorSlot is the selected slot, whose page the new layout opens on.
// No effect while one is already running.
void gridToggleSize(int cursorSlot);
bool gridToggling();
// Whole-grid zoom factor (20.12); 1 << 12 when not animating. Zoomed about GRID_ZOOM_CX/CY.
int gridZoom();
#define GRID_ZOOM_CX 128
#define GRID_ZOOM_CY 102

// The button: where it is on screen and whether a touch at (px, py) is on it
#define GRID_BUTTON_X 106
#define GRID_BUTTON_Y 173
#define GRID_BUTTON_W 44
#define GRID_BUTTON_H 17
bool gridButtonHit(int px, int py);

// Advances scrolling and zoom by one frame. Call once per rendered frame.
void gridStep();

#endif
