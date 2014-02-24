#include "DMU.h"
#include "../../../sqlite/sqlite-amalgamation-3071700/sqlite3.h"

#ifndef Q_MOC_RUN
#	include <boost/algorithm/string.hpp>
#endif
#include "../../InputModel.h"
#include "../../../Utilities/UUID.h"

std::string const Table_DMU_Identifier::DMU_CATEGORY_UUID = "DMU_CATEGORY_UUID";
std::string const Table_DMU_Identifier::DMU_CATEGORY_STRING_CODE = "DMU_CATEGORY_STRING_CODE";
std::string const Table_DMU_Identifier::DMU_CATEGORY_STRING_LONGHAND = "DMU_CATEGORY_STRING_LONGHAND";
std::string const Table_DMU_Identifier::DMU_CATEGORY_NOTES1 = "DMU_CATEGORY_NOTES1";
std::string const Table_DMU_Identifier::DMU_CATEGORY_NOTES2 = "DMU_CATEGORY_NOTES2";
std::string const Table_DMU_Identifier::DMU_CATEGORY_NOTES3 = "DMU_CATEGORY_NOTES3";
std::string const Table_DMU_Identifier::DMU_CATEGORY_FLAGS = "DMU_CATEGORY_FLAGS";

std::string const Table_DMU_Instance::DMU_SET_MEMBER_UUID = "DMU_SET_MEMBER_UUID";
std::string const Table_DMU_Instance::DMU_SET_MEMBER_STRING_CODE = "DMU_SET_MEMBER_STRING_CODE";
std::string const Table_DMU_Instance::DMU_SET_MEMBER_STRING_LONGHAND = "DMU_SET_MEMBER_STRING_LONGHAND";
std::string const Table_DMU_Instance::DMU_SET_MEMBER_NOTES1 = "DMU_SET_MEMBER_NOTES1";
std::string const Table_DMU_Instance::DMU_SET_MEMBER_NOTES2 = "DMU_SET_MEMBER_NOTES2";
std::string const Table_DMU_Instance::DMU_SET_MEMBER_NOTES3 = "DMU_SET_MEMBER_NOTES3";
std::string const Table_DMU_Instance::DMU_SET_MEMBER_FK_DMU_CATEGORY_UUID = "DMU_SET_MEMBER_FK_DMU_CATEGORY_UUID";
std::string const Table_DMU_Instance::DMU_SET_MEMBER_FLAGS = "DMU_SET_MEMBER_FLAGS";

void Table_DMU_Identifier::Load(sqlite3 * db, InputModel * input_model_)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	identifiers.clear();

	sqlite3_stmt * stmt = NULL;
	std::string sql("SELECT * FROM DMU_CATEGORY");
	sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()) + 1, &stmt, NULL);
	if (stmt == NULL)
	{
		return;
	}
	int step_result = 0;
	while ((step_result = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		char const * uuid = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_CATEGORY_UUID));
		char const * code = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_CATEGORY_STRING_CODE));
		char const * longhand = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_CATEGORY_STRING_LONGHAND));
		char const * notes1 = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_CATEGORY_NOTES1));
		char const * notes2 = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_CATEGORY_NOTES2));
		char const * notes3 = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_CATEGORY_NOTES3));
		char const * flags = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_CATEGORY_FLAGS));
		if (uuid && /* strlen(uuid) == UUID_LENGTH && */ code && strlen(code) && longhand && strlen(longhand))
		{
			WidgetInstanceIdentifier DMU_category_identifier(uuid, code, longhand, 0, flags, TIME_GRANULARITY__NONE, MakeNotes(notes1, notes2, notes3));
			identifiers.push_back(DMU_category_identifier);
		}
	}
	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

}

