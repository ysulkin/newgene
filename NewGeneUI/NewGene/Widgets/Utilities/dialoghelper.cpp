#ifndef Q_MOC_RUN
	#include <boost/filesystem.hpp>
	#include <boost/format.hpp>
#endif
#include "dialoghelper.h"
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include <QRadioButton>
#include <QListView>
#include <QSpacerItem>
#include <QStandardItemModel>
#include <QGroupBox>

#include "../Project/uiprojectmanager.h"
#include "../Project/uiinputproject.h"
#include "../../../../../NewGeneBackEnd/Utilities/Validation.h"
#include "../../../../NewGeneBackEnd/Utilities/TimeRangeHelper.h"
#include "../../../../NewGeneBackEnd/Model/InputModel.h"
#include "../../../../NewGeneBackEnd/Model/Tables/TableInstances/DMU.h"
#include "../../../../NewGeneBackEnd/Model/Tables/TableInstances/UOA.h"
#include "../../../../NewGeneBackEnd/Model/Tables/TableInstances/VariableGroup.h"
#include "htmldelegate.h"

#include <set>

void DialogHelper::AddFileChooserBlock(QDialog & dialog, QFormLayout & form, QBoxLayout & formFileSelection, QWidget & FileChooserWidget, QList<QLineEdit *> & fieldsFileChooser,
									   std::vector<std::string> const & fileChooserStrings)
{

	FileChooserWidget.setLayout(&formFileSelection);

	QString labelFilePathName = QString(fileChooserStrings[0].c_str());
	labelFilePathName += ":";

	QLineEdit * lineEditFilePathName = new QLineEdit(&FileChooserWidget);
	setLineEditWidth(lineEditFilePathName);
	QPushButton * buttonFilePathName = new QPushButton("Browse...", &FileChooserWidget);
	buttonFilePathName->setMaximumWidth(100);
	fieldsFileChooser.append(lineEditFilePathName);

	lineEditFilePathName->setMinimumWidth(400);

	QWidget * spacer1 = new QWidget {};
	spacer1->setMinimumHeight(20);
	form.addRow(spacer1);
	form.addRow(labelFilePathName, lineEditFilePathName);
	form.addRow("", buttonFilePathName);
	form.itemAt(form.rowCount() - 1, QFormLayout::FieldRole)->setAlignment(Qt::AlignRight);
	QWidget * spacer2 = new QWidget {};
	spacer2->setMinimumHeight(20);
	form.addRow(spacer2);

	QObject::connect(buttonFilePathName, &QPushButton::clicked, [ =, &dialog, &fileChooserStrings]()
	{
		QString the_file = QFileDialog::getOpenFileName(&dialog, fileChooserStrings[1].c_str(), fileChooserStrings[2].c_str(), fileChooserStrings[2].c_str());

		if (!the_file.isEmpty())
		{
			lineEditFilePathName->setText(the_file);
		}
	});

}

bool DialogHelper::ValidateFileChooserBlock(QList<QLineEdit *> & fieldsFileChooser, std::vector<std::string> & dataFileChooser, std::string & errorMsg)
{

	bool valid = true;
	dataFileChooser.clear();

	QLineEdit * data_column_file_pathname_field = fieldsFileChooser[0];

	if (data_column_file_pathname_field)
	{
		std::string filePathName(data_column_file_pathname_field->text().toStdString());

		if (!boost::filesystem::exists(filePathName))
		{
			valid = false;
			errorMsg = "The indicated file does not exist.";
		}
		else if (boost::filesystem::is_directory(filePathName))
		{
			valid = false;
			errorMsg = "The indicated file is a directory.";
		}

		if (valid)
		{
			dataFileChooser.push_back(filePathName);
		}
	}
	else
	{
		valid = false;
		errorMsg = "You must select a file.";
	}

	return valid;

}

void DialogHelper::AddTimeRangeGranularitySelectionBlock(QDialog & dialog, QFormLayout & form, QVBoxLayout & formTimeRangeGranularitySelection,
		QList<QRadioButton *> & radioButtonsTimeRangeGranularity)
{

	QWidget * spacer1 { new QWidget{} };
	spacer1->setMinimumHeight(20);
	form.addRow(spacer1);

	QRadioButton * NButton = new QRadioButton("None", &dialog);
	QRadioButton * YButton = new QRadioButton("Year", &dialog);
	QRadioButton * YMButton = new QRadioButton("Year, Month", &dialog);
	QRadioButton * YMDButton = new QRadioButton("Year, Month, Day", &dialog);

	formTimeRangeGranularitySelection.addWidget(NButton);
	formTimeRangeGranularitySelection.addWidget(YButton);
	formTimeRangeGranularitySelection.addWidget(YMButton);
	formTimeRangeGranularitySelection.addWidget(YMDButton);

	QGroupBox * groupBox = new QGroupBox("Time range granularity options");
	groupBox->setLayout(&formTimeRangeGranularitySelection);

	form.addRow(groupBox);

	QWidget * spacer2 { new QWidget{} };
	spacer2->setMinimumHeight(20);
	form.addRow(spacer2);

	NButton->setChecked(true);

	radioButtonsTimeRangeGranularity << NButton << YButton << YMButton << YMDButton;

}

