// AboutDialog.cpp
// 

#include "AboutDialog.h"

void cb_about_activate_email(GtkAboutDialog* about, const gchar* link, gpointer data) {
	g_message("send email : %s", link);
}

#ifdef WIN32
	#include <windows.h>

	void cb_about_activate_url(GtkAboutDialog* about, const gchar* link, gpointer data)
		{ ShellExecuteA(HWND_DESKTOP, "open", link, NULL, NULL, SW_SHOWNORMAL); }
#else
	//#include <libgnome/gnome-url.h>

	void cb_about_activate_url(GtkAboutDialog* about, const gchar* link, gpointer data)
		{ /*gnome_url_show(link, NULL);*/ }
#endif


void puss_show_about_dialog(GtkWindow* window) {
	const gchar* license =
		"This library is free software; you can redistribute it and/or\n"
		"modify it under the terms of the GNU Library General Public License as\n"
		"published by the Free Software Foundation; either version 2 of the\n"
		"License, or (at your option) any later version.\n"
		"\n"
		"This software is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
		"Library General Public License for more details.\n"
		"\n";

	const gchar* authors[] = {
		  "Louis LJ"
		, NULL
	};

	gtk_about_dialog_set_email_hook(&cb_about_activate_email, 0, 0);
	gtk_about_dialog_set_url_hook(&cb_about_activate_url, 0, 0);
	gtk_show_about_dialog( window,
		"name",      "puss",
		"version",   "0.1",
		"copyright", "(C) 2007-2008 The Puss Team",
		"license",   license,
		"website",   "http://ljedit.googlecode.com",
		"authors",   authors,
		NULL );
}

