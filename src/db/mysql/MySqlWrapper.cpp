/// @file   MySqlWrapper.cpp
/// @brief  MySQL Wrapper.
/// @author Alejandro Álvarez Ayllón <aalvarez@cern.ch>
#include "MySqlWrapper.h"

#include <cstring>
#include <cstdlib>
#include <mysql/mysqld_error.h>
#include <vector>

using namespace dmlite;

#define ASSIGN_POINTER_TYPECAST(type, where, value) (*((type*)where)) = value

#define SANITY_CHECK(status, method) \
if (this->status_ != status)\
  throw DmException(DMLITE_SYSERR(DMLITE_INTERNAL_ERROR),\
                    #method" called out of order");\

#define BIND_PARAM_SANITY() \
if (this->status_ != STMT_CREATED)\
  throw DmException(DMLITE_SYSERR(DMLITE_INTERNAL_ERROR),\
                    "bindParam called out of order");\
if (index > this->nParams_)\
  throw DmException(DMLITE_SYSERR(DMLITE_INTERNAL_ERROR),\
                    "Wrong index in bindParam");

#define BIND_RESULTS_SANITY() \
if (this->status_ < STMT_EXECUTED || this->status_ > STMT_RESULTS_BOUND)\
  throw DmException(DMLITE_SYSERR(DMLITE_INTERNAL_ERROR),\
                    "bindResult called out of order");\
if (index > this->nFields_)\
  throw DmException(DMLITE_SYSERR(DMLITE_INTERNAL_ERROR),\
                    "Wrong index in bindResult");



Statement::Statement(MYSQL* conn, const std::string& db, const char* query) throw (DmException):
  nFields_(0), result_(NULL), status_(STMT_CREATED)
{
  if (mysql_select_db(conn, db.c_str()) != 0)
    throw DmException(DMLITE_DBERR(mysql_errno(conn)),
                      std::string(mysql_error(conn)));

  this->stmt_ = mysql_stmt_init(conn);
  if (mysql_stmt_prepare(this->stmt_, query, strlen(query)) != 0)
    this->throwException();
  
  this->nParams_ = mysql_stmt_param_count(this->stmt_);
  this->params_  = new MYSQL_BIND [this->nParams_];
  std::memset(this->params_, 0, sizeof(MYSQL_BIND) * this->nParams_);
}



Statement::~Statement() throw ()
{
  mysql_stmt_free_result(this->stmt_);
  
  // Free param array
  if (this->params_ != NULL) {
    for (unsigned long i = 0; i < this->nParams_; ++i) {
      if (this->params_[i].buffer != NULL)
        std::free(this->params_[i].buffer);
      if (this->params_[i].length != NULL)
        std::free(this->params_[i].length);
    }
    delete [] this->params_;
  }

  // Free result array
  if (this->result_ != NULL) {
    delete [] this->result_;
  }

  // Close statement
  mysql_stmt_close(this->stmt_);
}



void Statement::bindParam(unsigned index, unsigned long value) throw (DmException)
{
  BIND_PARAM_SANITY();
  params_[index].buffer_type   = MYSQL_TYPE_LONG;
  params_[index].buffer        = std::malloc(sizeof(unsigned long));
  params_[index].is_unsigned   = true;
  params_[index].is_null_value = false;
  ASSIGN_POINTER_TYPECAST(unsigned long, params_[index].buffer, value);
}



void Statement::bindParam(unsigned index, const std::string& value) throw (DmException)
{
  BIND_PARAM_SANITY();

  size_t size = value.length();
  params_[index].buffer_type   = MYSQL_TYPE_VARCHAR;
  params_[index].length        = (unsigned long*)std::malloc(sizeof(unsigned long));
  params_[index].buffer        = std::malloc(sizeof(char) * size);
  params_[index].is_null_value = false;

  ASSIGN_POINTER_TYPECAST(unsigned long, params_[index].length, size);
  memcpy(params_[index].buffer, value.c_str(), size);
}



void Statement::bindParam(unsigned index, const char* value, size_t size) throw (DmException)
{
  BIND_PARAM_SANITY();

  params_[index].buffer_type  = MYSQL_TYPE_BLOB;
  params_[index].length_value = size;

  if (value != NULL) {
    params_[index].is_null_value = false;
    params_[index].buffer        = std::malloc(sizeof(char) * size);
    std::memcpy(params_[index].buffer, value, size);
  }
  else {
    params_[index].is_null_value = true;
  }
}



unsigned long Statement::execute(void) throw (DmException)
{
  SANITY_CHECK(STMT_CREATED, execute);

  mysql_stmt_bind_param(this->stmt_, this->params_);

  if (mysql_stmt_execute(this->stmt_) != 0)
    this->throwException();

  // Count fields and reserve
  MYSQL_RES* meta = mysql_stmt_result_metadata(this->stmt_);

  if (meta == NULL) {
    this->status_ = STMT_DONE; // No return set (UPDATE, DELETE, ...)
  }
  else {
    this->nFields_ = mysql_num_fields(meta);
    this->result_  = new MYSQL_BIND[this->nFields_];
    std::memset(this->result_, 0, sizeof(MYSQL_BIND) * this->nFields_);
    this->status_ = STMT_EXECUTED;
    // Free
    mysql_free_result(meta);
  }

  return (unsigned long) mysql_stmt_affected_rows(this->stmt_);
}



void Statement::bindResult(unsigned index, short* destination) throw (DmException)
{
  BIND_RESULTS_SANITY();
  result_[index].buffer_type = MYSQL_TYPE_SHORT;
  result_[index].buffer      = destination;
  result_[index].is_unsigned = false;
  this->status_ = STMT_RESULTS_UNBOUND;
}



void Statement::bindResult(unsigned index, signed int* destination) throw (DmException)
{
  BIND_RESULTS_SANITY();
  result_[index].buffer_type = MYSQL_TYPE_LONG;
  result_[index].buffer      = destination;
  result_[index].is_unsigned = false;
  this->status_ = STMT_RESULTS_UNBOUND;
}



void Statement::bindResult(unsigned index, unsigned int* destination) throw (DmException)
{
  BIND_RESULTS_SANITY();
  result_[index].buffer_type = MYSQL_TYPE_LONG;
  result_[index].buffer      = destination;
  result_[index].is_unsigned = true;
  this->status_ = STMT_RESULTS_UNBOUND;
}



void Statement::bindResult(unsigned index, signed long* destination) throw (DmException)
{
  BIND_RESULTS_SANITY();
  result_[index].buffer_type = MYSQL_TYPE_LONGLONG;
  result_[index].buffer      = destination;
  result_[index].is_unsigned = false;
  this->status_ = STMT_RESULTS_UNBOUND;
}



void Statement::bindResult(unsigned index, unsigned long* destination) throw (DmException)
{
  BIND_RESULTS_SANITY();
  result_[index].buffer_type = MYSQL_TYPE_LONGLONG;
  result_[index].buffer      = destination;
  result_[index].is_unsigned = true;
  this->status_ = STMT_RESULTS_UNBOUND;
}



void Statement::bindResult(unsigned index, signed long long* destination) throw (DmException)
{
  BIND_RESULTS_SANITY();
  result_[index].buffer_type = MYSQL_TYPE_LONGLONG;
  result_[index].buffer      = destination;
  result_[index].is_unsigned = false;
  this->status_ = STMT_RESULTS_UNBOUND;
}



void Statement::bindResult(unsigned index, unsigned long long* destination) throw (DmException)
{
  BIND_RESULTS_SANITY();
  result_[index].buffer_type = MYSQL_TYPE_LONGLONG;
  result_[index].buffer      = destination;
  result_[index].is_unsigned = true;
  this->status_ = STMT_RESULTS_UNBOUND;
}



void Statement::bindResult(unsigned index, char* destination, size_t size) throw (DmException)
{
  BIND_RESULTS_SANITY();
  result_[index].buffer_type   = MYSQL_TYPE_STRING;
  result_[index].buffer        = destination;
  result_[index].buffer_length = size;
  this->status_ = STMT_RESULTS_UNBOUND;
}



void Statement::bindResult(unsigned index, char* destination, size_t size, int) throw (DmException)
{
  BIND_RESULTS_SANITY();
  result_[index].buffer_type   = MYSQL_TYPE_BLOB;
  result_[index].buffer        = destination;
  result_[index].buffer_length = size;
  this->status_ = STMT_RESULTS_UNBOUND;
}



unsigned long Statement::count(void) throw ()
{
  if (this->status_ == STMT_RESULTS_UNBOUND) {
    mysql_stmt_bind_result(this->stmt_, this->result_);
    mysql_stmt_store_result(this->stmt_);
    this->status_ = STMT_RESULTS_BOUND;
  }
  SANITY_CHECK(STMT_RESULTS_BOUND, count);
  return mysql_stmt_num_rows(this->stmt_);
}



bool Statement::fetch(void) throw (DmException)
{
  if (this->status_ == STMT_RESULTS_UNBOUND) {
    mysql_stmt_bind_result(this->stmt_, this->result_);
    mysql_stmt_store_result(this->stmt_);
    this->status_ = STMT_RESULTS_BOUND;
  }

  SANITY_CHECK(STMT_RESULTS_BOUND, fetch);

  switch (mysql_stmt_fetch(this->stmt_)) {
    case 0:
      break;
    case MYSQL_NO_DATA:
      this->status_ = STMT_DONE;
      return false;
    default:
      this->throwException();
  }
  
  return true;
}



void Statement::throwException() throw (DmException)
{
  this->status_ = STMT_FAILED;
  throw DmException(DMLITE_DBERR(mysql_stmt_errno(this->stmt_)),
                    mysql_stmt_error(this->stmt_));
}
