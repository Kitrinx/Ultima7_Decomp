#ifndef SHAPEDRAW_H
#define SHAPEDRAW_H

struct View;

/* What a shape pixel does to the view pixel under it. */
enum ShapePixelMode {
	SHAPE_COPY,		/* replace it */
	SHAPE_TRANSLATE,	/* recolor it through a 256-byte table */
	SHAPE_BLEND		/* mix it through a translucency table */
};

/* Draw one frame of a run-length shape at x, y, clipped to the view's clip box.
 * Flipped swaps x and y, so each span runs down a column.
 * The table is a linear address, used by translate and blend.
 */
void DrawShapeFrame(struct View *view, int16_t x, int16_t y, int32_t shape, int16_t frameNum,
	bool flipped, enum ShapePixelMode mode, int32_t table);

#endif
