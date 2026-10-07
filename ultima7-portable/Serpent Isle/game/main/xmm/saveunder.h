#ifndef SAVEUNDER_H
#define SAVEUNDER_H

struct View;

/* Copy between a view and a save block for the box left..right, top..bottom, corners included.
 * The block holds the whole box; only the part inside the view's clip box is copied.
 * Restore copies the block onto the view, otherwise the view goes into the block.
 */
void CopySaveBlock(struct View *view, int32_t buffer, int16_t left, int16_t top, int16_t right,
	int16_t bottom, bool restore);

#endif
