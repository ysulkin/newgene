#ifndef Q_MOC_RUN
	#include <boost/lexical_cast.hpp>
#endif
#include "VariableGroupData.h"
#include "../TableManager.h"
#include "../../InputModel.h"
#include "../../../Utilities/NewGeneUUID.h"

#include <set>

#include "../../../Utilities/Validation.h"

std::string const Table_VariableGroupMetadata_PrimaryKeys::VG_DATA_TABLE_NAME = "VG_DATA_TABLE_NAME";
std::string const Table_VariableGroupMetadata_PrimaryKeys::VG_DATA_TABLE_PRIMARY_KEY_COLUMN_NAME = "VG_DATA_TABLE_PRIMARY_KEY_COLUMN_NAME";
std::string const Table_VariableGroupMetadata_PrimaryKeys::VG_DATA_TABLE_FK_DMU_CATEGORY_CODE = "VG_DATA_TABLE_FK_DMU_CATEGORY_CODE";
std::string const Table_VariableGroupMetadata_PrimaryKeys::VG_DATA_TABLE_PRIMARY_KEY_SEQUENCE_NUMBER = "VG_DATA_TABLE_PRIMARY_KEY_SEQUENCE_NUMBER";
std::string const Table_VariableGroupMetadata_PrimaryKeys::VG_DATA_TABLE_PRIMARY_KEY_FIELD_TYPE = "VG_DATA_TABLE_PRIMARY_KEY_FIELD_TYPE";

std::string const Table_VariableGroupMetadata_DateTimeColumns::VG_DATA_TABLE_NAME = "VG_DATA_TABLE_NAME";
std::string const Table_VariableGroupMetadata_DateTimeColumns::VG_DATA_TABLE_DATETIME_START_COLUMN_NAME = "VG_DATETIME_START_COLUMN_NAME";
std::string const Table_VariableGroupMetadata_DateTimeColumns::VG_DATA_TABLE_DATETIME_END_COLUMN_NAME = "VG_DATETIME_END_COLUMN_NAME";
std::string const Table_VariableGroupMetadata_DateTimeColumns::VG_DATA_FK_VG_CATEGORY_UUID = "VG_DATA_FK_VG_CATEGORY_UUID";

bool Table_VariableGroupData::ImportStart(sqlite3 * db, WidgetInstanceIdentifier const & variable_group, ImportDefinition const & import_definition, OutputModel * output_model_,
		InputModel * input_model_)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	if (!db)
	{
		boost::format msg("Invalid DB in import routine for VG.");
		throw NewGeneException() << newgene_error_description(msg.str());
	}

	//Executor executor(db);

	if (!variable_group.code || variable_group.code->empty() || !variable_group.uuid || variable_group.uuid->empty())
	{
		boost::format msg("Invalid variable group variable_group in import routine.");
		throw NewGeneException() << newgene_error_description(msg.str());
	}

	if (!tableManager().TableExists(db, TableNameFromVGCode(*variable_group.code)))
	{

		// Create the VG data table
		std::string sql_create;
		sql_create += "CREATE TABLE \"";
		sql_create += table_name;
		sql_create += "\" (";

		bool first = true;
		std::for_each(import_definition.output_schema.schema.cbegin(),
					  import_definition.output_schema.schema.cend(), [&import_definition, &sql_create, &first](SchemaEntry const & table_schema_entry)
		{
			if (!first)
			{
				sql_create += ", ";
			}

			first = false;
			sql_create += "`";
			sql_create += table_schema_entry.field_name;
			sql_create += "`";
			sql_create += " ";
			sql_create += GetSqlLiteFieldDataTypeAsString(table_schema_entry.field_type);

		});

		sql_create += ")";

		sqlite3_stmt * stmt = NULL;
		int err = sqlite3_prepare_v2(db, sql_create.c_str(), static_cast<int>(sql_create.size()) + 1, &stmt, NULL);

		if (stmt == NULL)
		{
			boost::format msg("Cannot create SQL \"%1%\" in VG import: %2%");
			msg % sql_create % sqlite3_errstr(err);
			throw NewGeneException() << newgene_error_description(msg.str());
		}

		int step_result = 0;

		if ((step_result = sqlite3_step(stmt)) != SQLITE_DONE)
		{
			boost::format msg("Cannot execute SQL \"%1%\" in VG import: %2%");
			msg % sql_create % sqlite3_errstr(step_result);
			throw NewGeneException() << newgene_error_description(msg.str());
		}

		if (stmt)
		{
			sqlite3_finalize(stmt);
			stmt = nullptr;
		}

	}

	//executor.success();

	//return executor.succeeded();
	return true;

}

bool Table_VariableGroupData::ImportEnd(sqlite3 * db, WidgetInstanceIdentifier const & identifier, ImportDefinition const & import_definition, OutputModel * output_model_,
										InputModel * input_model_)
{

	return true;

}

std::string Table_VariableGroupData::TableNameFromVGCode(std::string variable_group_code)
{
	std::string variable_group_data_table_name("VG_INSTANCE_DATA_");
	variable_group_data_table_name += variable_group_code;
	return variable_group_data_table_name;
}

std::string Table_VariableGroupData::ViewNameFromCountTemp(int const view_number, int const multiplicity_number)
{
	std::string view_name("vtmp");
	view_name += std::to_string(view_number);
	view_name += "_";
	view_name += std::to_string(multiplicity_number);
	return view_name;
}

std::string Table_VariableGroupData::ViewNameFromCountTempTimeRanged(int const view_number, int const multiplicity_number)
{
	std::string view_name("vtmp_TR");
	view_name += std::to_string(view_number);
	view_name += "_";
	view_name += std::to_string(multiplicity_number);
	return view_name;
}

std::string Table_VariableGroupData::ViewNameFromCount(int const view_number)
{
	std::string view_name("v");
	view_name += std::to_string(view_number);
	return view_name;
}

std::string Table_VariableGroupData::JoinViewNameFromCount(int const join_number)
{
	std::string join_view_name("j");
	join_view_name += std::to_string(join_number);
	return join_view_name;
}

std::string Table_VariableGroupData::JoinViewNameWithTimeRangesFromCount(int const join_number)
{
	std::string join_view_name("jt");
	join_view_name += std::to_string(join_number);
	return join_view_name;
}

Table_VariableGroupData * Table_VariableGroupData::GetInstanceTableFromTableName(sqlite3 * db, InputModel * input_model_, std::string const & table_name)
{

	// TODO: add mutex to model to handle this scenario
	//std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	if (!db)
	{
		return nullptr;
	}

	if (!input_model_)
	{
		return nullptr;
	}

	auto vg_instance_table = std::find_if(input_model_->t_vgp_data_vector.begin(), input_model_->t_vgp_data_vector.end(), [&](std::unique_ptr<Table_VariableGroupData> & test_vg_table)
	{
		if (test_vg_table)
		{
			if (boost::iequals(test_vg_table->table_name, table_name))
			{
				return true;
			}
		}

		return false;
	});

	if (vg_instance_table != input_model_->t_vgp_data_vector.end())
	{
		return vg_instance_table->get();
	}

	return nullptr;

}

bool Table_VariableGroupData::DeleteDataTable(sqlite3 * db, InputModel * input_model_)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	//Executor theExecutor(db);

	if (!db)
	{
		return false;
	}

	if (!input_model_)
	{
		return false;
	}

	WidgetInstanceIdentifier vg;
	input_model_->t_vgp_identifiers.getIdentifierFromStringCode(vg_category_string_code, vg);
	bool dropped_primary_key_metadata = input_model_->t_vgp_data_metadata__primary_keys.DeleteDataTable(db, input_model_, table_name);
	bool dropped_datetime_metadata = input_model_->t_vgp_data_metadata__datetime_columns.DeleteDataTable(db, input_model_, table_name);
	bool dropped_vg_setmember_metadata = input_model_->t_vgp_setmembers.DeleteVG(db, input_model_, vg);

	if (!dropped_primary_key_metadata || !dropped_datetime_metadata || !dropped_vg_setmember_metadata)
	{
		boost::format msg("Unable to delete metadata for variable group");
		throw NewGeneException() << newgene_error_description(msg.str());
	}

	boost::format drop_stmt("DROP TABLE IF EXISTS \"%1%\"");
	drop_stmt % table_name;
	char * errmsg = nullptr;
	sqlite3_exec(db, drop_stmt.str().c_str(), NULL, NULL, &errmsg);

	if (errmsg != nullptr)
	{
		boost::format msg("Unable to delete data for variable group: %1%");
		msg % errmsg;
		sqlite3_free(errmsg);
		throw NewGeneException() << newgene_error_description(msg.str());
	}

	//theExecutor.success();

	//return theExecutor.succeeded();
	return true;

}