bool Table_DMU_Identifier::Exists(sqlite3 * db, InputModel & input_model_, std::string const & dmu, bool const also_confirm_using_cache)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	std::string dmu_to_check(boost::to_upper_copy(dmu));

	sqlite3_stmt * stmt = NULL;
	std::string sql("SELECT COUNT(*) FROM DMU_CATEGORY WHERE DMU_CATEGORY_STRING_CODE = '");
	sql += dmu_to_check;
	sql += "'";
	sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()) + 1, &stmt, NULL);
	if (stmt == NULL)
	{
		boost::format msg("Unable to prepare SELECT statement to search for an existing DMU category.");
		throw NewGeneException() << newgene_error_description(msg.str());
	}
	int step_result = 0;
	bool exists = false;
	while ((step_result = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		int existing_dmu_count = sqlite3_column_int(stmt, 0);
		if (existing_dmu_count == 1)
		{
			exists = true;
		}
	}
	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

	if (also_confirm_using_cache)
	{

		// Safety check: Cache should match database
		if (getIdentifierFromStringCode(dmu_to_check, WidgetInstanceIdentifier()) != exists)
		{
			boost::format msg("Cache of DMU categories is out-of-sync.");
			throw NewGeneException() << newgene_error_description(msg.str());
		}

		//auto found = std::find_if(identifiers.cbegin(), identifiers.cend(), std::bind(&WidgetInstanceIdentifier::IsEqual, std::placeholders::_1, WidgetInstanceIdentifier::EQUALITY_CHECK_TYPE__STRING_CODE, WidgetInstanceIdentifier(dmu_to_check)));
		//bool exists_in_cache = (found != identifiers.cend());
		//if (exists != exists_in_cache)
		//{
		//	boost::format msg("Cache of DMU categories is out-of-sync.");
		//	throw NewGeneException() << newgene_error_description(msg.str());
		//}

	}

	return exists;

}

bool Table_DMU_Identifier::CreateNewDMU(sqlite3 * db, InputModel & input_model_, std::string const & dmu, std::string const & dmu_description)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	Executor theExecutor(db);

	bool already_exists = Exists(db, input_model_, dmu);
	if (already_exists)
	{
		return false;
	}

	std::string new_uuid(boost::to_upper_copy(newUUID(false)));
	sqlite3_stmt * stmt = NULL;
	std::string sql("INSERT INTO DMU_CATEGORY (DMU_CATEGORY_UUID, DMU_CATEGORY_STRING_CODE, DMU_CATEGORY_STRING_LONGHAND) VALUES ('");
	sql += new_uuid;
	sql += "', ?, ?)";
	sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()) + 1, &stmt, NULL);
	if (stmt == NULL)
	{
		boost::format msg("Unable to prepare INSERT statement to create a new DMU category.");
		throw NewGeneException() << newgene_error_description(msg.str());
	}
	std::string new_dmu(boost::to_upper_copy(dmu));
	sqlite3_bind_text(stmt, 1, new_dmu.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, dmu_description.c_str(), -1, SQLITE_TRANSIENT);
	int step_result = 0;
	step_result = sqlite3_step(stmt);
	if (step_result != SQLITE_DONE)
	{
		boost::format msg("Unable to execute INSERT statement to create a new DMU category: %1%");
		msg % sqlite3_errstr(step_result);
		throw NewGeneException() << newgene_error_description(msg.str());
	}
	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

	std::string flags;
	WidgetInstanceIdentifier DMU_category_identifier(new_uuid, new_dmu, dmu_description, 0, flags.c_str(), TIME_GRANULARITY__NONE, MakeNotes(std::string(), std::string(), std::string()));
	identifiers.push_back(DMU_category_identifier);
	Sort();

	theExecutor.success();

	return theExecutor.succeeded();

}

bool Table_DMU_Identifier::DeleteDMU(sqlite3 * db, InputModel & input_model_, WidgetInstanceIdentifier & dmu, DataChangeMessage & change_message)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	Executor theExecutor(db);

	if (!dmu.code || !dmu.uuid)
	{
		return false;
	}

	bool already_exists = Exists(db, input_model_, *dmu.code);
	if (!already_exists)
	{
		return false;
	}

	// Since the schema doesn't allow FK pointing from UOA to UOA_LOOKUP,
	// we must reverse-map to the UOA's here and then remove those
	WidgetInstanceIdentifiers uoas = input_model_.t_uoa_setmemberlookup.RetrieveUOAsGivenDMU(db, &input_model_, dmu);
	std::for_each(uoas.cbegin(), uoas.cend(), [&](WidgetInstanceIdentifier const & uoa)
	{
		input_model_.t_uoa_category.DeleteUOA(db, input_model_, uoa, change_message);
	});

	sqlite3_stmt * stmt = NULL;
	std::string sql("DELETE FROM DMU_CATEGORY WHERE DMU_CATEGORY_UUID = ?");
	int err = sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()) + 1, &stmt, NULL);
	if (stmt == NULL)
	{
		boost::format msg("Unable to prepare DELETE statement to delete a DMU category: %1%");
		msg % sqlite3_errstr(err);
		throw NewGeneException() << newgene_error_description(msg.str());
	}
	sqlite3_bind_text(stmt, 1, dmu.uuid->c_str(), -1, SQLITE_TRANSIENT);
	int step_result = 0;
	step_result = sqlite3_step(stmt);
	if (step_result != SQLITE_DONE)
	{
		boost::format msg("Unable to execute DELETE statement to delete a DMU category: %1%");
		msg % sqlite3_errstr(step_result);
		throw NewGeneException() << newgene_error_description(msg.str());
	}
	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

	// Remove from cache
	std::string flags;
	identifiers.erase(std::remove_if(identifiers.begin(), identifiers.end(), std::bind(&WidgetInstanceIdentifier::IsEqual, std::placeholders::_1, WidgetInstanceIdentifier::EQUALITY_CHECK_TYPE__UUID_PLUS_STRING_CODE, dmu)), identifiers.end());

	// ***************************************** //
	// Prepare data to send back to user interface
	// ***************************************** //
	DATA_CHANGE_TYPE type = DATA_CHANGE_TYPE__INPUT_MODEL__DMU_CHANGE;
	DATA_CHANGE_INTENTION intention = DATA_CHANGE_INTENTION__REMOVE;
	DataChange change(type, intention, dmu, WidgetInstanceIdentifiers());
	change_message.changes.push_back(change);

	theExecutor.success();

	return theExecutor.succeeded();

}

