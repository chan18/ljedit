// rmacro.h
// 

#ifndef LJCS_RMACRO_H
#define LJCS_RMACRO_H

#include "ds.h"

#include <string>
#include <map>

struct RMacro {
	bool		in_use;
	cpp::Macro*	macro;

	cpp::Macro* operator->() { return macro; }
};

typedef std::map<std::string, RMacro> TRMacros;

class MacroMgr {
public:
	void macro_insert(cpp::Macro* macro) {
		assert( macro!=0 && macro->type==cpp::ET_MACRO );
		RMacro rmacro = { false, macro };
		macros_[macro->name] = rmacro;
	}

	void macro_remove(const std::string& name)
		{ macros_.erase(name); }

	TRMacros& macros() { return macros_; }

	RMacro* find_macro(const std::string& name) {
		RMacro* result = 0;
		TRMacros::iterator it = macros().find(name);
		if( it != macros().end() )
			result = &(it->second);
		return result;
	}

	void reset_macros() { macros_.clear(); }

	virtual void on_include(cpp::Include& inc, const void* tag) = 0;

private:
	TRMacros				macros_;
};

#endif//LJCS_RMACRO_H

