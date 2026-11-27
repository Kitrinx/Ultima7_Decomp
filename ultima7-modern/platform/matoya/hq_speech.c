#include <stdlib.h>
#include <string.h>

#include "backend.h"

static uint16_t le16(const uint8_t *p)
{
	return p[0] | (uint16_t) p[1] << 8;
}

static uint32_t le32(const uint8_t *p)
{
	return le16(p) | (uint32_t) le16(p + 2) << 16;
}

static bool read_at(int16_t file, uint32_t offset, void *buffer, int32_t size)
{
	return plat_file_seek(file, (int32_t) offset, PLAT_SEEK_SET) == (int32_t) offset &&
		plat_file_read(file, buffer, size) == size;
}

static bool flex_entry(int16_t file, int16_t entry, uint32_t *offset, uint32_t *size)
{
	int32_t length = plat_file_length(file);
	uint8_t fields[8];
	uint32_t count;

	if (length < 128 || entry < 0 || !read_at(file, 80, fields, sizeof fields) ||
		le32(fields) != UINT32_C(0xffff1a00))
		return false;
	count = le32(fields + 4);
	if (count > (uint32_t) (length - 128) / 8 || (uint32_t) entry >= count ||
		!read_at(file, 128 + (uint32_t) entry * 8, fields, sizeof fields))
		return false;
	*offset = le32(fields);
	*size = le32(fields + 4);
	return *offset >= 128 + count * 8 && *offset <= (uint32_t) length &&
		*size >= 12 && *size <= (uint32_t) length - *offset;
}

static bool wave_format(int16_t file, uint32_t offset, uint32_t size)
{
	uint8_t fmt[16];

	return size >= sizeof fmt && read_at(file, offset, fmt, sizeof fmt) &&
		le16(fmt) == 1 && le16(fmt + 2) == 2 && le32(fmt + 4) == U7_AUDIO_RATE &&
		le32(fmt + 8) == U7_AUDIO_RATE * 4 && le16(fmt + 12) == 4 && le16(fmt + 14) == 16;
}

static uint8_t *read_wave(int16_t file, uint32_t offset, uint32_t size, uint32_t *frames)
{
	uint8_t header[12];
	uint32_t end, pos, data_offset = 0, data_size = 0;
	bool have_format = false;
	uint8_t *data;

	if (!read_at(file, offset, header, sizeof header) || memcmp(header, "RIFF", 4) != 0 ||
		memcmp(header + 8, "WAVE", 4) != 0 || le32(header + 4) < 4 ||
		le32(header + 4) > size - 8)
		return NULL;
	end = offset + 8 + le32(header + 4);
	pos = offset + sizeof header;
	while (end - pos >= 8 && (!have_format || data_size == 0)) {
		uint32_t chunk_size, padded;

		if (!read_at(file, pos, header, 8))
			return NULL;
		chunk_size = le32(header + 4);
		pos += 8;
		if (chunk_size > end - pos)
			return NULL;
		padded = chunk_size + (chunk_size & 1);
		if (padded > end - pos)
			return NULL;
		if (memcmp(header, "fmt ", 4) == 0) {
			if (!wave_format(file, pos, chunk_size))
				return NULL;
			have_format = true;
		} else if (memcmp(header, "data", 4) == 0) {
			data_offset = pos;
			data_size = chunk_size;
		}
		pos += padded;
	}
	if (!have_format || data_size == 0 || data_size % 4 != 0)
		return NULL;
	data = malloc(data_size);
	if (data != NULL && !read_at(file, data_offset, data, (int32_t) data_size)) {
		free(data);
		return NULL;
	}
	*frames = data_size / 4;
	return data;
}

bool plat_pcm_play_hq(const char *archive, int16_t entry)
{
	int16_t file = plat_file_open(archive, PLAT_FILE_READ);
	uint32_t offset, size, frames;
	uint8_t *data;

	if (file < 0)
		return false;
	data = flex_entry(file, entry, &offset, &size) ? read_wave(file, offset, size, &frames) : NULL;
	plat_file_close(file);
	if (data == NULL)
		return false;
	audio_play_pcm16(data, frames);
	return true;
}
