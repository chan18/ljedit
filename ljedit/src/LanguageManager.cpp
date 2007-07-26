// LanguageManager.cpp
// 

#include "LanguageManager.h"
#include "RuntimeException.h"

#include <gtksourceviewmm/init.h>


Glib::RefPtr<gtksourceview::SourceLanguagesManager> create_source_language_manager() {
    gtksourceview::init();

    return gtksourceview::SourceLanguagesManager::create();
}

Glib::RefPtr<gtksourceview::SourceLanguagesManager> source_language_manager() {
    static Glib::RefPtr<gtksourceview::SourceLanguagesManager> mgr = create_source_language_manager();
    return mgr;
}

Glib::RefPtr<gtksourceview::SourceLanguage> get_language_from_mime_type(const Glib::ustring& mime_type) {
    Glib::RefPtr<gtksourceview::SourceLanguage> lang;
    if( source_language_manager() )
        lang = source_language_manager()->get_language_from_mime_type(mime_type);
    return lang;
}

Glib::RefPtr<gtksourceview::SourceBuffer> create_cppfile_buffer() {
    static Glib::RefPtr<gtksourceview::SourceLanguage> lang = get_language_from_mime_type("text/x-c++hdr");
    if( !lang )
        throw RuntimeException("can not load source language!");

    Glib::RefPtr<gtksourceview::SourceBuffer> buffer = gtksourceview::SourceBuffer::create(lang);
    if( !buffer )
        throw RuntimeException("can not create source buffer!");

    buffer->set_highlight();
    return buffer;
}

