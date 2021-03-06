#include "globals.h"
#include <QObject>
#include "uiprojectmanager.h"
#include "uimodelmanager.h"
#include "uisettingsmanager.h"
#include "uidocumentmanager.h"
#include "uistatusmanager.h"
#include "uiloggingmanager.h"
#include "uiallglobalsettings_list.h"
#include "../../../../NewGeneBackEnd/Settings/GlobalSettings_list.h"
#include "../../../../NewGeneBackEnd/Settings/InputProjectSettings_list.h"
#include "../../../../NewGeneBackEnd/Settings/OutputProjectSettings_list.h"
#include "../../../../NewGeneBackEnd/Settings/InputModelSettings_list.h"
#include "../../../../NewGeneBackEnd/Settings/OutputModelSettings_list.h"
#include "../newgenewidget.h"
#include "uimessagersingleshot.h"
#include "newgenemainwindow.h"

#include <QFileDialog>
#include <QCoreApplication>

UIProjectManager::UIProjectManager(QObject * parent, UIMessager & messager)
	: QObject(parent)
	, loading(false)
	, loadingInput(false)
	, loadingOutput(false)
	, UIManager(messager)
	, EventLoopThreadManager<UI_PROJECT_MANAGER>(messager, number_worker_threads)
{

	// *************************************************************************
	// All Managers are instantiated AFTER the application event loop is running
	// *************************************************************************

	InitializeEventLoop(this);

}

UIProjectManager::~UIProjectManager()
{

	for_each(input_tabs.begin(), input_tabs.end(), [this](std::pair<NewGeneMainWindow * const, InputProjectTabs> & windows)
	{

		InputProjectTabs & tabs = windows.second;
		for_each(tabs.begin(), tabs.end(), [this](InputProjectTab & tab)
		{
			// Release the pointer so that when the "tabs" map is destroyed, its destructor will do the job
			UIInputProject * project_ptr = static_cast<UIInputProject *>(tab.project.release());
			RawCloseInputProject(project_ptr);
		});

	});

	for_each(output_tabs.begin(), output_tabs.end(), [this](std::pair<NewGeneMainWindow * const, OutputProjectTabs> & windows)
	{

		OutputProjectTabs & tabs = windows.second;
		for_each(tabs.begin(), tabs.end(), [this](OutputProjectTab & tab)
		{
			UIOutputProject * project_ptr = static_cast<UIOutputProject *>(tab.project.release());
			RawCloseOutputProject(project_ptr);
		});

	});

}

void UIProjectManager::showLoading()
{
	NewGeneMainWindow * mainWindow = theMainWindow;
	if (mainWindow == nullptr)
	{
		return;
	}
	mainWindow->showLoading(loadingInput || loadingOutput);
}

void UIProjectManager::EndAllLoops()
{

	for_each(input_tabs.begin(), input_tabs.end(), [this](std::pair<NewGeneMainWindow * const, InputProjectTabs> & windows)
	{

		InputProjectTabs & tabs = windows.second;
		for_each(tabs.begin(), tabs.end(), [this](InputProjectTab & tab)
		{
			UIInputProject * project_ptr = static_cast<UIInputProject *>(tab.project.release());
			RawCloseInputProject(project_ptr);

		});

	});

	for_each(output_tabs.begin(), output_tabs.end(), [this](std::pair<NewGeneMainWindow * const, OutputProjectTabs> & windows)
	{

		OutputProjectTabs & tabs = windows.second;
		for_each(tabs.begin(), tabs.end(), [this](OutputProjectTab & tab)
		{
			UIOutputProject * project_ptr = static_cast<UIOutputProject *>(tab.project.release());
			RawCloseOutputProject(project_ptr);

		});

	});

	EndLoopAndBackgroundPool();

};

