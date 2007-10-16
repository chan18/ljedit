// LJCSPluginUtils.h
// 

#ifndef LJED_INC_LJCSPLUGINUTILS_H
#define LJED_INC_LJCSPLUGINUTILS_H

#include "gtkenv.h"

#include "ljcs/ljcs.h"

class LJEditorDocIter : public IDocIter {
public:
    LJEditorDocIter(Gtk::TextBuffer::iterator& it)
        : IDocIter( (char)it.get_char() )
        , it_(it) {}

protected:
    virtual char do_prev() {
        if( it_.backward_char() )
            return it_.get_char();
        return '\0';
    }

    virtual char do_next() {
        if( it_.forward_char() )
            return it_.get_char();
        return '\0';
    }

private:
    Gtk::TextBuffer::iterator& it_;
};

bool is_in_comment(Gtk::TextBuffer::iterator& it);

bool find_keys( StrVector& keys
    , Gtk::TextBuffer::iterator& it
    , Gtk::TextBuffer::iterator& end
    , cpp::File* file
	, bool find_startswith);

size_t find_best_matched_index(cpp::Elements& elems);

cpp::Element* find_best_matched_element(cpp::ElementSet& eset);

bool check_cpp_files(const std::string& filepath);

#endif//LJED_INC_LJCSPLUGINUTILS_H

