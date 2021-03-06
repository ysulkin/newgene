#ifndef EVENTLOOPTHREADMANAGER_H
#define EVENTLOOPTHREADMANAGER_H

#include <QThread>
#include "../../../NewGeneBackEnd/Threads/ThreadPool.h"
#include "../../../NewGeneBackEnd/Threads/WorkerThread.h"
#include "workqueuemanager.h"

template<WORK_QUEUE_THREAD_LOOP_CLASS_ENUM UI_THREAD_LOOP_CLASS_ENUM>
class EventLoopThreadManager
{

	public:

		EventLoopThreadManager(UIMessager & messager, int const number_worker_threads, int const number_pools = 1)
			: work(work_service)
			, worker_pool_ui(messager, work_service, number_worker_threads)
			, work_2(work_service_2)
			, worker_pool_ui_2(messager, work_service_2, number_worker_threads, number_pools == 2)
			, work_queue_manager(nullptr)
			, work_queue_manager_2(nullptr)
		{
		}

	public:

		void InitializeEventLoop(void * me, int stackSize = 0)
		{
			work_queue_manager.reset(InstantiateWorkQueue(me));

			if (worker_pool_ui_2.isActive())
			{
				work_queue_manager_2.reset(InstantiateWorkQueue(me, true));
			}

#			ifdef Q_OS_WIN
			// Barfs on OSX - won't run the thread
			if (stackSize > 0)
			{
				work_queue_manager_thread.setStackSize(static_cast<uint>(stackSize));
			}
#			endif

			work_queue_manager_thread.start();

			work_queue_manager->moveToThread(&work_queue_manager_thread);

			if (worker_pool_ui_2.isActive())
			{
				work_queue_manager_2->moveToThread(&work_queue_manager_thread);
			}
		}

		QObject * getConnector()
		{
			return work_queue_manager.get();
		}

		QObject * getConnectorTwo()
		{
			return work_queue_manager_2.get();
		}

		WorkQueueManager<UI_THREAD_LOOP_CLASS_ENUM> * getQueueManager()
		{
			return work_queue_manager.get();
		}

		WorkQueueManager<UI_THREAD_LOOP_CLASS_ENUM> * getQueueManagerTwo()
		{
			return work_queue_manager_2.get();
		}

		QThread & getQueueManagerThread()
		{
			return work_queue_manager_thread;
		}

		void EndLoopAndBackgroundPool()
		{
			// First, kill the Boost thread pool (but complete pending tasks first)
			StopPoolAndWaitForTasksToComplete(); // Do this here so that this complete object is valid, and Qt thread is still running, while background tasks run to completion

			// Only now kill the Qt-layer event loop
			getQueueManagerThread().quit();
		}

		boost::asio::io_service & getWorkService()
		{
			return work_service;
		}

		boost::asio::io_service & getWorkServiceTwo()
		{
			return work_service_2;
		}

	protected:

		virtual WorkQueueManager<UI_THREAD_LOOP_CLASS_ENUM> * InstantiateWorkQueue(void *, bool isPool2_ = false)
		{
			Q_UNUSED(isPool2_);
			return nullptr;
		}

		// Boost-layer thread pool, which is accessible in both the UI layer
		// and in the backend layer
		boost::asio::io_service work_service;
		boost::asio::io_service::work work;
		ThreadPool<WorkerThread> worker_pool_ui;

		// Secondary Boost-layer thread pool, when requested
		boost::asio::io_service work_service_2;
		boost::asio::io_service::work work_2;
		ThreadPool<WorkerThread> worker_pool_ui_2;

		// Qt-layer thread, which runs the event loop/s in a separate background thread
		QThread work_queue_manager_thread;
		std::unique_ptr<WorkQueueManager<UI_THREAD_LOOP_CLASS_ENUM>> work_queue_manager;
		std::unique_ptr<WorkQueueManager<UI_THREAD_LOOP_CLASS_ENUM>> work_queue_manager_2;

	private:

		void StopPoolAndWaitForTasksToComplete()
		{
			// blocks
			worker_pool_ui.StopPoolAndWaitForTasksToComplete();
			worker_pool_ui_2.StopPoolAndWaitForTasksToComplete();
		}

};

#endif // EVENTLOOPTHREADMANAGER_H