void DialogHelper::AddTimeRangeSelectorBlock(

	QDialog & dialog,
	QFormLayout & form,
	QList<QLineEdit *> & fieldsTimeRange,
	QList<QRadioButton *> & radioButtonsTimeRange,
	QBoxLayout & formTimeRangeSelection,

	QWidget & YearWidget,
	QFormLayout & formYearOptions,

	QWidget & YearMonthDayWidget,
	QFormLayout & formYearMonthDayOptions,
	QWidget & YearMonthDayWidget_ints,
	QFormLayout & formYearMonthDayOptions_ints,
	QWidget & YearMonthDayWidget_strings,
	QFormLayout & formYearMonthDayOptions_strings,
	QBoxLayout & formYMDTimeRange_StringVsInt,
	QList<QRadioButton *> & radioButtonsYMD_StringVsInt_TimeRange,

	QWidget & YearMonthWidget,
	QFormLayout & formYearMonthOptions,
	QWidget & YearMonthWidget_ints,
	QFormLayout & formYearMonthOptions_ints,
	QWidget & YearMonthWidget_strings,
	QFormLayout & formYearMonthOptions_strings,
	QBoxLayout & formYMTimeRange_StringVsInt,
	QList<QRadioButton *> & radioButtonsYM_StringVsInt_TimeRange,

	TIME_GRANULARITY const & time_range_granularity

)
{

	// Time range RADIO BUTTONS

	// ************************************************************************************ //
	// Always hidden - But used by validation routine
	// to determine which time granularity is selected
	// ************************************************************************************ //
	QRadioButton * YButton = new QRadioButton("Year", &dialog);
	QRadioButton * YMDButton = new QRadioButton("Year, Month, Day", &dialog);
	QRadioButton * YMButton = new QRadioButton("Year, Month", &dialog);
	formTimeRangeSelection.addWidget(new QLabel("Time range options:"));
	formTimeRangeSelection.addWidget(YButton);
	formTimeRangeSelection.addWidget(YMDButton);
	formTimeRangeSelection.addWidget(YMButton);
	form.addRow(&formTimeRangeSelection);

	radioButtonsTimeRange << YButton << YMDButton << YMButton;
	YButton->hide();
	YMDButton->hide();
	YMButton->hide();




	// Time range OPTIONS - Year
	YearWidget.setLayout(&formYearOptions);

	// year
	QString labelyYearStart = QString("'Year Start' (integer or text) column heading:");
	QLineEdit * lineEdityYearStart = new QLineEdit(&YearWidget);
	formYearOptions.addRow(labelyYearStart, lineEdityYearStart);
	fieldsTimeRange << lineEdityYearStart;                                  // 1
	QString labelyYearEnd = QString("'Year End' (integer or text) column name (if present):");
	QLineEdit * lineEdityYearEnd = new QLineEdit(&YearWidget);
	formYearOptions.addRow(labelyYearEnd, lineEdityYearEnd);
	fieldsTimeRange << lineEdityYearEnd;                                    // 2

	YearWidget.hide();
	form.addRow(&YearWidget);




	// Time range OPTIONS - Year, Month, Day
	YearMonthDayWidget.setLayout(&formYearMonthDayOptions);

	QRadioButton * YMDIntButton = new QRadioButton("Time range columns are split: one for year, one for month, and one for day", &YearMonthDayWidget);
	QRadioButton * YMDStringButton = new QRadioButton("Time range columns contain text dates separated by slashes or dashes", &YearMonthDayWidget);
	formYMDTimeRange_StringVsInt.addWidget(YMDIntButton);
	formYMDTimeRange_StringVsInt.addWidget(YMDStringButton);
	formYMDTimeRange_StringVsInt.addWidget(new QLabel("        Examples are \"11/03/1992\", \"1992/03/20\", \"1992\\11\\07\", \"1992-09-07\", \"Feb 07, 1992\" and \"February 07, 1992\""));
	formYearMonthDayOptions.addRow(&formYMDTimeRange_StringVsInt);
	formYearMonthDayOptions.addRow(new QLabel());
	radioButtonsYMD_StringVsInt_TimeRange << YMDStringButton << YMDIntButton;

	// year
	QString labelymdYearStart = QString("'Year Start' (integer) column heading:");
	QLineEdit * lineEditymdYearStart = new QLineEdit(&YearMonthDayWidget_ints);
	formYearMonthDayOptions_ints.addRow(labelymdYearStart, lineEditymdYearStart);
	fieldsTimeRange << lineEditymdYearStart;                                // 3
	QString labelymdYearEnd = QString("'Year End' (integer) column heading (if present):");
	QLineEdit * lineEditymdYearEnd = new QLineEdit(&YearMonthDayWidget_ints);
	formYearMonthDayOptions_ints.addRow(labelymdYearEnd, lineEditymdYearEnd);
	fieldsTimeRange << lineEditymdYearEnd;                                  // 4

	// month
	QString labelymdMonthStart = QString("'Month Start' (integer) column heading:");
	QLineEdit * lineEditymdMonthStart = new QLineEdit(&YearMonthDayWidget_ints);
	formYearMonthDayOptions_ints.addRow(labelymdMonthStart, lineEditymdMonthStart);
	fieldsTimeRange << lineEditymdMonthStart;                               // 5
	QString labelymdMonthEnd = QString("'Month End' (integer) column heading (if present):");
	QLineEdit * lineEditymdMonthEnd = new QLineEdit(&YearMonthDayWidget_ints);
	formYearMonthDayOptions_ints.addRow(labelymdMonthEnd, lineEditymdMonthEnd);
	fieldsTimeRange << lineEditymdMonthEnd;                                 // 6

	// day
	QString labelymdDayStart = QString("'Day Start' (integer) column heading:");
	QLineEdit * lineEditymdDayStart = new QLineEdit(&YearMonthDayWidget_ints);
	formYearMonthDayOptions_ints.addRow(labelymdDayStart, lineEditymdDayStart);
	fieldsTimeRange << lineEditymdDayStart;                                 // 7
	QString labelymdDayEnd = QString("'Day End' (integer) column heading (if present):");
	QLineEdit * lineEditymdDayEnd = new QLineEdit(&YearMonthDayWidget_ints);
	formYearMonthDayOptions_ints.addRow(labelymdDayEnd, lineEditymdDayEnd);
	fieldsTimeRange << lineEditymdDayEnd;                                   // 8

	formYearMonthDayOptions.addRow(&YearMonthDayWidget_ints);
	YearMonthDayWidget_ints.hide();

	// string form of dates
	QString labelymdStart = QString("'Starting Day' (text) column heading:");
	QLineEdit * lineEditymdStart = new QLineEdit(&YearMonthDayWidget_strings);
	formYearMonthDayOptions_strings.addRow(labelymdStart, lineEditymdStart);
	fieldsTimeRange << lineEditymdStart;                                    // 9
	QString labelymdEnd = QString("'Ending Day' (text) column heading (if present):");
	QLineEdit * lineEditymdEnd = new QLineEdit(&YearMonthDayWidget_strings);
	formYearMonthDayOptions_strings.addRow(labelymdEnd, lineEditymdEnd);
	fieldsTimeRange << lineEditymdEnd;                                      // 10

	formYearMonthDayOptions.addRow(&YearMonthDayWidget_strings);
	YearMonthDayWidget_strings.hide();

	YearMonthDayWidget.hide();
	form.addRow(&YearMonthDayWidget);

	YMDIntButton->setChecked(true);

	if (YMDIntButton->isChecked())
	{
		YearMonthDayWidget_ints.show();
	}

	if (YMDStringButton->isChecked())
	{
		YearMonthDayWidget_strings.show();
	}

	QObject::connect(YMDIntButton, &QRadioButton::toggled, [ =, &YearMonthDayWidget_ints, &YearMonthDayWidget_strings]()
	{
		YearMonthDayWidget_ints.hide();
		YearMonthDayWidget_strings.hide();

		if (YMDIntButton->isChecked())
		{
			YearMonthDayWidget_ints.show();
		}

		if (YMDStringButton->isChecked())
		{
			YearMonthDayWidget_strings.show();
		}
	});

	QObject::connect(YMDStringButton, &QRadioButton::toggled, [ =, &YearMonthDayWidget_ints, &YearMonthDayWidget_strings]()
	{
		YearMonthDayWidget_ints.hide();
		YearMonthDayWidget_strings.hide();

		if (YMDIntButton->isChecked())
		{
			YearMonthDayWidget_ints.show();
		}

		if (YMDStringButton->isChecked())
		{
			YearMonthDayWidget_strings.show();
		}
	});





	// Time range OPTIONS - Month
	YearMonthWidget.setLayout(&formYearMonthOptions);

	QRadioButton * YMIntButton = new QRadioButton("Time range columns are split: one for year and one for month", &YearMonthDayWidget);
	QRadioButton * YMStringButton = new QRadioButton("Time range columns contain text dates separated by slashes or dashes", &YearMonthDayWidget);
	formYMTimeRange_StringVsInt.addWidget(YMIntButton);
	formYMTimeRange_StringVsInt.addWidget(YMStringButton);
	formYMTimeRange_StringVsInt.addWidget(new
										  QLabel("        Examples are \"03/1992\", \"1992/03\", \"1992\\03\", \"1992-03\", \"Feb 1992\" or \"February, 1992\"\n        You may also include the day (which will be ignored), such as \"11/03/1992\" or \"February 03, 1992\""));
	formYearMonthOptions.addRow(&formYMTimeRange_StringVsInt);
	formYearMonthOptions.addRow(new QLabel());
	radioButtonsYM_StringVsInt_TimeRange << YMStringButton << YMIntButton;

	// year
	QString labelymYearStart = QString("'Year Start' column heading:");
	QLineEdit * lineEditymYearStart = new QLineEdit(&YearMonthWidget_ints);
	formYearMonthOptions_ints.addRow(labelymYearStart, lineEditymYearStart);
	fieldsTimeRange << lineEditymYearStart;                                // 11
	QString labelymYearEnd = QString("'Year End' column heading (if present):");
	QLineEdit * lineEditymYearEnd = new QLineEdit(&YearMonthWidget_ints);
	formYearMonthOptions_ints.addRow(labelymYearEnd, lineEditymYearEnd);
	fieldsTimeRange << lineEditymYearEnd;                                  // 12

	// month
	QString labelymMonthStart = QString("'Month Start' column heading:");
	QLineEdit * lineEditymMonthStart = new QLineEdit(&YearMonthWidget_ints);
	formYearMonthOptions_ints.addRow(labelymMonthStart, lineEditymMonthStart);
	fieldsTimeRange << lineEditymMonthStart;                               // 13
	QString labelymMonthEnd = QString("'Month End' column heading (if present):");
	QLineEdit * lineEditymMonthEnd = new QLineEdit(&YearMonthWidget_ints);
	formYearMonthOptions_ints.addRow(labelymMonthEnd, lineEditymMonthEnd);
	fieldsTimeRange << lineEditymMonthEnd;                                 // 14

	formYearMonthOptions.addRow(&YearMonthWidget_ints);
	YearMonthWidget_ints.hide();

	// string form of dates
	QString labelymStart = QString("'Starting Month' (text) column heading:");
	QLineEdit * lineEditymStart = new QLineEdit(&YearMonthWidget_strings);
	formYearMonthOptions_strings.addRow(labelymStart, lineEditymStart);
	fieldsTimeRange << lineEditymStart;                                    // 15
	QString labelymEnd = QString("'Ending Month' (text) column heading (if present):");
	QLineEdit * lineEditymEnd = new QLineEdit(&YearMonthWidget_strings);
	formYearMonthOptions_strings.addRow(labelymEnd, lineEditymEnd);
	fieldsTimeRange << lineEditymEnd;                                      // 16

	formYearMonthOptions.addRow(&YearMonthWidget_strings);
	YearMonthWidget_strings.hide();

	YearMonthWidget.hide();
	form.addRow(&YearMonthWidget);

	YMIntButton->setChecked(true);

	if (YMIntButton->isChecked())
	{
		YearMonthWidget_ints.show();
	}

	if (YMStringButton->isChecked())
	{
		YearMonthWidget_strings.show();
	}

	QObject::connect(YMIntButton, &QRadioButton::toggled, [ =, &YearMonthWidget_ints, &YearMonthWidget_strings]()
	{
		YearMonthWidget_ints.hide();
		YearMonthWidget_strings.hide();

		if (YMIntButton->isChecked())
		{
			YearMonthWidget_ints.show();
		}

		if (YMStringButton->isChecked())
		{
			YearMonthWidget_strings.show();
		}
	});

	QObject::connect(YMStringButton, &QRadioButton::toggled, [ =, &YearMonthWidget_ints, &YearMonthWidget_strings]()
	{
		YearMonthWidget_ints.hide();
		YearMonthWidget_strings.hide();

		if (YMIntButton->isChecked())
		{
			YearMonthWidget_ints.show();
		}

		if (YMStringButton->isChecked())
		{
			YearMonthWidget_strings.show();
		}
	});



	switch (time_range_granularity)
	{

		case TIME_GRANULARITY__DAY:
			{
				YMDButton->setChecked(true);
				YearMonthDayWidget.show();
			}
			break;

		case TIME_GRANULARITY__YEAR:
			{
				YButton->setChecked(true);
				YearWidget.show();
			}
			break;

		case TIME_GRANULARITY__MONTH:
			{
				YMButton->setChecked(true);
				YearMonthWidget.show();
			}
			break;

		default:
			{
				boost::format msg("Invalid time range setting.");
				throw NewGeneException() << newgene_error_description(msg.str());
			}
			break;

	}

	QObject::connect(YButton, &QRadioButton::toggled, [&]()
	{
		YearMonthDayWidget.hide();
		YearWidget.hide();
		YearMonthWidget.hide();

		if (YButton->isChecked())
		{
			YearWidget.show();
		}

		if (YMDButton->isChecked())
		{
			YearMonthDayWidget.show();
		}

		if (YMButton->isChecked())
		{
			YearMonthWidget.show();
		}
	});

	QObject::connect(YMDButton, &QRadioButton::toggled, [&]()
	{
		YearMonthDayWidget.hide();
		YearWidget.hide();
		YearMonthWidget.hide();

		if (YButton->isChecked())
		{
			YearWidget.show();
		}

		if (YMDButton->isChecked())
		{
			YearMonthDayWidget.show();
		}

		if (YMButton->isChecked())
		{
			YearMonthWidget.show();
		}
	});

	QObject::connect(YMButton, &QRadioButton::toggled, [&]()
	{
		YearMonthDayWidget.hide();
		YearWidget.hide();
		YearMonthWidget.hide();

		if (YButton->isChecked())
		{
			YearWidget.show();
		}

		if (YMDButton->isChecked())
		{
			YearMonthDayWidget.show();
		}

		if (YMButton->isChecked())
		{
			YearMonthWidget.show();
		}
	});

}

