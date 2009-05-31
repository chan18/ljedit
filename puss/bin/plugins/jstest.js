/*
Gtk = imports.gi.Gtk

with(Gtk)
{
	var window = new Window();
	window.signal.hide.connect(function () { main_quit() });
	window.resize(300, 300);
	window.show_all();
}
*/

puss_plugin_active = function() {
	Seed.print("active");
	Seed.print("aaa");
	Seed.print(puss.plugins_path);
	puss.doc_new()
	return "aaa";
}

function puss_plugin_deactive(plugin) {
	Seed.print("deactive");
	Seed.print(plugin);
}
