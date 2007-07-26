// LanguageManager.h
// 

#ifndef LJED_INC_LANGUAGEMANAGER_H
#define LJED_INC_LANGUAGEMANAGER_H

#include "gtkenv.h"

#include <gtksourceviewmm/sourcebuffer.h>
#include <gtksourceviewmm/sourcelanguage.h>
#include <gtksourceviewmm/sourcelanguagesmanager.h>

Glib::RefPtr<gtksourceview::SourceLanguagesManager> source_language_manager();

Glib::RefPtr<gtksourceview::SourceLanguage> get_language_from_mime_type(const Glib::ustring& mime_type);

Glib::RefPtr<gtksourceview::SourceBuffer> create_cppfile_buffer();

#endif//LJED_INC_LANGUAGEMANAGER_H

