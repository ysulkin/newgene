#ifndef LIMIT_DMUS_REGION_H
#define LIMIT_DMUS_REGION_H

#include <QWidget>
#include "../../newgenewidget.h"

namespace Ui
{
    class limit_dmus_region;
}

class limit_dmus_region : public QWidget, public NewGeneWidget // do not reorder base classes; QWidget instance must be instantiated first
{

        Q_OBJECT

    public:

        explicit limit_dmus_region(QWidget *parent = 0);
        ~limit_dmus_region();

        void HandleChanges(DataChangeMessage const &);

    signals:

        void RefreshWidget(WidgetDataItemRequest_LIMIT_DMUS_TAB);

    public slots:

        void UpdateOutputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIOutputProject * project);
        void UpdateInputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIInputProject * project);
        void RefreshAllWidgets();
        void WidgetDataRefreshReceive(WidgetDataItem_LIMIT_DMUS_TAB);

    private slots:

        void ReceiveDMUSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);

    protected:

        void changeEvent( QEvent * e );
        void Empty();

    private:

        void EmptyDmuMembersPanes();
        void EmptyBottomLeftPane();
        void EmptyBottomRightPane();

        void ResetDmuMembersPanes(WidgetInstanceIdentifier const & dmu_category, bool const is_limited, WidgetInstanceIdentifiers const & dmu_set_members__all, WidgetInstanceIdentifiers const & dmu_set_members_not_limited, dmu_set_members__limited);
        void ResetBottomLeftPane(WidgetInstanceIdentifiers const & dmu_set_members__not_limited);
        void ResetBottomRightPane(WidgetInstanceIdentifiers const & dmu_set_members__limited);

    private:

        Ui::limit_dmus_region * ui;

    signals:

    public slots:

};

#endif // LIMIT_DMUS_REGION_H
