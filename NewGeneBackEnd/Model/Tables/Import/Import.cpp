#include "Import.h"

#ifndef Q_MOC_RUN
	#include <boost/algorithm/string.hpp>
	#include <boost/tokenizer.hpp>
	#include <boost/scope_exit.hpp>
	#include <boost/locale/generator.hpp>
#endif
#include <fstream>
#include <cstdint>
#include "../Table.h"
#include "../../OutputModel.h"
#include "../../../Utilities/Validation.h"

// caches for time ranges
std::map<std::tuple<int, int, int, int, int, int>, std::pair<std::int64_t, std::int64_t>> TimeRangeFieldMapping::ymd_ymd_int_mappings;
std::map<std::tuple<int, int, int>, std::pair<std::int64_t, std::int64_t>>                TimeRangeFieldMapping::ymd_int_mappings;
std::map<std::tuple<int, int, int, int>, std::pair<std::int64_t, std::int64_t>>           TimeRangeFieldMapping::ym_ym_int_mappings;
std::map<std::pair<int, int>, std::pair<std::int64_t, std::int64_t>>                      TimeRangeFieldMapping::ym_int_mappings;
std::map<std::pair<int, int>, std::pair<std::int64_t, std::int64_t>>                      TimeRangeFieldMapping::y_y_int_mappings;
std::map<int, std::pair<std::int64_t, std::int64_t>>                                      TimeRangeFieldMapping::y_int_mappings;
std::map<std::pair<std::string, std::string>, std::pair<std::int64_t, std::int64_t>>      TimeRangeFieldMapping::ymd_ymd_string_mappings;
std::map<std::string, std::pair<std::int64_t, std::int64_t>>                              TimeRangeFieldMapping::ymd_string_mappings;
std::map<std::pair<std::string, std::string>, std::pair<std::int64_t, std::int64_t>>      TimeRangeFieldMapping::ym_ym_string_mappings;
std::map<std::string, std::pair<std::int64_t, std::int64_t>>                              TimeRangeFieldMapping::ym_string_mappings;
std::map<std::pair<std::string, std::string>, std::pair<std::int64_t, std::int64_t>>      TimeRangeFieldMapping::y_y_string_mappings;
std::map<std::string, std::pair<std::int64_t, std::int64_t>>                              TimeRangeFieldMapping::y_string_mappings;

std::recursive_mutex Importer::is_performing_import_mutex;
std::atomic<bool> Importer::is_performing_import(false);
bool Importer::cancelled = false;

int TimeRangeFieldMapping::ConvertStringToDateFancy(boost::posix_time::ptime & the_time, std::string const & the_string, int const index_to_use)
{

	int index_used = -1;

	static boost::locale::generator gen;
	static const std::locale inputs[] =
	{

		// ********************************************************************************************* //
		// year-month-day possibilities
		// ********************************************************************************************* //

		// numeric string for the month part; i.e. "11/12/1990"
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%Y/%m/%d")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%Y/%m/%d %H:%M:%S")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%Y\\%m\\%d")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%Y\\%m\\%d %H:%M:%S")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%Y-%m-%d")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%Y-%m-%d %H:%M:%S")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%m/%d/%Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%m/%d/%Y %H:%M:%S")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%m\\%d\\%Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%m\\%d\\%Y %H:%M:%S")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%m-%d-%Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%m-%d-%Y %H:%M:%S")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%Y%m%d")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%Y%m%d %H:%M:%S")),

		// text string abbreviation for the month part; i.e. "Feb 12, 1990"
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%b %d, %Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%b %d, %Y %H:%M:%S")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%b %d %Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%b %d %Y %H:%M:%S")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%b-%d-%Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%b-%d-%Y %H:%M:%S")),

		// full text string for for the month part; i.e. "February 12, 1990"
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%B %d, %Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%B %d, %Y %H:%M:%S")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%B %d %Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%B %d %Y %H:%M:%S")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%B-%d-%Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%B-%d-%Y %H:%M:%S")),


		// ********************************************************************************************* //
		// year-month possibilities
		// ********************************************************************************************* //

		// numeric string for the month part; i.e. "02/1990"
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%Y/%m")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%Y\\%m")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%Y-%m")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%m/%Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%m\\%Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%m-%Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%Y%m")),

		// text string abbreviation for the month part; i.e. "Feb, 1990"
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%b, %Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%b %Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%b-%Y")),

		// full text string for for the month part; i.e. "February, 1990"
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%B, %Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%B %Y")),
		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%B-%Y")),


		// ********************************************************************************************* //
		// year possibilities
		// ********************************************************************************************* //

		std::locale(gen("en_US.UTF-8"), new boost::posix_time::time_input_facet("%Y")),

	};

	static int number_formats = 0;

	if (index_to_use < 0)
	{
		const size_t formats = sizeof(inputs) / sizeof(inputs[0]);
		number_formats = static_cast<int>(formats);

		the_time = boost::posix_time::not_a_date_time;

		for (int i = 0; i < number_formats; ++i)
		{
			std::istringstream ss(the_string);
			ss.imbue(inputs[i]);
			boost::posix_time::ptime this_time;
			ss >> this_time;

			if (this_time != boost::posix_time::not_a_date_time)
			{
				the_time = this_time;
				index_used = static_cast<int>(i);
				break;
			}
		}
	}
	else if (index_to_use >= number_formats)
	{
		// Caller screwed up
		return index_used;
	}
	else
	{
		index_used = index_to_use;
		std::istringstream ss(the_string);
		ss.imbue(inputs[index_to_use]);
		boost::posix_time::ptime this_time;
		ss >> this_time;

		if (this_time != boost::posix_time::not_a_date_time)
		{
			the_time = this_time;
		}
		else
		{
			index_used = -1;
		}
	}

	return index_used;

}

