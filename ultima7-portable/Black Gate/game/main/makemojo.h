#ifndef MAKEMOJO_H
#define MAKEMOJO_H

void CheckMojoBounds(uint32_t bound, uint32_t index);
uint8_t MakeMojo(int32_t count, int32_t size, int32_t *mem, int32_t *bound);
uint8_t ReadMojo(int16_t fd, int32_t pos, int32_t first, int32_t last, int32_t bound, int32_t mem, int32_t size);
void WriteMojo(int16_t fd, int32_t pos, int32_t first, int32_t last, int32_t bound, int32_t mem, int32_t size);

#endif
