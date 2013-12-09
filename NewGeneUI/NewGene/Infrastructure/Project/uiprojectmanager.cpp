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

UIProjectManager::UIProjectManager( QObject * parent )
	: QObject(parent)
	, UIManager()
	, EventLoopThreadManager<UI_PROJECT_MANAGER>(number_worker_threads)
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
			UIInputProject * project_ptr = static_cast<UIInputProject*>(tab.second.release());
			RawCloseInputProject(project_ptr);
		});

	});

	for_each(output_tabs.begin(), output_tabs.end(), [this](std::pair<NewGeneMainWindow * const, OutputProjectTabs> & windows)
	{

		OutputProjectTabs & tabs = windows.second;
		for_each(tabs.begin(), tabs.end(), [this](OutputProjectTab & tab)
		{
			UIOutputProject * project_ptr = static_cast<UIOutputProject*>(tab.second.release());
			RawCloseOutputProject(project_ptr);
		});

	});

}

void UIProjectManager::EndAllLoops()
{

	for_each(input_tabs.begin(), input_tabs.end(), [this](std::pair<NewGeneMainWindow * const, InputProjectTabs> & windows)
	{

		InputProjectTabs & tabs = windows.second;
		for_each(tabs.begin(), tabs.end(), [this](InputProjectTab & tab)
		{

			//ProjectPaths & paths = tab.first;
			UIInputProject * project_ptr = static_cast<UIInputProject*>(tab.second.release());
			RawCloseInputProject(project_ptr);

		});

	});

	for_each(output_tabs.begin(), output_tabs.end(), [this](std::pair<NewGeneMainWindow * const, OutputProjectTabs> & windows)
	{

		OutputProjectTabs & tabs = windows.second;
		for_each(tabs.begin(), tabs.end(), [this](OutputProjectTab & tab)
		{

			//ProjectPaths & paths = tab.first;
			UIOutputProject * project_ptr = static_cast<UIOutputProject*>(tab.second.release());
			RawCloseOutputProject(project_ptr);

		});

	});

	EndLoopAndBackgroundPool();

};

