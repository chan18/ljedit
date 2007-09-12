// searcher.cpp
// 
#include "crt_debug.h"

#include "searcher.h"

#include <sstream>
#include <list>

namespace {

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

struct SKey {
	char		type;
	std::string	value;
};

class SPath {
public:
	SPath() : cur_(0) {}

    bool empty() const { return path_.empty(); }

    size_t size() const { return path_.size(); }

	SKey& operator [] (size_t pos) {
        assert( pos < size() );
        return path_[pos];
    }

	const SKey& operator [] (size_t pos) const {
        assert( pos < size() );
        return path_[pos];
    }

    bool has_next() const { return (cur_ + 1) < size(); }

    void next() {
		assert( has_next() );
		++cur_;
    }

	SKey& cur()				{ return (*this)[pos()]; }

	size_t pos() const		{ return cur_; }
	void pos(size_t val)	{ cur_ = val; }
	void reset()			{ pos(0); }

	std::string key() {
		std::ostringstream oss;
		std::vector<SKey>::iterator it  = path_.begin();
		std::vector<SKey>::iterator end = path_.end();
		for( ; it!=end; ++it )
			oss << ':' << it->type << it->value;
		return oss.str();
	}

public:
    static bool parse(SPath& out, const std::string& path) {
		size_t ps = 0;
		size_t pe = 0;
		SKey key;

		if( path.size() >= 2 && path[0]==':' && path[1]==':' ) {
			key.value.clear();
			key.type = 'R';
			out.path_.push_back(key);
			ps = 2;
		}

		while( ps < path.size() ) {
			// key
			for( pe = ps; pe < path.size(); ++pe ) {
				char ch = path[pe];
				if( ch > 0 && (::isalnum(ch) || ch=='_') )
					continue;
				break;
			}

			if( path.compare(ps, (pe-ps), "this")==0 ) {
				key.value.clear();
			} else {
				key.value.assign(path, ps, pe-ps);
			}
			ps = pe;

			// type
			if( ps < path.size() ) {
				char ch = path[ps++];
				char ch_next = (ps < path.size()) ? path[ps] : '\0';
				switch( ch ) {
				case ':':
					if( ch_next != ':' )
						return false;
					if( key.value.empty() )
						return false;
					key.type = 'n';
					out.path_.push_back(key);
					++ps;
					break;

				case '-':
					if( ch_next!='>' )
						return false;
					++ps;
					// no break;

				case '.':
					if( key.value.empty() )
						break;
					key.type = 'v';
					out.path_.push_back(key);
					break;

				case '(':
					if( key.value.empty() )
						return false;
					if( ch_next=='\0' ) {
						key.type = '?';
						out.path_.push_back(key);
					} else if( ch_next==')' ) {
						key.type = '?';
						out.path_.push_back(key);
						++ps;

					} else {
						return false;
					}
					break;

				case '<':
					if( key.value.empty() )
						return false;
					if( ch_next != '>' )
						return false;
					++ps;
					break;

				case '[':
					if( key.value.empty() )
						return false;
					if( ch_next != ']' )
						return false;
					++ps;
					key.type = 'v';
					out.path_.push_back(key);
					break;

				case '$':
					key.type = '*';
					out.path_.push_back(key);
					return true;

				default:
					return false;
				}

			} else {
				key.type = 'S';	// startswith
			}
        }

		return true;
    }