void UIProjectManager::LoadOpenProjects(NewGeneMainWindow * mainWindow, QObject * mainWindowObject)
{

	loading = true;

	UIMessager messager;
	UIMessagerSingleShot messager_(messager);
	InputProjectFilesList::instance input_project_list = InputProjectFilesList::get(messager);

	connect(mainWindowObject, SIGNAL(SignalCloseCurrentInputDataset()), this, SLOT(CloseCurrentInputDataset()));
	connect(mainWindowObject, SIGNAL(SignalCloseCurrentOutputDataset()), this, SLOT(CloseCurrentOutputDataset()));
	connect(mainWindowObject, SIGNAL(SignalOpenInputDataset(STD_STRING, QObject *)), this, SLOT(OpenInputDataset(STD_STRING, QObject *)));
	connect(mainWindowObject, SIGNAL(SignalOpenOutputDataset(STD_STRING, QObject *)), this, SLOT(OpenOutputDataset(STD_STRING, QObject *)));
	connect(mainWindowObject, SIGNAL(SignalSaveCurrentInputDatasetAs(STD_STRING, QObject *)), this, SLOT(SaveCurrentInputDatasetAs(STD_STRING, QObject *)));
	connect(mainWindowObject, SIGNAL(SignalSaveCurrentOutputDatasetAs(STD_STRING, QObject *)), this, SLOT(SaveCurrentOutputDatasetAs(STD_STRING, QObject *)));

	bool isDefaultProject = false;
	bool previousMissing = false;

	if (input_project_list->files.size() == 2 && input_project_list->files[0] == boost::filesystem::path() && input_project_list->files[1] == boost::filesystem::path())
	{
		// Kluge!  This is the only way we know that a previously-opened .newgene.in.xml project file is missing
		// See "uiallglobalsettings_list.h"
		input_project_list->files.clear();
		previousMissing = true;
	}

	if (input_project_list->files.size() == 0)
	{

		// Disable the following block:
		// For now, do not prompt to open input dataset if none is found.
		if (false)
		{
			boost::format msg_title("Open input project at default location?");
			boost::format msg_text("You have no input project open.  Would you like to open the project at the default location?");
			QMessageBox::StandardButton reply;
			reply = QMessageBox::question(nullptr, QString(msg_title.str().c_str()), QString(msg_text.str().c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));

			if (reply == QMessageBox::No)
			{
				loading = false;
				return;
			}
		}

		boost::filesystem::path input_project_path = settingsManagerUI().ObtainGlobalPath(QStandardPaths::DocumentsLocation, "NewGene/Input",
				NewGeneFileNames::preLoadedInputProjectFileName);

		if (!boost::filesystem::is_regular_file(input_project_path))
		{
			input_project_path = settingsManagerUI().ObtainGlobalPath(QStandardPaths::DocumentsLocation, "NewGene/Input",
					NewGeneFileNames::defaultInputProjectFileName);

			isDefaultProject = true;
		}

		if (input_project_path != boost::filesystem::path())
		{
			settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_INPUT_PROJECTS_LIST, InputProjectFilesList(messager,
					input_project_path.string().c_str()));
			settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_INPUT_DATASET_FOLDER_PATH, OpenInputFilePath(messager,
					input_project_path.parent_path()));
			input_project_list = InputProjectFilesList::get(messager);
		}
	}

	if (input_project_list->files.size() == 1)
	{

		boost::filesystem::path input_project_settings_path = input_project_list->files[0];

		bool create_new_instance = false;

		if (input_tabs.find(mainWindow) == input_tabs.cend())
		{
			create_new_instance = true;
		}

		if (create_new_instance)
		{
			if (previousMissing || input_project_settings_path == boost::filesystem::path() || !boost::filesystem::is_regular_file(input_project_settings_path))
			{
				if (previousMissing || !isDefaultProject)
				{
					QMessageBox::StandardButton reply;
					reply = QMessageBox::question(nullptr, QString("Missing project file"), QString(std::string("The most recently opened input project file is missing.  Would you like to create a default input project?").c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));

					if (reply == QMessageBox::No)
					{
						create_new_instance = false;
					}
				}
			}
		}

		if (create_new_instance)
		{

			RawOpenInputProject(messager, input_project_settings_path, mainWindowObject);

		}

	}

}

UIInputProject * UIProjectManager::getActiveUIInputProject()
{

	// For now, just get the first main window
	if (input_tabs.size() == 0)
	{
		return nullptr;
	}

	InputProjectTabs & tabs = input_tabs.begin()->second;

	if (tabs.size() == 0)
	{
		return nullptr;
	}

	InputProjectTab & tab = *tabs.begin();

	UIInputProject * project = static_cast<UIInputProject *>(tab.project.get());

	return project;

}

UIOutputProject * UIProjectManager::getActiveUIOutputProject()
{

	// For now, just get the first main window
	if (output_tabs.size() == 0)
	{
		return nullptr;
	}

	OutputProjectTabs & tabs = output_tabs.begin()->second;

	if (tabs.size() == 0)
	{
		return nullptr;
	}

	OutputProjectTab & tab = *tabs.begin();

	UIOutputProject * project = static_cast<UIOutputProject *>(tab.project.get());

	return project;

}

void UIProjectManager::SignalMessageBox(STD_STRING msg)
{

	QMessageBox msgBox;
	msgBox.setText(msg.c_str());
	msgBox.exec();

}

