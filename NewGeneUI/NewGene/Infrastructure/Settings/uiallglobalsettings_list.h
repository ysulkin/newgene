#ifndef UIALLGLOBALSETTINGS_LIST_H
#define UIALLGLOBALSETTINGS_LIST_H

#ifndef Q_MOC_RUN
	#include <boost/algorithm/string.hpp>
	#include <boost/regex.hpp>
	#include <boost/algorithm/string/regex.hpp>
	#include <boost/filesystem.hpp>
	#include <boost/filesystem/operations.hpp>
#endif
#include "uisettingsmanager.h"
#include <fstream>


template<GLOBAL_SETTINGS_UI_NAMESPACE::GLOBAL_SETTINGS_UI SETTING>
class UIGlobalSetting_Path : public UIGlobalSetting, public PathSetting,
	public SimpleAccessSetting<UIGlobalSetting_Path<SETTING>, GLOBAL_SETTINGS_UI_NAMESPACE::GLOBAL_SETTINGS_UI, SETTING, UISettingsManager>
{

	public:

		UIGlobalSetting_Path(Messager & messager, boost::filesystem::path const & setting)
			: Setting(messager)
			, UISetting(messager)
			, GlobalSetting(messager)
			, UIGlobalSetting(messager)
			, PathSetting(messager, setting)
		{}

		std::string ToString() const { return StringSetting::ToString(); }

};

template<GLOBAL_SETTINGS_UI_NAMESPACE::GLOBAL_SETTINGS_UI SETTING>
class UIGlobalSetting_Projects_Files_List : public UIGlobalSetting, public StringSetting,
	public SimpleAccessSetting<UIGlobalSetting_Projects_Files_List<SETTING>, GLOBAL_SETTINGS_UI_NAMESPACE::GLOBAL_SETTINGS_UI, SETTING, UISettingsManager>
{

	public:

		UIGlobalSetting_Projects_Files_List(Messager & messager, std::string const & setting)
			: Setting(messager)
			, UISetting(messager)
			, GlobalSetting(messager)
			, UIGlobalSetting(messager)
			, StringSetting(messager, setting)
		{}

		virtual void DoSpecialParse(Messager & messager)
		{
			std::vector<std::string> projects;
			boost::split_regex(projects, string_setting, boost::regex(";;"));
			std::for_each(projects.begin(), projects.end(), [this, &messager](std::string & spath)
			{
				std::vector<std::string> files_;
				boost::split(files_, spath, boost::is_any_of(";"), boost::token_compress_on);

				boost::filesystem::path settings;

				try
				{
					if (files_.size() == 1)
					{
						settings = files_[0];

						bool missing {false};

						#						ifdef Q_OS_WIN32

						if (boost::filesystem::is_directory(settings.parent_path()) && boost::filesystem::windows_name(settings.filename().string()))
						#						else
						if (boost::filesystem::is_directory(settings.parent_path()) && boost::filesystem::portable_posix_name(settings.filename().string()))
						#						endif
						{
							if (!boost::filesystem::exists(settings))
							{
								missing = true;

								// NO!  Do not auto-generate non-existent files... this will be handled elsewhere
								if (false)
								{
									std::ofstream _touch;
									_touch.open(settings.string());

									if (_touch.is_open())
									{
										_touch.close();
									}
								}
								else
								{
									if (false)
									{
										messager.AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE_ENUM::MESSAGER_MESSAGE__MAIN_PROJECT_NOT_AVAILABLE, std::string("Project \"") + settings.string() + "\" is missing.  NewGene will create a default project."));
									}
								}
							}
						}

						if (boost::filesystem::is_regular_file(settings))
						{
							this->files.push_back(settings.string());
						}
						else
						{
							if (missing)
							{
								// Kluge!  Indicate that a file was saved, but is not present, by passing an empty path here
								this->files.push_back(boost::filesystem::path());
								this->files.push_back(boost::filesystem::path());
							}
						}
					}
				}
				catch (boost::filesystem::filesystem_error & e)
				{
					boost::format msg("Invalid path \"%1%\": %2%");
					msg % settings.string() % e.what();
					messager.AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE_ENUM::MESSAGER_MESSAGE__FILE_INVALID_FORMAT, msg.str()));
				}

			});
		}

		std::vector<boost::filesystem::path> files;

		std::string ToString() const { return StringSetting::ToString(); }

};


template<>
class SettingClassTypeTraits<SettingInfo::SETTING_CLASS_UI_GLOBAL_SETTING__MRU_INPUT_PROJECTS_LIST>
{
	public:
		typedef UIGlobalSetting_Projects_Files_List<GLOBAL_SETTINGS_UI_NAMESPACE::MRU_INPUT_PROJECTS_LIST> type;
};
typedef SettingClassTypeTraits<SettingInfo::SETTING_CLASS_UI_GLOBAL_SETTING__MRU_INPUT_PROJECTS_LIST>::type InputMRUFilesList;

template<>
class SettingClassTypeTraits<SettingInfo::SETTING_CLASS_UI_GLOBAL_SETTING__MRU_OUTPUT_PROJECTS_LIST>
{
	public:
		typedef UIGlobalSetting_Projects_Files_List<GLOBAL_SETTINGS_UI_NAMESPACE::MRU_OUTPUT_PROJECTS_LIST> type;
};
typedef SettingClassTypeTraits<SettingInfo::SETTING_CLASS_UI_GLOBAL_SETTING__MRU_OUTPUT_PROJECTS_LIST>::type OutputMRUFilesList;

template<>
class SettingClassTypeTraits<SettingInfo::SETTING_CLASS_UI_GLOBAL_SETTING__OPEN_INPUT_PROJECTS_LIST>
{
	public:
		typedef UIGlobalSetting_Projects_Files_List<GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_INPUT_PROJECTS_LIST> type;
};
typedef SettingClassTypeTraits<SettingInfo::SETTING_CLASS_UI_GLOBAL_SETTING__OPEN_INPUT_PROJECTS_LIST>::type InputProjectFilesList;

template<>
class SettingClassTypeTraits<SettingInfo::SETTING_CLASS_UI_GLOBAL_SETTING__OPEN_OUTPUT_PROJECTS_LIST>
{
	public:
		typedef UIGlobalSetting_Projects_Files_List<GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_OUTPUT_PROJECTS_LIST> type;
};
typedef SettingClassTypeTraits<SettingInfo::SETTING_CLASS_UI_GLOBAL_SETTING__OPEN_OUTPUT_PROJECTS_LIST>::type OutputProjectFilesList;

template<>
class SettingClassTypeTraits<SettingInfo::SETTING_CLASS_UI_GLOBAL_SETTING__OPEN_INPUT_DATASET_FOLDER_PATH>
{
	public:
		typedef UIGlobalSetting_Path<GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_INPUT_DATASET_FOLDER_PATH> type;
};
typedef SettingClassTypeTraits<SettingInfo::SETTING_CLASS_UI_GLOBAL_SETTING__OPEN_INPUT_DATASET_FOLDER_PATH>::type OpenInputFilePath;

template<>
class SettingClassTypeTraits<SettingInfo::SETTING_CLASS_UI_GLOBAL_SETTING__OPEN_OUTPUT_DATASET_FOLDER_PATH>
{
	public:
		typedef UIGlobalSetting_Path<GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_OUTPUT_DATASET_FOLDER_PATH> type;
};
typedef SettingClassTypeTraits<SettingInfo::SETTING_CLASS_UI_GLOBAL_SETTING__OPEN_OUTPUT_DATASET_FOLDER_PATH>::type OpenOutputFilePath;

#endif // UIALLGLOBALSETTINGS_LIST_H
