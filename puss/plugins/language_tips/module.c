// module.c
// 

#include "LanguageTips.h"

PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	LanguageTips* self;

	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	self = g_new0(LanguageTips, 1);
	self->app = app;

	self->cpp_guide = cpp_guide_new(TRUE, TRUE);

	parse_thread_init(self);

	self->re_include = g_regex_new("^[ \t]*#[ \t]*include[ \t]*(.*)", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);
	self->re_include_tip = g_regex_new("([\"<])(.*)", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);
	self->re_include_info = g_regex_new("([\"<])([^\">]*)[\">].*", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);

	ui_create(self);
	preview_init(self);
	controls_init(self);

	return self;
}

PUSS_EXPORT void puss_plugin_destroy(void* ext) {
	LanguageTips* self = (LanguageTips*)ext;
	if( !self )
		return;

	controls_final(self);
	preview_final(self);
	ui_destroy(self);

	g_regex_unref(self->re_include);
	g_regex_unref(self->re_include_tip);
	g_regex_unref(self->re_include_info);

	parse_thread_final(self);

	cpp_guide_free(self->cpp_guide);

	g_free(self);
}

