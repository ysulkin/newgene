#ifndef NEWGENEVARIABLESUMMARY_H
#define NEWGENEVARIABLESUMMARY_H

#include <QWidget>
#include "../../../newgenewidget.h"
#include <QStringListModel>

namespace Ui
{
	class NewGeneVariableSummary;
}

class NewGeneVariableSummary : public QWidget, public NewGeneWidget // do not reorder base classes; QWidget instance must be instantiated first
{

		Q_OBJECT

	public:

		explicit NewGeneVariableSummary(QWidget * parent = 0);
		~NewGeneVariableSummary();

	signals:

	public slots:

		void UpdateOutputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIOutputProject * project);
		void UpdateInputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIInputProject * project);

	protected:

		void changeEvent(QEvent * e);

	private:

		Ui::NewGeneVariableSummary * ui;

	public:

		friend class NewGeneMainWindow;

	private slots:
		void on_pushButtonDeselectAllVariables_clicked();
};

#endif // NEWGENEVARIABLESUMMARY_H
