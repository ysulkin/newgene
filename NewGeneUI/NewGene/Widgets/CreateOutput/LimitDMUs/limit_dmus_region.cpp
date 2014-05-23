#include "limit_dmus_region.h"
#include "ui_limit_dmus_region.h"

#include "../Project/uiprojectmanager.h"
#include "../Project/uiinputproject.h"
#include "../Project/uioutputproject.h"

#include <QStandardItemModel>
#include "../../Utilities/qsortfilterproxymodel_numberslast.h"

#include <algorithm>
#include <iterator>

limit_dmus_region::limit_dmus_region(QWidget *parent) :
	QWidget(parent),
	NewGeneWidget( WidgetCreationInfo(this, parent, WIDGET_NATURE_OUTPUT_WIDGET, LIMIT_DMUS_TAB, true) ), // 'this' pointer is cast by compiler to proper Widget instance, which is already created due to order in which base classes appear in class definition
	ui( new Ui::limit_dmus_region )
{

	ui->setupUi( this );

	PrepareOutputWidget();

}

limit_dmus_region::~limit_dmus_region()
{

	delete ui;

}

void limit_dmus_region::changeEvent( QEvent * e )
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

void limit_dmus_region::UpdateOutputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIOutputProject * project)
{
	NewGeneWidget::UpdateOutputConnections(connection_type, project);

	if (connection_type == NewGeneWidget::ESTABLISH_CONNECTIONS_OUTPUT_PROJECT)
	{
		connect(this, SIGNAL(RefreshWidget(WidgetDataItemRequest_LIMIT_DMUS_TAB)), outp->getConnector(), SLOT(RefreshWidget(WidgetDataItemRequest_LIMIT_DMUS_TAB)));
		connect(project->getConnector(), SIGNAL(WidgetDataRefresh(WidgetDataItem_LIMIT_DMUS_TAB)), this, SLOT(WidgetDataRefreshReceive(WidgetDataItem_LIMIT_DMUS_TAB)));
	}
	else if (connection_type == NewGeneWidget::RELEASE_CONNECTIONS_OUTPUT_PROJECT)
	{
		Empty();
	}
}

void limit_dmus_region::UpdateInputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIInputProject * project)
{
	NewGeneWidget::UpdateInputConnections(connection_type, project);
	if (connection_type == NewGeneWidget::ESTABLISH_CONNECTIONS_INPUT_PROJECT)
	{
		if (project)
		{
			//project->RegisterInterestInChange(this, DATA_CHANGE_TYPE__INPUT_MODEL__VG_CHANGE, false, "");
		}
	}
	else if (connection_type == NewGeneWidget::RELEASE_CONNECTIONS_INPUT_PROJECT)
	{
		if (inp)
		{
			//inp->UnregisterInterestInChanges(this);
		}
	}
}

void limit_dmus_region::RefreshAllWidgets()
{

	if (outp == nullptr)
	{
		Empty();
		return;
	}
	WidgetDataItemRequest_LIMIT_DMUS_TAB request(WIDGET_DATA_ITEM_REQUEST_REASON__REFRESH_ALL_WIDGETS);
	emit RefreshWidget(request);

}

