#include "globals.h"
#include "Settings/SettingsManager.h"
#include "Logging/LoggingManager.h"
#include "Project/ProjectManager.h"
#include "Model/ModelManager.h"
#include "Documents/DocumentManager.h"
#include "Status/StatusManager.h"
#include "Triggers/TriggerManager.h"
#include "Threads/ThreadManager.h"
#include "UIData/UIDataManager.h"
#include "UIAction/UIActionManager.h"
#include "ModelAction/ModelActionManager.h"

template<typename MANAGER_CLASS>
MANAGER_CLASS & get_a_manager()
{
	return static_cast<MANAGER_CLASS &>(MANAGER_CLASS::getManager());
}

SettingsManager & settingsManager()
{
	return get_a_manager<SettingsManager>();
}

LoggingManager & loggingManager()
{
	return get_a_manager<LoggingManager>();
}

ProjectManager & projectManager()
{
	return get_a_manager<ProjectManager>();
}

ModelManager & modelManager()
{
	return get_a_manager<ModelManager>();
}

DocumentManager & documentManager()
{
	return get_a_manager<DocumentManager>();
}

StatusManager & statusManager()
{
	return get_a_manager<StatusManager>();
}

TriggerManager & triggerManager()
{
	return get_a_manager<TriggerManager>();
}

ThreadManager & threadManager()
{
	return get_a_manager<ThreadManager>();
}

UIDataManager & uidataManager()
{
	return get_a_manager<UIDataManager>();
}

UIActionManager & uiactionManager()
{
	return get_a_manager<UIActionManager>();
}

ModelActionManager & modelactionManager()
{
	return get_a_manager<ModelActionManager>();
}
