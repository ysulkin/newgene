#ifndef DISPLAYDMUSREGION_H
#define DISPLAYDMUSREGION_H

#include <QWidget>
#include <QItemSelection>
#include <QStandardItem>

#include "../../newgenewidget.h"

namespace Ui
{
	class DisplayDMUsRegion;
}

class DisplayDMUsRegion : public QWidget, public NewGeneWidget // do not reorder base classes; QWidget instance must be instantiated first
{

		Q_OBJECT

	public:

		explicit DisplayDMUsRegion(QWidget * parent = 0);
		~DisplayDMUsRegion();

		bool event(QEvent * e);

		void HandleChanges(DataChangeMessage const &);

	protected:

		void changeEvent(QEvent * e);

	private:

		Ui::DisplayDMUsRegion * ui;

	signals:
		void RefreshWidget(WidgetDataItemRequest_MANAGE_DMUS_WIDGET);

		// Actions
		void AddDMU(WidgetActionItemRequest_ACTION_ADD_DMU);
		void DeleteDMU(WidgetActionItemRequest_ACTION_DELETE_DMU);
		void AddDMUMembers(WidgetActionItemRequest_ACTION_ADD_DMU_MEMBERS);
		void DeleteDMUMembers(WidgetActionItemRequest_ACTION_DELETE_DMU_MEMBERS);
		void RefreshDMUsFromFile(WidgetActionItemRequest_ACTION_REFRESH_DMUS_FROM_FILE);

	public slots:
		void UpdateInputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIInputProject * project);
		void UpdateOutputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIOutputProject * project);
		void RefreshAllWidgets();
		void WidgetDataRefreshReceive(WidgetDataItem_MANAGE_DMUS_WIDGET);
		void UpdateDMUImportProgressBar(int mode_, int min_, int max_, int val_);

	private slots:
		void ReceiveDMUSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
		void ReceiveBottomPaneSelectionCheckChanged(QStandardItem *);

		// Buttons
		void on_pushButton_add_dmu_clicked();
		void on_pushButton_delete_dmu_clicked();
		void on_pushButton_refresh_dmu_members_from_file_clicked();
		void on_pushButton_add_dmu_member_by_hand_clicked();
		void on_pushButton_delete_selected_dmu_members_clicked();
		void on_pushButton_deselect_all_dmu_members_clicked();
		void on_pushButton_select_all_dmu_members_clicked();

		void on_pushButton_cancel_clicked();

	protected:
		void Empty();

		enum LOWER_PANE_BUTTON_ENABLED_STATE
		{
			NO_DMU_SELECTED
			, DMU_IS_SELECTED
			, NO_PROJECT_OPEN
		};
		void ManageLowerPaneButtonEnabledStates(LOWER_PANE_BUTTON_ENABLED_STATE const state);

	private:
		bool GetSelectedDmuCategory(WidgetInstanceIdentifier & dmu_category, WidgetInstanceIdentifiers & dmu_members);
		void ResetDmuMembersPane(WidgetInstanceIdentifier const & dmu_category, WidgetInstanceIdentifiers const & dmu_members);
		void EmptyDmuMembersPane();

	private:
		bool refresh_dmu_called_after_create;

};

#endif // DISPLAYDMUSREGION_H
