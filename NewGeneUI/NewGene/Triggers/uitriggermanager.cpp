#include "uitriggermanager.h"
#include "..\..\NewGeneBackEnd\Utilities\NewGeneException.h"

UITriggerManager::UITriggerManager( QObject * parent )
	: QObject(parent)
	, UIManager()
{

	// *************************************************************************
	// All Managers are instantiated AFTER the application event loop is running
	// *************************************************************************

}
