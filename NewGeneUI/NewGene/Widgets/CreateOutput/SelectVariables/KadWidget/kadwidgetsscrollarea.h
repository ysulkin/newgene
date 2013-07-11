#ifndef KADWIDGETSSCROLLAREA_H
#define KADWIDGETSSCROLLAREA_H

#include <QWidget>
#include "..\..\..\newgenewidget.h"

class KadWidgetsScrollArea : public QWidget, public NewGeneWidget
{

        Q_OBJECT

    public:

        explicit KadWidgetsScrollArea(QWidget *parent = 0);

    signals:

        void RefreshWidget(WidgetDataItemRequest_KAD_SPIN_CONTROLS_AREA);

    public slots:

        void UpdateOutputConnections(UIProjectManager::UPDATE_CONNECTIONS_TYPE connection_type, UIOutputProject * project);
        void RefreshAllWidgets();
        void WidgetDataRefreshReceive(WidgetDataItem_KAD_SPIN_CONTROLS_AREA); // us, parent
        void WidgetDataRefreshReceive(WidgetDataItem_KAD_SPIN_CONTROL_WIDGET); // child

};

#endif // KADWIDGETSSCROLLAREA_H
