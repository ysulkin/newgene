#include "newgenevariablegroup.h"
#include "ui_newgenevariablegroup.h"

#include <QStandardItem>

NewGeneVariableGroup::NewGeneVariableGroup( QWidget * parent, WidgetInstanceIdentifier data_instance_, UIOutputProject * project ) :

	QWidget( parent ),

	NewGeneWidget( WidgetCreationInfo(
										this, // 'this' pointer is cast by compiler to proper Widget instance, which is already created due to order in which base classes appear in class definition
										parent,
										WIDGET_NATURE_OUTPUT_WIDGET,
										VARIABLE_GROUP_VARIABLE_GROUP_INSTANCE,
										false,
										data_instance_
									 )
				 ),

	ui( new Ui::NewGeneVariableGroup )

{

	ui->setupUi( this );

	PrepareOutputWidget();

	if (data_instance.uuid && project)
	{

		project->RegisterInterestInChange(this, DATA_CHANGE_TYPE__OUTPUT_MODEL__VG_CATEGORY_SET_MEMBER_SELECTION, true, *data_instance.uuid);

		UpdateOutputConnections(UIProjectManager::ESTABLISH_CONNECTIONS_OUTPUT_PROJECT, project);
		WidgetDataItemRequest_VARIABLE_GROUP_VARIABLE_GROUP_INSTANCE request(WIDGET_DATA_ITEM_REQUEST_REASON__REFRESH_ALL_WIDGETS, data_instance);
		emit RefreshWidget(request);

	}

}

NewGeneVariableGroup::~NewGeneVariableGroup()
{
	if (outp)
	{
		outp->UnregisterInterestInChanges(this);
	}
	delete ui;
}

void NewGeneVariableGroup::UpdateOutputConnections(UIProjectManager::UPDATE_CONNECTIONS_TYPE connection_type, UIOutputProject * project)
{
	if (connection_type == UIProjectManager::RELEASE_CONNECTIONS_OUTPUT_PROJECT)
	{
		outp->UnregisterInterestInChanges(this);
	}

	NewGeneWidget::UpdateOutputConnections(connection_type, project);

	if (connection_type == UIProjectManager::ESTABLISH_CONNECTIONS_OUTPUT_PROJECT)
	{
		connect(this, SIGNAL(RefreshWidget(WidgetDataItemRequest_VARIABLE_GROUP_VARIABLE_GROUP_INSTANCE)), outp->getConnector(), SLOT(RefreshWidget(WidgetDataItemRequest_VARIABLE_GROUP_VARIABLE_GROUP_INSTANCE)));
		connect(this, SIGNAL(SignalReceiveVariableItemChanged(WidgetActionItemRequest_ACTION_VARIABLE_GROUP_SET_MEMBER_SELECTION_CHANGED)), outp->getConnector(), SLOT(ReceiveVariableItemChanged(WidgetActionItemRequest_ACTION_VARIABLE_GROUP_SET_MEMBER_SELECTION_CHANGED)));
	}
}

void NewGeneVariableGroup::changeEvent( QEvent * e )
{
	QWidget::changeEvent( e );

	switch ( e->type() )
	{
		case QEvent::LanguageChange:
			ui->retranslateUi( this );
			break;

		default:
			break;
	}
}

void NewGeneVariableGroup::RefreshAllWidgets()
{
	WidgetDataItemRequest_VARIABLE_GROUP_VARIABLE_GROUP_INSTANCE request(WIDGET_DATA_ITEM_REQUEST_REASON__REFRESH_ALL_WIDGETS);
	emit RefreshWidget(request);
}

void NewGeneVariableGroup::WidgetDataRefreshReceive(WidgetDataItem_VARIABLE_GROUP_VARIABLE_GROUP_INSTANCE widget_data)
{

	if (!data_instance.uuid || !widget_data.identifier || !widget_data.identifier->uuid || (*data_instance.uuid) != (*widget_data.identifier->uuid) )
	{
		boost::format msg("Invalid widget refresh in NewGeneVariableGroup widget.");
		QMessageBox msgBox;
		msgBox.setText( msg.str().c_str() );
		msgBox.exec();
		return;
	}

	if (!ui->listView)
	{
		boost::format msg("Invalid list view in NewGeneVariableGroup widget.");
		QMessageBox msgBox;
		msgBox.setText( msg.str().c_str() );
		msgBox.exec();
		return;
	}

	QStandardItemModel * oldModel = static_cast<QStandardItemModel*>(ui->listView->model());
	if (oldModel != nullptr)
	{
		delete oldModel;
	}

	QItemSelectionModel * oldSelectionModel = ui->listView->selectionModel();
	QStandardItemModel * model = new QStandardItemModel(ui->listView);

	int index = 0;
	std::for_each(widget_data.identifiers.cbegin(), widget_data.identifiers.cend(), [this, &index, &model](WidgetInstanceIdentifier_Bool_Pair const & identifier)
	{
		if (identifier.first.longhand && !identifier.first.longhand->empty())
		{

			QStandardItem * item = new QStandardItem();
			item->setText(QString(identifier.first.longhand->c_str()));
			item->setCheckable( true );
			if (identifier.second)
			{
				item->setCheckState(Qt::Checked);
			}
			else
			{
				item->setCheckState(Qt::Unchecked);
			}
			QVariant v;
			v.setValue(identifier);
			item->setData(v);
			model->setItem( index, item );

			++index;

		}
	});

	connect(model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(ReceiveVariableItemChanged(QStandardItem*)));

	ui->listView->setModel(model);
	if (oldSelectionModel) delete oldSelectionModel;

}