bool DialogHelper::ValidateTimeRangeBlock
(
	QDialog & dialog,
	QFormLayout & form,
	QList<QLineEdit *> & fieldsTimeRange,
	QList<QRadioButton *> & radioButtonsTimeRange,

	QWidget & YearWidget,
	QFormLayout & formYearOptions,

	QWidget & YearMonthDayWidget,
	QFormLayout & formYearMonthDayOptions,
	QWidget & YearMonthDayWidget_ints,
	QFormLayout & formYearMonthDayOptions_ints,
	QWidget & YearMonthDayWidget_strings,
	QFormLayout & formYearMonthDayOptions_strings,
	QList<QRadioButton *> & radioButtonsYMD_StringVsInt_TimeRange,

	QWidget & YearMonthWidget,
	QFormLayout & formYearMonthOptions,
	QWidget & YearMonthWidget_ints,
	QFormLayout & formYearMonthOptions_ints,
	QWidget & YearMonthWidget_strings,
	QFormLayout & formYearMonthOptions_strings,
	QList<QRadioButton *> & radioButtonsYM_StringVsInt_TimeRange,

	TIME_GRANULARITY const & time_range_granularity,
	std::vector<std::string> & dataTimeRange,
	bool & warnEmptyEndingTimeCols,
	std::string & errorMsg
)
{

	warnEmptyEndingTimeCols = false;

	dataTimeRange.clear();

	QRadioButton * YButton = radioButtonsTimeRange[0];
	QRadioButton * YMDButton = radioButtonsTimeRange[1];
	QRadioButton * YMButton = radioButtonsTimeRange[2];

	if (!YButton || !YMDButton || !YMButton)
	{
		boost::format msg("Invalid time range selection radio buttons");
		errorMsg = msg.str();
		return false;
	}

	QRadioButton * YMDStringButton = radioButtonsYMD_StringVsInt_TimeRange[0];
	QRadioButton * YMDIntButton = radioButtonsYMD_StringVsInt_TimeRange[1];

	if (!YMDStringButton || !YMDIntButton)
	{
		boost::format msg("Invalid time range (text vs. int) radio buttons");
		errorMsg = msg.str();
		return false;
	}

	QRadioButton * YMStringButton = radioButtonsYM_StringVsInt_TimeRange[0];
	QRadioButton * YMIntButton = radioButtonsYM_StringVsInt_TimeRange[1];

	if (!YMStringButton || !YMIntButton)
	{
		boost::format msg("Invalid time range (text vs. int) radio buttons");
		errorMsg = msg.str();
		return false;
	}

	int currentIndex = 0;
	QLineEdit * timeRange_y_yearStart = fieldsTimeRange[currentIndex++];
	QLineEdit * timeRange_y_yearEnd = fieldsTimeRange[currentIndex++];

	if (YButton->isChecked())
	{

		if (!timeRange_y_yearStart || !timeRange_y_yearEnd)
		{
			boost::format msg("Invalid date fields");
			errorMsg = msg.str();
			return false;
		}

		std::string y_yearStart(timeRange_y_yearStart->text().toStdString());
		std::string y_yearEnd(timeRange_y_yearEnd->text().toStdString());

		bool valid = true;

		if (valid)
		{
			valid = Validation::ValidateColumnName(y_yearStart, "Start Year", true, errorMsg);
		}

		if (valid)
		{
			if (!boost::trim_copy(y_yearEnd).empty())
			{
				valid = Validation::ValidateColumnName(y_yearEnd, "End Year", false, errorMsg);

				if (y_yearEnd == y_yearStart)
				{
					boost::format msg("Duplicate time range column names");
					errorMsg = msg.str();
					return false;
				}
			}
			else
			{
				warnEmptyEndingTimeCols = true;
			}
		}

		if (valid)
		{
			dataTimeRange.push_back(y_yearStart);

			if (!y_yearEnd.empty())
			{
				dataTimeRange.push_back(y_yearEnd);
			}
		}

		return valid;

	}

	QLineEdit * timeRange_ymd_yearStart = fieldsTimeRange[currentIndex++];
	QLineEdit * timeRange_ymd_yearEnd = fieldsTimeRange[currentIndex++];
	QLineEdit * timeRange_ymd_monthStart = fieldsTimeRange[currentIndex++];
	QLineEdit * timeRange_ymd_monthEnd = fieldsTimeRange[currentIndex++];
	QLineEdit * timeRange_ymd_dayStart = fieldsTimeRange[currentIndex++];
	QLineEdit * timeRange_ymd_dayEnd = fieldsTimeRange[currentIndex++];
	QLineEdit * timeRange_ymd_Start = fieldsTimeRange[currentIndex++];
	QLineEdit * timeRange_ymd_End = fieldsTimeRange[currentIndex++];

	if (YMDButton->isChecked())
	{

		if (!timeRange_ymd_yearStart || !timeRange_ymd_yearEnd || !timeRange_ymd_monthStart || !timeRange_ymd_monthEnd || !timeRange_ymd_dayStart || !timeRange_ymd_dayEnd
			|| !timeRange_ymd_Start || !timeRange_ymd_End)
		{
			boost::format msg("Invalid date fields");
			errorMsg = msg.str();
			return false;
		}

		std::string ymd_yearStart(timeRange_ymd_yearStart->text().toStdString());
		std::string ymd_yearEnd(timeRange_ymd_yearEnd->text().toStdString());
		std::string ymd_monthStart(timeRange_ymd_monthStart->text().toStdString());
		std::string ymd_monthEnd(timeRange_ymd_monthEnd->text().toStdString());
		std::string ymd_dayStart(timeRange_ymd_dayStart->text().toStdString());
		std::string ymd_dayEnd(timeRange_ymd_dayEnd->text().toStdString());
		std::string ymd_Start(timeRange_ymd_Start->text().toStdString());
		std::string ymd_End(timeRange_ymd_End->text().toStdString());

		bool valid = true;

		bool using_string_fields = false;

		if (YMDStringButton->isChecked())
		{
			using_string_fields = true;
		}

		if (using_string_fields)
		{

			if (valid)
			{
				valid = Validation::ValidateColumnName(ymd_Start, "Starting Day", true, errorMsg);
			}

			if (valid)
			{
				valid = Validation::ValidateColumnName(ymd_End, "Ending Day", false, errorMsg);
			}

		}
		else
		{

			if (valid)
			{
				valid = Validation::ValidateColumnName(ymd_yearStart, "Start Year", true, errorMsg);
			}

			if (valid)
			{
				valid = Validation::ValidateColumnName(ymd_monthStart, "Start Month", true, errorMsg);
			}

			if (valid)
			{
				valid = Validation::ValidateColumnName(ymd_dayStart, "Start Day", true, errorMsg);
			}

			if (valid)
			{
				valid = Validation::ValidateColumnName(ymd_yearEnd, "End Year", false, errorMsg);
			}

			if (valid)
			{
				valid = Validation::ValidateColumnName(ymd_monthEnd, "End Month", false, errorMsg);
			}

			if (valid)
			{
				valid = Validation::ValidateColumnName(ymd_dayEnd, "End Day", false, errorMsg);
			}

		}

		if (valid && using_string_fields)
		{
			dataTimeRange.push_back(ymd_Start);

			if (!ymd_End.empty())
			{
				dataTimeRange.push_back(ymd_End);
			}
			else
			{
				warnEmptyEndingTimeCols = true;
			}
		}
		else if (valid)
		{
			dataTimeRange.push_back(ymd_yearStart);
			dataTimeRange.push_back(ymd_monthStart);
			dataTimeRange.push_back(ymd_dayStart);

			if (!ymd_yearEnd.empty() || !ymd_monthEnd.empty() || !ymd_dayEnd.empty())
			{
				dataTimeRange.push_back(ymd_yearEnd);
				dataTimeRange.push_back(ymd_monthEnd);
				dataTimeRange.push_back(ymd_dayEnd);
			}
			else
			{
				warnEmptyEndingTimeCols = true;
			}
		}

		if (valid)
		{
			std::for_each(dataTimeRange.begin(), dataTimeRange.end(), std::bind(boost::trim<std::string>, std::placeholders::_1, std::locale()));
			int nEmptyCols = 0;
			std::for_each(dataTimeRange.begin(), dataTimeRange.end(), [&](std::string & colname) { if (colname.empty()) { ++nEmptyCols; } });

			if (nEmptyCols > 0) { --nEmptyCols; }

			std::set<std::string> testtimerangecols(dataTimeRange.cbegin(), dataTimeRange.cend());

			if (testtimerangecols.size() != dataTimeRange.size() - nEmptyCols)
			{
				boost::format msg("Duplicate time range column headings");
				errorMsg = msg.str();
				return false;
			}
		}

		return valid;

	}

	QLineEdit * timeRange_ym_yearStart = fieldsTimeRange[currentIndex++];
	QLineEdit * timeRange_ym_yearEnd = fieldsTimeRange[currentIndex++];
	QLineEdit * timeRange_ym_monthStart = fieldsTimeRange[currentIndex++];
	QLineEdit * timeRange_ym_monthEnd = fieldsTimeRange[currentIndex++];
	QLineEdit * timeRange_ym_Start = fieldsTimeRange[currentIndex++];
	QLineEdit * timeRange_ym_End = fieldsTimeRange[currentIndex++];

	if (YMButton->isChecked())
	{

		if (!timeRange_ym_yearStart || !timeRange_ym_yearEnd || !timeRange_ym_monthStart || !timeRange_ym_monthEnd || !timeRange_ym_Start || !timeRange_ym_End)
		{
			boost::format msg("Invalid date fields");
			errorMsg = msg.str();
			return false;
		}

		std::string ym_yearStart(timeRange_ym_yearStart->text().toStdString());
		std::string ym_yearEnd(timeRange_ym_yearEnd->text().toStdString());
		std::string ym_monthStart(timeRange_ym_monthStart->text().toStdString());
		std::string ym_monthEnd(timeRange_ym_monthEnd->text().toStdString());
		std::string ym_Start(timeRange_ym_Start->text().toStdString());
		std::string ym_End(timeRange_ym_End->text().toStdString());

		bool valid = true;

		bool using_string_fields = false;

		if (YMStringButton->isChecked())
		{
			using_string_fields = true;
		}

		if (using_string_fields)
		{

			if (valid)
			{
				valid = Validation::ValidateColumnName(ym_Start, "Starting Month", true, errorMsg);
			}

			if (valid)
			{
				valid = Validation::ValidateColumnName(ym_End, "Ending Month", false, errorMsg);
			}

		}
		else
		{

			if (valid)
			{
				valid = Validation::ValidateColumnName(ym_yearStart, "Start Year", true, errorMsg);
			}

			if (valid)
			{
				valid = Validation::ValidateColumnName(ym_monthStart, "Start Month", true, errorMsg);
			}

			if (valid)
			{
				valid = Validation::ValidateColumnName(ym_yearEnd, "End Year", false, errorMsg);
			}

			if (valid)
			{
				valid = Validation::ValidateColumnName(ym_monthEnd, "End Month", false, errorMsg);
			}

		}

		if (valid && using_string_fields)
		{
			dataTimeRange.push_back(ym_Start);

			if (!ym_End.empty())
			{
				dataTimeRange.push_back(ym_End);
			}
			else
			{
				warnEmptyEndingTimeCols = true;
			}
		}
		else if (valid)
		{
			dataTimeRange.push_back(ym_yearStart);
			dataTimeRange.push_back(ym_monthStart);

			if (!ym_yearEnd.empty() || !ym_monthEnd.empty())
			{
				dataTimeRange.push_back(ym_yearEnd);
				dataTimeRange.push_back(ym_monthEnd);
			}
			else
			{
				warnEmptyEndingTimeCols = true;
			}
		}

		if (valid)
		{
			std::for_each(dataTimeRange.begin(), dataTimeRange.end(), std::bind(boost::trim<std::string>, std::placeholders::_1, std::locale()));
			int nEmptyCols = 0;
			std::for_each(dataTimeRange.begin(), dataTimeRange.end(), [&](std::string & colname) { if (colname.empty()) { ++nEmptyCols; } });

			if (nEmptyCols > 0) { --nEmptyCols; }

			std::set<std::string> testtimerangecols(dataTimeRange.cbegin(), dataTimeRange.cend());

			if (testtimerangecols.size() != dataTimeRange.size() - nEmptyCols)
			{
				boost::format msg("Duplicate time range column headings");
				errorMsg = msg.str();
				return false;
			}
		}

		return valid;

	}

	boost::format msg("Invalid time range specification.");
	errorMsg = msg.str();

	return false;

}