bool Table_VariableGroupData::DeleteDmuMemberRows(sqlite3 * db, InputModel * input_model_, WidgetInstanceIdentifier const & dmu_member, std::string const & column_name)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	//Executor theExecutor(db);

	if (!dmu_member.uuid || dmu_member.uuid->empty() || !dmu_member.identifier_parent || !dmu_member.identifier_parent->code || !dmu_member.identifier_parent->uuid)
	{
		return false;
	}

	WidgetInstanceIdentifier dmu_category = *dmu_member.identifier_parent;

	sqlite3_stmt * stmt = NULL;
	boost::format sql("DELETE FROM \"%1%\" WHERE `%2%` = '%3%'");
	sql % table_name % column_name % *dmu_member.uuid;
	int err = sqlite3_prepare_v2(db, sql.str().c_str(), static_cast<int>(sql.str().size()) + 1, &stmt, NULL);

	if (stmt == NULL)
	{
		boost::format msg("Unable to prepare DELETE statement to delete DMU member data from column %1% in table %2% corresponding to DMU member %3% in DMU category %4%: %5%");
		msg % column_name % table_name % Table_DMU_Instance::GetDmuMemberDisplayText(dmu_member) % Table_DMU_Identifier::GetDmuCategoryDisplayText(dmu_category) % sqlite3_errstr(err);
		throw NewGeneException() << newgene_error_description(msg.str());
	}

	int step_result = 0;
	step_result = sqlite3_step(stmt);

	if (step_result != SQLITE_DONE)
	{
		boost::format msg("Unable to execute DELETE statement to delete DMU member data from column %1% in table %2% corresponding to DMU member %3% in DMU category %4%: %5%");
		msg % column_name % table_name % Table_DMU_Instance::GetDmuMemberDisplayText(dmu_member) % Table_DMU_Identifier::GetDmuCategoryDisplayText(dmu_category) % sqlite3_errstr(err);
		throw NewGeneException() << newgene_error_description(msg.str());
	}

	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

	//theExecutor.success();

	//return theExecutor.succeeded();
	return true;

}