void UIProjectManager::LoadOpenProjects(NewGeneMainWindow* mainWindow, QObject * mainWindowObject)
{
	UIMessager messager;
	UIMessagerSingleShot messager_(messager);
	InputProjectFilesList::instance input_project_list = InputProjectFilesList::get(messager);
	OutputProjectFilesList::instance output_project_list = OutputProjectFilesList::get(messager);

	connect(mainWindowObject, SIGNAL(SignalCloseCurrentInputDataset()), this, SLOT(CloseCurrentInputDataset()));
	connect(mainWindowObject, SIGNAL(SignalCloseCurrentOutputDataset()), this, SLOT(CloseCurrentOutputDataset()));
	connect(mainWindowObject, SIGNAL(SignalOpenInputDataset(STD_STRING, QObject *)), this, SLOT(OpenInputDataset(STD_STRING, QObject *)));
	connect(mainWindowObject, SIGNAL(SignalOpenOutputDataset(STD_STRING, QObject *)), this, SLOT(OpenOutputDataset(STD_STRING, QObject *)));

	bool success = false;

	if (input_project_list->files.size() == 0)
	{
		boost::filesystem::path input_project_path = settingsManagerUI().ObtainGlobalPath(QStandardPaths::DataLocation, NewGeneFileNames::defaultInputProjectFileName);
		if (input_project_path != boost::filesystem::path())
		{
			settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_INPUT_PROJECTS_LIST, InputProjectFilesList(messager, input_project_path.string().c_str()));
			settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_INPUT_DATASET_FOLDER_PATH, OpenInputFilePath(messager, input_project_path.parent_path()));
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
		else
		{
			//InputProjectTabs & tabs = input_tabs[mainWindow];
			//for_each(tabs.begin(), tabs.end(), [](InputProjectTab & tab)
			//{
				//ProjectPaths & paths = tab.first;
				//UIInputProject * project_ptr = static_cast<UIInputProject*>(tab.second.get());
			//});
		}

		if (create_new_instance)
		{

			success = RawOpenInputProject(messager, input_project_settings_path, mainWindowObject);

		}

	}

	if (success && output_project_list->files.size() == 0)
	{
		boost::filesystem::path output_project_path = settingsManagerUI().ObtainGlobalPath(QStandardPaths::DataLocation, NewGeneFileNames::defaultOutputProjectFileName);
		if (output_project_path != boost::filesystem::path())
		{
			settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_OUTPUT_PROJECTS_LIST, OutputProjectFilesList(messager, output_project_path.string().c_str()));
			settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_OUTPUT_DATASET_FOLDER_PATH, OpenOutputFilePath(messager, output_project_path.parent_path()));
			output_project_list = OutputProjectFilesList::get(messager);
		}
	}

	if (success && output_project_list->files.size() == 1)
	{

		boost::filesystem::path output_project_settings_path = output_project_list->files[0];

		bool create_new_instance = false;

		if (output_tabs.find(mainWindow) == output_tabs.cend())
		{
			create_new_instance = true;
		}
		else
		{
			//OutputProjectTabs & tabs = output_tabs[mainWindow];
			//for_each(tabs.begin(), tabs.end(), [](OutputProjectTab & tab)
			//{
				//ProjectPaths & paths = tab.first;
				//UIOutputProject * project_ptr = static_cast<UIOutputProject*>(tab.second.get());
			//});
		}

		if (create_new_instance)
		{

			success = RawOpenOutputProject(messager, output_project_settings_path, mainWindowObject);

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

	UIInputProject * project = static_cast<UIInputProject *>(tab.second.get());

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

	UIOutputProject * project = static_cast<UIOutputProject *>(tab.second.get());

	return project;

}

void UIProjectManager::SignalMessageBox(STD_STRING msg)
{

	QMessageBox msgBox;
	msgBox.setText( msg.c_str() );
	msgBox.exec();

}

void UIProjectManager::DoneLoadingFromDatabase(UI_INPUT_MODEL_PTR model_)
{

	UIMessagerSingleShot messager;

	if (!getActiveUIInputProject()->is_model_equivalent(messager.get(), model_))
	{
		messager.get().AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__INPUT_MODELS_DO_NOT_MATCH, "Input model has completed loading from database, but does not match current input model."));
		return;
	}

	if (!model_->loaded())
	{
		messager.get().AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__INPUT_MODEL_NOT_LOADED, "Input model has completed loading from database, but is marked as not loaded."));
		return;
	}

	if (getActiveUIOutputProject() == nullptr)
	{
		return;
	}

	emit LoadFromDatabase(&getActiveUIOutputProject()->model());

}

