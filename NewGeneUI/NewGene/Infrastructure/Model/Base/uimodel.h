#ifndef UIMODEL_H
#define UIMODEL_H

#include <QObject>
#include "../../../NewGeneBackEnd/Model/Model.h"
#include "uimessager.h"
#include "../../../NewGeneBackEnd/globals.h"
#include "eventloopthreadmanager.h"

template<WORK_QUEUE_THREAD_LOOP_CLASS_ENUM UI_THREAD_LOOP_CLASS_ENUM>
class UIModel : public EventLoopThreadManager<UI_THREAD_LOOP_CLASS_ENUM>
{

	protected:
		static int const number_of_pools;

	public:

		UIModel(UIMessager & messager, int const number_worker_threads)
			: EventLoopThreadManager<UI_THREAD_LOOP_CLASS_ENUM>(number_worker_threads, number_of_pools)
		{

		}


	protected:

		template<typename BACKEND_MODEL_CLASS>
		class _impl_base
		{

			public:

				_impl_base(UIMessager &)
				{}

				class _RelatedImpl_base
				{

					public:

						BACKEND_MODEL_CLASS & getModel()
						{
							if (!_model)
							{
								boost::format msg( "Model instance not yet constructed." );
								throw NewGeneException() << newgene_error_description( msg.str() );
							}
							return *_model;
						}

						std::shared_ptr<BACKEND_MODEL_CLASS> & getModelSharedPtr()
						{
							return _model;
						}


					protected:

						_RelatedImpl_base(UIMessager & messager, std::shared_ptr<BACKEND_MODEL_CLASS> const & backend_model_instance)
							: _model(backend_model_instance)
						{
						}

						std::shared_ptr<BACKEND_MODEL_CLASS> _model;

				};

				std::unique_ptr<_RelatedImpl_base> __impl;

				_RelatedImpl_base & getInternalImplementation()
				{
					if (!__impl)
					{
						boost::format msg( "Internal backend model implementation not yet constructed." );
						throw NewGeneException() << newgene_error_description( msg.str() );
					}
					return *(__impl.get());
				}

		};


	protected:

		template<typename BACKEND_MODEL_CLASS>
		BACKEND_MODEL_CLASS &
		getBackendModel_base(_impl_base<BACKEND_MODEL_CLASS> & impl)
		{
			_impl_base<BACKEND_MODEL_CLASS>::_RelatedImpl_base & impl_backend = impl.getInternalImplementation();
			BACKEND_MODEL_CLASS & model = impl_backend.getModel();
			model;
		}

		template<typename BACKEND_MODEL_CLASS>
		std::shared_ptr<BACKEND_MODEL_CLASS>
		getBackendModelSharedPtr_base(_impl_base<BACKEND_MODEL_CLASS> & impl)
		{
			_impl_base<BACKEND_MODEL_CLASS>::_RelatedImpl_base & impl_backend = impl.getInternalImplementation();
			return impl_backend.getModelSharedPtr();
		}

};

template<WORK_QUEUE_THREAD_LOOP_CLASS_ENUM UI_THREAD_LOOP_CLASS_ENUM>
int const UIModel<UI_THREAD_LOOP_CLASS_ENUM>::number_of_pools = 2;

#endif // UImodel_H
