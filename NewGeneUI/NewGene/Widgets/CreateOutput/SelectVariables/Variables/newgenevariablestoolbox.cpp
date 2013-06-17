#include "newgenevariablestoolbox.h"

#include <QLayout>

NewGeneVariablesToolbox::NewGeneVariablesToolbox( QWidget * parent ) :
	QToolBox( parent ),
	NewGeneWidget( this ) // 'this' pointer is cast by compiler to proper Widget instance, which is already created due to order in which base classes appear in class definition
{
	PrepareOutputWidget(VARIABLE_GROUPS_TOOLBOX);
	layout()->setSpacing( 1 );
}

void NewGeneVariablesToolbox::UpdateOutputConnections(UIProjectManager::UPDATE_CONNECTIONS_TYPE connection_type, UIOutputProject * project)
{
	NewGeneWidget::UpdateOutputConnections(connection_type, project);
	connect(project->getConnector(), SIGNAL(WidgetDataRefresh(WidgetDataItem_VARIABLE_GROUPS_TOOLBOX)), this, SLOT(WidgetDataRefreshReceive(WidgetDataItem_VARIABLE_GROUPS_TOOLBOX)));
}

void NewGeneVariablesToolbox::RefreshAllWidgets()
{
	emit RefreshWidget(VARIABLE_GROUPS_SCROLL_AREA);
}

void NewGeneVariablesToolbox::WidgetDataRefreshReceive(WidgetDataItem_VARIABLE_GROUPS_TOOLBOX widget_data)
{
	std::for_each(widget_data.variable_group_long_names.cbegin(), widget_data.variable_group_long_names.cend(), [&](std::string const & variable_group_long_name)
	{
		NewGeneVariableGroup * tmpGrp = new NewGeneVariableGroup( this );
		addItem( tmpGrp, variable_group_long_name.c_str() );
	});
//	groups = new NewGeneVariableGroup( this );
//	addItem( groups, "Country Variables" );

//	NewGeneVariableGroup * tmpGrp = new NewGeneVariableGroup( this );
//	addItem( tmpGrp, "MID Detail Variables" );
}