bool Table_VariableGroupData::BuildImportDefinition
(
	sqlite3 * db,
	InputModel * input_model_,
	WidgetInstanceIdentifier const & vg,
	std::vector<std::string> const & timeRangeCols,
	std::vector<std::pair<WidgetInstanceIdentifier, std::string>> const & dmusAndCols,
	boost::filesystem::path const & filepathname,
	TIME_GRANULARITY const & the_time_granularity,
	bool const inputFileContainsColumnDescriptions,
	bool const inputFileContainsColumnDataTypes,
	ImportDefinition & definition,
	std::string & errorMsg
)
{

	definition.primary_keys_info.clear();

	if (!vg.uuid || vg.uuid->empty() || !vg.code || vg.code->empty() || !vg.identifier_parent || !vg.identifier_parent->uuid || vg.identifier_parent->uuid->empty())
	{
		boost::format msg("Missing the VG or its associated UOA to refresh in the model.");
		throw NewGeneException() << newgene_error_description(msg.str());
	}

	//std::string table_name = Table_VariableGroupData::TableNameFromVGCode(*vg.code);

	definition.import_type = ImportDefinition::IMPORT_TYPE__INPUT_MODEL;
	definition.input_file = filepathname;
	definition.first_row_is_header_row = true;
	definition.second_row_is_column_description_row = inputFileContainsColumnDescriptions;
	definition.third_row_is_data_type_row = inputFileContainsColumnDataTypes;
	definition.format_qualifiers = ImportDefinition::FORMAT_QUALIFIERS__COMMA_DELIMITED | ImportDefinition::FORMAT_QUALIFIERS__STRINGS_ARE_EITHER_DOUBLEQUOTED_OR_SINGLEQUOTED |
								   ImportDefinition::FORMAT_QUALIFIERS__BACKSLASH_ESCAPE_CHAR;

	Schema schema_input;
	Schema schema_output;

	SchemaVector input_schema_vector;
	SchemaVector output_schema_vector;

	ImportDefinition::ImportMappings mappings;

	{

		std::fstream data_file;
		data_file.open(definition.input_file.c_str(), std::ios::in);

		if (!data_file.is_open() || !data_file.good())
		{
			data_file.close();
			boost::format msg("Cannot open data file \"%1%\"");
			msg % definition.input_file;
			errorMsg = msg.str();
			return false;
		}



		char line[MAX_LINE_SIZE]; // Input data
		char parsedline[MAX_LINE_SIZE]; // A place to store a copy of the input data, but with NULL's separating the fields
		long linenum = 0;

		if (!definition.first_row_is_header_row)
		{
			data_file.close();
			boost::format msg("The first row in the input file must contain the column names.");
			errorMsg = msg.str();
			return false;
		}



		// handle the column names row
		std::vector<std::string> colnames;

		if (definition.first_row_is_header_row && data_file.good())
		{
			++linenum;

			data_file.getline(line, MAX_LINE_SIZE - 1);

			if (!data_file.good())
			{
				data_file.close();
				boost::format msg("Cannot read column names from the data file \"%1%\"");
				msg % definition.input_file;
				errorMsg = msg.str();
				return false;
			}

			boost::split(colnames, line, boost::is_any_of(","));
			std::for_each(colnames.begin(), colnames.end(), std::bind(boost::trim<std::string>, std::placeholders::_1, std::locale()));

		}

		std::set<std::string> testcols(colnames.cbegin(), colnames.cend());

		if (testcols.size() != colnames.size())
		{
			boost::format msg("There are duplicate column names in the input.");
			errorMsg = msg.str();
			return false;
		}



		int nCols = colnames.size();



		// handle the column descriptions row
		std::vector<std::string> coldescriptions(nCols);

		if (definition.second_row_is_column_description_row && data_file.good())
		{

			++linenum;

			data_file.getline(line, MAX_LINE_SIZE - 1);

			if (!data_file.good())
			{
				data_file.close();
				boost::format msg("Cannot read column descriptions from the data file \"%1%\"");
				msg % definition.input_file;
				errorMsg = msg.str();
				return false;
			}

			char * current_line_ptr = line;
			char * parsed_line_ptr = parsedline;
			*parsed_line_ptr = '\0';
			bool stop = false;

			for (int ncol = 0; ncol < nCols; ++ncol)
			{

				// Create temporary SchemaEntry just for the purpose of parsing the column description
				// the same way it's done in the main import routine
				DataFields fields;
				FIELD_TYPE field_type = FIELD_TYPE_STRING_FIXED; // This is a column description field - it's a string
				std::string field_name = colnames[ncol];
				Importer::InstantiateDataFieldInstance(field_type, field_name, fields);
				BaseField & dataField = *fields[0];
				SchemaEntry entry(field_type, colnames[ncol]);
				std::string fieldReadErrorMsg;
				Importer::ReadFieldFromFileStatic(current_line_ptr, parsed_line_ptr, stop, entry, dataField, definition, linenum, ncol + 1, ncol == nCols - 1, fieldReadErrorMsg);

				if (stop)
				{
					boost::format msg("Unable to read field in file: %1%");
					msg % fieldReadErrorMsg;
					errorMsg = msg.str();
					break;
				}

				coldescriptions[ncol] = (dataField.GetStringRef());

			}

			if (stop)
			{
				return false;
			}

			std::for_each(coldescriptions.begin(), coldescriptions.end(), std::bind(boost::trim<std::string>, std::placeholders::_1, std::locale()));

		}

		if (coldescriptions.size() != colnames.size())
		{
			boost::format msg("The number of column descriptions does not match the number of columns.");
			errorMsg = msg.str();
			return false;
		}

		int colseq = 0;
		std::for_each(coldescriptions.begin(), coldescriptions.end(), [&](std::string & coldescription)
		{
			if (coldescription.empty())
			{
				coldescription = colnames[colseq];
			}

			++colseq;
		});



		// handle the column data types
		bool badColTypes = false;
		std::vector<std::string> coltypes(nCols);

		if (definition.third_row_is_data_type_row && data_file.good())
		{

			++linenum;

			data_file.getline(line, MAX_LINE_SIZE - 1);

			if (!data_file.good())
			{
				data_file.close();
				boost::format msg("Cannot read column data types from the data file \"%1%\"");
				msg % definition.input_file;
				errorMsg = msg.str();
				return false;
			}

			boost::split(coltypes, line, boost::is_any_of(","));
			std::for_each(coltypes.begin(), coltypes.end(), std::bind(boost::trim<std::string>, std::placeholders::_1, std::locale()));

			std::for_each(coltypes.begin(), coltypes.end(), [&](std::string & coltype)
			{
				boost::to_lower(coltype);

				if (coltype != "int" && coltype != "int64" && coltype != "string" && coltype != "float")
				{
					badColTypes = true;
				}
			});

		}

		if (coltypes.size() != colnames.size())
		{
			boost::format msg("The number of column types does not match the number of columns.");
			errorMsg = msg.str();
			return false;
		}

		if (badColTypes)
		{
			boost::format msg("Data types must be one of \"int\", \"int64\", \"float\" and \"string\".");
			errorMsg = msg.str();
			return false;
		}



		// Convert from text representation of field type, to a vector of FIELD_TYPE
		std::vector<FIELD_TYPE> fieldtypes;
		bool validtype = true;
		std::for_each(coltypes.cbegin(), coltypes.cend(), [&](std::string coltype)
		{
			if (!validtype)
			{
				return;
			}

			boost::trim(coltype);
			boost::to_lower(coltype);

			if (coltype == "int" || coltype == "int32")
			{
				fieldtypes.push_back(FIELD_TYPE_INT32);
			}
			else if (coltype == "int64")
			{
				fieldtypes.push_back(FIELD_TYPE_INT64);
			}
			else if (coltype == "string")
			{
				fieldtypes.push_back(FIELD_TYPE_STRING_FIXED);
			}
			else if (coltype == "float" || coltype == "double")
			{
				fieldtypes.push_back(FIELD_TYPE_FLOAT);
			}
			else if (coltype.empty())
			{
				// Default to int
				fieldtypes.push_back(FIELD_TYPE_INT32);
			}
			else
			{
				validtype = false;
			}
		});

		if (!validtype)
		{
			boost::format msg("Data types must be one of \"int\", \"int64\", \"float\" and \"string\".");
			errorMsg = msg.str();
			return false;
		}

		// If the table already exists, check that the columns match
		WidgetInstanceIdentifiers existing_column_identifiers = input_model_->t_vgp_setmembers.getIdentifiers(*vg.uuid);

		if (!existing_column_identifiers.empty())
		{

			if (vg.identifier_parent->time_granularity != TIME_GRANULARITY__NONE)
			{
				// Account for the two internal time range columns

				// ************************************************************************************************************ //
				// DN: DATETIME_ROW_START_TODO - set this to the following line (include the "- 2")
				// if ever setting option to display DATETIME_ROW_START/END columns as variable possibilities to end user
				// if (existing_column_identifiers.size() - 2 != colnames.size())
				// ************************************************************************************************************ //

				if (existing_column_identifiers.size() - 2 != colnames.size())
				{
					boost::format msg("The number of columns in the input file does not match the number of columns in the existing data.");
					errorMsg = msg.str();
					return false;
				}

				// ... and remove them for the following test
				existing_column_identifiers.pop_back();
				existing_column_identifiers.pop_back();
			}
			else
			{
				if (existing_column_identifiers.size() != colnames.size())
				{
					boost::format msg("The number of columns in the input file does not match the number of columns in the existing data.");
					errorMsg = msg.str();
					return false;
				}

			}

			bool mismatch = false;
			int index = 0;
			std::for_each(existing_column_identifiers.cbegin(), existing_column_identifiers.cend(), [&](WidgetInstanceIdentifier const & existing_column_identifier)
			{

				if (mismatch)
				{
					return;
				}

				if (!existing_column_identifier.code || existing_column_identifier.code->empty())
				{
					mismatch = true;
					return;
				}

				if (*existing_column_identifier.code != colnames[index])
				{
					mismatch = true;
					return;
				}

				++index;

			});

			if (mismatch)
			{
				boost::format msg("The column names in the input file do not match the column names for the existing data.");
				errorMsg = msg.str();
				return false;
			}

		}



		// ************************************************************************************************* //
		// Create the import definition itself, field-by-field
		// ************************************************************************************************* //

		int number_primary_key_cols = 0;
		int number_time_range_cols = 0;
		int colindex = 0;
		std::map<std::string, int> timeRangeColName_To_Index;
		bool time_ranges_are_strings = false;
		std::for_each(colnames.cbegin(), colnames.cend(), [&](std::string const & colname)
		{

			std::string test_col_name(colname);

			if (!Validation::ValidateColumnName(test_col_name, std::string("variable group"), true, errorMsg))
			{
				boost::format msg("%1%");
				msg % errorMsg.c_str();
				throw NewGeneException() << newgene_error_description(msg.str());
			}

			if (test_col_name != colname)
			{
				boost::format msg("Invalid column name");
				throw NewGeneException() << newgene_error_description(msg.str());
			}

			// In this block, determine if this is a primary key field
			bool primary_key_col = true;

			// Also in this block, determine if this is a time range column
			bool time_range_col = true;

			WidgetInstanceIdentifier the_dmu;

			// Determine if this is a primary key column
			if (std::find_if(dmusAndCols.cbegin(), dmusAndCols.cend(), [&](std::pair<WidgetInstanceIdentifier, std::string> const & dmuAndCol) -> bool
		{
			if (dmuAndCol.second == colname)
				{
					the_dmu = dmuAndCol.first;
					return true;
				}
				return false;
			}) == dmusAndCols.cend())
			{
				// Not a key field
				primary_key_col = false;
			}
			else
			{
				++number_primary_key_cols;
			}

			if (primary_key_col)
			{

				if (!the_dmu.code)
				{
					boost::format msg("The DMU associated with a primary key column specifications is invalid in the variable group import.");
					throw NewGeneException() << newgene_error_description(msg.str());
				}

				if (!Validation::ValidateDmuCode(*the_dmu.code, errorMsg))
				{
					boost::format msg("%1%");
					msg % errorMsg.c_str();
					throw NewGeneException() << newgene_error_description(msg.str());
				}

				if (the_dmu.longhand)
				{
					if (!Validation::ValidateDmuDescription(*the_dmu.longhand, errorMsg))
					{
						boost::format msg("%1%");
						msg % errorMsg.c_str();
						throw NewGeneException() << newgene_error_description(msg.str());
					}
				}

				if (fieldtypes[colindex] == FIELD_TYPE_FLOAT)
				{
					boost::format msg("A primary key column cannot be defined as a floating-point value type.");
					throw NewGeneException() << newgene_error_description(msg.str());
				}

			}

			// Determine if this is a time range column
			if (std::find_if(timeRangeCols.cbegin(), timeRangeCols.cend(), [&](std::string const & timeRangeCol) -> bool
		{
			if (timeRangeCol == colname)
				{
					return true;
				}
				return false;
			}) == timeRangeCols.cend())
			{
				time_range_col = false;
			}
			else
			{
				++number_time_range_cols;
			}

			if (time_range_col)
			{

				timeRangeColName_To_Index[colname] = colindex;

				// Check validity of the type of the time range column

				switch (the_time_granularity)
				{

					case TIME_GRANULARITY__DAY:
						{
							if (IsFieldTypeString(fieldtypes[colindex]))
							{
								if (timeRangeCols.size() != 2 && timeRangeCols.size() != 1)
								{
									boost::format
									msg("There must be 1 or 2 string time range columns when any of the time range columns are defined as 'string' in the input file.  Also, the corresponding option must be selected when you perform the data import (in case you did not do so).");
									throw NewGeneException() << newgene_error_description(msg.str());
								}

								time_ranges_are_strings = true;
							}
							else
							{
								if (!IsFieldTypeInt(fieldtypes[colindex]))
								{
									boost::format msg("The time range columns must be an integral type.");
									throw NewGeneException() << newgene_error_description(msg.str());
								}

								if (timeRangeCols.size() != 6 && timeRangeCols.size() != 3)
								{
									boost::format
									msg("There must be 3 or 6 time range columns when any of the time range columns are defined as 'integer' in the raw data file.  If you chose the option for 'string' columns for the time range column/s, please ensure that there is a row in the raw data file that contains column data types, select the corresponding option when you import the data, and make sure to label the time columns as 'string' in the data file.");
									throw NewGeneException() << newgene_error_description(msg.str());
								}
							}
						}
						break;

					case TIME_GRANULARITY__YEAR:
						{
							if (!IsFieldTypeInt(fieldtypes[colindex]) && !IsFieldTypeString(fieldtypes[colindex]))
							{
								boost::format msg("The time range column/s must be either a string or an integral type.");
								throw NewGeneException() << newgene_error_description(msg.str());
							}

							if (IsFieldTypeString(fieldtypes[colindex]))
							{
								if (timeRangeCols.size() != 2 && timeRangeCols.size() != 1)
								{
									boost::format msg("There must be 1 or 2 string time range columns.");
									throw NewGeneException() << newgene_error_description(msg.str());
								}

								time_ranges_are_strings = true;
							}
							else
							{
								if (!IsFieldTypeInt(fieldtypes[colindex]))
								{
									boost::format msg("The time range column/s must be an integral type.");
									throw NewGeneException() << newgene_error_description(msg.str());
								}

								if (timeRangeCols.size() != 2 && timeRangeCols.size() != 1)
								{
									boost::format msg("There must be 1 or 2 time range columns.");
									throw NewGeneException() << newgene_error_description(msg.str());
								}
							}
						}
						break;

					case TIME_GRANULARITY__MONTH:
						{
							if (IsFieldTypeString(fieldtypes[colindex]))
							{
								if (timeRangeCols.size() != 2 && timeRangeCols.size() != 1)
								{
									boost::format
									msg("There must be 1 or 2 string time range columns when any of the time range columns are defined as 'string' in the input file.  Also, the corresponding option must be selected when you perform the data import (in case you did not do so).");
									throw NewGeneException() << newgene_error_description(msg.str());
								}

								time_ranges_are_strings = true;
							}
							else
							{
								if (!IsFieldTypeInt(fieldtypes[colindex]))
								{
									boost::format msg("The time range columns must be an integral type.");
									throw NewGeneException() << newgene_error_description(msg.str());
								}

								if (timeRangeCols.size() != 4 && timeRangeCols.size() != 2)
								{
									boost::format
									msg("There must be 2 or 4 time range columns when any of the time range columns are defined as 'integer' in the raw data file.  If you chose the option for 'string' columns for the time range column/s, please ensure that there is a row in the raw data file that contains column data types, select the corresponding option when you import the data, and make sure to label the time columns as 'string' in the data file.");
									throw NewGeneException() << newgene_error_description(msg.str());
								}
							}
						}
						break;

					default:
						{
							boost::format msg("Invalid time range specification in determination of time range column data types during data import of variable group.");
							throw NewGeneException() << newgene_error_description(msg.str());
						}
						break;

				}

			}

			if (primary_key_col)
			{

				if (IsFieldTypeFloat(fieldtypes[colindex]))
				{
					boost::format msg("A primary key column cannot be defined as a floating-point value type.");
					throw NewGeneException() << newgene_error_description(msg.str());
				}

				if (IsFieldTypeInt(fieldtypes[colindex]))
				{
					fieldtypes[colindex] = FIELD_TYPE_DMU_MEMBER_UUID_NUMERIC;
				}

				if (IsFieldTypeString(fieldtypes[colindex]))
				{
					fieldtypes[colindex] = FIELD_TYPE_DMU_MEMBER_UUID_STRING;
				}

				if (time_range_col)
				{
					// Time range column data type overrides primary key

					// no-op here

					// **************************************************************************** //
					// Note! The field type, in the schema entry, will be set BELOW,
					// where the time range columns are further validated and parsed,
					// overriding the field type being set here
					// **************************************************************************** //
				}

				SchemaEntry inputSchemaEntry(*the_dmu.code, fieldtypes[colindex], colname);
				SchemaEntry outputSchemaEntry(*the_dmu.code, fieldtypes[colindex], colname, true);
				inputSchemaEntry.field_description = coldescriptions[colindex];
				outputSchemaEntry.field_description = coldescriptions[colindex];
				input_schema_vector.push_back(inputSchemaEntry);
				output_schema_vector.push_back(outputSchemaEntry);

				definition.primary_keys_info.push_back(std::make_tuple(the_dmu, colname, fieldtypes[colindex]));
			}
			else
			{
				if (time_range_col)
				{
					// Time range column data type overrides primary key

					// no-op here

					// **************************************************************************** //
					// Note! The field type, in the schema entry, will be set BELOW,
					// where the time range columns are further validated and parsed,
					// overriding the field type being set here
					// **************************************************************************** //
				}

				SchemaEntry inputSchemaEntry(fieldtypes[colindex], colname);
				SchemaEntry outputSchemaEntry(fieldtypes[colindex], colname, true);
				inputSchemaEntry.field_description = coldescriptions[colindex];
				outputSchemaEntry.field_description = coldescriptions[colindex];
				input_schema_vector.push_back(inputSchemaEntry);
				output_schema_vector.push_back(outputSchemaEntry);
			}

			++colindex;

		});



		// No double-counting, because all column names are guaranteed unique,
		// with a test both on the client side (for duplicate DMU primary key column names)
		// and a test above (for any duplicate column names)
		if (number_primary_key_cols != dmusAndCols.size())
		{
			boost::format msg("Not all specified DMU primary key columns could be found in the input file.");
			errorMsg = msg.str();
			return false;
		}

		bool timeRangeHasOnlyStartDate = false;

		if (the_time_granularity == TIME_GRANULARITY__YEAR)
		{
			if (number_time_range_cols == 1)
			{
				// special case: this is OK, just one column
				timeRangeHasOnlyStartDate = true;
			}
		}
		else if (the_time_granularity == TIME_GRANULARITY__MONTH)
		{
			if (time_ranges_are_strings)
			{
				// strings provided in individual columns, such as "11/1990"
				if (number_time_range_cols == 1)
				{
					// special case: this is OK, just one column
					timeRangeHasOnlyStartDate = true;
				}
			}
			else
			{
				// ints provided, such as one column with "11" and another column with "1990"
				if (number_time_range_cols == 2)
				{
					// special case: this is OK, just one column
					timeRangeHasOnlyStartDate = true;
				}
			}
		}
		else if (the_time_granularity == TIME_GRANULARITY__DAY)
		{
			if (time_ranges_are_strings)
			{
				// strings provided in individual columns, such as "11/12/1990"
				if (number_time_range_cols == 1)
				{
					// special case: this is OK, just one column
					timeRangeHasOnlyStartDate = true;
				}
			}
			else
			{
				// ints provided, such as one column with "11", another column with "12", and another column with "1990"
				if (number_time_range_cols == 3)
				{
					// special case: this is OK, just one column
					timeRangeHasOnlyStartDate = true;
				}
			}
		}

		// Prepare output time range columns, in case they're needed
		// (The code below also prepares input time range columns for the mapping)
		std::string timeRangeStartFieldNameAndDescription = Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeStartColumnName;
		std::string timeRangeEndFieldNameAndDescription = Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeEndColumnName;
		SchemaEntry outputTimeRangeStartEntry(FIELD_TYPE_TIME_RANGE_OUTPUT_START_DATETIME, timeRangeStartFieldNameAndDescription, true);
		SchemaEntry outputTimeRangeEndEntry(FIELD_TYPE_TIME_RANGE_OUTPUT_END_DATETIME, timeRangeEndFieldNameAndDescription, true);
		outputTimeRangeStartEntry.field_description = timeRangeStartFieldNameAndDescription;
		outputTimeRangeEndEntry.field_description = timeRangeEndFieldNameAndDescription;
		outputTimeRangeStartEntry.SetIsTimeRange(true);
		outputTimeRangeEndEntry.SetIsTimeRange(true);

		// First, the time range mapping
		switch (the_time_granularity)
		{

			case TIME_GRANULARITY__DAY:
				{
					// First, add the output time range columns to the schema
					output_schema_vector.push_back(outputTimeRangeStartEntry);
					output_schema_vector.push_back(outputTimeRangeEndEntry);

					if (timeRangeCols.size() == 6 || timeRangeCols.size() == 3)
					{

						// Integer data from user:
						// One column for YEAR
						// One column for MONTH
						// One column for DAY
						// ... for both start & end dates, or just for start dates if that is all the user provides

						// Now add the input time range columns to the mapping
						// (they have already been added to the schema, because they are just regular input columns)
						std::shared_ptr<TimeRangeFieldMapping> time_range_mapping;

						if (!timeRangeHasOnlyStartDate)
						{
							time_range_mapping = std::make_shared<TimeRangeFieldMapping>(TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__INTS__DAY__FROM__START_DAY__TO__END_DAY);
						}
						else
						{
							time_range_mapping = std::make_shared<TimeRangeFieldMapping>(TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__INTS__DAY__START_DAY_ONLY);
						}

						FieldTypeEntries input_file_fields;
						FieldTypeEntries output_table_fields;
						int colindex = 0;
						FieldTypeEntry input_time_field__YearStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_YEAR);
						FieldTypeEntry input_time_field__MonthStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_MONTH);
						FieldTypeEntry input_time_field__DayStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_DAY);
						FieldTypeEntry input_time_field__YearEnd;
						FieldTypeEntry input_time_field__MonthEnd;
						FieldTypeEntry input_time_field__DayEnd;

						if (!timeRangeHasOnlyStartDate)
						{
							input_time_field__YearEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_YEAR);
							input_time_field__MonthEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_MONTH);
							input_time_field__DayEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_DAY);
						}

						input_file_fields.push_back(input_time_field__YearStart);
						input_file_fields.push_back(input_time_field__MonthStart);
						input_file_fields.push_back(input_time_field__DayStart);

						if (!timeRangeHasOnlyStartDate)
						{
							input_file_fields.push_back(input_time_field__YearEnd);
							input_file_fields.push_back(input_time_field__MonthEnd);
							input_file_fields.push_back(input_time_field__DayEnd);
						}

						FieldTypeEntry output_time_field__DayStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeStartColumnName),
								FIELD_TYPE_TIME_RANGE_OUTPUT_START_DATETIME);
						FieldTypeEntry output_time_field__DayEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeEndColumnName),
								FIELD_TYPE_TIME_RANGE_OUTPUT_END_DATETIME);
						output_table_fields.push_back(output_time_field__DayStart);
						output_table_fields.push_back(output_time_field__DayEnd);
						time_range_mapping->input_file_fields = input_file_fields;
						time_range_mapping->output_table_fields = output_table_fields;
						mappings.push_back(time_range_mapping);

						// Now modify the field type of these time range columns in the schema (overriding the primary key field type, if applicable)
						colindex = 0;
						int colindex_yearStart = timeRangeColName_To_Index[timeRangeCols[colindex++]];
						int colindex_monthStart = timeRangeColName_To_Index[timeRangeCols[colindex++]];
						int colindex_dayStart = timeRangeColName_To_Index[timeRangeCols[colindex++]];
						int colindex_yearEnd = 0;
						int colindex_monthEnd = 0;
						int colindex_dayEnd = 0;

						if (!timeRangeHasOnlyStartDate)
						{
							colindex_yearEnd = timeRangeColName_To_Index[timeRangeCols[colindex++]];
							colindex_monthEnd = timeRangeColName_To_Index[timeRangeCols[colindex++]];
							colindex_dayEnd = timeRangeColName_To_Index[timeRangeCols[colindex++]];
						}

						if (input_schema_vector[colindex_yearStart].IsPrimaryKey())
						{
							input_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_YEAR;
							output_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_YEAR;
						}
						else
						{
							input_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_YEAR;
							output_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_YEAR;
						}

						if (input_schema_vector[colindex_monthStart].IsPrimaryKey())
						{
							input_schema_vector[colindex_monthStart].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_MONTH;
							output_schema_vector[colindex_monthStart].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_MONTH;
						}
						else
						{
							input_schema_vector[colindex_monthStart].field_type = FIELD_TYPE_MONTH;
							output_schema_vector[colindex_monthStart].field_type = FIELD_TYPE_MONTH;
						}

						if (input_schema_vector[colindex_dayStart].IsPrimaryKey())
						{
							input_schema_vector[colindex_dayStart].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DAY;
							output_schema_vector[colindex_dayStart].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DAY;
						}
						else
						{
							input_schema_vector[colindex_dayStart].field_type = FIELD_TYPE_DAY;
							output_schema_vector[colindex_dayStart].field_type = FIELD_TYPE_DAY;
						}

						if (!timeRangeHasOnlyStartDate)
						{
							if (input_schema_vector[colindex_yearEnd].IsPrimaryKey())
							{
								input_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_YEAR;
								output_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_YEAR;
							}
							else
							{
								input_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_YEAR;
								output_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_YEAR;
							}

							if (input_schema_vector[colindex_monthEnd].IsPrimaryKey())
							{
								input_schema_vector[colindex_monthEnd].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_MONTH;
								output_schema_vector[colindex_monthEnd].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_MONTH;
							}
							else
							{
								input_schema_vector[colindex_monthEnd].field_type = FIELD_TYPE_MONTH;
								output_schema_vector[colindex_monthEnd].field_type = FIELD_TYPE_MONTH;
							}

							if (input_schema_vector[colindex_dayEnd].IsPrimaryKey())
							{
								input_schema_vector[colindex_dayEnd].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DAY;
								output_schema_vector[colindex_dayEnd].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DAY;
							}
							else
							{
								input_schema_vector[colindex_dayEnd].field_type = FIELD_TYPE_DAY;
								output_schema_vector[colindex_dayEnd].field_type = FIELD_TYPE_DAY;
							}
						}
					}
					else
					{

						// String text from user.
						// One for start date and one for end date.

						// Now add the input time range columns to the mapping
						// (they have already been added to the schema, because they are just regular input columns)
						std::shared_ptr<TimeRangeFieldMapping> time_range_mapping;

						if (!timeRangeHasOnlyStartDate)
						{
							time_range_mapping = std::make_shared<TimeRangeFieldMapping>(TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__STRINGS__DAY__FROM__START_DAY__TO__END_DAY);
						}
						else
						{
							time_range_mapping = std::make_shared<TimeRangeFieldMapping>(TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__STRINGS__DAY__START_DAY_ONLY);
						}

						FieldTypeEntries input_file_fields;
						FieldTypeEntries output_table_fields;
						int colindex = 0;
						FieldTypeEntry input_time_field__Start = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_DATETIME_STRING);
						FieldTypeEntry input_time_field__End;

						if (!timeRangeHasOnlyStartDate)
						{
							input_time_field__End = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_DATETIME_STRING);
						}

						input_file_fields.push_back(input_time_field__Start);

						if (!timeRangeHasOnlyStartDate)
						{
							input_file_fields.push_back(input_time_field__End);
						}

						FieldTypeEntry output_time_field__DayStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeStartColumnName),
								FIELD_TYPE_TIME_RANGE_OUTPUT_START_DATETIME);
						FieldTypeEntry output_time_field__DayEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeEndColumnName),
								FIELD_TYPE_TIME_RANGE_OUTPUT_END_DATETIME);
						output_table_fields.push_back(output_time_field__DayStart);
						output_table_fields.push_back(output_time_field__DayEnd);
						time_range_mapping->input_file_fields = input_file_fields;
						time_range_mapping->output_table_fields = output_table_fields;
						mappings.push_back(time_range_mapping);

						// Now modify the field type of these time range columns in the schema (overriding the primary key field type, if applicable)
						colindex = 0;
						int colindex_End = 0;
						int colindex_Start = timeRangeColName_To_Index[timeRangeCols[colindex++]];

						if (!timeRangeHasOnlyStartDate)
						{
							colindex_End = timeRangeColName_To_Index[timeRangeCols[colindex++]];
						}

						if (input_schema_vector[colindex_Start].IsPrimaryKey())
						{
							input_schema_vector[colindex_Start].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DATETIME_STRING;
							output_schema_vector[colindex_Start].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DATETIME_STRING;
						}
						else
						{
							input_schema_vector[colindex_Start].field_type = FIELD_TYPE_DATETIME_STRING;
							output_schema_vector[colindex_Start].field_type = FIELD_TYPE_DATETIME_STRING;
						}

						if (!timeRangeHasOnlyStartDate)
						{
							if (input_schema_vector[colindex_End].IsPrimaryKey())
							{
								input_schema_vector[colindex_End].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DATETIME_STRING;
								output_schema_vector[colindex_End].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DATETIME_STRING;
							}
							else
							{
								input_schema_vector[colindex_End].field_type = FIELD_TYPE_DATETIME_STRING;
								output_schema_vector[colindex_End].field_type = FIELD_TYPE_DATETIME_STRING;
							}
						}

					}

				}
				break;

			case TIME_GRANULARITY__MONTH:
				{

					// First, add the output time range columns to the schema
					output_schema_vector.push_back(outputTimeRangeStartEntry);
					output_schema_vector.push_back(outputTimeRangeEndEntry);

					if (timeRangeCols.size() == 4 || (timeRangeCols.size() == 2 && !time_ranges_are_strings))
					{

						// Integer data from user:
						// One column for YEAR
						// One column for MONTH
						// ... for both start & end dates, or just for start dates if that is all the user provides

						// Now add the input time range columns to the mapping
						// (they have already been added to the schema, because they are just regular input columns)
						std::shared_ptr<TimeRangeFieldMapping> time_range_mapping;

						if (!timeRangeHasOnlyStartDate)
						{
							time_range_mapping = std::make_shared<TimeRangeFieldMapping>(TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__INTS__MONTH__FROM__START_MONTH__TO__END_MONTH);
						}
						else
						{
							time_range_mapping = std::make_shared<TimeRangeFieldMapping>(TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__INTS__MONTH__START_MONTH_ONLY);
						}

						FieldTypeEntries input_file_fields;
						FieldTypeEntries output_table_fields;
						int colindex = 0;
						FieldTypeEntry input_time_field__YearStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_YEAR);
						FieldTypeEntry input_time_field__MonthStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_MONTH);
						FieldTypeEntry input_time_field__YearEnd;
						FieldTypeEntry input_time_field__MonthEnd;

						if (!timeRangeHasOnlyStartDate)
						{
							input_time_field__YearEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_YEAR);
							input_time_field__MonthEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_MONTH);
						}

						input_file_fields.push_back(input_time_field__YearStart);
						input_file_fields.push_back(input_time_field__MonthStart);

						if (!timeRangeHasOnlyStartDate)
						{
							input_file_fields.push_back(input_time_field__YearEnd);
							input_file_fields.push_back(input_time_field__MonthEnd);
						}

						FieldTypeEntry output_time_field__DayStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeStartColumnName),
								FIELD_TYPE_TIME_RANGE_OUTPUT_START_DATETIME);
						FieldTypeEntry output_time_field__DayEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeEndColumnName),
								FIELD_TYPE_TIME_RANGE_OUTPUT_END_DATETIME);
						output_table_fields.push_back(output_time_field__DayStart);
						output_table_fields.push_back(output_time_field__DayEnd);
						time_range_mapping->input_file_fields = input_file_fields;
						time_range_mapping->output_table_fields = output_table_fields;
						mappings.push_back(time_range_mapping);

						// Now modify the field type of these time range columns in the schema (overriding the primary key field type, if applicable)
						colindex = 0;
						int colindex_yearStart = timeRangeColName_To_Index[timeRangeCols[colindex++]];
						int colindex_monthStart = timeRangeColName_To_Index[timeRangeCols[colindex++]];
						int colindex_yearEnd = 0;
						int colindex_monthEnd = 0;

						if (!timeRangeHasOnlyStartDate)
						{
							colindex_yearEnd = timeRangeColName_To_Index[timeRangeCols[colindex++]];
							colindex_monthEnd = timeRangeColName_To_Index[timeRangeCols[colindex++]];
						}

						if (input_schema_vector[colindex_yearStart].IsPrimaryKey())
						{
							input_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_YEAR;
							output_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_YEAR;
						}
						else
						{
							input_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_YEAR;
							output_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_YEAR;
						}

						if (input_schema_vector[colindex_monthStart].IsPrimaryKey())
						{
							input_schema_vector[colindex_monthStart].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_MONTH;
							output_schema_vector[colindex_monthStart].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_MONTH;
						}
						else
						{
							input_schema_vector[colindex_monthStart].field_type = FIELD_TYPE_MONTH;
							output_schema_vector[colindex_monthStart].field_type = FIELD_TYPE_MONTH;
						}

						if (!timeRangeHasOnlyStartDate)
						{
							if (input_schema_vector[colindex_yearEnd].IsPrimaryKey())
							{
								input_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_YEAR;
								output_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_YEAR;
							}
							else
							{
								input_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_YEAR;
								output_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_YEAR;
							}

							if (input_schema_vector[colindex_monthEnd].IsPrimaryKey())
							{
								input_schema_vector[colindex_monthEnd].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_MONTH;
								output_schema_vector[colindex_monthEnd].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_MONTH;
							}
							else
							{
								input_schema_vector[colindex_monthEnd].field_type = FIELD_TYPE_MONTH;
								output_schema_vector[colindex_monthEnd].field_type = FIELD_TYPE_MONTH;
							}
						}
					}
					else
					{

						// String text from user.
						// One for start date and one for end date.

						// Now add the input time range columns to the mapping
						// (they have already been added to the schema, because they are just regular input columns)
						std::shared_ptr<TimeRangeFieldMapping> time_range_mapping;

						if (!timeRangeHasOnlyStartDate)
						{
							time_range_mapping = std::make_shared<TimeRangeFieldMapping>(TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__STRINGS__MONTH__FROM__START_MONTH__TO__END_MONTH);
						}
						else
						{
							time_range_mapping = std::make_shared<TimeRangeFieldMapping>(TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__STRINGS__MONTH__START_MONTH_ONLY);
						}

						FieldTypeEntries input_file_fields;
						FieldTypeEntries output_table_fields;
						int colindex = 0;
						FieldTypeEntry input_time_field__Start = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_DATETIME_STRING);
						FieldTypeEntry input_time_field__End;

						if (!timeRangeHasOnlyStartDate)
						{
							input_time_field__End = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_DATETIME_STRING);
						}

						input_file_fields.push_back(input_time_field__Start);

						if (!timeRangeHasOnlyStartDate)
						{
							input_file_fields.push_back(input_time_field__End);
						}

						FieldTypeEntry output_time_field__DayStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeStartColumnName),
								FIELD_TYPE_TIME_RANGE_OUTPUT_START_DATETIME);
						FieldTypeEntry output_time_field__DayEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeEndColumnName),
								FIELD_TYPE_TIME_RANGE_OUTPUT_END_DATETIME);
						output_table_fields.push_back(output_time_field__DayStart);
						output_table_fields.push_back(output_time_field__DayEnd);
						time_range_mapping->input_file_fields = input_file_fields;
						time_range_mapping->output_table_fields = output_table_fields;
						mappings.push_back(time_range_mapping);

						// Now modify the field type of these time range columns in the schema (overriding the primary key field type, if applicable)
						colindex = 0;
						int colindex_Start = timeRangeColName_To_Index[timeRangeCols[colindex++]];
						int colindex_End = 0;

						if (!timeRangeHasOnlyStartDate)
						{
							colindex_End = timeRangeColName_To_Index[timeRangeCols[colindex++]];
						}

						if (input_schema_vector[colindex_Start].IsPrimaryKey())
						{
							input_schema_vector[colindex_Start].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DATETIME_STRING;
							output_schema_vector[colindex_Start].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DATETIME_STRING;
						}
						else
						{
							input_schema_vector[colindex_Start].field_type = FIELD_TYPE_DATETIME_STRING;
							output_schema_vector[colindex_Start].field_type = FIELD_TYPE_DATETIME_STRING;
						}

						if (!timeRangeHasOnlyStartDate)
						{
							if (input_schema_vector[colindex_End].IsPrimaryKey())
							{
								input_schema_vector[colindex_End].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DATETIME_STRING;
								output_schema_vector[colindex_End].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DATETIME_STRING;
							}
							else
							{
								input_schema_vector[colindex_End].field_type = FIELD_TYPE_DATETIME_STRING;
								output_schema_vector[colindex_End].field_type = FIELD_TYPE_DATETIME_STRING;
							}
						}

					}

				}
				break;

			case TIME_GRANULARITY__YEAR:
				{

					// First, add the output time range columns to the schema
					output_schema_vector.push_back(outputTimeRangeStartEntry);
					output_schema_vector.push_back(outputTimeRangeEndEntry);

					// Now add the input time range columns to the mapping
					// (they have already been added to the schema, because they are just regular input columns)
					if (timeRangeCols.size() == 1)
					{

						// We do not have enough info yet to know if user intends to pull the year portion
						// from a string such as "11/1/2000", or whether they intend just to enter an int such as "2000"
						// ... until a checkbox is provided in the UI.
						// So for now, assume int
						if (false)
						{
							// Treat as text string if it doesn't validate as an integer

							std::shared_ptr<TimeRangeFieldMapping> time_range_mapping = std::make_shared<TimeRangeFieldMapping>
									(TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__STRINGS__YEAR__START_YEAR_ONLY);
							FieldTypeEntries input_file_fields;
							FieldTypeEntries output_table_fields;
							int colindex = 0;
							FieldTypeEntry input_time_field__Year = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_DATETIME_STRING);
							input_file_fields.push_back(input_time_field__Year);
							FieldTypeEntry output_time_field__YearStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeStartColumnName),
									FIELD_TYPE_TIME_RANGE_OUTPUT_START_DATETIME);
							FieldTypeEntry output_time_field__YearEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeEndColumnName),
									FIELD_TYPE_TIME_RANGE_OUTPUT_END_DATETIME);
							output_table_fields.push_back(output_time_field__YearStart);
							output_table_fields.push_back(output_time_field__YearEnd);
							time_range_mapping->input_file_fields = input_file_fields;
							time_range_mapping->output_table_fields = output_table_fields;
							mappings.push_back(time_range_mapping);

							// Now modify the field type of these time range columns in the schema (overriding the primary key field type, if applicable)
							colindex = 0;
							int colindex_year = timeRangeColName_To_Index[timeRangeCols[colindex++]];

							if (input_schema_vector[colindex_year].IsPrimaryKey())
							{
								input_schema_vector[colindex_year].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DATETIME_STRING;
								output_schema_vector[colindex_year].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DATETIME_STRING;
							}
							else
							{
								input_schema_vector[colindex_year].field_type = FIELD_TYPE_DATETIME_STRING;
								output_schema_vector[colindex_year].field_type = FIELD_TYPE_DATETIME_STRING;
							}
						}
						else
						{
							std::shared_ptr<TimeRangeFieldMapping> time_range_mapping = std::make_shared<TimeRangeFieldMapping>
									(TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__INTS__YEAR__START_YEAR_ONLY);
							FieldTypeEntries input_file_fields;
							FieldTypeEntries output_table_fields;
							int colindex = 0;
							FieldTypeEntry input_time_field__Year = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_YEAR);
							input_file_fields.push_back(input_time_field__Year);
							FieldTypeEntry output_time_field__YearStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeStartColumnName),
									FIELD_TYPE_TIME_RANGE_OUTPUT_START_DATETIME);
							FieldTypeEntry output_time_field__YearEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeEndColumnName),
									FIELD_TYPE_TIME_RANGE_OUTPUT_END_DATETIME);
							output_table_fields.push_back(output_time_field__YearStart);
							output_table_fields.push_back(output_time_field__YearEnd);
							time_range_mapping->input_file_fields = input_file_fields;
							time_range_mapping->output_table_fields = output_table_fields;
							mappings.push_back(time_range_mapping);

							// Now modify the field type of these time range columns in the schema (overriding the primary key field type, if applicable)
							colindex = 0;
							int colindex_year = timeRangeColName_To_Index[timeRangeCols[colindex++]];

							if (input_schema_vector[colindex_year].IsPrimaryKey())
							{
								input_schema_vector[colindex_year].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_YEAR;
								output_schema_vector[colindex_year].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_YEAR;
							}
							else
							{
								input_schema_vector[colindex_year].field_type = FIELD_TYPE_YEAR;
								output_schema_vector[colindex_year].field_type = FIELD_TYPE_YEAR;
							}
						}

					}
					else
					{

						// We do not have enough info yet to know if user intends to pull the year portion
						// from a string such as "11/1/2000", or whether they intend just to enter an int such as "2000"
						// ... until a checkbox is provided in the UI.
						// So for now, assume int
						if (false)
						{
							// Treat as text string

							std::shared_ptr<TimeRangeFieldMapping> time_range_mapping = std::make_shared<TimeRangeFieldMapping>
									(TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__STRINGS__YEAR__FROM__START_YEAR__TO__END_YEAR);
							FieldTypeEntries input_file_fields;
							FieldTypeEntries output_table_fields;
							int colindex = 0;
							FieldTypeEntry input_time_field__YearStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_DATETIME_STRING);
							FieldTypeEntry input_time_field__YearEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_DATETIME_STRING);
							input_file_fields.push_back(input_time_field__YearStart);
							input_file_fields.push_back(input_time_field__YearEnd);
							FieldTypeEntry output_time_field__YearStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeStartColumnName),
									FIELD_TYPE_TIME_RANGE_OUTPUT_START_DATETIME);
							FieldTypeEntry output_time_field__YearEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeEndColumnName),
									FIELD_TYPE_TIME_RANGE_OUTPUT_END_DATETIME);
							output_table_fields.push_back(output_time_field__YearStart);
							output_table_fields.push_back(output_time_field__YearEnd);
							time_range_mapping->input_file_fields = input_file_fields;
							time_range_mapping->output_table_fields = output_table_fields;
							mappings.push_back(time_range_mapping);

							// Now modify the field type of these time range columns in the schema (overriding the primary key field type, if applicable)
							colindex = 0;
							int colindex_yearStart = timeRangeColName_To_Index[timeRangeCols[colindex++]];
							int colindex_yearEnd = timeRangeColName_To_Index[timeRangeCols[colindex++]];

							if (input_schema_vector[colindex_yearStart].IsPrimaryKey())
							{
								input_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DATETIME_STRING;
								output_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DATETIME_STRING;
							}
							else
							{
								input_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_DATETIME_STRING;
								output_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_DATETIME_STRING;
							}

							if (input_schema_vector[colindex_yearEnd].IsPrimaryKey())
							{
								input_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DATETIME_STRING;
								output_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_DATETIME_STRING;
							}
							else
							{
								input_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_DATETIME_STRING;
								output_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_DATETIME_STRING;
							}

						}
						else
						{

							std::shared_ptr<TimeRangeFieldMapping> time_range_mapping = std::make_shared<TimeRangeFieldMapping>
									(TimeRangeFieldMapping::TIME_RANGE_FIELD_MAPPING_TYPE__INTS__YEAR__FROM__START_YEAR__TO__END_YEAR);
							FieldTypeEntries input_file_fields;
							FieldTypeEntries output_table_fields;
							int colindex = 0;
							FieldTypeEntry input_time_field__YearStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_YEAR);
							FieldTypeEntry input_time_field__YearEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, timeRangeCols[colindex++]), FIELD_TYPE_YEAR);
							input_file_fields.push_back(input_time_field__YearStart);
							input_file_fields.push_back(input_time_field__YearEnd);
							FieldTypeEntry output_time_field__YearStart = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeStartColumnName),
									FIELD_TYPE_TIME_RANGE_OUTPUT_START_DATETIME);
							FieldTypeEntry output_time_field__YearEnd = std::make_pair(NameOrIndex(NameOrIndex::NAME, Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeEndColumnName),
									FIELD_TYPE_TIME_RANGE_OUTPUT_END_DATETIME);
							output_table_fields.push_back(output_time_field__YearStart);
							output_table_fields.push_back(output_time_field__YearEnd);
							time_range_mapping->input_file_fields = input_file_fields;
							time_range_mapping->output_table_fields = output_table_fields;
							mappings.push_back(time_range_mapping);

							// Now modify the field type of these time range columns in the schema (overriding the primary key field type, if applicable)
							colindex = 0;
							int colindex_yearStart = timeRangeColName_To_Index[timeRangeCols[colindex++]];
							int colindex_yearEnd = timeRangeColName_To_Index[timeRangeCols[colindex++]];

							if (input_schema_vector[colindex_yearStart].IsPrimaryKey())
							{
								input_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_YEAR;
								output_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_YEAR;
							}
							else
							{
								input_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_YEAR;
								output_schema_vector[colindex_yearStart].field_type = FIELD_TYPE_YEAR;
							}

							if (input_schema_vector[colindex_yearEnd].IsPrimaryKey())
							{
								input_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_YEAR;
								output_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_DMU_PRIMARY_KEY_AND_YEAR;
							}
							else
							{
								input_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_YEAR;
								output_schema_vector[colindex_yearEnd].field_type = FIELD_TYPE_YEAR;
							}

						}

					}

				}
				break;

			case TIME_GRANULARITY__NONE:
				{
					// no-op: no time range columns appear in the variable group instance table when the time granularity is none
				}
				break;

			default:
				{
					boost::format msg("Invalid time range specification in construction of import definition for variable group.");
					errorMsg = msg.str();
					return false;
				}
				break;

		}

		colindex = 0;
		std::for_each(colnames.cbegin(), colnames.cend(), [&](std::string const & colname)
		{
			mappings.push_back(std::make_shared<OneToOneFieldMapping>(std::make_pair(NameOrIndex(NameOrIndex::NAME, colname), fieldtypes[colindex]),
							   std::make_pair(NameOrIndex(NameOrIndex::NAME, colname), fieldtypes[colindex])));
			++colindex;
		});

	}

	schema_input.schema = input_schema_vector;
	schema_output.schema = output_schema_vector;

	definition.input_schema = schema_input;
	definition.output_schema = schema_output;

	definition.mappings = mappings;

	return true;

}