void TimeRangeFieldMapping::PerformMapping(DataFields const & input_data_fields, DataFields const & output_data_fields)
{
	validTimeFields = true;

	try
	{
		switch (time_range_type)
		{

			case TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__INTS__YEAR__START_YEAR_ONLY:
				{

					std::shared_ptr<BaseField> const the_input_field = RetrieveDataField(input_file_fields[0], input_data_fields);
					std::shared_ptr<BaseField> the_output_field_year_start = RetrieveDataField(output_table_fields[0], output_data_fields);
					std::shared_ptr<BaseField> the_output_field_year_end = RetrieveDataField(output_table_fields[1], output_data_fields);

					if (!the_input_field || !the_output_field_year_start || !the_output_field_year_end || the_input_field->empty)
					{
						validTimeFields = false;
						return;
					}

					std::pair<std::int64_t, std::int64_t> & cached_times = y_int_mappings[the_input_field->GetInt32Ref()];

					if (cached_times.first != 0 && cached_times.second != 0)
					{
						the_output_field_year_start->SetValueInt64(cached_times.first);
						the_output_field_year_end->SetValueInt64(cached_times.second);
						break;
					}

					// convert year to ms since jan 1, 1970 00:00:00.000
					boost::posix_time::ptime time_t_epoch__1970(boost::gregorian::date(1970, 1, 1));
					boost::posix_time::ptime time_t_epoch__rowdatestart(boost::gregorian::date(the_input_field->GetInt32Ref(), 1, 1));
					boost::posix_time::ptime time_t_epoch__rowdateend(boost::gregorian::date(the_input_field->GetInt32Ref() + 1, 1, 1));

					boost::posix_time::time_duration diff_start_from_1970 = time_t_epoch__rowdatestart - time_t_epoch__1970;
					boost::posix_time::time_duration diff_end_from_1970 = time_t_epoch__rowdateend - time_t_epoch__1970;

					cached_times.first = diff_start_from_1970.total_milliseconds();
					cached_times.second = diff_end_from_1970.total_milliseconds();

					the_output_field_year_start->SetValueInt64(diff_start_from_1970.total_milliseconds());
					the_output_field_year_end->SetValueInt64(diff_end_from_1970.total_milliseconds());

				}
				break;

			case TIME_RANGE_FIELD_MAPPING_TYPE__INTS__YEAR__FROM__START_YEAR__TO__END_YEAR:
				{

					std::shared_ptr<BaseField> the_input_field_start = RetrieveDataField(input_file_fields[0], input_data_fields);
					std::shared_ptr<BaseField> the_input_field_end = RetrieveDataField(input_file_fields[1], input_data_fields);
					std::shared_ptr<BaseField> the_output_field_year_start = RetrieveDataField(output_table_fields[0], output_data_fields);
					std::shared_ptr<BaseField> the_output_field_year_end = RetrieveDataField(output_table_fields[1], output_data_fields);

					if (!the_input_field_start || !the_input_field_end || !the_output_field_year_start || !the_output_field_year_end || (the_input_field_start->empty && the_input_field_end->empty))
					{
						validTimeFields = false;
						return;
					}

					if (the_input_field_start->empty)
					{
						the_input_field_start->SetValueInt32(the_input_field_end->GetInt32Ref());
					}

					if (the_input_field_end->empty)
					{
						the_input_field_end->SetValueInt32(the_input_field_start->GetInt32Ref());
					}

					std::pair<std::int64_t, std::int64_t> & cached_times = y_y_int_mappings[std::make_pair(the_input_field_start->GetInt32Ref(), the_input_field_end->GetInt32Ref())];

					if (cached_times.first != 0 && cached_times.second != 0)
					{
						the_output_field_year_start->SetValueInt64(cached_times.first);
						the_output_field_year_end->SetValueInt64(cached_times.second);
						break;
					}

					// convert year to ms since jan 1, 1970 00:00:00.000
					boost::posix_time::ptime time_t_epoch__1970(boost::gregorian::date(1970, 1, 1));
					boost::posix_time::ptime time_t_epoch__rowdatestart(boost::gregorian::date(the_input_field_start->GetInt32Ref(), 1, 1));
					boost::posix_time::ptime time_t_epoch__rowdateend(boost::gregorian::date(the_input_field_end->GetInt32Ref() + 1, 1, 1));

					boost::posix_time::time_duration diff_start_from_1970 = time_t_epoch__rowdatestart - time_t_epoch__1970;
					boost::posix_time::time_duration diff_end_from_1970 = time_t_epoch__rowdateend - time_t_epoch__1970;

					cached_times.first = diff_start_from_1970.total_milliseconds();
					cached_times.second = diff_end_from_1970.total_milliseconds();

					the_output_field_year_start->SetValueInt64(diff_start_from_1970.total_milliseconds());
					the_output_field_year_end->SetValueInt64(diff_end_from_1970.total_milliseconds());

				}
				break;

			case TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__STRINGS__YEAR__START_YEAR_ONLY:
				{

					std::shared_ptr<BaseField> const the_input_field_datetime_year = RetrieveDataField(input_file_fields[0], input_data_fields);
					std::shared_ptr<BaseField> the_output_field_datetime_start = RetrieveDataField(output_table_fields[0], output_data_fields);
					std::shared_ptr<BaseField> the_output_field_datetime_end = RetrieveDataField(output_table_fields[1], output_data_fields);

					if (!the_input_field_datetime_year || !the_output_field_datetime_start || !the_output_field_datetime_end || the_input_field_datetime_year->empty)
					{
						validTimeFields = false;
						return;
					}

					std::pair<std::int64_t, std::int64_t> & cached_times = y_string_mappings[the_input_field_datetime_year->GetStringRef()];

					if (cached_times.first != 0 && cached_times.second != 0)
					{
						the_output_field_datetime_start->SetValueInt64(cached_times.first);
						the_output_field_datetime_end->SetValueInt64(cached_times.second);
						break;
					}

					boost::posix_time::ptime time_t_epoch__1970(boost::gregorian::date(1970, 1, 1));

					int conversion_index = the_input_field_datetime_year->GetDateFormatIndex();

					boost::posix_time::ptime the_year_start;
					conversion_index = ConvertStringToDateFancy(the_year_start, the_input_field_datetime_year->GetStringRef(), conversion_index);

					the_input_field_datetime_year->SetDateFormatIndex(conversion_index);

					if (conversion_index < 0)
					{
						validTimeFields = false;
						return;
					}

					// Round down to year
					the_year_start = boost::posix_time::ptime(boost::gregorian::date(the_year_start.date().year(), boost::gregorian::Jan, 1));

					boost::posix_time::ptime the_year_end = boost::posix_time::ptime(boost::gregorian::date(the_year_start.date().year(), boost::gregorian::Jan, 1) + boost::gregorian::years(1));

					boost::posix_time::time_duration diff_start_from_1970 = the_year_start - time_t_epoch__1970;
					boost::posix_time::time_duration diff_end_from_1970 = the_year_end - time_t_epoch__1970;

					cached_times.first = diff_start_from_1970.total_milliseconds();
					cached_times.second = diff_end_from_1970.total_milliseconds();

					the_output_field_datetime_start->SetValueInt64(diff_start_from_1970.total_milliseconds());
					the_output_field_datetime_end->SetValueInt64(diff_end_from_1970.total_milliseconds());

				}
				break;

			case TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__STRINGS__YEAR__FROM__START_YEAR__TO__END_YEAR:
				{

					std::shared_ptr<BaseField> the_input_field_datetime_start = RetrieveDataField(input_file_fields[0], input_data_fields);
					std::shared_ptr<BaseField> the_input_field_datetime_end = RetrieveDataField(input_file_fields[1], input_data_fields);
					std::shared_ptr<BaseField> the_output_field_datetime_start = RetrieveDataField(output_table_fields[0], output_data_fields);
					std::shared_ptr<BaseField> the_output_field_datetime_end = RetrieveDataField(output_table_fields[1], output_data_fields);

					if (!the_input_field_datetime_start || !the_input_field_datetime_end || !the_output_field_datetime_start || !the_output_field_datetime_end
						|| (the_input_field_datetime_start->empty && the_input_field_datetime_end->empty))
					{
						validTimeFields = false;
						return;
					}

					if (the_input_field_datetime_start->empty)
					{
						the_input_field_datetime_start->SetValueString(the_input_field_datetime_end->GetStringRef());
					}

					if (the_input_field_datetime_end->empty)
					{
						the_input_field_datetime_end->SetValueString(the_input_field_datetime_start->GetStringRef());
					}

					std::pair<std::int64_t, std::int64_t> & cached_times = y_y_string_mappings[std::make_pair(the_input_field_datetime_start->GetStringRef(),
							the_input_field_datetime_end->GetStringRef())];

					if (cached_times.first != 0 && cached_times.second != 0)
					{
						the_output_field_datetime_start->SetValueInt64(cached_times.first);
						the_output_field_datetime_end->SetValueInt64(cached_times.second);
						break;
					}

					boost::posix_time::ptime time_t_epoch__1970(boost::gregorian::date(1970, 1, 1));

					int conversion_index = the_input_field_datetime_start->GetDateFormatIndex();

					boost::posix_time::ptime the_time_start;
					conversion_index = ConvertStringToDateFancy(the_time_start, the_input_field_datetime_start->GetStringRef(), conversion_index);

					the_input_field_datetime_start->SetDateFormatIndex(conversion_index);

					if (conversion_index < 0)
					{
						validTimeFields = false;
						return;
					}

					// Round down to year
					the_time_start = boost::posix_time::ptime(boost::gregorian::date(the_time_start.date().year(), boost::gregorian::Jan, 1));

					boost::posix_time::ptime the_time_end;
					conversion_index = ConvertStringToDateFancy(the_time_end, the_input_field_datetime_end->GetStringRef(), conversion_index);

					if (conversion_index < 0)
					{
						validTimeFields = false;
						return;
					}

					// Round down to year
					// Then add 1 year so that the full year is included (days start at midnight, so no seconds of the next year are included)
					the_time_end = boost::posix_time::ptime(boost::gregorian::date(the_time_end.date().year(), boost::gregorian::Jan, 1) + boost::gregorian::years(1));

					boost::posix_time::time_duration diff_start_from_1970 = the_time_start - time_t_epoch__1970;
					boost::posix_time::time_duration diff_end_from_1970 = the_time_end - time_t_epoch__1970;

					cached_times.first = diff_start_from_1970.total_milliseconds();
					cached_times.second = diff_end_from_1970.total_milliseconds();

					the_output_field_datetime_start->SetValueInt64(diff_start_from_1970.total_milliseconds());
					the_output_field_datetime_end->SetValueInt64(diff_end_from_1970.total_milliseconds());

				}
				break;

			case TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__INTS__MONTH__START_MONTH_ONLY:
				{

					std::shared_ptr<BaseField> const the_input_field_year_start = RetrieveDataField(input_file_fields[0], input_data_fields);
					std::shared_ptr<BaseField> const the_input_field_month_start = RetrieveDataField(input_file_fields[1], input_data_fields);
					std::shared_ptr<BaseField> the_output_field_month_start = RetrieveDataField(output_table_fields[0], output_data_fields);
					std::shared_ptr<BaseField> the_output_field_month_end = RetrieveDataField(output_table_fields[1], output_data_fields);

					if (!the_input_field_month_start || !the_input_field_year_start || !the_output_field_month_start || !the_output_field_month_end || (the_input_field_year_start->empty
							|| the_input_field_month_start->empty))
					{
						validTimeFields = false;
						return;
					}

					std::pair<std::int64_t, std::int64_t> & cached_times = ym_int_mappings[std::make_pair(the_input_field_year_start->GetInt32Ref(), the_input_field_month_start->GetInt32Ref())];

					if (cached_times.first != 0 && cached_times.second != 0)
					{
						the_output_field_month_start->SetValueInt64(cached_times.first);
						the_output_field_month_end->SetValueInt64(cached_times.second);
						break;
					}

					int year_start = the_input_field_year_start->GetInt32Ref();
					int month_start = the_input_field_month_start->GetInt32Ref();

					if (month_start > 12)
					{
						month_start = 1;
					}

					bool start_month_valid = month_start > 0;

					if (!start_month_valid)
					{
						month_start = 1;
					}

					int year_end = year_start;
					int month_end = month_start + 1;

					if (month_end > 12)
					{
						month_end = 1;
						++year_end;
					}

					// convert year to ms since jan 1, 1970 00:00:00.000
					boost::posix_time::ptime time_t_epoch__1970(boost::gregorian::date(1970, 1, 1));
					boost::posix_time::ptime time_t_epoch__rowdatestart(boost::gregorian::date(year_start, month_start, 1));
					boost::posix_time::ptime time_t_epoch__rowdateend(boost::gregorian::date(year_end, month_end, 1));

					boost::posix_time::time_duration diff_start_from_1970 = time_t_epoch__rowdatestart - time_t_epoch__1970;
					boost::posix_time::time_duration diff_end_from_1970 = time_t_epoch__rowdateend - time_t_epoch__1970;

					cached_times.first = diff_start_from_1970.total_milliseconds();
					cached_times.second = diff_end_from_1970.total_milliseconds();

					the_output_field_month_start->SetValueInt64(diff_start_from_1970.total_milliseconds());
					the_output_field_month_end->SetValueInt64(diff_end_from_1970.total_milliseconds());

				}
				break;

			case TIME_RANGE_FIELD_MAPPING_TYPE__INTS__MONTH__FROM__START_MONTH__TO__END_MONTH:
				{

					std::shared_ptr<BaseField> the_input_field_year_start = RetrieveDataField(input_file_fields[0], input_data_fields);
					std::shared_ptr<BaseField> the_input_field_month_start = RetrieveDataField(input_file_fields[1], input_data_fields);
					std::shared_ptr<BaseField> the_input_field_year_end = RetrieveDataField(input_file_fields[2], input_data_fields);
					std::shared_ptr<BaseField> the_input_field_month_end = RetrieveDataField(input_file_fields[3], input_data_fields);
					std::shared_ptr<BaseField> the_output_field_day_start = RetrieveDataField(output_table_fields[0], output_data_fields);
					std::shared_ptr<BaseField> the_output_field_day_end = RetrieveDataField(output_table_fields[1], output_data_fields);

					bool emptyStart = (the_input_field_year_start->empty || the_input_field_month_start->empty);
					bool emptyEnd = (the_input_field_year_end->empty || the_input_field_month_end->empty);

					if (!the_input_field_month_start || !the_input_field_year_start || !the_input_field_month_end
						|| !the_input_field_year_end || !the_output_field_day_start || !the_output_field_day_end || (emptyStart && emptyEnd))
					{
						validTimeFields = false;
						return;
					}

					if (emptyStart)
					{
						the_input_field_year_start->SetValueInt32(the_input_field_year_end->GetInt32Ref());
						the_input_field_month_start->SetValueInt32(the_input_field_month_end->GetInt32Ref());
					}

					if (emptyEnd)
					{
						the_input_field_year_end->SetValueInt32(the_input_field_year_start->GetInt32Ref());
						the_input_field_month_end->SetValueInt32(the_input_field_month_start->GetInt32Ref());
					}

					std::pair<std::int64_t, std::int64_t> & cached_times = ym_ym_int_mappings[std::tuple<int, int, int, int>(the_input_field_year_start->GetInt32Ref(),
							the_input_field_month_start->GetInt32Ref(), the_input_field_year_end->GetInt32Ref(), the_input_field_month_end->GetInt32Ref())];

					if (cached_times.first != 0 && cached_times.second != 0)
					{
						the_output_field_day_start->SetValueInt64(cached_times.first);
						the_output_field_day_end->SetValueInt64(cached_times.second);
						break;
					}

					// convert year to ms since jan 1, 1970 00:00:00.000
					boost::posix_time::ptime time_t_epoch__1970(boost::gregorian::date(1970, 1, 1));


					int year_start = the_input_field_year_start->GetInt32Ref();
					int year_end = the_input_field_year_end->GetInt32Ref();
					int month_start = the_input_field_month_start->GetInt32Ref();
					int month_end = the_input_field_month_end->GetInt32Ref();

					if (month_start > 12)
					{
						month_start = 1;
					}

					if (month_end > 12)
					{
						month_end = 1;
					}

					bool start_month_valid = month_start > 0;
					bool end_month_valid = month_end > 0;

					if (!start_month_valid)
					{
						month_start = 1;
					}

					if (!end_month_valid)
					{
						month_end = month_start + 1;
					}
					else
					{
						month_end += 1; // Really, go to 1st of month AFTER the end month (so the whole month is covered)
					}

					if (month_end > 12)
					{
						month_end = 1;
						year_end += 1;
					}

					boost::gregorian::date row_start_date(year_start, month_start, 1);
					boost::posix_time::ptime time_t_epoch__rowdatestart(row_start_date);

					boost::gregorian::date row_end_date(year_end, month_end, 1);

					boost::posix_time::ptime time_t_epoch__rowdateend(row_end_date);

					boost::posix_time::time_duration diff_start_from_1970 = time_t_epoch__rowdatestart - time_t_epoch__1970;
					boost::posix_time::time_duration diff_end_from_1970 = time_t_epoch__rowdateend - time_t_epoch__1970;

					cached_times.first = diff_start_from_1970.total_milliseconds();
					cached_times.second = diff_end_from_1970.total_milliseconds();

					the_output_field_day_start->SetValueInt64(diff_start_from_1970.total_milliseconds());
					the_output_field_day_end->SetValueInt64(diff_end_from_1970.total_milliseconds());

				}
				break;

			case TIME_RANGE_FIELD_MAPPING_TYPE__STRINGS__MONTH__START_MONTH_ONLY:
				{

					std::shared_ptr<BaseField> const the_input_field_datetime_month = RetrieveDataField(input_file_fields[0], input_data_fields);
					std::shared_ptr<BaseField> the_output_field_datetime_start = RetrieveDataField(output_table_fields[0], output_data_fields);
					std::shared_ptr<BaseField> the_output_field_datetime_end = RetrieveDataField(output_table_fields[1], output_data_fields);

					if (!the_input_field_datetime_month || !the_output_field_datetime_start || !the_output_field_datetime_end || the_input_field_datetime_month->empty)
					{
						validTimeFields = false;
						return;
					}

					std::pair<std::int64_t, std::int64_t> & cached_times = ym_string_mappings[the_input_field_datetime_month->GetStringRef()];

					if (cached_times.first != 0 && cached_times.second != 0)
					{
						the_output_field_datetime_start->SetValueInt64(cached_times.first);
						the_output_field_datetime_end->SetValueInt64(cached_times.second);
						break;
					}

					boost::posix_time::ptime time_t_epoch__1970(boost::gregorian::date(1970, 1, 1));

					int conversion_index = the_input_field_datetime_month->GetDateFormatIndex();

					boost::posix_time::ptime the_month_start;
					conversion_index = ConvertStringToDateFancy(the_month_start, the_input_field_datetime_month->GetStringRef(), conversion_index);

					the_input_field_datetime_month->SetDateFormatIndex(conversion_index);

					if (conversion_index < 0)
					{
						validTimeFields = false;
						return;
					}

					// Round down to month
					the_month_start = boost::posix_time::ptime(boost::gregorian::date(the_month_start.date().year(), the_month_start.date().month(), 1));

					boost::posix_time::ptime the_month_end = boost::posix_time::ptime(boost::gregorian::date(the_month_start.date().year(), the_month_start.date().month(),
							1) + boost::gregorian::months(1));

					boost::posix_time::time_duration diff_start_from_1970 = the_month_start - time_t_epoch__1970;
					boost::posix_time::time_duration diff_end_from_1970 = the_month_end - time_t_epoch__1970;

					cached_times.first = diff_start_from_1970.total_milliseconds();
					cached_times.second = diff_end_from_1970.total_milliseconds();

					the_output_field_datetime_start->SetValueInt64(diff_start_from_1970.total_milliseconds());
					the_output_field_datetime_end->SetValueInt64(diff_end_from_1970.total_milliseconds());

				}
				break;

			case TIME_RANGE_FIELD_MAPPING_TYPE__STRINGS__MONTH__FROM__START_MONTH__TO__END_MONTH:
				{

					std::shared_ptr<BaseField> the_input_field_datetime_start = RetrieveDataField(input_file_fields[0], input_data_fields);
					std::shared_ptr<BaseField> the_input_field_datetime_end = RetrieveDataField(input_file_fields[1], input_data_fields);
					std::shared_ptr<BaseField> the_output_field_datetime_start = RetrieveDataField(output_table_fields[0], output_data_fields);
					std::shared_ptr<BaseField> the_output_field_datetime_end = RetrieveDataField(output_table_fields[1], output_data_fields);

					if (!the_input_field_datetime_start || !the_input_field_datetime_end || !the_output_field_datetime_start || !the_output_field_datetime_end
						|| (the_input_field_datetime_start->empty && the_input_field_datetime_end->empty))
					{
						validTimeFields = false;
						return;
					}

					if (the_input_field_datetime_start->empty)
					{
						the_input_field_datetime_start->SetValueString(the_input_field_datetime_end->GetStringRef());
					}

					if (the_input_field_datetime_end->empty)
					{
						the_input_field_datetime_end->SetValueString(the_input_field_datetime_start->GetStringRef());
					}

					std::pair<std::int64_t, std::int64_t> & cached_times = ym_ym_string_mappings[std::make_pair(the_input_field_datetime_start->GetStringRef(),
							the_input_field_datetime_end->GetStringRef())];

					if (cached_times.first != 0 && cached_times.second != 0)
					{
						the_output_field_datetime_start->SetValueInt64(cached_times.first);
						the_output_field_datetime_end->SetValueInt64(cached_times.second);
						break;
					}

					boost::posix_time::ptime time_t_epoch__1970(boost::gregorian::date(1970, 1, 1));

					int conversion_index = the_input_field_datetime_start->GetDateFormatIndex();

					boost::posix_time::ptime the_time_start;
					conversion_index = ConvertStringToDateFancy(the_time_start, the_input_field_datetime_start->GetStringRef(), conversion_index);

					the_input_field_datetime_start->SetDateFormatIndex(conversion_index);

					if (conversion_index < 0)
					{
						validTimeFields = false;
						return;
					}

					// Round down to month
					the_time_start = boost::posix_time::ptime(boost::gregorian::date(the_time_start.date().year(), the_time_start.date().month(), 1));

					boost::posix_time::ptime the_time_end;
					conversion_index = ConvertStringToDateFancy(the_time_end, the_input_field_datetime_end->GetStringRef(), conversion_index);

					if (conversion_index < 0)
					{
						validTimeFields = false;
						return;
					}

					// Round down to month
					// Then add 1 month so that the full year is included (days start at midnight, so no seconds of the next year are included)
					the_time_end = boost::posix_time::ptime(boost::gregorian::date(the_time_end.date().year(), the_time_end.date().month(), 1) + boost::gregorian::months(1));

					boost::posix_time::time_duration diff_start_from_1970 = the_time_start - time_t_epoch__1970;
					boost::posix_time::time_duration diff_end_from_1970 = the_time_end - time_t_epoch__1970;

					cached_times.first = diff_start_from_1970.total_milliseconds();
					cached_times.second = diff_end_from_1970.total_milliseconds();

					the_output_field_datetime_start->SetValueInt64(diff_start_from_1970.total_milliseconds());
					the_output_field_datetime_end->SetValueInt64(diff_end_from_1970.total_milliseconds());

				}
				break;

			case TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__INTS__DAY__START_DAY_ONLY:
				{

					std::shared_ptr<BaseField> const the_input_field_year_start = RetrieveDataField(input_file_fields[0], input_data_fields);
					std::shared_ptr<BaseField> const the_input_field_month_start = RetrieveDataField(input_file_fields[1], input_data_fields);
					std::shared_ptr<BaseField> const the_input_field_day_start = RetrieveDataField(input_file_fields[2], input_data_fields);
					std::shared_ptr<BaseField> the_output_field_day_start = RetrieveDataField(output_table_fields[0], output_data_fields);
					std::shared_ptr<BaseField> the_output_field_day_end = RetrieveDataField(output_table_fields[1], output_data_fields);

					if (!the_input_field_day_start || !the_input_field_month_start || !the_input_field_year_start || !the_output_field_day_start || !the_output_field_day_end
						|| (the_input_field_year_start->empty || the_input_field_month_start->empty || the_input_field_day_start->empty))
					{
						validTimeFields = false;
						return;
					}

					std::pair<std::int64_t, std::int64_t> & cached_times = ymd_int_mappings[std::tuple<int, int, int>(the_input_field_year_start->GetInt32Ref(),
							the_input_field_month_start->GetInt32Ref(), the_input_field_day_start->GetInt32Ref())];

					if (cached_times.first != 0 && cached_times.second != 0)
					{
						the_output_field_day_start->SetValueInt64(cached_times.first);
						the_output_field_day_end->SetValueInt64(cached_times.second);
						break;
					}

					int year_start = the_input_field_year_start->GetInt32Ref();
					int month_start = the_input_field_month_start->GetInt32Ref();
					int day_start = the_input_field_day_start->GetInt32Ref();

					if (month_start > 12 || month_start < 1)
					{
						month_start = 1;
					}

					// convert year to ms since jan 1, 1970 00:00:00.000
					boost::posix_time::ptime time_t_epoch__1970(boost::gregorian::date(1970, 1, 1));
					boost::posix_time::ptime time_t_epoch__rowdatestart(boost::gregorian::date(year_start, month_start, day_start));
					boost::posix_time::ptime time_t_epoch__rowdateend(boost::gregorian::date(year_start, month_start, day_start) + boost::gregorian::days(1));

					boost::posix_time::time_duration diff_start_from_1970 = time_t_epoch__rowdatestart - time_t_epoch__1970;
					boost::posix_time::time_duration diff_end_from_1970 = time_t_epoch__rowdateend - time_t_epoch__1970;

					cached_times.first = diff_start_from_1970.total_milliseconds();
					cached_times.second = diff_end_from_1970.total_milliseconds();

					the_output_field_day_start->SetValueInt64(diff_start_from_1970.total_milliseconds());
					the_output_field_day_end->SetValueInt64(diff_end_from_1970.total_milliseconds());

				}
				break;

			case TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__INTS__DAY__FROM__START_DAY__TO__END_DAY:
				{

					std::shared_ptr<BaseField> the_input_field_year_start = RetrieveDataField(input_file_fields[0], input_data_fields);
					std::shared_ptr<BaseField> the_input_field_month_start = RetrieveDataField(input_file_fields[1], input_data_fields);
					std::shared_ptr<BaseField> the_input_field_day_start = RetrieveDataField(input_file_fields[2], input_data_fields);
					std::shared_ptr<BaseField> the_input_field_year_end = RetrieveDataField(input_file_fields[3], input_data_fields);
					std::shared_ptr<BaseField> the_input_field_month_end = RetrieveDataField(input_file_fields[4], input_data_fields);
					std::shared_ptr<BaseField> the_input_field_day_end = RetrieveDataField(input_file_fields[5], input_data_fields);
					std::shared_ptr<BaseField> the_output_field_day_start = RetrieveDataField(output_table_fields[0], output_data_fields);
					std::shared_ptr<BaseField> the_output_field_day_end = RetrieveDataField(output_table_fields[1], output_data_fields);

					bool emptyStart = (the_input_field_year_start->empty || the_input_field_month_start->empty || the_input_field_day_start->empty);
					bool emptyEnd = (the_input_field_year_end->empty || the_input_field_month_end->empty || the_input_field_day_end->empty);

					if (!the_input_field_day_start || !the_input_field_month_start || !the_input_field_year_start || !the_input_field_day_end || !the_input_field_month_end
						|| !the_input_field_year_end || !the_output_field_day_start || !the_output_field_day_end || (emptyStart && emptyEnd))
					{
						validTimeFields = false;
						return;
					}

					if (emptyStart)
					{
						the_input_field_year_start->SetValueInt32(the_input_field_year_end->GetInt32Ref());
						the_input_field_month_start->SetValueInt32(the_input_field_month_end->GetInt32Ref());
						the_input_field_day_start->SetValueInt32(the_input_field_day_end->GetInt32Ref());
					}

					if (emptyEnd)
					{
						the_input_field_year_end->SetValueInt32(the_input_field_year_start->GetInt32Ref());
						the_input_field_month_end->SetValueInt32(the_input_field_month_start->GetInt32Ref());
						the_input_field_day_end->SetValueInt32(the_input_field_day_start->GetInt32Ref());
					}

					std::pair<std::int64_t, std::int64_t> & cached_times = ymd_ymd_int_mappings[std::tuple<int, int, int, int, int, int>(the_input_field_year_start->GetInt32Ref(),
							the_input_field_month_start->GetInt32Ref(), the_input_field_day_start->GetInt32Ref(), the_input_field_year_end->GetInt32Ref(), the_input_field_month_end->GetInt32Ref(),
							the_input_field_day_end->GetInt32Ref())];

					if (cached_times.first != 0 && cached_times.second != 0)
					{
						the_output_field_day_start->SetValueInt64(cached_times.first);
						the_output_field_day_end->SetValueInt64(cached_times.second);
						break;
					}

					// convert year to ms since jan 1, 1970 00:00:00.000
					boost::posix_time::ptime time_t_epoch__1970(boost::gregorian::date(1970, 1, 1));


					int year_start = the_input_field_year_start->GetInt32Ref();
					int year_end = the_input_field_year_end->GetInt32Ref();
					int month_start = the_input_field_month_start->GetInt32Ref();
					int month_end = the_input_field_month_end->GetInt32Ref();
					int day_start = the_input_field_day_start->GetInt32Ref();
					int day_end = the_input_field_day_end->GetInt32Ref();

					bool start_month_valid = (month_start > 0 && month_start <= 12);
					bool end_month_valid = (month_end > 0 && month_end <= 12);
					bool start_day_valid = (day_start > 0 && day_start <= 31);
					bool end_day_valid = (day_end > 0 && day_end <= 31);

					if (!start_day_valid && !start_month_valid)
					{
						month_start = 1;
						day_start = 1;
					}
					else if (start_day_valid && !start_month_valid)
					{
						month_start = 1;
					}
					else if (!start_day_valid && start_month_valid)
					{
						day_start = 1;
					}

					// add a day to the end? (since days start at the very end of the previous day - to give the specified day a full 24 hours)
					bool end_handled = false;

					bool add_month = false; // See comment below

					if (!end_day_valid && !end_month_valid)
					{
						year_end += 1;
						month_end = 1;
						day_end = 1;

						// do not add a day to the end; we've already handled it
						end_handled = true;
					}
					else if (end_day_valid && !end_month_valid)
					{
						month_end = 12;
					}
					else if (!end_day_valid && end_month_valid)
					{
						// trick: start at beginning of the known month, and then add a month
						day_end = 1;
						add_month = true;
						end_handled = true; // The end is handled by setting add_month
					}

					boost::gregorian::date row_start_date(year_start, month_start, day_start);
					boost::posix_time::ptime time_t_epoch__rowdatestart(row_start_date);

					boost::gregorian::date row_end_date(year_end, month_end, day_end);

					if (add_month)
					{
						row_end_date += boost::gregorian::months(1);
					}

					if (!end_handled)
					{
						row_end_date += boost::gregorian::days(1);
					}

					boost::posix_time::ptime time_t_epoch__rowdateend(row_end_date);

					boost::posix_time::time_duration diff_start_from_1970 = time_t_epoch__rowdatestart - time_t_epoch__1970;
					boost::posix_time::time_duration diff_end_from_1970 = time_t_epoch__rowdateend - time_t_epoch__1970;

					cached_times.first = diff_start_from_1970.total_milliseconds();
					cached_times.second = diff_end_from_1970.total_milliseconds();

					the_output_field_day_start->SetValueInt64(diff_start_from_1970.total_milliseconds());
					the_output_field_day_end->SetValueInt64(diff_end_from_1970.total_milliseconds());

				}
				break;

			case TIME_RANGE_FIELD_MAPPING_TYPE__STRINGS__DAY__START_DAY_ONLY:
				{

					std::shared_ptr<BaseField> const the_input_field_datetime_day = RetrieveDataField(input_file_fields[0], input_data_fields);
					std::shared_ptr<BaseField> the_output_field_datetime_start = RetrieveDataField(output_table_fields[0], output_data_fields);
					std::shared_ptr<BaseField> the_output_field_datetime_end = RetrieveDataField(output_table_fields[1], output_data_fields);

					if (!the_input_field_datetime_day || !the_output_field_datetime_start || !the_output_field_datetime_end || the_input_field_datetime_day->empty)
					{
						validTimeFields = false;
						return;
					}

					std::pair<std::int64_t, std::int64_t> & cached_times = ymd_string_mappings[the_input_field_datetime_day->GetStringRef()];

					if (cached_times.first != 0 && cached_times.second != 0)
					{
						the_output_field_datetime_start->SetValueInt64(cached_times.first);
						the_output_field_datetime_end->SetValueInt64(cached_times.second);
						break;
					}

					boost::posix_time::ptime time_t_epoch__1970(boost::gregorian::date(1970, 1, 1));

					int conversion_index = the_input_field_datetime_day->GetDateFormatIndex();

					boost::posix_time::ptime the_day_start;
					conversion_index = ConvertStringToDateFancy(the_day_start, the_input_field_datetime_day->GetStringRef(), conversion_index);

					the_input_field_datetime_day->SetDateFormatIndex(conversion_index);

					if (conversion_index < 0)
					{
						validTimeFields = false;
						return;
					}

					// Round down to month
					the_day_start = boost::posix_time::ptime(boost::gregorian::date(the_day_start.date().year(), the_day_start.date().month(), the_day_start.date().day()));

					boost::posix_time::ptime the_day_end = boost::posix_time::ptime(boost::gregorian::date(the_day_start.date().year(), the_day_start.date().month(),
														   the_day_start.date().day()) + boost::gregorian::days(1));

					boost::posix_time::time_duration diff_start_from_1970 = the_day_start - time_t_epoch__1970;
					boost::posix_time::time_duration diff_end_from_1970 = the_day_end - time_t_epoch__1970;

					cached_times.first = diff_start_from_1970.total_milliseconds();
					cached_times.second = diff_end_from_1970.total_milliseconds();

					the_output_field_datetime_start->SetValueInt64(diff_start_from_1970.total_milliseconds());
					the_output_field_datetime_end->SetValueInt64(diff_end_from_1970.total_milliseconds());

				}
				break;

			case TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__STRINGS__DAY__FROM__START_DAY__TO__END_DAY:
				{

					std::shared_ptr<BaseField> the_input_field_datetime_start = RetrieveDataField(input_file_fields[0], input_data_fields);
					std::shared_ptr<BaseField> the_input_field_datetime_end = RetrieveDataField(input_file_fields[1], input_data_fields);
					std::shared_ptr<BaseField> the_output_field_datetime_start = RetrieveDataField(output_table_fields[0], output_data_fields);
					std::shared_ptr<BaseField> the_output_field_datetime_end = RetrieveDataField(output_table_fields[1], output_data_fields);

					if (!the_input_field_datetime_start || !the_input_field_datetime_end || !the_output_field_datetime_start || !the_output_field_datetime_end
						|| (the_input_field_datetime_start->empty && the_input_field_datetime_end->empty))
					{
						validTimeFields = false;
						return;
					}

					if (the_input_field_datetime_start->empty)
					{
						the_input_field_datetime_start->SetValueString(the_input_field_datetime_end->GetStringRef());
					}

					if (the_input_field_datetime_end->empty)
					{
						the_input_field_datetime_end->SetValueString(the_input_field_datetime_start->GetStringRef());
					}

					std::pair<std::int64_t, std::int64_t> & cached_times = ymd_ymd_string_mappings[std::make_pair(the_input_field_datetime_start->GetStringRef(),
							the_input_field_datetime_end->GetStringRef())];

					if (cached_times.first != 0 && cached_times.second != 0)
					{
						the_output_field_datetime_start->SetValueInt64(cached_times.first);
						the_output_field_datetime_end->SetValueInt64(cached_times.second);
						break;
					}

					boost::posix_time::ptime time_t_epoch__1970(boost::gregorian::date(1970, 1, 1));

					int conversion_index = the_input_field_datetime_start->GetDateFormatIndex();

					boost::posix_time::ptime the_time_start;
					conversion_index = ConvertStringToDateFancy(the_time_start, the_input_field_datetime_start->GetStringRef(), conversion_index);

					the_input_field_datetime_start->SetDateFormatIndex(conversion_index);

					if (conversion_index < 0)
					{
						validTimeFields = false;
						return;
					}

					// Round down to day
					the_time_start = boost::posix_time::ptime(the_time_start.date());

					boost::posix_time::ptime the_time_end;
					conversion_index = ConvertStringToDateFancy(the_time_end, the_input_field_datetime_end->GetStringRef(), conversion_index);

					if (conversion_index < 0)
					{
						validTimeFields = false;
						return;
					}

					// Round down to day
					// Then add 1 day so that the full day is included (days start at midnight, so no seconds of the next day are included)
					the_time_end = boost::posix_time::ptime(the_time_end.date() + boost::gregorian::days(1));

					boost::posix_time::time_duration diff_start_from_1970 = the_time_start - time_t_epoch__1970;
					boost::posix_time::time_duration diff_end_from_1970 = the_time_end - time_t_epoch__1970;

					cached_times.first = diff_start_from_1970.total_milliseconds();
					cached_times.second = diff_end_from_1970.total_milliseconds();

					the_output_field_datetime_start->SetValueInt64(diff_start_from_1970.total_milliseconds());
					the_output_field_datetime_end->SetValueInt64(diff_end_from_1970.total_milliseconds());

				}
				break;

			// **************************************************************************************************************** //
			// The following is deprecated and unused
			// **************************************************************************************************************** //

			case TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__STRING_RANGE:
				{

					// **************************************************************************************************************** //
					// Deprecated and unused
					// **************************************************************************************************************** //

					std::shared_ptr<BaseField> the_input_field_datetime_start = RetrieveDataField(input_file_fields[0], input_data_fields);
					std::shared_ptr<BaseField> the_input_field_datetime_end = RetrieveDataField(input_file_fields[1], input_data_fields);
					std::shared_ptr<BaseField> the_output_field_datetime_start = RetrieveDataField(output_table_fields[0], output_data_fields);
					std::shared_ptr<BaseField> the_output_field_datetime_end = RetrieveDataField(output_table_fields[1], output_data_fields);

					if (!the_input_field_datetime_start || !the_input_field_datetime_end || !the_output_field_datetime_start || !the_output_field_datetime_end
						|| (the_input_field_datetime_start->empty && the_input_field_datetime_end->empty))
					{
						validTimeFields = false;
						return;
					}

					if (the_input_field_datetime_start->empty)
					{
						the_input_field_datetime_start->SetValueString(the_input_field_datetime_end->GetStringRef());
					}

					if (the_input_field_datetime_end->empty)
					{
						the_input_field_datetime_end->SetValueString(the_input_field_datetime_start->GetStringRef());
					}

					boost::posix_time::ptime time_t_epoch__1970(boost::gregorian::date(1970, 1, 1));

					int year = 0, month = 0, day = 0;
					ConvertStringToDate(year, month, day, the_input_field_datetime_start->GetStringRef());
					boost::posix_time::ptime time_t_epoch__rowdatestart(boost::gregorian::date(year, month, day));

					year = 0; month = 0; day = 0;
					ConvertStringToDate(year, month, day, the_input_field_datetime_end->GetStringRef());
					boost::posix_time::ptime time_t_epoch__rowdateend(boost::gregorian::date(year, month, day));

					boost::posix_time::time_duration diff_start_from_1970 = time_t_epoch__rowdatestart - time_t_epoch__1970;
					boost::posix_time::time_duration diff_end_from_1970 = time_t_epoch__rowdateend - time_t_epoch__1970;

					the_output_field_datetime_start->SetValueInt64(diff_start_from_1970.total_milliseconds());
					the_output_field_datetime_end->SetValueInt64(diff_end_from_1970.total_milliseconds());

				}
				break;

		}
	}
	catch (std::exception const &)
	{
		validTimeFields = false;
		return;
	}
}

