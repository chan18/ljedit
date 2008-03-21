// DocManager.h
// 

#ifndef PUSS_INC_DOCMANAGER_H
#define PUSS_INC_DOCMANAGER_H

#include <gtk/gtk.h>

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

GtkTextView*	puss_doc_get_view_from_page_num( gint page_num );
GtkTextBuffer*	puss_doc_get_buffer_from_page_num( gint page_num );

gint			puss_doc_find_page_from_url( const gchar* url );

void			puss_doc_new();
gboolean		puss_doc_open(const gchar* url, gint line, gint line_offset, gboolean show_message_if_open_failed);
gboolean		puss_doc_locate(gint page_num, gint line, gint line_offset, gboolean add_pos_locate );
void			puss_doc_save_current(gboolean save_as );
gboolean		puss_doc_close_current();
void			puss_doc_save_all();
gboolean		puss_doc_close_all();

#endif//PUSS_INC_DOCMANAGER_H