void UIProjectManager::DoneLoadingFromDatabase(UI_INPUT_MODEL_PTR model_, QObject * mainWindowObject)
{

	bool was_loading = loading; // Initial (automatic) load when NewGene runs?  If so, 'loading' is true.  If user chose to open the input dataset by hand, 'loading' is false.
	loading = false;
	loadingInput = false;
	showLoading();

	UIMessagerSingleShot messager;

	if (!getActiveUIInputProject()->is_model_equivalent(messager.get(), model_))
	{
		messager.get().AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__INPUT_MODELS_DO_NOT_MATCH,
									 "Input model has completed loading from database, but does not match current input model."));
		return;
	}

	if (!model_->loaded())
	{
		messager.get().AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__INPUT_MODEL_NOT_LOADED, "Input model has completed loading from database, but is marked as not loaded."));
		return;
	}

	projectManager().input_project_is_open = true;

	UIInputProject * input_project = getActiveUIInputProject();

	if (!input_project)
	{
		return;
	}

	settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager.get(), GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_INPUT_PROJECTS_LIST, InputProjectFilesList(messager.get(),
			input_project->projectSettings().getUISettings().GetSettingsPath().string().c_str()));
	settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager.get(), GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_INPUT_DATASET_FOLDER_PATH, OpenInputFilePath(messager.get(),
			input_project->projectSettings().getUISettings().GetSettingsPath().parent_path()));

	getActiveUIInputProject()->DoRefreshAllWidgets();

	if (was_loading)
	{

		OutputProjectFilesList::instance output_project_list = OutputProjectFilesList::get(messager.get());

		bool isDefaultProject = false;
		bool previousMissing = false;

		if (output_project_list->files.size() == 2 && output_project_list->files[0] == boost::filesystem::path() && output_project_list->files[1] == boost::filesystem::path())
		{
			// Kluge!  This is the only way we know that a previously-opened .newgene.out.xml project file is missing
			// See "uiallglobalsettings_list.h"
			output_project_list->files.clear();
			previousMissing = true;
		}

		if (output_project_list->files.size() == 0)
		{

			// Disable the following block:
			// For now, during initial load,
			// do not prompt to open output dataset if none is found
			if (false)
			{
				boost::format msg_title("Open output project at default location?");
				boost::format msg_text("You have no output project open.  Would you like to open the project at the default location?");
				QMessageBox::StandardButton reply;
				reply = QMessageBox::question(nullptr, QString(msg_title.str().c_str()), QString(msg_text.str().c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));

				if (reply == QMessageBox::No)
				{
					return;
				}
			}

			boost::filesystem::path output_project_path = settingsManagerUI().ObtainGlobalPath(QStandardPaths::DocumentsLocation, "NewGene/Output",
					NewGeneFileNames::preLoadedOutputProjectFileName);

			if (!boost::filesystem::is_regular_file(output_project_path))
			{
				output_project_path = settingsManagerUI().ObtainGlobalPath(QStandardPaths::DocumentsLocation, "NewGene/Output",
						NewGeneFileNames::defaultOutputProjectFileName);

				isDefaultProject = true;
			}

			if (output_project_path != boost::filesystem::path())
			{
				settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager.get(), GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_OUTPUT_PROJECTS_LIST, OutputProjectFilesList(messager.get(),
						output_project_path.string().c_str()));
				settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager.get(), GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_OUTPUT_DATASET_FOLDER_PATH, OpenOutputFilePath(messager.get(),
						output_project_path.parent_path()));
				output_project_list = OutputProjectFilesList::get(messager.get());
			}
		}

		if (output_project_list->files.size() == 1)
		{

			boost::filesystem::path output_project_settings_path = output_project_list->files[0];

			bool create_new_instance = false;

			NewGeneMainWindow * mainWindow = nullptr;

			try
			{
				mainWindow = dynamic_cast<NewGeneMainWindow *>(mainWindowObject);

				if (mainWindow == nullptr)
				{
					return;
				}
			}
			catch (std::bad_cast &)
			{
				return;
			}

			if (output_tabs.find(mainWindow) == output_tabs.cend())
			{
				create_new_instance = true;
			}

			if (create_new_instance)
			{
				if (previousMissing || output_project_settings_path == boost::filesystem::path() || !boost::filesystem::is_regular_file(output_project_settings_path))
				{
					if (previousMissing || !isDefaultProject)
					{
						QMessageBox::StandardButton reply;
						reply = QMessageBox::question(nullptr, QString("Missing project file"), QString(std::string("The most recently opened output project file is missing.  Would you like to create a default output project?").c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));

						if (reply == QMessageBox::No)
						{
							create_new_instance = false;
						}
					}
				}
			}

			if (create_new_instance)
			{

				RawOpenOutputProject(messager.get(), output_project_settings_path, mainWindowObject);

			}

		}

	}
	else
	{

		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(nullptr, QString("Open output project?"), QString("Would you also like to open an associated output project?"),
									  QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));

		if (reply == QMessageBox::Yes)
		{
			OpenOutputFilePath::instance folder_path = OpenOutputFilePath::get(messager.get());
			QWidget * mainWindow = nullptr;

			try
			{
				mainWindow = dynamic_cast<QWidget *>(mainWindowObject);
			}
			catch (std::bad_cast &)
			{

			}

			if (mainWindow)
			{
				QString the_file = QFileDialog::getOpenFileName(mainWindow, "Choose output dataset", folder_path ? folder_path->getPath().string().c_str() : "",
								   "NewGene output settings file (*.xml)");

				if (the_file.size())
				{
					std::string the_file_str = the_file.toStdString();
					if (checkValidProjectFilenameExtension(false, the_file_str))
					{
						if (boost::filesystem::exists(the_file.toStdString()) && !boost::filesystem::is_directory(the_file.toStdString()))
						{
							boost::filesystem::path file_path(the_file.toStdString());
							settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager.get(), GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_OUTPUT_DATASET_FOLDER_PATH, OpenOutputFilePath(messager.get(),
									file_path.parent_path()));
							OpenOutputDataset(file_path.string(), mainWindowObject);
						}
					}
				}
			}
		}

	}

}

void UIProjectManager::DoneLoadingFromDatabase(UI_OUTPUT_MODEL_PTR model_, QObject * mainWindowObject)
{

	loading = false;
	loadingOutput = false;
	showLoading();
	UIMessagerSingleShot messager;

	if (!getActiveUIOutputProject()->is_model_equivalent(messager.get(), model_))
	{
		messager.get().AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__OUTPUT_MODELS_DO_NOT_MATCH,
									 "Output model has completed loading from database, but does not match current output model."));
		return;
	}

	if (!model_->loaded())
	{
		messager.get().AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__OUTPUT_MODEL_NOT_LOADED,
									 "Output model has completed loading from database, but is marked as not loaded."));
		return;
	}

#if __APPLE__
	if (model_->backend().getInputModel().t_vgp_data_vector.size() == 0)
	{
		boost::format msg("Loading NewGene's data requires special steps for the Mac.  To load data, please go to the NewGene website at http://www.newgenesoftware.org/ for special Mac instructions.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}
#endif

	UIOutputProject * output_project = getActiveUIOutputProject();

	if (!output_project)
	{
		return;
	}

	projectManager().output_project_is_open = true;

	settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager.get(), GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_OUTPUT_PROJECTS_LIST, OutputProjectFilesList(messager.get(),
			output_project->projectSettings().getUISettings().GetSettingsPath().string()));

	getActiveUIOutputProject()->DoRefreshAllWidgets();

}