ImportDefinition::ImportDefinition()
	: stmt_insert(nullptr)
	, stmt_update(nullptr)
{
}

ImportDefinition::ImportDefinition(ImportDefinition const & rhs)
	: mappings(rhs.mappings)
	, input_file(rhs.input_file)
	, first_row_is_header_row(rhs.first_row_is_header_row)
	, second_row_is_column_description_row(rhs.second_row_is_column_description_row)
	, third_row_is_data_type_row(rhs.third_row_is_data_type_row)
	, input_schema(rhs.input_schema)
	, output_schema(rhs.output_schema)
	, format_qualifiers(rhs.format_qualifiers)
	, import_type(rhs.import_type)
	, stmt_insert(rhs.stmt_insert)
	, stmt_update(rhs.stmt_update)
{
}

ImportDefinition::~ImportDefinition()
{

	if (stmt_update)
	{
		sqlite3_finalize(stmt_update);
		stmt_update = nullptr;
	}

	if (stmt_insert)
	{
		sqlite3_finalize(stmt_insert);
		stmt_insert = nullptr;
	}

}

bool ImportDefinition::IsEmpty()
{
	if (mappings.size() == 0)
	{
		return true;
	}

	return false;
}

Importer::Importer(ImportDefinition const & import_definition_, Model_basemost * model_, Table_basemost * table_, Mode const mode_, WidgetInstanceIdentifier const & identifier_,
				   TableImportCallbackFn table_write_callback_, WHICH_IMPORT const & which_import_, std::string & errorMsg)
	: import_definition(import_definition_)
	, table_write_callback(table_write_callback_)
	, model(model_)
	, table(table_)
	, identifier(identifier_)
	, mode(mode_)
	, which_import(which_import_)
	, badreadlines(0)
	, badwritelines(0)
	, goodreadlines(0)
	, goodwritelines(0)
	, goodupdatelines(0)
	, block_size(0)
{

	InputModel * inputModel = nullptr;
	OutputModel * outputModel = nullptr;

	if (mode == INSERT_OR_UPDATE)
	{
		// The SQLite limit of 500 does not apply here
		block_size = block_size_no_sqlite_limit;
	}
	else
	{
		block_size = block_size_sqlite_limit;
	}

	if (table->table_model_type == Table_basemost::TABLE_MODEL_TYPE__INPUT_MODEL)
	{

		bool bad = false;

		try
		{
			inputModel = dynamic_cast<InputModel *>(model);
		}
		catch (std::bad_cast &)
		{
			bad = true;
		}

		if (inputModel == nullptr)
		{
			bad = true;
		}

		if (bad)
		{
			boost::format msg("Bad input mode in DoImport.");
			errorMsg = msg.str();
			return;
		}

		{

			Executor executor(inputModel->getDb());

			if (!table->ImportStart(inputModel->getDb(), identifier, import_definition, nullptr, inputModel))
			{
				boost::format msg("Failed to start input data import.");
				errorMsg = msg.str();
				return;
			}

			executor.success();

		}

	}

	if (table->table_model_type == Table_basemost::TABLE_MODEL_TYPE__OUTPUT_MODEL)
	{

		bool bad = false;

		try
		{
			outputModel = dynamic_cast<OutputModel *>(model);
		}
		catch (std::bad_cast &)
		{
			bad = true;
		}

		if (outputModel == nullptr)
		{
			bad = true;
		}

		if (bad)
		{
			boost::format msg("Bad output mode in DoImport.");
			errorMsg = msg.str();
			return;
		}

		inputModel = &outputModel->getInputModel();

		{

			Executor executor(outputModel->getDb());

			if (!table->ImportStart(outputModel->getDb(), identifier, import_definition, outputModel, inputModel))
			{
				boost::format msg("Failed to start output data import.");
				errorMsg = msg.str();
				return;
			}

			executor.success();

		}

	}

}

