// Environ.cpp
// 

#include "Environ.h"

#include <cstring>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <sys/stat.h>

// c++ keywords & macro keywords
// 
const char* cpp_keywords[] = { "asm"
	, "auto"
	, "bool"
	, "break"
	, "case"
	, "catch"
	, "char"
	, "class"
	, "const"
	, "const_cast"
	, "continue"
	, "default"
	, "define"			// macro
	, "delete"
	, "do"
	, "double"
	, "dynamic_cast"
	, "else"
	, "endif"			// macro
	, "enum"
	, "explicit"
	, "export"
	, "extern"
	, "false"
	, "float"
	, "for"
	, "friend"
	, "goto"
	, "if"
	, "ifdef"			// macro
	, "ifndef"			// macro
	, "include"			// macro
	, "inline"
	, "int"
	, "long"
	, "mutable"
	, "namespace"
	, "new"
	, "operator"
	, "private"
	, "protected"
	, "public"
	, "register"
	, "reinterpret_cast"
	, "return"
	, "short"
	, "signed"
	, "sizeof"
	, "static"
	, "static_cast"
	, "struct"
	, "switch"
	, "template"
	, "this"
	, "throw"
	, "true"
	, "try"
	, "typedef"
	, "typeid"
	, "typename"
	, "union"
	, "unsigned"
	, "using"
	, "virtual"
	, "void"
	, "volatile"
	, "wchar_t"
	, "while"
	, 0
};

Environ::Environ() : keywords_("") {
	keywords_.ref_count = 1;

	index_->set_env(this);

	for( const char** key=cpp_keywords; *key!=0; ++key ) {
		cpp::KeywordElement* elem = new cpp::KeywordElement(keywords_, *key);
		if( elem )
			keywords_.scope.elems.push_back(elem);
	}
}

Environ::~Environ() {
}

void Environ::set_include_paths(const StrVector& paths) {
	StrVector vec;
	{
		std::string path;

		StrVector::const_iterator it = paths.begin();
		StrVector::const_iterator end = paths.end();
		for( ; it!=end; ++it ) {
			assert( app_ );
			gchar* str = app_->format_filename(it->c_str());
			path = str;
			g_free(str);

			if( std::find(vec.begin(), vec.end(), path)==vec.end() )
				vec.push_back(path);
		}
	}

	include_paths_.set(vec);
}

void Environ::set_pre_parse_files(const StrVector& files) {
	StrVector vec;
	{
		std::string file;

		StrVector::const_iterator it = files.begin();
		StrVector::const_iterator end = files.end();
		for( ; it!=end; ++it ) {
			assert( app_ );
			gchar* str = app_->format_filename(it->c_str());
			file = str;
			g_free(str);

			if( file.empty() )
				continue;

			if( std::find(vec.begin(), vec.end(), file)==vec.end() )
				vec.push_back(file);
		}
	}

	pre_parse_files_.set(vec);
}

cpp::File* Environ::find_parsed(const std::string& filekey) {
	RLocker locker(parsed_files_);

	cpp::FileMap::iterator it = parsed_files_->find(filekey);
	if( it!=parsed_files_->end() ) {
		file_incref(it->second);
		return it->second;
	}
	return 0;
}

cpp::File* Environ::find_parsed_in_include_path(const std::string& filename) {
	std::string filepath;

	cpp::File* retval = 0;

	RLocker locker(include_paths_);

	StrVector::iterator it = include_paths_->begin();
	StrVector::iterator end = include_paths_->end();
	for( ; it!=end && retval==0; ++it ) {
		filepath = *it + filename;

		assert( app_ );
		gchar* str = app_->format_filename(filepath.c_str());
		filepath = str;
		g_free(str);

		retval = find_parsed(filepath);
	}
	return retval;
}

bool Environ::in_include_path(const std::string& path) {
	assert( app_ );
	gchar* str = app_->format_filename(path.c_str());
	std::string abspath = str;
	g_free(str);

	{
		RLocker locker(include_paths_);

		StrVector::iterator it = include_paths_->begin();
		StrVector::iterator end = include_paths_->end();
		while( it!=end ) {
			if( abspath.compare(0, it->size(), *it)==0 )
				return true;
			++it;
		}
	}

	return false;
}

void Environ::re_make_index() {
	RLocker locker(parsed_files_);

	index_->clear();

	cpp::FileMap::iterator it = parsed_files_->begin();
	cpp::FileMap::iterator end = parsed_files_->end();
	for( ; it!=end; ++it ) {
		index_->add( *it->second );
	}
}