std::string Table_DMU_Identifier::GetDmuCategoryDisplayText(WidgetInstanceIdentifier const & dmu_category)
{

	if (!dmu_category.code || dmu_category.code->empty())
	{
		return std::string();
	}

	std::string dmu_code(*dmu_category.code);

	std::string dmu_description;
	if (dmu_category.longhand)
	{
		dmu_description = *dmu_category.longhand;
	}

	std::string text(dmu_code);
	if (!dmu_description.empty())
	{
		text += " (";
		text += dmu_description.c_str();
		text += ")";
	}

	return text;

}

void Table_DMU_Instance::Load(sqlite3 * db, InputModel * input_model_)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	identifiers_map.clear();

	sqlite3_stmt * stmt = NULL;
	std::string sql("SELECT * FROM DMU_SET_MEMBER");
	sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()) + 1, &stmt, NULL);
	if (stmt == NULL)
	{
		return;
	}
	int step_result = 0;

	while ((step_result = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		char const * uuid = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_SET_MEMBER_UUID));
		char const * code = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_SET_MEMBER_STRING_CODE));
		char const * longhand = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_SET_MEMBER_STRING_LONGHAND));
		char const * notes1 = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_SET_MEMBER_NOTES1));
		char const * notes2 = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_SET_MEMBER_NOTES2));
		char const * notes3 = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_SET_MEMBER_NOTES3));
		char const * fk_DMU_uuid = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_SET_MEMBER_FK_DMU_CATEGORY_UUID));
		char const * flags = reinterpret_cast<char const *>(sqlite3_column_text(stmt, INDEX__DMU_SET_MEMBER_FLAGS));
		if (uuid && fk_DMU_uuid)
		{
			std::string code_string;
			if (code)
			{
				code_string = code;
			}
			std::string longhand_string;
			if (longhand)
			{
				longhand_string = longhand;
			}
			std::string flags_string;
			if (flags)
			{
				flags_string = flags;
			}
			std::string notes_string_1;
			if (notes1)
			{
				notes_string_1 = notes1;
			}
			std::string notes_string_2;
			if (notes2)
			{
				notes_string_2 = notes2;
			}
			std::string notes_string_3;
			if (notes3)
			{
				notes_string_3 = notes3;
			}
			identifiers_map[fk_DMU_uuid].push_back(WidgetInstanceIdentifier(uuid, input_model_->t_dmu_category.getIdentifier(fk_DMU_uuid), code_string.c_str(), longhand_string.c_str(), 0, flags_string.c_str(), TIME_GRANULARITY__NONE, MakeNotes(notes_string_1.c_str(), notes_string_2.c_str(), notes_string_3.c_str())));
		}
	}

	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

}

