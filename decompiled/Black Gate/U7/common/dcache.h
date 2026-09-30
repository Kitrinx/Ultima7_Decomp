#ifndef DCACHE_H
#define DCACHE_H

/* Records loaded by id into one near buffer; when it fills, the oldest go first. */
class DataCache {
	char *buf;
	unsigned size;
	unsigned used;
public:
	void init(unsigned bytes);
	~DataCache();
	char *get(int id);
	char *find(int id);
	unsigned char makeRoom(unsigned len);
	unsigned discard();
	char *add(char *data, int id, unsigned len);
	unsigned char allocateBuffer(unsigned bytes);
	void freeBuffer();
	virtual char *read(int id, int *len, char *data) = 0;
	virtual char *allocate(unsigned bytes);
	virtual void release(char *p);
};

#endif
