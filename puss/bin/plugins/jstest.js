/*
Seed.import_namespace("Gtk");

with(Gtk)
{
	var window = new Window();
	window.signal.hide.connect(function () { main_quit() });
	window.resize(300, 300);
	window.show_all();
}
*/

function JsTest() {
	this.a = "vvv";

	this.active = function() {
		Seed.print("active");
		Seed.print(this.a);
	}

	this.deactive = function() {
		Seed.print("deactive");
		Seed.print(this.a);
	}
}

new JsTest();
