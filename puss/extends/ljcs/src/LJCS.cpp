// LJCS.cpp
// 

#include "LJCS.h"

LJCS::LJCS() : app(0) {}

LJCS::~LJCS() { destroy(); }

bool LJCS::create(Puss* _app) {
	app = _app;

	icons.create(app);
	preview_page.create(app, &env);

	GtkNotebook* doc_panel = puss_get_doc_panel(app);
	g_signal_connect(doc_panel, "page-added",   G_CALLBACK(&LJCS::on_doc_page_added),   this);
	g_signal_connect(doc_panel, "page-removed", G_CALLBACK(&LJCS::on_doc_page_removed), this);

	return true;
}

void LJCS::destroy() {
	preview_page.destroy();
	icons.destroy();

	app = 0;
}

void LJCS::on_doc_page_added(GtkNotebook* doc_panel, GtkWidget* child, guint page_num, gpointer self) {

}

void LJCS::on_doc_page_removed(GtkNotebook* doc_panel, GtkWidget* child, guint page_num, gpointer self) {

}