void DialogHelper::AddUoaCreationBlock(QDialog & dialog, QFormLayout & form, QWidget & UoaConstructionWidget, QVBoxLayout & formOverall, QWidget & UoaConstructionPanes,
									   QHBoxLayout & formConstructionPanes, QVBoxLayout & formConstructionDivider, QListView *& lhs, QListView *& rhs, WidgetInstanceIdentifiers const & dmu_categories)
{

	QString labelTitle = QString("Define the DMU categories for the new unit of analysis:");
	QLabel * title = new QLabel(labelTitle, &dialog);

	lhs = new QListView(&UoaConstructionPanes);
	QWidget * middle = new QWidget(&UoaConstructionPanes);
	middle->setLayout(&formConstructionDivider);
	rhs = new QListView(&UoaConstructionPanes);

	QSpacerItem * middlespacetop = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed);
	QPushButton * add = new QPushButton(">>>", middle);
	QPushButton * remove = new QPushButton("<<<", middle);
	QSpacerItem * middlespacebottom = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed);
	formConstructionDivider.addItem(middlespacetop);
	formConstructionDivider.addWidget(add);
	formConstructionDivider.addWidget(remove);
	formConstructionDivider.addItem(middlespacebottom);

	formConstructionPanes.addWidget(lhs);
	formConstructionPanes.addWidget(middle);
	formConstructionPanes.addWidget(rhs);

	UoaConstructionPanes.setLayout(&formConstructionPanes);
	formOverall.addWidget(title);
	formOverall.addWidget(&UoaConstructionPanes);
	UoaConstructionWidget.setLayout(&formOverall);
	form.addRow(&UoaConstructionWidget);

	{

		QStandardItemModel * model = new QStandardItemModel(lhs);

		int index = 0;
		std::for_each(dmu_categories.cbegin(), dmu_categories.cend(), [&](WidgetInstanceIdentifier const & dmu_category)
		{
			if (dmu_category.uuid && !dmu_category.uuid->empty() && dmu_category.code && !dmu_category.code->empty())
			{

				QStandardItem * item = new QStandardItem();
				std::string text = Table_DMU_Identifier::GetDmuCategoryDisplayText(dmu_category);
				item->setText(text.c_str());
				item->setEditable(false);
				item->setCheckable(false);
				QVariant v;
				v.setValue(dmu_category);
				item->setData(v);
				model->setItem(index, item);

				++index;

			}
		});

		model->sort(0);

		lhs->setModel(model);


		QStandardItemModel * rhsModel = new QStandardItemModel(rhs);
		rhs->setModel(rhsModel);

	}

	{

		QObject::connect(add, &QPushButton::clicked, [&]()
		{

			QStandardItemModel * lhsModel = static_cast<QStandardItemModel *>(lhs->model());

			if (lhsModel == nullptr)
			{
				boost::format msg("Invalid list view items in Construct UOA popup.");
				QMessageBox msgBox;
				msgBox.setText(msg.str().c_str());
				msgBox.exec();
				return false;
			}

			QStandardItemModel * rhsModel = static_cast<QStandardItemModel *>(rhs->model());

			if (rhsModel == nullptr)
			{
				boost::format msg("Invalid rhs list view items in Construct UOA popup.");
				QMessageBox msgBox;
				msgBox.setText(msg.str().c_str());
				msgBox.exec();
				return false;
			}

			QItemSelectionModel * dmu_selectionModel = lhs->selectionModel();

			if (dmu_selectionModel == nullptr)
			{
				boost::format msg("Invalid selection in Create UOA widget.");
				QMessageBox msgBox;
				msgBox.setText(msg.str().c_str());
				msgBox.exec();
				return false;
			}

			QModelIndex selectedIndex = dmu_selectionModel->currentIndex();

			if (!selectedIndex.isValid())
			{
				// No selection
				return false;
			}

			QVariant dmu_category_variant = lhsModel->item(selectedIndex.row())->data();
			WidgetInstanceIdentifier dmu_category = dmu_category_variant.value<WidgetInstanceIdentifier>();

			std::string text = Table_DMU_Identifier::GetDmuCategoryDisplayText(dmu_category);
			QStandardItem * item = new QStandardItem();
			item->setText(text.c_str());
			item->setEditable(false);
			item->setCheckable(false);

			QVariant v;
			v.setValue(dmu_category);
			item->setData(v);
			rhsModel->appendRow(item);

			return true;

		});

		QObject::connect(remove, &QPushButton::clicked, [&]()
		{

			QStandardItemModel * rhsModel = static_cast<QStandardItemModel *>(rhs->model());

			if (rhsModel == nullptr)
			{
				boost::format msg("Invalid rhs list view items in Construct UOA popup.");
				QMessageBox msgBox;
				msgBox.setText(msg.str().c_str());
				msgBox.exec();
				return false;
			}

			QItemSelectionModel * dmu_selectionModel = rhs->selectionModel();

			if (dmu_selectionModel == nullptr)
			{
				boost::format msg("Invalid rhs selection in Create UOA widget.");
				QMessageBox msgBox;
				msgBox.setText(msg.str().c_str());
				msgBox.exec();
				return false;
			}

			QModelIndex selectedIndex = dmu_selectionModel->currentIndex();

			if (!selectedIndex.isValid())
			{
				return false;
			}

			//QVariant dmu_category_variant = rhsModel->item(selectedIndex.row())->data();
			//WidgetInstanceIdentifier dmu_category = dmu_category_variant.value<WidgetInstanceIdentifier>();

			//std::string text = Table_DMU_Identifier::GetDmuCategoryDisplayText(dmu_category);

			rhsModel->takeRow(selectedIndex.row());
			dmu_selectionModel->clearSelection();

			// TODO: memory leak here - must delete the QStandardItem at the row.
			// But this is so minor I haven't gotten to it yet.

			return true;

		});

		QObject::connect(lhs, &QListView::doubleClicked, [ = ](const QModelIndex & index)
		{
			add->click();
		});

		QObject::connect(rhs, &QListView::doubleClicked, [ = ](const QModelIndex & index)
		{
			remove->click();
		});

	}

}

