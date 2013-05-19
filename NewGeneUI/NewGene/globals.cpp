#include "globals.h"
#include "newgenemainwindow.h"

NewGeneMainWindow * theMainWindow = NULL;

template<typename MANAGER_CLASS>
MANAGER_CLASS & get_a_ui_manager()
{
	return static_cast<MANAGER_CLASS&>(MANAGER_CLASS::getManager());
}

UISettingsManager & settingsManagerUI()
{
	return get_a_ui_manager<UISettingsManager>();
}

UILoggingManager & loggingManagerUI()
{
	return get_a_ui_manager<UILoggingManager>();
}

UIProjectManager & projectManagerUI()
{
	return get_a_ui_manager<UIProjectManager>();
}

UIModelManager & modelManagerUI()
{
	return get_a_ui_manager<UIModelManager>();
}

UIDocumentManager & documentManagerUI()
{
	return get_a_ui_manager<UIDocumentManager>();
}

UIStatusManager & statusManagerUI()
{
	return get_a_ui_manager<UIStatusManager>();
}