void Table_VariableGroupMetadata_PrimaryKeys::Load(sqlite3 * db, InputModel * input_model_)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	identifiers_map.clear();

	sqlite3_stmt * stmt = NULL;
	std::string sql("SELECT * FROM VG_DATA_METADATA__PRIMARY_KEYS");
	sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()) + 1, &stmt, NULL);

	if (stmt == NULL)
	{
		return;
	}

	int step_result = 0;

	while ((step_result = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		char const * vg_data_table_name = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__VG_DATA_TABLE_NAME));
		char const * vg_data_primary_key_column_name = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__VG_DATA_TABLE_PRIMARY_KEY_COLUMN_NAME));
		char const * vg_data_dmu_category_code = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__VG_DATA_TABLE_FK_DMU_CATEGORY_CODE));
		int vg_data_primary_key_sequence_number = sqlite3_column_int(stmt, INDEX__VG_DATA_TABLE_PRIMARY_KEY_SEQUENCE_NUMBER);
		char const * vg_data_primary_key_field_type = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__VG_DATA_TABLE_PRIMARY_KEY_FIELD_TYPE));

		if (vg_data_dmu_category_code && strlen(vg_data_dmu_category_code))
		{
			// maps:
			// vg_data_table_name =>
			// a vector of primary keys (each a DMU category identifier)
			// In the WidgetInstanceIdentifier, the CODE is set to the DMU category code,
			// the LONGHAND is set to the column name corresponding to this DMU in the variable group data table,
			// and the SEQUENCE NUMBER is set to the sequence number of the primary key in this variable group.
			std::string flags;

			std::string the_primary_key_field_type(vg_data_primary_key_field_type);


			// ************************************************************************************************************ //
			// float not allowed as primary key field type!
			// ************************************************************************************************************ //
			if (boost::iequals(the_primary_key_field_type, "float") || boost::iequals(the_primary_key_field_type, "double"))
			{
				boost::format msg("Floating-point column type is not allowed as a primary key column!");
				throw NewGeneException() << newgene_error_description(msg.str());
			}



			if (boost::iequals(the_primary_key_field_type, "int") || boost::iequals(the_primary_key_field_type, "int64"))
			{
				// "n" for int
				flags = "n";
			}

			if (boost::iequals(the_primary_key_field_type, "string"))
			{
				// "s" for string
				flags = "s";
			}

			identifiers_map[vg_data_table_name].push_back(WidgetInstanceIdentifier(std::string(vg_data_dmu_category_code), vg_data_primary_key_column_name, vg_data_primary_key_sequence_number,
					flags.c_str()));
		}
	}

	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

}