void DialogHelper::AddVgCreationBlock(QDialog & dialog, QFormLayout & form, QWidget & VgConstructionWidget, QVBoxLayout & formOverall, QWidget & VgConstructionPanes,
									  QHBoxLayout & formConstructionPane, QListView *& listpane, WidgetInstanceIdentifiers const & uoas)
{

	QString labelTitle = QString("Choose the unit of analysis:");
	QLabel * title = new QLabel(labelTitle, &dialog);

	listpane = new QListView(&VgConstructionPanes);
	listpane->setMinimumWidth(400);
	listpane->setMinimumHeight(400);
	listpane->setAlternatingRowColors(true);
	listpane->setSelectionBehavior(QAbstractItemView::SelectRows);
	listpane->setResizeMode(QListView::Adjust);
	listpane->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
	listpane->setSpacing(5);
	listpane->setStyleSheet("QListView {selection-color: black;}");

	formConstructionPane.addWidget(listpane);

	VgConstructionPanes.setLayout(&formConstructionPane);
	formOverall.addWidget(title);
	formOverall.addWidget(&VgConstructionPanes);
	VgConstructionWidget.setLayout(&formOverall);
	form.addRow(&VgConstructionWidget);

	{

		QStandardItemModel * model = new QStandardItemModel(listpane);

		int index = 0;
		std::for_each(uoas.cbegin(), uoas.cend(), [&](WidgetInstanceIdentifier const & uoa)
		{
			if (uoa.uuid && !uoa.uuid->empty() && uoa.code && !uoa.code->empty())
			{

				if (!uoa.foreign_key_identifiers)
				{
					boost::format msg("Missing foreign_key_identifiers in UOA object.");
					throw NewGeneException() << newgene_error_description(msg.str());
				}

				QStandardItem * item = new QStandardItem();
				std::string text = Table_UOA_Identifier::GetUoaCategoryDisplayText(uoa, *uoa.foreign_key_identifiers, false, true);
				item->setText(text.c_str());
				item->setEditable(false);
				item->setCheckable(false);
				QVariant v;
				v.setValue(uoa);
				item->setData(v);
				model->setItem(index, item);

				++index;

			}
		});

		model->sort(0);

		listpane->setModel(model);
		listpane->setItemDelegate(new HtmlDelegate{});
	}

}

