#ifndef MODELINPUTSETTINGS_H
#define MODELINPUTSETTINGS_H

#include "ModelSettings.h"

namespace INPUT_MODEL_SETTINGS_NAMESPACE
{

	enum INPUT_MODEL_SETTINGS
	{
		  SETTING_FIRST = 0
		, SETTING_LAST
	};

}

class InputModelSettings : public ModelSettings<INPUT_MODEL_SETTINGS_NAMESPACE::INPUT_MODEL_SETTINGS, InputModelSetting>
{

public:

	InputModelSettings(Messager & messager, boost::filesystem::path const model_settings_path)
		: ModelSettings(messager, model_settings_path)
	{}

	virtual ~InputModelSettings() {}

	void SetMapEntry(Messager & messager, SettingInfo & setting_info, boost::property_tree::ptree & pt);
	InputModelSetting * CloneSetting(Messager & messager, InputModelSetting * current_setting, SettingInfo & setting_info) const;
	InputModelSetting * NewSetting(Messager & messager, SettingInfo & setting_info, void const * setting_value_void = NULL);
	void SetPTreeEntry(Messager & messager, INPUT_MODEL_SETTINGS_NAMESPACE::INPUT_MODEL_SETTINGS which_setting, boost::property_tree::ptree & pt);

};

#endif
