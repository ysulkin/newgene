#include "UIDataManager.h"

#include "../Project/InputProject.h"
#include "../Project/OutputProject.h"

/************************************************************************/
// VARIABLE_GROUPS_SCROLL_AREA
/************************************************************************/
void UIDataManager::DoRefreshOutputWidget(Messager & messager, WidgetDataItemRequest_VARIABLE_GROUPS_SCROLL_AREA const & widget_request, OutputProject & project)
{

}
/************************************************************************/
// VARIABLE_GROUPS_TOOLBOX
/************************************************************************/
void UIDataManager::DoRefreshOutputWidget(Messager & messager, WidgetDataItemRequest_VARIABLE_GROUPS_TOOLBOX const & widget_request, OutputProject & project)
{
	if (sqlite3_threadsafe() == 0)
	{
		messager.ShowMessageBox("SQLite is not threadsafe!");
		return;
	}
	InputModel & input_model = project.model().getInputModel();
	WidgetDataItem_VARIABLE_GROUPS_TOOLBOX variable_groups(widget_request);
	variable_groups.identifiers = input_model.t_vgp_identifiers.getIdentifiers();
	messager.EmitOutputWidgetDataRefresh(variable_groups);
}

/************************************************************************/
// VARIABLE_GROUP_VARIABLE_GROUP_INSTANCE
/************************************************************************/
void UIDataManager::DoRefreshOutputWidget(Messager & messager, WidgetDataItemRequest_VARIABLE_GROUP_VARIABLE_GROUP_INSTANCE const & widget_request, OutputProject & project)
{
	InputModel & input_model = project.model().getInputModel();
	OutputModel & output_model = project.model();
	WidgetDataItem_VARIABLE_GROUP_VARIABLE_GROUP_INSTANCE variable_group(widget_request);
	if (widget_request.identifier && widget_request.identifier->uuid)
	{
		WidgetInstanceIdentifiers identifiers = input_model.t_vgp_setmembers.getIdentifiers(*widget_request.identifier->uuid);
		WidgetInstanceIdentifiers selectedIdentifiers = output_model.t_variables_selected_identifiers.getIdentifiers(*widget_request.identifier->uuid);
		std::for_each(identifiers.cbegin(), identifiers.cend(), [&selectedIdentifiers, &variable_group](WidgetInstanceIdentifier const & identifier)
		{
			bool found = false;
			std::for_each(selectedIdentifiers.cbegin(), selectedIdentifiers.cend(), [&identifier, &found, &variable_group](WidgetInstanceIdentifier const & selectedIdentifier)
			{
				if (identifier.IsEqual(WidgetInstanceIdentifier::EQUALITY_CHECK_TYPE__UUID_PLUS_STRING_CODE, selectedIdentifier))
				{
					found = true;
					return; // from lambda
				}
			});
			if (found)
			{
				variable_group.identifiers.push_back(std::make_pair(identifier, true));
			}
			else
			{
				variable_group.identifiers.push_back(std::make_pair(identifier, false));
			}
		});
	}
	std::sort(variable_group.identifiers.begin(), variable_group.identifiers.end());
	messager.EmitOutputWidgetDataRefresh(variable_group);
}

/************************************************************************/
// VARIABLE_GROUPS_SUMMARY
/************************************************************************/
void UIDataManager::DoRefreshOutputWidget(Messager & messager, WidgetDataItemRequest_VARIABLE_GROUPS_SUMMARY_SCROLL_AREA const & widget_request, OutputProject & project)
{
	InputModel & input_model = project.model().getInputModel();
	WidgetDataItem_VARIABLE_GROUPS_SUMMARY_SCROLL_AREA variable_groups(widget_request);
	variable_groups.identifiers = input_model.t_vgp_identifiers.getIdentifiers();
	std::sort(variable_groups.identifiers.begin(), variable_groups.identifiers.end());
	messager.EmitOutputWidgetDataRefresh(variable_groups);
}

/************************************************************************/
// VARIABLE_GROUPS_SUMMARY_VARIABLE_GROUP_INSTANCE
/************************************************************************/
void UIDataManager::DoRefreshOutputWidget(Messager & messager, WidgetDataItemRequest_VARIABLE_GROUPS_SUMMARY_VARIABLE_GROUP_INSTANCE const & widget_request, OutputProject & project)
{
	OutputModel & output_model = project.model();
	WidgetDataItem_VARIABLE_GROUPS_SUMMARY_VARIABLE_GROUP_INSTANCE variable_group(widget_request);
	if (widget_request.identifier && widget_request.identifier->uuid)
	{
		variable_group.identifiers = output_model.t_variables_selected_identifiers.getIdentifiers(*widget_request.identifier->uuid);
	}
	messager.EmitOutputWidgetDataRefresh(variable_group);
}

/************************************************************************/
// KAD_SPIN_CONTROLS_AREA
/************************************************************************/
void UIDataManager::DoRefreshOutputWidget(Messager & messager, WidgetDataItemRequest_KAD_SPIN_CONTROLS_AREA const & widget_request, OutputProject & project)
{
	InputModel & input_model = project.model().getInputModel();
	WidgetDataItem_KAD_SPIN_CONTROLS_AREA variable_groups(widget_request);
	variable_groups.active_dmus = project.model().t_variables_selected_identifiers.GetActiveDMUs(&project.model(), &project.model().getInputModel());
	variable_groups.identifiers = input_model.t_dmu_category.getIdentifiers();
	messager.EmitOutputWidgetDataRefresh(variable_groups);
}

