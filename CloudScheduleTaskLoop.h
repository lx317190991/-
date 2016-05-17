#ifndef __CLOUDSCHEDULETASKLOOP__HH__
#define __CLOUDSCHEDULETASKLOOP__HH__
#include <sstream>
#include "CloudKafkaMessageHandler.h"
#include "Bmco/Process.h"
#include "Bmco/Util/Application.h"

#include "IniFileConfigurationNew.h"
#include "Bmco/Util/IniFileConfiguration.h"
#include "Bmco/NumberFormatter.h"
#include "BolCommon/ControlRegionOp.h"
#include "BolCommon/ControlRegionBasedTask.h"
#include "CloudConfigurationUpdater.h"
#include "CloudDatabaseHandler.h"
#include "CloudCmdTaskLoop.h"

using Bmco::StringTokenizer;
// using namespace Bmco::Data;

namespace BM35
{
	// const std::string TABLE_BPCB_INFO = "c_info_bpcb";

	struct bolinfo
	{
		std::string bol_id;
		std::string bol_cloud_name;
		// std::string user_pwd_id;
		std::string node_id;
		std::string node_name;
		std::string ip_addr;
		std::string hostname;
		std::string auto_flex_flag;
		std::string master_flag;
		std::string subsys;
		std::string domainName;
		std::string nedir;
		std::string userName;
	};

	struct kpiinfo
	{
		std::string kpi_id;
		// std::string kpi_type;
		std::string seq_no;
		// std::string subsys_type;
		// std::string domian_type;
		std::string bol_cloud_name;
		std::string kpi_value;
		// std::string creata_date;
		// std::string modi_date;
	};

	struct flexinfo
	{
		std::string rule_id;
		std::string subsys_type;
		std::string domain_type;
		std::string rule_name;
		std::string rule_desc;
		std::string rule_type;
		std::string kpi_name_str;
		std::string value_str;
		std::string status;
		std::string create_date;
		std::string modi_date;
	};

	struct dictinfo
	{
		std::string id;
		std::string type;
		std::string key;
		std::string value;
		std::string desc;
		std::string begin_date;
		std::string end_date;
	};
	
	typedef struct Session
	{
		std::vector<struct kpiinfo> sessionKpiInfo;
		std::vector<struct bolinfo> sessionBolInfo;
		std::vector<struct flexinfo> sessionFlexInfo;
		std::vector<struct dictinfo> sessionDictInfo;
		std::string sessionSubSys;
		std::string sessionDomainName;
		std::string sessionBolName;
	}SessionContent;

