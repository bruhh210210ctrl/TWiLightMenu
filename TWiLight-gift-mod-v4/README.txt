TWiLight Menu++ mod, version 4 (3DS theme): gift icons + "Multiple rows" setting
===============================================================================

New in this version
- A new setting, "Multiple rows", in the TWiLight settings (GUI page, just under
  "Top screen photo"). Off by default. When it is ON (and you use the 3DS theme),
  the bottom screen shows a grid of 3 boxes x 2 rows instead of one scrolling row.
    * D-pad: Left/Right step through apps, Up/Down jump a row. The grid scrolls one row at a time.
    * Touch: tap a box to select it, tap the selected box to open it.
    * L / R still flip between pages of 40 apps.
    * The app's name is shown under the grid (there is no title bubble in this view).
    * Gifts, the spinning gift on the top screen, folders and box art all work in the grid.
- Turn the setting OFF and everything goes back to the single row, exactly as before.
- Moving apps with Up is only available with the setting OFF (Up/Down move between rows instead).

Everything from version 3 is included (gifts stay gifts when moved, slower spin, livelier bow).

How to install (same as before, nothing to install on your computer)
1. Unzip this folder.
2. On your fork on GitHub: Add file -> Upload files, and drag in these folders together:
   romsel_dsimenutheme, settings, universal.  Click "Commit changes".
   (Files with the same name are replaced automatically.)
3. Open the Actions tab. A build starts by itself (about 6 minutes). Wait for the green check.
4. Open that run, scroll to "Artifacts" at the bottom, download "build", unzip it,
   open TWiLightMenu-Flashcard.7z.
5. Copy these TWO files onto your card, replacing the old ones (keep backups first):
      _nds/TWiLightMenu/dsimenu.srldr     (the menu)
      _nds/TWiLightMenu/settings.srldr    (the settings, which now has the new toggle)
6. Open TWiLight settings, go to the GUI page, set "Multiple rows" to On.

If the build shows a red X, open the failed step, copy the red error lines and send them to me.

Notes
- This is the first version of the grid. Tell me what to change: columns/rows, spacing, where the name goes.
