#ifndef UIPROJECT_H
#define UIPROJECT_H

#include "globals.h"
#include <QObject>
#include <memory>
#ifndef Q_MOC_RUN
#	include <boost/filesystem.hpp>
#endif

class UIModelManager;
class UISettingsManager;
class UIDocumentManager;
class UIStatusManager;
class UIProjectManager;
class UIModel;
class NewGeneMainWindow;
class UIAllProjectSettings;
class UILoggingManager;
class UIProjectManager;

class UIProject : public QObject
{
		Q_OBJECT
	public:
		explicit UIProject(Messager & messager, NewGeneMainWindow * parent = 0);
		explicit UIProject(Messager & messager, boost::filesystem::path const path_to_settings, NewGeneMainWindow * parent = 0);

		UIModel * model();
		UIAllProjectSettings * settings();

	signals:

	public slots:

	protected:
		std::unique_ptr<UIModel> _model;
		std::unique_ptr<UIAllProjectSettings> _project_settings;

};

#endif // UIPROJECT_H
