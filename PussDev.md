# Puss develop #

## Puss Application ##

Introduction :

Puss is a lightly C/C++ sourcecode editor, support C/C++ intelligent language tips、auto-complete and support some very usefull dev-tools. It can be used in Windows or Linux.

Puss is based Glib/Gtk+ and GtkSourceView, and can use Python as it's default embeded script engine now, it will be replaced by javascript, but we will support it always.

Puss used three-layer architecture:
  1. **the core layer is PussEditor**, can be extends by **IPuss** interface.
  1. **the second layer is PussExtends**, it's an optional layer, it's can be queried by plug-ins through **IPuss** interface.
  1. **the three layer is PussPlugin**, it's a terminal layer, it's can use **IPuss**, but can not be queried by other plug-ins. Puss embeded DllPluginEngine, so we can create plug-ins use C/C++ languages, but also support script plug-ins when Python or Javascript Engine Extends loaded(now we can use python engine only, because only python-gtk can used in windows platform).

> TODO : introduce extend manager and how to extend works.

> TODO : introduce plugin manager and how to plugin engine works.

### IPuss ###
support puss extends & plug-ins.
  * UIManager interface : marge extend/plug-in UI into puss.
  * DocManager interface : support document/view options, for example find/open/close document.
  * OptionManager interface : support common option interface, extend/plug-in can use existed option or regist new option
  * ExtendManager interface : support query existed extend interface.
  * PluginManager interface : support regist new plug-in engine.
  * other utils interface : some utils for extend/plug-in.

PS: extend can not unload on runtime, but plug-in can dynamic load/unload.

#### UIManager ####

Interface Define
```
GtkBuilder* get_ui_builder();
void        panel_append(GtkWidget* panel, GtkWidget* tab_label, const gchar* id, PanelPosition default_pos);
void        panel_remove(GtkWidget* panel);
gboolean    panel_get_pos(GtkWidget* panel, GtkNotebook** parent, gint* page_num);
```

Interface Usage
  * **get\_ui\_builder()** return puss UI which load from UI defined XML files, you can find all static created UI widgets from those files in puss/bin/res/**.xml.
    * puss\_main\_window.xml   // main window
    * puss\_ui\_manager        // menu & actions**

> for examples: get main\_toolbar and hide it.
```
IPuss*      puss;	// you can get it when you plug-in load
GtkBuilder* ui;
GtkWidget*  main_toolbar;

/*
  Open puss_main_window.xml, you will see toolbar ID is “main_toolbar”,
  so you can get it from GtkBuilder returns from IPuss::get_ui_builder()
*/
ui = puss->get_ui_builder();
main_toolbar = GTK_WIDGET( gtk_builder_get_object(ui, “main_toolbar”) );
gtk_widget_hide(main_toolbar);
```

  * **panel\_append()** used append widget into puss panels(left/right/bottom)
  * **panel\_remove()** used remove widget whitch append by puss\_append()

> for example: append an button widget into puss bottom panel
```
IPuss*      puss;	// you can get it when you plug-in load

GtkWidget*  label;
GtkWidget*  widget;

label = gtk_label_new("test");
widget = gtk_button_new_with_label("test button");

// when create
puss->panel_append(widget, label, "test_id", PUSS_PANEL_POS_BOTTOM);

// when destroy
puss->panel_remove(widget);
```

> more example: insert menu item & tool item
```
static const gchar* ui_info =
	"<ui>"
	"  <menubar name='main_menubar'>"
	"     <menu action='tool_menu'>"
	"      <placeholder name='tool_menu_plugin_place'>"
	"        <menuitem action='test_action1'/>"
	"        <menuitem action='test_action2'/>"
	"      </placeholder>"
	"    </menu>"
	"  </menubar>"
	""
	"  <toolbar name='main_toolbar'>"
	"    <placeholder name='main_toolbar_tool_place'>"
	"      <toolitem action='test_action1'/>"
	"    </placeholder>"
	"  </toolbar>"
	"</ui>"
	;

static GtkActionEntry test_actions[] = {
	  { "test_action1", GTK_STOCK_PREFERENCES, "test1", 0, "test 1",	(GCallback)on_action1 }
	, { "test_action2", GTK_STOCK_PREFERENCES, "test2", 0, "test 2",	(GCallback)on_action2 }
};

typedef struct {
	Puss*           app;
	GtkActionGroup* action_group;
	guint           ui_meger_id;
} TestPlugin;

void ui_create(TestPlugin* self) {
	GtkUIManager* ui_mgr = puss_get_ui_manager(self->app);

	self->action_group = gtk_action_group_new("gdbmi_action_group");

	gtk_action_group_add_actions(self->action_group, test_actions, 2, self);
	gtk_ui_manager_insert_action_group(, self->action_group, 0);

	self->ui_merge_id = gtk_ui_manager_add_ui_from_string(ui_mgr, ui_info, -1, 0)
	gtk_ui_manager_ensure_update(ui_mgr);
}

void ui_destroy(TestPlugin* self) {
	GtkUIManager* ui_mgr = puss_get_ui_manager(self->app);

	gtk_ui_manager_remove_ui(ui_mgr, self->ui_meger_id);
	gtk_ui_manager_remove_action_group(ui_mgr, self->action_group);
	g_object_unref( G_OBJECT(self->action_group) );
}

```

#### DocManager ####

Interface Define
```
// now url is filename, I want to use url when I begin puss, but now it's filename.
// 
void            doc_set_url( GtkTextBuffer* buffer, const gchar* url );
GString*        doc_get_url( GtkTextBuffer* buffer );

