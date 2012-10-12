#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <db/generic/Se.h>
#include <db/generic/SeConfig.h>
#include <db/generic/SeAndConfig.h>

using namespace boost::python;



inline bool operator == (const Se& a, const Se& b)
{
  return a.NAME == b.NAME;
}



inline bool operator == (const SeConfig& a, const SeConfig& b)
{
  return a.SE_NAME == b.SE_NAME && a.SHARE_ID == b.SHARE_ID;
}



inline bool operator == (const SeAndConfig& a, const SeAndConfig& b)
{
  return a.NAME == b.NAME;
}



void export_se_types(void)
{
  // Storage element and vector
  class_<Se>("Se", init<>())
    .def_readonly("ENDPOINT", &Se::ENDPOINT)
    .def_readonly("SE_TYPE", &Se::SE_TYPE)
    .def_readonly("SITE", &Se::SITE)
    .def_readonly("NAME", &Se::NAME)
    .def_readonly("STATE", &Se::STATE)
    .def_readonly("VERSION", &Se::VERSION)
    .def_readonly("HOST", &Se::HOST)
    .def_readonly("SE_TRANSFER_TYPE", &Se::SE_TRANSFER_TYPE)
    .def_readonly("SE_TRANSFER_PROTOCOL", &Se::SE_TRANSFER_PROTOCOL)
    .def_readonly("SE_CONTROL_PROTOCOL", &Se::SE_CONTROL_PROTOCOL)
    .def_readonly("GOCDB_ID", &Se::GOCDB_ID);
  
  class_< std::vector<Se> >("vector_Se")
    .def(vector_indexing_suite< std::vector<Se> >());
  
  // Storage element configuration and vector
  class_<SeConfig>("SeConfig", init<>())
    .def_readonly("SHARE_TYPE", &SeConfig::SHARE_TYPE)
    .def_readonly("SE_NAME", &SeConfig::SE_NAME)
    .def_readonly("SHARE_ID", &SeConfig::SHARE_ID)
    .def_readonly("SHARE_VALUE", &SeConfig::SHARE_VALUE);

  class_< std::vector<SeConfig> >("vector_SeConfig")
    .def(vector_indexing_suite< std::vector<SeConfig> >());
  
  // Storage element and configuration (and vector)
  class_<SeAndConfig>("SeAndConfig", init<>())
    .def_readonly("ENDPOINT", &SeAndConfig::ENDPOINT)
    .def_readonly("SE_TYPE", &SeAndConfig::SE_TYPE)
    .def_readonly("SITE", &SeAndConfig::SITE)
    .def_readonly("NAME", &SeAndConfig::NAME)
    .def_readonly("STATE", &SeAndConfig::STATE)
    .def_readonly("VERSION", &SeAndConfig::VERSION)
    .def_readonly("HOST", &SeAndConfig::HOST)
    .def_readonly("SE_TRANSFER_TYPE", &SeAndConfig::SE_TRANSFER_TYPE)
    .def_readonly("SE_TRANSFER_PROTOCOL", &SeAndConfig::SE_TRANSFER_PROTOCOL)
    .def_readonly("SE_CONTROL_PROTOCOL", &SeAndConfig::SE_CONTROL_PROTOCOL)
    .def_readonly("GOCDB_ID", &SeAndConfig::GOCDB_ID)
    .def_readonly("SE_NAME", &SeAndConfig::SE_NAME)
    .def_readonly("SHARE_TYPE", &SeAndConfig::SHARE_TYPE)
    .def_readonly("SHARE_ID", &SeAndConfig::SHARE_ID)
    .def_readonly("SHARE_VALUE", &SeAndConfig::SHARE_VALUE);

	class_< std::vector<SeAndConfig> >("vector_SeAndConfig")
    .def(vector_indexing_suite< std::vector<SeAndConfig> >());
}
