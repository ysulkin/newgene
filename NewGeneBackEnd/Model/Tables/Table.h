#ifndef TABLE_H
#define TABLE_H

#include <tuple>
#include <vector>

#include "TableTypes.h"

template<FIELD_TYPE THE_FIELD_TYPE>
class FieldValue
{
public:
	FieldValue()
	{

	}
	FieldValue(typename FieldTypeTraits<THE_FIELD_TYPE>::type & value_)
		: value(value_)
	{

	}
	typename FieldTypeTraits<THE_FIELD_TYPE>::type value;
};

template <FIELD_TYPE THE_FIELD_TYPE>
struct FieldData
{
	typedef std::tuple<FIELD_TYPE, std::string, FieldValue<THE_FIELD_TYPE>> type;
};

class BaseField
{
public:
	virtual FIELD_TYPE GetType() = 0;
	virtual std::string GetName() = 0;
};

template <FIELD_TYPE THE_FIELD_TYPE>
class Field : public BaseField
{

public:

	Field(std::string const field_name, FieldValue<THE_FIELD_TYPE> const & field_value)
		: BaseField()
		, data(std::make_tuple(THE_FIELD_TYPE, field_name, field_value))

	Field(FieldData<field_type>::type const & data_)
		: BaseField()
		, data_(data_)
	{

	}

	FIELD_TYPE GetType()
	{
		return THE_FIELD_TYPE;
	}

	std::string GetName()
	{
		return data.get<1>(data);
	}

	typename FieldTypeTraits<THE_FIELD_TYPE>::type GetValue()
	{
		return data.get<2>(data);
	}

	typename FieldData<THE_FIELD_TYPE>::type const data;
};

class TableMetadata
{
public:

};

template<TABLE_TYPES TABLE_TYPE>
class Table
{

	public:

		TableMetadata metadata;

		static std::vector<FIELD_TYPE> const types;
		static std::vector<std::string> const names;

		std::vector<BaseField> fields;

};

template<TABLE_TYPES TABLE_TYPE>
std::vector<FIELD_TYPE> Table<TABLE_TYPE>::types = TableTypeTraits<TABLE_TYPE>::types;
template<TABLE_TYPES TABLE_TYPE>
std::vector<std::string> Table<TABLE_TYPE>::names = TableTypeTraits<TABLE_TYPE>::names;

#endif
