#ifndef NEWGENEVARIABLESUMMARYSCROLLAREA_H
#define NEWGENEVARIABLESUMMARYSCROLLAREA_H

#include <QWidget>
#include "../../../newgenewidget.h"
#include "NewGeneVariableSummaryGroup.h"

namespace Ui
{
	class NewGeneVariableSummaryScrollArea;
}

class NewGeneVariableSummaryScrollArea : public QWidget, public NewGeneWidget // do not reorder base classes; QWidget instance must be instantiated first
{
		Q_OBJECT

	public:

		explicit NewGeneVariableSummaryScrollArea(QWidget * parent = 0);
		~NewGeneVariableSummaryScrollArea();

		void HandleChanges(DataChangeMessage const &);

	protected:

		void changeEvent(QEvent * e);

	private:

		Ui::NewGeneVariableSummaryScrollArea * ui;

	public:

	signals:

		void RefreshWidget(WidgetDataItemRequest_VARIABLE_GROUPS_SUMMARY_SCROLL_AREA);

	public slots:

		void UpdateOutputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIOutputProject * project);
		void UpdateInputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIInputProject * project);
		void RefreshAllWidgets();
		void WidgetDataRefreshReceive(WidgetDataItem_VARIABLE_GROUPS_SUMMARY_SCROLL_AREA); // us, parent
		void WidgetDataRefreshReceive(WidgetDataItem_VARIABLE_GROUPS_SUMMARY_VARIABLE_GROUP_INSTANCE); // child
		void DoTabChange(WidgetInstanceIdentifier);

	protected:

		void Empty();

	private:

		NewGeneVariableSummaryGroup * groups;
		WidgetInstanceIdentifier cached_active_vg;

};

#endif // NEWGENEVARIABLESUMMARYSCROLLAREA_H