	// ��ȡ�ĻỰ����
	// ����KPI,BOL,FLEX,DICT����Ϣ
	// Ӧ���ڶ���߳�֮��
	typedef struct sessionData
	{
		typedef BIP::interprocess_sharable_mutex	RWMtxType;
		typedef BIP::sharable_lock<RWMtxType>		SHARABLE_LOCK;
		typedef BIP::scoped_lock<RWMtxType> 		ECLUSIVE_LOCK;
		
		SessionContent m_sessionContent;
		RWMtxType rwlock;

		void setBolName(std::string bolName)
		{
			ECLUSIVE_LOCK _threadguard(rwlock);
			m_sessionContent.sessionBolName = bolName;
		}

		std::string getBolName()
		{
			SHARABLE_LOCK _threadguard(rwlock);
			return m_sessionContent.sessionBolName;
		}

		void setDomainName(std::string domainName)
		{
			ECLUSIVE_LOCK _threadguard(rwlock);
			m_sessionContent.sessionDomainName = domainName;
		}

		std::string getDomainName()
		{
			SHARABLE_LOCK _threadguard(rwlock);
			return m_sessionContent.sessionDomainName;
		}

		void setSubSys(std::string subSys)
		{
			ECLUSIVE_LOCK _threadguard(rwlock);
			m_sessionContent.sessionSubSys = subSys;
		}

		std::string getsetSubSys()
		{
			SHARABLE_LOCK _threadguard(rwlock);
			return m_sessionContent.sessionSubSys;
		}
				
		void setKpiInfo(std::vector<struct kpiinfo> vec)
		{
			ECLUSIVE_LOCK _threadguard(rwlock);
			m_sessionContent.sessionKpiInfo.clear();
			m_sessionContent.sessionKpiInfo.insert(m_sessionContent.sessionKpiInfo.begin(), 
				vec.begin(), vec.end());
		}

		void getKpiInfo(std::vector<struct kpiinfo> &vec)
		{
			SHARABLE_LOCK _threadguard(rwlock);
			vec.clear();
			vec.insert(vec.begin(), m_sessionContent.sessionKpiInfo.begin(),
				m_sessionContent.sessionKpiInfo.end());
		}
		
		void setBolInfo(std::vector<struct bolinfo> vec)
		{
			ECLUSIVE_LOCK _threadguard(rwlock);
			m_sessionContent.sessionBolInfo.clear();
			m_sessionContent.sessionBolInfo.insert(m_sessionContent.sessionBolInfo.begin(),
				vec.begin(), vec.end());
		}

		void getBolInfo(std::vector<struct bolinfo> &vec)
		{
			SHARABLE_LOCK _threadguard(rwlock);
			vec.clear();
			vec.insert(vec.begin(), m_sessionContent.sessionBolInfo.begin(),
				m_sessionContent.sessionBolInfo.end());
		}

		bool getBolDomain(const std::string bolName, std::string &domainName)
		{
			SHARABLE_LOCK _threadguard(rwlock);
			std::vector<struct bolinfo>::iterator it;
			for (it = m_sessionContent.sessionBolInfo.begin(); 
				it != m_sessionContent.sessionBolInfo.end();it++)
			{
				if (0 == it->bol_cloud_name.compare(bolName))
				{
					domainName = it->domainName;
					return true;
				}
			}

			return false;
		}

		void setFlexInfo(std::vector<struct flexinfo> vec)
		{
			ECLUSIVE_LOCK _threadguard(rwlock);
			m_sessionContent.sessionFlexInfo.clear();
			m_sessionContent.sessionFlexInfo.insert(m_sessionContent.sessionFlexInfo.begin(),
				vec.begin(), vec.end());
		}

		void getFlexInfo(std::vector<struct flexinfo> &vec)
		{
			SHARABLE_LOCK _threadguard(rwlock);
			vec.clear();
			vec.insert(vec.begin(), m_sessionContent.sessionFlexInfo.begin(),
				m_sessionContent.sessionFlexInfo.end());
		}

		void setDictInfo(std::vector<struct dictinfo> vec)
		{
			ECLUSIVE_LOCK _threadguard(rwlock);
			m_sessionContent.sessionDictInfo.clear();
			m_sessionContent.sessionDictInfo.insert(m_sessionContent.sessionDictInfo.begin(),
				vec.begin(), vec.end());
		}

		void getDictInfo(std::vector<struct dictinfo> &vec)
		{
			SHARABLE_LOCK _threadguard(rwlock);
			vec.clear();
			vec.insert(vec.begin(), m_sessionContent.sessionDictInfo.begin(),
				m_sessionContent.sessionDictInfo.end());
		}
	}sessionData;

	// ȫ�ֹ���zookeeper��Ϣ�Ự����
	extern sessionData g_sessionData;

	/// CloudAgent run time task name, it will be used as thread name by taskmanager
	#define BOL_CLOUDSCHEDULE_TASK_NAME  "Bol_Cloud_Schedule_Task_Loop_Thread"

	/// which will be run in a dedicated thread and managed by Bmco::TaskManager
	class CloudScheduleTaskLoop: public ControlRegionBasedTask<Bmco::Logger>
	{
	public:
		CloudScheduleTaskLoop(ControlRegionOp::Ptr& ptr):
			ControlRegionBasedTask<Bmco::Logger>(BOL_CLOUDSCHEDULE_TASK_NAME, ptr,
			                                     Bmco::Util::Application::instance().logger())
		{
		}
		virtual ~CloudScheduleTaskLoop(void)
		{
		}
		
		/// do check and initlization before task loop
		virtual bool beforeTask()
		{
			theLogger.debug("CloudScheduleTaskLoop::beforeTask()");
			if (!init())
			{
				return false;
			}
			else
			{
				return true;
			}
		}

