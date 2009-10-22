
#include <boost/asio.hpp>
#include <map>
#include <vector>
#include <queue>
#include "../asyncCraqUtil.hpp"
#include "asyncConnectionGet.hpp"



#ifndef __ASYNC_CRAQ_GET_CLASS_H__
#define __ASYNC_CRAQ_GET_CLASS_H__

namespace CBR
{


class AsyncCraqGet
{
public:
  AsyncCraqGet();
  ~AsyncCraqGet();
  
  int runReQuery();
  
  enum AsyncCraqReqStatus{REQUEST_PROCESSED, REQUEST_NOT_PROCESSED};

  void initialize(std::vector<CraqInitializeArgs>);

  boost::asio::io_service io_service;  //creates an io service

  int set(CraqDataSetGet cdSet);
  int get(CraqDataSetGet cdGet);

  void runTestOfConnection();
  void runTestOfAllConnections();
  void tick(std::vector<CraqOperationResult*>&getResults, std::vector<CraqOperationResult*>&trackedSetResults);

  int queueSize();
  int numStillProcessing();
  
private:
  
  void processGetResults       (std::vector <CraqOperationResult*> & getRes);
  void processErrorResults     (std::vector <CraqOperationResult*> & errorRes);
  void processTrackedSetResults(std::vector <CraqOperationResult*> & trackedSetRes);

  void straightPoll();
  std::vector<CraqInitializeArgs> mIpAddPort;
  std::vector<AsyncConnectionGet*> mConnections;
  int mCurrentTrackNum;
  bool connected;

  std::queue<CraqDataSetGet> mQueue;
  

  void reInitializeNode(int s);
  void checkConnections(int s);
  
};


}//end namespace

#endif


