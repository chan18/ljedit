// LJCSPluginImpl.h
// 

#ifndef LJED_INC_LJCSPLUGINIMPL_H
#define LJED_INC_LJCSPLUGINIMPL_H

#include "gtkenv.h"

#include <string>
#include <list>
#include <map>

#include "TipWindow.h"
#include "OutlinePage.h"
#include "PreviewPage.h"

#include "ParseThread.h"

class DocPage;
class LJEditor;

typedef std::list<sigc::connection>			TConnectionList;
typedef std::map<void*, TConnectionList>	TConnectionListMap;

inline void disconnect_all_connections(TConnectionList& cons) {
	std::for_each( cons.begin()
		, cons.end()
		, std::mem_fun_ref(&sigc::connection::disconnect) );
    cons.clear();
}

class LJCSPluginImpl : public sigc::trackable {
public:
    LJCSPluginImpl(LJEditor& editor);

    void create(const char* plugin_filename);
    void destroy();

private:	// for options
	void on_option_changed(const std::string& id, const std::string& value, const std::string& old);

	void option_set_include_path(const std::string& option_text);

private:	// auto complete
    void active_page(DocPage& page);
    void deactive_page(DocPage& page);

    void show_hint(DocPage& page
        , Gtk::TextBuffer::iterator& it
        , Gtk::TextBuffer::iterator& end
        , char tag);
	void locate_sub_hint(DocPage& page);

    void auto_complete(DocPage& page);

private:
	void set_show_hint_timer(DocPage& page);
	void kill_show_hint_timer();

	bool on_show_hint_timeout(DocPage* page, int tag);

private:
    void on_doc_page_added(Gtk::Widget* widget, guint page_num);
    void on_doc_page_removed(Gtk::Widget* widget, guint page_num);

    bool on_key_press_event(GdkEventKey* event, DocPage* page);
    bool on_key_release_event(GdkEventKey* event, DocPage* page);
    bool on_button_release_event(GdkEventButton* event, DocPage* page);
    bool on_motion_notify_event(GdkEventMotion* event, DocPage* page);
    bool on_focus_out_event(GdkEventFocus* event, DocPage* page);

	void on_modified_changed(DocPage* page);

private:
	void do_button_release_event(GdkEventButton* event, DocPage* page, cpp::File* file);

	void open_include_file(const char* filename, bool system_header, cpp::File* owner);
	void show_include_hint(const std::string& filename, bool system_header, DocPage& page);

private:	// outline
    void outline_update_page();

    void outline_on_switch_page(GtkNotebookPage*, guint);
    void outline_on_elem_actived(const cpp::Element& elem);

private:
    TipWindow	tip_;
    OutlinePage	outline_;
    PreviewPage	preview_;
	int			preview_page_;

private:
	sigc::connection		show_hint_timer_;
	int						show_hint_tag_;

private:
    LJEditor&				editor_;
	std::string				plugin_path_;

    ParseThread				parse_thread_;

    TConnectionListMap		connections_map_;
};

#endif//LJED_INC_LJCSPLUGINIMPL_H

