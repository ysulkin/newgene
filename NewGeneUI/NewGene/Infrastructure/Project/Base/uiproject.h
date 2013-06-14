#ifndef UIPROJECT_H
#define UIPROJECT_H

//#include "globals.h"
#include <QObject>
#include <memory>
#ifndef Q_MOC_RUN
#	include <boost/filesystem.hpp>
#	include <boost/asio/io_service.hpp>
#endif
#include "../../../NewGeneBackEnd/Project/Project.h"
#include "../Model/Base/uimodel.h"
#include "../../../NewGeneBackEnd/Threads/ThreadPool.h"
#include "../../../NewGeneBackEnd/Threads/WorkerThread.h"
#include "workqueuemanager.h"
#include <QThread>

class UIModelManager;
class UISettingsManager;
class UIDocumentManager;
class UIStatusManager;
class UIProjectManager;
class NewGeneMainWindow;
class UILoggingManager;
class UIProjectManager;

template<typename BACKEND_PROJECT_CLASS, typename UI_PROJECT_SETTINGS_CLASS, typename UI_MODEL_SETTINGS_CLASS, typename UI_MODEL_CLASS>
class UIProject
{
	public:

		static int const number_worker_threads = 1; // For now, single thread only in pool

		UIProject(UIMessager * messager, std::shared_ptr<UI_PROJECT_SETTINGS_CLASS> const & ui_settings,
										 std::shared_ptr<UI_MODEL_SETTINGS_CLASS> const & ui_model_settings,
										 std::shared_ptr<UI_MODEL_CLASS> const & ui_model)
			: project_messager(messager)
			, _project_settings(ui_settings)
			, _model_settings(ui_model_settings)
			, _model(ui_model)
			, _backend_project( new BACKEND_PROJECT_CLASS(*project_messager, _project_settings->getBackendSettingsSharedPtr(), _model_settings->getBackendSettingsSharedPtr(), _model->getBackendModelSharedPtr()) )
			, work(work_service)
			, worker_pool_ui(work_service, number_worker_threads)
		{
			work_queue_manager_thread.start();
			work_queue_manager.moveToThread(&work_queue_manager_thread);
		}

		~UIProject()
		{
		}

		// TODO: Test for validity
		UI_PROJECT_SETTINGS_CLASS & projectSettings()
		{
			return *_project_settings;
		}

		// TODO: Test for validity
		UI_PROJECT_SETTINGS_CLASS & modelSettings()
		{
			return backend().modelSettings();
		}

		// TODO: Test for validity
		UI_MODEL_CLASS & model()
		{
			return *_model;
		}

		// TODO: Test for validity
		BACKEND_PROJECT_CLASS & backend()
		{
			return *_backend_project;
		}

		WorkQueueManager & getQueueManager()
		{
			return work_queue_manager;
		}

		QThread & getQueueManagerThread()
		{
			return work_queue_manager_thread;
		}

	protected:

		// The order of initialization is important.
		// C++ data members are initialized in the order they appear
		// in the class declaration (this file).
		// Do not reorder the declarations of these member variables.

		std::unique_ptr<UIMessager> project_messager;
		std::shared_ptr<UI_PROJECT_SETTINGS_CLASS> const _project_settings;
		std::shared_ptr<UI_MODEL_SETTINGS_CLASS> const _model_settings;
		std::shared_ptr<UI_MODEL_CLASS> const _model;
		std::unique_ptr<BACKEND_PROJECT_CLASS> const _backend_project;

		boost::asio::io_service work_service;
		boost::asio::io_service::work work;
		ThreadPool<WorkerThread> worker_pool_ui;

		QThread work_queue_manager_thread;
		WorkQueueManager work_queue_manager;

};

#endif // UIPROJECT_H
