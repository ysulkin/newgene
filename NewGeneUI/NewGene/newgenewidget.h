#ifndef NEWGENEWIDGET_H
#define NEWGENEWIDGET_H

#include "..\..\NewGeneBackEnd\Utilities\NewGeneException.h"
#include "uisettingsmanager.h"
#include "uimodel.h"
#include "uimodelmanager.h"
class QWidget;
class NewGeneMainWindow;

class NewGeneWidget
{
public:
    explicit NewGeneWidget(QWidget * self_ = 0);

protected:
    NewGeneMainWindow & mainWindow();
    UIModel & model();

private:
    QWidget * self;

};

#endif // NEWGENEWIDGET_H
