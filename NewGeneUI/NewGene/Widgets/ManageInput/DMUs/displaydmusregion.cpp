#ifndef Q_MOC_RUN
	#include <boost/algorithm/string.hpp>
	#include <boost/regex.hpp>
#endif
#include "displaydmusregion.h"
#include "ui_displaydmusregion.h"
#include <QStandardItem>
#include <QInputDialog>
#include <QDialog>
#include <QFormLayout>
#include <QList>
#include <QLineEdit>
#include <QString>
#include <QLabel>
#include <QDialogButtonBox>
#include <QModelIndexList>
#include <QSortFilterProxyModel>
#include <QPushButton>
#include <QFileDialog>
#include <QRadioButton>
#include "../Project/uiprojectmanager.h"
#include "../Project/uiinputproject.h"
#include "../../Utilities/qsortfilterproxymodel_numberslast.h"
#include "../../Utilities/dialoghelper.h"
#include "../../../../../NewGeneBackEnd/Utilities/Validation.h"
#include "../../../../NewGeneBackEnd/Utilities/TimeRangeHelper.h"
#include "../../../../NewGeneBackEnd/Model/Tables/Import/Import.h"

DisplayDMUsRegion::DisplayDMUsRegion(QWidget * parent) :
	QWidget(parent),
	NewGeneWidget(WidgetCreationInfo(this, parent, WIDGET_NATURE_INPUT_WIDGET, MANAGE_DMUS_WIDGET,
									 true)),   // 'this' pointer is cast by compiler to proper Widget instance, which is already created due to order in which base classes appear in class definition
	ui(new Ui::DisplayDMUsRegion)
{

	ui->setupUi(this);
	ui->label_importProgress->hide();
	ui->progressBar_importDMU->hide();
	ui->pushButton_cancel->hide();
	ui->splitter_dmus_dmu_members->setStretchFactor(0, 1);
	ui->splitter_dmus_dmu_members->setStretchFactor(1, 3);
	PrepareInputWidget(true);

	refresh_dmu_called_after_create = false;

}

DisplayDMUsRegion::~DisplayDMUsRegion()
{
	delete ui;
}

void DisplayDMUsRegion::changeEvent(QEvent * e)
{
	QWidget::changeEvent(e);

	switch (e->type())
	{
		case QEvent::LanguageChange:
			ui->retranslateUi(this);
			break;

		default:
			break;
	}
}

void DisplayDMUsRegion::UpdateInputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIInputProject * project)
{

	NewGeneWidget::UpdateInputConnections(connection_type, project);

	if (connection_type == NewGeneWidget::ESTABLISH_CONNECTIONS_INPUT_PROJECT)
	{
		connect(this, SIGNAL(RefreshWidget(WidgetDataItemRequest_MANAGE_DMUS_WIDGET)), inp->getConnector(), SLOT(RefreshWidget(WidgetDataItemRequest_MANAGE_DMUS_WIDGET)));
		connect(project->getConnector(), SIGNAL(WidgetDataRefresh(WidgetDataItem_MANAGE_DMUS_WIDGET)), this, SLOT(WidgetDataRefreshReceive(WidgetDataItem_MANAGE_DMUS_WIDGET)));
		connect(this, SIGNAL(AddDMU(WidgetActionItemRequest_ACTION_ADD_DMU)), inp->getConnector(), SLOT(AddDMU(WidgetActionItemRequest_ACTION_ADD_DMU)));
		connect(this, SIGNAL(DeleteDMU(WidgetActionItemRequest_ACTION_DELETE_DMU)), inp->getConnector(), SLOT(DeleteDMU(WidgetActionItemRequest_ACTION_DELETE_DMU)));
		connect(this, SIGNAL(AddDMUMembers(WidgetActionItemRequest_ACTION_ADD_DMU_MEMBERS)), inp->getConnector(), SLOT(AddDMUMembers(WidgetActionItemRequest_ACTION_ADD_DMU_MEMBERS)));
		connect(this, SIGNAL(DeleteDMUMembers(WidgetActionItemRequest_ACTION_DELETE_DMU_MEMBERS)), inp->getConnector(),
				SLOT(DeleteDMUMembers(WidgetActionItemRequest_ACTION_DELETE_DMU_MEMBERS)));
		connect(this, SIGNAL(RefreshDMUsFromFile(WidgetActionItemRequest_ACTION_REFRESH_DMUS_FROM_FILE)), inp->getConnector(),
				SLOT(RefreshDMUsFromFile(WidgetActionItemRequest_ACTION_REFRESH_DMUS_FROM_FILE)));
		connect(project->getConnector(), SIGNAL(SignalUpdateDMUImportProgressBar(int, int, int, int)), this, SLOT(UpdateDMUImportProgressBar(int, int, int, int)));

		if (project)
		{
			project->RegisterInterestInChange(this, DATA_CHANGE_TYPE__INPUT_MODEL__DMU_CHANGE, false, "");
			project->RegisterInterestInChange(this, DATA_CHANGE_TYPE__INPUT_MODEL__DMU_MEMBERS_CHANGE, false, "");
			ManageLowerPaneButtonEnabledStates(LOWER_PANE_BUTTON_ENABLED_STATE::NO_DMU_SELECTED);
		}
	}
	else if (connection_type == NewGeneWidget::RELEASE_CONNECTIONS_INPUT_PROJECT)
	{
		if (inp)
		{
			inp->UnregisterInterestInChanges(this);
		}

		Empty();
	}

}

void DisplayDMUsRegion::UpdateOutputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIOutputProject * project)
{

	NewGeneWidget::UpdateOutputConnections(connection_type, project);

	if (connection_type == NewGeneWidget::ESTABLISH_CONNECTIONS_OUTPUT_PROJECT)
	{
		if (project)
		{
			connect(this, SIGNAL(DeleteDMU(WidgetActionItemRequest_ACTION_DELETE_DMU)), project->getConnector(), SLOT(DeleteDMU(WidgetActionItemRequest_ACTION_DELETE_DMU)));
			connect(this, SIGNAL(DeleteDMUMembers(WidgetActionItemRequest_ACTION_DELETE_DMU_MEMBERS)), project->getConnector(),
					SLOT(DeleteDMUMembers(WidgetActionItemRequest_ACTION_DELETE_DMU_MEMBERS)));
		}
	}
	else if (connection_type == NewGeneWidget::RELEASE_CONNECTIONS_INPUT_PROJECT)
	{
		if (project)
		{
		}
	}

}

