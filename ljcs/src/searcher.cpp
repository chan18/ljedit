// searcher.cpp
// 
#include "crt_debug.h"

#include "searcher.h"

#include <sstream>
#include <list>

namespace {

class SPath {
public:
	SPath(const std::string& path) : cur_(0) {
        parse(path);
    }

    bool empty() const { return path_.empty(); }

    size_t size() const { return path_.size(); }

    std::string& operator [] (size_t pos) {
        assert( pos < size() );
        return path_[pos];
    }

    bool has_next() const { return (cur_ + 1) < size(); }

    void next() {
		assert( has_next() );
		++cur_;
    }

	std::string& cur()		{ return (*this)[pos()]; }

	size_t pos() const		{ return cur_; }
	void pos(size_t val)	{ cur_ = val; }
	void reset()			{ pos(0); }

private:
    void parse(const std::string& path) {
        if( path.size() < 2 )
            return;

        size_t ps = 0;
        while( ps != path.npos && (ps + 2) <= path.size() ) {
            size_t pe = path.find(':', ps + 2);
            path_.push_back( path.substr(ps, (pe - ps)) );
            ps = pe;
        }
    }

private:
    std::vector<std::string>    path_;
    size_t cur_;
};

// type的含义为:
//    n : namespace
//    ? : namespace or type(class or enum or typedef)
//    t : type
//    f : function return type
//    v : var datatype
//    R : root node
//    L : this
//    S : search startswith
//    
bool follow_type(char& last, char cur) {
	switch(last) {
	case 't':
		switch(cur) {
		case 'n':	return false;
		case '?':	last = 't';	return true;
		}
		break;
	case 'S':
		return false;
	}

	last = cur;
	return true;
}

void loop_insert(std::vector<std::string>& paths, SPath& path, const std::string& rep) {
    assert( rep.size() >= 2 );
    size_t root_num = (rep[1]=='R') ? 1 : path.pos() + 1;
    for( ; root_num > 0; --root_num ) {
        size_t i;
        std::ostringstream oss;
		char last = '?';
		bool ok = true;
		for( i=0; ok && i<(root_num - 1); ++i ) {
			ok = follow_type(last, path[i][1]);
			if( ok ) {
				oss << ':' << last;
				oss.write(&path[i][2], std::streamsize(path[i].size() - 2));
			}
		}

		for( i=0; ok && i<rep.size(); ++i ) {
			oss << rep[i];
			if( rep[i]==':' ) {
				++i;
				assert( i < rep.size() );
				ok = follow_type(last, rep[i]);
				if( ok )
					oss << last;
			}
		}

		for( i=path.pos() + 1; ok && i<path.size(); ++i ) {
			ok = follow_type(last, path[i][1]);
			if( ok ) {
				oss << ':' << last;
				oss.write(&path[i][2], std::streamsize(path[i].size() - 2));
			}
		}

		if( ok )
			paths.push_back(oss.str());
    }
}

std::string key_to_rep(const std::string& key, char mtype, char etype) {
	assert( !key.empty() );
	size_t ps = 0;
	std::ostringstream oss;
	if( key[0]=='.' ) {
		oss << ":R";
		++ps;
	}
	
	size_t pe = ps;
	while( ps != key.npos ) {
		pe = key.find('.', ps);
		if( pe!=key.npos ) {
			oss << ':' << mtype << key.substr(ps, pe-ps);
			++pe;
		} else {
			oss << ':' << etype << key.substr(ps);
		}
		ps = pe;
	}
	return oss.str();
}

inline std::string typekey_to_rep(const std::string& typekey) {
	return key_to_rep(typekey, '?', 't');
}

inline void loop_insert_typekey(std::vector<std::string>& paths, SPath& path, const std::string& typekey) {
	if( !typekey.empty() )
		loop_insert(paths, path, typekey_to_rep(typekey));
}

inline std::string ns_to_rep(const std::string& nskey) {
	return key_to_rep(nskey, 'n', 'n');
}

inline void loop_insert_nskey(std::vector<std::string>& paths, SPath& path, const std::string& nskey) {
	if( !nskey.empty() )
		loop_insert(paths, path, ns_to_rep(nskey));
}

class Searcher {
public:
	Searcher(IMatched& cb) : cb_(cb), searching_(false) {}

	void start(const std::string& path, cpp::File& file, size_t line = 0);

	bool searching() const { return searching_; }

	void stop() { searching_ = false; }

	void add_matched(cpp::Element* elem) {
		assert( elem != 0 );
		if( elem->type != cpp::ET_NCSCOPE )
			cb_.on_matched(*elem);
	}

private:
	void locate(cpp::Scope& scope, size_t line, const std::string& path);

