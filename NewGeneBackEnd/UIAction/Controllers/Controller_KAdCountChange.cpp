#include "../UIActionManager.h"

#ifndef Q_MOC_RUN
	#include <boost/scope_exit.hpp>
#endif
#include "../../Project/InputProject.h"
#include "../../Project/OutputProject.h"
#include "../../UIData/DataChanges.h"
#include "../../UIAction/ActionChanges.h"

/************************************************************************/
// ACTION_KAD_COUNT_CHANGE
/************************************************************************/
void UIActionManager::DoKAdCountChange(Messager & messager, WidgetActionItemRequest_ACTION_KAD_COUNT_CHANGE const & action_request, OutputProject & project)
{

	if (FailIfBusy(messager))
	{
		return;
	}

	BOOST_SCOPE_EXIT(this_)
	{
		this_->EndFailIfBusy();
	} BOOST_SCOPE_EXIT_END

	if (!action_request.items)
	{
		return;
	}

	OutputModel & output_model = project.model();
	InputModel & input_model = project.model().getInputModel();

	switch (action_request.reason)
	{
		case WIDGET_ACTION_ITEM_REQUEST_REASON__ADD_ITEMS:
			{

			}
			break;

		case WIDGET_ACTION_ITEM_REQUEST_REASON__REMOVE_ITEMS:
			{

			}
			break;

		case WIDGET_ACTION_ITEM_REQUEST_REASON__UPDATE_ITEMS:
			{

				DataChangeMessage change_response(&project);

				for_each(action_request.items->cbegin(), action_request.items->cend(), [&input_model, &output_model, &messager, &change_response](InstanceActionItem const & instanceActionItem)
				{

					Executor executor(input_model.getDb());

					if (!instanceActionItem.second)
					{
						return;
					}

					WidgetInstanceIdentifier const & identifier = instanceActionItem.first;

					if (!identifier.uuid)
					{
						return;
					}

					// ************************************* //
					// Retrieve data sent by user interface
					// ************************************* //
					WidgetActionItem const & actionItem = *instanceActionItem.second;
					WidgetActionItem__Spinbox const & actionItemSpinbox = static_cast<WidgetActionItem__Spinbox const &>(actionItem);

					// ***************************************** //
					// Prepare data to send back to user interface
					// ***************************************** //
					DATA_CHANGE_TYPE type = DATA_CHANGE_TYPE__OUTPUT_MODEL__KAD_COUNT_CHANGE;
					DATA_CHANGE_INTENTION intention = DATA_CHANGE_INTENTION__UPDATE;
					WidgetInstanceIdentifiers child_identifiers;
					child_identifiers.push_back(identifier);
					DataChange change(type, intention, WidgetInstanceIdentifier(identifier), child_identifiers);
					change.SetPacket(std::make_shared<DataChangePacket_int>(actionItemSpinbox.getValue()));
					change_response.changes.push_back(change);

					// ***************************************** //
					// Update database and cache
					// ***************************************** //
					output_model.t_kad_count.Update(output_model.getDb(), output_model, input_model, change_response);

					executor.success();

				});

				messager.EmitChangeMessage(change_response);

			}
			break;

		default:
			break;
	}

}