	bool create(const SPath& path, size_t rep_begin, size_t rep_end, const std::vector<SKey>& rep) {
		size_t i;
		char last = '?';
		bool ok = true;
		for( i=0; ok && i<rep_begin; ++i ) {
			ok = follow_type(last, path[i].type);
			if( ok ) {
				path_.push_back(path[i]);
				path_.back().type = last;
			}
		}

		for( i=0; ok && i<rep.size(); ++i ) {
			path_.push_back(rep[i]);
			ok = follow_type(last, rep[i].type);
			if( ok ) {
				path_.back().type = last;
			}
		}

		for( i=rep_end; ok && i<path.size(); ++i ) {
			ok = follow_type(last, path[i].type);
			if( ok ) {
				path_.push_back(path[i]);
				path_.back().type = last;
			}
		}

		return ok;
	}

private:
    std::vector<SKey>	path_;
    size_t				cur_;
};

void loop_insert(std::list<SPath>& paths, SPath& path, const std::vector<SKey>& rep) {
	assert( !rep.empty() );

	size_t ps = rep[0].type=='R' ? 0 : path.pos();
	size_t pe = path.pos() + 1;
	for( ; ps > 0; --ps) {
		SPath rep_path;
		if( rep_path.create(path, ps, pe, rep) )
			paths.push_back(rep_path);
	}
}

void key_to_rep(std::vector<SKey>& out, const std::string& key, char mtype, char etype) {
	assert( !key.empty() );
	out.clear();

	size_t ps = 0;
	size_t pe = 0;
	SKey skey;

	if( key[0]=='.' ) {
		skey.value.clear();
		skey.type = 'R';
		out.push_back(skey);
		++ps;
	}

	while( ps != key.npos ) {
		pe = key.find('.', ps);
		if( pe!=key.npos ) {
			skey.type = mtype;
			skey.value.assign(key, ps, pe-ps);
			++pe;
		} else {
			skey.type = mtype;
			skey.value.assign(key, ps, key.size()-ps);
		}
		out.push_back(skey);
		ps = pe;
	}
}

inline void typekey_to_rep(std::vector<SKey>& out, const std::string& typekey) {
	key_to_rep(out, typekey, '?', 't');
}

inline void loop_insert_typekey(std::list<SPath>& paths, SPath& path, const std::string& typekey) {
	if( !typekey.empty() ) {
		std::vector<SKey> rep;
		typekey_to_rep(rep, typekey);
		loop_insert(paths, path, rep);
	}
}

inline void ns_to_rep(std::vector<SKey>& out, const std::string& nskey) {
	key_to_rep(out, nskey, 'n', 'n');
}

inline void loop_insert_nskey(std::list<SPath>& paths, SPath& path, const std::string& nskey) {
	if( !nskey.empty() ) {
		std::vector<SKey> rep;
		ns_to_rep(rep, nskey);
		loop_insert(paths, path, rep);
	}
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
	void locate(cpp::Scope& scope, size_t line, SPath& path);

	void do_locate(cpp::Scope& scope
		, size_t line
		, SPath& path
		, bool& need_walk);

	void walk(cpp::File* file, SPath& path);

	void do_walk(cpp::Scope& scope, SPath& path);

private:	// inner
	typedef std::set<cpp::File*>			FileSet;
	typedef std::map<std::string, FileSet>	WalkedFileMap;

	bool walked(cpp::File* f, SPath& path) {
		if( f != 0 ) {
			FileSet& files = walked_[path.key()];
			FileSet::iterator it = files.find(f);
			if( it==files.end() ) {
				files.insert(f);
				return false;
			}
		}
		return true;
	}

private:
	std::list<SPath>			paths_;
	WalkedFileMap				walked_;
	std::set<cpp::Scope*>		walked_scopes_;
	IMatched&					cb_;
	bool						searching_;
};

void Searcher::start(const std::string& path, cpp::File& file, size_t line) {
	if( path.size() < 2 )
		return;

	SPath key;
	if( !SPath::parse(key, path) )
		return;

	searching_ = true;
	locate(file.scope, line, key);

	// 先不进行优化操作
	while( !paths_.empty() ) {
		SPath key = paths_.front();
		paths_.erase(paths_.begin());
		key.reset();

		walk(&file, key);
	}
}

void Searcher::locate(cpp::Scope& scope, size_t line, SPath& path) {
	assert( !path.empty() );

	bool need_walk = true;
	if( line != 0 && path[0].type!='R' )
		do_locate(scope, line, path, need_walk);

	if( need_walk )
		paths_.push_back(path);
}

// 上下文相关搜索, 定位key所在的域, 并搜索定位时所经过的路径
// 
void Searcher::do_locate(cpp::Scope& scope
	, size_t line
	, SPath& path
	, bool& need_walk)
{
	assert( path.pos() < path.size() );
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
				if( path.cur().type!='L' ) {
					size_t pos = path.pos();
					do_walk(r.impl, path);
					path.pos(pos);
				}

				if( !r.nskey.empty() ) {
					std::vector<SKey>	rep;
					typekey_to_rep(rep, r.nskey);
					
					size_t ps = rep[0].type=='R' ? 0 : path.pos();
					size_t pe = path.pos();
					SPath rep_path;
					if( rep_path.create(path, ps, pe, rep) )
						paths_.push_back(rep_path);
				}
			}
			break;

		case cpp::ET_CLASS: {
				cpp::Class& r = (cpp::Class&)elem;
				if( !r.scope.empty() ) {
					std::vector<SKey>	rep(1);
					SKey& skey = rep.back();
					skey.type = 't';
					skey.value = r.name;

					size_t ps = rep[0].type=='R' ? 0 : path.pos();
					size_t pe = path.pos() + 1;
					SPath rep_path;
					if( rep_path.create(path, ps, pe, rep) ) {
						rep_path.pos(path.pos() + 1);
						do_locate(r.scope, line, rep_path, need_walk);

						if( need_walk ) {
							if( path.cur().type=='L' )
								need_walk = false;
							paths_.push_back(rep_path);
						}
					}
				}
			}
			break;

		case cpp::ET_NAMESPACE: {
				cpp::Namespace& r = (cpp::Namespace&)elem;
				if( !r.scope.empty() ) {
					std::vector<SKey>	rep(1);
					SKey& skey = rep.back();
					skey.type = 'n';
					skey.value = r.name;

					size_t ps = rep[0].type=='R' ? 0 : path.pos();
					size_t pe = path.pos() + 1;
					SPath rep_path;
					if( rep_path.create(path, ps, pe, rep) ) {
						rep_path.pos(path.pos() + 1);
						do_locate(r.scope, line, rep_path, need_walk);

						if( need_walk )
							paths_.push_back(rep_path);
					}
				}
			}
			break;
		}
		break;	// only find first cpp::Element
	}
}

void Searcher::walk(cpp::File* file, SPath& path) {
	if( walked(file, path) )
		return;

	if( path.size() > 8 )
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
    char type = path.cur().type;
	if( type=='L' )
		return;

	if( type=='R' ) {
		if( !path.has_next() )
			return;
		path.next();
    	type = path.cur().type;
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

	std::string& key = path.cur().value;

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
							path.cur().type = 't';
							assert( !ps->empty() );
							loop_insert_typekey(paths_, path, *ps);
							path.cur().type = type;
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
					if( elem.type==cpp::ET_CLASS && path.cur().type=='L' && path.has_next() )
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

