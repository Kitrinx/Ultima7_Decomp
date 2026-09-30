/* Black Gate U7.EXE, overlay segment 226 (file offsets 0x067f90 to 0x0681de, 590 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include <string.h>
#include "oops.h"
#include "dcache.h"

/* each record sits in the buffer as this header followed by its data */
struct RecordHeader {
	int16_t id;
	uint16_t len;
};

void DataCache::init(uint16_t bytes)
{
	buf = 0;
	size = 0;
	if (!allocateBuffer(bytes))
		ReportOutOfFarMemory();
	used = 0;
}

DataCache::~DataCache()
{
	if (buf != 0)
		freeBuffer();
}

char *DataCache::get(int16_t id)
{
	char *p = 0;
	int16_t len;
	char data[256];

	p = find(id);
	if (p == 0) {
		p = read(id, &len, data);
		if (p != 0) {
			if (makeRoom(len))
				p = add(p, id, len);
			else
				p = "";
		}
	}
	return p;
}

char *DataCache::find(int16_t id)
{
	char *p = buf;
	char *end = buf + used;

	for (; p < end; p += ((RecordHeader *)p)->len + sizeof(RecordHeader))
		if (((RecordHeader *)p)->id == id)
			break;
	if (p == end)
		p = 0;
	else
		p += sizeof(RecordHeader);
	return p;
}

uint8_t DataCache::makeRoom(uint16_t len)
{
	uint16_t avail = size - used;
	uint16_t n;

	len += sizeof(RecordHeader);
	while (len > avail) {
		n = discard();
		avail += n;
		if (n == 0)
			break;
	}
	return len <= avail;
}

uint16_t DataCache::discard()
{
	uint16_t n = 0;

	if (used != 0) {
		n = ((RecordHeader *)buf)->len + sizeof(RecordHeader);
		used -= n;
		memcpy(buf, buf + n, used);
	}
	return n;
}

char *DataCache::add(char *data, int16_t id, uint16_t len)
{
	RecordHeader *header = (RecordHeader *)(buf + used);

	header->id = id;
	header->len = len;
	header++;
	memcpy(header, data, len);
	used += len + sizeof(RecordHeader);
	return (char *)header;
}

uint8_t DataCache::allocateBuffer(uint16_t bytes)
{
	char *p = 0;

	if (buf == 0) {
		p = allocate(bytes);
		if (p != 0) {
			buf = p;
			size = bytes;
		}
	}
	return p != 0;
}

void DataCache::freeBuffer()
{
	if (buf != 0)
		release(buf);
	buf = 0;
	size = 0;
	used = 0;
}

char *DataCache::allocate(uint16_t bytes)
{
	char *p = new char[bytes];
	uint16_t i;

	for (i = 0; i < bytes; i++)
		p[i] = '?';
	return p;
}

void DataCache::release(char *p)
{
	delete p;
}