void UIProjectManager::DoneLoadingFromDatabase(UI_OUTPUT_MODEL_PTR model_)
{

	UIMessagerSingleShot messager;

	if (!getActiveUIOutputProject()->is_model_equivalent(messager.get(), model_))
	{
		messager.get().AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__OUTPUT_MODELS_DO_NOT_MATCH, "Output model has completed loading from database, but does not match current output model."));
		return;
	}

	if (!model_->loaded())
	{
		messager.get().AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__OUTPUT_MODEL_NOT_LOADED, "Output model has completed loading from database, but is marked as not loaded."));
		return;
	}

	getActiveUIInputProject()->DoRefreshAllWidgets();
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
		boost::format msg("Please open an input dataset before attempting to open an output dataset.");
		QMessageBox msgBox;
		msgBox.setText( msg.str().c_str() );
		msgBox.exec();
		return;
	}

	bool success = false;

	if (boost::filesystem::exists(the_output_dataset) && !boost::filesystem::is_directory(the_output_dataset))
	{
		success = RawOpenOutputProject(messager, boost::filesystem::path(the_output_dataset), mainWindowObject);
	}

	if (success)
	{
		settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_OUTPUT_PROJECTS_LIST, OutputProjectFilesList(messager, the_output_dataset));
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

	if (!tab.second)
	{
		tabs.clear();
		return;
	}

	UIOutputProject * project_ptr = static_cast<UIOutputProject*>(tab.second.release());
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
	if (boost::filesystem::exists(the_input_dataset) && !boost::filesystem::is_directory(the_input_dataset))
	{
		success = RawOpenInputProject(messager, boost::filesystem::path(the_input_dataset), mainWindowObject);
	}

	if (success)
	{

		settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_INPUT_PROJECTS_LIST, InputProjectFilesList(messager, the_input_dataset));

		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(nullptr, QString("Open output project?"), QString("Would you also like to open an associated output project?"), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));
		if (reply == QMessageBox::Yes)
		{
			OpenOutputFilePath::instance folder_path = OpenOutputFilePath::get(messager);
			QWidget * mainWindow = nullptr;
			try
			{
				mainWindow = dynamic_cast<QWidget*>(mainWindowObject);
			}
			catch (std::bad_cast &)
			{

			}
			if (mainWindow)
			{
				QString the_file = QFileDialog::getOpenFileName(mainWindow, "Choose output dataset", folder_path ? folder_path->getPath().string().c_str() : "", "NewGene output settings file (*.newgene.out.xml)");
				if (the_file.size())
				{
					if (boost::filesystem::exists(the_file.toStdString()) && !boost::filesystem::is_directory(the_file.toStdString()))
					{
						boost::filesystem::path file_path(the_file.toStdString());
						settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_OUTPUT_DATASET_FOLDER_PATH, OpenOutputFilePath(messager, file_path.parent_path()));
						OpenOutputDataset(file_path.string(), mainWindowObject);
					}
				}
			}
		}

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

	if (!tab.second)
	{
		tabs.clear();
		return;
	}

	UIInputProject * project_ptr = static_cast<UIInputProject*>(tab.second.release());
	RawCloseInputProject(project_ptr);

	tabs.clear();

	UIMessager messager;
	settingsManagerUI().globalSettings().getUISettings().UpdateSetting(messager, GLOBAL_SETTINGS_UI_NAMESPACE::OPEN_INPUT_PROJECTS_LIST, InputProjectFilesList(messager, ""));

}