void limit_dmus_region::WidgetDataRefreshReceive(WidgetDataItem_LIMIT_DMUS_TAB widget_data)
{

	Empty();

	UIOutputProject * project = projectManagerUI().getActiveUIOutputProject();
	if (project == nullptr)
	{
		return;
	}

	UIMessager messager(project);

	if (!ui->listView_limit_dmus_top_pane)
	{
		boost::format msg("Invalid top pane list view in Limit DMUs widget.");
		QMessageBox msgBox;
		msgBox.setText( msg.str().c_str() );
		msgBox.exec();
		return;
	}

	QStandardItemModel * oldModel = static_cast<QStandardItemModel*>(ui->listView_limit_dmus_top_pane->model());
	if (oldModel != nullptr)
	{
		delete oldModel;
	}

	QItemSelectionModel * oldSelectionModel = ui->listView_limit_dmus_top_pane->selectionModel();
	QStandardItemModel * model = new QStandardItemModel(ui->listView_limit_dmus_top_pane);

	std::vector<dmu_category_limit_members_info_tuple> & dmu_category_limit_members_info = widget_data.dmu_category_limit_members_info;

	int index = 0;
	for (auto & dmu_category_tuple : dmu_category_limit_members_info)
	{
		WidgetInstanceIdentifier const & dmu_category = std::get<0>(dmu_category_tuple);
		bool const is_limited = std::get<1>(dmu_category_tuple);
		WidgetInstanceIdentifiers const & dmu_set_members_all = std::get<2>(dmu_category_tuple);
		WidgetInstanceIdentifiers const & dmu_set_members_not_limited = std::get<3>(dmu_category_tuple);
		WidgetInstanceIdentifiers const & dmu_set_members_limited = std::get<4>(dmu_category_tuple);
		if (dmu_category.code && !dmu_category.code->empty())
		{

			QStandardItem * item = new QStandardItem();
			std::string text = Table_DMU_Identifier::GetDmuCategoryDisplayText(dmu_category);
			item->setText(text.c_str());
			item->setEditable(false);
			item->setCheckable(false);
			QVariant v;
			v.setValue(dmu_category);
			item->setData(v);
			model->setItem( index, item );

			++index;

		}
	};

	model->sort(0);

	ui->listView_limit_dmus_top_pane->setModel(model);
	if (oldSelectionModel) delete oldSelectionModel;

	EmptyDmuMembersPanes();

	connect( ui->listView_limit_dmus_top_pane->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(ReceiveDMUSelectionChanged(const QItemSelection &, const QItemSelection &)));

}

void limit_dmus_region::Empty()
{

	if (!ui->listView_limit_dmus_top_pane || !ui->listView_limit_dmus_bottom_left_pane || !ui->listView_limit_dmus_bottom_right_pane)
	{
		boost::format msg("Invalid list view in Limit DMU's tab.");
		QMessageBox msgBox;
		msgBox.setText( msg.str().c_str() );
		msgBox.exec();
		return;
	}

	QSortFilterProxyModel * oldModel = nullptr;
	QAbstractItemModel * oldSourceModel = nullptr;
	QStandardItemModel * oldDmuModel = nullptr;
	QItemSelectionModel * oldSelectionModel = nullptr;

	EmptyDmuMembersPanes();

	oldDmuModel = static_cast<QStandardItemModel*>(ui->listView_limit_dmus_top_pane->model());
	if (oldDmuModel != nullptr)
	{
		delete oldDmuModel;
		oldDmuModel = nullptr;
	}

	oldSelectionModel = ui->listView_limit_dmus_top_pane->selectionModel();
	if (oldSelectionModel != nullptr)
	{
		delete oldSelectionModel;
		oldSelectionModel = nullptr;
	}

}

void limit_dmus_region::EmptyDmuMembersPanes()
{

	EmptyBottomLeftPane();
	EmptyBottomRightPane();

}

void limit_dmus_region::EmptyBottomLeftPane()
{

	QItemSelectionModel * oldSelectionModel = ui->listView_limit_dmus_bottom_left_pane->selectionModel();
	if (oldSelectionModel != nullptr)
	{
		delete oldSelectionModel;
		oldSelectionModel = nullptr;
	}

	QSortFilterProxyModel_NumbersLast * dmuSetMembersProxyModel = static_cast<QSortFilterProxyModel_NumbersLast*>(ui->listView_limit_dmus_bottom_left_pane->model());
	if (dmuSetMembersProxyModel != nullptr)
	{

		QAbstractItemModel * dmuSetMembersSourceModel = dmuSetMembersProxyModel->sourceModel();
		if (dmuSetMembersSourceModel != nullptr)
		{

			delete dmuSetMembersSourceModel;
			dmuSetMembersSourceModel = nullptr;

		}

		delete dmuSetMembersProxyModel;
		dmuSetMembersProxyModel = nullptr;

	}

}

void limit_dmus_region::EmptyBottomRightPane()
{

	QItemSelectionModel * oldSelectionModel = ui->listView_limit_dmus_bottom_right_pane->selectionModel();
	if (oldSelectionModel != nullptr)
	{
		delete oldSelectionModel;
		oldSelectionModel = nullptr;
	}

	QSortFilterProxyModel_NumbersLast * dmuSetMembersProxyModel = static_cast<QSortFilterProxyModel_NumbersLast*>(ui->listView_limit_dmus_bottom_right_pane->model());
	if (dmuSetMembersProxyModel != nullptr)
	{

		QAbstractItemModel * dmuSetMembersSourceModel = dmuSetMembersProxyModel->sourceModel();
		if (dmuSetMembersSourceModel != nullptr)
		{

			delete dmuSetMembersSourceModel;
			dmuSetMembersSourceModel = nullptr;

		}

		delete dmuSetMembersProxyModel;
		dmuSetMembersProxyModel = nullptr;

	}

}