Importer::~Importer()
{

	InputModel * inputModel = nullptr;
	OutputModel * outputModel = nullptr;

	if (table->table_model_type == Table_basemost::TABLE_MODEL_TYPE__INPUT_MODEL)
	{
		try
		{
			inputModel = dynamic_cast<InputModel *>(model);
		}
		catch (std::bad_cast &)
		{
			// no throws in ctor
			return;
		}

		if (inputModel == nullptr)
		{
			// no throws in ctor
			return;
		}

		if (!table->ImportEnd(inputModel->getDb(), identifier, import_definition, nullptr, inputModel))
		{
			// no throws in ctor
			return;
		}
	}

	if (table->table_model_type == Table_basemost::TABLE_MODEL_TYPE__OUTPUT_MODEL)
	{
		try
		{
			outputModel = dynamic_cast<OutputModel *>(model);
		}
		catch (std::bad_cast &)
		{
			// no throws in ctor
			return;
		}

		if (outputModel == nullptr)
		{
			// no throws in ctor
			return;
		}

		if (!table->ImportEnd(outputModel->getDb(), identifier, import_definition, outputModel, inputModel))
		{
			// no throws in ctor
			return;
		}
	}

}

void Importer::InitializeFields()
{

	for (int n = 0; n < block_size; ++n)
	{

		DataFields fields;

		std::for_each(import_definition.input_schema.schema.cbegin(), import_definition.input_schema.schema.cend(), [&fields](SchemaEntry const & column)
		{
			FIELD_TYPE field_type = column.field_type;
			std::string field_name = column.field_name;
			InstantiateDataFieldInstance(field_type, field_name, fields);
		});

		input_block.push_back(fields);

	}

	for (int n = 0; n < block_size; ++n)
	{

		DataFields fields;

		std::for_each(import_definition.output_schema.schema.cbegin(), import_definition.output_schema.schema.cend(), [&fields](SchemaEntry const & column)
		{

			FIELD_TYPE field_type = column.field_type;
			std::string field_name = column.field_name;

			std::shared_ptr<BaseField> field;
			FieldFactory(field_type, field_name, field);
			fields.push_back(field);

		});

		output_block.push_back(fields);

	}

}