void NewGeneVariableGroup::ReceiveVariableItemChanged(QStandardItem * currentItem)
{

	QStandardItemModel * model = static_cast<QStandardItemModel*>(ui->listView->model());
	if (model == nullptr)
	{
		// Todo: messager error
		return;
	}

	InstanceActionItems actionItems;

	if (currentItem)
	{
		bool checked = false;
		if (currentItem->checkState() == Qt::Checked)
		{
			checked = true;
		}
		QVariant currentIdentifier = currentItem->data();
        WidgetInstanceIdentifier_Bool_Pair identifier_bool = currentIdentifier.value<WidgetInstanceIdentifier_Bool_Pair>();
        WidgetInstanceIdentifier identifier = identifier_bool.first;
		actionItems.push_back(std::make_pair(identifier, std::shared_ptr<WidgetActionItem>(static_cast<WidgetActionItem*>(new WidgetActionItem__Checkbox(checked)))));
	}

	WidgetActionItemRequest_ACTION_VARIABLE_GROUP_SET_MEMBER_SELECTION_CHANGED action_request(WIDGET_ACTION_ITEM_REQUEST_REASON__UPDATE_ITEMS, actionItems);
	emit SignalReceiveVariableItemChanged(action_request);

}

void NewGeneVariableGroup::HandleChanges(DataChangeMessage const & change_message)
{
	std::for_each(change_message.changes.cbegin(), change_message.changes.cend(), [this](DataChange const & change)
	{
		switch (change.change_type)
		{
			case DATA_CHANGE_TYPE::DATA_CHANGE_TYPE__OUTPUT_MODEL__VG_CATEGORY_SET_MEMBER_SELECTION:
				{
					switch (change.change_intention)
					{
						case DATA_CHANGE_INTENTION__ADD:
						case DATA_CHANGE_INTENTION__REMOVE:
							{
								// This is the OUTPUT model changing.
								// "Add" means to simply add an item that is CHECKED (previously unchecked) -
								// NOT to add a new variable.  That would be input model change type.

								QStandardItemModel * model = static_cast<QStandardItemModel*>(ui->listView->model());
								if (model == nullptr)
								{
									return; // from lambda
								}

								if (change.child_identifiers.size() == 0)
								{
									return; // from lambda
								}

								std::for_each(change.child_identifiers.cbegin(), change.child_identifiers.cend(), [&model, &change, this](WidgetInstanceIdentifier const & child_identifier)
								{
									int number_variables = model->rowCount();
									for (int n=0; n<number_variables; ++n)
									{
										QStandardItem * currentItem = model->item(n);
										if (currentItem)
										{
											bool checked = false;
											if (currentItem->checkState() == Qt::Checked)
											{
												checked = true;
											}
											QVariant currentIdentifier = currentItem->data();
											WidgetInstanceIdentifier identifier = currentIdentifier.value<WidgetInstanceIdentifier>();
											if (identifier.uuid && child_identifier.uuid && *identifier.uuid == *child_identifier.uuid)
											{

												if (change.change_intention == DATA_CHANGE_INTENTION__ADD)
												{
													if (!checked)
													{
														currentItem->setCheckState(Qt::Checked);
													}
												}
												else if (change.change_intention == DATA_CHANGE_INTENTION__REMOVE)
												{
													if (checked)
													{
														currentItem->setCheckState(Qt::Unchecked);
													}
												}

											}

										}
									}
								});

							}
							break;
						case DATA_CHANGE_INTENTION__UPDATE:
							{
								// Should never receive this.
							}
						case DATA_CHANGE_INTENTION__RESET_ALL:
							{
								// Ditto above.
							}
							break;
					}
				}
				break;
		}
	});
}