void UIProjectManager::OpenOutputDataset(STD_STRING the_output_dataset, QObject * mainWindowObject)
{

	// blocks - all widgets that respond to the UIProjectManager
	CloseCurrentOutputDataset();

	UIMessager messager;

	UIInputProject * input_project = getActiveUIInputProject();

	if (!input_project)
	{
		boost::format msg("Please open or create an input dataset before attempting to open or create an output dataset.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	if (!boost::filesystem::is_directory(the_output_dataset))
	{
		RawOpenOutputProject(messager, boost::filesystem::path(the_output_dataset), mainWindowObject);
	}

}

void UIProjectManager::CloseCurrentOutputDataset()
{

	// One per main window (currently only 1 supported per application)
	if (output_tabs.size() > 1 || output_tabs.size() == 0)
	{
		return;
	}

	OutputProjectTabs & tabs = (*output_tabs.begin()).second;

	// One per project (corresponding to an actual physical tab; currently only 1 per main window supported)
	if (tabs.size() > 1 || tabs.size() == 0)
	{
		return;
	}

	OutputProjectTab & tab = *tabs.begin();

	if (!tab.project)
	{
		tabs.clear();
		return;
	}

	UIOutputProject * project_ptr = static_cast<UIOutputProject *>(tab.project.release());
	RawCloseOutputProject(project_ptr);

	tabs.clear();

	UIMessager messager;
	settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_OUTPUT_PROJECTS_LIST, InputProjectFilesList(messager, ""));

}

void UIProjectManager::OpenInputDataset(STD_STRING the_input_dataset, QObject * mainWindowObject)
{

	// blocks - all widgets that respond to the UIProjectManager
	CloseCurrentOutputDataset();

	// blocks - all widgets that respond to the UIProjectManager
	CloseCurrentInputDataset();

	bool success = false;

	UIMessager messager;

	if (!boost::filesystem::is_directory(the_input_dataset))
	{
		RawOpenInputProject(messager, boost::filesystem::path(the_input_dataset), mainWindowObject);
	}

}

void UIProjectManager::CloseCurrentInputDataset()
{

	// Blocks - All happens in main thread
	CloseCurrentOutputDataset();

	// One per main window (currently only 1 supported per application)
	if (input_tabs.size() > 1 || input_tabs.size() == 0)
	{
		return;
	}

	InputProjectTabs & tabs = (*input_tabs.begin()).second;

	// One per project (corresponding to an actual physical tab; currently only 1 per main window supported)
	if (tabs.size() > 1 || tabs.size() == 0)
	{
		return;
	}

	InputProjectTab & tab = *tabs.begin();

	if (!tab.project)
	{
		tabs.clear();
		return;
	}

	UIInputProject * project_ptr = static_cast<UIInputProject *>(tab.project.release());
	RawCloseInputProject(project_ptr);

	tabs.clear();

	UIMessager messager;
	settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_INPUT_PROJECTS_LIST, InputProjectFilesList(messager, ""));

}