void limit_dmus_region::ReceiveDMUSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{

	UIOutputProject * project = projectManagerUI().getActiveUIOutputProject();
	if (project == nullptr)
	{
		return;
	}

	UIMessager messager(project);

	if (!ui->listView_limit_dmus_top_pane || !ui->listView_limit_dmus_bottom_left_pane || !ui->listView_limit_dmus_bottom_right_pane)
	{
		boost::format msg("Invalid list view in Limit DMU's tab.");
		QMessageBox msgBox;
		msgBox.setText( msg.str().c_str() );
		msgBox.exec();
		return;
	}

	if(!selected.indexes().isEmpty())
	{

		QStandardItemModel * dmuModel = static_cast<QStandardItemModel*>(ui->listView_limit_dmus_top_pane->model());
		QModelIndex selectedIndex = selected.indexes().first();
		QVariant dmu_category_variant = dmuModel->item(selectedIndex.row())->data();
		WidgetInstanceIdentifier dmu_category = dmu_category_variant.value<WidgetInstanceIdentifier>();

		WidgetInstanceIdentifiers dmu_set_members__all = project->model().backend().getInputModel().t_dmu_setmembers.getIdentifiers(*dmu_category.uuid);
		bool is_limited = project->model().backend().t_limit_dmus_categories.Exists(project->model().backend().getDb(), project->model().backend(), project->model().backend().getInputModel(), *dmu_category.code);
		WidgetInstanceIdentifiers dmu_set_members__limited = project->model().backend().t_limit_dmus_set_members.getIdentifiers(*dmu_category.code);
		std::sort(dmu_set_members__all.begin(), dmu_set_members__all.end());
		std::sort(dmu_set_members__limited.begin(), dmu_set_members__limited.end());

		WidgetInstanceIdentifiers dmu_set_members_not_limited;
		std::set_difference(dmu_set_members__all.cbegin(), dmu_set_members__all.cend(), dmu_set_members__limited.cbegin(), dmu_set_members__limited.cend(), std::inserter(dmu_set_members_not_limited, dmu_set_members_not_limited.begin()));

		ResetDmuMembersPanes(dmu_category, is_limited, dmu_set_members__all, dmu_set_members_not_limited, dmu_set_members__limited);

	}

}

void limit_dmus_region::ResetDmuMembersPanes(WidgetInstanceIdentifier const & dmu_category, bool const is_limited, WidgetInstanceIdentifiers const & dmu_set_members__all, WidgetInstanceIdentifiers const & dmu_set_members_not_limited, WidgetInstanceIdentifiers const & dmu_set_members__limited)
{

	EmptyDmuMembersPanes();

	ResetBottomLeftPane(dmu_set_members_not_limited);
	ResetBottomRightPane(dmu_set_members__limited);

}

void limit_dmus_region::ResetBottomLeftPane(WidgetInstanceIdentifiers const & dmu_set_members__not_limited)
{

	QItemSelectionModel * oldSelectionModel = ui->listView_limit_dmus_bottom_left_pane->selectionModel();
	QStandardItemModel * model = new QStandardItemModel();

	int index = 0;
	for (auto & dmu_member : dmu_set_members__not_limited)
	{
		if (dmu_member.uuid && !dmu_member.uuid->empty())
		{

			std::string text = Table_DMU_Instance::GetDmuMemberDisplayText(dmu_member);

			QStandardItem * item = new QStandardItem();
			item->setText(text.c_str());
			item->setEditable(false);
			item->setCheckable(true);
			QVariant v;
			v.setValue(dmu_member);
			item->setData(v);
			model->setItem( index, item );

			++index;

		}
	}

	QSortFilterProxyModel_NumbersLast *proxyModel = new QSortFilterProxyModel_NumbersLast(ui->listView_limit_dmus_bottom_left_pane);
	proxyModel->setDynamicSortFilter(true);
	proxyModel->setSourceModel(model);
	proxyModel->sort(0);
	ui->listView_limit_dmus_bottom_left_pane->setModel(proxyModel);
	if (oldSelectionModel) delete oldSelectionModel;

}