void DisplayDMUsRegion::RefreshAllWidgets()
{
	if (inp == nullptr)
	{
		Empty();
		return;
	}

	WidgetDataItemRequest_MANAGE_DMUS_WIDGET request(WIDGET_DATA_ITEM_REQUEST_REASON__REFRESH_ALL_WIDGETS);
	emit RefreshWidget(request);
}

void DisplayDMUsRegion::WidgetDataRefreshReceive(WidgetDataItem_MANAGE_DMUS_WIDGET widget_data)
{

	UIInputProject * project = projectManagerUI().getActiveUIInputProject();

	if (project == nullptr)
	{
		return;
	}

	UIMessager messager(project);

	if (!ui->listView_dmus)
	{
		boost::format msg("Invalid list view in DisplayDMUsRegion widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	QStandardItemModel * oldModel = static_cast<QStandardItemModel *>(ui->listView_dmus->model());

	if (oldModel != nullptr)
	{
		delete oldModel;
	}

	QItemSelectionModel * oldSelectionModel = ui->listView_dmus->selectionModel();
	QStandardItemModel * model = new QStandardItemModel(ui->listView_dmus);

	int index = 0;
	std::for_each(widget_data.dmus_and_members.cbegin(), widget_data.dmus_and_members.cend(), [this, &index,
				  &model](std::pair<WidgetInstanceIdentifier, WidgetInstanceIdentifiers> const & dmu_and_members)
	{
		WidgetInstanceIdentifier const & dmu = dmu_and_members.first;
		WidgetInstanceIdentifiers const & dmu_members = dmu_and_members.second;

		if (dmu.code && !dmu.code->empty())
		{

			QStandardItem * item = new QStandardItem();
			std::string text = Table_DMU_Identifier::GetDmuCategoryDisplayText(dmu);
			item->setText(text.c_str());
			item->setEditable(false);
			item->setCheckable(false);
			QVariant v;
			v.setValue(dmu_and_members);
			item->setData(v);
			model->setItem(index, item);

			++index;

		}
	});

	model->sort(0);

	ui->listView_dmus->setModel(model);

	if (oldSelectionModel) { delete oldSelectionModel; }

	EmptyDmuMembersPane();

	connect(ui->listView_dmus->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(ReceiveDMUSelectionChanged(const QItemSelection &,
			const QItemSelection &)));

}

void DisplayDMUsRegion::Empty()
{

	if (!ui->listView_dmus || !ui->listView_dmu_members)
	{
		boost::format msg("Invalid list view in DisplayDMUsRegion widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	QSortFilterProxyModel * oldModel = nullptr;
	QAbstractItemModel * oldSourceModel = nullptr;
	QStandardItemModel * oldDmuModel = nullptr;
	QItemSelectionModel * oldSelectionModel = nullptr;

	EmptyDmuMembersPane();

	oldDmuModel = static_cast<QStandardItemModel *>(ui->listView_dmus->model());

	if (oldDmuModel != nullptr)
	{
		delete oldDmuModel;
		oldDmuModel = nullptr;
	}

	oldSelectionModel = ui->listView_dmus->selectionModel();

	if (oldSelectionModel != nullptr)
	{
		delete oldSelectionModel;
		oldSelectionModel = nullptr;
	}

	ManageLowerPaneButtonEnabledStates(LOWER_PANE_BUTTON_ENABLED_STATE::NO_PROJECT_OPEN);

}

void DisplayDMUsRegion::ReceiveDMUSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{

	UIInputProject * project = projectManagerUI().getActiveUIInputProject();

	if (project == nullptr)
	{
		return;
	}

	UIMessager messager(project);

	if (!ui->listView_dmus || !ui->listView_dmu_members)
	{
		boost::format msg("Invalid list view in DisplayDMUsRegion widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	if (!selected.indexes().isEmpty())
	{

		QStandardItemModel * dmuModel = static_cast<QStandardItemModel *>(ui->listView_dmus->model());
		QModelIndex selectedIndex = selected.indexes().first();
		QVariant dmu_and_members_variant = dmuModel->item(selectedIndex.row())->data();
		std::pair<WidgetInstanceIdentifier, WidgetInstanceIdentifiers> dmu_and_members = dmu_and_members_variant.value<std::pair<WidgetInstanceIdentifier, WidgetInstanceIdentifiers>>();
		WidgetInstanceIdentifier & dmu_category = dmu_and_members.first;
		WidgetInstanceIdentifiers & dmu_members = dmu_and_members.second;

		ResetDmuMembersPane(dmu_category, dmu_members);

	}
	else
	{
		ManageLowerPaneButtonEnabledStates(LOWER_PANE_BUTTON_ENABLED_STATE::NO_DMU_SELECTED);
	}

}

void DisplayDMUsRegion::on_pushButton_add_dmu_clicked()
{

	// From http://stackoverflow.com/a/17512615/368896
	QDialog dialog(this);
	dialog.setWindowTitle("Create DMU");
	dialog.setWindowFlags(dialog.windowFlags() & ~(Qt::WindowContextHelpButtonHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint));
	QFormLayout form(&dialog);
	QList<QLineEdit *> fields;
	QLineEdit * lineEditName = new QLineEdit(&dialog);
	setLineEditWidth(lineEditName, 12);
	QString labelName = QString("Enter Decision Making Unit (DMU) category name:");
	form.addRow(labelName, lineEditName);
	fields << lineEditName;
	QLineEdit * lineEditDescription = new QLineEdit(&dialog);
	setLineEditWidth(lineEditDescription, 20);
	QString labelDescription = QString("Short description (max 20 characters):");
	form.addRow(labelDescription, lineEditDescription);
	fields << lineEditDescription;

	// Add some standard buttons (Cancel/Ok) at the bottom of the dialog
	QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
	form.addRow(&buttonBox);

	std::string proposed_dmu_name;
	std::string dmu_description;

	QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
	QObject::connect(&buttonBox, &QDialogButtonBox::accepted, [&]()
	{

		proposed_dmu_name.clear();
		dmu_description.clear();

		std::string errorMsg;

		QLineEdit * proposed_dmu_name_field = fields[0];
		QLineEdit * dmu_description_field = fields[1];

		if (proposed_dmu_name_field && dmu_description_field)
		{
			proposed_dmu_name = proposed_dmu_name_field->text().toStdString();
			dmu_description = dmu_description_field->text().toStdString();

			if (proposed_dmu_name.empty())
			{
				boost::format msg("The DMU category must have a name.");
				QMessageBox msgBox;
				msgBox.setText(msg.str().c_str());
				msgBox.exec();
				return;
			}
		}
		else
		{
			boost::format msg("Unable to determine new DMU name and description.");
			QMessageBox msgBox;
			msgBox.setText(msg.str().c_str());
			msgBox.exec();
			return;
		}

		boost::trim(proposed_dmu_name);
		boost::trim(dmu_description);

		if (proposed_dmu_name.empty())
		{
			boost::format msg("The DMU category you entered is empty.");
			QMessageBox msgBox;
			msgBox.setText(msg.str().c_str());
			msgBox.exec();
			return;
		}

		bool valid = true;

		if (valid)
		{
			valid = Validation::ValidateDmuCode(proposed_dmu_name, errorMsg);
		}

		if (valid)
		{
			valid = Validation::ValidateDmuDescription(dmu_description, errorMsg);
		}

		if (!valid)
		{
			boost::format msg("%1%");
			msg % errorMsg;
			QMessageBox msgBox;
			msgBox.setText(msg.str().c_str());
			msgBox.exec();
			return;
		}

		if (valid)
		{
			dialog.accept();
		}

	});

	if (dialog.exec() != QDialog::Accepted)
	{
		return;
	}

	InstanceActionItems actionItems;
	actionItems.push_back(std::make_pair(WidgetInstanceIdentifier(),
										 std::shared_ptr<WidgetActionItem>(static_cast<WidgetActionItem *>(new WidgetActionItem__StringVector(std::vector<std::string> {proposed_dmu_name, dmu_description})))));
	WidgetActionItemRequest_ACTION_ADD_DMU action_request(WIDGET_ACTION_ITEM_REQUEST_REASON__ADD_ITEMS, actionItems);

	emit AddDMU(action_request);

}

void DisplayDMUsRegion::on_pushButton_delete_dmu_clicked()
{

	UIInputProject * project = projectManagerUI().getActiveUIInputProject();

	if (project == nullptr)
	{
		return;
	}

	UIMessager messager(project);

	if (!ui->listView_dmus || !ui->listView_dmu_members)
	{
		boost::format msg("Invalid list view in DisplayDMUsRegion widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	WidgetInstanceIdentifier dmu;
	WidgetInstanceIdentifiers dmu_members;
	bool is_selected = GetSelectedDmuCategory(dmu, dmu_members);

	if (!is_selected)
	{
		return;
	}

	QStandardItemModel * dmuModel = static_cast<QStandardItemModel *>(ui->listView_dmus->model());

	if (dmuModel == nullptr)
	{
		boost::format msg("Invalid model in DisplayDMUsRegion DMU category widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	if (!dmu.code || dmu.code->empty())
	{
		boost::format msg("Invalid DMU being deleted.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	QMessageBox::StandardButton reply;
	boost::format
	msg("Are you certain you wish to delete the DMU \"%1%\"?  This will permanently delete all associated units of analysis and variable groups.  Proceed with deletion?");
	msg % *dmu.code;
	boost::format msgTitle("Delete DMU \"%1%\"?");
	msgTitle % *dmu.code;
	reply = QMessageBox::question(nullptr, QString(msgTitle.str().c_str()), QString(msg.str().c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));

	if (reply == QMessageBox::No)
	{
		return;
	}

	InstanceActionItems actionItems;
	actionItems.push_back(std::make_pair(WidgetInstanceIdentifier(dmu),
										 std::shared_ptr<WidgetActionItem>(static_cast<WidgetActionItem *>(new WidgetActionItem__WidgetInstanceIdentifier(dmu)))));
	WidgetActionItemRequest_ACTION_DELETE_DMU action_request(WIDGET_ACTION_ITEM_REQUEST_REASON__REMOVE_ITEMS, actionItems);
	emit DeleteDMU(action_request);

}

void DisplayDMUsRegion::on_pushButton_refresh_dmu_members_from_file_clicked()
{

	if (!refresh_dmu_called_after_create)
	{
		// Refreshing data is VASTLY slower than simply inserting new data;
		// give warning
		refresh_dmu_called_after_create = false;
		boost::format msg_title("Refreshing data can be slow");
		boost::format
		msg_text("Warning: If there is a large amount of existing data AND a large amount of data being refreshed, it will be MUCH, MUCH faster to delete the existing DMU, and re-import from scratch.  However, this will also delete all units of analysis and variable groups associated with this DMU - so it might be worth the wait.  Continue?");
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(nullptr, QString(msg_title.str().c_str()), QString(msg_text.str().c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));

		if (reply == QMessageBox::No)
		{
			return;
		}
	}

	bool do_refresh_not_plain_insert = !refresh_dmu_called_after_create;
	refresh_dmu_called_after_create = false;

	WidgetInstanceIdentifier dmu_category;
	WidgetInstanceIdentifiers dmu_members;
	bool found = GetSelectedDmuCategory(dmu_category, dmu_members);

	if (!found)
	{
		boost::format msg("A DMU category must be selected.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	QDialog dialog(this);
	dialog.setWindowTitle("DMU refresh");
	dialog.setWindowFlags(dialog.windowFlags() & ~(Qt::WindowContextHelpButtonHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint));
	QFormLayout form(&dialog);
	QWidget FileChooserWidget;
	QBoxLayout formFileSelection(QBoxLayout::LeftToRight);

	QList<QLineEdit *> fields;

	//form.addRow(new QLabel("DMU member refresh details"));

	QString labelColumnNameUuid = QString("Enter 'id' column heading (uniquely identifies each DMU member):");
	QLineEdit * lineEditColumnNameUuid = new QLineEdit(&dialog);
	setLineEditWidth(lineEditColumnNameUuid, 20);
	form.addRow(labelColumnNameUuid, lineEditColumnNameUuid);
	fields << lineEditColumnNameUuid;

	QString labelColumnNameCode = QString("Enter 'abbreviation' column heading (if present):");
	QLineEdit * lineEditColumnNameCode = new QLineEdit(&dialog);
	setLineEditWidth(lineEditColumnNameCode, 20);
	form.addRow(labelColumnNameCode, lineEditColumnNameCode);
	fields << lineEditColumnNameCode;

	QString labelColumnNameDescription = QString("Enter 'description' column heading (if present):");
	QLineEdit * lineEditColumnNameDescription = new QLineEdit(&dialog);
	setLineEditWidth(lineEditColumnNameDescription, 20);
	form.addRow(labelColumnNameDescription, lineEditColumnNameDescription);
	fields << lineEditColumnNameDescription;

	QList<QLineEdit *> fieldsFileChooser;
	std::vector<std::string> const & fileChooserStrings { "Choose comma-delimited file", "Choose DMU comma-delimited data file location", "", "" };
	DialogHelper::AddFileChooserBlock(dialog, form, formFileSelection, FileChooserWidget, fieldsFileChooser, fileChooserStrings);

	// Add some standard buttons (Cancel/Ok) at the bottom of the dialog
	QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
	form.addRow(&buttonBox);

	std::string data_column_name_uuid;
	std::string data_column_name_code;
	std::string data_column_name_description;

	std::vector<std::string> dataFileChooser;
	std::vector<std::string> dataTimeRange;

	QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
	QObject::connect(&buttonBox, &QDialogButtonBox::accepted, [&]()
	{

		data_column_name_uuid.clear();
		data_column_name_code.clear();
		data_column_name_description.clear();

		dataFileChooser.clear();
		dataTimeRange.clear();

		// data validation here
		bool valid = true;

		std::string errorMsg;

		QLineEdit * data_column_name_uuid_field = fields[0];
		QLineEdit * data_column_name_code_field = fields[1];
		QLineEdit * data_column_name_description_field = fields[2];

		// Custom validation
		if (valid)
		{

			if (!data_column_name_uuid_field && !data_column_name_code_field && !data_column_name_description_field)
			{
				valid = false;
				errorMsg = "Invalid DMU.";
			}

		}

		if (valid)
		{

			data_column_name_uuid = data_column_name_uuid_field->text().toStdString();
			data_column_name_code = data_column_name_code_field->text().toStdString();
			data_column_name_description = data_column_name_description_field->text().toStdString();

		}

		if (valid)
		{
			valid = Validation::ValidateColumnName(data_column_name_uuid, "Code", true, errorMsg);
		}

		if (valid)
		{
			valid = Validation::ValidateColumnName(data_column_name_code, "Abbreviation", false, errorMsg);
		}

		if (valid)
		{
			valid = Validation::ValidateColumnName(data_column_name_description, "Description", false, errorMsg);
		}

		// Factored validation
		if (valid)
		{
			valid = DialogHelper::ValidateFileChooserBlock(fieldsFileChooser, dataFileChooser, errorMsg);
		}

		//if (valid)
		//{
		//	valid = DialogHelper::ValidateTimeRangeBlock(fieldsTimeRange, radioButtonsTimeRange, dataTimeRange, timeRangeMode, errorMsg);
		//}

		if (!valid)
		{
			boost::format msg(errorMsg);
			QMessageBox msgBox;
			msgBox.setText(msg.str().c_str());
			msgBox.exec();
		}

		if (valid)
		{
			dialog.accept();
		}

	});

	if (dialog.exec() != QDialog::Accepted)
	{
		return;
	}

	// validation has already taken place
	boost::filesystem::path data_column_file_pathname(dataFileChooser[0]);

	InstanceActionItems actionItems;
	std::vector<std::string> column_names{data_column_file_pathname.string(), data_column_name_uuid, data_column_name_code, data_column_name_description};
	//column_names.insert(column_names.end(), dataTimeRange.begin(), dataTimeRange.end());
	actionItems.push_back(std::make_pair(dmu_category, std::shared_ptr<WidgetActionItem>(static_cast<WidgetActionItem *>(new WidgetActionItem__StringVector_Plus_Int(column_names,
										 do_refresh_not_plain_insert ? 1 : 0)))));
	WidgetActionItemRequest_ACTION_REFRESH_DMUS_FROM_FILE action_request(WIDGET_ACTION_ITEM_REQUEST_REASON__DO_ACTION, actionItems);
	emit RefreshDMUsFromFile(action_request);

}

void DisplayDMUsRegion::on_pushButton_add_dmu_member_by_hand_clicked()
{

	// Get selected DMU category
	WidgetInstanceIdentifier dmu_category;
	WidgetInstanceIdentifiers dmu_members;
	bool is_selected = GetSelectedDmuCategory(dmu_category, dmu_members);

	if (!is_selected)
	{
		return;
	}

	// From http://stackoverflow.com/a/17512615/368896
	QDialog dialog(this);
	dialog.setWindowTitle("Add DMU member");
	dialog.setWindowFlags(dialog.windowFlags() & ~(Qt::WindowContextHelpButtonHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint));
	QFormLayout form(&dialog);
	boost::format title("Add DMU member to %1%");
	title % *dmu_category.code;
	form.addRow(new QLabel(title.str().c_str()));
	QList<QLineEdit *> fields;
	QLineEdit * lineEditCode = new QLineEdit(&dialog);
	setLineEditWidth(lineEditCode, 12);
	QString labelCode = QString("Enter uniquely identifying DMU member code:");
	form.addRow(labelCode, lineEditCode);
	fields << lineEditCode;
	QLineEdit * lineEditName = new QLineEdit(&dialog);
	setLineEditWidth(lineEditName, 20);
	QString labelName = QString("(Optional) Enter a short abbreviation:");
	form.addRow(labelName, lineEditName);
	fields << lineEditName;
	QLineEdit * lineEditDescription = new QLineEdit(&dialog);
	setLineEditWidth(lineEditDescription);
	QString labelDescription = QString("(Optional) Enter full descriptive text:");
	form.addRow(labelDescription, lineEditDescription);
	fields << lineEditDescription;

	// Add some standard buttons (Cancel/Ok) at the bottom of the dialog
	QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
	form.addRow(&buttonBox);

	std::string proposed_dmu_member_uuid;
	std::string proposed_dmu_member_code;
	std::string proposed_dmu_member_description;

	QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
	QObject::connect(&buttonBox, &QDialogButtonBox::accepted, [&]()
	{

		proposed_dmu_member_uuid.clear();
		proposed_dmu_member_code.clear();
		proposed_dmu_member_description.clear();

		bool valid = true;
		std::string errorMsg;

		QLineEdit * proposed_dmu_member_uuid_field = fields[0];
		QLineEdit * proposed_dmu_member_code_field = fields[1];
		QLineEdit * proposed_dmu_member_description_field = fields[2];

		if (!proposed_dmu_member_uuid_field || !proposed_dmu_member_code_field || !proposed_dmu_member_description_field)
		{

			boost::format msg("Unable to determine new DMU member code.");
			errorMsg = msg.str();
			valid = false;

		}

		if (valid)
		{

			proposed_dmu_member_uuid = proposed_dmu_member_uuid_field->text().toStdString();
			proposed_dmu_member_code = proposed_dmu_member_code_field->text().toStdString();
			proposed_dmu_member_description = proposed_dmu_member_description_field->text().toStdString();

		}

		if (valid)
		{
			valid = Validation::ValidateDmuMemberUUID(proposed_dmu_member_uuid, false, errorMsg);
		}

		if (valid)
		{
			valid = Validation::ValidateDmuMemberCode(proposed_dmu_member_code, errorMsg);
		}

		if (valid)
		{
			valid = Validation::ValidateDmuMemberDescription(proposed_dmu_member_description, errorMsg);
		}

		if (!valid)
		{
			QMessageBox msgBox;
			msgBox.setText(errorMsg.c_str());
			msgBox.exec();
			return;
		}

		dialog.accept();

	});

	if (dialog.exec() != QDialog::Accepted)
	{
		return;
	}

	InstanceActionItems actionItems;
	actionItems.push_back(std::make_pair(dmu_category, std::shared_ptr<WidgetActionItem>(static_cast<WidgetActionItem *>(new WidgetActionItem__StringVector(std::vector<std::string> {proposed_dmu_member_uuid, proposed_dmu_member_code, proposed_dmu_member_description})))));
	WidgetActionItemRequest_ACTION_ADD_DMU_MEMBERS action_request(WIDGET_ACTION_ITEM_REQUEST_REASON__ADD_ITEMS, actionItems);

	emit AddDMUMembers(action_request);

}

void DisplayDMUsRegion::on_pushButton_delete_selected_dmu_members_clicked()
{

	UIInputProject * project = projectManagerUI().getActiveUIInputProject();

	if (project == nullptr)
	{
		return;
	}

	UIMessager messager(project);

	if (!ui->listView_dmus || !ui->listView_dmu_members)
	{
		boost::format msg("Invalid list view in DisplayDMUsRegion widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	QSortFilterProxyModel_NumbersLast * dmuMembersModel = static_cast<QSortFilterProxyModel_NumbersLast *>(ui->listView_dmu_members->model());

	if (dmuMembersModel == nullptr)
	{
		boost::format msg("Invalid model in DisplayDMUsRegion DMU category widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	QStandardItemModel * model = nullptr;
	bool bad = false;

	try
	{
		model = dynamic_cast<QStandardItemModel *>(dmuMembersModel->sourceModel());
	}
	catch (std::bad_cast &)
	{
		bad = true;
	}

	if (model == nullptr)
	{
		bad = true;
	}

	if (bad)
	{
		boost::format msg("Unable to obtain model for DMU member list.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	QMessageBox::StandardButton reply;
	boost::format
	msg("Are you certain you wish to delete the selected DMU members?  This will permanently delete all data rows tied to the given DMU members.  Proceed with deletion?");
	boost::format msgTitle("Delete DMU members?");
	reply = QMessageBox::question(nullptr, QString(msgTitle.str().c_str()), QString(msg.str().c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));

	if (reply == QMessageBox::No)
	{
		return;
	}

	InstanceActionItems actionItems;

	for (int i = 0; i < dmuMembersModel->rowCount(); ++i)
	{

		QModelIndex testIndex = dmuMembersModel->index(i, 0);
		QStandardItem * testItem = model->item(testIndex.row());

		if (testItem->checkState() == Qt::Checked)
		{
			WidgetInstanceIdentifier dmu_member = testItem->data().value<WidgetInstanceIdentifier>();
			actionItems.push_back(std::make_pair(WidgetInstanceIdentifier(dmu_member),
												 std::shared_ptr<WidgetActionItem>(static_cast<WidgetActionItem *>(new WidgetActionItem__WidgetInstanceIdentifier(dmu_member)))));
		}

	}

	WidgetActionItemRequest_ACTION_DELETE_DMU_MEMBERS action_request(WIDGET_ACTION_ITEM_REQUEST_REASON__REMOVE_ITEMS, actionItems);
	emit DeleteDMUMembers(action_request);

}

void DisplayDMUsRegion::on_pushButton_deselect_all_dmu_members_clicked()
{
	if (!ui->listView_dmu_members)
	{
		boost::format msg("Invalid list view in DisplayDMUsRegion widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	QSortFilterProxyModel_NumbersLast * model = static_cast<QSortFilterProxyModel_NumbersLast *>(ui->listView_dmu_members->model());

	if (model == nullptr)
	{
		return;
	}

	QAbstractItemModel * sourceModel = model->sourceModel();

	if (sourceModel == nullptr)
	{
		return;
	}

	QStandardItemModel * sourceStandardModel = nullptr;

	try
	{
		sourceStandardModel = dynamic_cast<QStandardItemModel *>(sourceModel);
	}
	catch (std::bad_cast &)
	{
		// guess not
		return;
	}

	if (sourceStandardModel == nullptr)
	{
		// guess not
		return;
	}

	int nrows = sourceStandardModel->rowCount();

	for (int row = 0; row < nrows; ++row)
	{
		QStandardItem * item = sourceStandardModel->item(row, 0);

		if (item)
		{
			item->setCheckState(Qt::Unchecked);
		}
	}
}

void DisplayDMUsRegion::on_pushButton_select_all_dmu_members_clicked()
{
	if (!ui->listView_dmu_members)
	{
		boost::format msg("Invalid list view in DisplayDMUsRegion widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	QSortFilterProxyModel_NumbersLast * model = static_cast<QSortFilterProxyModel_NumbersLast *>(ui->listView_dmu_members->model());

	if (model == nullptr)
	{
		return;
	}

	QAbstractItemModel * sourceModel = model->sourceModel();

	if (sourceModel == nullptr)
	{
		return;
	}

	QStandardItemModel * sourceStandardModel = nullptr;

	try
	{
		sourceStandardModel = dynamic_cast<QStandardItemModel *>(sourceModel);
	}
	catch (std::bad_cast &)
	{
		// guess not
		return;
	}

	if (sourceStandardModel == nullptr)
	{
		// guess not
		return;
	}

	int nrows = sourceStandardModel->rowCount();

	for (int row = 0; row < nrows; ++row)
	{
		QStandardItem * item = sourceStandardModel->item(row, 0);

		if (item)
		{
			item->setCheckState(Qt::Checked);
		}
	}
}

void DisplayDMUsRegion::HandleChanges(DataChangeMessage const & change_message)
{

	UIInputProject * project = projectManagerUI().getActiveUIInputProject();

	if (project == nullptr)
	{
		return;
	}

	UIMessager messager(project);

	if (!ui->listView_dmus)
	{
		boost::format msg("Invalid list view in DisplayDMUsRegion widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	QStandardItemModel * itemModel = static_cast<QStandardItemModel *>(ui->listView_dmus->model());

	if (itemModel == nullptr)
	{
		boost::format msg("Invalid list view items in DisplayDMUsRegion widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	QSortFilterProxyModel_NumbersLast * proxyModel = nullptr;

	std::for_each(change_message.changes.cbegin(), change_message.changes.cend(), [this, &itemModel, &proxyModel](DataChange const & change)
	{

		switch (change.change_type)
		{

			case DATA_CHANGE_TYPE::DATA_CHANGE_TYPE__INPUT_MODEL__DMU_CHANGE:
				{

					switch (change.change_intention)
					{

						case DATA_CHANGE_INTENTION__ADD:
							{

								if (!change.parent_identifier.uuid || (*change.parent_identifier.uuid).empty() || !change.parent_identifier.code || (*change.parent_identifier.code).empty())
								{
									boost::format msg("Invalid new DMU name.");
									QMessageBox msgBox;
									msgBox.setText(msg.str().c_str());
									msgBox.exec();
									return;
								}

								WidgetInstanceIdentifier const & dmu_category = change.parent_identifier;

								std::string text = Table_DMU_Identifier::GetDmuCategoryDisplayText(dmu_category);

								QStandardItem * item = new QStandardItem();
								item->setText(text.c_str());
								item->setEditable(false);
								item->setCheckable(false);

								std::pair<WidgetInstanceIdentifier, WidgetInstanceIdentifiers> dmu_and_members = std::make_pair(change.parent_identifier, change.child_identifiers);
								QVariant v;
								v.setValue(dmu_and_members);
								item->setData(v);
								itemModel->appendRow(item);

								QItemSelectionModel * selectionModel = ui->listView_dmus->selectionModel();

								if (selectionModel != nullptr)
								{
									QModelIndex itemIndex = itemModel->indexFromItem(item);
									ui->listView_dmus->setCurrentIndex(itemIndex);
								}

								QEvent * event = new QEvent(QEVENT_PROMPT_FOR_DMU_REFRESH);
								QApplication::postEvent(this, event);

							}
							break;

						case DATA_CHANGE_INTENTION__REMOVE:
							{

								if (!change.parent_identifier.code || (*change.parent_identifier.code).empty() || !change.parent_identifier.longhand)
								{
									boost::format msg("Invalid DMU name or description.");
									QMessageBox msgBox;
									msgBox.setText(msg.str().c_str());
									msgBox.exec();
									return;
								}

								std::string text = Table_DMU_Identifier::GetDmuCategoryDisplayText(change.parent_identifier);
								QList<QStandardItem *> items = itemModel->findItems(text.c_str());

								if (items.count() == 1)
								{
									QStandardItem * dmu_to_remove = items.at(0);

									if (dmu_to_remove != nullptr)
									{
										QModelIndex index_to_remove = itemModel->indexFromItem(dmu_to_remove);
										itemModel->takeRow(index_to_remove.row());

										delete dmu_to_remove;
										dmu_to_remove = nullptr;

										QItemSelectionModel * selectionModel = ui->listView_dmus->selectionModel();

										if (selectionModel != nullptr)
										{

											selectionModel->clearSelection();
											EmptyDmuMembersPane();

										}

									}
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

			case DATA_CHANGE_TYPE::DATA_CHANGE_TYPE__INPUT_MODEL__DMU_MEMBERS_CHANGE:
				{

					QSortFilterProxyModel_NumbersLast * memberModel = static_cast<QSortFilterProxyModel_NumbersLast *>(ui->listView_dmu_members->model());

					if (memberModel == nullptr)
					{
						boost::format msg("Invalid members list view items in DisplayDMUsRegion widget.");
						QMessageBox msgBox;
						msgBox.setText(msg.str().c_str());
						msgBox.exec();
						return;
					}

					/*
					 * This is being handled server-side to deal with UI glitches
					 * when attempting to manage adding/removing individual DMU members
					 * here - namely, we are currently not properly adding/removing the DMU's
					 * from the upper pane's data structures, as well as the lower pane's.
					 * Fixing this will wait for a later version of NewGene.
					 * For now, just force a full refresh of the entire DMU category,
					 * because the back end will send DATA_CHANGE_INTENTION__RESET_ALL,
					 * rather than sending DATA_CHANGE_INTENTION__ADD or DATA_CHANGE_INTENTION__REMOVE.
					*/
					if (false)
					{
						RefreshAllWidgets();
						break;
					}

					switch (change.change_intention)
					{

						case DATA_CHANGE_INTENTION__ADD:
							{

								if (!change.parent_identifier.uuid || (*change.parent_identifier.uuid).empty())
								{
									boost::format msg("Invalid new DMU member.");
									QMessageBox msgBox;
									msgBox.setText(msg.str().c_str());
									msgBox.exec();
									return;
								}

								WidgetInstanceIdentifier new_dmu_member(change.parent_identifier);

								QAbstractItemModel * memberSourceModel = memberModel->sourceModel();

								if (memberSourceModel)
								{

									QStandardItemModel * memberStandardSourceModel = nullptr;

									try
									{
										memberStandardSourceModel = dynamic_cast<QStandardItemModel *>(memberSourceModel);
									}
									catch (std::bad_cast &)
									{
										// guess not
										return;
									}

									if (memberStandardSourceModel == nullptr)
									{
										// guess not
										return;
									}

									QStandardItem * item = new QStandardItem();
									std::string text = Table_DMU_Instance::GetDmuMemberDisplayText(new_dmu_member);

									item->setText(text.c_str());
									item->setEditable(false);
									item->setCheckable(true);
									QVariant v;
									v.setValue(new_dmu_member);
									item->setData(v);

									memberStandardSourceModel->appendRow(item);

								}

							}
							break;

						case DATA_CHANGE_INTENTION__REMOVE:
							{

								if (!change.parent_identifier.uuid || (*change.parent_identifier.uuid).empty())
								{
									boost::format msg("Invalid DMU member.");
									QMessageBox msgBox;
									msgBox.setText(msg.str().c_str());
									msgBox.exec();
									return;
								}

								WidgetInstanceIdentifier const & dmu_member = change.parent_identifier;

								QAbstractItemModel * memberSourceModel = memberModel->sourceModel();

								if (memberSourceModel)
								{

									QStandardItemModel * memberStandardSourceModel = nullptr;

									try
									{
										memberStandardSourceModel = dynamic_cast<QStandardItemModel *>(memberSourceModel);
									}
									catch (std::bad_cast &)
									{
										// guess not
										return;
									}

									if (memberStandardSourceModel == nullptr)
									{
										// guess not
										return;
									}

									std::string text = Table_DMU_Instance::GetDmuMemberDisplayText(dmu_member);
									QList<QStandardItem *> items = memberStandardSourceModel->findItems(text.c_str());

									if (items.count() == 1)
									{
										QStandardItem * dmu_member_to_remove = items.at(0);

										if (dmu_member_to_remove != nullptr)
										{
											QModelIndex index_to_remove = memberStandardSourceModel->indexFromItem(dmu_member_to_remove);
											memberStandardSourceModel->takeRow(index_to_remove.row());

											delete dmu_member_to_remove;
											dmu_member_to_remove = nullptr;
										}
									}

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

								if (!change.parent_identifier.uuid || (*change.parent_identifier.uuid).empty())
								{
									boost::format msg("Invalid DMU member.");
									QMessageBox msgBox;
									msgBox.setText(msg.str().c_str());
									msgBox.exec();
									return;
								}

								WidgetInstanceIdentifier const & dmu_category = change.parent_identifier;
								WidgetInstanceIdentifiers const & dmu_members = change.child_identifiers;

								std::string text = Table_DMU_Identifier::GetDmuCategoryDisplayText(dmu_category);
								QList<QStandardItem *> theItems = itemModel->findItems(text.c_str());

								if (theItems.size() != 1)
								{
									boost::format msg("Cannot find DMU in the list.");
									QMessageBox msgBox;
									msgBox.setText(msg.str().c_str());
									msgBox.exec();
									return;
								}

								QStandardItem * theItem = theItems[0];
								QVariant v;
								v.setValue(std::make_pair(dmu_category, dmu_members));
								theItem->setData(v);

								ResetDmuMembersPane(dmu_category, dmu_members);

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

	if (proxyModel != nullptr)
	{
		proxyModel->sort(0);
	}

}

bool DisplayDMUsRegion::GetSelectedDmuCategory(WidgetInstanceIdentifier & dmu_category, WidgetInstanceIdentifiers & dmu_members)
{

	QItemSelectionModel * dmu_selectionModel = ui->listView_dmus->selectionModel();

	if (dmu_selectionModel == nullptr)
	{
		boost::format msg("Invalid selection in DisplayDMUsRegion widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return false;
	}

	QModelIndexList selectedIndexes = dmu_selectionModel->selectedIndexes();

	if (selectedIndexes.empty())
	{
		// No selection
		return false;
	}

	if (selectedIndexes.size() > 1)
	{
		boost::format msg("Simultaneous selections not allowed.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return false;
	}

	QModelIndex selectedIndex = selectedIndexes[0];

	if (!selectedIndex.isValid())
	{
		// No selection
		return false;
	}

	QStandardItemModel * dmuModel = static_cast<QStandardItemModel *>(ui->listView_dmus->model());

	if (dmuModel == nullptr)
	{
		boost::format msg("Invalid model in DisplayDMUsRegion DMU category widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return false;
	}

	QVariant dmu_and_members_variant = dmuModel->item(selectedIndex.row())->data();
	std::pair<WidgetInstanceIdentifier, WidgetInstanceIdentifiers> dmu_and_members = dmu_and_members_variant.value<std::pair<WidgetInstanceIdentifier, WidgetInstanceIdentifiers>>();
	dmu_category = dmu_and_members.first;
	dmu_members = dmu_and_members.second;

	return true;

}

void DisplayDMUsRegion::ResetDmuMembersPane(WidgetInstanceIdentifier const & dmu_category, WidgetInstanceIdentifiers const & dmu_members)
{

	EmptyDmuMembersPane();
	ManageLowerPaneButtonEnabledStates(LOWER_PANE_BUTTON_ENABLED_STATE::DMU_IS_SELECTED);

	QItemSelectionModel * oldSelectionModel = ui->listView_dmu_members->selectionModel();
	QStandardItemModel * model = new QStandardItemModel();

	int index = 0;
	std::for_each(dmu_members.cbegin(), dmu_members.cend(), [this, &index, &model](WidgetInstanceIdentifier const & dmu_member)
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
			model->setItem(index, item);

			++index;

		}
	});

	QSortFilterProxyModel_NumbersLast * proxyModel = new QSortFilterProxyModel_NumbersLast(ui->listView_dmu_members);
	proxyModel->setDynamicSortFilter(true);
	proxyModel->setSourceModel(model);
	proxyModel->sort(0);
	ui->listView_dmu_members->setModel(proxyModel);

	if (oldSelectionModel) { delete oldSelectionModel; }

	connect(model, SIGNAL(itemChanged(QStandardItem *)), this, SLOT(ReceiveBottomPaneSelectionCheckChanged(QStandardItem *)));

}

bool DisplayDMUsRegion::event(QEvent * e)
{

	bool returnVal = false;

	if (e->type() == QEVENT_PROMPT_FOR_DMU_REFRESH)
	{
		WidgetInstanceIdentifier dmu_category;
		WidgetInstanceIdentifiers dmu_members;
		bool is_selected = GetSelectedDmuCategory(dmu_category, dmu_members);

		if (!is_selected)
		{
			return true; // Even though no DMU is selected, we have recognized and processed our own custom event
		}

		QMessageBox::StandardButton reply;
		boost::format msg("DMU '%1%' successfully created.\n\nWould you like to import members for the new DMU \"%1%\" now?");
		msg % *dmu_category.code;
		boost::format msgTitle("Import data?");
		reply = QMessageBox::question(nullptr, QString(msgTitle.str().c_str()), QString(msg.str().c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));

		if (reply == QMessageBox::Yes)
		{
			QEvent * event = new QEvent(QEVENT_CLICK_DMU_REFRESH);
			QApplication::postEvent(this, event);
		}

		returnVal = true;
	}
	else if (e->type() == QEVENT_CLICK_DMU_REFRESH)
	{
		refresh_dmu_called_after_create = true;
		ui->pushButton_refresh_dmu_members_from_file->click();
		returnVal = true;
	}
	else
	{
		returnVal = QWidget::event(e);
	}

	return returnVal;

}

void DisplayDMUsRegion::UpdateDMUImportProgressBar(int mode_, int min_, int max_, int val_)
{

	PROGRESS_UPDATE_MODE mode = (PROGRESS_UPDATE_MODE)(mode_);

	switch (mode)
	{

		case PROGRESS_UPDATE_MODE__SHOW:
			{
				ui->label_importProgress->show();
				ui->progressBar_importDMU->setTextVisible(true);
				ui->progressBar_importDMU->show();
				ui->pushButton_cancel->show();
			}
			break;

		case PROGRESS_UPDATE_MODE__SET_LIMITS:
			{
				ui->progressBar_importDMU->setRange(min_, max_);
				ui->progressBar_importDMU->setValue(min_);
			}
			break;

		case PROGRESS_UPDATE_MODE__SET_VALUE:
			{
				ui->progressBar_importDMU->setValue(val_);
			}
			break;

		case PROGRESS_UPDATE_MODE__HIDE:
			{
				ui->label_importProgress->hide();
				ui->progressBar_importDMU->hide();
				ui->pushButton_cancel->hide();
			}
			break;

		default:
			{
				// no-op
			}
			break;

	}
}

void DisplayDMUsRegion::EmptyDmuMembersPane()
{

	QItemSelectionModel * oldSelectionModel = ui->listView_dmu_members->selectionModel();

	if (oldSelectionModel != nullptr)
	{
		delete oldSelectionModel;
		oldSelectionModel = nullptr;
	}

	QSortFilterProxyModel_NumbersLast * dmuSetMembersProxyModel = static_cast<QSortFilterProxyModel_NumbersLast *>(ui->listView_dmu_members->model());

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

	ManageLowerPaneButtonEnabledStates(LOWER_PANE_BUTTON_ENABLED_STATE::NO_DMU_SELECTED);

}

void DisplayDMUsRegion::on_pushButton_cancel_clicked()
{
	{
		std::lock_guard<std::recursive_mutex> guard(Importer::is_performing_import_mutex);

		if (Importer::is_performing_import)
		{
			QMessageBox::StandardButton reply;
			reply = QMessageBox::question(nullptr, QString("Cancel?"), QString("Are you sure you wish to cancel?"), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));

			if (reply == QMessageBox::Yes)
			{
				// No lock - not necessary for a boolean whose read/write is guaranteed to be in proper sequence
				Importer::cancelled = true;
			}
		}
	}
}

void DisplayDMUsRegion::ReceiveBottomPaneSelectionCheckChanged(QStandardItem *)
{

	UIInputProject * project = projectManagerUI().getActiveUIInputProject();

	if (project == nullptr)
	{
		return;
	}

	UIMessager messager(project);

	if (!ui->listView_dmus || !ui->listView_dmu_members)
	{
		boost::format msg("Invalid list view in DisplayDMUsRegion widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	QSortFilterProxyModel_NumbersLast * dmuMembersModel = static_cast<QSortFilterProxyModel_NumbersLast *>(ui->listView_dmu_members->model());

	if (dmuMembersModel == nullptr)
	{
		boost::format msg("Invalid model in DisplayDMUsRegion DMU category widget.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	QStandardItemModel * model = nullptr;
	bool bad = false;

	try
	{
		model = dynamic_cast<QStandardItemModel *>(dmuMembersModel->sourceModel());
	}
	catch (std::bad_cast &)
	{
		bad = true;
	}

	if (model == nullptr)
	{
		bad = true;
	}

	if (bad)
	{
		boost::format msg("Unable to obtain model for DMU member list.");
		QMessageBox msgBox;
		msgBox.setText(msg.str().c_str());
		msgBox.exec();
		return;
	}

	int numberChecked {};

	for (int i = 0; i < dmuMembersModel->rowCount(); ++i)
	{

		QModelIndex testIndex = dmuMembersModel->index(i, 0);
		QStandardItem * testItem = model->item(testIndex.row());

		if (testItem->checkState() == Qt::Checked)
		{
			++numberChecked;
		}

	}

	ui->pushButton_delete_selected_dmu_members->setEnabled(numberChecked > 0);
	ui->pushButton_deselect_all_dmu_members->setEnabled(numberChecked > 0);
	ui->pushButton_select_all_dmu_members->setEnabled(numberChecked != dmuMembersModel->rowCount());

}

void DisplayDMUsRegion::ManageLowerPaneButtonEnabledStates(LOWER_PANE_BUTTON_ENABLED_STATE const state)
{
	switch (state)
	{
		case LOWER_PANE_BUTTON_ENABLED_STATE::NO_DMU_SELECTED:
			{
				ui->pushButton_add_dmu->setEnabled(true);
				ui->pushButton_delete_dmu->setEnabled(false);
				ui->pushButton_refresh_dmu_members_from_file->setEnabled(false);
				ui->pushButton_add_dmu_member_by_hand->setEnabled(false);
				ui->pushButton_delete_selected_dmu_members->setEnabled(false);
				ui->pushButton_select_all_dmu_members->setEnabled(false);
				ui->pushButton_deselect_all_dmu_members->setEnabled(false);
			}
			break;

		case LOWER_PANE_BUTTON_ENABLED_STATE::DMU_IS_SELECTED:
			{
				ui->pushButton_add_dmu->setEnabled(true);
				ui->pushButton_delete_dmu->setEnabled(true);
				ui->pushButton_refresh_dmu_members_from_file->setEnabled(true);
				ui->pushButton_add_dmu_member_by_hand->setEnabled(true);
				ui->pushButton_delete_selected_dmu_members->setEnabled(false);
				ui->pushButton_select_all_dmu_members->setEnabled(true);
				ui->pushButton_deselect_all_dmu_members->setEnabled(false);
			}
			break;

		case LOWER_PANE_BUTTON_ENABLED_STATE::NO_PROJECT_OPEN:
			{
				ui->pushButton_add_dmu->setEnabled(false);
				ui->pushButton_delete_dmu->setEnabled(false);
				ui->pushButton_refresh_dmu_members_from_file->setEnabled(false);
				ui->pushButton_add_dmu_member_by_hand->setEnabled(false);
				ui->pushButton_delete_selected_dmu_members->setEnabled(false);
				ui->pushButton_select_all_dmu_members->setEnabled(false);
				ui->pushButton_deselect_all_dmu_members->setEnabled(false);
			}
			break;

		default:
			break;
	}
}