		/// do uninitlization after task loop
		virtual bool afterTask()
		{
			theLogger.debug("CloudScheduleTaskLoop::afterTask()");

			// �˳�ʱ�����ϱ�zookeeper������
			/*std::string bolName = g_sessionData.getBolName();
			std::string bolStr = ZNODE_ZOOKEEPER_STATUS_BPCB + "/" + bolName;
			std::string chunkStr = ZNODE_ZOOKEEPER_STATUS_CHUNK + "/" + bolName;
			std::string kpiStr = ZNODE_ZOOKEEPER_STATUS_KPI + "/bol/" + bolName;

			if (CloudDatabaseHandler::instance()->nodeExist(bolStr))
			{
				if (!CloudDatabaseHandler::instance()->removeNode(bolStr))
				{
					bmco_error_f3(theLogger, "%s|%s|removeNode %s error when quit", 
						std::string("0"), std::string(__FUNCTION__), bolStr);
				}
			}

			if (CloudDatabaseHandler::instance()->nodeExist(chunkStr))
			{
				if (!CloudDatabaseHandler::instance()->removeNode(chunkStr))
				{
					bmco_error_f3(theLogger, "%s|%s|removeNode %s error when quit", 
						std::string("0"), std::string(__FUNCTION__), chunkStr);
				}
			}

			if (CloudDatabaseHandler::instance()->nodeExist(kpiStr))
			{
				if (!CloudDatabaseHandler::instance()->removeNode(kpiStr))
				{
					bmco_error_f3(theLogger, "%s|%s|removeNode %s error when quit", 
						std::string("0"), std::string(__FUNCTION__), kpiStr);
				}
			}*/

			//�˳�ʱ����BPCB
			MetaBpcbInfoOp *bpcbInfoPtr = dynamic_cast<MetaBpcbInfoOp *>(ctlOper->getObjectPtr(MetaBpcbInfoOp::getObjName()));
			if (false == bpcbInfoPtr->updateStatusByBpcbID(BOL_PROCESS_CLOUDAGENT, ST_STOP))
			{
				bmco_error_f2(theLogger, "%s|%s|update  BpcbInfoTable error when quit", std::string("0"), std::string(__FUNCTION__));
				return false;
			}
			return true;
		}

