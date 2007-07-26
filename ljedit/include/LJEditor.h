// LJEditor.h
//

#ifndef LJED_INC_LJEDITOR_H
#define LJED_INC_LJEDITOR_H

#include "MainWindow.h"

class LJEditor {
public:
    virtual MainWindow& main_window() = 0;
};

#endif//LJED_INC_LJEDITOR_H

