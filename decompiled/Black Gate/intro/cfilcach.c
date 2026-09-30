/* Black Gate INTRO.EXE, one module of resident segment 30 (file offsets 0x00ffd7 to 0x01012f, 344 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "cflxbuf.h"

FileSpeechCache::FileSpeechCache()
{
	streaming = 1;
}

FileSpeechCache::FileSpeechCache(char streamed)
{
	streaming = streamed;
}

FileSpeechCache::~FileSpeechCache()
{
	if (isOpen())
		close();
}

unsigned char FileSpeechCache::open(char *name)
{
	return file.open(name, FILE_OPEN) == 1;
}

void FileSpeechCache::close()
{
	file.close();
}

long FileSpeechCache::read(void far *to, unsigned long n)
{
	return file.read(to, n);
}

char FileSpeechCache::isOpen()
{
	return file.handle != -1;
}

unsigned long FileSpeechCache::fileSize()
{
	return file.getLength();
}
