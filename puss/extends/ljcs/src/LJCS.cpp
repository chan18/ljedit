// LJCS.cpp
// 

#include "LJCS.h"

LJCS::LJCS() : app(0) {}

LJCS::~LJCS() {}

bool LJCS::create(Puss* _app) {
	app = _app;

	icons.create(app);

	return true;
}

void LJCS::destroy() {
	icons.destroy();

	app = 0;
}

