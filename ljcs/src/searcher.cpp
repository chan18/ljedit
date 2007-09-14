// searcher.cpp
// 
#include "crt_debug.h"

#include "searcher.h"

#include <sstream>
#include <list>

#ifdef WIN32
	#include <Windows.h>
	
	inline size_t lj_get_current_time() { return GetTickCount(); }
	
#else
	#include <sys/time.h>
	
	inline size_t lj_get_current_time() {
		timeval tv;
		gettimeofday(&tv, 0);
		return tv.tv_usec;
	}

#endif

class ljdebug_time_count {
public:
	ljdebug_time_count(size_t& val) : val_(val)
		{ s_ = lj_get_current_time(); }
		
	~ljdebug_time_count()
		{ val_ += (lj_get_current_time() - s_); }

private:
	size_t& val_;
	size_t	s_;
};

size_t __search_call_time__ = 0;

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

	void swap(SPath& o) {
		path_.swap(o.path_);
		cur_ = o.cur_;
	}

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

	void get_union_key(std::string& key) {
		key.clear();
		std::vector<SKey>::iterator it  = path_.begin();
		std::vector<SKey>::iterator end = path_.end();
		for( ; it!=end; ++it ) {
			key += ':';
			key += it->type;
			key += it->value;
		}
	}

public:
    bool parse(const std::string& path) {
		char ch = '\0';
		char ch_next = '\0';
		size_t ps = 0;
		size_t pe = 0;
		SKey key;

		if( path.size() >= 2 && path[0]==':' && path[1]==':' ) {
			if( path.size() > 2 ) {
				ch = path[2];
				if( ch<0 || !(::isalpha(ch) || ch=='_') )
					return false;
			}

			key.type = 'R';
			path_.push_back(key);
			ps = 2;

			if( path.size()==2 ) {
				key.type = 'S';
				key.value.clear();
				path_.push_back(key);
				return true;
			}

		} else if( path.size() >=4 && path.compare(0, 4, "this")==0 ) {
			if( path.size()==4 || path[4]<0 || !(path[4]=='_' || ::isalnum(path[4])) ) {
				key.type = 'L';
				path_.push_back(key);
				ps = 4;
			}
		}

		while( ps < path.size() ) {
			// key
			for( pe = ps; pe < path.size(); ++pe ) {
				ch = path[pe];
				if( ch > 0 && (::isalnum(ch) || ch=='_') )
					continue;
				break;
			}

			key.value.assign(path, ps, pe-ps);
			ps = pe;

			// type
			if( ps < path.size() ) {
				ch = path[ps++];
				ch_next = (ps < path.size()) ? path[ps] : '\0';
				switch( ch ) {
				case ':':
					if( key.value.empty() )
						return false;
					if( ch_next != ':' )
						return false;

					key.type = '?';
					path_.push_back(key);

					++ps;
					ch_next = (ps < path.size()) ? path[ps] : '\0';

					if( ch_next=='\0' ) {
						key.type = 'S';
						key.value.clear();
						path_.push_back(key);
					}
					break;

				case '-':
					if( ch_next!='>' )
						return false;
					++ps;
					ch_next = (ps < path.size()) ? path[ps] : '\0';
					// no break;

				case '.':
					if( !key.value.empty() ) {
						key.type = 'v';
						path_.push_back(key);
					}

					if( ch_next=='\0' ) {
						key.type = 'S';
						key.value.clear();
						path_.push_back(key);
					}
					break;

				case '(':
					if( key.value.empty() )
						return false;

					if( ch_next=='\0' ) {
						key.type = 'f';
						path_.push_back(key);

					} else if( ch_next==')' ) {
						key.type = 'f';
						path_.push_back(key);
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
					path_.push_back(key);
					break;

				case '$':
					key.type = '*';
					path_.push_back(key);
					return true;

				default:
					return false;
				}

			} else {
				key.type = 'S';
				path_.push_back(key);
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

typedef std::list<SPath*>	SPathList;

void loop_insert(SPathList& paths, SPath& path, const std::vector<SKey>& rep) {
	assert( !rep.empty() );

	int ps = rep[0].type=='R' ? 0 : (int)path.pos();
	int pe = (int)path.pos() + 1;
	for( ; ps >= 0; --ps) {
		SPath* rep_path = new SPath;
		if( rep_path->create(path, ps, pe, rep) )
			paths.push_back(rep_path);
		else
			delete rep_path;
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

inline void loop_insert_typekey(SPathList& paths, SPath& path, const std::string& typekey) {
	if( !typekey.empty() ) {
		std::vector<SKey> rep;
		typekey_to_rep(rep, typekey);
		loop_insert(paths, path, rep);
	}
}

inline void ns_to_rep(std::vector<SKey>& out, const std::string& nskey) {
	key_to_rep(out, nskey, 'n', 'n');
}

inline void loop_insert_nskey(SPathList& paths, SPath& path, const std::string& nskey) {
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
	void locate(cpp::Scope& scope, size_t line, const std::string& path);

	void do_locate(cpp::Scope& scope
		, size_t line
		, SPath& path
		, bool& need_walk);

	void walk(cpp::File* file, SPath& path);

	void do_walk(cpp::Scope& scope, SPath& path);

private:
	SPathList				paths_;
	std::set<std::string>	walked_spaths_;
	std::set<cpp::Scope*>	walked_scopes_;
	IMatched&				cb_;
	bool					searching_;

private:	// inner
	// for walk()
	typedef std::set<cpp::File*>			FileSet;

	FileSet					__walked_;
};

void Searcher::start(const std::string& path, cpp::File& file, size_t line) {
	if( path.empty() )
		return;

	searching_ = true;
	
	ljdebug_time_count __count__(__search_call_time__);
	
	locate(file.scope, line, path);

	std::string key;

	// 先不进行优化操作
	while( !paths_.empty() ) {
		SPath* spath = paths_.front();
		paths_.erase(paths_.begin());

		spath->get_union_key(key);
		if( walked_spaths_.find(key)==walked_spaths_.end() ) {
			walked_spaths_.insert(key);

			__walked_.clear();
			walk(&file, *spath);
		}

		delete spath;
	}
}

void Searcher::locate(cpp::Scope& scope, size_t line, const std::string& path) {
	assert( !path.empty() );

	SPath* spath = new SPath();
	if( spath == 0 )
		return;

	if( !spath->parse(path) ) {
		delete spath;
		return;
	}

	bool need_walk = true;
	if( line != 0 && (*spath)[0].type!='R' )
		do_locate(scope, line, *spath, need_walk);

	if( need_walk )
		paths_.push_back(spath);
	else
		delete spath;
}

// 上下文相关搜�? 定位key所在的�? 并搜索定位时所经过的路�?// 
void Searcher::do_locate(cpp::Scope& scope
	, size_t line
	, SPath& path
	, bool& need_walk)
{
	if( path.pos() >= path.size() )
		return;

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
					SPath* rep_path = new SPath;
					if( rep_path->create(path, ps, pe, rep) )
						paths_.push_back(rep_path);
					else
						delete rep_path;
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
					size_t pe = path.pos();
					SPath* rep_path = new SPath;
					if( rep_path->create(path, ps, pe, rep) ) {
						rep_path->pos(path.pos() + 1);
						do_locate(r.scope, line, *rep_path, need_walk);

						if( need_walk ) {
							if( path.cur().type=='L' )
								need_walk = false;
							paths_.push_back(rep_path);
						} else {
							delete rep_path;
						}
					} else {
						delete rep_path;
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
					size_t pe = path.pos();
					SPath* rep_path = new SPath;
					if( rep_path->create(path, ps, pe, rep) ) {
						rep_path->pos(path.pos() + 1);
						do_locate(r.scope, line, *rep_path, need_walk);

						if( need_walk )
							paths_.push_back(rep_path);
						else
							delete rep_path;
					} else {
						delete rep_path;
					}
				}
			}
			break;
		}
		break;	// only find first cpp::Element
	}
}

void Searcher::walk(cpp::File* file, SPath& path) {
	if( __walked_.find(file) != __walked_.end() )
		return;
	__walked_.insert(file);

	path.reset();

	if( path.size() > 8 )
		return;

	assert( file != 0 );

	do_walk(file->scope, path);

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

		return;
	}

	cpp::IndexMap::iterator it = scope.imap.lower_bound(key);
	cpp::IndexMap::iterator end = scope.imap.upper_bound(key);
	for( ; searching() && (it != end); ++it ) {
		cpp::Element& elem = scope.imap.get(it);
		if( !path.has_next() ) {
			switch( elem.type ) {
			case cpp::ET_USING:
				if( type=='?' || type=='t' || type=='*' ) {
					assert( !((cpp::Using&)elem).isns );
					loop_insert_typekey(paths_, path, elem.name);
				}
				break;
			}
			
			add_matched(&elem);
			continue;
		}

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

	__search_call_time__ = 0;

	searcher.start(key, file, line);
	
	//ljdebug_trace(...);
	
	printf("search : %d\n", __search_call_time__);

#ifdef WIN32
	char buf[512];
	sprintf(buf, "search : %d\n", __search_call_time__);
	::OutputDebugStringA(buf);
#endif
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

