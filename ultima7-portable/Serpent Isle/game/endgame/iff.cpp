/* Black Gate ENDGAME.EXE: iff.c and chunknam.c, the parts the ending uses, over the platform's
 * files.
 */

#include "u7port.h"
#include "plat.h"
#include "iff.h"

namespace Endgame {

static const char FormId[] = "FORM";

/* Opens the file and reads its outer FORM. */
int16_t IffFile::open(const char *name)
{
	char message[80];

	close();
	handle = plat_file_open(name, PLAT_FILE_READ);
	if (handle < 0) {
		snprintf(message, sizeof message, "Unable to open '%s'.\n", name);
		plat_fatal(message);
	}
	offset = depth = 0;
	chunk.start = chunk.size = chunk.end = 0;
	chunk.id[4] = 0;
	form = 0;
	readHeader();
	if (isId(FormId))
		readId();
	return 1;
}

void IffFile::close()
{
	while (depth > 0)
		leaveForm();
	if (handle >= 0)
		plat_file_close(handle);
	handle = -1;
}

int16_t IffFile::findChunk(const char *id)
{
	int16_t found = 0;

	rewindForm();
	for (;;) {
		readHeader();
		if (isId(id)) {
			found = 1;
			break;
		} else
			skipChunk();
		if (atFormEnd())
			break;
	}
	return found;
}

void IffFile::read(int32_t size, void *buffer)
{
	plat_file_seek(handle, offset, PLAT_SEEK_SET);
	plat_file_read(handle, buffer, size);
	offset += size;
}

int16_t IffFile::readWord()
{
	uint8_t bytes[2] = { 0, 0 };

	read(2, bytes);
	return (int16_t) (bytes[0] | bytes[1] << 8);
}

/* Reads the chunk's size in bytes from where the file is, and moves past the chunk. */
int32_t IffFile::readChunk(void *buffer)
{
	int32_t count;

	plat_file_seek(handle, offset, PLAT_SEEK_SET);
	count = plat_file_read(handle, buffer, chunk.size);
	offset = chunk.end;
	return count;
}

void IffFile::readId()
{
	read(4, chunk.id);
}

/* Sizes are stored high byte first. */
void IffFile::readSize()
{
	uint8_t bytes[4];

	read(4, bytes);
	chunk.size = (int32_t) ((uint32_t) bytes[0] << 24 | bytes[1] << 16 | bytes[2] << 8 | bytes[3]);
}

/* Chunks are padded to an even length. */
void IffFile::readHeader()
{
	chunk.start = offset;
	readId();
	readSize();
	chunk.end = offset + chunk.size + (chunk.size & 1);
}

void IffFile::rewindForm()
{
	if (form == 0)
		offset = 0;
	else
		offset = form->start + FORM_HEADER;
}

void IffFile::enterForm()
{
	IffChunk *inner = new IffChunk;

	inner->prev = form;
	inner->start = chunk.start;
	inner->size = chunk.size;
	strcpy(inner->id, chunk.id);
	inner->end = chunk.end;
	form = inner;
	depth++;
}

void IffFile::leaveForm()
{
	IffChunk *outer = form;

	offset = form->end;
	form = form->prev;
	delete outer;
	depth--;
}

void FindNamedChunk(IffFile *file, const char *id, const char *name)
{
	char text[9];

	file->findChunk(id);
	while (file->isId(id)) {
		file->read(8, text);
		text[8] = 0;
		if (strcmp(text, name)) {
			file->skipChunk();
			file->readHeader();
		} else
			break;
	}
}

}
