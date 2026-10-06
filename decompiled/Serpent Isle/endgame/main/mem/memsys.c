/* Serpent Isle ENDGAME.EXE, resident segment 28 (file offsets 0x00c9fb to 0x00ceb3, 1208 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include "memsys.h"

/* Codes FatalCode reports. */
#define ERROR_ALREADY_ATTACHED  0x400
#define ERROR_NO_HANDLER        0x401
#define ERROR_BAD_TYPE          0x403
#define ERROR_NOT_ATTACHED      0x404

MemoryHandler *MemoryManager::handlers[MEMORY_TYPES] = { 0 };
unsigned char MemoryManager::instanced = 0;
MemoryManager Memory;

void MemoryManager::attach(MemoryHandler *handler, int)
{
	int type = handler->type();

	if (handlers[type]) {
		delete handler;
		handler = 0;
		FatalCode(ERROR_ALREADY_ATTACHED);
	}
	if (!handler)
		FatalCode(ERROR_NO_HANDLER);
	handlers[type] = handler;
}

void MemoryManager::detach(unsigned char type)
{
	validate(type);
	delete handlers[type];
	handlers[type] = 0;
}

void far *MemoryManager::allocate(long size, unsigned char type, unsigned char flags, unsigned char fail)
{
	void far *block;

	validate(type);
	block = handlers[type]->allocateChecked(size, flags, fail);
	return block;
}

void far *MemoryManager::allocate(long size, unsigned char type, unsigned char flags, unsigned char fail,
	int code)
{
	void far *block;

	validate(type);
	block = handlers[type]->allocateChecked(size, flags, fail, code);
	return block;
}

void far *MemoryManager::allocate(long size, unsigned char type, unsigned char flags, unsigned char fail,
	char *what)
{
	void far *block;

	validate(type);
	block = handlers[type]->allocateChecked(size, flags, fail, what);
	return block;
}

void MemoryManager::release(void far **block, unsigned char type, int)
{
	validate(type);
	handlers[type]->release(block);
}

void MemoryManager::copy(void far *to, void far *from, long size, unsigned char type)
{
	validate(type);
	handlers[type]->copy(to, from, size);
}

void far *MemoryManager::lock(void far *block, unsigned char type, int)
{
	validate(type);
	return handlers[type]->lock(block);
}

long MemoryManager::available(unsigned char type, int)
{
	validate(type);
	return handlers[type]->available();
}

int MemoryManager::version(unsigned char type)
{
	validate(type);
	if (type != EMS_MEMORY)
		return 0;
	return handlers[type]->version();
}

int MemoryManager::checkHeap(unsigned char type)
{
	validate(type);
	return handlers[type]->checkHeap();
}

unsigned char MemoryManager::isAttached(unsigned char type, int)
{
	if (type > MEMORY_TYPES)
		FatalCode(ERROR_BAD_TYPE);
	else if (handlers[type])
		return 1;
	return 0;
}

void MemoryManager::report()
{
	int type;

	for (type = 0; type < MEMORY_TYPES; type++) {
		if (handlers[type])
			ShowMessage("handler %d attached with %ld bytes available \n", type, available(type));
		else
			ShowMessage("handler %d off\n", type);
	}
	ShowMessage("\n");
}

void MemoryManager::validate(unsigned char type)
{
	if (!handlers[type])
		FatalCode(ERROR_NOT_ATTACHED);
	else if (type > MEMORY_TYPES)
		FatalCode(ERROR_BAD_TYPE);
}

MemoryManager::MemoryManager()
{
	int type;

	if (instanced)
		FatalMessage("Illegal re-instantiation of MemoryManger\n");
	instanced = 1;
	for (type = 0; type < MEMORY_TYPES; type++)
		handlers[type] = 0;
	attach(new NearMemory);
	attach(new FarMemory);
}

MemoryManager::~MemoryManager()
{
	int type;

	instanced = 0;
	for (type = 0; type < MEMORY_TYPES; type++)
		if (handlers[type])
			delete handlers[type];
}