void DialogHelper::AddTopLevelVariableGroupChooserBlock(QDialog & dialog, QFormLayout & form, QWidget & VgConstructionWidget, QVBoxLayout & formOverall,
		QWidget & VgConstructionPanes, QHBoxLayout & formConstructionPane, QListView *& listpane, std::string const & dlgQuestion, std::vector<WidgetInstanceIdentifier> const & vg_list)
{

	QString labelQuestion = QString(dlgQuestion.c_str());
	QLabel * question = new QLabel(labelQuestion, &dialog);

	// The parent of the list view is a widget, not a layout
	listpane = new QListView(&VgConstructionPanes);
	listpane->setMinimumWidth(400);
	listpane->setMinimumHeight(400);
	listpane->setAlternatingRowColors(true);
	listpane->setSelectionBehavior(QAbstractItemView::SelectRows);
	listpane->setResizeMode(QListView::Adjust);
	listpane->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
	listpane->setSpacing(5);
	listpane->setStyleSheet("QListView {selection-color: black;}");

	// But the list view gets added to the layout, not to its parent widget
	formConstructionPane.addWidget(listpane);

	// Now we set the layout of the parent widget
	VgConstructionPanes.setLayout(&formConstructionPane);

	// Now we start adding elements to the overall (high-level) form layout
	formOverall.addWidget(question);

	// ... this includes adding the parent widget of the list view that has the layout (with the list view already added) as its layout
	formOverall.addWidget(&VgConstructionPanes);

	// The high-level form layout must be the layout of some widget, which is the high-level widget
	VgConstructionWidget.setLayout(&formOverall);

	// The entire dialog has a single form layout as its layout.  Add the high-level widget to the dialog's form layout.
	form.addRow(&VgConstructionWidget);

	{

		QStandardItemModel * model = new QStandardItemModel(listpane);

		int index = 0;
		std::for_each(vg_list.cbegin(), vg_list.cend(), [&](WidgetInstanceIdentifier const & vg)
		{

			QStandardItem * item = new QStandardItem();
			std::string text = Table_VG_CATEGORY::GetVgDisplayText(vg, true);
			item->setText(text.c_str());
			item->setEditable(false);
			item->setCheckable(false);
			QVariant v;
			v.setValue(vg);
			item->setData(v);
			model->setItem(index, item);

			++index;

		});

		model->sort(0);

		listpane->setModel(model);
		listpane->setItemDelegate(new HtmlDelegate{});

		listpane->clearSelection();

	}

}

