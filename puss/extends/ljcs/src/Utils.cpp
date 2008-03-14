// Utils.cpp
// 

#include "Utils.h"

#include "Environ.h"


gboolean is_in_comment(GtkTextIter* it) {
	GtkTextBuffer* buf = gtk_text_iter_get_buffer(it);
	GtkTextTagTable* tag_table = gtk_text_buffer_get_tag_table(buf);
	GtkTextTag* block_comment_tag = gtk_text_tag_table_lookup(tag_table, "Block Comment");
	GtkTextTag* line_comment_tag = gtk_text_tag_table_lookup(tag_table, "Line Comment");

	if( block_comment_tag && gtk_text_iter_has_tag(it, block_comment_tag) )
		return TRUE;

	if( line_comment_tag && gtk_text_iter_has_tag(it, line_comment_tag) )
		return TRUE;

    return FALSE;
}

gboolean find_keys( StrVector& keys
    , GtkTextIter* it
    , GtkTextIter* end
    , cpp::File* file
	, gboolean find_startswith)
{
    LJEditorDocIter ps(it);
    LJEditorDocIter pe(end);
    std::string key;
	if( !find_key(key, ps, pe, find_startswith ? true : false) )
        return FALSE;
    keys.push_back(key);

	/*
    std::string rkey = it.get_text(end);
    ljcs_parse_macro_replace(rkey, file);
	parse_key(rkey, rkey, find_startswith);
    if( !rkey.empty() && rkey!=key )
        keys.push_back(rkey);
	*/
    return TRUE;
}

bool compare_best_matched_less(cpp::Element& l, cpp::Element& r) {
	bool result = false;

	using namespace cpp;

	switch( l.type ) {
	case ET_MACRO:
		result = true;
		break;

	case ET_UNDEF:
		switch( r.type ) {
		case ET_MACRO:
		case ET_VAR:
		case ET_FUN:
		case ET_ENUMITEM:
		case ET_ENUM:
		case ET_CLASS:
		case ET_USING:
		case ET_NAMESPACE:
		case ET_TYPEDEF:
			result = true;
			break;
		}
		break;
		
	case ET_USING:
		switch( r.type ) {
		case ET_MACRO:
		case ET_VAR:
		case ET_FUN:
		case ET_ENUMITEM:
		case ET_ENUM:
		case ET_CLASS:
		case ET_NAMESPACE:
		case ET_TYPEDEF:
			result = true;
			break;
		}
		break;

	case ET_ENUMITEM:
	case ET_TYPEDEF:
	case ET_VAR:
		switch( r.type ) {
		case ET_MACRO:
			result = true;
			break;
		}
		break;

	case ET_FUN:
		switch( r.type ) {
		case ET_MACRO:
			result = true;
			break;

		case ET_FUN:
			if( ((Function&)l).impl.empty() )
				result = !((Function&)r).impl.empty();
			break;
		}
		break;

	case ET_ENUM:
	case ET_CLASS:
	case ET_NAMESPACE:
		switch( r.type ) {
		case ET_MACRO:
			result = true;
			break;

		case ET_ENUM:
		case ET_CLASS:
		case ET_NAMESPACE:
			if( ((NCScope&)l).scope.empty() )
				result = !((NCScope&)r).scope.empty();
			break;
		}
		break;
	}

	return result;
}

size_t find_best_matched_index(cpp::Elements& elems) {
	size_t index = 0;

	size_t count = elems.size();
	if( count > 1 ) {
		cpp::Element* matched = elems[0];

		for( size_t i=1; i<count; ++i ) {
			if( compare_best_matched_less(*matched, *elems[i]) ) {
				index = i;
				matched = elems[index];
			}
		}
	}

	return index;
}

cpp::Element* find_best_matched_element(cpp::ElementSet& eset) {
	cpp::Element* matched = 0;
	
	if( !eset.empty() ) {
		cpp::ElementSet::iterator it = eset.begin();
		cpp::ElementSet::iterator end = eset.end();
		matched = *it;
		for( ++it; it!=end; ++it )
			if( compare_best_matched_less(*matched, **it) )
				matched = *it;
	}

	return matched;
}

#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcelanguagemanager.h>

GtkTextBuffer* set_cpp_lang_to_source_view(GtkTextView* source_view) {
	GtkSourceLanguageManager* lm = gtk_source_language_manager_get_default();
	GtkSourceLanguage* lang = gtk_source_language_manager_get_language(lm, "cpp");

	GtkSourceBuffer* source_buffer = gtk_source_buffer_new_with_language(lang);
	GtkTextBuffer* retval = GTK_TEXT_BUFFER(source_buffer);
	gtk_text_view_set_buffer(source_view, retval);
	gtk_source_buffer_set_max_undo_levels(source_buffer, 0);
	g_object_unref(G_OBJECT(source_buffer));

	return retval;
}