bool Table_VariableGroupMetadata_PrimaryKeys::AddDataTable(sqlite3 * db, InputModel * input_model_, WidgetInstanceIdentifier const & vg,
		std::vector<std::tuple<WidgetInstanceIdentifier, std::string, FIELD_TYPE>> const & primary_keys_info, std::string & errorMsg)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	//Executor theExecutor(db);

	if (!db)
	{
		return false;
	}

	if (!input_model_)
	{
		return false;
	}

	if (!vg.uuid || vg.uuid->empty() || !vg.code || vg.code->empty())
	{
		return false;
	}

	std::string new_table_name = Table_VariableGroupData::TableNameFromVGCode(*vg.code);

	// For now - do a quick-and-dirty test for the presence of the fields
	// Later, we can make this granular
	if (!identifiers_map[new_table_name].empty())
	{
		return true;
	}

	errorMsg.clear();
	int sequence = 1;
	std::for_each(primary_keys_info.cbegin(), primary_keys_info.cend(), [&](std::tuple<WidgetInstanceIdentifier, std::string, FIELD_TYPE> const & primary_key_info)
	{

		if (!errorMsg.empty())
		{
			return;
		}

		WidgetInstanceIdentifier const & dmu = std::get<0>(primary_key_info);
		std::string const & column_name = std::get<1>(primary_key_info);
		FIELD_TYPE const & field_type = std::get<2>(primary_key_info);

		if (!dmu.uuid || dmu.uuid->empty() || !dmu.code || dmu.code->empty() || column_name.empty())
		{
			errorMsg = "Bad DMU or column name when attempting to create primary key entry in metadata table for a variable group.";
			return;
		}

		boost::format
		add_stmt("INSERT INTO VG_DATA_METADATA__PRIMARY_KEYS (VG_DATA_TABLE_NAME, VG_DATA_TABLE_PRIMARY_KEY_COLUMN_NAME, VG_DATA_TABLE_FK_DMU_CATEGORY_CODE, VG_DATA_TABLE_PRIMARY_KEY_SEQUENCE_NUMBER, VG_DATA_TABLE_PRIMARY_KEY_FIELD_TYPE) VALUES ('%1%', '%2%', '%3%', %4%, '%5%')");
		add_stmt % new_table_name;
		add_stmt % column_name.c_str();
		add_stmt % dmu.code->c_str();
		add_stmt % boost::lexical_cast<std::string>(sequence).c_str();

		std::string flags;

		if (IsFieldTypeInt32(field_type))
		{
			add_stmt % "INT";
			flags = "n";
		}
		else if (IsFieldTypeInt64(field_type))
		{
			add_stmt % "INT";
			flags = "n";
		}
		else if (IsFieldTypeFloat(field_type))
		{
			boost::format msg("Floating-point value not allowed as primary key field.");
			throw NewGeneException() << newgene_error_description(msg.str());
		}
		else if (IsFieldTypeString(field_type))
		{
			add_stmt % "STRING";
			flags = "s";
		}
		else
		{
			add_stmt % "";
		}

		char * errmsg = nullptr;
		sqlite3_exec(db, add_stmt.str().c_str(), NULL, NULL, &errmsg);

		if (errmsg != nullptr)
		{
			boost::format msg("Unable to add primary key data for variable group into metadata table: %1% (%2%)");
			msg % errmsg % add_stmt;
			sqlite3_free(errmsg);
			errorMsg = msg.str();
			return;
		}

		// maps:
		// vg_data_table_name =>
		// a vector of primary keys (each a DMU category identifier)
		// In the WidgetInstanceIdentifier, the CODE is set to the DMU category code,
		// the LONGHAND is set to the column name corresponding to this DMU in the variable group data table,
		// and the SEQUENCE NUMBER is set to the sequence number of the primary key in this variable group.
		identifiers_map[new_table_name].push_back(WidgetInstanceIdentifier(std::string(*dmu.code), column_name, sequence, flags.c_str()));

		++sequence;

	});

	//theExecutor.success();

	//return theExecutor.succeeded();
	if (!errorMsg.empty())
	{
		return false;
	}

	return true;

}