// This and the above function could be templatized.  If a third is added, perhaps I will.
void DialogHelper::AddStringChooserBlock(QDialog & dialog, QFormLayout & form, QWidget & VgConstructionWidget, QVBoxLayout & formOverall,
		QWidget & VgConstructionPanes, QHBoxLayout & formConstructionPane, QListView *& listpane, std::string const & dlgQuestion, std::vector<std::string> const & string_list)
{

	// For now - while the only usage is for the 'append or overwrite output file' prompt -
	// in a later version we can add an enum for the sizing mode
	bool hardCodeForAppendOverwriteDlg = true;

	QString labelQuestion = QString(dlgQuestion.c_str());
	QLabel * question = new QLabel(labelQuestion, &dialog);

	if (hardCodeForAppendOverwriteDlg)
	{
		question->setMaximumWidth(300);
		question->setWordWrap(true);
	}

	// The parent of the list view is a widget, not a layout
	listpane = new QListView(&VgConstructionPanes);
	listpane->setMinimumWidth(300);
	listpane->setMinimumHeight(150);

	if (hardCodeForAppendOverwriteDlg)
	{
		listpane->setMaximumWidth(300);
		listpane->setMaximumHeight(150);
	}

	listpane->setAlternatingRowColors(true);
	listpane->setSelectionBehavior(QAbstractItemView::SelectRows);

	listpane->setResizeMode(QListView::Adjust);
	listpane->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

	listpane->setSpacing(5);
	listpane->setStyleSheet("QListView {selection-color: black;}");

	// But the list view gets added to the layout, not to its parent widget
	formConstructionPane.addWidget(listpane);

	// Now we set the layout of the parent widget
	VgConstructionPanes.setLayout(&formConstructionPane);

	// Now we start adding elements to the overall (high-level) form layout
	formOverall.addWidget(question);

	// ... this includes adding the parent widget of the list view that has the layout (with the list view already added) as its layout
	formOverall.addWidget(&VgConstructionPanes);

	// The high-level form layout must be the layout of some widget, which is the high-level widget
	VgConstructionWidget.setLayout(&formOverall);

	// The entire dialog has a single form layout as its layout.  Add the high-level widget to the dialog's form layout.
	form.addRow(&VgConstructionWidget);

	{

		QStandardItemModel * model = new QStandardItemModel(listpane);

		int index = 0;
		std::for_each(string_list.cbegin(), string_list.cend(), [&](std::string const & st)
		{

			QStandardItem * item = new QStandardItem();
			std::string text = st;
			item->setText(text.c_str());
			item->setEditable(false);
			item->setCheckable(false);
			QVariant v;
			v.setValue(st);
			item->setData(v);
			model->setItem(index, item);

			++index;

		});

		model->sort(0);

		listpane->setModel(model);
		listpane->setItemDelegate(new HtmlDelegate{true});

		listpane->clearSelection();

		if (hardCodeForAppendOverwriteDlg)
		{
			// Select first option ('append')
			QModelIndex itemIndex = model->index(0,0);
			listpane->setCurrentIndex(itemIndex);
		}

	}

}
