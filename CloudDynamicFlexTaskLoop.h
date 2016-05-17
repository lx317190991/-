#ifndef __CLOUDDYNAMICFLEXTASKLOOP__HH__
#define __CLOUDDYNAMICFLEXTASKLOOP__HH__
#include <sstream>
#include "CloudKafkaMessageHandler.h"
#include "Bmco/Process.h"
#include "Bmco/Util/Application.h"
#include "Bmco/Data/SessionFactory.h"
#include "Bmco/Data/Session.h"
#include "Bmco/Data/RecordSet.h"
#include "Bmco/Data/Column.h"
#include "Bmco/Data/MySQL/Connector.h"
#include "IniFileConfigurationNew.h"
#include "Bmco/Util/IniFileConfiguration.h"
#include "Bmco/NumberFormatter.h"
#include "BolCommon/ControlRegionOp.h"
#include "BolCommon/ControlRegionBasedTask.h"
#include "BolCommon/MetaKPIInfoOp.h"
#include <boost/algorithm/string.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include "CloudConfigurationUpdater.h"
#include "CloudDatabaseHandler.h"
#include "CloudScheduleTaskLoop.h"
#include "CloudCmdTaskLoop.h"

using Bmco::StringTokenizer;
using namespace Bmco::Data;

namespace BM35
{
	const std::string TABLE_MQ_RELA = "c_info_mq_rela";
	const std::string TABLE_KPI_INFO = "c_info_kpi";

	typedef struct _relaInfo
	{
		std::string flowId;
		std::string processName;
		std::string bolName;
		std::string instanceId;
		std::string inTopicName;
		std::string outTopicName;
		std::string inPartitionName;
		std::string outPartitionName;
		std::string inMqName;
		std::string outMqName;
		std::string subsys;
		std::string domain;
	}relaInfo;

	typedef struct _bolInstance
	{
		std::string bolName;
		Bmco::UInt16 instanceId;
		bool operator < (const struct _bolInstance &r) const
		{
			return (instanceId < r.instanceId);
		}
	}bolInstance;

	enum directionMark
	{
		DM_Before = 0,
		DM_After = 1
	};

	typedef struct mqCalcOffsetDataStruct
	{
		double upStreamOffset;
		double downStreamOffset;
		mqCalcOffsetDataStruct()
		{
			upStreamOffset = 0.0;
			downStreamOffset = 0.0;
		}
	}mqCalcOffsetData;

	/// CloudAgent run time task name, it will be used as thread name by taskmanager
	#define BOL_CLOUDDYNAMICFLEX_TASK_NAME  "Bol_Cloud_DynamicFlex_Task_Loop_Thread"

	/// This class collect information and send them to Mysql.
	/// which will be run in a dedicated thread and managed by Bmco::TaskManager
	class CloudDynamicFlexTaskLoop: public ControlRegionBasedTask<Bmco::Logger>
	{
	public:
		CloudDynamicFlexTaskLoop(ControlRegionOp::Ptr& ptr, std::string bolName):
			ControlRegionBasedTask<Bmco::Logger>(BOL_CLOUDDYNAMICFLEX_TASK_NAME, ptr,
			                                     Bmco::Util::Application::instance().logger())
		{
			g_sessionData.setBolName(bolName);
			m_relaBuildVec.clear();
		}
		virtual ~CloudDynamicFlexTaskLoop(void)
		{
			m_relaBuildVec.clear();
			m_DefPtr = NULL;
		}
		/// do check and initlization before task loop
		virtual bool beforeTask()
		{
			bmco_debug_f2(theLogger, "%s|%s|CloudDynamicFlexTaskLoop::beforeTask", std::string("0"),std::string(__FUNCTION__));
			if (!init())
			{
				return false;
			}
			else
			{
				bmco_debug_f2(theLogger, "%s|%s|CloudDynamicFlexTaskLoop::recordBOLInfo", std::string("0"),std::string(__FUNCTION__));
				recordBOLInfo();
				bmco_debug_f2(theLogger, "%s|%s|CloudDynamicFlexTaskLoop::recordFlexInfo", std::string("0"),std::string(__FUNCTION__));
				recordFlexInfo();
				bmco_debug_f2(theLogger, "%s|%s|CloudDynamicFlexTaskLoop::recordParentKPIPath", std::string("0"),std::string(__FUNCTION__));
				recordParentKPIPath();
				bmco_debug_f2(theLogger, "%s|%s|CloudDynamicFlexTaskLoop::recordDictInfo", std::string("0"),std::string(__FUNCTION__));
				recordDictInfo();
				bmco_debug_f2(theLogger, "%s|%s|CloudDynamicFlexTaskLoop::beforeTask", std::string("0"),std::string(__FUNCTION__));
				return true;
			}
		}


