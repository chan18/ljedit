// DocManager.h
// 

#ifndef PUSS_INC_DOCMANAGER_H
#define PUSS_INC_DOCMANAGER_H

#include <gtk/gtk.h>

struct Puss;

// doc & view
void			puss_doc_set_url( GtkTextBuffer* buffer, const gchar* url );
GString*		puss_doc_get_url( GtkTextBuffer* buffer );

void			puss_doc_set_charset( GtkTextBuffer* buffer, const gchar* charset );
GString*		puss_doc_get_charset( GtkTextBuffer* buffer );

void			puss_doc_replace_all( GtkTextBuffer* buf
									, const gchar* find_text
									, const gchar* replace_text
									, gint flags );

GtkTextView*	puss_doc_get_view_from_page( GtkWidget* page );
GtkTextBuffer*	puss_doc_get_buffer_from_page( GtkWidget* page );

// doc manager

GtkLabel*		puss_doc_get_label_from_page_num( Puss* app, int page_num );
GtkTextView*	puss_doc_get_view_from_page_num( Puss* app, gint page_num );
GtkTextBuffer*	puss_doc_get_buffer_from_page_num( Puss* app, gint page_num );

gint			puss_doc_find_page_from_url( Puss* app, const gchar* url );

void			puss_doc_new( Puss* app );
gboolean		puss_doc_open( Puss* app, const gchar* url, gint line, gint line_offset );
gboolean		puss_doc_locate( Puss* app, const gchar* url, gint line, gint line_offset );
void			puss_doc_save_current( Puss* app, gboolean save_as );
void			puss_doc_save_current_as( Puss* app );
gboolean		puss_doc_close_current( Puss* app );
void			puss_doc_save_all( Puss* app );
gboolean		puss_doc_close_all( Puss* app );

#endif//PUSS_INC_DOCMANAGER_H

