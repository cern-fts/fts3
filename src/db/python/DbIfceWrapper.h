#ifndef DBIFCEWRAPPER_H
#define	DBIFCEWRAPPER_H

#include <boost/noncopyable.hpp>
#include <db/generic/GenericDbIfce.h>

class DbIfceWrapper: public boost::noncopyable {
 public:
  static DbIfceWrapper& getInstance(void);
   
  void init(std::string username, std::string password, std::string connectString);
   
  std::vector<TransferFiles>      getByJobId(std::vector<TransferJobs>& jobs);
  std::vector<FileTransferStatus> getTransferFileStatus(std::string requestID);
  std::vector<JobStatus>          getTransferJobStatus(std::string requestID);
  std::vector<TransferJobs>       getSubmittedJobs(void);
   
  std::vector<std::string> getSiteGroupNames(void);
  std::vector<std::string> getSiteGroupMembers(std::string GroupName);
   
  std::vector<JobStatus> listRequests(std::vector<std::string>& inGivenStates,
                                       std::string restrictToClientDN,
                                       std::string forDN, std::string VOname);
   
  
  Se                    getSe(std::string seName);
  std::vector<Se>       getAllSeInfoNoCriteria(void);
  std::set<std::string> getAllMatchingSeNames(std::string name);
  std::set<std::string> getAllMatchingSeGroupNames(std::string name);
  std::vector<SeConfig> getAllShareConfigNoCriteria(void);
  std::vector<SeAndConfig*> getAllShareAndConfigWithCriteria(std::string SE_NAME,
                                                             std::string SHARE_ID,
                                                             std::string SHARE_TYPE,
                                                             std::string SHARE_VALUE);
  
  int getSeCreditsInUse(std::string srcSeName, std::string destSeName, std::string voName);
  int getGroupCreditsInUse(std::string srcSeName, std::string destSeName, std::string voName);
   
 private:
   static DbIfceWrapper instance;
   GenericDbIfce* fts3db;
   
   DbIfceWrapper();
   ~DbIfceWrapper();
};


#endif // DBIFCEWRAPPER_H
