// LJCSPluginUtils.cpp
// 

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
    , cpp::File* file)
{
    LJEditorDocIter ps(it);
    LJEditorDocIter pe(end);
    std::string key;
    if( !find_key(key, ps, pe) )
        return false;
    keys.push_back(key);

    std::string rkey = it.get_text(end);
    ljcs_parse_macro_replace(rkey, file);
	parse_key(rkey, rkey);
    if( !rkey.empty() && rkey!=key )
        keys.push_back(rkey);

    return true;
}

