/* Black Gate INTRO.EXE, cfilcach.c: speech read from a plain file. The intro's older speech cache
 * took its buffer from the far heap, where U7's takes extended memory; the buffer calls keep that.
 */

#include "u7port.h"
#include "dosio.h"
#include "memapi.h"
#include "chkfile.h"
#include "cflxbuf.h"
#include "cfilcach.h"

namespace Intro {

FileSpeechCache::FileSpeechCache()
{
	streaming = 1;
}

FileSpeechCache::FileSpeechCache(int8_t streamed)
{
	streaming = streamed;
}

FileSpeechCache::~FileSpeechCache()
{
	if (isOpen())
		close();
	freeBuffer();
}

uint8_t FileSpeechCache::allocateBuffer(uint32_t n)
{
	buffer = PointerToLinear(AllocateFarHeap((int32_t) n, 0));
	return buffer != 0;
}

void FileSpeechCache::freeBuffer()
{
	if (buffer != 0)
		FreeFarHeap(LinearToPointer(buffer));
	buffer = 0;
}

uint8_t FileSpeechCache::open(char *name)
{
	return file.open(name, FILE_OPEN) == 1;
}

void FileSpeechCache::close()
{
	file.close();
}

/* U7's speech cache passes its buffer's linear address. */
int32_t FileSpeechCache::read(void *to, uint32_t n)
{
	return file.read(LinearToPointer((int32_t) (intptr_t) to), (int32_t) n);
}

int8_t FileSpeechCache::isOpen()
{
	return file.handle != -1;
}

uint32_t FileSpeechCache::fileSize()
{
	return (uint32_t) file.getLength();
}

}
