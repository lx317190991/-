#ifndef __CLOUDDATABASEHANDLER__HH__
#define __CLOUDDATABASEHANDLER__HH__
#include <string.h>
#include "zookeeper.h"
#include "zookeeper.jute.h"
#include "zookeeper_log.h"
#include "Bmco/Util/Application.h"
#include "Bmco/Util/ServerApplication.h"
#include "Bmco/Condition.h"
#include "Bmco/Mutex.h"

//#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/lock_guard.hpp>
//#include "ResultIniFormat.h"
//#include "CFileOperInterface.h"
#include "BolCommon/ControlRegionOp.h"
#include "BolCommon/ControlRegionBasedTask.h"

const std::string ROOTDIR = "/groupmembers";
using namespace Bmco;
using namespace Bmco::Util;

namespace BM35
{
/*	void CloudAgent_dump_stat(const struct Stat *stat)
	{
		char tctimes[40];
		char tmtimes[40];
		time_t tctime;
		time_t tmtime;
		if (!stat)
		{
			fprintf(stderr, "null\n");
			return;
		}
		tctime = stat->ctime / 1000;
		tmtime = stat->mtime / 1000;
		ctime_r(&tmtime, tmtimes);
		ctime_r(&tctime, tctimes);
		fprintf(stderr, "\tctime = %s\tczxid=%llx\n"
		        "\tmtime=%s\tmzxid=%llx\n"
		        "\tversion=%x\taversion=%x\n"
		        "\tephemeralOwner = %llx\n",
		        tctimes, stat->czxid,
		        tmtimes, stat->mzxid,
		        (unsigned int) stat->version,
		        (unsigned int) stat->aversion,
		        stat->ephemeralOwner);
	}
*/
	/// This class get the changed data from zookeeper server
	/// which will be called by the Bol_Cloud_Sync_Task_Loop_Thread
class CloudDatabaseHandler 
{
	typedef BIP::interprocess_sharable_mutex	RWMtxType;
	typedef BIP::sharable_lock<RWMtxType>		SHARABLE_LOCK;
	typedef BIP::scoped_lock<RWMtxType> 		ECLUSIVE_LOCK;

    typedef struct _SyncData 
	{
        bool isDataReady;
        int flag;
		int curFlag;
        std::string data;
        std::string tmpPath; //!< store tmporary path -- only used by create node now.
		std::string nodePath;
		//boost::mutex mutex;
        //boost::condition_variable condition;
        Bmco::Condition _cond;
		Bmco::Mutex _mutex;
        zhandle_t *zkHandle;
        Bmco::Logger &logger;
        _SyncData(zhandle_t *h, Bmco::Logger &l) : isDataReady(false), zkHandle(h), logger(l) 
		{
    	}
		void wait()
		{
			_mutex.lock();
			_cond.wait(_mutex);
			_mutex.unlock();
		}
		void signal()
		{	
			_cond.signal();
		}
    } SyncData;

	// ���ӱ仯�¼���ʵ��
	// ���ݱ仯�˲����Ѿ����ûص������ı�ʶnotifyReady
	// �仯�˵�����data
	/*typedef struct _MonitorData
	{
		bool notifyReady;
		std::string data;
		_MonitorData() : notifyReady(true){}
	}MonitorData;*/
	
public:
	CloudDatabaseHandler()
		: theLogger(Bmco::Util::Application::instance().logger()), m_zkHandle(NULL),
		m_config(Bmco::Util::Application::instance().config()) 
	{
		setZKLogFilePoint();
	}
	~CloudDatabaseHandler() 
	{
        if (m_zkHandle != NULL) 
		{
			zookeeper_close(m_zkHandle);
        }
		if (m_fp != NULL)
		{
			fclose(m_fp);
		}
	}
    // Do initialization.
	// bool init(std::string hosts = "");
	bool init();
	
	// ��Ȩ��Ч���
	bool isValidHandle();

	// ����zk�ͻ�����־�ļ���ָ��
	void setZKLogFilePoint();
	
	// ���zookeeper�������
	void clearZkHandle();
	
	// ��ȡzookeeper���Ӵ�
	bool loadXMLParamForZK(const std::string fileName, 
		std::string &hostName);
	