bool Table_VariableGroupMetadata_PrimaryKeys::DeleteDataTable(sqlite3 * db, InputModel * input_model_, std::string const & table_name)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	//Executor theExecutor(db);

	if (!db)
	{
		return false;
	}

	if (!input_model_)
	{
		return false;
	}

	boost::format delete_stmt("DELETE FROM VG_DATA_METADATA__PRIMARY_KEYS WHERE VG_DATA_TABLE_NAME = '%1%'");
	delete_stmt % table_name;
	char * errmsg = nullptr;
	sqlite3_exec(db, delete_stmt.str().c_str(), NULL, NULL, &errmsg);

	if (errmsg != nullptr)
	{
		boost::format msg("Unable to delete data for variable group in primary keys table: %1%");
		msg % errmsg;
		sqlite3_free(errmsg);
		throw NewGeneException() << newgene_error_description(msg.str());
	}

	// delete from cache
	identifiers_map.erase(table_name);

	//theExecutor.success();

	//return theExecutor.succeeded();
	return true;

}

std::vector<std::pair<WidgetInstanceIdentifier, std::vector<std::string>>> Table_VariableGroupMetadata_PrimaryKeys::GetColumnNamesCorrespondingToPrimaryKeys(sqlite3 * db,
		InputModel * input_model_, std::string const & table_name)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	WidgetInstanceIdentifiers primary_keys = identifiers_map[table_name];

	// temp storage
	std::map<std::string, std::vector<std::string>> map_from_dmu_category_code_to_vector_of_primary_key_column_names;
	std::for_each(primary_keys.cbegin(), primary_keys.cend(), [&](WidgetInstanceIdentifier const & primary_key)
	{
		// From above:
		// In the WidgetInstanceIdentifier, the CODE is set to the DMU category code,
		// the LONGHAND is set to the column name corresponding to this DMU in the variable group data table,
		// and the SEQUENCE NUMBER is set to the sequence number of the primary key in this variable group.
		map_from_dmu_category_code_to_vector_of_primary_key_column_names[*primary_key.code].push_back(*primary_key.longhand);
	});

	// create output in desired format
	std::vector<std::pair<WidgetInstanceIdentifier, std::vector<std::string>>> output;
	std::for_each(map_from_dmu_category_code_to_vector_of_primary_key_column_names.cbegin(),
				  map_from_dmu_category_code_to_vector_of_primary_key_column_names.cend(), [&](std::pair<std::string const, std::vector<std::string>> const & single_dmu_category_primary_key_columns)
	{
		std::string const & dmu_category_string_code = single_dmu_category_primary_key_columns.first;
		std::vector<std::string> const & primary_key_column_names = single_dmu_category_primary_key_columns.second;
		WidgetInstanceIdentifier dmu_category;
		bool found = input_model_->t_dmu_category.getIdentifierFromStringCode(dmu_category_string_code, dmu_category);

		if (!found)
		{
			return;
		}

		output.push_back(std::make_pair(dmu_category, primary_key_column_names));
	});

	return output;

}

