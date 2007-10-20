// LJCSEnv.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "LJCSEnv.h"

#include <string>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include "LJEditor.h"


LJCSEnv& LJCSEnv::self() {
	static LJCSEnv me_;
	return me_;
}

LJCSEnv::LJCSEnv() : ljedit_(0) {
}

LJCSEnv::~LJCSEnv() {
}

void LJCSEnv::set_include_paths(const StrVector& paths) {
	StrVector vec;
	{
		std::string path;

		StrVector::const_iterator it = paths.begin();
		StrVector::const_iterator end = paths.end();
		for( ; it!=end; ++it ) {
			path = *it;

			assert( ljedit_ != 0 );
			ljedit().utils().format_filekey(path);

			if( path.empty() )
				continue;

			if( path[path.size() - 1]!='/' )
				path += '/';

			if( std::find(vec.begin(), vec.end(), path)==vec.end() )
				vec.push_back(path);
		}
	}

	include_paths_.set(vec);
}

void LJCSEnv::set_pre_parse_files(const StrVector& files) {
	StrVector vec;
	{
		std::string file;

		StrVector::const_iterator it = files.begin();
		StrVector::const_iterator end = files.end();
		for( ; it!=end; ++it ) {
			file = *it;
			ljedit().utils().format_filekey(file);

			if( file.empty() )
				continue;

			if( std::find(vec.begin(), vec.end(), file)==vec.end() )
				vec.push_back(file);
		}
	}

	pre_parse_files_.set(vec);
}

cpp::File* LJCSEnv::find_parsed(const std::string& filekey) {
	RdLocker<cpp::FileMap> locker(parsed_files_);

	cpp::FileMap::iterator it = parsed_files_->find(filekey);
	return it!=parsed_files_->end() ? it->second->ref() : 0;
}

cpp::File* LJCSEnv::find_parsed_in_include_path(const std::string& filename) {
	std::string filepath;

	cpp::File* retval = 0;

	RdLocker<StrVector>    locker(include_paths_);

	StrVector::iterator it = include_paths_->begin();
	StrVector::iterator end = include_paths_->end();
	for( ; it!=end && retval==0; ++it ) {
		filepath = *it + filename;
		ljedit().utils().format_filekey(filepath);
		retval = find_parsed(filepath);
	}
	return retval;
}

bool LJCSEnv::in_include_path(const std::string& path) {
	std::string abspath = path;
	ljedit().utils().format_filekey(abspath);

	{
		RdLocker<StrVector> locker(include_paths_);

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

void LJCSEnv::re_make_index() {
	RdLocker<cpp::FileMap> locker(parsed_files_);

	cpp::FileMap::iterator it = parsed_files_->begin();
	cpp::FileMap::iterator end = parsed_files_->end();
	for( ; it!=end; ++it ) {
		index_->add( *it->second );
	}
}

void LJCSEnv::add_parsed(cpp::File* file) {
	if( file != 0 ) {
		WrLocker<cpp::FileMap> locker(parsed_files_);

		cpp::FileMap::iterator it = parsed_files_->find(file->filename);
		if( it!=parsed_files_->end() ) {
			it->second->unref();
			it->second = file->ref();

		} else {
			parsed_files_->insert( std::make_pair(file->filename, file->ref()) );
		}
	}
}

void LJCSEnv::remove_parsed(cpp::File* file) {
	if( file!=0 )
		remove_parsed(file->filename);
}

void LJCSEnv::remove_parsed(const std::string& file) {
	WrLocker<cpp::FileMap> locker(parsed_files_);

	cpp::FileMap::iterator it = parsed_files_->find(file);
	if( it!=parsed_files_->end() ) {
		it->second->unref();
		parsed_files_->erase(it);
	}
}

bool LJCSEnv::pe_is_abspath(std::string& filepath) {
	return Glib::path_is_absolute(filepath);
}

void LJCSEnv::pe_format_filekey(std::string& filename) {
	ljedit().utils().format_filekey(filename);
}

void LJCSEnv::pe_get_path(std::string& path, const std::string& filename) {
	path = Glib::path_get_dirname(filename);
}

void LJCSEnv::pe_build_filekey(std::string& filekey, const std::string& path, const std::string& name) {
	filekey = Glib::build_filename(path, name);
	ljedit().utils().format_filekey(filekey);
}

cpp::File* LJCSEnv::pe_find_parsed(const std::string& filekey) {
	return find_parsed(filekey);
}

cpp::File* LJCSEnv::pe_check_parsed(const std::string& filekey, time_t& mtime) {
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
			file->unref();
			file = 0;
		}
	}

	return file;
}

bool LJCSEnv::pe_get_content(const std::string& filekey, std::ostream& out) {
	std::string buf;

	try {
		buf = Glib::file_get_contents(filekey);
		out << buf;
		return true;

	} catch( Glib::FileError ) {
	}

	return false;
}

void LJCSEnv::pe_on_parsed(cpp::File* file) {
	add_parsed(file);
}