		/// perform business logic
		virtual bool handleOneUnit()
		{
			// ���zookeeper������ʱ�ڵ㣬��ʧ��Ҫ������
			// if (!checkZookeeperEphemeralZode())
			// {
			//	 return false;
			// }

			MetaBpcbInfoOp *bpcbInfoPtr = dynamic_cast<MetaBpcbInfoOp *>(ctlOper->getObjectPtr(MetaBpcbInfoOp::getObjName()));

			MetaBpcbInfo::Ptr bpcb =  new MetaBpcbInfo(0, 0, 0, 0, 0,
			                                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
			if (!bpcbInfoPtr->getBpcbInfoByBpcbID(BOL_PROCESS_CLOUDAGENT, bpcb))
			{
				bmco_error_f3(theLogger, "%s|%s|BPCB_ID[%?d]:failed to query  bpcb item",
				              std::string("0"), std::string(__FUNCTION__), BOL_PROCESS_CLOUDAGENT);
			}

			if (bpcb->m_iAction != PROCESS_ACTION_PAUSE)
			{
				if (bpcb->m_iAction == PROCESS_ACTION_ACTIVATE)
				{
					if (! bpcbInfoPtr->updateActionByBpcbID(BOL_PROCESS_CLOUDAGENT, PROCESS_ACTION_NULL))
					{
						bmco_error_f2(theLogger, "%s|%s|Failed to execute updateActionByBpcbID on MetaShmBpcbInfoTable.",
						              std::string("0"), std::string(__FUNCTION__));
					}
					if (false == bpcbInfoPtr->updateStatusByBpcbID(BOL_PROCESS_CLOUDAGENT, ST_READY))
					{
						bmco_error_f2(theLogger, "%s|%s|update  BpcbInfoTable error ", std::string("0"), std::string(__FUNCTION__));
						return false;
					}
					this->sleep(1000);
				}

				//����BPCB(status)
				if (false == bpcbInfoPtr->updateStatusByBpcbID(BOL_PROCESS_CLOUDAGENT, ST_RUNNING))
				{
					bmco_error_f2(theLogger, "%s|%s|update  BpcbInfoTable error", std::string("0"), std::string(__FUNCTION__));
					return false;
				}

				CloudDatabaseHandler::instance()->rebuildLogFile();

				// �ϱ�bpcb������Ϣ
				refreshBpcbInfo();
				// �ϱ��ڴ����Ϣ
				refreshChunkInfo();
				// �ϱ�kpiָ����Ϣ
				refreshKpiInfo();
				// doMasterTaskMission();

				// refreshLogLevel();	//��Ӧ��־�ȼ����
				if (false == bpcbInfoPtr->updateStatusByBpcbID(BOL_PROCESS_CLOUDAGENT, ST_READY))
				{
					bmco_error_f2(theLogger, "%s|%s|update  BpcbInfoTable error", std::string("0"), std::string(__FUNCTION__));
					return false;
				}
			}
			else
			{
				if (!bpcbInfoPtr->updateStatusByBpcbID(BOL_PROCESS_CLOUDAGENT, ST_HOLDING))
				{
					bmco_error_f2(theLogger, "%s|%s|update  BpcbInfoTable error ", std::string("0"), std::string(__FUNCTION__));
				}
			}
			
			this->sleep(4000);
			return true;
		}

	protected:
		//! Doing initialization.
		bool init()
		{
			//����BPCB(status and syspid)
			MetaBpcbInfoOp *bpcbInfoPtr = dynamic_cast<MetaBpcbInfoOp *>(ctlOper->getObjectPtr(MetaBpcbInfoOp::getObjName()));
			if ((false == bpcbInfoPtr->updateStatusByBpcbID(BOL_PROCESS_CLOUDAGENT, ST_READY))
			    || (false == bpcbInfoPtr->updateSysPidByBpcbID(BOL_PROCESS_CLOUDAGENT, Bmco::Process::id())))
			{
				bmco_error_f2(theLogger, "%s|%s|update  BpcbInfoTable error before init", std::string("0"), std::string(__FUNCTION__));
				return false;
			}

			std::string bpcbInfoPath = ZNODE_ZOOKEEPER_STATUS_BPCB;

			if (!CloudDatabaseHandler::instance()->nodeExist(bpcbInfoPath))
			{
				if (!CloudDatabaseHandler::instance()->createNode(bpcbInfoPath, 0))
				{
					bmco_error_f1(theLogger, "Faild to createNode %s", bpcbInfoPath);
					return false;
				}
			}

			std::string chunkInfoPath = ZNODE_ZOOKEEPER_STATUS_CHUNK;

			if (!CloudDatabaseHandler::instance()->nodeExist(chunkInfoPath))
			{
				if (!CloudDatabaseHandler::instance()->createNode(chunkInfoPath, 0))
				{
					bmco_error_f1(theLogger, "Faild to createNode MetaBpcbInfo", chunkInfoPath);
					return false;
				}
			}

			std::string kpiInfoPath = ZNODE_ZOOKEEPER_STATUS_KPI + "/bol";

			if (!CloudDatabaseHandler::instance()->nodeExist(kpiInfoPath))
			{
				if (!CloudDatabaseHandler::instance()->createNode(kpiInfoPath, 0))
				{
					bmco_error_f1(theLogger, "Faild to createNode MetaBpcbInfo", kpiInfoPath);
					return false;
				}
			}

			return true;
		}

		//! refresh #0 log level
		void refreshLogLevel()
		{
			MetaBolConfigOp *c = dynamic_cast<MetaBolConfigOp *>(ctlOper->getObjectPtr(MetaBolConfigOp::getObjName()));
			assert (NULL != c);
			MetaBolConfigInfo::Ptr cfgpayload(new MetaBolConfigInfo("logging.logger.CloudAgent.level"));
			if (c->Query(cfgpayload))
			{
				theLogger.setLevel(cfgpayload->_strval.c_str());
			}
			else
			{
				theLogger.setLevel("warning");
			}
		}

		// �ϱ�bpcbʵʱ��Ϣ
		void refreshBpcbInfo();

		// �ϱ��ڴ��ʵʱ��Ϣ
		void refreshChunkInfo();

		// �ϱ�ʵʱָ����Ϣ
		void refreshKpiInfo();

		// void doMasterTaskMission();

		// �жϵ�ǰCloudAgent�Ƿ���ΪMaster
		// bool isMasterCloudAgent();

		// �ϱ�Master��Bol��Ϣ��zookeeper
		// bool refreshMasterInfo(std::string MasterBol);

		// bool checkZookeeperEphemeralZode();
		
	private:
	};

} //namespace BM35

#endif //__CLOUDSCHEDULETASKLOOP__HH__


