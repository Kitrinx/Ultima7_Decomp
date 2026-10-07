#ifndef DCACHE_H
#define DCACHE_H

/* Records loaded by id into one near buffer; when it fills, the oldest go first. */
class DataCache {
	char *buf;
	uint16_t size;
	uint16_t used;
public:
	void init(uint16_t bytes);
	~DataCache();
	char *get(int16_t id);
	char *find(int16_t id);
	uint8_t makeRoom(uint16_t len);
	uint16_t discard();
	char *add(char *data, int16_t id, uint16_t len);
	uint8_t allocateBuffer(uint16_t bytes);
	void freeBuffer();
	virtual char *read(int16_t id, int16_t *len, char *data) = 0;
	virtual char *allocate(uint16_t bytes);
	virtual void release(char *p);
};

#endif
