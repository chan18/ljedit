// LJCSPluginUtils.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "LJCSPluginUtils.h"


bool is_in_comment(Gtk::TextBuffer::iterator& it) {
    typedef Glib::SListHandle< Glib::RefPtr<Gtk::TextTag> > TagList;
    TagList tags = it.get_tags();
    for( TagList::iterator tag_it = tags.begin(); tag_it!=tags.end(); ++tag_it ) {
        Glib::RefPtr<Gtk::TextTag> tag = *tag_it;
        Glib::ustring name = tag->property_name();
        if( name=="Block Comment" || name=="Line Comment" )
            return true;
    }

    return false;
}

bool find_keys( StrVector& keys
    , Gtk::TextBuffer::iterator& it
    , Gtk::TextBuffer::iterator& end
    , cpp::File* file
	, bool find_startswith)
{
    LJEditorDocIter ps(it);
    LJEditorDocIter pe(end);
    std::string key;
    if( !find_key(key, ps, pe, find_startswith) )
        return false;
    keys.push_back(key);

    std::string rkey = it.get_text(end);
    ljcs_parse_macro_replace(rkey, file);
	parse_key(rkey, rkey, find_startswith);
    if( !rkey.empty() && rkey!=key )
        keys.push_back(rkey);

    return true;
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

std::set<std::string> init_exts() {
	std::set<std::string> exts;
	exts.insert("c");
	exts.insert("h");
	exts.insert("cpp");
	exts.insert("hpp");
	exts.insert("cc");
	exts.insert("hh");
	exts.insert("inl");
	exts.insert("cxx");
	exts.insert("hxx");

	return exts;
}

bool check_cpp_files(const std::string& filepath) {
	std::string name = Glib::path_get_basename(filepath);
	size_t pos = name.find('.');
	if( pos!=name.npos ) {
		// check ext filename is .c .h .cpp .hpp .cc .hh .inl ...
		name.erase(0, pos+1);
		std::transform(name.begin(), name.end(), name.begin(), tolower);

		static std::set<std::string> exts = init_exts();
		return ( exts.find(name) != exts.end() );

	} else {
		// if no ext filename, check path in system header path
		return ParserEnviron::self().in_include_path(filepath);
	}

	return false;
}

