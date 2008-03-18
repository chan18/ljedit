// OptionManager.cpp
//

#include "OptionManager.h"

#include "Puss.h"

struct OptionNotifer {
	OptionNotifer*		next;
	OptionChanged		fun;
	gpointer			tag;
};

struct OptionNode {
	gchar*			current_value;
	gchar*			default_value;
	OptionSetter	setter_fun;
	gpointer		setter_tag;

	OptionNotifer*	notifer_list;
};

void option_node_free(OptionNode* node) {
	g_free(node->current_value);
	g_free(node->default_value);

	while( node->notifer_list ) {
		OptionNotifer* p = node->notifer_list;
		node->notifer_list = p->next;
		g_free(p);
	}

	g_free(node);
}

struct OptionManager {
	gchar*		filepath;
	GKeyFile*	keyfile;

	GHashTable*	option_groups;	// group, GHashTable(key, OptionNode)
};

gboolean puss_option_manager_create() {
	g_assert( !puss_app->option_manager );

	puss_app->option_manager = g_new0(OptionManager, 1);
	if( !puss_app->option_manager ) {
		g_printerr("ERROR(option_manager) : new option manager failed!\n");
		return FALSE;
	}

	OptionManager* self = puss_app->option_manager;
	self->filepath = g_build_filename(g_get_user_config_dir(), ".puss_config", NULL);
	if( !self->filepath ) {
		g_printerr("ERROR(option_manager) : build filename failed!\n");
		return FALSE;
	}

	self->keyfile = g_key_file_new();
	if( !self->keyfile  ) {
		g_printerr("ERROR(option_manager) : new key file failed!\n");
		return FALSE;
	}

	self->option_groups = (GHashTable*)g_hash_table_new_full( &g_str_hash, &g_str_equal, &g_free, GDestroyNotify(&g_hash_table_destroy) );
	if( !self->option_groups ) {
		g_printerr("ERROR(option_manager) : new hash table failed!\n");
		return FALSE;
	}

	// now ignore error, TODO :
	// 
	g_key_file_load_from_file(self->keyfile, self->filepath, G_KEY_FILE_KEEP_COMMENTS, 0);

	return TRUE;
}

void puss_option_manager_destroy() {
	OptionManager* self = puss_app->option_manager;
	puss_app->option_manager = 0;

	if( self ) {
		g_key_file_free(self->keyfile);
		g_free(self->filepath);
		g_hash_table_destroy(self->option_groups);

		g_free(self);
	}
}

gboolean option_manager_check_option(const gchar* group, const gchar* key) {
	OptionManager* self = puss_app->option_manager;

	GHashTable* option_group = (GHashTable*)g_hash_table_lookup(self->option_groups, group);
	if( !option_group )
		return FALSE;

	OptionNode* node = (OptionNode*)g_hash_table_lookup(option_group, key);
	if( !node )
		return FALSE;

	gchar* value = g_key_file_get_value(self->keyfile, group, key, 0);
	if( value==0 && node->current_value==0 )
		return FALSE;

	if( value && node->current_value && g_str_equal(value, node->current_value) )
		return FALSE;

	for( OptionNotifer* p = node->notifer_list; p; p = p->next ) {
		g_assert( p->fun );
		p->fun(group, key, value, node->current_value, p->tag);
	}

	// call

	g_free(node->current_value);
	node->current_value = value;
	return TRUE;
}

void option_manager_save() {
	OptionManager* self = puss_app->option_manager;

	GError* err = 0;
	gsize length = 0;
	gchar* content = g_key_file_to_data(self->keyfile, &length, &err);
	if( !content ) {
		g_printerr("ERROR(g_key_file_to_data) : %s\n", err->message);
		return;
	}

	if( !g_file_set_contents(self->filepath, content, length, &err) )
		g_printerr("ERROR(g_file_set_contents) : %s\n", err->message);

	g_free(content);
}

gboolean puss_option_manager_option_reg(const gchar* group, const gchar* key, const gchar* default_value, OptionSetter fun, gpointer tag) {
	OptionManager* self = puss_app->option_manager;

	GHashTable* option_group = (GHashTable*)g_hash_table_lookup(self->option_groups, group);
	if( !option_group ) {
		option_group = (GHashTable*)g_hash_table_new_full( &g_str_hash, &g_str_equal, &g_free, GDestroyNotify(&option_node_free) );
		if( !option_group )
			return FALSE;
		g_hash_table_insert(self->option_groups, g_strdup(group), option_group);
	}

	OptionNode* node = (OptionNode*)g_hash_table_lookup(option_group, key);
	if( node )
		return FALSE;

	node = g_new0(OptionNode, 1);
	if( node ) {
		node->current_value = g_key_file_get_value(self->keyfile, group, key, 0);
		if( default_value )
			node->default_value = g_strdup(default_value);
		node->setter_fun = fun;
		node->setter_tag = tag;
		node->notifer_list = 0;

		g_hash_table_insert(option_group, g_strdup(key), node);
		return TRUE;
	}

	return TRUE;
}

gboolean puss_option_manager_monitor_reg(const gchar* group, const gchar* key, OptionChanged fun, gpointer tag ) {
	OptionManager* self = puss_app->option_manager;

	GHashTable* option_group = (GHashTable*)g_hash_table_lookup(self->option_groups, group);
	if( !option_group )
		return FALSE;

	OptionNode* node = (OptionNode*)g_hash_table_lookup(option_group, key);
	if( !node )
		return FALSE;

	OptionNotifer* notifer = g_new0(OptionNotifer, 1);
	if( !notifer )
		return FALSE;

	notifer->next = node->notifer_list;
	notifer->fun = fun;
	notifer->tag = tag;
	node->notifer_list = notifer;
	return TRUE;
}

gboolean puss_default_option_setter(GKeyFile* options, const gchar* group, const gchar* key, gpointer tag) {
	// TODO : 
	const gchar* args = (const char*)tag;

	if( args==0 ) {
		g_key_file_set_string(options, group, key, "aaaaaaaaaaaa");
		return TRUE;

	} else if( g_str_equal(args, "bool") ) {
		g_key_file_set_boolean(options, group, key, TRUE);
		return TRUE;
	}
	// ...

	return FALSE;
}

// test
// 
void fff(const gchar* group, const gchar* key, const gchar* new_value, const gchar* current_value, gpointer tag) {
	g_print("fff");
}

void ggg(const gchar* group, const gchar* key, const gchar* new_value, const gchar* current_value, gpointer tag) {
	g_print("ggg");
}

void testtttttt() {
	OptionManager* self = puss_app->option_manager;

	puss_option_manager_option_reg("puss", "aaa", "abcdef", &puss_default_option_setter, 0);
	puss_option_manager_option_reg("puss", "bbb", "true", 0, "bool");

	puss_option_manager_monitor_reg("puss", "aaa", &fff, 0);
	puss_option_manager_monitor_reg("puss", "aaa", &ggg, 0);

	if( puss_default_option_setter(self->keyfile, "puss", "aaa", 0) )
		option_manager_check_option("puss", "aaa");
	
	if( puss_default_option_setter(self->keyfile, "puss", "aaa", "bool") )
		option_manager_check_option("puss", "aaa");

	if( puss_default_option_setter(self->keyfile, "puss", "aaa", "xxxx") )
		option_manager_check_option("puss", "aaa");

	g_print("aaaa\n");
}

void puss_option_manager_active() {
	// TODO : show option UI
	// when row active [row]
	//		call [row].option_setter()
	// then
	//		call option_manager_check_option()

	testtttttt();

	option_manager_save();
}

