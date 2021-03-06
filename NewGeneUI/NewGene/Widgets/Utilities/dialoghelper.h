#ifndef IMPORTDIALOGHELPER_H
#define IMPORTDIALOGHELPER_H

#include "../../../../NewGeneBackEnd/Utilities/TimeRangeHelper.h"
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QRadioButton>
#include <QList>
#include <vector>
#include <string>
#include <QListView>
#include "../../../../NewGeneBackEnd/Utilities/WidgetIdentifier.h"

namespace DialogHelper
{

	void AddFileChooserBlock(QDialog & dialog, QFormLayout & form, QBoxLayout & formFileSelection, QWidget & FileChooserWidget, QList<QLineEdit *> & fieldsFileChooser,
							 std::vector<std::string> const & fileChooserStrings);
	bool ValidateFileChooserBlock(QList<QLineEdit *> & fieldsFileChooser, std::vector<std::string> & dataFileChooser, std::string & errorMsg);

	void AddTimeRangeGranularitySelectionBlock(QDialog & dialog, QFormLayout & form, QVBoxLayout & formTimeRangeGranularitySelection,
			QList<QRadioButton *> & radioButtonsTimeRangeGranularity);

	void AddTimeRangeSelectorBlock(

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

	);

	bool ValidateTimeRangeBlock(

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

	);

	void AddUoaCreationBlock(QDialog & dialog, QFormLayout & form, QWidget & UoaConstructionWidget, QVBoxLayout & formOverall, QWidget & UoaConstructionPanes,
							 QHBoxLayout & formConstructionPanes, QVBoxLayout & formConstructionDivider, QListView *& lhs, QListView *& rhs, WidgetInstanceIdentifiers const & dmu_categories);
	void AddVgCreationBlock(QDialog & dialog, QFormLayout & form, QWidget & VgConstructionWidget, QVBoxLayout & formOverall, QWidget & VgConstructionPanes,
							QHBoxLayout & formConstructionPane, QListView *& listpane, WidgetInstanceIdentifiers const & uoas);
	void AddTopLevelVariableGroupChooserBlock(QDialog & dialog, QFormLayout & form, QWidget & VgConstructionWidget, QVBoxLayout & formOverall, QWidget & VgConstructionPanes,
			QHBoxLayout & formConstructionPane, QListView *& listpane, std::string const & dlgQuestion, std::vector<WidgetInstanceIdentifier> const & vg_list);
	void AddStringChooserBlock(QDialog & dialog, QFormLayout & form, QWidget & VgConstructionWidget, QVBoxLayout & formOverall, QWidget & VgConstructionPanes,
			QHBoxLayout & formConstructionPane, QListView *& listpane, std::string const & dlgQuestion, std::vector<std::string> const & string_list);
}

#endif // IMPORTDIALOGHELPER_H