void UIProjectManager::RawOpenInputProject(UIMessager & messager, boost::filesystem::path const & input_project_settings_path, QObject * mainWindowObject)
{

	loadingInput = true;
	showLoading();

	if (boost::filesystem::is_directory(input_project_settings_path))
	{
		QMessageBox msgBox;
		boost::format msg("%1%, the input project settings filename, is not a valid file.");
		msg % input_project_settings_path.string();
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		loading = false;
		loadingInput = false;
		showLoading();
		return;
	}

	// Internally creates both an instance of UI-layer project settings, and an instance of backend-layer project settings
	// via SettingsRepositoryFactory
	std::shared_ptr<UIInputProjectSettings> project_settings(new UIInputProjectSettings(messager, input_project_settings_path));
	project_settings->WriteSettingsToFile(messager); // Writes default settings for those settings not already present

	// When the UIInputModelSettings instance is created below, it internally creates
	// an instance of backend-layer model settings
	auto path_to_model_settings_ = InputProjectPathToModel::get(messager, project_settings->getBackendSettings());
	boost::filesystem::path path_to_model_settings = path_to_model_settings_->getPath();

	if (path_to_model_settings.is_relative())
	{
		boost::filesystem::path new_path = input_project_settings_path.parent_path();
		new_path /= path_to_model_settings;
		path_to_model_settings = new_path;
	}

	if (boost::filesystem::is_directory(path_to_model_settings))
	{
		path_to_model_settings /= (input_project_settings_path.stem().string() + ".model.xml");
	}

	NewGeneMainWindow * mainWindow = nullptr;

	try
	{
		mainWindow = dynamic_cast<NewGeneMainWindow *>(mainWindowObject);
	}
	catch (std::bad_cast &)
	{
		project_settings->EndLoopAndBackgroundPool(); // blocks
		loading = false;
		loadingInput = false;
		showLoading();
		return;
	}
	if (mainWindow == nullptr)
	{
		project_settings->EndLoopAndBackgroundPool(); // blocks
		loading = false;
		loadingInput = false;
		showLoading();
		return;
	}

	bool continueLoading = true;

	if (!mainWindow->newInputDataset && !boost::filesystem::is_regular_file(path_to_model_settings))
	{
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(nullptr, QString("Missing model settings"), QString((std::string{"The input model settings file \""} + path_to_model_settings.string() + "\" is missing.  Would you like to create an empty input model settings file with that name?").c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));
		if (reply == QMessageBox::No)
		{
			project_settings->EndLoopAndBackgroundPool(); // blocks
			loading = false;
			loadingInput = false;
			showLoading();
			continueLoading = false;
		}
	}

	if (continueLoading)
	{
		std::shared_ptr<UIInputModelSettings> model_settings(new UIInputModelSettings(messager, path_to_model_settings));
		model_settings->WriteSettingsToFile(messager); // Writes default settings for those settings not already present

		// Backend model does not know about the current project's settings, because multiple settings might point to the same model.
		auto path_to_model_database_ = InputModelPathToDatabase::get(messager, model_settings->getBackendSettings());
		boost::filesystem::path path_to_model_database = path_to_model_database_->getPath();

		if (path_to_model_database.is_relative())
		{
			boost::filesystem::path new_path = path_to_model_settings.parent_path();
			new_path /= path_to_model_database;
			path_to_model_database = new_path;
		}

		if (boost::filesystem::is_directory(path_to_model_database))
		{
			path_to_model_database /= (input_project_settings_path.stem().string() + ".db");
		}

		if (!mainWindow->newInputDataset && !boost::filesystem::is_regular_file(path_to_model_database))
		{
			QMessageBox::StandardButton reply;
			reply = QMessageBox::question(nullptr, QString("Missing input database"), QString((std::string{"The input database \""} + path_to_model_database.string() + "\" is missing.  Would you like to create an empty input model database with that name?").c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));
			if (reply == QMessageBox::No)
			{
				project_settings->EndLoopAndBackgroundPool(); // blocks
				model_settings->EndLoopAndBackgroundPool(); // blocks
				loading = false;
				loadingInput = false;
				showLoading();
				continueLoading = false;
			}
		}

		if (continueLoading)
		{
			std::shared_ptr<InputModel> backend_model;

			try
			{
				backend_model.reset(ModelFactory<InputModel>()(messager, path_to_model_database));
			}
			catch (boost::exception & e)
			{
				project_settings->EndLoopAndBackgroundPool(); // blocks
				model_settings->EndLoopAndBackgroundPool(); // blocks
				loading = false;
				loadingInput = false;
				showLoading();

				if (std::string const * error_desc = boost::get_error_info<newgene_error_description>(e))
				{
					boost::format msg(error_desc->c_str());
					QMessageBox msgBox;
					msgBox.setText(msg.str().c_str());
					msgBox.exec();
				}
				else
				{
					std::string the_error = boost::diagnostic_information(e);
					boost::format msg("Error: %1%");
					msg % the_error.c_str();
					QMessageBox msgBox;
					msgBox.setText(msg.str().c_str());
					msgBox.exec();
				}

				boost::format msg("Unable to create input project database.");
				messager.AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__INPUT_MODEL_DATABASE_CANNOT_BE_CREATED, msg.str()));
				return;
			}

			std::shared_ptr<UIInputModel> project_model(new UIInputModel(messager, backend_model));

			// ************************************************************************************************************************************* //
			// Clang workaround: http://stackoverflow.com/questions/20583591/clang-only-a-pairpath-path-can-be-emplaced-into-a-vector-so-can-a-pairuniq
			// ... cannot pass const filesystem::path, so must create temp from the const that can act as rvalue
			// ************************************************************************************************************************************* //
			std::unique_ptr<UIMessagerInputProject> messager_ptr(new UIMessagerInputProject(nullptr));
			std::unique_ptr<UIInputProject> project_ptr(new UIInputProject(project_settings, model_settings, project_model, mainWindowObject, nullptr, *messager_ptr));
			input_tabs[mainWindow].emplace_back(ProjectPaths(input_project_settings_path, path_to_model_settings, path_to_model_database),
												project_ptr.release(), // can't use move() in the initialization list, I think, because we might have a custom deleter
												messager_ptr.release());

			UIInputProject * project = getActiveUIInputProject();

			if (!project)
			{
				project_settings->EndLoopAndBackgroundPool(); // blocks
				model_settings->EndLoopAndBackgroundPool(); // blocks
				project_model->EndLoopAndBackgroundPool(); // blocks
				boost::format msg("No input dataset is open.");
				messager.AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__PROJECT_IS_NULL, msg.str()));
				loading = false;
				loadingInput = false;
				showLoading();
				return;
			}

			project->InitializeEventLoop(project); // cannot use 'this' in base class with multiple inheritance
			project->model().InitializeEventLoop(&project->model()); // cannot use 'this' in base class with multiple inheritance
			project->modelSettings().InitializeEventLoop(&project->modelSettings()); // cannot use 'this' in base class with multiple inheritance
			project->projectSettings().InitializeEventLoop(&project->projectSettings()); // cannot use 'this' in base class with multiple inheritance

			project_settings->UpdateConnections();
			model_settings->UpdateConnections();
			project_model->UpdateConnections();
			project->UpdateConnections();

			mainWindow->newInputDataset = false;

			// blocks, because all connections are in NewGeneWidget which are all associated with the UI event loop
			emit UpdateInputConnections(NewGeneWidget::ESTABLISH_CONNECTIONS_INPUT_PROJECT, project);

			emit LoadFromDatabase(&project->model(), mainWindowObject);
		}
	}

}

