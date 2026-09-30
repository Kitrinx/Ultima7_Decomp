#include "u7/sha256.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void check_digest(const char *message, const char *expected) {
	char hex[65];
	u7_sha256_hex(message, strlen(message), hex);
	if (strcmp(hex, expected) != 0) {
		printf("FAIL sha256(\"%s\") = %s\n", message, hex);
		failures += 1;
	}
}

int main(void) {
	check_digest("", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
	check_digest("abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
	/* 56 bytes: the length no longer fits the last block. */
	check_digest("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
			"248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
	if (failures == 0) {
		printf("PASS test_sha256\n");
	}
	return failures == 0 ? 0 : 1;
}