bool Importer::ValidateMapping()
{

	std::vector<std::pair<int, bool>> indices_required_are_matched;

	// loop through input schema pushing back required indices
	int current_index = 0;
	std::for_each(import_definition.input_schema.schema.cbegin(),
				  import_definition.input_schema.schema.cend(), [this, &indices_required_are_matched, &current_index](SchemaEntry const & schema_entry)
	{
		if (schema_entry.required)
		{
			indices_required_are_matched.push_back(std::make_pair(current_index, false)); // start off with false, indicating NOT MATCHED yet
		}

		++current_index;
	});

	std::for_each(import_definition.mappings.cbegin(), import_definition.mappings.cend(), [this, &indices_required_are_matched](std::shared_ptr<FieldMapping> const & field_mapping)
	{

		if (!field_mapping)
		{
			// TODO: Import problem warning
			return;
		}

		if (!field_mapping->Validate())
		{
			// TODO: Import problem warning
			return;
		}

		std::for_each(field_mapping->input_file_fields.cbegin(), field_mapping->input_file_fields.cend(), [this](FieldTypeEntry const & input_entry)
		{
			if (input_entry.first.name_or_index == NameOrIndex::NAME)
			{
				// Make certain the name exists in the input schema
			}
			else
			{
				// Make certain the index exists in the input schema
			}
		});

		std::for_each(field_mapping->output_table_fields.cbegin(), field_mapping->output_table_fields.cend(), [this](FieldTypeEntry const & output_entry)
		{
			if (output_entry.first.name_or_index == NameOrIndex::NAME)
			{
				// Make certain the name exists in the output table
			}
			else
			{
				// Make certain the index exists in the output table
			}
		});

		switch (field_mapping->field_mapping_type)
		{

			case FieldMapping::FIELD_MAPPING_TYPE__ONE_TO_ONE:
				{
					try
					{
						OneToOneFieldMapping const & the_mapping = dynamic_cast<OneToOneFieldMapping const &>(*field_mapping);
						FieldTypeEntry const & output_entry = field_mapping->output_table_fields[0];

						// search the output schema to see what numeric index this corresponds to, and set it in indices_required_are_matched
						int current_index = 0;
						std::for_each(import_definition.output_schema.schema.cbegin(),
									  import_definition.output_schema.schema.cend(), [this, &output_entry, &indices_required_are_matched, &current_index](SchemaEntry const & output_schema_entry)
						{
							bool match = false;

							if (output_entry.first.name_or_index == NameOrIndex::NAME)
							{
								if (boost::iequals(output_entry.first.name, output_schema_entry.field_name))
								{
									match = true;
								}
							}
							else
							{
								if (output_entry.first.index == current_index)
								{
									match = true;
								}
							}

							if (match)
							{
								std::for_each(indices_required_are_matched.begin(), indices_required_are_matched.end(), [&current_index](std::pair<int, bool> & the_pair)
								{
									if (the_pair.first == current_index)
									{
										the_pair.second = true;
									}
								});
							}

							++current_index;
						});
					}
					catch (std::bad_cast &)
					{
						// TODO: Import problem warning
						return;
					}
				}
				break;

			case FieldMapping::FIELD_MAPPING_TYPE__HARD_CODED:
				{
					try
					{
						HardCodedFieldMapping const & the_mapping = dynamic_cast<HardCodedFieldMapping const &>(*field_mapping);
						FieldTypeEntry const & output_entry = field_mapping->output_table_fields[0];

						// search the output schema to see what numeric index this corresponds to, and set it in indices_required_are_matched
						int current_index = 0;
						std::for_each(import_definition.output_schema.schema.cbegin(),
									  import_definition.output_schema.schema.cend(), [this, &output_entry, &indices_required_are_matched, &current_index](SchemaEntry const & output_schema_entry)
						{
							bool match = false;

							if (output_entry.first.name_or_index == NameOrIndex::NAME)
							{
								if (boost::iequals(output_entry.first.name, output_schema_entry.field_name))
								{
									match = true;
								}
							}
							else
							{
								if (output_entry.first.index == current_index)
								{
									match = true;
								}
							}

							if (match)
							{
								std::for_each(indices_required_are_matched.begin(), indices_required_are_matched.end(), [&current_index](std::pair<int, bool> & the_pair)
								{
									if (the_pair.first == current_index)
									{
										the_pair.second = true;
									}
								});
							}

							++current_index;
						});
					}
					catch (std::bad_cast &)
					{
						// TODO: Import problem warning
						return;
					}
				}
				break;

			case FieldMapping::FIELD_MAPPING_TYPE__TIME_RANGE:
				{
					try
					{
						TimeRangeFieldMapping const & the_mapping = dynamic_cast<TimeRangeFieldMapping const &>(*field_mapping);
					}
					catch (std::bad_cast &)
					{
						// TODO: Import problem warning
						return;
					}
				}
				break;

		}
	});

	bool something_did_not_match = false;
	std::for_each(indices_required_are_matched.begin(), indices_required_are_matched.end(), [&something_did_not_match](std::pair<int, bool> & the_pair)
	{
		if (the_pair.second == false)
		{
			something_did_not_match = true;
		}
	});

	if (something_did_not_match)
	{
		// Not all required table entries have matched!
		// Error!
		return false;
	}

	return true;

}