bool UIProjectManager::RawOpenInputProject(UIMessager & messager, boost::filesystem::path const & input_project_settings_path, QObject * mainWindowObject)
{

	if (!boost::filesystem::exists(input_project_settings_path) || boost::filesystem::is_directory(input_project_settings_path))
	{
		QMessageBox msgBox;
		boost::format msg("%1%, the input project settings filename, is not a valid file.");
		msg % input_project_settings_path.string();
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return false;
	}

	// Internally creates both an instance of UI-layer project settings, and an instance of backend-layer project settings
	// via SettingsRepositoryFactory
	std::shared_ptr<UIInputProjectSettings> project_settings(new UIInputProjectSettings(messager, input_project_settings_path));
	project_settings->WriteSettingsToFile(messager); // Writes default settings for those settings not already present

	// Internally creates an instance of backend-layer model settings via SettingsRepositoryFactory
	auto path_to_model_settings_ = InputProjectPathToModel::get(messager, project_settings->getBackendSettings());
	boost::filesystem::path path_to_model_settings = path_to_model_settings_->getPath();
	if (path_to_model_settings.is_relative())
	{
		boost::filesystem::path new_path = input_project_settings_path.parent_path();
		new_path /= path_to_model_settings;
		path_to_model_settings = new_path;
	}
	if (!boost::filesystem::exists(path_to_model_settings) || boost::filesystem::is_directory(path_to_model_settings))
	{
		QMessageBox msgBox;
		boost::format msg("%1%, the input model settings filename, is not a valid file.");
		msg % path_to_model_settings.string();
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return false;
	}
	std::shared_ptr<UIInputModelSettings> model_settings(new UIInputModelSettings(messager, path_to_model_settings));
	model_settings->WriteSettingsToFile(messager); // Writes default settings for those settings not already present

	// Backend model does not know its settings, because multiple settings might point to the same model.
	auto path_to_model_database_ = InputModelPathToDatabase::get(messager, model_settings->getBackendSettings());
	boost::filesystem::path path_to_model_database = path_to_model_database_->getPath();
	if (path_to_model_database.is_relative())
	{
		boost::filesystem::path new_path = path_to_model_settings.parent_path();
		new_path /= path_to_model_database;
		path_to_model_database = new_path;
	}
	if (!boost::filesystem::exists(path_to_model_database) || boost::filesystem::is_directory(path_to_model_database))
	{
		QMessageBox msgBox;
		boost::format msg("%1%, the input model database filename, is not a valid file.");
		msg % path_to_model_database.string();
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return false;
	}
	std::shared_ptr<InputModel> backend_model(ModelFactory<InputModel>()(messager, path_to_model_database));
	std::shared_ptr<UIInputModel> project_model(new UIInputModel(messager, backend_model));

	NewGeneMainWindow * mainWindow = nullptr;
	try
	{
		mainWindow = dynamic_cast<NewGeneMainWindow *>(mainWindowObject);
	}
	catch (std::bad_cast &)
	{
		return false;
	}

	if (mainWindow == nullptr)
	{
		return false;
	}

    input_tabs[mainWindow].push_back(std::make_pair(ProjectPaths(input_project_settings_path, path_to_model_settings, path_to_model_database),
                                                    std::unique_ptr<UIInputProject>(new UIInputProject(project_settings, model_settings, project_model, mainWindowObject))));

    UIInputProject * project = getActiveUIInputProject();

	if (!project)
	{
		boost::format msg("No input dataset is open.");
		messager.AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__PROJECT_IS_NULL, msg.str()));
		return false;
	}

	project->InitializeEventLoop(project); // cannot use 'this' in base class with multiple inheritance
	project->model().InitializeEventLoop(&project->model()); // cannot use 'this' in base class with multiple inheritance
	project->modelSettings().InitializeEventLoop(&project->modelSettings()); // cannot use 'this' in base class with multiple inheritance
	project->projectSettings().InitializeEventLoop(&project->projectSettings()); // cannot use 'this' in base class with multiple inheritance

	project_settings->UpdateConnections();
	model_settings->UpdateConnections();
	project_model->UpdateConnections();
	project->UpdateConnections();

	// blocks, because all connections are in NewGeneWidget which are all associated with the UI event loop
	emit UpdateInputConnections(ESTABLISH_CONNECTIONS_INPUT_PROJECT, project);

	emit LoadFromDatabase(&project->model());

	boost::format msg("%1% successfully loaded.");
	msg % input_project_settings_path;
	messager.UpdateStatusBarText(msg.str(), nullptr);

	return true;

}