std::string Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeStartColumnName = "DATETIME_ROW_START";
std::string Table_VariableGroupMetadata_DateTimeColumns::DefaultDatetimeEndColumnName = "DATETIME_ROW_END";

void Table_VariableGroupMetadata_DateTimeColumns::Load(sqlite3 * db, InputModel * input_model_)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	identifiers_map.clear();

	sqlite3_stmt * stmt = NULL;
	std::string sql("SELECT * FROM VG_DATA_METADATA__DATETIME_COLUMNS");
	sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()) + 1, &stmt, NULL);

	if (stmt == NULL)
	{
		return;
	}

	int step_result = 0;

	while ((step_result = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		char const * vg_data_table_name = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__VG_DATA_TABLE_NAME));
		char const * vg_data_datetime_start_column_name = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__VG_DATA_TABLE_DATETIME_START_COLUMN_NAME));
		char const * vg_data_datetime_end_column_name = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__VG_DATA_TABLE_DATETIME_END_COLUMN_NAME));

		// Only add to cache if we have a time granularity.
		// Note: The entry must be in the table even with no time granularity
		// in order to allow a FK relationship with the "primary key" table
		// to support automatic cascading deletes, which are tied to this table through this FK relationship.
		if (!boost::trim_copy(std::string(vg_data_datetime_start_column_name)).empty() && !boost::trim_copy(std::string(vg_data_datetime_end_column_name)).empty())
		{
			// maps:
			// vg_data_table_name =>
			// a pair of datetime keys stuck into a vector (the first for the start datetime; the second for the end datetime)
			// In the WidgetInstanceIdentifier, the CODE is set to the column name
			identifiers_map[vg_data_table_name].push_back(WidgetInstanceIdentifier(std::string(vg_data_datetime_start_column_name)));
			identifiers_map[vg_data_table_name].push_back(WidgetInstanceIdentifier(std::string(vg_data_datetime_end_column_name)));
		}
	}

	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

}

