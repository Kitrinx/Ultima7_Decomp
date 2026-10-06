/* Serpent Isle SI.EXE, resident segment 83 (file offsets 0x03323a to 0x033583, 841 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -b- rebuilds it byte for byte as C++.
 * Option field name inferred from the observed token and field access.
 */

#include <ctype.h>
#include "chkfile.h"
#include "vstring.h"

/* The sound setup: a music device with its port, and a digital device. */
struct SoundConfig {
	char device;            /* 's' Sound Blaster, 'a' Adlib, 'r' Roland, 'p' none */
	int musicPort;
	int speechPort;
	int irq;
	int dma;
	char speechEnabled;
	char optionEnabled;
	SoundConfig();
	void reset();
	void readConfig(char *name);
	void close();
	int isRoland();
	int isAdlib();
	int isSoundBlaster();
	int hasMusic();
};

int ParseHexSuffix(char *s);

SoundConfig::SoundConfig()
{
	reset();
}

void SoundConfig::reset()
{
	device = 'p';
	speechPort = 0;
	irq = 0;
	musicPort = 0;
	speechEnabled = 0;
	optionEnabled = 0;
}

/* The first line names the music device and its port; the second gives the speech port, IRQ and DMA. */
void SoundConfig::readConfig(char *name)
{
	DataFile f;
	long n;
	char *save;
	char buf[81];
	char *p;

	reset();
	if (!(unsigned char)f.open(name, 1))
		return;
	n = f.readUntil('\r', buf, 80L);
	if (n == 0)
		return;
	buf[n] = 0;
	p = NextToken(buf, "\r\n ", &save);
	switch (*p) {
	case 'S':
	case 's':
		device = 's';
		musicPort = 0x388;
		p = NextToken(0, "\r\n ", &save);
		if (p != 0)
			musicPort = ParseHexSuffix(p);
		break;
	case 'A':
	case 'a':
		device = 'a';
		musicPort = 0x388;
		p = NextToken(0, "\r\n ", &save);
		if (p != 0)
			musicPort = ParseHexSuffix(p);
		break;
	case 'R':
	case 'r':
		musicPort = 0;
		device = 'r';
		p = NextToken(0, "\r\n ", &save);
		if (p != 0)
			musicPort = ParseHexSuffix(p);
		break;
	default:
		device = 'p';
	}
	n = f.readUntil('\r', buf, 80L);
	if (n == 0)
		return;
	buf[n] = 0;
	p = NextToken(buf, "\r\n ", &save);
	if (p != 0) {
		speechPort = ParseHexSuffix(p);
		p = NextToken(0, "\r\n ", &save);
		if (p != 0) {
			irq = ParseHexSuffix(p);
			p = NextToken(0, "\r\n ", &save);
			if (p != 0) {
				dma = *p - '0';
				speechEnabled = 1;
				p = NextToken(0, "\r\n ", &save);
				if (*p == 'o')
					optionEnabled = 1;
			}
		}
	}
}

void SoundConfig::close()
{
}

int SoundConfig::isRoland()
{
	return device == 'r';
}

int SoundConfig::isAdlib()
{
	return device == 'a';
}

int SoundConfig::isSoundBlaster()
{
	return device == 's' || speechEnabled != 0;
}

int SoundConfig::hasMusic()
{
	return device != 'p';
}

/* The value of the hexadecimal number that ends a string. */
int ParseHexSuffix(char *s)
{
	char *p = s;
	int value = 0;
	int shift = 0;
	int c;

	while (*p)
		p++;
	while (--p >= s) {
		c = (unsigned char) *p;
		if (c == 0 || c == ' ')
			break;
		c = toupper(c);
		if (c < '0' || c > '9' && c < 'A' || c > 'F') {
			value = 0;
			continue;
		}
		c -= '0';
		if (c > 9) {
			c -= 'A' - '9' - 1;
			if (c > 15) {
				c -= 80;
				if (c > 15)
					break;
			}
		}
		value += c << shift;
		shift += 4;
	}
	return value;
}
