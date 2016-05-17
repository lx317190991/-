#ifndef __CLOUDKAFKAMESSAGEHANDLER__HH__
#define __CLOUDKAFKAMESSAGEHANDLER__HH__

#include "Consumer.h"
#include "Config.h"
//#include "KafkaWorker.h"
#include <string.h>
#include "Bmco/Util/Application.h"
#include "Bmco/Util/ServerApplication.h"
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/lock_guard.hpp>

using namespace Bmco;
using namespace Bmco::Util;
namespace BM35
{
class CloudKafkaMessageHandler {
	
public:
	CloudKafkaMessageHandler()
		: theLogger(Bmco::Util::Application::instance().logger()),
		m_config(Bmco::Util::Application::instance().config())
	{
		//m_worker = NULL;
		m_consume = NULL;
	}
	~CloudKafkaMessageHandler() 
	{	
		/*if (m_worker != NULL)
		{
			delete m_worker;
			m_worker = NULL;
		}*/
		if (m_consume != NULL)
		{
			delete m_consume;
			m_consume = NULL;
		}
	}
    //Do initialization.
	bool init(std::string brokerList, std::string zookeeperList = "");
public:
    static CloudKafkaMessageHandler *instance
		(std::string brokerList, std::string zookeeperList = "") 
	{
        boost::mutex::scoped_lock lock(_instanceMutex);
        if (_pInstance == NULL) {
            _pInstance = new CloudKafkaMessageHandler();
        }
        if (_pInstance != NULL) {
            try {
                if (!_pInstance->init(brokerList, zookeeperList)) {
                    delete _pInstance;
                    _pInstance = NULL;
                    return NULL;
                }
            } catch (...) {
                delete _pInstance;
                _pInstance = NULL;
            }
        }
        return _pInstance;
    }

	bool add_consume_new(std::string brokerList, 
			std::string zookeeperList);

	bool add_worker_new(std::string brokerList, 
			std::string zookeeperList);

	// ������Ϣ�ѻ���
    bool getMessageAccumuNumByPatition(std::string inTopicName,
	 	std::string inPartitionName, std::string outTopicName,
	 	std::string outPartitionName, Bmco::Int64 &accumulateNum);

	// ��ȡ��ǰ��дƫ����
	bool calcuMessageGrossByPartition(std::string topicName,
		std::string partitionName, Bmco::Int64 &offset);

	// ��ȡ���μ�¼�Ķ�ƫ����
	bool calcuReadMessageOffsetByPartition(std::string topicName,
	 	std::string partitionName, Bmco::Int64 &offset);

	// ��ȡָ������ķ�����
	// Bmco::Int32 getPartitionCount(std::string topicName);

	static void releaseStaticHandle();

	void releaseKafkaHandle();

	std::string getZookeeperList();

	// void setLastTestTimeStamp();

	// void getTestIntervalTime(std::string topicName);
	
private:
	Bmco::Logger& theLogger;
    Bmco::Util::AbstractConfiguration &m_config;
    static CloudKafkaMessageHandler *_pInstance;
    static boost::mutex _instanceMutex;
	dmsg::Consumer *m_consume;
	// kafkacpp::KafkaWorker *m_worker;
	std::string m_brokerList;
	std::string m_zookeeperList;

	// test
	// Bmco::Timestamp _lastTestTimeStamp;
};
} //namespace BM35
#endif //__CLOUDKafkaMessageHANDLER__HH__

