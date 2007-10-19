// rmacro.h
// 

#ifndef LJCS_RMACRO_H
#define LJCS_RMACRO_H

#include "ds.h"

#include <string>
#include <map>

class RMacro {
public:
	RMacro()
		: in_use(false)
		, args(0)
		, value(0)
		, rvalue(0) {}

	~RMacro() {
		delete args;
		delete value;
		delete rvalue;
	}

	bool			in_use;
	StrVector*		args;
	std::string*	value;
	std::string*	rvalue;

private:
	RMacro(const RMacro& o);
	RMacro& operator = (const RMacro& o);
};

typedef std::map<std::string, RMacro*> TRMacros;

class MacroMgr {
public:
	~MacroMgr() { clear(); }

	void macro_insert( cpp::Macro* macro
		, const std::string* value
		, const std::string* rvalue)
	{
		assert( macro!=0 && macro->type==cpp::ET_MACRO );

		TRMacros::iterator it = macros_.find(macro->name);
		RMacro* rmacro = 0;
		if( it!=macros_.end() ) {
			rmacro = it->second;

		} else {
			rmacro = new RMacro();
			if( rmacro==0 )
				return;
			macros_[macro->name] = rmacro;
		}

		assert( rmacro != 0 );

		// set rmacro
		rmacro->in_use = false;

		if( macro->args==0 ) {
			if( rmacro->args!=0 ) {
				delete rmacro->args;
				rmacro->args = 0;
			}

		} else {
			if( rmacro->args==0 ) {
				rmacro->args = new StrVector();
				if( rmacro->args==0 ) {
					macros_.erase(macro->name);
					return;
				}
			}

			*(rmacro->args) = *(macro->args);
		}

		if( value==0 ) {
			if( rmacro->value!=0 ) {
				delete rmacro->value;
				rmacro->value = 0;
			}

		} else {
			if( rmacro->value==0 ) {
				rmacro->value = new std::string();
				if( rmacro->value==0 ) {
					macros_.erase(macro->name);
					return;
				}
			}

			*(rmacro->value) = *value;
		}

		if( rvalue==0 ) {
			if( rmacro->rvalue!=0 ) {
				delete rmacro->rvalue;
				rmacro->rvalue = 0;
			}

		} else {
			if( rmacro->rvalue==0 ) {
				rmacro->rvalue = new std::string();
				if( rmacro->rvalue==0 ) {
					macros_.erase(macro->name);
					return;
				}
			}

			*(rmacro->rvalue) = *rvalue;
		}
	}

	void macro_remove(const std::string& name) {
		TRMacros::iterator it = macros_.find(name);
		if( it != macros_.end() ) {
			delete it->second;
			macros_.erase(it);
		}
	}

	TRMacros& macros() { return macros_; }

	RMacro* find_macro(const std::string& name) {
		RMacro* result = 0;
		TRMacros::iterator it = macros_.find(name);
		if( it != macros().end() )
			result = it->second;
		return result;
	}

	void clear() {
		TRMacros::iterator it = macros_.begin();
		TRMacros::iterator end = macros_.end();
		for( ; it!=end; ++it )
			delete it->second;
		macros_.clear();
	}

	virtual void on_include(cpp::Include& inc, const void* tag) = 0;

private:
	TRMacros				macros_;
};

#endif//LJCS_RMACRO_H

