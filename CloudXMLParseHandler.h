#include "Bmco/XML/XMLString.h"
#include "Bmco/DOM/NodeFilter.h"
#include "Bmco/DOM/AutoPtr.h"
#include "Bmco/DOM/NodeIterator.h"
#include "Bmco/DOM/NamedNodeMap.h"
#include "Bmco/Util/AbstractConfiguration.h"
#include "Bmco/Util/XMLConfiguration.h"
#include "Bmco/DOM/DOMParser.h"
#include "Bmco/DOM/Node.h"
#include "Bmco/DOM/Document.h"
#include "Bmco/AutoPtr.h"
#include "Bmco/Util/Application.h"

namespace BM35{

class CloudXMLParse
{
// ����ZK���Ӵ�����
enum E_Step
{
	// ��λ��������key,��"db_id"
	E_FINDZKTYPEKET = 0,
	// ��λ��������ֵ,��"zookeeper"
	E_FINDZKTYPEVALUE = 1,
	// ��λ���Ӵ�����key,��"connect_str"
	E_FINDCONNECTKEY = 2,
	// ��λ���Ӵ�����ֵ,��"1.1.1.1:1111"
	E_FINDCONNECTVALUE = 3
};

public:
	CloudXMLParse()
		: theLogger(Bmco::Util::Application::instance().logger())
	{
	}
	~CloudXMLParse(){}
	
	bool loadParamFromXMLFile(std::string fileName);
	std::string getZKConncetStr();
	std::string getDecodeConnectStr();
private:
	Bmco::Logger& theLogger;
	std::string m_ZKConnectStr;
};
}