void UIProjectManager::RawOpenOutputProject(UIMessager & messager, boost::filesystem::path const & output_project_settings_path, QObject * mainWindowObject)
{

	loadingOutput = true;
	showLoading();

	if (boost::filesystem::is_directory(output_project_settings_path))
	{
		QMessageBox msgBox;
		boost::format msg("%1%, the input project settings filename, is not a valid file.");
		msg % output_project_settings_path.string();
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		loading = false;
		loadingOutput = false;
		showLoading();
		return;
	}

	// Internally creates both an instance of UI-layer project settings, and an instance of backend-layer project settings
	// via SettingsRepositoryFactory
	std::shared_ptr<UIOutputProjectSettings> project_settings(new UIOutputProjectSettings(messager, output_project_settings_path));
	project_settings->WriteSettingsToFile(messager); // Writes default settings for those settings not already present

	// When the UIOutputModelSettings instance is created below, it internally creates
	// an instance of backend-layer model settings
	auto path_to_model_settings_ = OutputProjectPathToModel::get(messager, project_settings->getBackendSettings());
	boost::filesystem::path path_to_model_settings = path_to_model_settings_->getPath();

	if (path_to_model_settings.is_relative())
	{
		boost::filesystem::path new_path = output_project_settings_path.parent_path();
		new_path /= path_to_model_settings;
		path_to_model_settings = new_path;
	}

	if (boost::filesystem::is_directory(path_to_model_settings))
	{
		path_to_model_settings /= (output_project_settings_path.stem().string() + ".model.xml");
	}

	NewGeneMainWindow * mainWindow = nullptr;

	try
	{
		mainWindow = dynamic_cast<NewGeneMainWindow *>(mainWindowObject);
	}
	catch (std::bad_cast &)
	{
		project_settings->EndLoopAndBackgroundPool(); // blocks
		loading = false;
		loadingOutput = false;
		showLoading();
		return;
	}
	if (mainWindow == nullptr)
	{
		project_settings->EndLoopAndBackgroundPool(); // blocks
		loading = false;
		loadingOutput = false;
		showLoading();
		return;
	}

	bool continueLoading = true;

	if (!mainWindow->newOutputDataset && !boost::filesystem::is_regular_file(path_to_model_settings))
	{
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(nullptr, QString("Missing model settings"), QString((std::string{"The output model settings file \""} + path_to_model_settings.string() + "\" is missing.  Would you like to create an empty output model settings file with that name?").c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));
		if (reply == QMessageBox::No)
		{
			project_settings->EndLoopAndBackgroundPool(); // blocks
			loading = false;
			loadingOutput = false;
			showLoading();
			continueLoading = false;
		}
	}

	if (continueLoading)
	{
		std::shared_ptr<UIOutputModelSettings> model_settings(new UIOutputModelSettings(messager, path_to_model_settings));
		model_settings->WriteSettingsToFile(messager); // Writes default settings for those settings not already present

		// The input model and settings are necessary in order to instantiate the output model
		UIInputProject * input_project = getActiveUIInputProject();

		if (!input_project)
		{
			project_settings->EndLoopAndBackgroundPool(); // blocks
			model_settings->EndLoopAndBackgroundPool(); // blocks
			boost::format msg("NULL input project during attempt to instantiate output project.");
			messager.AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__PROJECT_IS_NULL, msg.str()));
			loading = false;
			loadingOutput = false;
			showLoading();
			return;
		}

		// Backend model does not know about the current project's settings, because multiple settings might point to the same model.
		auto path_to_model_database_ = OutputModelPathToDatabase::get(messager, model_settings->getBackendSettings());
		boost::filesystem::path path_to_model_database = path_to_model_database_->getPath();

		if (path_to_model_database.is_relative())
		{
			boost::filesystem::path new_path = path_to_model_settings.parent_path();
			new_path /= path_to_model_database;
			path_to_model_database = new_path;
		}

		if (boost::filesystem::is_directory(path_to_model_database))
		{
			path_to_model_database /= (output_project_settings_path.stem().string() + ".db");
		}

		if (!mainWindow->newOutputDataset && !boost::filesystem::is_regular_file(path_to_model_database))
		{
			QMessageBox::StandardButton reply;
			reply = QMessageBox::question(nullptr, QString("Missing output database"), QString((std::string{"The output database \""} + path_to_model_database.string() + "\" is missing.  Would you like to create an empty output model database with that name?").c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));
			if (reply == QMessageBox::No)
			{
				project_settings->EndLoopAndBackgroundPool(); // blocks
				model_settings->EndLoopAndBackgroundPool(); // blocks
				loading = false;
				loadingOutput = false;
				showLoading();
				continueLoading = false;
			}
		}

		if (continueLoading)
		{
			std::shared_ptr<OutputModel> backend_model;

			try
			{
				backend_model.reset(ModelFactory<OutputModel>()(messager, path_to_model_database, std::dynamic_pointer_cast<InputModelSettings>(input_project->backend().modelSettingsSharedPtr()),
									input_project->backend().modelSharedPtr()));
			}
			catch (boost::exception & e)
			{
				project_settings->EndLoopAndBackgroundPool(); // blocks
				model_settings->EndLoopAndBackgroundPool(); // blocks
				loading = false;
				loadingOutput = false;
				showLoading();

				if (std::string const * error_desc = boost::get_error_info<newgene_error_description>(e))
				{
					boost::format msg(error_desc->c_str());
					QMessageBox msgBox;
					msgBox.setText(msg.str().c_str());
					msgBox.exec();
				}
				else
				{
					std::string the_error = boost::diagnostic_information(e);
					boost::format msg("Error: %1%");
					msg % the_error.c_str();
					QMessageBox msgBox;
					msgBox.setText(msg.str().c_str());
					msgBox.exec();
				}

				boost::format msg("Unable to create output project database.");
				messager.AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__OUTPUT_MODEL_DATABASE_CANNOT_BE_CREATED, msg.str()));
				return;
			}

			std::shared_ptr<UIOutputModel> project_model(new UIOutputModel(messager, backend_model));

			std::unique_ptr<UIMessagerOutputProject> messager_ptr(new UIMessagerOutputProject(nullptr));
			std::unique_ptr<UIOutputProject> project_ptr(new UIOutputProject(project_settings, model_settings, project_model, mainWindowObject, nullptr, *messager_ptr, input_project));
			output_tabs[mainWindow].emplace_back(ProjectPaths(output_project_settings_path, path_to_model_settings, path_to_model_database),
												 project_ptr.release(), // can't use move() in the initialization list, I think, because we might have a custom deleter
												 messager_ptr.release());

			UIOutputProject * project = getActiveUIOutputProject();

			if (!project)
			{
				project_settings->EndLoopAndBackgroundPool(); // blocks
				model_settings->EndLoopAndBackgroundPool(); // blocks
				project_model->EndLoopAndBackgroundPool(); // blocks
				boost::format msg("NULL output project during attempt to instantiate project.");
				messager.AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__PROJECT_IS_NULL, msg.str()));
				loading = false;
				loadingOutput = false;
				showLoading();
				return;
			}

			project->InitializeEventLoop(project, 15000000); // cannot use 'this' in base class with multiple inheritance
			project->model().InitializeEventLoop(&project->model()); // cannot use 'this' in base class with multiple inheritance
			project->modelSettings().InitializeEventLoop(&project->modelSettings()); // cannot use 'this' in base class with multiple inheritance
			project->projectSettings().InitializeEventLoop(&project->projectSettings()); // cannot use 'this' in base class with multiple inheritance

			project_settings->UpdateConnections();
			model_settings->UpdateConnections();
			project_model->UpdateConnections();
			project->UpdateConnections();

			mainWindow->newOutputDataset = false;

			// blocks, because all connections are in NewGeneWidget which are all associated with the UI event loop
			emit UpdateOutputConnections(NewGeneWidget::ESTABLISH_CONNECTIONS_OUTPUT_PROJECT, project);

			emit LoadFromDatabase(&getActiveUIOutputProject()->model(), mainWindowObject);
		}

	}
}

