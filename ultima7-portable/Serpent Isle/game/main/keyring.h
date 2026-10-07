#ifndef KEYRING_H
#define KEYRING_H

class Keyring {
public:
	int8_t unusedFlag;
	char keys[32];

	void dump();
	Keyring();
	void add(uint8_t key);
	uint8_t contains(uint8_t key);
	uint8_t acceptsKey(uint8_t frame, uint8_t quality);
	void encode();
	void decode();
};

extern Keyring KeyRing;

#endif
