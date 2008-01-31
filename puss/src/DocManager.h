// DocManager.h
// 

#ifndef PUSS_INC_DOCMANAGER_H
#define PUSS_INC_DOCMANAGER_H

#include <gtk/gtk.h>

struct Puss;

void		puss_doc_new( Puss* app );
gboolean	puss_doc_open( Puss* app, const gchar* filepath, int line=0, int line_offset=0 );
gboolean	puss_doc_locate( Puss* app, const gchar* filepath, int line=0, int line_offset=0 );
void		puss_doc_save_current( Puss* app );
void		puss_doc_save_current_as( Puss* app );
void		puss_doc_close_current( Puss* app );
void		puss_doc_save_all( Puss* app );
void		puss_doc_close_all( Puss* app );

#endif//PUSS_INC_DOCMANAGER_H