bool UIProjectManager::RawOpenOutputProject(UIMessager & messager, boost::filesystem::path const & output_project_settings_path, QObject * mainWindowObject)
{

	if (!boost::filesystem::exists(output_project_settings_path) || boost::filesystem::is_directory(output_project_settings_path))
	{
		QMessageBox msgBox;
		boost::format msg("%1%, the input project settings filename, is not a valid file.");
		msg % output_project_settings_path.string();
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return false;
	}

	// Internally creates both an instance of UI-layer project settings, and an instance of backend-layer project settings
	// via SettingsRepositoryFactory
	std::shared_ptr<UIOutputProjectSettings> project_settings(new UIOutputProjectSettings(messager, output_project_settings_path));
	project_settings->WriteSettingsToFile(messager); // Writes default settings for those settings not already present

	// Internally creates an instance of backend-layer model settings via SettingsRepositoryFactory
	auto path_to_model_settings_ = OutputProjectPathToModel::get(messager, project_settings->getBackendSettings());
	boost::filesystem::path path_to_model_settings = path_to_model_settings_->getPath();
	if (path_to_model_settings.is_relative())
	{
		boost::filesystem::path new_path = output_project_settings_path.parent_path();
		new_path /= path_to_model_settings;
		path_to_model_settings = new_path;
	}
	if (!boost::filesystem::exists(path_to_model_settings) || boost::filesystem::is_directory(path_to_model_settings))
	{
		QMessageBox msgBox;
		boost::format msg("%1%, the output model settings filename, is not a valid file.");
		msg % path_to_model_settings.string();
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return false;
	}
	std::shared_ptr<UIOutputModelSettings> model_settings(new UIOutputModelSettings(messager, path_to_model_settings));
	model_settings->WriteSettingsToFile(messager); // Writes default settings for those settings not already present

	// The input model and settings are necessary in order to instantiate the output model
	UIInputProject * input_project = getActiveUIInputProject();
	if (!input_project)
	{
		boost::format msg("NULL input project during attempt to instantiate output project.");
		messager.AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__PROJECT_IS_NULL, msg.str()));
		return false;
	}

	// Backend model does not know its settings, because multiple settings might point to the same model.
	auto path_to_model_database_ = OutputModelPathToDatabase::get(messager, model_settings->getBackendSettings());
	boost::filesystem::path path_to_model_database = path_to_model_database_->getPath();
	if (path_to_model_database.is_relative())
	{
		boost::filesystem::path new_path = path_to_model_settings.parent_path();
		new_path /= path_to_model_database;
		path_to_model_database = new_path;
	}
	if (!boost::filesystem::exists(path_to_model_database) || boost::filesystem::is_directory(path_to_model_database))
	{
		QMessageBox msgBox;
		boost::format msg("%1%, the output model database filename, is not a valid file.");
		msg % path_to_model_database.string();
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return false;
	}
	std::shared_ptr<OutputModel> backend_model(ModelFactory<OutputModel>()(messager, path_to_model_database, std::dynamic_pointer_cast<InputModelSettings>(input_project->backend().modelSettingsSharedPtr()), input_project->backend().modelSharedPtr()));
	std::shared_ptr<UIOutputModel> project_model(new UIOutputModel(messager, backend_model));

	NewGeneMainWindow * mainWindow = nullptr;
	try
	{
		mainWindow = dynamic_cast<NewGeneMainWindow *>(mainWindowObject);
	}
	catch (std::bad_cast &)
	{
		return false;
	}

	if (mainWindow == nullptr)
	{
		return false;
	}

	output_tabs[mainWindow].push_back(std::make_pair(ProjectPaths(output_project_settings_path, path_to_model_settings, path_to_model_database),
													std::unique_ptr<UIOutputProject>(new UIOutputProject(project_settings, model_settings, project_model, mainWindowObject))));

	UIOutputProject * project = getActiveUIOutputProject();

	if (!project)
	{
		boost::format msg("NULL output project during attempt to instantiate project.");
		messager.AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__PROJECT_IS_NULL, msg.str()));
		return false;
	}

	project->InitializeEventLoop(project); // cannot use 'this' in base class with multiple inheritance
	project->model().InitializeEventLoop(&project->model()); // cannot use 'this' in base class with multiple inheritance
	project->modelSettings().InitializeEventLoop(&project->modelSettings()); // cannot use 'this' in base class with multiple inheritance
	project->projectSettings().InitializeEventLoop(&project->projectSettings()); // cannot use 'this' in base class with multiple inheritance

	project_settings->UpdateConnections();
	model_settings->UpdateConnections();
	project_model->UpdateConnections();
	project->UpdateConnections();

	// blocks, because all connections are in NewGeneWidget which are all associated with the UI event loop
	emit UpdateOutputConnections(ESTABLISH_CONNECTIONS_OUTPUT_PROJECT, project);

	emit LoadFromDatabase(&getActiveUIOutputProject()->model());

	boost::format msg("%1% successfully loaded.");
	msg % output_project_settings_path;
	messager.UpdateStatusBarText(msg.str(), nullptr);

	return true;

}

void UIProjectManager::RawCloseInputProject(UIInputProject * input_project)
{

	if (!input_project)
	{
		return;
	}

	// blocks, because all connections are in NewGeneWidget which are all associated with the UI event loop
	emit UpdateInputConnections(RELEASE_CONNECTIONS_INPUT_PROJECT, input_project);

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

	// blocks, because all connections are in NewGeneWidget which are all associated with the UI event loop
	emit this->UpdateOutputConnections(RELEASE_CONNECTIONS_OUTPUT_PROJECT, output_project);

	output_project->model().EndLoopAndBackgroundPool(); // blocks
	output_project->modelSettings().EndLoopAndBackgroundPool(); // blocks
	output_project->projectSettings().EndLoopAndBackgroundPool(); // blocks
	output_project->EndLoopAndBackgroundPool(); // blocks

	output_project->deleteLater();

}
