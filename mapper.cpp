#include "nes.h"
#include "mapper.h"
#include "mapper0.h"
#include "mapper1.h"

Mapper *Mapper::makeMapper(NES* nes, uint8_t mapper) {
	Mapper* ret = nullptr;
	if (mapper == 0) {
		ret = new Mapper0(nes);
	}
	if (mapper == 1) {
		ret = new Mapper1(nes);
	}
	return ret;
}