void            doc_set_charset( GtkTextBuffer* buffer, const gchar* charset );
GString*        doc_get_charset( GtkTextBuffer* buffer );

GtkTextView*    doc_get_view_from_page( GtkWidget* page );
GtkTextBuffer*  doc_get_buffer_from_page( GtkWidget* page );

GtkTextView*    doc_get_view_from_page_num( gint page_num );
GtkTextBuffer*  doc_get_buffer_from_page_num( gint page_num );
gint            doc_find_page_from_url( const gchar* url );

void            doc_new();
gboolean        doc_open(const gchar* url, gint line, gint line_offset, gboolean show_message_if_open_failed );
gboolean        doc_open_locate(const gchar* url, FindLocation fun, gpointer tag, gboolean show_message_if_open_failed );
gboolean        doc_locate( gint page_num, gint line, gint line_offset, gboolean add_pos_locate );
void            doc_save_current( gboolean save_as );
gboolean        doc_close_current();
void            doc_save_all();
gboolean        doc_close_all();
```

Interface Usage
> for example: get all opened file & print filenames.
```
IPuss* puss;

gint i;
GtkNotebook* doc_panel = puss_get_doc_panel(puss);
gint pages = gtk_notebook_get_n_pages(doc_panel);

for( i=0; i<pages; ++i ) {
	GtkTextBuffer* buf = puss->doc_get_buffer_from_page_num(i);
	GString* filename = puss->doc_get_url(buf);
	if( filename )
		g_print("opend : %s\n", filename->str);
	else
		g_print("new document not saved!\n");
}
```

> for example: tes use doc manager.
```
IPuss* puss;
GtkNotebook* doc_panel = puss_get_doc_panel(puss);

// get current file, insert some text at current cursor place
{
	gint page_num;
	GtkTextBuffer* buf;

	page_num = gtk_notebook_get_current_page(doc_panel);
	buf = puss->doc_get_buffer_from_page_num(page_num);
	gtk_text_buffer_insert_at_cursor(buf, "some text", -1);
}

// open & locate test.c line 4 col 8, if not find file, puss will show message dialog!
// if file already opened, it will active file page and do locate.
//
puss->doc_open("test.c", 4, 8, TRUE);
```

#### OptionManager ####

Interface Define
```
// option manager
const Option*   option_reg(const gchar* group, const gchar* key, const gchar* default_value);
const Option*   option_find(const gchar* group, const gchar* key);
void            option_set(const Option* option, const gchar* value);

gpointer        option_monitor_reg(const Option* option, OptionChanged fun, gpointer tag, GFreeFunc tag_free_fun);
void            option_monitor_unreg(gpointer handler);

// option setup
gboolean        option_setup_reg(const gchar* id, const gchar* name, CreateSetupWidget creator, gpointer tag, GDestroyNotify tag_destroy);
void            option_setup_unreg(const gchar* id);
```

Interface Usage
  * **option\_reg()** used regist option, the group always use your plugin name.
> for example:
```
const Option* option = puss->option_reg("my_plugin_group_id", "enable", "yes");
```

  * **option\_find()** used find exist option.
> for example:
```
const Option* option = puss->option_find("puss", "editor.font");
```

  * **option\_set()** used set option value, it will active each monitor registed on this option.
  * **option\_monitor\_reg()/option\_monitor\_unreg()** used regist/unregist option monitor.
> for exmaple:
```
static void parse_editor_font_option(const Option* option, const gchar* old, MyPlugin* self) {
	PangoFontDescription* desc = pango_font_description_from_string(option->value);
	if( desc ) {
		gtk_widget_modify_font(GTK_WIDGET(self->my_view), desc);
		pango_font_description_free(desc);
	}
}

gpointer handler;

// when create, regist monitor
option = puss->option_find("puss", "editor.font");
handler = puss->option_monitor_reg(option, (OptionChanged)parse_editor_font_option, self, 0);

// when set value, it will call parse_editor_font_option()
puss->option_set(option, "san 16");

// when destroy, unregist monitor
puss->option_monitor_unreg(handler);
	}
```

  * **option\_setup\_reg()/option\_setup\_unreg()** used regist/unregist option setup UI, the id used the left tree in puss option setup dialog.
> for example: create a plug-in, need insert two setup UI into puss option dialog.
```
// when crate plugin
puss->option_setup_reg("test", "test setup", (CreateSetupWidget)create_setup_1, self, 0); // default
puss->option_setup_reg("test.one", "test setup sub1", (CreateSetupWidget)create_setup_ui_1, self, 0);
puss->option_setup_reg("test.two", "test setup sub2", (CreateSetupWidget)create_setup_ui_2, self, 0);

// when destroy plugin
puss->option_setup_unreg("test");
puss->option_setup_unreg("test.one");
puss->option_setup_unreg("test.two");
```

#### ExtendManager ####

Interface Define
```
gpointer extend_query(const gchar* ext_name, const gchar* interface_name);
```

Interface Usage
  * **extend\_query()** used to query extend C/C++ interface when write plug-in. it's unusually.
> > The interface defined header file in puss/bin/include. Less PussExtend used export interface, but it'will help us create common environment, for example, we can create Common Debug Environment(UI/Toolkit and so on), it will help make different language-debug plug-ins.


#### PluginManager ####

Interface Define
```
void plugin_engine_regist( const gchar* key
		, PluginLoader loader
		, PluginUnloader unloader
		, PluginEngineDestroy destroy
		, gpointer tag );
```

Interface Usage
  * **plugin\_engine\_regist()** used create PussPluginEngine, PussExtend is the best place to create PussPluginEngine, so this interface only used when create PluginEngine extend.