#ifndef UILOGGINGMANAGER_H
#define UILOGGINGMANAGER_H

#include "uimanager.h"
#include "..\..\..\NewGeneBackEnd\Logging\LoggingManager.h"
#ifndef Q_MOC_RUN
#	include <boost/filesystem.hpp>
#endif

class NewGeneMainWindow;

class UILoggingManager : public QObject, public UIManager<UILoggingManager, LoggingManager, MANAGER_DESCRIPTION_NAMESPACE::MANAGER_LOGGING>
{
		Q_OBJECT
	public:
		explicit UILoggingManager( QObject * parent = 0 );

	signals:

	public slots:

	protected:

		bool ObtainLogfilePath();

	private:

		boost::filesystem::path loggingPath;

};

#endif // UILOGGINGMANAGER_H
