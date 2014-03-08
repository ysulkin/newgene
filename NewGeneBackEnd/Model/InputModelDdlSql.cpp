#include "InputModelDdlSql.h"

std::string InputModelDDLSQL()
{

	return std::string(R"~~~(




/*************************/
/*                       */
/* InputModelDefault.SQL */
/*                       */
/*************************/

/* Disable Foreign Keys */
pragma foreign_keys = off;
/* Begin Transaction */
begin transaction;

/* Database [DefaultInputProjectSettings.newgene.in] */
pragma auto_vacuum=0;
pragma encoding='UTF-8';
pragma page_size=1024;

/* Drop table [main].[CMU_CATEGORY] */
drop table if exists [main].[CMU_CATEGORY];

/* Table structure [main].[CMU_CATEGORY] */
CREATE TABLE [main].[CMU_CATEGORY] (
  [CMU_CATEGORY_UUID] CHAR(36) NOT NULL ON CONFLICT FAIL CONSTRAINT [CMU_CATEGORY_UUID_CONSTRAINT] UNIQUE ON CONFLICT FAIL, 
  [CMU_CATEGORY_STRING_CODE] varchar(128) NOT NULL ON CONFLICT FAIL CONSTRAINT [CMU_CATEGORY_STRING_CODE_CONSTRAINT] UNIQUE ON CONFLICT FAIL, 
  [CMU_CATEGORY_STRING_LONGHAND] varchar(4096), 
  [CMU_CATEGORY_NOTES1] varchar(16384), 
  [CMU_CATEGORY_NOTES2] varchar(16384), 
  [CMU_CATEGORY_NOTES3] varchar(16384), 
  [CMU_CATEGORY_FK_DMU_CATEGORY_UUID_1] CHAR(36) NOT NULL ON CONFLICT FAIL CONSTRAINT [CMU_CATEGORY_FK_DMU_CATEGORY_UUID_1__FK__DMU_CATEGORY_UUID] REFERENCES [DMU_CATEGORY]([DMU_CATEGORY_UUID]) ON DELETE CASCADE ON UPDATE CASCADE, 
  [CMU_CATEGORY_FK_DMU_CATEGORY_UUID_2] CHAR(36) NOT NULL ON CONFLICT FAIL CONSTRAINT [CMU_CATEGORY_FK_DMU_CATEGORY_UUID_2__FK__DMU_CATEGORY_UUID] REFERENCES [DMU_CATEGORY]([DMU_CATEGORY_UUID]) ON DELETE CASCADE ON UPDATE CASCADE, 
  [CMU_CATEGORY_FLAGS] CHAR(16), 
  CONSTRAINT [sqlite_autoindex_CMU_CATEGORY_3] PRIMARY KEY ([CMU_CATEGORY_STRING_CODE], [CMU_CATEGORY_FK_DMU_CATEGORY_UUID_1], [CMU_CATEGORY_FK_DMU_CATEGORY_UUID_2]) ON CONFLICT FAIL);

/* Drop table [main].[CMU_SET_MEMBER] */
drop table if exists [main].[CMU_SET_MEMBER];

/* Table structure [main].[CMU_SET_MEMBER] */
CREATE TABLE [main].[CMU_SET_MEMBER] (
  [CMU_SET_MEMBER_UUID] CHAR(36) NOT NULL ON CONFLICT FAIL CONSTRAINT [CMU_SET_MEMBER_UUID_CONSTRAINT] UNIQUE ON CONFLICT FAIL, 
  [CMU_SET_MEMBER_FK_CMU_CATEGORY_UUID] CHAR(36) NOT NULL ON CONFLICT FAIL CONSTRAINT [CMU_SET_MEMBER_FK_CMU_CATEGORY_UUID__FK__CMU_CATEGORY_UUID] REFERENCES [CMU_CATEGORY]([CMU_CATEGORY_UUID]) ON DELETE CASCADE ON UPDATE CASCADE, 
  [CMU_SET_MEMBER_FK_DMU_SET_MEMBER_UUID_1] CHAR(36) NOT NULL ON CONFLICT FAIL, 
  [CMU_SET_MEMBER_FK_DMU_SET_MEMBER_FK_DMU_CATEGORY_UUID_1] CHAR(36) NOT NULL ON CONFLICT FAIL, 
  [CMU_SET_MEMBER_FK_DMU_SET_MEMBER_UUID_2] CHAR(36) NOT NULL ON CONFLICT FAIL, 
  [CMU_SET_MEMBER_FK_DMU_SET_MEMBER_FK_DMU_CATEGORY_UUID_2] CHAR(36) NOT NULL ON CONFLICT FAIL, 
  CONSTRAINT [CMU_SET_MEMBER_FK_DMU_SET_MEMBER_UUID_2__FK__DMU_SET_MEMBER_UUID] FOREIGN KEY([CMU_SET_MEMBER_FK_DMU_SET_MEMBER_UUID_2], [CMU_SET_MEMBER_FK_DMU_SET_MEMBER_FK_DMU_CATEGORY_UUID_2]) REFERENCES [DMU_SET_MEMBER]([DMU_SET_MEMBER_UUID], [DMU_SET_MEMBER_FK_DMU_CATEGORY_UUID]) ON DELETE CASCADE ON UPDATE CASCADE, 
  CONSTRAINT [CMU_SET_MEMBER_FK_DMU_SET_MEMBER_UUID_1__FK__DMU_SET_MEMBER_UUID] FOREIGN KEY([CMU_SET_MEMBER_FK_DMU_SET_MEMBER_UUID_1], [CMU_SET_MEMBER_FK_DMU_SET_MEMBER_FK_DMU_CATEGORY_UUID_1]) REFERENCES [DMU_SET_MEMBER]([DMU_SET_MEMBER_UUID], [DMU_SET_MEMBER_FK_DMU_CATEGORY_UUID]) ON DELETE CASCADE ON UPDATE CASCADE, 
  CONSTRAINT [sqlite_autoindex_CMU_SET_MEMBER_2] PRIMARY KEY ([CMU_SET_MEMBER_FK_CMU_CATEGORY_UUID], [CMU_SET_MEMBER_FK_DMU_SET_MEMBER_UUID_1], [CMU_SET_MEMBER_FK_DMU_SET_MEMBER_UUID_2]) ON CONFLICT FAIL);

/* Drop table [main].[DMU_CATEGORY] */
drop table if exists [main].[DMU_CATEGORY];

/* Table structure [main].[DMU_CATEGORY] */
CREATE TABLE [main].[DMU_CATEGORY] (
  [DMU_CATEGORY_UUID] char(36) NOT NULL ON CONFLICT FAIL CONSTRAINT [DMU_CATEGORY_UUID_CONSTRAINT] UNIQUE ON CONFLICT FAIL, 
  [DMU_CATEGORY_STRING_CODE] varchar(128) NOT NULL ON CONFLICT FAIL CONSTRAINT [DMU_CATEGORY_STRING_CODE_CONSTRAINT] UNIQUE ON CONFLICT FAIL, 
  [DMU_CATEGORY_STRING_LONGHAND] varchar(4096), 
  [DMU_CATEGORY_NOTES1] varchar(16384), 
  [DMU_CATEGORY_NOTES2] varchar(16384), 
  [DMU_CATEGORY_NOTES3] varchar(16384), 
  [DMU_CATEGORY_FLAGS] CHAR(16), 
  CONSTRAINT [sqlite_autoindex_DMU_CATEGORY_2] PRIMARY KEY ([DMU_CATEGORY_STRING_CODE]) ON CONFLICT FAIL);

/* Drop table [main].[DMU_SET_MEMBER] */
drop table if exists [main].[DMU_SET_MEMBER];

/* Table structure [main].[DMU_SET_MEMBER] */
CREATE TABLE [main].[DMU_SET_MEMBER] (
  [DMU_SET_MEMBER_UUID] char(36) NOT NULL ON CONFLICT Fail, 
  [DMU_SET_MEMBER_STRING_CODE] varchar(128), 
  [DMU_SET_MEMBER_STRING_LONGHAND] varchar(4096), 
  [DMU_SET_MEMBER_NOTES1] varchar(16384), 
  [DMU_SET_MEMBER_NOTES2] varchar(16384), 
  [DMU_SET_MEMBER_NOTES3] varhcar(16384), 
  [DMU_SET_MEMBER_FK_DMU_CATEGORY_UUID] char(36) NOT NULL ON CONFLICT Fail REFERENCES [DMU_CATEGORY]([DMU_CATEGORY_UUID]) ON DELETE CASCADE ON UPDATE CASCADE, 
  [DMU_SET_MEMBER_FLAGS] CHAR(16), 
  CONSTRAINT [DMU_SET_MEMBER__UNIQUE] UNIQUE([DMU_SET_MEMBER_UUID], [DMU_SET_MEMBER_FK_DMU_CATEGORY_UUID]) ON CONFLICT FAIL);

/* Drop table [main].[TIME_GRANULARITY] */
drop table if exists [main].[TIME_GRANULARITY];

/* Table structure [main].[TIME_GRANULARITY] */
CREATE TABLE [main].[TIME_GRANULARITY] (
  [TIME_GRANULARITY_CODE] INT NOT NULL ON CONFLICT FAIL, 
  [TIME_GRANULARITY_STRING_CODE] CHAR(6) NOT NULL ON CONFLICT FAIL CONSTRAINT [TIME_GRANULARITY_STRING_CODE_CONSTRAINT] UNIQUE ON CONFLICT FAIL, 
  [TIME_GRANULARITY_DESCRIPTION] VARCHAR(4096) NOT NULL ON CONFLICT FAIL, 
  CONSTRAINT [sqlite_autoindex_TIME_GRANULARITY_2] PRIMARY KEY ([TIME_GRANULARITY_CODE]));

/* Data [main].[TIME_GRANULARITY] */
insert into [main].[TIME_GRANULARITY] values(10, 'sec', 'second');
insert into [main].[TIME_GRANULARITY] values(20, 'min', 'minute');
insert into [main].[TIME_GRANULARITY] values(30, 'hr', 'hour');
insert into [main].[TIME_GRANULARITY] values(40, 'day', 'day');
insert into [main].[TIME_GRANULARITY] values(50, 'wk', 'week');
insert into [main].[TIME_GRANULARITY] values(60, 'mth', 'month');
insert into [main].[TIME_GRANULARITY] values(70, 'qt', 'quarter');
insert into [main].[TIME_GRANULARITY] values(80, 'yr', 'year');
insert into [main].[TIME_GRANULARITY] values(90, 'bi', 'biennial');
insert into [main].[TIME_GRANULARITY] values(100, 'qd', 'quadrennial');
insert into [main].[TIME_GRANULARITY] values(110, 'dec', 'decade');
insert into [main].[TIME_GRANULARITY] values(120, 'cnt', 'century');
insert into [main].[TIME_GRANULARITY] values(130, 'mil', 'millenium');
insert into [main].[TIME_GRANULARITY] values(0, 'none', 'none');


/* Drop table [main].[UOA_CATEGORY] */
drop table if exists [main].[UOA_CATEGORY];

/* Table structure [main].[UOA_CATEGORY] */
CREATE TABLE [main].[UOA_CATEGORY] (
  [UOA_CATEGORY_UUID] CHAR(36) NOT NULL ON CONFLICT FAIL, 
  [UOA_CATEGORY_STRING_CODE] VARCHAR(128), 
  [UOA_CATEGORY_STRING_LONGHAND] VARCHAR(4096), 
  [UOA_CATEGORY_TIME_GRANULARITY] INT NOT NULL ON CONFLICT FAIL CONSTRAINT [UOA_CATEGORY_TIME_GRANULARITY__FK__TIME_GRANULARITY_CODE] REFERENCES [TIME_GRANULARITY]([TIME_GRANULARITY_CODE]) ON DELETE CASCADE ON UPDATE CASCADE, 
  [UOA_CATEGORY_NOTES1] VARCHAR(16384), 
  [UOA_CATEGORY_NOTES2] VARCHAR(16384), 
  [UOA_CATEGORY_NOTES3] VARCHAR(16384), 
  [UOA_CATEGORY_FLAGS] CHAR(128), 
  CONSTRAINT [sqlite_autoindex_UOA_CATEGORY_1] PRIMARY KEY ([UOA_CATEGORY_UUID]));

/* Drop table [main].[UOA_CATEGORY_LOOKUP] */
drop table if exists [main].[UOA_CATEGORY_LOOKUP];

/* Table structure [main].[UOA_CATEGORY_LOOKUP] */
CREATE TABLE [main].[UOA_CATEGORY_LOOKUP] (
  [UOA_CATEGORY_LOOKUP_FK_UOA_CATEGORY_UUID] CHAR(36) NOT NULL ON CONFLICT FAIL CONSTRAINT [UOA_CATEGORY_LOOKUP_FK_UOA_CATEGORY_UUID__FK__UOA_CATEGORY_UUID] REFERENCES [UOA_CATEGORY]([UOA_CATEGORY_UUID]) ON DELETE CASCADE ON UPDATE CASCADE, 
  [UOA_CATEGORY_LOOKUP_SEQUENCE_NUMBER] INT NOT NULL ON CONFLICT FAIL, 
  [UOA_CATEGORY_LOOKUP_FK_DMU_CATEGORY_UUID] CHAR(36) NOT NULL ON CONFLICT FAIL CONSTRAINT [UOA_CATEGORY_LOOKUP_FK_DMU_CATEGORY_UUID__FK__DMU_CATEGORY_UUID] REFERENCES [DMU_CATEGORY]([DMU_CATEGORY_UUID]) ON DELETE CASCADE ON UPDATE CASCADE, 
  CONSTRAINT [sqlite_autoindex_UOA_CATEGORY_LOOKUP_1] PRIMARY KEY ([UOA_CATEGORY_LOOKUP_FK_UOA_CATEGORY_UUID], [UOA_CATEGORY_LOOKUP_FK_DMU_CATEGORY_UUID], [UOA_CATEGORY_LOOKUP_SEQUENCE_NUMBER]) ON CONFLICT FAIL);

/* Drop table [main].[VG_CATEGORY] */
drop table if exists [main].[VG_CATEGORY];

/* Table structure [main].[VG_CATEGORY] */
CREATE TABLE [main].[VG_CATEGORY] (
  [VG_CATEGORY_UUID] CHAR(36) NOT NULL ON CONFLICT FAIL CONSTRAINT [VG_CATEGORY_UUID_CONSTRAINT] UNIQUE ON CONFLICT FAIL, 
  [VG_CATEGORY_STRING_CODE] VARCHAR(128) NOT NULL ON CONFLICT FAIL CONSTRAINT [VG_CATEGORY_CODE_CONSTRAINT] UNIQUE ON CONFLICT FAIL, 
  [VG_CATEGORY_STRING_LONGHAND] VARCHAR(4096) NOT NULL ON CONFLICT FAIL, 
  [VG_CATEGORY_NOTES1] VARCHAR(16384), 
  [VG_CATEGORY_NOTES2] VARCHAR(16384), 
  [VG_CATEGORY_NOTES3] VARCHAR(16384), 
  [VG_CATEGORY_FK_UOA_CATEGORY_UUID] CHAR(36) NOT NULL ON CONFLICT FAIL CONSTRAINT [VG_CATEGORY_FK_UOA_CATEGORY_UUID__FK__UOA_CATEGORY_UUID] REFERENCES [UOA_CATEGORY]([UOA_CATEGORY_UUID]) ON DELETE CASCADE ON UPDATE CASCADE, 
  [VG_CATEGORY_FLAGS] CHAR(16), 
  CONSTRAINT [sqlite_autoindex_VG_CATEGORY_2] PRIMARY KEY ([VG_CATEGORY_STRING_CODE]) ON CONFLICT FAIL);

/* Drop table [main].[VG_DATA_METADATA__DATETIME_COLUMNS] */
drop table if exists [main].[VG_DATA_METADATA__DATETIME_COLUMNS];

/* Table structure [main].[VG_DATA_METADATA__DATETIME_COLUMNS] */
CREATE TABLE [main].[VG_DATA_METADATA__DATETIME_COLUMNS] (
  [VG_DATA_TABLE_NAME] CHAR(255) NOT NULL ON CONFLICT FAIL, 
  [VG_DATETIME_START_COLUMN_NAME] CHAR(255) NOT NULL ON CONFLICT FAIL, 
  [VG_DATETIME_END_COLUMN_NAME] CHAR(255) NOT NULL ON CONFLICT FAIL, 
  [VG_DATA_FK_VG_CATEGORY_UUID] VARCHAR(128) NOT NULL ON CONFLICT FAIL REFERENCES [VG_CATEGORY]([VG_CATEGORY_UUID]) ON DELETE CASCADE ON UPDATE CASCADE, 
  CONSTRAINT [sqlite_autoindex_VG_DATA_METADATA__DATETIME_COLUMNS_1] PRIMARY KEY ([VG_DATA_FK_VG_CATEGORY_UUID]) ON CONFLICT FAIL);
CREATE UNIQUE INDEX [main].[VG_INSTANCE_DATA_DATETIME_COLUMNS_TABLE_NAME_INDEX] ON [VG_DATA_METADATA__DATETIME_COLUMNS] ([VG_DATA_TABLE_NAME]);

/* Drop table [main].[VG_DATA_METADATA__PRIMARY_KEYS] */
drop table if exists [main].[VG_DATA_METADATA__PRIMARY_KEYS];

/* Table structure [main].[VG_DATA_METADATA__PRIMARY_KEYS] */
CREATE TABLE [main].[VG_DATA_METADATA__PRIMARY_KEYS] (
  [VG_DATA_TABLE_NAME] CHAR(255) NOT NULL ON CONFLICT FAIL REFERENCES [VG_DATA_METADATA__DATETIME_COLUMNS]([VG_DATA_TABLE_NAME]) ON DELETE CASCADE ON UPDATE CASCADE, 
  [VG_DATA_TABLE_PRIMARY_KEY_COLUMN_NAME] CHAR(256) NOT NULL ON CONFLICT FAIL, 
  [VG_DATA_TABLE_FK_DMU_CATEGORY_CODE] VARCHAR(128) NOT NULL ON CONFLICT FAIL, 
  [VG_DATA_TABLE_PRIMARY_KEY_SEQUENCE_NUMBER] INT NOT NULL ON CONFLICT FAIL, 
  [VG_DATA_TABLE_PRIMARY_KEY_FIELD_TYPE] VARCHAR(32) NOT NULL ON CONFLICT FAIL);

/* Drop table [main].[VG_SET_MEMBER] */
drop table if exists [main].[VG_SET_MEMBER];

/* Table structure [main].[VG_SET_MEMBER] */
CREATE TABLE [main].[VG_SET_MEMBER] (
  [VG_SET_MEMBER_UUID] CHAR(36) NOT NULL ON CONFLICT FAIL CONSTRAINT [VG_SET_MEMBER_UUID_CONSTRAINT] UNIQUE ON CONFLICT FAIL, 
  [VG_SET_MEMBER_STRING_CODE] VARCHAR(128) NOT NULL ON CONFLICT FAIL, 
  [VG_SET_MEMBER_STRING_LONGHAND] VARCHAR(4096) NOT NULL ON CONFLICT FAIL, 
  [VG_SET_MEMBER_SEQUENCE_NUMBER] INT NOT NULL ON CONFLICT FAIL, 
  [VG_SET_MEMBER_NOTES1] VARCHAR(16384), 
  [VG_SET_MEMBER_NOTES2] VARCHAR(16384), 
  [VG_SET_MEMBER_NOTES3] VARCHAR(16384), 
  [VG_SET_MEMBER_FK_VG_CATEGORY_UUID] CHAR(36) NOT NULL ON CONFLICT FAIL CONSTRAINT [VG_SET_MEMBER_FK_VG_CATEGORY_UUID__FK__VG_CATEGORY_UUID] REFERENCES [VG_CATEGORY]([VG_CATEGORY_UUID]) ON DELETE CASCADE ON UPDATE CASCADE, 
  [VG_SET_MEMBER_FLAGS] CHAR(16), 
  [VG_SET_MEMBER_DATA_TYPE] CHAR(32), 
  CONSTRAINT [sqlite_autoindex_VG_SET_MEMBER_2] PRIMARY KEY ([VG_SET_MEMBER_FK_VG_CATEGORY_UUID], [VG_SET_MEMBER_STRING_CODE]) ON CONFLICT FAIL);

/* Commit Transaction */
commit transaction;

/* Enable Foreign Keys */
pragma foreign_keys = on;





	)~~~");

}