void limit_dmus_region::ResetBottomRightPane(WidgetInstanceIdentifiers const & dmu_set_members__limited)
{

	QItemSelectionModel * oldSelectionModel = ui->listView_limit_dmus_bottom_right_pane->selectionModel();
	QStandardItemModel * model = new QStandardItemModel();

	int index = 0;
	for (auto & dmu_member : dmu_set_members__limited)
	{
		if (dmu_member.uuid && !dmu_member.uuid->empty())
		{

			std::string text = Table_DMU_Instance::GetDmuMemberDisplayText(dmu_member);

			QStandardItem * item = new QStandardItem();
			item->setText(text.c_str());
			item->setEditable(false);
			item->setCheckable(true);
			QVariant v;
			v.setValue(dmu_member);
			item->setData(v);
			model->setItem( index, item );

			++index;

		}
	}

	QSortFilterProxyModel_NumbersLast *proxyModel = new QSortFilterProxyModel_NumbersLast(ui->listView_limit_dmus_bottom_right_pane);
	proxyModel->setDynamicSortFilter(true);
	proxyModel->setSourceModel(model);
	proxyModel->sort(0);
	ui->listView_limit_dmus_bottom_right_pane->setModel(proxyModel);
	if (oldSelectionModel) delete oldSelectionModel;

}

void limit_dmus_region::HandleChanges(DataChangeMessage const & change_message)
{

	UIInputProject * project = projectManagerUI().getActiveUIInputProject();
	if (project == nullptr)
	{
		return;
	}

	UIMessager messager(project);

	std::for_each(change_message.changes.cbegin(), change_message.changes.cend(), [this](DataChange const & change)
	{
		switch (change.change_type)
		{
			case DATA_CHANGE_TYPE::DATA_CHANGE_TYPE__INPUT_MODEL__VG_CHANGE:
				{
					switch (change.change_intention)
					{

						case DATA_CHANGE_INTENTION__ADD:
							{

//                                if (change.parent_identifier.uuid && change.parent_identifier.code && change.parent_identifier.longhand)
//                                {
//                                    WidgetInstanceIdentifier new_identifier(change.parent_identifier);
//                                    NewGeneVariableSummaryGroup * tmpGrp = new NewGeneVariableSummaryGroup( this, new_identifier, outp );
//                                    tmpGrp->setTitle(new_identifier.longhand->c_str());
//                                    layout()->addWidget(tmpGrp);
//                                }

							}
							break;

						case DATA_CHANGE_INTENTION__REMOVE:
							{

								if (change.parent_identifier.code && change.parent_identifier.uuid)
								{

//                                    WidgetInstanceIdentifier vg_to_remove(change.parent_identifier);

//                                    int current_number = layout()->count();
//                                    bool found = false;
//                                    QWidget * widgetToRemove = nullptr;
//                                    QLayoutItem * layoutItemToRemove = nullptr;
//                                    int i = 0;
//                                    for (i=0; i<current_number; ++i)
//                                    {
//                                        QLayoutItem * testLayoutItem = layout()->itemAt(i);
//                                        QWidget * testWidget(testLayoutItem->widget());
//                                        try
//                                        {
//                                            NewGeneVariableSummaryGroup * testVG = dynamic_cast<NewGeneVariableSummaryGroup*>(testWidget);
//                                            if (testVG->data_instance.IsEqual(WidgetInstanceIdentifier::EQUALITY_CHECK_TYPE__UUID_PLUS_STRING_CODE, vg_to_remove))
//                                            {
//                                                widgetToRemove = testVG;
//                                                layoutItemToRemove = testLayoutItem;
//                                                found = true;
//                                                break;
//                                            }
//                                        }
//                                        catch (std::bad_cast &)
//                                        {
//                                            // guess not
//                                        }

//                                    }

//                                    if (found && widgetToRemove != nullptr)
//                                    {
//                                        layout()->takeAt(i);
//                                        delete widgetToRemove;
//                                        delete layoutItemToRemove;
//                                        widgetToRemove = nullptr;
//                                        layoutItemToRemove = nullptr;
//                                    }

								}

							}
							break;

						case DATA_CHANGE_INTENTION__UPDATE:
							{
								// Should never receive this.
							}
							break;

						case DATA_CHANGE_INTENTION__RESET_ALL:
							{
								// Ditto above.
							}
							break;

						default:
							{
							}
							break;

					}
				}
				break;
			default:
				{
				}
				break;
		}
	});

}