bool Table_DMU_Instance::Exists(sqlite3 * db, InputModel & input_model_, WidgetInstanceIdentifier const & dmu_category, std::string const & dmu_member_uuid, bool const also_confirm_using_cache)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	if (!dmu_category.uuid || dmu_category.uuid->empty())
	{
		boost::format msg("Unable to prepare SELECT statement to search for an existing DMU member.");
		throw NewGeneException() << newgene_error_description(msg.str());
	}

	std::string dmu_member_to_check(boost::to_upper_copy(dmu_member_uuid));

	sqlite3_stmt * stmt = NULL;
	std::string sql("SELECT COUNT(*) FROM DMU_SET_MEMBER WHERE UPPER(DMU_SET_MEMBER_UUID) = '");
	sql += dmu_member_to_check;
	sql += "' AND UPPER(DMU_SET_MEMBER_FK_DMU_CATEGORY_UUID) = '";
	sql += *dmu_category.uuid;
	sql += "'";
	sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()) + 1, &stmt, NULL);
	if (stmt == NULL)
	{
		boost::format msg("Unable to prepare SELECT statement to search for an existing DMU member.");
		throw NewGeneException() << newgene_error_description(msg.str());
	}
	int step_result = 0;
	bool exists = false;
	while ((step_result = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		int existing_dmu_count = sqlite3_column_int(stmt, 0);
		if (existing_dmu_count == 1)
		{
			exists = true;
		}
	}
	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

	if (also_confirm_using_cache)
	{
		// Safety check: Cache should match database
		if (getIdentifier(dmu_member_uuid, *dmu_category.uuid).IsEmpty() == exists)
		{
			boost::format msg("Cache of DMU members is out-of-sync.");
			throw NewGeneException() << newgene_error_description(msg.str());
		}
	}

	return exists;

}

WidgetInstanceIdentifier Table_DMU_Instance::CreateNewDmuMember(sqlite3 * db, InputModel & input_model_, WidgetInstanceIdentifier const & dmu_category, std::string const & dmu_member_uuid, std::string const & dmu_member_code, std::string const & dmu_member_description)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	Executor theExecutor(db);

	bool already_exists = Exists(db, input_model_, dmu_category, dmu_member_uuid);
	if (already_exists)
	{
		return WidgetInstanceIdentifier();
	}

	std::string new_uuid(dmu_member_uuid);
	sqlite3_stmt * stmt = NULL;
	std::string sql("INSERT INTO DMU_SET_MEMBER (DMU_SET_MEMBER_UUID, DMU_SET_MEMBER_STRING_CODE, DMU_SET_MEMBER_STRING_LONGHAND, DMU_SET_MEMBER_FK_DMU_CATEGORY_UUID) VALUES (");
	sql += "?, ?, ?, ')";
	sql += *dmu_category.uuid;
	sql += ")";
	sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()) + 1, &stmt, NULL);
	if (stmt == NULL)
	{
		boost::format msg("Unable to prepare INSERT statement to create a new DMU member.");
		throw NewGeneException() << newgene_error_description(msg.str());
	}
	sqlite3_bind_text(stmt, 1, dmu_member_uuid.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, dmu_member_code.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, dmu_member_description.c_str(), -1, SQLITE_TRANSIENT);
	int step_result = 0;
	step_result = sqlite3_step(stmt);
	if (step_result != SQLITE_DONE)
	{
		boost::format msg("Unable to execute INSERT statement to create a new DMU member: %1%");
		msg % sqlite3_errstr(step_result);
		throw NewGeneException() << newgene_error_description(msg.str());
	}
	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

	// Append to cache
	WidgetInstanceIdentifier dmu_member(dmu_member_uuid, dmu_category, dmu_member_code.c_str(), dmu_member_description.c_str(), 0);
	identifiers_map[*dmu_category.uuid].push_back(dmu_member);

	theExecutor.success();

	return dmu_member;

}