void UIProjectManager::RawCloseInputProject(UIInputProject * input_project)
{

	if (!input_project)
	{
		return;
	}

	projectManager().input_project_is_open = false;

	// blocks, because all connections are in NewGeneWidget which are all associated with the UI event loop
	emit UpdateInputConnections(NewGeneWidget::RELEASE_CONNECTIONS_INPUT_PROJECT, input_project);

	input_project->model().EndLoopAndBackgroundPool(); // blocks
	input_project->modelSettings().EndLoopAndBackgroundPool(); // blocks
	input_project->projectSettings().EndLoopAndBackgroundPool(); // blocks
	input_project->EndLoopAndBackgroundPool(); // blocks

	input_project->deleteLater();

}

void UIProjectManager::RawCloseOutputProject(UIOutputProject * output_project)
{

	if (!output_project)
	{
		return;
	}

	projectManager().output_project_is_open = false;

	output_project->setUIInputProject(nullptr);

	// blocks, because all connections are in NewGeneWidget which are all associated with the UI event loop
	emit this->UpdateOutputConnections(NewGeneWidget::RELEASE_CONNECTIONS_OUTPUT_PROJECT, output_project);

	output_project->model().EndLoopAndBackgroundPool(); // blocks
	output_project->modelSettings().EndLoopAndBackgroundPool(); // blocks
	output_project->projectSettings().EndLoopAndBackgroundPool(); // blocks
	output_project->EndLoopAndBackgroundPool(); // blocks

	output_project->deleteLater();

}

void UIProjectManager::SaveCurrentInputDatasetAs(STD_STRING the_input_dataset, QObject * mainWindowObject)
{

	UIMessager messager;
	UIMessagerSingleShot messager_(messager);

	UIInputProject * active_input_project = getActiveUIInputProject();

	if (active_input_project != nullptr)
	{

		// Create a path object for the path to the project settings
		boost::filesystem::path input_project_settings_path(the_input_dataset.c_str());

		// Create a path object for the path to the model settings
		boost::filesystem::path path_to_model_settings(input_project_settings_path.parent_path() / (input_project_settings_path.stem().string() + ".model.xml"));

		// Create a path object for the path to the model database file
		boost::filesystem::path path_to_model_database(input_project_settings_path.parent_path() / (input_project_settings_path.stem().string() + ".db"));

		bool project_settings_exist = boost::filesystem::exists(input_project_settings_path);
		bool model_settings_exist = boost::filesystem::exists(path_to_model_settings);
		bool model_database_exists = boost::filesystem::exists(path_to_model_database);

		if (project_settings_exist || model_settings_exist || model_database_exists)
		{
			QMessageBox::StandardButton reply;
			std::string formatting("The following files will be overwritten:\n");
			int count = 0;

			if (project_settings_exist)
			{
				formatting += "%1%\n";
				++count;
			}

			if (model_settings_exist)
			{
				formatting += "%2%\n";
				++count;
			}

			if (model_database_exists)
			{
				formatting += "%3%\n";
				++count;
			}

			formatting += "Would you like to overwrite ";
			formatting += (count > 1 ? " these files?" : "this file?");
			boost::format msg(formatting.c_str());
			msg % input_project_settings_path.string() % path_to_model_settings.string() % path_to_model_database.string();
			reply = QMessageBox::question(nullptr, QString("Overwrite files?"), QString(msg.str().c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));

			if (reply == QMessageBox::No)
			{
				return;
			}

			if (project_settings_exist)
			{
				boost::filesystem::remove(input_project_settings_path);
			}

			if (model_settings_exist)
			{
				boost::filesystem::remove(path_to_model_settings);
			}

			if (model_database_exists)
			{
				boost::filesystem::remove(path_to_model_database);
			}
		}

		// Set the new path for the project settings in the currently open project
		active_input_project->SetProjectPaths(input_project_settings_path, path_to_model_settings);

		settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_INPUT_PROJECTS_LIST, InputProjectFilesList(messager,
				input_project_settings_path.string().c_str()));
		settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_INPUT_DATASET_FOLDER_PATH, OpenInputFilePath(messager,
				input_project_settings_path.parent_path()));
		active_input_project->projectSettings().getBackendSettings().UpdateSetting(messager, INPUT_PROJECT_SETTINGS_BACKEND_NAMESPACE::PATH_TO_MODEL, InputProjectPathToModel(messager,
				input_project_settings_path.string().c_str()));

		// Write the project settings to file
		active_input_project->projectSettings().WriteSettingsToFile(messager);

		// Write the model settings to file
		active_input_project->modelSettings().WriteSettingsToFile(messager);

		// Copy the database
		active_input_project->backend().model().SaveDatabaseAs(messager, path_to_model_database);

		// Now set path to database - it must be done last so that the previous path is available, above, when the database is copied
		active_input_project->modelSettings().getBackendSettings().UpdateSetting(messager, INPUT_MODEL_SETTINGS_NAMESPACE::PATH_TO_MODEL_DATABASE, InputModelPathToDatabase(messager,
				path_to_model_database.string().c_str()));

		NewGeneMainWindow * mainWindow = nullptr;

		try
		{
			mainWindow = dynamic_cast<NewGeneMainWindow *>(mainWindowObject);

			if (mainWindow == nullptr)
			{
				return;
			}
		}
		catch (std::bad_cast &)
		{
			boost::format msg("The project was successfully saved to a different name, but the text at the top of the Manage Input pane failed to change (oops!).");
			QMessageBox msgBox;
			msgBox.setText((msg.str().c_str()));
			msgBox.exec();
		}

	}

}