	void do_locate(cpp::Scope& scope
		, size_t line
		, const std::string& path
		, const std::string& ns
		, bool& need_walk);

	void walk(cpp::File* file, const std::string& path);

	void do_walk(cpp::Scope& scope, SPath& path);

private:	// inner
	typedef std::set<cpp::File*>			FileSet;
	typedef std::map<std::string, FileSet>	WalkedFileMap;

	bool walked(cpp::File* f, const std::string& path) {
		if( f != 0 ) {
			FileSet& files = walked_[path];
			FileSet::iterator it = files.find(f);
			if( it==files.end() ) {
				files.insert(f);
				return false;
			}
		}
		return true;
	}

private:
	std::vector<std::string>	paths_;
	WalkedFileMap				walked_;
	std::set<cpp::Scope*>		walked_scopes_;
	IMatched&					cb_;
	bool						searching_;
};

void Searcher::start(const std::string& path, cpp::File& file, size_t line) {
	if( path.size() < 2 )
		return;

	searching_ = true;

	locate(file.scope, line, path);

	// 先不进行优化操作
	while( !paths_.empty() ) {
		std::string key = paths_[0];
		paths_.erase(paths_.begin());

		//std::cout << "walk : " << key << std::endl;

		walk(&file, key);
	}
}

void Searcher::locate(cpp::Scope& scope, size_t line, const std::string& path) {
	static const std::string root_ns = "";

	assert( path.size() >= 2 );

	bool need_walk = true;
	if( line != 0 && path[1]!='R' )
		do_locate(scope, line, path, root_ns, need_walk);

	if( need_walk )
		paths_.push_back(path);
}

// 上下文相关搜索, 定位key所在的域, 并搜索定位时所经过的路径
// 
void Searcher::do_locate(cpp::Scope& scope
	, size_t line
	, const std::string& path
	, const std::string& ns
	, bool& need_walk)
{
	assert( path.size() >= 2 );

	cpp::Elements::iterator it = scope.elems.begin();
	cpp::Elements::iterator end = scope.elems.end();
	for( ; searching_ && it!=end; ++it ) {
		cpp::Element& elem = **it;
		if( line < elem.sline)
			break;

		if( line > elem.eline )
			continue;

		switch( elem.type ) {
		case cpp::ET_FUN: {
				cpp::Function& r = (cpp::Function&)elem;
				if( path[1]!='L' ) {
					SPath spath(path);
					do_walk(r.impl, spath);
				}

				if( !r.nskey.empty() )
					paths_.push_back(ns + typekey_to_rep(r.nskey) + path);
			}
			break;

		case cpp::ET_CLASS: {
				cpp::Class& r = (cpp::Class&)elem;
				if( !r.scope.empty() ) {
					std::string cur_ns = ns + ":t" + r.name;
					do_locate(r.scope, line, path, cur_ns, need_walk);

					if( need_walk ) {
						if( path[1]=='L' )
							need_walk = false;
						paths_.push_back(cur_ns + path);
					}
				}
			}
			break;

		case cpp::ET_NAMESPACE: {
				cpp::Namespace& r = (cpp::Namespace&)elem;
				if( !r.scope.empty() ) {
					std::string cur_ns = ns + ":n" + r.name;
					do_locate(r.scope, line, path, cur_ns, need_walk);

					if( need_walk )
						paths_.push_back(cur_ns + path);
				}
			}
			break;
		}
		
		break;	// only find first cpp::Element
	}
}

void Searcher::walk(cpp::File* file, const std::string& path) {
	if( path.size() < 2 || walked(file, path) )
		return;

	if( path.size() > 128 )
		return;

	assert( file != 0 );

	SPath tpath(path);
	do_walk(file->scope, tpath);

	cpp::Includes::iterator it = file->includes.begin();
	cpp::Includes::iterator end = file->includes.end();
	for( ; searching() && (it != end); ++it ) {
		assert( *it != 0 );
		if( (*it)->include_file.empty() )
			continue;

		cpp::File* incfile = ParserEnviron::self().abspath_find_parsed((*it)->include_file);
		walk( incfile, path );
	}
}

void Searcher::do_walk(cpp::Scope& scope, SPath& path) {
    char type = path.cur()[1];
	if( type=='L' )
		return;

	if( type=='R' ) {
		if( !path.has_next() )
			return;
		path.next();
    	type = path.cur()[1];
	}

    if( walked_scopes_.find(&scope) == walked_scopes_.end() ) {
        walked_scopes_.insert(&scope);

		if( type=='?' || type=='n' ) {
			if( !scope.usings.empty() ) {
				cpp::Usings::iterator ps = scope.usings.begin ();
				cpp::Usings::iterator pe = scope.usings.end();
				for( ; ps != pe; ++ps ) {
					cpp::Using& elem = **ps;
					loop_insert_nskey(paths_, path, elem.nskey);
				}
			}
		}
    }

	std::string key = path.cur().substr(2);

	if( type=='S' ) {
		// search start with
		if( path.has_next() )	// bad logic
			return;

		cpp::IndexMap::iterator it = scope.imap.begin();
		cpp::IndexMap::iterator end = scope.imap.end();
		for( ; searching() && (it != end); ++it ) {
			if( (*it)->size() < key.size() )
				continue;

			if( (*it)->compare(0, key.size(), key)==0 )
				add_matched(&scope.imap.get(it));
		}
		
	} else {
		cpp::IndexMap::iterator it = scope.imap.lower_bound(key);
		cpp::IndexMap::iterator end = scope.imap.upper_bound(key);
		for( ; searching() && (it != end); ++it ) {
			cpp::Element& elem = scope.imap.get(it);
			if( path.has_next() ) {
				cpp::Scope* sub = 0;

				switch( elem.type ) {
				case cpp::ET_NCSCOPE:
					if( type=='?' || type=='t' || type=='n' ) {
						sub = &(((cpp::NCScope&)elem).scope);
					}
					break;
				case cpp::ET_CLASS:
					if( type=='?' || type=='t' ) {
						cpp::Class& r = (cpp::Class&)elem;
						std::vector<std::string>::iterator ps = r.inhers.begin();
						std::vector<std::string>::iterator pe = r.inhers.end();
						for( ; ps != pe; ++ps ) {
							path.cur()[1] = 't';
							assert( !ps->empty() );
							loop_insert_typekey(paths_, path, *ps);
							path.cur()[1] = type;
						}
						
						sub = &r.scope;
					}
					break;
				case cpp::ET_ENUM:
					if( type=='?' || type=='t' ) {
						sub = &(((cpp::Enum&)elem).scope);
					}
					break;
				case cpp::ET_NAMESPACE:
					if( type=='?' || type=='n' ) {
						sub = &(((cpp::Namespace&)elem).scope);
					}
					break;
				case cpp::ET_TYPEDEF:
					if( type=='?' || type=='t' ) {
						loop_insert_typekey(paths_, path, ((cpp::Typedef&)elem).typekey);
					}
					break;
				case cpp::ET_FUN:
					if( type=='f' ) {
						loop_insert_typekey(paths_, path, ((cpp::Function&)elem).typekey);
					}
					break;
				case cpp::ET_VAR:
					if( type=='v' ) {
						loop_insert_typekey(paths_, path, ((cpp::Var&)elem).typekey);
					}
					break;
				case cpp::ET_USING:
					if( type=='?' || type=='t' ) {
						assert( !((cpp::Using&)elem).isns );
						loop_insert_nskey(paths_, path, ((cpp::Using&)elem).nskey);
					}
					break;
				}

				if( sub!=0 ) {
					size_t old = path.pos();
					path.next();
					if( elem.type==cpp::ET_CLASS && path.cur()[1]=='L' && path.has_next() )
						path.next();
							
					do_walk(*sub, path);
					path.pos(old);
				}

			} else {
				switch( elem.type ) {
				case cpp::ET_USING:
					if( type=='?' || type=='t' || type=='*' ) {
						assert( !((cpp::Using&)elem).isns );
						loop_insert_typekey(paths_, path, elem.name);
					}
					break;
				}
				
				add_matched(&elem);
			}
		}
	}
}

}//anonymous namespace

// start search
// 
void search( const std::string& key
	, IMatched& cb
	, cpp::File& file
	, size_t line )
{
	Searcher searcher(cb);
	searcher.start(key, file, line);
}

void search_keys(const StrVector& keys, IMatched& cb, cpp::File& file, size_t line) {
	MatchedSet mset;

	// search
	{
		StrVector::const_iterator it = keys.begin();
		StrVector::const_iterator end = keys.end();
		for( ; it!=end; ++it )
			search(*it, mset, file, line);
	}

	// return
	{
		cpp::ElementSet::iterator it = mset.elems.begin(); 
		cpp::ElementSet::iterator end = mset.elems.end(); 
		for( ; it!=end; ++it )
			cb.on_matched(**it);
	}
}

