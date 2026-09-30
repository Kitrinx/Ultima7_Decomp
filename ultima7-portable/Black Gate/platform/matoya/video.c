/* The 320x200 indexed screen, drawn at 4:3 in the window.
 *
 * The game thread converts each presented frame to RGBA with the palette; the app thread
 * draws the newest converted frame every display refresh.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "backend.h"

#define WIDTH PLAT_SCREEN_WIDTH
#define HEIGHT PLAT_SCREEN_HEIGHT
#define PIXELS (WIDTH * HEIGHT)
/* 320x200 shown with 320x240 proportions. */
#define ASPECT_HEIGHT 240
#define RETRACE_HZ 70.086
/* yield shows the frame again no more often than this. */
#define REFRESH_US 8000

/* Game thread. */
static uint8_t palette[256][4];
static bool palette_dirty;
static const uint8_t *last_pixels;
static int64_t last_present_us;

/* Handed to the app thread under frame_lock. */
static MTY_Mutex *frame_lock;
static uint8_t shared_rgba[PIXELS][4];
static bool shared_new;

/* App thread. */
static uint8_t draw_rgba[PIXELS][4];

void video_init(void)
{
	frame_lock = MTY_MutexCreate();
	for (int i = 0; i < 256; i++)
		palette[i][3] = 0xFF;
	for (int i = 0; i < PIXELS; i++)
		draw_rgba[i][3] = 0xFF;
}

void plat_video_set_palette(int16_t first, int16_t count, const uint8_t *rgb)
{
	for (int16_t i = 0; i < count; i++) {
		int index = first + i;

		if (index < 0 || index > 255)
			continue;
		for (int c = 0; c < 3; c++) {
			uint8_t v = rgb[3 * i + c] & 0x3F;

			palette[index][c] = (uint8_t) (v << 2 | v >> 4);
		}
	}
	palette_dirty = true;
}

static void convert(const uint8_t *pixels)
{
	MTY_MutexLock(frame_lock);
	for (int i = 0; i < PIXELS; i++)
		memcpy(shared_rgba[i], palette[pixels[i]], 4);
	shared_new = true;
	MTY_MutexUnlock(frame_lock);
	palette_dirty = false;
	last_present_us = backend_elapsed_us();
}

void plat_video_present(const uint8_t *pixels)
{
	last_pixels = pixels;
	convert(pixels);
}

void video_refresh(bool force)
{
	if (last_pixels == NULL)
		return;
	if (force || palette_dirty || backend_elapsed_us() - last_present_us >= REFRESH_US)
		convert(last_pixels);
}

void plat_video_wait_retrace(void)
{
	double period_us = 1000000.0 / RETRACE_HZ;
	int64_t now, next;

	plat_pump();
	/* On VGA a palette write showed at once; fades rely on that. */
	if (palette_dirty)
		video_refresh(true);
	now = backend_elapsed_us();
	next = (int64_t) ((floor((double) now / period_us) + 1.0) * period_us);
	/* Rounding up lands past the boundary, so the next wait finds a later one. */
	MTY_Sleep((uint32_t) ((next - now + 999) / 1000));
}

static uint32_t integer_scale(MTY_Size view)
{
	uint32_t sx = view.w / WIDTH;
	uint32_t sy = view.h / ASPECT_HEIGHT;

	return sx < sy ? sx : sy;
}

float video_scale(MTY_Size view)
{
	return (float) integer_scale(view);
}

void video_view_rect(MTY_Size view, int32_t *x, int32_t *y, int32_t *w, int32_t *h)
{
	uint32_t s = integer_scale(view);

	if (s > 0) {
		*w = (int32_t) (WIDTH * s);
		*h = (int32_t) (ASPECT_HEIGHT * s);
	} else if ((uint64_t) view.w * ASPECT_HEIGHT >= (uint64_t) view.h * WIDTH) {
		*h = (int32_t) view.h;
		*w = (int32_t) ((uint64_t) view.h * WIDTH / ASPECT_HEIGHT);
	} else {
		*w = (int32_t) view.w;
		*h = (int32_t) ((uint64_t) view.w * ASPECT_HEIGHT / WIDTH);
	}
	*x = ((int32_t) view.w - *w) / 2;
	*y = ((int32_t) view.h - *h) / 2;
}

void video_draw(MTY_App *app, MTY_Window window)
{
	MTY_Size view = MTY_WindowGetSize(app, window);
	MTY_RenderDesc desc = {
		.format = MTY_COLOR_FORMAT_RGBA,
		.filter = MTY_FILTER_NEAREST,
		.imageWidth = WIDTH,
		.imageHeight = HEIGHT,
		.cropWidth = WIDTH,
		.cropHeight = HEIGHT,
		.aspectRatio = (float) WIDTH / ASPECT_HEIGHT,
		.scale = video_scale(view),
	};

	MTY_MutexLock(frame_lock);
	if (shared_new) {
		memcpy(draw_rgba, shared_rgba, sizeof draw_rgba);
		shared_new = false;
	}
	MTY_MutexUnlock(frame_lock);

	MTY_WindowDrawQuad(app, window, draw_rgba, &desc);
	MTY_WindowPresent(app, window);
}

int16_t plat_cursor_is_hardware(void)
{
	return 0;
}

void plat_cursor_set_shape(const uint8_t *pixels, int16_t width, int16_t height,
	int16_t hot_x, int16_t hot_y)
{
	(void) pixels;
	(void) width;
	(void) height;
	(void) hot_x;
	(void) hot_y;
}

void plat_cursor_show(int16_t visible)
{
	(void) visible;
}

bool video_write_shot(const char *path)
{
	FILE *f = fopen(path, "wb");

	if (f == NULL)
		return false;
	fprintf(f, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
	MTY_MutexLock(frame_lock);
	for (int i = 0; i < PIXELS; i++)
		fwrite(shared_rgba[i], 1, 3, f);
	MTY_MutexUnlock(frame_lock);
	fclose(f);
	return true;
}