void Importer::RetrieveStringField(char *& current_line_ptr, char *& parsed_line_ptr, bool & stop, ImportDefinition const & import_definition, std::string & errorMsg)
{
	bool is_quoted_field = false;
	bool quote_type_is_single = false;
	char * starting_parsed_pointer = parsed_line_ptr;

	// Note: Whitespace has already been stripped

	if (import_definition.format_qualifiers & ImportDefinition::FORMAT_QUALIFIERS__STRINGS_ARE_SINGLEQUOTED)
	{
		is_quoted_field = true;
		quote_type_is_single = true;

		if (*current_line_ptr != '\'')
		{
			is_quoted_field = false;
		}
		else
		{
			++current_line_ptr;
		}
	}
	else if (import_definition.format_qualifiers & ImportDefinition::FORMAT_QUALIFIERS__STRINGS_ARE_DOUBLEQUOTED)
	{
		is_quoted_field = true;
		quote_type_is_single = false;

		if (*current_line_ptr != '"')
		{
			is_quoted_field = false;
		}
		else
		{
			++current_line_ptr;
		}
	}
	else if (import_definition.format_qualifiers & ImportDefinition::FORMAT_QUALIFIERS__STRINGS_ARE_EITHER_DOUBLEQUOTED_OR_SINGLEQUOTED)
	{
		is_quoted_field = true;

		if (*current_line_ptr != '"' && *current_line_ptr != '\'')
		{
			is_quoted_field = false;
		}
		else
		{
			if (*current_line_ptr == '"')
			{
				quote_type_is_single = false;
			}
			else if (*current_line_ptr == '\'')
			{
				quote_type_is_single = true;
			}

			++current_line_ptr;
		}
	}

	if (*current_line_ptr == '\0')
	{

		// Empty field
		return;

	}

	bool escapemode = false;
	bool done = false;

	while (!done)
	{
		if (*current_line_ptr == '\0')
		{
			// Whitespace on right
			if (import_definition.format_qualifiers & ImportDefinition::FORMAT_QUALIFIERS__TAB_DELIMITED)
			{
				while (parsed_line_ptr > starting_parsed_pointer)
				{
					--parsed_line_ptr;

					if (*parsed_line_ptr == ' ')
					{
						*parsed_line_ptr = '\0';
					}
					else
					{
						++parsed_line_ptr;
						break;
					}
				}
			}
			else
			{
				while (parsed_line_ptr > starting_parsed_pointer)
				{
					--parsed_line_ptr;

					if (*parsed_line_ptr == ' ' || *parsed_line_ptr == '\t')
					{
						*parsed_line_ptr = '\0';
					}
					else
					{
						++parsed_line_ptr;
						break;
					}
				}
			}

			done = true;
			continue;
		}

		if (escapemode)
		{
			*(parsed_line_ptr) = *current_line_ptr;
			++parsed_line_ptr;
			*parsed_line_ptr = '\0';
			escapemode = false;
		}
		else
		{
			if (import_definition.format_qualifiers & ImportDefinition::FORMAT_QUALIFIERS__BACKSLASH_ESCAPE_CHAR)
			{
				if (*current_line_ptr == '\\')
				{
					escapemode = true;
					++current_line_ptr;
					continue;
				}
			}

			if (is_quoted_field && import_definition.format_qualifiers & ImportDefinition::FORMAT_QUALIFIERS__STRINGS_ARE_SINGLEQUOTED)
			{
				if (*current_line_ptr == '\'')
				{
					done = true;
				}
			}
			else if (is_quoted_field && import_definition.format_qualifiers & ImportDefinition::FORMAT_QUALIFIERS__STRINGS_ARE_DOUBLEQUOTED)
			{
				if (*current_line_ptr == '"')
				{
					done = true;
				}
			}
			else if (is_quoted_field && import_definition.format_qualifiers & ImportDefinition::FORMAT_QUALIFIERS__STRINGS_ARE_EITHER_DOUBLEQUOTED_OR_SINGLEQUOTED)
			{
				if (quote_type_is_single)
				{
					if (*current_line_ptr == '\'')
					{
						done = true;
					}
				}
				else
				{
					if (*current_line_ptr == '"')
					{
						done = true;
					}
				}
			}
			else
			{
				if (import_definition.format_qualifiers & ImportDefinition::FORMAT_QUALIFIERS__TAB_DELIMITED)
				{
					if (*current_line_ptr == '\t')
					{
						done = true;
					}
				}
				else
				{
					if (*current_line_ptr == ',')
					{
						done = true;
					}
				}
			}

			if (!done)
			{
				*(parsed_line_ptr) = *current_line_ptr;
				++parsed_line_ptr;
				*parsed_line_ptr = '\0';
			}
		}

		if (!done)
		{
			++current_line_ptr;
		}

		if (done)
		{
			// Whitespace on right
			if (import_definition.format_qualifiers & ImportDefinition::FORMAT_QUALIFIERS__TAB_DELIMITED)
			{
				while (parsed_line_ptr > starting_parsed_pointer)
				{
					--parsed_line_ptr;

					if (*parsed_line_ptr == ' ')
					{
						*parsed_line_ptr = '\0';
					}
					else
					{
						++parsed_line_ptr;
						break;
					}
				}
			}
			else
			{
				while (parsed_line_ptr > starting_parsed_pointer)
				{
					--parsed_line_ptr;

					if (*parsed_line_ptr == ' ' || *parsed_line_ptr == '\t')
					{
						*parsed_line_ptr = '\0';
					}
					else
					{
						++parsed_line_ptr;
						break;
					}
				}
			}

			++parsed_line_ptr;
			*parsed_line_ptr = '\0';
		}
	}

	parsed_line_ptr = starting_parsed_pointer;
}

std::shared_ptr<BaseField> RetrieveDataField(FieldTypeEntry const & field_type_entry, DataFields const & data_fields)
{
	std::shared_ptr<BaseField> the_field;

	if (field_type_entry.first.name_or_index == NameOrIndex::NAME)
	{
		std::string const & input_field_name = field_type_entry.first.name;
		std::for_each(data_fields.cbegin(), data_fields.cend(), [&input_field_name, &the_field](std::shared_ptr<BaseField> const & input_field)
		{
			if (input_field && boost::iequals(input_field->GetName(), input_field_name))
			{
				the_field = input_field;
			}
		});
	}
	else
	{
		if (field_type_entry.first.index < (int)data_fields.size())
		{
			the_field = data_fields[field_type_entry.first.index];
		}
	}

	return the_field;
}

void Importer::ReadFieldFromFile(char *& current_line_ptr, int & current_lines_read, int const & current_column_index, char *& parsed_line_ptr, bool & stop,
								 SchemaEntry const & column, long const line, int const col, bool const is_final_col, std::string & errorMsg)
{

	// use sscanf to read in the type, based on switch on "column"'s type type-traits... read it into input_block[current_lines_read]
	std::shared_ptr<BaseField> field_entry = input_block[current_lines_read][current_column_index];

	if (!field_entry)
	{
		boost::format msg("Invalid block cache while retrieving line %1%, col %2%");
		msg % boost::lexical_cast<std::string>(line + 1) % boost::lexical_cast<std::string>(col + 1);
		errorMsg = msg.str();
		stop = true;
		return;
	}

	ReadFieldFromFileStatic(current_line_ptr, parsed_line_ptr, stop, column, *field_entry, import_definition, line + 1, col + 1, is_final_col, errorMsg);

}

void Importer::ReadFieldFromFileStatic(char *& current_line_ptr, char *& parsed_line_ptr, bool & stop, SchemaEntry const & column, BaseField & theField,
									   ImportDefinition const & import_definition, long line, int col, bool const is_final_col, std::string & errorMsg)
{

	EatWhitespace(current_line_ptr, import_definition);

	if (!is_final_col && *current_line_ptr == '\0')
	{
		boost::format msg("End of row was reached prematurely when attempting to read the column from the input file at row %1%, column %2%");
		msg % boost::lexical_cast<std::string>(line + 1) % boost::lexical_cast<std::string>(col + 1);
		errorMsg = msg.str();
		stop = true;
		return;
	}

	if (*current_line_ptr == '\0')
	{
		// final column is empty
	}

	ReadOneDataField(column, theField, current_line_ptr, parsed_line_ptr, stop, import_definition, line, col, errorMsg);

	if (stop)
	{
		boost::format msg("Error reading data field for line %1%, column %2%: %3%");
		msg % line % col % errorMsg.c_str();
		errorMsg = msg.str();
		return;
	}

	EatWhitespace(current_line_ptr, import_definition);
	EatSeparator(current_line_ptr, import_definition);

}

void Importer::SkipFieldInFile(char *& current_line_ptr, char *& parsed_line_ptr, bool & stop, ImportDefinition const & import_definition, long line, int col,
							   bool const is_final_col, std::string & errorMsg)
{

	EatWhitespace(current_line_ptr, import_definition);

	if (!is_final_col && *current_line_ptr == '\0')
	{
		boost::format msg("End of row was reached prematurely when attempting to read the column from the input file at row %1%, column %2%");
		msg % boost::lexical_cast<std::string>(line + 1) % boost::lexical_cast<std::string>(col + 1);
		errorMsg = msg.str();
		stop = true;
		return;
	}

	if (*current_line_ptr == '\0')
	{
		// final column is empty
	}

	// read it, and do nothing with it
	RetrieveStringField(current_line_ptr, parsed_line_ptr, stop, import_definition, errorMsg);

	if (stop)
	{
		boost::format msg("Error reading data field for line %1%, column %2%: %3%");
		msg % line % col % errorMsg.c_str();
		errorMsg = msg.str();
		return;
	}

	EatWhitespace(current_line_ptr, import_definition);
	EatSeparator(current_line_ptr, import_definition);

}

