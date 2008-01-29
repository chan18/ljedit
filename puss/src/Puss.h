// Puss.h
//

#ifndef PUSS_INC_PUSS_H
#define PUSS_INC_PUSS_H

#include "MainWindow.h"

class Puss {
public:
	MainWindow& main_window() { return main_window_; }

	void create();
	void run();

private:
	MainWindow main_window_;
};

#endif//PUSS_INC_PUSS_H

