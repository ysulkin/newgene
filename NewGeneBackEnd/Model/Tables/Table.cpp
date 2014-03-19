#include "table.h"

void Table_basemost::ImportBlockBulk(sqlite3 * db, ImportDefinition const & import_definition, OutputModel * output_model_, InputModel * input_model_, DataBlock const & block,
	int const number_rows_in_block, long & linenum, long & badwritelines, std::vector<std::string> & errors)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	//Executor executor(db);

	std::string sql_insert;

	sql_insert += "INSERT OR FAIL INTO \"";
	sql_insert += GetTableName();
	sql_insert += "\" (";

	bool first = true;
	std::for_each(import_definition.output_schema.schema.cbegin(),
				  import_definition.output_schema.schema.cend(), [&import_definition, &sql_insert, &first](SchemaEntry const & schema_entry)
	{

		if (!first)
		{
			sql_insert += ", ";
		}

		first = false;

		sql_insert += "`";
		sql_insert += schema_entry.field_name;
		sql_insert += "`";

	});

	sql_insert += ") VALUES ";

	bool failed = false;
	bool first_row = true;

	for (int row = 0; row < number_rows_in_block; ++row)
	{

		if (!first_row)
		{
			sql_insert += ", ";
		}

		first_row = false;

		sql_insert += "(";

		DataFields const & row_fields = block[row];
		first = true;
		std::for_each(row_fields.cbegin(), row_fields.cend(), [&](std::shared_ptr<BaseField> const & field_data)
		{

			if (failed)
			{
				return; // from lambda
			}

			if (!field_data)
			{
				boost::format msg("Bad row/col in block cache");
				errors.push_back(msg.str());
				failed = true;
				return; // from lambda
			}

			if (!first)
			{
				sql_insert += ", ";
			}

			first = false;

			FieldDataAsSqlText(field_data, sql_insert);

		});

		//if (failed)
		//{
		//	break;
		//}

		sql_insert += ")";

		++linenum;

	}

	//if (failed)
	//{
	//	return false;
	//}

	sqlite3_stmt * stmt = NULL;
	sqlite3_prepare_v2(db, sql_insert.c_str(), static_cast<int>(sql_insert.size()) + 1, &stmt, NULL);

	if (stmt == NULL)
	{
		boost::format msg("Unable to prepare SQL query: %1%");
		msg % sql_insert.c_str();
		errors.push_back(msg.str());
		return;
	}

	int step_result = 0;

	if ((step_result = sqlite3_step(stmt)) != SQLITE_DONE)
	{
		boost::format msg("Unable to execute SQL query: %1%");
		msg % sql_insert.c_str();
		errors.push_back(msg.str());
		if (stmt)
		{
			sqlite3_finalize(stmt);
			stmt = nullptr;
		}

		return;
	}

	//executor.success();

	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

}

void Table_basemost::ImportBlockUpdate(sqlite3 * db, ImportDefinition const & import_definition, OutputModel * output_model_, InputModel * input_model_, DataBlock const & block,
	int const number_rows_in_block, long & linenum, long & badwritelines, std::vector<std::string> & errors)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	//Executor executor(db);

	std::string errorMsg;
	for (int row = 0; row < number_rows_in_block; ++row)
	{

		errorMsg.clear();
		bool failed = false;
		int changes = TryUpdateRow(block, row, failed, import_definition, db, errorMsg);

		if (failed || !errorMsg.empty())
		{
			boost::format msg("Unable to update line %1 during import: %2%");
			msg % boost::lexical_cast<std::string>(linenum + 1) % errorMsg;
			errorMsg = msg.str();
			errors.push_back(errorMsg);
			errorMsg.clear();
			++linenum;
			++badwritelines;
			continue; // try some other rows
		}

		if (changes == 0)
		{
			// Update did not affect any row.  Insert a new row
			TryInsertRow(block, row, failed, import_definition, db, errorMsg);
		}

		if (failed || !errorMsg.empty())
		{
			boost::format msg("Unable to insert line %1% during import: %2%");
			msg % boost::lexical_cast<std::string>(linenum + 1) % errorMsg;
			errorMsg = msg.str();
			errors.push_back(errorMsg);
			errorMsg.clear();
			++linenum;
			++badwritelines;
			continue; // try some other rows
		}

		++linenum;

	}

	//executor.success();

}

void Table_basemost::FieldDataAsSqlText(std::shared_ptr<BaseField> const & field_data, std::string & sql_insert)
{

	FIELD_TYPE const field_type = field_data->GetType();

	if (IsFieldTypeInt32(field_type))
	{
		sql_insert += boost::lexical_cast<std::string>(field_data->GetInt32Ref());
	}
	else
	if (IsFieldTypeInt64(field_type))
	{
		sql_insert += boost::lexical_cast<std::string>(field_data->GetInt64Ref());
	}
	else
	if (IsFieldTypeFloat(field_type))
	{
		sql_insert += boost::lexical_cast<std::string>(field_data->GetDouble());
	}
	else
	if (IsFieldTypeString(field_type))
	{
		sql_insert += '\'';
		sql_insert += Table_basemost::EscapeTicks(field_data->GetString());
		sql_insert += '\'';
	}
	else
	{
		boost::format msg("Invalid data type when obtaining raw data for field!");
		throw NewGeneException() << newgene_error_description(msg.str());
	}

}

