// LJEditorImpl.h
//

#ifndef LJED_INC_LJEDITORIMPL_H
#define LJED_INC_LJEDITORIMPL_H

#include "LJEditor.h"

#include "MainWindowImpl.h"


class LJEditorImpl : public LJEditor {
public:
    static LJEditorImpl& self() {
        static LJEditorImpl me;
        return me;
    }

public:
    virtual MainWindow& main_window()
        { return main_window_; }

public:
    bool create();
    void run();
    void destroy();

private:
    LJEditorImpl() {}
    ~LJEditorImpl() {}

    LJEditorImpl(const LJEditorImpl&);
    LJEditorImpl& operator = (const LJEditorImpl&);

private:
    MainWindowImpl main_window_;
};

#endif//LJED_INC_LJEDITORIMPL_H