int Importer::ReadBlockFromFile(std::fstream & data_file, char * line, char * parsedline, long & linenum, std::string & errorMsg, Messager & messager)
{
	int current_lines_read = 0;

	while (data_file.getline(line, MAX_LINE_SIZE - 1) && !data_file.fail() && !data_file.bad() && (!data_file.eof()
			|| boost::algorithm::trim_copy_if(std::string(line), boost::algorithm::is_any_of(" \t")).size() > 0))
	{

		if (CheckCancelled())
		{
			return 0;
		}

		char * current_line_ptr = line;
		char * parsed_line_ptr = parsedline;
		*parsed_line_ptr = '\0';
		bool stop = false;

		int nCols = import_definition.input_schema.validcols.size();
		int nValColIdx = 0;

		for (int ncol = 0; ncol < nCols; ++ncol)
		{

			parsed_line_ptr = parsedline;
			*parsed_line_ptr = '\0';

			if (import_definition.input_schema.validcols[ncol])
			{
				ReadFieldFromFile(current_line_ptr, current_lines_read, nValColIdx, parsed_line_ptr, stop, import_definition.input_schema.schema[nValColIdx], linenum, ncol, ncol == nCols - 1,
								  errorMsg);
				++nValColIdx;
			}
			else
			{
				SkipFieldInFile(current_line_ptr, parsed_line_ptr, stop, import_definition, linenum, ncol, ncol == nCols - 1, errorMsg);
			}

			if (stop)
			{
				boost::format msg("Skipping row number %1% in the input file (\"%2%\"): %3%");
				msg % boost::lexical_cast<std::string>(linenum + 1) % line % errorMsg.c_str();
				errorMsg = msg.str();
				break;
			}

		}

		if (stop)
		{
			errors.push_back(errorMsg);
			errorMsg.clear();
			++linenum;
			++badreadlines;
			continue;
		}
		else
		{
			++goodreadlines;
		}

		// loop through mappings to generate output
		DataFields & input_data_fields = input_block[current_lines_read];
		DataFields & output_data_fields = output_block[current_lines_read];
		stop = false;
		bool continueNextLine = false;
		std::for_each(import_definition.mappings.begin(), import_definition.mappings.end(), [this, &input_data_fields, &output_data_fields, &stop, &continueNextLine, &linenum, &line,
					  &errorMsg](
						  std::shared_ptr<FieldMapping> & field_mapping)
		{
			if (!field_mapping)
			{
				boost::format msg("Invalid mapping cache while constructing data fields to pass to database from the file being imported for line %1%");
				msg % boost::lexical_cast<std::string>(linenum);
				errorMsg = msg.str();
				stop = true;
				return;
			}

			// This is where more complex validation would go
			if (!field_mapping->Validate())
			{
				boost::format msg("Invalid field mapping while constructing data fields to pass to database from the file being imported for line %1%.");
				msg % boost::lexical_cast<std::string>(linenum);
				errorMsg = msg.str();
				stop = true;
				return;
			}

			switch (field_mapping->field_mapping_type)
			{

				case FieldMapping::FIELD_MAPPING_TYPE__ONE_TO_ONE:
					{

						OneToOneFieldMapping const & the_mapping = dynamic_cast<OneToOneFieldMapping const &>(*field_mapping);
						FieldTypeEntry const & input_entry = field_mapping->input_file_fields[0];
						FieldTypeEntry const & output_entry = field_mapping->output_table_fields[0];

						if (input_entry.second != output_entry.second)
						{
							boost::format msg("Invalid field mapping object while constructing data fields to pass to database from the file being imported for line %1%.");
							msg % boost::lexical_cast<std::string>(linenum);
							errorMsg = msg.str();
							stop = true;
							return;
						}

						// perform the mapping here

						std::shared_ptr<BaseField> the_input_field = RetrieveDataField(input_entry, input_data_fields);
						std::shared_ptr<BaseField> the_output_field = RetrieveDataField(output_entry, output_data_fields);;

						if (the_input_field && the_output_field)
						{
							if (the_input_field->GetType() != the_output_field->GetType())
							{
								boost::format msg("Mapping type mismatch for one-to-one mapping while constructing data fields to pass to database from the file being imported for line %1%.");
								msg % boost::lexical_cast<std::string>(linenum);
								errorMsg = msg.str();
								stop = true;
								return;
							}

							if (IsFieldTypeInt32(the_input_field->GetType()))
							{
								the_output_field->SetValueInt32(the_input_field->GetInt32Ref());
							}
							else if (IsFieldTypeInt64(the_input_field->GetType()))
							{
								the_output_field->SetValueInt64(the_input_field->GetInt64Ref());
							}
							else if (IsFieldTypeFloat(the_input_field->GetType()))
							{
								the_output_field->SetValueDouble(the_input_field->GetDouble());
							}
							else if (IsFieldTypeString(the_input_field->GetType()))
							{
								the_output_field->SetValueString(the_input_field->GetString());
							}

						}

					}
					break;

				case FieldMapping::FIELD_MAPPING_TYPE__HARD_CODED:
					{

						HardCodedFieldMapping const & the_mapping = dynamic_cast<HardCodedFieldMapping const &>(*field_mapping);
						FieldTypeEntry const & output_entry = field_mapping->output_table_fields[0];

						// perform the mapping here

						std::shared_ptr<BaseField> the_input_field = the_mapping.data;
						std::shared_ptr<BaseField> the_output_field = RetrieveDataField(output_entry, output_data_fields);

						if (the_input_field && the_output_field)
						{

							if (the_input_field->GetType() != the_output_field->GetType())
							{
								boost::format msg("Mapping type mismatch for hard-coded mapping while constructing data fields to pass to database from the file being imported for line %1%.");
								msg % boost::lexical_cast<std::string>(linenum);
								errorMsg = msg.str();
								stop = true;
								return;
							}

							if (IsFieldTypeInt32(the_input_field->GetType()))
							{
								the_output_field->SetValueInt32(the_input_field->GetInt32Ref());
							}
							else if (IsFieldTypeInt64(the_input_field->GetType()))
							{
								the_output_field->SetValueInt64(the_input_field->GetInt64Ref());
							}
							else if (IsFieldTypeFloat(the_input_field->GetType()))
							{
								the_output_field->SetValueDouble(the_input_field->GetDouble());
							}
							else if (IsFieldTypeString(the_input_field->GetType()))
							{
								the_output_field->SetValueString(the_input_field->GetString());
							}

						}

					}
					break;

				case FieldMapping::FIELD_MAPPING_TYPE__TIME_RANGE:
					{

						TimeRangeFieldMapping & the_mapping = dynamic_cast<TimeRangeFieldMapping &>(*field_mapping);
						the_mapping.PerformMapping(input_data_fields, output_data_fields);

						if (!the_mapping.validTimeFields)
						{
							errorMsg = "Invalid time range fields";
							boost::format msg("Skipping row number %1% in the input file (\"%2%\"): %3%");
							msg % boost::lexical_cast<std::string>(linenum + 1) % line % errorMsg.c_str();
							errors.push_back(msg.str());
							errorMsg.clear();
							++linenum;
							--goodreadlines; // undo the increment from above
							++badreadlines;
							continueNextLine = true;
							return; // from lambda
						}

					}
					break;

			}
		});

		if (continueNextLine)
		{
			continue;
		}

		++current_lines_read;
		++linenum;

		if (linenum % 100 == 0)
		{
			if (which_import == IMPORT_DMU_SET_MEMBER)
			{
				messager.EmitSignalUpdateDMUImportProgressBar(PROGRESS_UPDATE_MODE__SET_VALUE, 0, 0, linenum);
			}
			else if (which_import == IMPORT_VG_INSTANCE_DATA)
			{
				messager.EmitSignalUpdateVGImportProgressBar(PROGRESS_UPDATE_MODE__SET_VALUE, 0, 0, linenum);
			}
		}

		// This stop is a true error condition - do not continue
		// (note it was reset to false above, before entering second loop above)
		if (stop)
		{
			return -1;
		}

		if (current_lines_read == block_size)
		{
			return current_lines_read;
		}

	}

	return current_lines_read;
}

