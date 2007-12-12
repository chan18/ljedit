// ConfigManagerImpl.h
// 

#ifndef LJED_INC_CONFIGMANAGERIMPL_H
#define LJED_INC_CONFIGMANAGERIMPL_H

#include "ConfigManager.h"

class ConfigManagerImpl : public ConfigManager {
public:
	static ConfigManagerImpl& self();

private:
	ConfigManagerImpl(void* impl);
	~ConfigManagerImpl();

public:
	void create();
	void destroy();

public:	// ConfigManager interface
	virtual void regist_option(const std::string& id, const std::string& type, const std::string& default_value);

	virtual bool get_option_value(const std::string& id, std::string& value);

private:
	void* impl_;
};

#endif//LJED_INC_CONFIGMANAGERIMPL_H

