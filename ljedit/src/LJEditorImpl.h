// LJEditorImpl.h
//

#ifndef LJED_INC_LJEDITORIMPL_H
#define LJED_INC_LJEDITORIMPL_H

#include "LJEditor.h"

#include "LJEditorUtilsImpl.h"
#include "MainWindowImpl.h"

class LJEditorImpl : public LJEditor {
public:
    static LJEditorImpl& self() {
        static LJEditorImpl me;
        return me;
    }

public:
    virtual MainWindow& main_window() { return main_window_; }

	virtual LJEditorUtils& utils()    { return LJEditorUtilsImpl::self(); }

public:
    bool create(const std::string& path);
	void add_open_file(const char* filename);
    void run();
    void destroy();

private:
	LJEditorImpl();
    ~LJEditorImpl();

    LJEditorImpl(const LJEditorImpl&);
    LJEditorImpl& operator = (const LJEditorImpl&);

private:
    MainWindowImpl		main_window_;
};

#endif//LJED_INC_LJEDITORIMPL_H

