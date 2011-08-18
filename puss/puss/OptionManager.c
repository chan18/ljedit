// OptionManager.c
//

#include "OptionManager.h"

#include <string.h>
#include <assert.h>

#include "Puss.h"
#include "Utils.h"

typedef struct _OptionNotifer	OptionNotifer;
typedef struct _OptionNode		OptionNode;

struct _OptionNotifer {
	OptionNotifer*	next;
	OptionNode*		owner;
	OptionChanged	fun;
	gpointer		tag;
	GFreeFunc		tag_free_fun;
};

struct _OptionNode {
	Option			option;
	gchar*			value;
	gchar*			default_value;
	OptionNotifer*	notifer_list;
};

static void option_node_free(OptionNode* node) {
	OptionNotifer* p;

	g_free(node->value);
	g_free(node->default_value);

	while( node->notifer_list ) {
		p = node->notifer_list;
		node->notifer_list = p->next;
		if( p->tag_free_fun )
			(*(p->tag_free_fun))(p->tag);
		g_free(p);
	}

	g_free(node);
}

typedef struct _OptionManager  OptionManager;

struct _OptionManager {
	gchar*		filepath;
	GKeyFile*	keyfile;
	gboolean	modified;

	GTree*		option_groups;	// group, GTree(key, OptionNode)
};

static OptionManager*	puss_option_manager;

static gint compare_data_func_wrapper(gpointer a, gpointer b, GCompareFunc cmp) { return (*cmp)(a, b); }

gboolean puss_option_manager_create() {
	assert( !puss_option_manager );

	puss_option_manager = g_new0(OptionManager, 1);

	puss_option_manager->filepath = g_build_filename(g_get_user_config_dir(), ".puss_config", NULL);
	if( !puss_option_manager->filepath ) {
		g_printerr("ERROR(option_manager) : build filename failed!\n");
		return FALSE;
	}

	puss_option_manager->keyfile = g_key_file_new();
	if( !puss_option_manager->keyfile  ) {
		g_printerr("ERROR(option_manager) : new key file failed!\n");
		return FALSE;
	}

	puss_option_manager->option_groups = g_tree_new_full( (GCompareDataFunc)&compare_data_func_wrapper, (gpointer)&g_ascii_strcasecmp, (GDestroyNotify)&g_free, (GDestroyNotify)&g_tree_destroy );
	if( !puss_option_manager->option_groups ) {
		g_printerr("ERROR(option_manager) : new tree failed!\n");
		return FALSE;
	}

	// now ignore error, TODO :
	// 
	g_key_file_load_from_file(puss_option_manager->keyfile, puss_option_manager->filepath, G_KEY_FILE_KEEP_COMMENTS, 0);

	return TRUE;
}

void option_manager_save() {
	GError* err = 0;
	gsize length = 0;
	gchar* content = g_key_file_to_data(puss_option_manager->keyfile, &length, &err);
	if( !content ) {
		g_printerr("ERROR(g_key_file_to_data) : %s\n", err->message);
		return;
	}

	if( !g_file_set_contents(puss_option_manager->filepath, content, length, &err) )
		g_printerr("ERROR(g_file_set_contents) : %s\n", err->message);

	g_free(content);
}

void puss_option_manager_destroy() {
	puss_option_manager_save();

	if( puss_option_manager ) {
		g_key_file_free(puss_option_manager->keyfile);
		g_free(puss_option_manager->filepath);
		g_tree_destroy(puss_option_manager->option_groups);

		g_free(puss_option_manager);
		puss_option_manager = 0;
	}
}

void puss_option_manager_save() {
	if( puss_option_manager && puss_option_manager->modified )
		option_manager_save();
}

const Option* puss_option_manager_option_reg( const gchar* group, const gchar* key, const gchar* default_value ) {
	OptionNode* node;
	gchar* ptr_group = 0;
	GTree* option_group = 0;

	if( !g_tree_lookup_extended(puss_option_manager->option_groups, group, (gpointer*)&ptr_group, (gpointer*)&option_group) ) {
		option_group = g_tree_new_full((GCompareDataFunc)&compare_data_func_wrapper, (gpointer)&strcmp, (GDestroyNotify)&g_free, (GDestroyNotify)&option_node_free );
		if( !option_group )
			return 0;
		ptr_group = g_strdup(group);
		g_tree_insert(puss_option_manager->option_groups, ptr_group, option_group);
	}

	node = (OptionNode*)g_tree_lookup(option_group, key);
	if( !node ) {
		node = g_new0(OptionNode, 1);
		node->option.group = ptr_group;
		node->option.key = g_strdup(key);

		node->option.value = g_key_file_get_string(puss_option_manager->keyfile, group, key, 0);
		if( default_value ) {
			node->option.default_value = g_strdup(default_value);
			if( !node->option.value )
				node->option.value = g_strdup(default_value);
		}

		g_tree_insert(option_group, (gpointer)node->option.key, node);
	}

	return &node->option;
}

const Option* puss_option_manager_option_find(const gchar* group, const gchar* key) {
	OptionNode* node;
	GTree* option_group = (GTree*)g_tree_lookup(puss_option_manager->option_groups, group);
	if( option_group ) {
		node = (OptionNode*)g_tree_lookup(option_group, key);
		if( node )
			return &(node->option);
	}

	return 0;
}

void puss_option_manager_option_set(const Option* option, const gchar* value) {
	gchar* old;
	OptionNotifer* p;
	OptionNode* node = (OptionNode*)option;

	if( value && (node->option.value==0 || !g_str_equal(node->option.value, value)) ) {
		old = node->option.value;
		node->option.value = g_strdup(value);
		g_key_file_set_string(puss_option_manager->keyfile, option->group, option->key, node->option.value);
		puss_option_manager->modified = TRUE;

		for( p = node->notifer_list; p; p = p->next ) {
			if( p->fun ) {
				p->fun(option, old, p->tag);
			} else  {
				OptionNotifer* t = p;
				p = p->next;
				if( t==node->notifer_list )
					node->notifer_list = p;
				g_free(t);
			}
		}

		g_free(old);
	}
}

gpointer puss_option_manager_monitor_reg(const Option* option, OptionChanged fun, gpointer tag, GFreeFunc tag_free_fun) {
	OptionNotifer* notifer = 0;
	OptionNode* node = (OptionNode*)option;

	if( fun ) {
		notifer = g_new0(OptionNotifer, 1);
		notifer->next = node->notifer_list;
		notifer->owner = node;
		notifer->fun = fun;
		notifer->tag = tag;
		notifer->tag_free_fun = tag_free_fun;
		node->notifer_list = notifer;
	} else {
		if( tag_free_fun )
			tag_free_fun(tag);
	}

	return notifer;
}

void puss_option_manager_monitor_unreg(gpointer handler) {
	OptionNotifer* p = (OptionNotifer*)handler; 
	if( p ) {
		p->fun = 0;
		if( p->tag_free_fun ) {
			p->tag_free_fun( p->tag );
			p->tag_free_fun = 0;
		}

		if( p->owner->notifer_list==p ) {
			p->owner->notifer_list = p->next;
		} else {
			OptionNotifer* q;
			for( q=p->owner->notifer_list; q && q->next!=p; q=q->next );
			if( q )
				q->next = p->next;
		}

		g_free(p);
	}
}