	// ��·�����ڵ�ݹ鴴��·��
	int createRootNodeAsync(const SyncData &sync);
	
	// Create node. flag:ZOO_EPHEMERAL(��ʱ) or 0(����)
    bool createNode(std::string nodePath, int flag = 0);
	
    // Check node existence.
    bool nodeExist(std::string nodePath);
	
    //Get data of given node.
    int getNodeData(std::string nodePath, std::string &data);
    //Monitor given node and get the changed data.
    //int getChangedData(std::string nodePath, std::string &data);
	//Set node data.
    bool setNodeData(std::string nodePath, std::string buffer);
    /*! 
         *  \brief Get slave number.
         *  \retval -1 Failed to get slave number.
         *  \retval >=0 Slave number.
         */
    // int getSlaveNumber();
    
	//Get node's children node.
	bool GetChildrenNode(std::string nodePath, std::vector<std::string> &children);

	// set watcher if no watcher
    // get if changed or not flag if has watcher
	bool getNodeChangedFlag(std::string nodePath, bool& isChanged);

	// get changed data async
	bool getChangedData(std::string nodePath);

	// delete node sync
	bool removeNode(std::string nodePath);

	// ����ZK�ͻ�����־�ļ�
	void rebuildLogFile();

	// ˢ�¼������м���
	void refreshChangedMap();

	// ����������ʶ���ڵ�
	bool createGroupNode();
	
public:
    static CloudDatabaseHandler *instance() 
	{
        boost::mutex::scoped_lock lock(_instanceMutex);
        if (_pInstance == NULL) 
		{
            _pInstance = new CloudDatabaseHandler();
        }
        if (_pInstance != NULL) 
		{
            try 
			{
				// ��������ã���Ҫ���½�����ʼ����������ʱ�ڵ�
				if (!_pInstance->isValidHandle())
				{
					if (!_pInstance->init()
						|| !_pInstance->createGroupNode()) 
					{
						throw(Bmco::Exception(std::string("init zkhandler or create ephnode failed!!!")));
	                }
				}
            } 
			catch(Bmco::Exception &e)
			{

				bmco_error_f3(_pInstance->theLogger,"%s|%s|%s",std::string("0"),std::string(__FUNCTION__),e.displayText());
				delete _pInstance;
	            _pInstance = NULL;
				return NULL;
			}
			catch (...) 
            {
				bmco_error_f2(_pInstance->theLogger,"%s|%s|zkhandle operate error!!!",std::string("0"),std::string(__FUNCTION__));
				delete _pInstance;
                _pInstance = NULL;
				return NULL;
            }
        }
        return _pInstance;
    }

	// Async Callback functions
	
	static void releaseStaticHandle();
    //Callback for checking createNode result.
    static void createNodeCompletion(int rc, const char *name, const void *data);
    //Callback for getting set result.
    static void setNodeDataCompletion(int rc, const struct Stat *stat, const void *data);
    //Callback for getting watch event.
    static void changedNodeDataWatcher(zhandle_t *zh, int type, int state,
                              const char *path, void *watcherCtx);
    //Callback for getting node data.
    static void nodeDataCompletion(int rc, const char *value, int value_len,
                                    const struct Stat *stat, const void *data);
    //Callback for getting changed node data.
    static void nodeChangedDataCompletion(int rc, const char *value, int value_len,
                                    const struct Stat *stat, const void *data);
private:
	/// the zkserver connecting string
	Bmco::Logger& theLogger;
	zhandle_t *m_zkHandle; //!< the handle to access zookeeper server
    Bmco::Util::AbstractConfiguration &m_config;
    static CloudDatabaseHandler *_pInstance;
    static boost::mutex _instanceMutex;
	//static std::map<std::string, MonitorData> m_dataVec;
	static std::map<std::string, bool> m_changePath;
	std::string m_logPath;

	RWMtxType rwlock;

	FILE *m_fp;
	//static bool m_notifyReady;
	//static std::map<std::string, std::string> m_dataVec;
};
} //namespace BM35
#endif //__CLOUDDATABASEHANDLER__HH__