		// do uninitlization after task loop
		virtual bool afterTask()
		{
			theLogger.debug("CloudDynamicFlexTaskLoop::afterTask()");
			return true;
		}

		// perform business logic
		virtual bool handleOneUnit()
		{
			// bmco_information_f2(theLogger, "%s|%s|CloudDynamicFlexTaskLoop::handleOneUnit", std::string("0"),std::string(__FUNCTION__));
			bool nodeChanged = false;
			bool bolChanged = false;
		    if (!CloudDatabaseHandler::instance()->getNodeChangedFlag(ZNODE_ZOOKEEPER_CLUSTER+"/node", nodeChanged)
				|| !CloudDatabaseHandler::instance()->getNodeChangedFlag(ZNODE_ZOOKEEPER_CLUSTER+"/bolcfg", bolChanged))
	    	{
	    		bmco_error_f2(theLogger, "%s|%s|node or bolcfg getNodeChangedFlag failed!", 
					std::string("0"),std::string(__FUNCTION__));
				return false;
	    	}

			if (nodeChanged || bolChanged)
				recordBOLInfo();

			// Master�Ĺ���
			if (!isMasterCloudAgent())
			{
				this->sleep(4000);
				return true;
			}

			bool isChanged = false;
		    CloudDatabaseHandler::instance()->getNodeChangedFlag(ZNODE_ZOOKEEPER_FlEX, isChanged);

			if (isChanged)
				recordFlexInfo();

		    // CloudDatabaseHandler::instance()->getNodeChangedFlag(ZNODE_ZOOKEEPER_STATUS_KPI, isChanged);

			// if (isChanged)
			// recordParentKPIPath();
			
		    CloudDatabaseHandler::instance()->getNodeChangedFlag(ZNODE_ZOOKEEPER_DICT, isChanged);

			if (isChanged)
				recordDictInfo();

			recordParentKPIPath();

			// test
			if (m_testControl)
			{
				doRuleCheckDynamicFlex();
			}
			
			this->sleep(10000);
			return true;
		}

	protected:

		//! Doing initialization.
		bool init()
		{
			m_needExtendFlex = false;
			m_needShrinkFlex = false;
			m_testControl = true;
			m_topicInfoMap.clear();
			m_relaInfoVec.clear();
			m_DefPtr = dynamic_cast<MetaMQDefOp*>(ctlOper->getObjectPtr(MetaMQDefOp::getObjName()));
			return true;
		}

		// ��¼����bol����
		// void recordHostBaseInfo();

		// ��¼kpi·���µ�ÿ��bol����Ϣ
		void recordParentKPIPath();

		// ��¼��bol���ֵ�bol��Ϣ
		void recordKPIInfo(std::string bolName, std::vector<struct kpiinfo> &tmpVec);

		// ��¼bol�����Ϣ
		void recordBOLInfo();

		// ��¼zookeeper�������Ϣ
		void recordFlexInfo();

		// void recordMQDefInfo();

		void recordDictInfo();

		bool recordTopicInfo(std::string currTopicName, 
			std::string &domainName);

		bool recordRelaInfo();

		// �жϵ�ǰ��CloudAgent�Ƿ�ִ��Master����
		bool isMasterCloudAgent();

		// �ϱ�Master����Ϣ
		bool refreshMasterInfo(std::string MasterBol);

		// �жϱ�������˸�bol�����Ƿ�������bol����
		bool isTheOnlyBolForTheTopic(std::string topicName,
			std::string bolName);

		// ��̬�������
		void doRuleCheckDynamicFlex();

		// ��n+1�Ĳ����жϽӿ�
		void doExtendOperationOrNot(std::string topicName, std::string code, 
					std::string interval, std::string value, 
					std::map<std::string, Bmco::Int64> topicMessageNumMap);

		// ��n-1�Ĳ����жϽӿ�
		void doShrinkOperationOrNot(std::string topicName, std::string code, 
			std::string interval, std::string value, 
			std::map<std::string, Bmco::Int64> topicMessageNumMap);

		// ѡ��һ��bolΪ��չ����������
		bool chooseSuitAbleBolForLaunch(std::string domainName, std::string &launchBol);

		// ִ��n+1����
		bool extendPartitionAndBolFlow(std::string topicName);

		// ��ȡ�״�������bol��Ӧ���̵�ʵ����
		bool getFirstStartInstanceId(std::string processName, 
				Bmco::UInt32 &instanceId);

		// ����bol�Ľ���ʵ��
		bool launchInstanceReq(std::string bolName,std::string processName);

		// �޸�topic�����
		bool modifyMQTopicTable();

		// �޸���Ϣ���й�ϵ��
		bool modifyMQRelaTable();

