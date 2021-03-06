// PreviewPage.c
// 

#include "LanguageTips.h"


typedef struct {
	gpointer spath;
	CppFile* file;
	gint     line;
} PreviewSearchKey;

static PreviewSearchKey PREVIEW_SEARCH_THREAD_EXIT_SIGN = {0,0,0};

static void search_key_free(PreviewSearchKey* key) {
	if( key != &PREVIEW_SEARCH_THREAD_EXIT_SIGN ) {
		cpp_spath_free(key->spath);
		if( key->file )
			cpp_file_unref(key->file);
		g_free(key);
	}
}

typedef struct {
	LanguageTips*	self;
	gpointer		key;
	GSequence*		seq;
} PreviewIdleArg;

static gboolean preview_show(PreviewIdleArg* arg) {
	if( arg->self->preview_search_key == arg->key ) {
		if( arg->self->preview_search_seq )
			g_sequence_free(arg->self->preview_search_seq);
		arg->self->preview_search_seq = arg->seq;
		arg->self->preview_last_index  = 0;
		preview_update(arg->self);

	} else {
		search_key_free(arg->key);
		if( arg->seq )
			g_sequence_free(arg->seq);
	}
				
	g_free(arg);
	return FALSE;		
}

static gpointer tips_preview_search_thread(LanguageTips* self) {
	PreviewSearchKey* key;
	GSequence* seq;
	GAsyncQueue* queue = self->preview_search_queue;
	if( !queue )
		return 0;

	for(;;) {
		key = (PreviewSearchKey*)g_async_queue_pop(queue);
		if( key==&PREVIEW_SEARCH_THREAD_EXIT_SIGN )
			break;

		if( !key )
			continue;

		seq = cpp_guide_search(self->cpp_guide, key->spath, 0, key->file, key->line, -1, -1);
		if( seq ) {
			PreviewIdleArg* arg = g_new0(PreviewIdleArg, 1);
			arg->self = self;
			arg->key = key;
			arg->seq = seq;
			g_idle_add((GSourceFunc)preview_show, arg);
		}
	}

	g_async_queue_unref(queue);

	return 0;
}

void preview_init(LanguageTips* self) {
	self->preview_search_key = 0;
	self->preview_search_seq = 0;
	self->preview_search_queue = g_async_queue_new_full(g_free);
	g_async_queue_ref(self->preview_search_queue);
	self->preview_search_thread = g_thread_create((GThreadFunc)tips_preview_search_thread, self, TRUE, 0);
}

void preview_final(LanguageTips* self) {
	if( self->preview_search_queue ) {
		g_async_queue_push(self->preview_search_queue, &PREVIEW_SEARCH_THREAD_EXIT_SIGN);
		g_async_queue_unref(self->preview_search_queue);
	}

	if( self->preview_search_thread )
		g_thread_join(self->preview_search_thread);

	if( self->preview_search_key )
		search_key_free((PreviewSearchKey*)(self->preview_search_key));

	if( self->preview_search_seq )
		g_sequence_free(self->preview_search_seq);
}

static gboolean scroll_to_define_line(LanguageTips* self) {
	GSequenceIter* iter;
	GtkTextBuffer* buffer;
	CppElem* elem;

	iter = g_sequence_get_iter_at_pos(self->preview_search_seq, self->preview_last_index);
	elem = g_sequence_iter_is_end(iter) ? 0 : (CppElem*)g_sequence_get(iter);

	if( elem ) {
		buffer = gtk_text_view_get_buffer(self->preview_view);

		if( elem->sline < (guint)gtk_text_buffer_get_line_count(buffer) ) {
			GtkTextIter it;
			gtk_text_buffer_get_iter_at_line(buffer, &it, elem->sline - 1);
			if( !gtk_text_iter_is_end(&it) ) {
				gtk_text_buffer_place_cursor(buffer, &it);
				gtk_text_view_scroll_to_iter(self->preview_view, &it, 0.0, TRUE, 1.0, 0.25);
			}
		}
	}

    return FALSE;
}

static void preview_do_update(LanguageTips* self, gint index) {
	gint num;
	gsize len = 0;
	gchar* text;
	CppElem* elem;
	GSequenceIter* iter;
	GtkTextBuffer* buffer;

	buffer = gtk_text_view_get_buffer(self->preview_view);

	if( !self->preview_search_seq ) {
		gtk_label_set_text(self->preview_filename_label, "");
		gtk_button_set_label(self->preview_number_button, "0/0");
		gtk_text_buffer_set_text(buffer, "", 0);
		self->preview_last_file = 0;
		return;
	}

	num = g_sequence_get_length(self->preview_search_seq);
	self->preview_last_index = index % num;
	iter = g_sequence_get_iter_at_pos(self->preview_search_seq, self->preview_last_index);
	elem = g_sequence_iter_is_end(iter) ? 0 : (CppElem*)g_sequence_get(iter);
	if( !elem )
		return;

	// set number button
	text = g_strdup_printf("%d/%d", self->preview_last_index + 1, num);
	gtk_button_set_label(self->preview_number_button, text);
	g_free(text);

	// set filename label
	text = g_strdup_printf("%s:%d", elem->file->filename->buf, elem->sline);
	gtk_label_set_text(self->preview_filename_label, text);
	g_free(text);

	// set text
	if( elem->file != self->preview_last_file ) {
		self->preview_last_file = elem->file;

		if( !self->app->load_file(elem->file->filename->buf, &text, &len, 0, 0) )
			return;

		gtk_text_buffer_set_text(buffer, text, len);
		g_free(text);

		g_idle_add((GSourceFunc)scroll_to_define_line, self);

	} else {
		scroll_to_define_line(self);
	}
}

void preview_update(LanguageTips* self) {
	preview_do_update(self, self->preview_last_index);
}

void preview_set(LanguageTips* self, gpointer spath, CppFile* file, gint line) {
	PreviewSearchKey* key;
	if( !spath )
		return;

	if( self->preview_search_key ) {
		key = self->preview_search_key;
		if( key->file==file && key->line==line && cpp_spath_equal(key->spath, spath) ) {
			cpp_spath_free(spath);
			preview_do_update(self, self->preview_last_index + 1);
			return;
		}
	}

	key = g_new0(PreviewSearchKey, 1);
	key->spath = spath;
	key->file = file ? cpp_file_ref(file) : file;
	key->line = line;

	self->preview_search_key = key;

	g_async_queue_push(self->preview_search_queue, key);
}

SIGNAL_CALLBACK void preview_cb_number_button_clicked(GtkButton* button, LanguageTips* self) {
	preview_do_update(self, self->preview_last_index + 1);
}

SIGNAL_CALLBACK void preview_cb_filename_button_clicked(GtkButton* button, LanguageTips* self) {
	GSequenceIter* iter;
	CppElem* elem;

	iter = g_sequence_get_iter_at_pos(self->preview_search_seq, self->preview_last_index);
	elem = g_sequence_iter_is_end(iter) ? 0 : (CppElem*)g_sequence_get(iter);
	open_and_locate_elem(self, elem);
}

