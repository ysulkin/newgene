#ifndef THREADPOOL_H
#define THREADPOOL_H

#ifndef Q_MOC_RUN
	#include <boost/asio/io_service.hpp>
	#include <boost/thread.hpp>
#endif
#include <memory>
#include <vector>

#include "../Messager/Messager.h"


template<typename WORKER_THREAD_CLASS>
class ThreadPool
{

	public:

		typedef std::pair<std::shared_ptr<boost::thread>, std::shared_ptr<WORKER_THREAD_CLASS>> thread_worker;

		ThreadPool(Messager & messager, boost::asio::io_service & work_service, int const nThreads, bool const active_ = true)
			: active(active_)
		{
			if (!active)
			{
				return;
			}

			for (int n = 0; n < nThreads; ++n)
			{
				auto worker = std::make_shared<WORKER_THREAD_CLASS>(messager, work_service);
				threads.push_back(std::make_pair(std::make_shared<boost::thread>(boost::bind(&WORKER_THREAD_CLASS::operator(), worker)), worker));
			}
		}

		void StopPoolAndWaitForTasksToComplete()
		{
			if (!active)
			{
				return;
			}

			// blocks
			for_each(threads.begin(), threads.end(), [](thread_worker & thread)
			{
				thread.second->stop(); // shuts down "work" so that the service will exit when all existing tasks are completed and will not accept new tasks
				thread.first->join(); // Now wait for existing tasks to complete
			});
		}

		~ThreadPool()
		{
		}

		bool isActive()
		{
			return active;
		}

	protected:

		std::vector<thread_worker> threads;
		bool const active;

};

#endif