		// ʵʱ����topic�ķ�����
		bool setPartitionCount(std::string topicName, 
				const Bmco::UInt32 &partitionNum);

		// ʵʱ��ȡtopic�ķ�����
		bool getPartitionCount(std::string topicName, 
				Bmco::UInt32 &partitionNum);

		// ��������bol֮��Ķ��й�ϵ
		bool buildRelationAfterNewBol(std::string topicName);

		// ��������bol֮ǰ�Ķ��й�ϵ
		bool buildRelationBeforeNewBol(std::string topicName);

		// ִ��n-1����
		bool shrinkPartitionAndBolFlow(std::string topicName);

		// ѡ��һ���������ٵ�ʵ��
		bool chooseInstanceBuildRela(std::string topicName, 
			bolInstance &ansBolInstance, enum directionMark mark);

		// ѡ��һ�����ʵ�bolͣ��
		bool chooseSuitAbleBolForStop(std::string topicName, 
			std::string &ShrinkBol);

		// �����ƹ���Χ��bol״̬
		bool setRemoteBolStatus(std::string bolName, BolStatus status);

		// �����õļ��ʱ�����Ƿ����е�HLA���̶��Ѿ�ͣ��
		bool hasStoppedALLHLAProcessTimeOut(std::string ShrinkBol);

		// ���HLA�����Ƿ��Ѿ�ͣ��
		bool defineHLAProcessShutdownAlready(std::string bolName);

		// ����Ϣ���й�ϵָ����seqNo��Ϊ��Ч
		bool setBolMessageRelaInvalid(std::vector<Bmco::UInt32> seqNoVec);

		// ����n-1�����������µĹ��ص�
		bool setRelaWithConsumeBolInstance(std::string topicName, 
			std::string ShrinkBol);

		// ����bol�ڵ�
		bool launchBolNode(const std::string bolName);

		// �ռ�topic����Ӧ�Ķѻ���Ϣ��
		bool accumulateMessageNum(const Bmco::Timespan &intervalTime,
			std::map<std::string, Bmco::Int64> &TopicAccumulateNumVec,
			std::map<std::string, mqCalcOffsetData> &TopicUpDownOffsetVec);

		bool getMessageAccumuNumByPatition(std::string inTopicName,
			std::string inPartitionName, std::string inMqName,
			std::string outTopicName, std::string outPartitionName,
			std::string outMqName, Bmco::Int64 &accumulateNum, 
			mqCalcOffsetData &result);
		
		// 崻������߼�����
		void doBrokenDownBolOperation();

		// ����Ƿ����崻���bol
		bool checkIfExistBolBrokenDown(std::string &brokenBolName);

		void refreshKPIInfoByMaster(std::map<std::string, Bmco::Int64> topicMessageNumMap, 
			std::map<std::string, mqCalcOffsetData> TopicUpDownOffsetVec);

		// fullname : topic#partition
		// intervalTime : ʱ����
		// result : ���λ�ȡ�Ķ�дƫ�����Ͷѻ���
		// calcResult : �����Ķ�д�ٶȺͶѻ���
		void calcRWSpeed(std::string fullname, 
		    const Bmco::Timespan &intervalTime, 
			const mqCalcOffsetData &result, mqCalcOffsetData &calcResult);

		void updateLastAccumulateNumMap();

		void setLastTimeStamp();

		void getIntervalTime(Bmco::Timespan& timeCost);

	private:
		// CloudDatabaseHandler zkHandle;
		std::string m_brokerlist;
		std::string m_zookeeperlist;
		// SessionContent m_sessionContent;
		std::vector<relaInfo> m_relaBuildVec;
		bool m_needExtendFlex;  // �Ƿ���Ҫn+1��ʶ
		bool m_needShrinkFlex;  // �Ƿ���Ҫn-1��ʶ
		Bmco::Timestamp m_exLatestMonitorTime; // n+1�������ʱ��
		Bmco::Timestamp m_shLatestMonitorTime; // n-1�������ʱ��
		bool m_testControl;
		std::map<std::string, Bmco::UInt32> m_topicInfoMap;
		std::vector<MetaMQRela> m_relaInfoVec;
		MetaMQDefOp *m_DefPtr;

		// ���λ�ȡ�Ķ�дƫ�����Ͷѻ���Ϣ��
		std::map<std::string, mqCalcOffsetData> m_CurrAccumulateNumMap;
		// �ϴλ�ȡ�Ķ�дƫ�����Ͷѻ���Ϣ��
		// ÿ���ϱ������Ҫ����
		std::map<std::string, mqCalcOffsetData> m_LastAccumulateNumMap;
		// �ϴλ�ȡ��ʱ��
		Bmco::Timestamp _lastTimeStamp;
	};

} //namespace BM35

#endif //__CLOUDDynamicFlexTASKLOOP__HH__


