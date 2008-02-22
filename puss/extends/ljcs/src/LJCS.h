// LJCS.h
// 

#ifndef PUSS_EXTEND_INC_LJCS_H
#define PUSS_EXTEND_INC_LJCS_H

#include "IPuss.h"
#include "Environ.h"
#include "Icons.h"

class LJCS {
public:
	Puss*		app;

	Environ		env;
	Icons		icons;

public:
	LJCS();
	~LJCS();

    bool create(Puss* _app);
    void destroy();
};

#endif//PUSS_EXTEND_INC_LJCS_H