void UIProjectManager::SaveCurrentOutputDatasetAs(STD_STRING the_output_dataset, QObject * mainWindowObject)
{

	UIMessager messager;
	UIMessagerSingleShot messager_(messager);

	UIOutputProject * active_output_project = getActiveUIOutputProject();

	if (active_output_project != nullptr)
	{

		// Create a path object for the path to the project settings
		boost::filesystem::path output_project_settings_path(the_output_dataset.c_str());

		// Create a path object for the path to the model settings
		boost::filesystem::path path_to_model_settings(output_project_settings_path.parent_path() / (output_project_settings_path.stem().string() + ".model.xml"));

		// Create a path object for the path to the model database file
		boost::filesystem::path path_to_model_database(output_project_settings_path.parent_path() / (output_project_settings_path.stem().string() + ".db"));

		bool project_settings_exist = boost::filesystem::exists(output_project_settings_path);
		bool model_settings_exist = boost::filesystem::exists(path_to_model_settings);
		bool model_database_exists = boost::filesystem::exists(path_to_model_database);

		if (project_settings_exist || model_settings_exist || model_database_exists)
		{
			QMessageBox::StandardButton reply;
			std::string formatting("The following files will be overwritten:\n");
			int count = 0;

			if (project_settings_exist)
			{
				formatting += "%1%\n";
				++count;
			}

			if (model_settings_exist)
			{
				formatting += "%2%\n";
				++count;
			}

			if (model_database_exists)
			{
				formatting += "%3%\n";
				++count;
			}

			formatting += "Would you like to overwrite ";
			formatting += (count > 1 ? " these files?" : "this file?");
			boost::format msg(formatting.c_str());
			msg % output_project_settings_path.string() % path_to_model_settings.string() % path_to_model_database.string();
			reply = QMessageBox::question(nullptr, QString("Overwrite files?"), QString(msg.str().c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));

			if (reply == QMessageBox::No)
			{
				return;
			}

			if (project_settings_exist)
			{
				boost::filesystem::remove(output_project_settings_path);
			}

			if (model_settings_exist)
			{
				boost::filesystem::remove(path_to_model_settings);
			}

			if (model_database_exists)
			{
				boost::filesystem::remove(path_to_model_database);
			}
		}

		// Set the new path for the project settings in the currently open project
		active_output_project->SetProjectPaths(output_project_settings_path, path_to_model_settings);

		settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_OUTPUT_PROJECTS_LIST, OutputProjectFilesList(messager,
				output_project_settings_path.string().c_str()));
		settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_OUTPUT_DATASET_FOLDER_PATH, OpenOutputFilePath(messager,
				output_project_settings_path.parent_path()));
		active_output_project->projectSettings().getBackendSettings().UpdateSetting(messager, OUTPUT_PROJECT_SETTINGS_BACKEND_NAMESPACE::PATH_TO_MODEL, OutputProjectPathToModel(messager,
				output_project_settings_path.string().c_str()));

		// Write the project settings to file
		active_output_project->projectSettings().WriteSettingsToFile(messager);

		// Write the model settings to file
		active_output_project->modelSettings().WriteSettingsToFile(messager);

		// Copy the database
		active_output_project->backend().model().SaveDatabaseAs(messager, path_to_model_database);

		// Now set path to database - it must be done last so that the previous path is available, above, when the database is copied
		active_output_project->modelSettings().getBackendSettings().UpdateSetting(messager, OUTPUT_MODEL_SETTINGS_NAMESPACE::PATH_TO_MODEL_DATABASE, OutputModelPathToDatabase(messager,
				path_to_model_database.string().c_str()));

		NewGeneMainWindow * mainWindow = nullptr;

		try
		{
			mainWindow = dynamic_cast<NewGeneMainWindow *>(mainWindowObject);

			if (mainWindow == nullptr)
			{
				return;
			}
		}
		catch (std::bad_cast &)
		{
			boost::format msg("The project was successfully saved to a different name, but the text at the top of the Manage Input pane failed to change (oops!).");
			QMessageBox msgBox;
			msgBox.setText((msg.str().c_str()));
			msgBox.exec();
		}

	}

}
