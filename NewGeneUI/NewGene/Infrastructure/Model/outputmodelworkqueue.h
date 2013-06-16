#ifndef OUTPUTMODELWORKQUEUE_H
#define OUTPUTMODELWORKQUEUE_H

#include "Base/outputmodelworkqueue_base.h"

class UIOutputModel;

class OutputModelWorkQueue : public WorkQueueManager<UI_OUTPUT_MODEL>
{

	public:

		explicit OutputModelWorkQueue(bool isPool2_ = false, QObject * parent = NULL);

		void SetUIObject(void * ui_input_object_)
		{
			inp = ui_input_object_;
		}

		void SetConnections();

	private:

		void * inp;

	protected:

		UIOutputModel * get();

		void TestSlot();
		void LoadModel(void *);

};

#endif // OUTPUTMODELWORKQUEUE_H
