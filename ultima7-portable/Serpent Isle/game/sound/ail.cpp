/* The Miles AIL 2.14 driver interface: sixteen driver handles, each standing for a loaded .ADV
 * image, served on the platform's sound tick once initialized.
 */

#include "u7port.h"
#include "plat.h"
#include "aildrv.h"

#define MAX_DRIVERS 16

struct DriverSlot {
	int16_t kind;
	uint8_t active;     /* initialized and served */
};

static DriverSlot Drivers[MAX_DRIVERS];
static uint8_t TickInstalled;

int16_t AilDriverKind(HDRIVER driver)
{
	if (driver < 0 || driver >= MAX_DRIVERS)
		return AIL_KIND_NONE;
	return Drivers[driver].kind;
}

/* The device an .ADV image is for, from the device names it lists near its start. */
static int16_t ImageKind(const uint8_t *image)
{
	if (image == 0 || memcmp(image + 2, "Copyright", 9) != 0)
		return AIL_KIND_NONE;
	for (uint16_t i = 2; i < 512; i++) {
		if (memcmp(image + i, "Roland MT-32", 12) == 0)
			return AIL_KIND_MT32;
		if (memcmp(image + i, "Digital Sound", 13) == 0)
			return AIL_KIND_DIGITAL;
		if (memcmp(image + i, "Ad Lib", 6) == 0)
			return AIL_KIND_NONE;
	}
	return AIL_KIND_NONE;
}

/* The drivers' interrupt-time work: the MT-32 driver at 120 Hz, the digital one to keep the
 * output fed. */
static void SoundTick(void)
{
	for (int16_t i = 0; i < MAX_DRIVERS; i++) {
		if (!Drivers[i].active)
			continue;
		if (Drivers[i].kind == AIL_KIND_MT32) {
			Mt32Serve();
			Mt32Serve();
		} else if (Drivers[i].kind == AIL_KIND_DIGITAL)
			DigitalServe();
	}
}

static void UpdateTick(void)
{
	uint8_t any = 0;

	for (int16_t i = 0; i < MAX_DRIVERS; i++)
		any |= Drivers[i].active;
	if (any != TickInstalled) {
		plat_sound_tick_set(any ? SoundTick : 0);
		TickInstalled = any;
	}
}

static void ShutdownDriver(HDRIVER driver, const char *signOff)
{
	if (AilDriverKind(driver) == AIL_KIND_NONE || !Drivers[driver].active)
		return;
	Drivers[driver].active = 0;
	UpdateTick();
	if (Drivers[driver].kind == AIL_KIND_MT32)
		Mt32Shutdown(signOff);
	else
		DigitalShutdown();
}

void AIL_startup(void)
{
	plat_sound_lock();
	memset(Drivers, 0, sizeof Drivers);
	UpdateTick();
	plat_sound_unlock();
}

void AIL_shutdown(const char *signoff_msg)
{
	plat_sound_lock();
	for (int16_t i = 0; i < MAX_DRIVERS; i++)
		ShutdownDriver(i, signoff_msg);
	plat_sound_unlock();
}

/* -1 when the image is no driver the port has; there is no Ad Lib driver. */
HDRIVER AIL_register_driver(void *driver_base_addr)
{
	int16_t kind = ImageKind((const uint8_t *) driver_base_addr);
	HDRIVER driver;

	if (kind == AIL_KIND_NONE)
		return -1;
	plat_sound_lock();
	for (driver = 0; driver < MAX_DRIVERS && Drivers[driver].kind != AIL_KIND_NONE; driver++)
		;
	if (driver == MAX_DRIVERS)
		driver = -1;
	else
		Drivers[driver].kind = kind;
	plat_sound_unlock();
	return driver;
}

void AIL_release_driver_handle(HDRIVER driver)
{
	if (driver < 0 || driver >= MAX_DRIVERS)
		return;
	plat_sound_lock();
	Drivers[driver].kind = AIL_KIND_NONE;
	plat_sound_unlock();
}

drvr_desc *AIL_describe_driver(HDRIVER driver)
{
	switch (AilDriverKind(driver)) {
	case AIL_KIND_MT32:
		return Mt32Describe();
	case AIL_KIND_DIGITAL:
		return DigitalDescribe();
	}
	return 0;
}

/* The device is the platform's: nonzero when it can play. */
uint16_t AIL_detect_device(HDRIVER driver, uint16_t IO_addr, uint16_t IRQ, uint16_t DMA,
	uint16_t DRQ)
{
	switch (AilDriverKind(driver)) {
	case AIL_KIND_MT32:
		return Mt32Detect();
	case AIL_KIND_DIGITAL:
		return DigitalDetect();
	}
	return 0;
}

void AIL_init_driver(HDRIVER driver, uint16_t IO_addr, uint16_t IRQ, uint16_t DMA, uint16_t DRQ)
{
	int16_t kind = AilDriverKind(driver);

	if (kind == AIL_KIND_NONE)
		return;
	plat_sound_lock();
	if (kind == AIL_KIND_MT32)
		Mt32Init();
	else
		DigitalInit();
	Drivers[driver].active = 1;
	UpdateTick();
	plat_sound_unlock();
}

void AIL_shutdown_driver(HDRIVER driver, const char *signoff_msg)
{
	plat_sound_lock();
	ShutdownDriver(driver, signoff_msg);
	plat_sound_unlock();
}

extern "C" void ResetAilGlobals(void)
{
	memset(Drivers, 0, sizeof Drivers);
	TickInstalled = 0;
}
