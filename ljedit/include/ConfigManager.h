// ConfigManager.h
//

#ifndef LJED_INC_CONFIGMANAGER_H
#define LJED_INC_CONFIGMANAGER_H

#include <string>
#include <sigc++/sigc++.h>

typedef sigc::signal<void, const std::string&, const std::string&, const std::string&> OptionValueChangeSignal;

class ConfigManager {
protected:
	virtual ~ConfigManager() {}

public:
	virtual void regist_option( const std::string& id	// aa.bb.cc
		, const std::string& type						// typeid:typeinfo
		, const std::string& default_value
		, const std::string& tip ) = 0;

	virtual bool get_option_value(const std::string& id, std::string& value) = 0;

	virtual OptionValueChangeSignal& signal_option_changed() = 0;

public:
	bool get_option_value_bool(const std::string& id, bool& value) {
		std::string v;
		if( get_option_value(id, v) ) {
			value = v=="true";
			return true;
		}
		return false;
	}

	int get_option_value_int(const std::string& id, int& value) {
		std::string v;
		if( get_option_value(id, v) ) {
			value = atoi(v.c_str());
			return true;
		}
		return false;
	}
};

#endif//LJED_INC_CONFIGMANAGER_H