bool Importer::DoImport(std::string & errorMsg, Messager & messager)
{

	is_performing_import = true;
	cancelled = false;

	BOOST_SCOPE_EXIT(&is_performing_import, &is_performing_import_mutex, &messager, this_)
	{
		is_performing_import = false;

		if (this_->which_import == IMPORT_DMU_SET_MEMBER)
		{
			messager.EmitSignalUpdateDMUImportProgressBar(PROGRESS_UPDATE_MODE__HIDE, 0, 0, 0);
		}
		else if (this_->which_import == IMPORT_VG_INSTANCE_DATA)
		{
			messager.EmitSignalUpdateVGImportProgressBar(PROGRESS_UPDATE_MODE__HIDE, 0, 0, 0);
		}
	} BOOST_SCOPE_EXIT_END

	// Count lines in file
	std::fstream data_file_count_lines;
	data_file_count_lines.open(import_definition.input_file.c_str(), std::ios::in);

	if (data_file_count_lines.is_open())
	{
		char line[MAX_LINE_SIZE];
		long linenum = 0;

		while (data_file_count_lines.getline(line, MAX_LINE_SIZE - 1) && data_file_count_lines.good())
		{
			++linenum;
		}

		if (which_import == IMPORT_DMU_SET_MEMBER)
		{
			messager.EmitSignalUpdateDMUImportProgressBar(PROGRESS_UPDATE_MODE__SHOW, 0, 0, 0);
			messager.EmitSignalUpdateDMUImportProgressBar(PROGRESS_UPDATE_MODE__SET_LIMITS, 0, linenum, 0);
		}
		else if (which_import == IMPORT_VG_INSTANCE_DATA)
		{
			messager.EmitSignalUpdateVGImportProgressBar(PROGRESS_UPDATE_MODE__SHOW, 0, 0, 0);
			messager.EmitSignalUpdateVGImportProgressBar(PROGRESS_UPDATE_MODE__SET_LIMITS, 0, linenum, 0);
		}

		data_file_count_lines.close();
	}
	else
	{
		boost::format msg("Failed to open input file.");
		errorMsg = msg.str();
		return false;
	}

	// TODO: try/finally or exit block
	// open file
	std::fstream data_file;
	data_file.open(import_definition.input_file.c_str(), std::ios::in);

	if (data_file.is_open())
	{

		BOOST_SCOPE_EXIT(&data_file)
		{
			data_file.close();
		} BOOST_SCOPE_EXIT_END

		char line[MAX_LINE_SIZE];
		char parsedline[MAX_LINE_SIZE];
		long linenum = 0;

		// handle the column names row
		if (import_definition.first_row_is_header_row && data_file.good())
		{

			data_file.getline(line, MAX_LINE_SIZE - 1);

			if (!data_file.good())
			{
				boost::format msg("Failed to read column names row from the input file.");
				errorMsg = msg.str();
				return true;
			}

			// The desired columns in the schema might be a subset of the columns in the input file,
			// and they might appear in a different order than the columns in the file
			std::vector<std::string> colnames;
			boost::split(colnames, line, boost::is_any_of(","));
			std::for_each(colnames.begin(), colnames.end(), std::bind(boost::trim<std::string>, std::placeholders::_1, std::locale()));
			import_definition.input_schema.ReorderAccToColumnNames(colnames);

			bool bad = false;
			std::for_each(import_definition.input_schema.schema.cbegin(), import_definition.input_schema.schema.cend(), [&](SchemaEntry const & schema_entry)
			{

				if (bad)
				{
					return;
				}

				// Validate column name
				std::string test_col_name = schema_entry.field_name;
				bool validated = Validation::ValidateColumnName(test_col_name, "variable group column", true, errorMsg);

				if (test_col_name != schema_entry.field_name)
				{
					validated = false;
				}

				if (!validated)
				{
					bad = true;
				}

				// Validate column description
				std::string test_col_description = schema_entry.field_description;
				validated = Validation::ValidateColumnDescription(test_col_description, "variable group column", false, errorMsg);

				if (test_col_description != schema_entry.field_description)
				{
					validated = false;
				}

				if (!validated)
				{
					bad = true;
				}

			});

			if (bad)
			{
				return false;
			}

			++linenum;

		}

		// handle the column description row
		if (import_definition.second_row_is_column_description_row && data_file.good())
		{

			data_file.getline(line, MAX_LINE_SIZE - 1);

			if (!data_file.good())
			{
				boost::format msg("Failed to read the column description row in the input file.");
				errorMsg = msg.str();
				errors.push_back(errorMsg);
				errorMsg.clear();
				import_definition.second_row_is_column_description_row = false;
			}

			// The column descriptions have already been read

			++linenum;

		}

		// handle the column data types
		if (import_definition.third_row_is_data_type_row && data_file.good())
		{

			data_file.getline(line, MAX_LINE_SIZE - 1);

			if (!data_file.good())
			{
				boost::format msg("Failed to read the data type row in the input file.");
				errorMsg = msg.str();
				errors.push_back(errorMsg);
				errorMsg.clear();
				return false;
			}

			// The data types have already been read

			++linenum;

		}

		InitializeFields();

		// validate
		if (!ValidateMapping())
		{
			boost::format msg("Bad mapping of input to output columns.");
			errorMsg = msg.str();
			errors.push_back(errorMsg);
			errorMsg.clear();
			return false;
		}

		std::int64_t current_lines_read = 0;

		int currently_read_lines = 0;

		while (true)
		{

			long saved_linenum = linenum;

			std::string blockErrorMsg;
			currently_read_lines = ReadBlockFromFile(data_file, line, parsedline, linenum, blockErrorMsg, messager);

			if (CheckCancelled())
			{
				break;
			}

			if (currently_read_lines == -1)
			{
				boost::format msg("Irrecoverable error: %1%");
				msg % blockErrorMsg;
				errorMsg = msg.str();
				errors.push_back(errorMsg);
				errorMsg.clear();
				return false;
			}
			else if (currently_read_lines == 0)
			{
				// nothing to do
				break;
			}
			else
			{
				// Write rows to database here
				table_write_callback(this, model, import_definition, table, output_block, currently_read_lines, saved_linenum, badwritelines, goodwritelines, goodupdatelines, errors);

				if (CheckCancelled())
				{
					break;
				}

			}

		}

	}
	else
	{
		boost::format msg("Failed to open input file.");
		errorMsg = msg.str();
		errors.push_back(errorMsg);
		errorMsg.clear();
		return false;
	}

	if (CheckCancelled())
	{
		// TODO: re-enable this when there is a distinction between import LOGFILE and import ERROR FILE
		//boost::format msg("Import operation cancelled.");
		//errors.push_back(msg.str());
	}

	// Return true even if cancelled
	return true;

}

void Importer::EatWhitespace(char *& current_line_ptr, ImportDefinition const & import_definition)
{
	if (import_definition.format_qualifiers & ImportDefinition::FORMAT_QUALIFIERS__TAB_DELIMITED)
	{
		while (*current_line_ptr == ' ') { ++current_line_ptr; }
	}
	else
	{
		while (*current_line_ptr == ' ' || *current_line_ptr == '\t') { ++current_line_ptr; }
	}
}

void Importer::EatSeparator(char *& current_line_ptr, ImportDefinition const & import_definition)
{
	// For quoted fields, if the closing quote occurs early, just eat the remaining text up to the next separator
	if (import_definition.format_qualifiers & ImportDefinition::FORMAT_QUALIFIERS__TAB_DELIMITED)
	{
		while (*current_line_ptr != '\t' && *current_line_ptr != '\0') { ++current_line_ptr; }

		if (*current_line_ptr == '\t') { ++current_line_ptr; }
	}
	else
	{
		while (*current_line_ptr != ',' && *current_line_ptr != '\0') { ++current_line_ptr; }

		if (*current_line_ptr == ',') { ++current_line_ptr; }
	}
}

void Importer::ReadOneDataField(SchemaEntry const & column, BaseField & theField, char *& current_line_ptr, char *& parsed_line_ptr, bool & stop,
								ImportDefinition const & import_definition, long const line, int const col, std::string & errorMsg)
{

	size_t number_chars_read = 0;

	theField.empty = false;

	// For all data types, retrieve the full field as a string, to start
	std::string stringFieldErrorMsg;
	RetrieveStringField(current_line_ptr, parsed_line_ptr, stop, import_definition, stringFieldErrorMsg);

	if (stop)
	{
		boost::format msg("Cannot read data field from input file!  Line %1%, column %2%: %3%");
		msg % line % col % stringFieldErrorMsg;
		errorMsg = msg.str();
		return;
	}

	bool empty_field(std::string(parsed_line_ptr).empty());

	if (empty_field)
	{

		theField.empty = true;

		// Check if this is a required field -
		// either a primary key field,
		// or a time field

		bool field_is_required = false;

		if (IsFieldTypeTimeRange(theField.GetType()))
		{
			field_is_required = false;
		}

		if (column.IsPrimaryKey() && !IsFieldTypeTimeRange(theField.GetType()))
		{
			field_is_required = true;
		}

		if (field_is_required)
		{
			boost::format msg("A required field is missing!  Row %1%, col %2%");
			msg % line % col;
			errorMsg = msg.str();
			stop = true;
			return;
		}

	}

	if (IsFieldTypeInt32(theField.GetType()))
	{
		// For performance reasons - no validation.
		// TODO: allow option for end-user to validate, in which case we perform a
		// boost::lexical_cast<> via the Validation helper class and if it throws, we throw
		theField.SetValueInt32(0);

		if (!empty_field)
		{
			sscanf(parsed_line_ptr, "%d%n", &theField.GetInt32Ref(), &number_chars_read);

			if (number_chars_read < strlen(parsed_line_ptr))
			{
				boost::format msg("Invalid int32 data field from input file!  Line %1%, column %2%");
				msg % line % col;
				errorMsg = msg.str();
				stop = true;
				return;
			}
		}
	}
	else if (IsFieldTypeInt64(theField.GetType()))
	{
		// For performance reasons - no validation.
		// TODO: allow option for end-user to validate, in which case we perform a
		// boost::lexical_cast<> via the Validation helper class and if it throws, we throw
		theField.SetValueInt64(0);

		if (!empty_field)
		{
			sscanf(parsed_line_ptr, "%I64d%n", &theField.GetInt64Ref(), &number_chars_read);

			if (number_chars_read < strlen(parsed_line_ptr))
			{
				boost::format msg("Invalid int64 data field from input file!  Line %1%, column %2%");
				msg % line % col;
				errorMsg = msg.str();
				stop = true;
				return;
			}
		}
	}
	else if (IsFieldTypeFloat(theField.GetType()))
	{
		// For performance reasons - no validation.
		// TODO: allow option for end-user to validate, in which case we perform a
		// boost::lexical_cast<> via the Validation helper class and if it throws, we throw
		theField.SetValueDouble(0.0);

		if (!empty_field)
		{
			double temp;
			sscanf(parsed_line_ptr, "%lf%n", &temp, &number_chars_read);
			theField.SetValueDouble(temp);

			if (number_chars_read < strlen(parsed_line_ptr))
			{
				boost::format msg("Invalid floating-point data field from input file!  Line %1%, column %2%");
				msg % line % col;
				errorMsg = msg.str();
				stop = true;
				return;
			}
		}
	}
	else if (IsFieldTypeString(theField.GetType()))
	{
		theField.SetValueString(parsed_line_ptr);
	}

	// Extra validation here
	bool valid = ValidateFieldData(theField);

	if (!valid)
	{
		boost::format msg("Invalid data field from input file!  Line %1%, column %2%");
		msg % line % col;
		errorMsg = msg.str();
		stop = true;
		return;
	}

}

void Importer::InstantiateDataFieldInstance(FIELD_TYPE field_type, std::string field_name, DataFields & fields)
{

	std::shared_ptr<BaseField> field;
	FieldFactory(field_type, field_name, field);
	fields.push_back(field);

}

void TimeRangeFieldMapping::ConvertStringToDate(int & year, int & month, int & day, std::string const & the_string)
{

	boost::tokenizer<> tok(the_string);
	int count = 0;

	for (boost::tokenizer<>::iterator it = tok.begin(); it != tok.end(); ++it)
	{
		switch (count)
		{
			case 0:
				{
					std::string test = *it;
					year = boost::lexical_cast<int>(*it);
				}
				break;

			case 1:
				{
					std::string test = *it;
					std::string the_month = *it;

					if (boost::iequals(*it, "jan"))
					{
						month = 1;
					}
					else if (boost::iequals(*it, "feb"))
					{
						month = 2;
					}
					else if (boost::iequals(*it, "mar"))
					{
						month = 3;
					}
					else if (boost::iequals(*it, "apr"))
					{
						month = 4;
					}
					else if (boost::iequals(*it, "may"))
					{
						month = 5;
					}
					else if (boost::iequals(*it, "jun"))
					{
						month = 6;
					}
					else if (boost::iequals(*it, "jul"))
					{
						month = 7;
					}
					else if (boost::iequals(*it, "aug"))
					{
						month = 8;
					}
					else if (boost::iequals(*it, "sep"))
					{
						month = 9;
					}
					else if (boost::iequals(*it, "oct"))
					{
						month = 10;
					}
					else if (boost::iequals(*it, "nov"))
					{
						month = 11;
					}
					else if (boost::iequals(*it, "dec"))
					{
						month = 12;
					}
				}
				break;

			case 2:
				{
					std::string test = *it;
					day = boost::lexical_cast<int>(*it);
				}
				break;
		}

		++count;
	}

}
