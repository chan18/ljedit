// LJEditor.h
//

#ifndef LJED_INC_LJEDITOR_H
#define LJED_INC_LJEDITOR_H

#include "MainWindow.h"
#include "ConfigManager.h"
#include "LJEditorUtils.h"

class LJEditor {
public:
    virtual MainWindow&     main_window() = 0;

	virtual ConfigManager&	config_manager() = 0;

	virtual LJEditorUtils&  utils()       = 0;
};

#endif//LJED_INC_LJEDITOR_H

