#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include "Settings.h"
#include "Setting.h"
#include <map>

namespace GLOBAL_SETTINGS_BACKEND_NAMESPACE
{

	enum GLOBAL_SETTINGS_BACKEND
	{
		  SETTING_FIRST = 0

		, TEST_SETTING

		, SETTING_LAST
	};

}

class GlobalSettings : public Settings<GLOBAL_SETTINGS_BACKEND_NAMESPACE::GLOBAL_SETTINGS_BACKEND, BackendGlobalSetting>
{

	public:

		static int const number_worker_threads = 1; // For now, single thread only in pool

		GlobalSettings(Messager & messager, boost::filesystem::path const global_settings_path);
		virtual ~GlobalSettings() {}


	protected:

		void SetMapEntry(Messager & messager, SettingInfo & setting_info, boost::property_tree::ptree & pt);
		BackendGlobalSetting * CloneSetting(Messager & messager, BackendGlobalSetting * current_setting, SettingInfo & setting_info) const;
		BackendGlobalSetting * NewSetting(Messager & messager, SettingInfo & setting_info, void const * setting_value_void = NULL);
		void SetPTreeEntry(Messager & messager, GLOBAL_SETTINGS_BACKEND_NAMESPACE::GLOBAL_SETTINGS_BACKEND which_setting, boost::property_tree::ptree & pt);

};

#endif
