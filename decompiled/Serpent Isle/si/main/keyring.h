#ifndef KEYRING_H
#define KEYRING_H

class Keyring {
public:
	char unusedFlag;
	char keys[32];

	void dump();
	Keyring();
	void add(unsigned char key);
	unsigned char contains(unsigned char key);
	unsigned char acceptsKey(unsigned char frame, unsigned char quality);
	void encode();
	void decode();
};

extern Keyring KeyRing;

#endif
