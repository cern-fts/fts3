#include <config/serverconfig.h>
#include <db/generic/SingleDbInstance.h>
#include "DbIfceWrapper.h"

DbIfceWrapper DbIfceWrapper::instance;



DbIfceWrapper::DbIfceWrapper()
{
  fts3::config::theServerConfig().read(0, NULL);
  fts3db = db::DBSingleton::instance().getDBObjectInstance();
}



DbIfceWrapper::~DbIfceWrapper()
{
  // Nothing
}



DbIfceWrapper& DbIfceWrapper::getInstance(void)
{
  return instance;
}



void DbIfceWrapper::init(std::string username, std::string password,
                         std::string connectString)
{
  fts3db->init(username, password, connectString);
}



std::vector<TransferFiles> DbIfceWrapper::getByJobId(std::vector<TransferJobs>& jobs)
{
  std::vector<TransferJobs*> tjobs;
  std::vector<TransferJobs>::iterator tjobi;
  for (tjobi = jobs.begin(); tjobi != jobs.end(); ++tjobi) {
    tjobs.push_back(&(*tjobi));
  }
  
  std::vector<TransferFiles*> tfiles;
  fts3db->getByJobId(tjobs, tfiles);
  
  std::vector<TransferFiles> copy;
  std::vector<TransferFiles*>::const_iterator i;
  for (i = tfiles.begin(); i != tfiles.end(); ++i) {
    copy.push_back(*(*i));
    delete *i;
  }
  
  return copy;
}



std::vector<FileTransferStatus> DbIfceWrapper::getTransferFileStatus(std::string requestID)
{
  std::vector<FileTransferStatus*> tstatus;
  fts3db->getTransferFileStatus(requestID, tstatus);

  std::vector<FileTransferStatus> copy;
  std::vector<FileTransferStatus*>::const_iterator i;
  for (i = tstatus.begin(); i != tstatus.end(); ++i) {
    copy.push_back(*(*i));
    delete *i;
  }
  
  return copy;
}



std::vector<JobStatus> DbIfceWrapper::getTransferJobStatus(std::string requestID)
{
  std::vector<JobStatus*> statuses;
  fts3db->getTransferJobStatus(requestID, statuses);
  
  std::vector<JobStatus> copy;
  std::vector<JobStatus*>::const_iterator i;
  for (i = statuses.begin(); i != statuses.end(); ++i) {
    copy.push_back(*(*i));
    delete *i;
  }
  
  return copy;
}



std::vector<TransferJobs> DbIfceWrapper::getSubmittedJobs(void)
{
  const string vos("");
  std::vector<TransferJobs*> jobs;
  fts3db->getSubmittedJobs(jobs,vos);
  
  std::vector<TransferJobs> copy;
  std::vector<TransferJobs*>::const_iterator i;
  for (i = jobs.begin(); i != jobs.end(); ++i) {
    copy.push_back(*(*i));
    delete *i;
  }
  
  return copy;
}



std::vector<std::string> DbIfceWrapper::getSiteGroupNames(void)
{
  return fts3db->getSiteGroupNames();
}



std::vector<std::string> DbIfceWrapper::getSiteGroupMembers(std::string GroupName)
{
  return fts3db->getSiteGroupMembers(GroupName);
}



std::vector<JobStatus> DbIfceWrapper::listRequests(std::vector<std::string>& inGivenStates,
                                                   std::string restrictToClientDN,
                                                   std::string forDN,
                                                   std::string VOname)
{
  std::vector<JobStatus*> jstatus;
  fts3db->listRequests(jstatus, inGivenStates, restrictToClientDN, forDN, VOname);
  
  std::vector<JobStatus> copy;
  std::vector<JobStatus*>::const_iterator i;
  for (i = jstatus.begin(); i != jstatus.end(); ++i) {
    copy.push_back(*(*i));
    delete *i;
  }
  
  return copy;
}



Se DbIfceWrapper::getSe(std::string seName)
{
  Se* se = NULL, copy;
  fts3db->getSe(se, seName);
  if (se) {
    copy = *se;
    delete se;
  }
  return copy;
}



std::vector<Se> DbIfceWrapper::getAllSeInfoNoCriteria(void)
{
  std::vector<Se*> ses;
  fts3db->getAllSeInfoNoCritiria(ses);
  
  std::vector<Se> copy;
  std::vector<Se*>::const_iterator i;
  for (i = ses.begin(); i != ses.end(); ++i) {
    copy.push_back(*(*i));
    delete *i;
  }
  
  return copy;
}



std::set<std::string> DbIfceWrapper::getAllMatchingSeNames(std::string name)
{
  return fts3db->getAllMatchingSeNames(name);
}



std::set<std::string> DbIfceWrapper::getAllMatchingSeGroupNames(std::string name)
{
  return fts3db->getAllMatchingSeGroupNames(name);
}



std::vector<SeConfig> DbIfceWrapper::getAllShareConfigNoCriteria(void)
{
  std::vector<SeConfig*> config;
  fts3db->getAllShareConfigNoCritiria(config);
  
  std::vector<SeConfig> copy;
  std::vector<SeConfig*>::const_iterator i;
  for (i = config.begin(); i != config.end(); ++i) {
    copy.push_back(*(*i));
    delete *i;
  }
  
  return copy;
}



std::vector<SeAndConfig*> DbIfceWrapper::getAllShareAndConfigWithCriteria(std::string SE_NAME,
                                                                          std::string SHARE_ID,
                                                                          std::string SHARE_TYPE,
                                                                          std::string SHARE_VALUE)
{
  std::vector<SeAndConfig*> seconfig;
  fts3db->getAllShareAndConfigWithCritiria(seconfig, SE_NAME, SHARE_ID,
                                           SHARE_TYPE, SHARE_VALUE);
  return seconfig;
}



int DbIfceWrapper::getSeCreditsInUse(std::string srcSeName,
                                     std::string destSeName,
                                     std::string voName)
{
  int v;
  fts3db->getSeCreditsInUse(v, srcSeName, destSeName, voName);
  return v;
}



int DbIfceWrapper::getGroupCreditsInUse(std::string srcSeName,
                                        std::string destSeName,
                                        std::string voName)
{
  int v;
  fts3db->getGroupCreditsInUse(v, srcSeName, destSeName, voName);
  return v;
}
