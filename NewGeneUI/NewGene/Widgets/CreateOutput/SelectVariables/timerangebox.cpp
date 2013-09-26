#include "timerangebox.h"
#include "ui_timerangebox.h"

#include <QCheckBox>
#include <QLineEdit>

TimeRangeBox::TimeRangeBox( QWidget * parent ) :
	QFrame( parent ),
	NewGeneWidget( WidgetCreationInfo(this, parent, WIDGET_NATURE_OUTPUT_WIDGET, TIMERANGE_REGION_WIDGET, true) ), // 'this' pointer is cast by compiler to proper Widget instance, which is already created due to order in which base classes appear in class definition
	ui( new Ui::TimeRangeBox )
{

	ui->setupUi( this );

	PrepareOutputWidget();

}

TimeRangeBox::~TimeRangeBox()
{
	delete ui;
}

void TimeRangeBox::changeEvent( QEvent * e )
{
	QFrame::changeEvent( e );

	switch ( e->type() )
	{
		case QEvent::LanguageChange:
			ui->retranslateUi( this );
			break;

		default:
			break;
	}
}

void TimeRangeBox::UpdateOutputConnections(UIProjectManager::UPDATE_CONNECTIONS_TYPE connection_type, UIOutputProject * project)
{

	NewGeneWidget::UpdateOutputConnections(connection_type, project);

	if (connection_type == UIProjectManager::ESTABLISH_CONNECTIONS_OUTPUT_PROJECT)
	{
		connect(this, SIGNAL(RefreshWidget(WidgetDataItemRequest_TIMERANGE_REGION_WIDGET)), outp->getConnector(), SLOT(RefreshWidget(WidgetDataItemRequest_TIMERANGE_REGION_WIDGET)));
		connect(project->getConnector(), SIGNAL(WidgetDataRefresh(WidgetDataItem_TIMERANGE_REGION_WIDGET)), this, SLOT(WidgetDataRefreshReceive(WidgetDataItem_TIMERANGE_REGION_WIDGET)));
		connect(this, SIGNAL(UpdateDoRandomSampling(WidgetActionItemRequest_ACTION_DO_RANDOM_SAMPLING_CHANGE)), outp->getConnector(), SLOT(ReceiveVariableItemChanged(WidgetActionItemRequest_ACTION_DO_RANDOM_SAMPLING_CHANGE)));
		connect(this, SIGNAL(UpdateRandomSamplingCount(WidgetActionItemRequest_ACTION_RANDOM_SAMPLING_COUNT_PER_STAGE_CHANGE)), outp->getConnector(), SLOT(ReceiveVariableItemChanged(WidgetActionItemRequest_ACTION_RANDOM_SAMPLING_COUNT_PER_STAGE_CHANGE)));
	}
	else if (connection_type == UIProjectManager::RELEASE_CONNECTIONS_OUTPUT_PROJECT)
	{
		Empty();
	}

}

void TimeRangeBox::RefreshAllWidgets()
{
	if (outp == nullptr)
	{
		Empty();
		return;
	}
	WidgetDataItemRequest_TIMERANGE_REGION_WIDGET request(WIDGET_DATA_ITEM_REQUEST_REASON__REFRESH_ALL_WIDGETS);
	emit RefreshWidget(request);
}

void TimeRangeBox::RefreshWidget(WidgetDataItemRequest_TIMERANGE_REGION_WIDGET)
{

}

void TimeRangeBox::WidgetDataRefreshReceive(WidgetDataItem_TIMERANGE_REGION_WIDGET widget_data)
{

	UIOutputProject * project = projectManagerUI().getActiveUIOutputProject();
	if (project == nullptr)
	{
		return;
	}

	UIMessager messager(project);

	bool do_random_sampling = widget_data.do_random_sampling;
	std::int64_t random_sampling_count_per_stage = widget_data.random_sampling_count_per_stage;

	QCheckBox * checkBox = this->findChild<QCheckBox*>("doRandomSampling");
	if (checkBox)
	{
		bool is_checked = checkBox->isCheckded();
		if (is_checked != do_random_sampling)
		{
			checkBox->setChecked(do_random_sampling);
		}
	}

	QLineEdit * lineEdit = this->findChild<QLineEdit*>("randomSamplingHowManyRows");
	if (lineEdit)
	{
		QString the_string = lineEdit->text();
		std::int64_t the_number = boost::lexical_cast<std::int64_t>(the_string.toStdString());
		if (the_number != random_sampling_count_per_stage)
		{
			lineEdit->setText(QString((boost::lexical_cast<std::string>(random_sampling_count_per_stage).c_str())));
		}
	}

}

void TimeRangeBox::on_checkBox_stateChanged(int arg1)
{

	UIOutputProject * project = projectManagerUI().getActiveUIOutputProject();
	if (project == nullptr)
	{
		return;
	}

	UIMessager messager(project);

	QCheckBox * checkBox = this->findChild<QCheckBox*>("doRandomSampling");
	if (checkBox)
	{
		bool is_checked = checkBox->isCheckded();
		InstanceActionItems actionItems;
		actionItems.push_back(std::make_pair(WidgetInstanceIdentifier(), std::shared_ptr<WidgetActionItem>(static_cast<WidgetActionItem*>(new WidgetActionItem__Checkbox(is_checked)))));
		WidgetActionItemRequest_ACTION_DO_RANDOM_SAMPLING_CHANGE action_request(WIDGET_ACTION_ITEM_REQUEST_REASON__UPDATE_ITEMS, actionItems);
		emit UpdateDoRandomSampling(action_request);
	}

}

void TimeRangeBox::on_lineEdit_editingFinished()
{

	QLineEdit * lineEdit = this->findChild<QLineEdit*>("randomSamplingHowManyRows");
	if (lineEdit)
	{
		QString the_string = lineEdit->text();
		std::int64_t the_number = boost::lexical_cast<std::int64_t>(the_string.toStdString());
		InstanceActionItems actionItems;
		actionItems.push_back(std::make_pair(WidgetInstanceIdentifier(), std::shared_ptr<WidgetActionItem>(static_cast<WidgetActionItem*>(new WidgetActionItem__Int64(the_number)))));
		WidgetActionItemRequest_ACTION_RANDOM_SAMPLING_COUNT_PER_STAGE_CHANGE action_request(WIDGET_ACTION_ITEM_REQUEST_REASON__UPDATE_ITEMS, actionItems);
		emit UpdateRandomSamplingCount(action_request);
	}

}
