#ifndef UISETTINGSMANAGER_H
#define UISETTINGSMANAGER_H

#ifndef Q_MOC_RUN
	#include <boost/filesystem.hpp>
	#include <boost/property_tree/ptree.hpp>
	#include <boost/property_tree/xml_parser.hpp>
#endif
#include "globals.h"
#include "Infrastructure/uimanager.h"
#include "../../../NewGeneBackEnd/Settings/SettingsManager.h"
#include <QString>
#include "Base/uisetting.h"
#include "uiallsettings.h"
#include "uiallglobalsettings.h"
#include "uiinputprojectsettings.h"
#include "uioutputprojectsettings.h"
#include "../../../NewGeneBackEnd/Settings/Setting.h"
#include "../../../NewGeneBackEnd/Settings/Settings.h"
#include "../../../NewGeneBackEnd/Settings/GlobalSettings.h"
#include "../../../NewGeneBackEnd/Settings/ProjectSettings.h"
#include "../../../NewGeneBackEnd/Settings/InputProjectSettings.h"
#include "../../../NewGeneBackEnd/Settings/OutputProjectSettings.h"
#include "../../../NewGeneBackEnd/Settings/ModelSettings.h"
#include "../../../NewGeneBackEnd/Settings/InputModelSettings.h"
#include "../../../NewGeneBackEnd/Settings/OutputModelSettings.h"

#include <QStandardPaths>

class NewGeneMainWindow;

class UISettingsManager : public QObject, public
	UIManager<UISettingsManager, SettingsManager, MANAGER_DESCRIPTION_NAMESPACE::MANAGER_SETTINGS_UI, MANAGER_DESCRIPTION_NAMESPACE::MANAGER_SETTINGS>
{

		Q_OBJECT

	public:

		explicit UISettingsManager(QObject * parent, UIMessager & messager);

		std::unique_ptr<BackendGlobalSetting> getSetting(Messager & messager_, GLOBAL_SETTINGS_BACKEND_NAMESPACE::GLOBAL_SETTINGS_BACKEND const which_setting);
		std::unique_ptr<UIGlobalSetting> getSetting(Messager & messager_, GLOBAL_SETTINGS_UI_NAMESPACE::GLOBAL_SETTINGS_UI const which_setting);

		std::unique_ptr<BackendProjectInputSetting> getSetting(Messager & messager_, InputProjectSettings * project_settings,
				INPUT_PROJECT_SETTINGS_BACKEND_NAMESPACE::INPUT_PROJECT_SETTINGS_BACKEND const which_setting);
		std::unique_ptr<BackendProjectOutputSetting> getSetting(Messager & messager_, OutputProjectSettings * project_settings,
				OUTPUT_PROJECT_SETTINGS_BACKEND_NAMESPACE::OUTPUT_PROJECT_SETTINGS_BACKEND const which_setting);
		std::unique_ptr<InputModelSetting> getSetting(Messager & messager_, InputModelSettings * model_settings, INPUT_MODEL_SETTINGS_NAMESPACE::INPUT_MODEL_SETTINGS const which_setting);
		std::unique_ptr<OutputModelSetting> getSetting(Messager & messager_, OutputModelSettings * model_settings,
				OUTPUT_MODEL_SETTINGS_NAMESPACE::OUTPUT_MODEL_SETTINGS const which_setting);
		std::unique_ptr<UIProjectInputSetting> getSetting(Messager & messager_, UIInputProjectSettings * project_settings,
				INPUT_PROJECT_SETTINGS_UI_NAMESPACE::INPUT_PROJECT_SETTINGS_UI const which_setting);
		std::unique_ptr<UIProjectOutputSetting> getSetting(Messager & messager_, UIOutputProjectSettings * project_settings,
				OUTPUT_PROJECT_SETTINGS_UI_NAMESPACE::OUTPUT_PROJECT_SETTINGS_UI const which_setting);

		boost::filesystem::path ObtainGlobalPath(QStandardPaths::StandardLocation const & location, QString const & postfix_part_of_path, QString const & filename_no_path,
				bool const create_if_not_found = false);
		boost::filesystem::path getGlobalSettingsPath() { return global_settings_path; }

		UIAllGlobalSettings & globalSettings() { return *_global_settings; }

	signals:

	public slots:

	protected:


	private:

		boost::filesystem::path global_settings_path;
		std::unique_ptr<UIAllGlobalSettings> _global_settings;

};

#endif // UISETTINGSMANAGER_H