int Table_basemost::TryUpdateRow(DataBlock const & block, int row, bool & failed, ImportDefinition const & import_definition, sqlite3 * db, std::string & errorMsg)
{
	std::string sql_update;

	DataFields const & row_fields = block[row];

	sql_update += "UPDATE \"";
	sql_update += GetTableName();
	sql_update += "\" SET ";

	bool first = true;
	int index = 0;
	bool innerFailed = false;
	std::for_each(import_definition.output_schema.schema.cbegin(),
				  import_definition.output_schema.schema.cend(), [&](SchemaEntry const & schema_entry)
	{

		if (innerFailed)
		{
			++index;
			return;
		}

		// Disable skipping primary key and time range fields.
		// Instead, overwrite them even if it's known they will have the same values.
		// This way, if there ARE no other fields, we will nonetheless have something in the 
		// SET clause, which is required for valid SQL
		if (false && schema_entry.IsPrimaryKey() || schema_entry.IsTimeRange())
		{
			++index;
			return; // These are automatically covered by the "WHERE" clause
		}

		std::shared_ptr<BaseField> const & field_data = row_fields[index];

		if (!field_data)
		{
			// Todo: log error
			boost::format msg("Unable to obtain BaseField in TryUpdateRow.");
			errorMsg = msg.str();
			++index;
			innerFailed = true;
			return;
		}

		if (!first)
		{
			sql_update += ", ";
		}

		first = false;

		sql_update += "`";
		sql_update += schema_entry.field_name;
		sql_update += "`";

		sql_update += " = ";

		FieldDataAsSqlText(field_data, sql_update);

		++index;

	});

	if (innerFailed)
	{
		failed = true;
		return 0;
	}

	sql_update += " WHERE ";

	first = true;
	index = 0;
	innerFailed = false;
	std::for_each(import_definition.output_schema.schema.cbegin(),
				  import_definition.output_schema.schema.cend(), [&](SchemaEntry const & schema_entry)
	{

		if (innerFailed)
		{
			++index;
			return;
		}

		if (schema_entry.IsPrimaryKey() || schema_entry.IsTimeRange())
		{

			std::shared_ptr<BaseField> const & field_data = row_fields[index];

			if (!field_data)
			{
				// Todo: log error
				++index;
				innerFailed = true;
				return;
			}

			if (!first)
			{
				sql_update += " AND ";
			}

			first = false;

			sql_update += "`";
			sql_update += schema_entry.field_name;
			sql_update += "`";

			sql_update += " = ";

			FieldDataAsSqlText(field_data, sql_update);

		}

		++index;

	});

	if (innerFailed)
	{
		failed = true;
		return 0;
	}

	sqlite3_stmt * stmt = nullptr;
	sqlite3_prepare_v2(db, sql_update.c_str(), static_cast<int>(sql_update.size()) + 1, &stmt, NULL);

	if (stmt == NULL)
	{
		// TODO: Log error
		boost::format msg("Unable to prepare SQL query in TryUpdateRow: %1%");
		msg % sql_update;
		errorMsg = msg.str();
		failed = true;
		return 0;
	}

	int step_result = 0;

	if ((step_result = sqlite3_step(stmt)) != SQLITE_DONE)
	{
		// TODO: Log error
		if (stmt)
		{
			sqlite3_finalize(stmt);
			stmt = nullptr;
		}

		boost::format msg("Unable to execute SQL query in TryUpdateRow: %1%");
		msg % sql_update;
		errorMsg = msg.str();
		failed = true;
		return 0;
	}

	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

	return sqlite3_changes(db);

}

void Table_basemost::TryInsertRow(DataBlock const & block, int row, bool & failed, ImportDefinition const & import_definition, sqlite3 * db, std::string & errorMsg)
{

	std::string sql_insert;

	DataFields const & row_fields = block[row];

	sql_insert += "INSERT INTO \"";
	sql_insert += GetTableName();
	sql_insert += "\" ( ";

	bool first = true;
	int index = 0;
	std::for_each(import_definition.output_schema.schema.cbegin(),
				  import_definition.output_schema.schema.cend(), [&](SchemaEntry const & schema_entry)
	{

		if (!first)
		{
			sql_insert += ", ";
		}

		first = false;

		sql_insert += "`";
		sql_insert += schema_entry.field_name;
		sql_insert += "`";

		++index;

	});

	sql_insert += ") ";

	sql_insert += " VALUES ";

	sql_insert += "(";

	first = true;
	index = 0;
	bool innerFailed = false;
	std::for_each(import_definition.output_schema.schema.cbegin(),
				  import_definition.output_schema.schema.cend(), [&](SchemaEntry const & schema_entry)
	{

		if (innerFailed)
		{
			++index;
			return;
		}

		std::shared_ptr<BaseField> const & field_data = row_fields[index];

		if (!field_data)
		{
			// Todo: log error
			boost::format msg("Unable to retrieve base field in TryInsertRow");
			errorMsg = msg.str();
			++index;
			innerFailed = true;
			return;
		}

		if (!first)
		{
			sql_insert += ", ";
		}

		first = false;

		FieldDataAsSqlText(field_data, sql_insert);

		++index;

	});

	sql_insert += ") ";

	if (innerFailed)
	{
		failed = true;
		return;
	}

	sqlite3_stmt * stmt = nullptr;
	sqlite3_prepare_v2(db, sql_insert.c_str(), static_cast<int>(sql_insert.size()) + 1, &stmt, NULL);

	if (stmt == NULL)
	{
		// TODO: Log error
		boost::format msg("Unable to prepare SQL query in TryInsertRow: %1%");
		msg % sql_insert;
		errorMsg = msg.str();
		failed = true;
		return;
	}

	int step_result = 0;

	if ((step_result = sqlite3_step(stmt)) != SQLITE_DONE)
	{
		// TODO: Log error
		if (stmt)
		{
			sqlite3_finalize(stmt);
			stmt = nullptr;
		}

		boost::format msg("Unable to execute SQL query in TryInsertRow: %1%");
		msg % sql_insert;
		errorMsg = msg.str();
		failed = true;
		return;
	}

	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

}