void Environ::find_keyword(const std::string& key, IMatched& cb) {
	// search start with
	cpp::Elements::iterator it = keywords_.scope.elems.begin();
	cpp::Elements::iterator end = keywords_.scope.elems.end();
	for( ; it != end; ++it ) {
		if( (*it)->name.size() < key.size() )
			continue;

		if( (*it)->name.compare(0, key.size(), key)==0 )
			cb.on_matched(**it);
	}
}

void Environ::add_parsed(cpp::File* file) {
	if( file != 0 ) {
		WLocker locker(parsed_files_);
		file_incref(file);
		
		cpp::FileMap::iterator it = parsed_files_->find(file->filename);
		if( it!=parsed_files_->end() ) {
			file_decref(it->second);
			it->second = file;

		} else {
			parsed_files_->insert( std::make_pair(file->filename, file) );
		}
	}
}

void Environ::remove_parsed(cpp::File* file) {
	if( file!=0 )
		remove_parsed(file->filename);
}

void Environ::file_incref(cpp::File* file) {
	assert( file != 0 );
	g_atomic_int_add(&file->ref_count, 1);
}

void Environ::file_decref(cpp::File* file) {
	assert( file != 0 );
	g_atomic_int_add(&file->ref_count, -1);

	assert( file->ref_count >= 0 );
	if( file->ref_count==0 )
		delete file;
}

void Environ::remove_parsed(const std::string& file) {
	WLocker locker(parsed_files_);

	cpp::FileMap::iterator it = parsed_files_->find(file);
	if( it!=parsed_files_->end() ) {
		file_decref(it->second);
		parsed_files_->erase(it);
	}
}

std::set<std::string> init_exts() {
	std::set<std::string> exts;
	exts.insert("c");
	exts.insert("h");
	exts.insert("cpp");
	exts.insert("hpp");
	exts.insert("cc");
	exts.insert("hh");
	exts.insert("inl");
	exts.insert("cxx");
	exts.insert("hxx");

	return exts;
}

bool Environ::check_cpp_files(const std::string& filepath) {
	gchar* name = g_path_get_basename(filepath.c_str());
	size_t len = strlen(name);
	gchar* ptr = name + len;
	for( ; ptr!=name && *ptr != '.'; --ptr );

	bool retval = false;
	if( ptr!=name ) {
		// check ext filename is .c .h .cpp .hpp .cc .hh .inl ...
		++ptr;
		for(char* p=ptr; p!=(name+len); ++p)
			*p = tolower(*p);

		static std::set<std::string> exts = init_exts();
		retval = exts.find(ptr) != exts.end();

	} else {
		// if no ext filename, check path in system header path
		retval = in_include_path(filepath);
	}
	g_free(name);

	return retval;
}

bool Environ::pe_is_abspath(std::string& filepath) {
	return g_path_is_absolute(filepath.c_str()) != 0;
}

void Environ::pe_format_filekey(std::string& filename) {
	assert( app_ );
	gchar* str = app_->format_filename(filename.c_str());
	filename = str;
	g_free(str);
}

void Environ::pe_get_path(std::string& path, const std::string& filename) {
	gchar* str = g_path_get_dirname(filename.c_str());
	path.assign(str);
	g_free(str);
}

void Environ::pe_build_filekey(std::string& filekey, const std::string& path, const std::string& name) {
	assert( app_ );
	gchar* fname = g_build_filename(path.c_str(), name.c_str(), NULL);
	gchar* str = app_->format_filename(fname);
	filekey = str;
	g_free(str);
	g_free(fname);
}

cpp::File* Environ::pe_find_parsed(const std::string& filekey) {
	return find_parsed(filekey);
}

cpp::File* Environ::pe_check_parsed(const std::string& filekey, time_t& mtime) {
	struct stat filestat;
	memset(&filestat, 0, sizeof(filestat));
	stat(filekey.c_str(), &filestat);
	mtime = filestat.st_mtime;

	cpp::File* file = find_parsed(filekey);
	if( file!=0 ) {
		// if stat()
		//      succeed : compare file->datetime & filestat.st_mtime
		//      failed  : filestat.st_mtime==0, also compare them
		// 
		if( file->datetime != mtime ) {
			file_decref(file);
			file = 0;
		}
	}

	return file;
}

bool Environ::pe_get_content(const std::string& filekey, std::ostream& out) {
	std::string buf;

	gchar* contents = 0;
	gsize  length = 0;
	if( g_file_get_contents(filekey.c_str(), &contents, &length, NULL) ) {
		out.write(contents, length);
		return true;
	}

	return false;
}

void Environ::pe_on_parsed(cpp::File* file) {
	add_parsed(file);
}