bool Table_VariableGroupMetadata_DateTimeColumns::AddDataTable(sqlite3 * db, InputModel * input_model_, WidgetInstanceIdentifier const & vg, std::string & errorMsg,
		TIME_GRANULARITY const time_granularity)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	//Executor theExecutor(db);

	if (!db)
	{
		return false;
	}

	if (!input_model_)
	{
		return false;
	}

	if (!vg.uuid || vg.uuid->empty() || !vg.code || vg.code->empty())
	{
		return false;
	}

	std::string new_table_name = Table_VariableGroupData::TableNameFromVGCode(*vg.code);

	// For now - do a quick-and-dirty test for the presence of the fields
	// Later, we can make this granular
	if (!identifiers_map[new_table_name].empty())
	{
		return true;
	}

	boost::format
	add_stmt("INSERT INTO VG_DATA_METADATA__DATETIME_COLUMNS (VG_DATA_TABLE_NAME, VG_DATETIME_START_COLUMN_NAME, VG_DATETIME_END_COLUMN_NAME, VG_DATA_FK_VG_CATEGORY_UUID) VALUES ('%1%', '%2%', '%3%', '%4%')");
	add_stmt % new_table_name;

	if (time_granularity != TIME_GRANULARITY__NONE)
	{
		add_stmt % DefaultDatetimeStartColumnName % DefaultDatetimeEndColumnName;
	}
	else
	{
		add_stmt % std::string() % std::string();
	}

	add_stmt % *vg.uuid;
	char * errmsg = nullptr;
	sqlite3_exec(db, add_stmt.str().c_str(), NULL, NULL, &errmsg);

	if (errmsg != nullptr)
	{
		boost::format msg("Unable to add data for variable group into datetime table: %1%");
		msg % errmsg;
		sqlite3_free(errmsg);
		errorMsg = msg.str();
		return false;
	}

	if (time_granularity != TIME_GRANULARITY__NONE)
	{
		// add to cache
		// maps:
		// vg_data_table_name =>
		// a pair of datetime keys stuck into a vector (the first for the start datetime; the second for the end datetime)
		// In the WidgetInstanceIdentifier, the CODE is set to the column name
		identifiers_map[new_table_name].push_back(WidgetInstanceIdentifier(DefaultDatetimeStartColumnName));
		identifiers_map[new_table_name].push_back(WidgetInstanceIdentifier(DefaultDatetimeEndColumnName));
	}

	//theExecutor.success();

	//return theExecutor.succeeded();
	return true;

}

bool Table_VariableGroupMetadata_DateTimeColumns::DeleteDataTable(sqlite3 * db, InputModel * input_model_, std::string const & table_name)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	//Executor theExecutor(db);

	if (!db)
	{
		return false;
	}

	if (!input_model_)
	{
		return false;
	}

	boost::format delete_stmt("DELETE FROM VG_DATA_METADATA__DATETIME_COLUMNS WHERE VG_DATA_TABLE_NAME = '%1%'");
	delete_stmt % table_name;
	char * errmsg = nullptr;
	sqlite3_exec(db, delete_stmt.str().c_str(), NULL, NULL, &errmsg);

	if (errmsg != nullptr)
	{
		boost::format msg("Unable to delete data for variable group in datetime table: %1%");
		msg % errmsg;
		sqlite3_free(errmsg);
		throw NewGeneException() << newgene_error_description(msg.str());
	}

	// delete from cache
	identifiers_map.erase(table_name);

	//theExecutor.success();

	//return theExecutor.succeeded();
	return true;

}
