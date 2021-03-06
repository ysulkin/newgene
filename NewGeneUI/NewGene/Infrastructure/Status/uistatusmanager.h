#ifndef UISTATUSMANAGER_H
#define UISTATUSMANAGER_H

#include "globals.h"
#include "Infrastructure/uimanager.h"
#include "../../../NewGeneBackEnd/Status/StatusManager.h"

class NewGeneMainWindow;

class UIStatusManager : public QObject, public
	UIManager<UIStatusManager, StatusManager, MANAGER_DESCRIPTION_NAMESPACE::MANAGER_STATUS_UI, MANAGER_DESCRIPTION_NAMESPACE::MANAGER_STATUS>
{
		Q_OBJECT

	public:

		enum IMPORTANCE
		{
			IMPORTANCE_DEBUG
			, IMPORTANCE_STANDARD
			, IMPORTANCE_HIGH
			, IMPORTANCE_CRITICAL
		};

		explicit UIStatusManager(QObject * parent, UIMessager & messager);

		void LogStatus(QString const & _status, IMPORTANCE const importance_level = IMPORTANCE_STANDARD);
		void PostStatus(QString const & _status, IMPORTANCE const importance_level = IMPORTANCE_STANDARD, bool const forbidWritingToLog = false);

	signals:

	public slots:
		void ReceiveStatus(STD_STRING, int, bool);

	private:

};

#endif // UISTATUSMANAGER_H