bool Table_DMU_Instance::DeleteDmuMember(sqlite3 * db, InputModel & input_model_, WidgetInstanceIdentifier & dmu_member)
{

	std::lock_guard<std::recursive_mutex> data_lock(data_mutex);

	Executor theExecutor(db);

	if (!dmu_member.uuid || dmu_member.uuid->empty() || !dmu_member.identifier_parent || !dmu_member.identifier_parent->code || !dmu_member.identifier_parent->uuid)
	{
		return false;
	}

	WidgetInstanceIdentifier dmu_category = *dmu_member.identifier_parent;

	bool already_exists = Exists(db, input_model_, dmu_category, *dmu_member.uuid);
	if (!already_exists)
	{
		return false;
	}

	// Remove corresponding rows from any instance data tables
	// ... *dmu_member.identifier_parent is the DMU category corresponding to this DMU member
	WidgetInstanceIdentifiers uoas = input_model_.t_uoa_setmemberlookup.RetrieveUOAsGivenDMU(db, &input_model_, *dmu_member.identifier_parent);
	std::for_each(uoas.cbegin(), uoas.cend(), [&](WidgetInstanceIdentifier const & uoa)
	{
		WidgetInstanceIdentifiers vgs_to_delete = input_model_.t_vgp_identifiers.RetrieveVGsFromUOA(db, &input_model_, *uoa.uuid);
		std::for_each(vgs_to_delete.cbegin(), vgs_to_delete.cend(), [&](WidgetInstanceIdentifier const & vg_to_delete)
		{
			std::for_each(input_model_.t_vgp_data_vector.begin(), input_model_.t_vgp_data_vector.end(), [&](std::unique_ptr<Table_VariableGroupData> & vg_instance_table)
			{
				if (vg_instance_table)
				{
					if (boost::iequals(*vg_to_delete.code, vg_instance_table->vg_category_string_code))
					{
						std::string table_name = vg_instance_table->table_name;
						std::vector<std::pair<WidgetInstanceIdentifier, std::vector<std::string>>> dmu_category_and_corresponding_column_names = input_model_.t_vgp_data_metadata__primary_keys.GetColumnNamesCorrespondingToPrimaryKeys(db, &input_model_, table_name);
						std::for_each(dmu_category_and_corresponding_column_names.cbegin(), dmu_category_and_corresponding_column_names.cend(), [&](std::pair<WidgetInstanceIdentifier, std::vector<std::string>> const & single_dmu_category_and_corresponding_column_names)
						{
							WidgetInstanceIdentifier const & dmu_category_primary_key_column = single_dmu_category_and_corresponding_column_names.first;
							if (dmu_category.IsEqual(WidgetInstanceIdentifier::EQUALITY_CHECK_TYPE__UUID_PLUS_STRING_CODE, dmu_category_primary_key_column))
							{
								std::vector<std::string> const & corresponding_column_names = single_dmu_category_and_corresponding_column_names.second;
								std::for_each(corresponding_column_names.cbegin(), corresponding_column_names.cend(), [&](std::string const & corresponding_column_name)
								{
									vg_instance_table->DeleteDmuMemberRows(db, &input_model_, dmu_member, corresponding_column_name);
								});
							}
						});
					}
				}
			});
		});
	});

	// Remove from DMU_SET_MEMBER table
	sqlite3_stmt * stmt = NULL;
	boost::format sql("DELETE FROM DMU_SET_MEMBER WHERE DMU_SET_MEMBER_UUID = '%1%' AND DMU_SET_MEMBER_FK_DMU_CATEGORY_UUID = '%2%'");
	sql % *dmu_member.uuid % *dmu_category.uuid;
	int err = sqlite3_prepare_v2(db, sql.str().c_str(), static_cast<int>(sql.str().size()) + 1, &stmt, NULL);
	if (stmt == NULL)
	{
		boost::format msg("Unable to prepare DELETE statement to delete DMU member: %1%");
		msg % sqlite3_errstr(err);
		throw NewGeneException() << newgene_error_description(msg.str());
	}
	int step_result = 0;
	step_result = sqlite3_step(stmt);
	if (step_result != SQLITE_DONE)
	{
		boost::format msg("Unable to prepare DELETE statement to delete DMU member: %1%");
		msg % sqlite3_errstr(err);
		throw NewGeneException() << newgene_error_description(msg.str());
	}
	if (stmt)
	{
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

	// Remove from cache
	identifiers_map[*dmu_category.uuid].erase(std::remove_if(identifiers_map[*dmu_category.uuid].begin(), identifiers_map[*dmu_category.uuid].end(), [&](WidgetInstanceIdentifier & test_dmu_member)
	{
		if (test_dmu_member.IsEqual(WidgetInstanceIdentifier::EQUALITY_CHECK_TYPE__UUID, dmu_member))
		{
			return true;
		}
		return false;
	}), identifiers_map[*dmu_category.uuid].end());

	theExecutor.success();

	return theExecutor.succeeded();

}

std::string Table_DMU_Instance::GetDmuMemberDisplayText(WidgetInstanceIdentifier const & dmu_member)
{

	std::string text;
	if (dmu_member.longhand && !dmu_member.longhand->empty())
	{
		text += dmu_member.longhand->c_str();
	}
	if (dmu_member.code && !dmu_member.code->empty())
	{
		bool use_parentheses = false;
		if (dmu_member.longhand && !dmu_member.longhand->empty())
		{
			use_parentheses = true;
		}
		if (use_parentheses)
		{
			text += " (";
		}
		text += dmu_member.code->c_str();
		if (use_parentheses)
		{
			text += ")";
		}
	}
	bool use_parentheses = false;
	if ((dmu_member.code && !dmu_member.code->empty()) || (dmu_member.longhand && !dmu_member.longhand->empty()))
	{
		use_parentheses = true;
	}
	if (use_parentheses)
	{
		text += " (";
	}
	text += dmu_member.uuid->c_str();
	if (use_parentheses)
	{
		text += ")";
	}

	return text;

}
