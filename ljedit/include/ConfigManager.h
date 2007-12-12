// ConfigManager.h
//

#ifndef LJED_INC_CONFIGMANAGER_H
#define LJED_INC_CONFIGMANAGER_H

#include <string>

class ConfigManager {
public:
	virtual void regist_option(const std::string& id, const std::string& type, const std::string& default_value) = 0;

	virtual bool get_option_value(const std::string& id, std::string& value) = 0;
};

#endif//LJED_INC_CONFIGMANAGER_H