/************************************************************************/
// KAD_SPIN_CONTROL_WIDGET
/************************************************************************/
void UIDataManager::DoRefreshOutputWidget(Messager & messager, WidgetDataItemRequest_KAD_SPIN_CONTROL_WIDGET const & widget_request, OutputProject & project)
{
	OutputModel & output_model = project.model();
	WidgetDataItem_KAD_SPIN_CONTROL_WIDGET kad_spincontrol(widget_request);
	if (widget_request.identifier && widget_request.identifier->uuid)
	{
		WidgetInstanceIdentifier_Int_Pair spinControlData = output_model.t_kad_count.getIdentifier(*widget_request.identifier->uuid);
		kad_spincontrol.count = spinControlData.second;
	}
	messager.EmitOutputWidgetDataRefresh(kad_spincontrol);
}

/************************************************************************/
// TIMERANGE_REGION_WIDGET
/************************************************************************/
void UIDataManager::DoRefreshOutputWidget(Messager & messager, WidgetDataItemRequest_TIMERANGE_REGION_WIDGET const & widget_request, OutputProject & project)
{
	OutputModel & output_model = project.model();
	WidgetDataItem_TIMERANGE_REGION_WIDGET timerange_region(widget_request);
	WidgetInstanceIdentifier_Int64_Pair timerange_start_identifier;
	std::pair<bool, std::int64_t> info = output_model.t_general_options.getRandomSamplingInfo(project.model().getDb());
	timerange_region.do_random_sampling = info.first;
	timerange_region.random_sampling_count_per_stage = info.second;
	messager.EmitOutputWidgetDataRefresh(timerange_region);
}

/************************************************************************/
// DATETIME_WIDGET
/************************************************************************/
void UIDataManager::DoRefreshOutputWidget(Messager & messager, WidgetDataItemRequest_DATETIME_WIDGET const & widget_request, OutputProject & project)
{
	OutputModel & output_model = project.model();
	WidgetDataItem_DATETIME_WIDGET timerange_datetimecontrol(widget_request);
	if (widget_request.identifier && widget_request.identifier->code && *widget_request.identifier->code == "0")
	{
		if (widget_request.identifier->flags == "s")
		{
			WidgetInstanceIdentifier_Int64_Pair timerange_start_identifier;
			bool found = output_model.t_time_range.getIdentifierFromStringCodeAndFlags("0", "s", timerange_start_identifier);
			if (found)
			{
				timerange_datetimecontrol.the_date_time = timerange_start_identifier.second;
			}
		}
		else if (widget_request.identifier->flags == "e")
		{
			WidgetInstanceIdentifier_Int64_Pair timerange_end_identifier;
			bool found = output_model.t_time_range.getIdentifierFromStringCodeAndFlags("0", "e", timerange_end_identifier);
			if (found)
			{
				timerange_datetimecontrol.the_date_time = timerange_end_identifier.second;
			}
		}
	}
	messager.EmitOutputWidgetDataRefresh(timerange_datetimecontrol);
}

/************************************************************************/
// GENERATE_OUTPUT_TAB
/************************************************************************/
void UIDataManager::DoRefreshOutputWidget(Messager & messager, WidgetDataItemRequest_GENERATE_OUTPUT_TAB const & widget_request, OutputProject & project)
{
	//OutputModel & output_model = project.model();
	WidgetDataItem_GENERATE_OUTPUT_TAB generate_output_tab_data(widget_request);
	WidgetInstanceIdentifier_Int64_Pair timerange_start_identifier;
	messager.EmitOutputWidgetDataRefresh(generate_output_tab_data);
}

/************************************************************************/
// MANAGE_DMUS_WIDGET
/************************************************************************/
void UIDataManager::DoRefreshInputWidget(Messager & messager, WidgetDataItemRequest_MANAGE_DMUS_WIDGET const & widget_request, InputProject & project)
{
	InputModel & input_model = project.model();
	WidgetDataItem_MANAGE_DMUS_WIDGET dmu_management(widget_request);
	WidgetInstanceIdentifiers dmus = input_model.t_dmu_category.getIdentifiers();
	std::for_each(dmus.cbegin(), dmus.cend(), [this, &dmu_management, &input_model](WidgetInstanceIdentifier const & single_dmu)
	{
		WidgetInstanceIdentifiers dmu_members = input_model.t_dmu_setmembers.getIdentifiers(*single_dmu.uuid);
		dmu_management.dmus_and_members.push_back(std::make_pair(single_dmu, dmu_members));
	});
	messager.EmitInputWidgetDataRefresh(dmu_management);
}

/************************************************************************/
// MANAGE_UOAS_WIDGET
/************************************************************************/
void UIDataManager::DoRefreshInputWidget(Messager & messager, WidgetDataItemRequest_MANAGE_UOAS_WIDGET const & widget_request, InputProject & project)
{
	InputModel & input_model = project.model();
	WidgetDataItem_MANAGE_UOAS_WIDGET uoa_management(widget_request);
	WidgetInstanceIdentifiers uoas = input_model.t_uoa_category.getIdentifiers();
	std::for_each(uoas.cbegin(), uoas.cend(), [this, &uoa_management, &input_model](WidgetInstanceIdentifier const & single_uoa)
	{
		if (!single_uoa.uuid || single_uoa.uuid->empty())
		{
			boost::format msg("Bad UOA in action handler.");
			throw NewGeneException() << newgene_error_description(msg.str());
		}
		WidgetInstanceIdentifiers dmu_categories = input_model.t_uoa_category.RetrieveDMUCategories(input_model.getDb(), &input_model, *single_uoa.uuid);
		uoa_management.uoas_and_dmu_categories.push_back(std::make_pair(single_uoa, dmu_categories));
	});
	messager.EmitInputWidgetDataRefresh(uoa_management);
}
