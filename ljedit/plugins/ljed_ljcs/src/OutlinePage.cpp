// OutlinePage.cpp
// 

#include "OutlinePage.h"

OutlinePage::OutlinePage() : file_(0), line_(0) {
}

OutlinePage::~OutlinePage() {
    if( file_ != 0 ) {
        file_->unref();
        file_ = 0;
        line_ = 0;
    }
}

void OutlinePage::create() {
    view_.signal_row_activated().connect(sigc::mem_fun(this, &OutlinePage::on_elem_clicked));
    add_columns();

    sw_.add(view_);
    sw_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    vbox_.pack_start(sw_);
    vbox_.show_all();
}

void OutlinePage::set_file(cpp::File* file, int line) {
    if( file!=file_ ) {
        store_ = Gtk::TreeStore::create(columns_);

        // add children
        if( file!=0 ) {
            const Gtk::TreeNodeChildren& node = store_->children();
            cpp::Elements::iterator it = file->scope.elems.begin();
            cpp::Elements::iterator end = file->scope.elems.end();
            for( ; it!=end; ++it )
                view_add_elem(*it, node);
        }

        if( file_ != 0 )
            file_->unref();

        file_ = (file == 0) ? 0 : file->ref();

        view_.set_model(store_);
    }

    if( file!=0 && line_!=line ) {
        line_ = line;

        view_.get_selection()->unselect_all();
        const Gtk::TreeNodeChildren& root = store_->children();
        view_locate_line((size_t)(line + 1), root);
    }
}

void OutlinePage::add_columns() {
    // type
    {
        //view_.append_column("type", Gtk::CellRendererPixbuf());
    }

    // decl
    {
        int cols_count = view_.append_column("decl",  columns_.decl);
        Gtk::TreeViewColumn* col = view_.get_column(cols_count-1);
        assert( col != 0 );
        Gtk::CellRenderer* render = col->get_first_cell_renderer();
        col->set_clickable();
    }
}

void OutlinePage::view_add_elem(const cpp::Element* elem, const Gtk::TreeNodeChildren& parent) {
    Gtk::TreeRow row = *(store_->append(parent));
    row[columns_.decl] = elem->decl;
    row[columns_.elem] = elem;

    cpp::Scope* scope = 0;
    switch(elem->type) {
    case cpp::ET_ENUM:		scope = &((cpp::Enum*)elem)->scope;			break;
    case cpp::ET_CLASS:		scope = &((cpp::Class*)elem)->scope;		break;
    case cpp::ET_NAMESPACE:	scope = &((cpp::Namespace*)elem)->scope;	break;
    }

    if( scope != 0 ) {
        // add children
        const Gtk::TreeNodeChildren& node = row.children();
        cpp::Elements::iterator it = scope->elems.begin();
        cpp::Elements::iterator end = scope->elems.end();
        for( ; it!=end; ++it )
            view_add_elem(*it, node);
    }
}

void OutlinePage::view_locate_line(size_t line, const Gtk::TreeNodeChildren& parent) {
    Gtk::TreeStore::iterator it = parent.begin();
    Gtk::TreeStore::iterator end = parent.end();
    for( ; it!=end; ++it ) {
        const cpp::Element* elem = it->get_value(columns_.elem);
        assert( elem != 0 );

        if( line < elem->sline )
            break;

        else if( line > elem->eline )
            continue;

        view_.get_selection()->select(it);
        view_.expand_to_path(store_->get_path(it));
        view_.scroll_to_row(store_->get_path(it));
        if( !it->children().empty() )
            view_locate_line( line, it->children() );
        break;
    }
}

void OutlinePage::on_elem_clicked(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn*) {
    Gtk::TreeModel::iterator it = store_->get_iter(path);
    const cpp::Element* elem = it->get_value(columns_.elem);
    if( elem!=0 )
        signal_elem_actived(*elem);
}

