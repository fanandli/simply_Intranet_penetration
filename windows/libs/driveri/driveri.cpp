#include <memory.h>
#include <vector>
#include <map>
#include "base.h"
#include "json-c/json.h"
#include "unidef.h"
#include "sockex.h"
#include "driver_inner.h"
#include "tlv.h"
#include "driveri.h"
#include "mswsock.h"

struct IpPort {//��������뵽��Զ�˵�ip+port
	struct in_addr ip;
	UINT16 port = 0;
};

struct IpPort IpPortt[65540];//������±��Ǳ���port

map<SOCKET, selfServiceConnect*>SoftFwder::mapserandpc;

//ȫ�ֱ�����ͬһ���ļ��ж��壬�����Ⱥ�˳���ʼ����ȫ�ֱ���������ǰ��ʼ��������driveri����ȫ�������ȫ�ֱ����ǿ��е�
driveri::driveri() {
	/* ת����Ϳ������̷߳��룬ʵ��ת�����completeio��ת�����߳�ʵ�� */
	tlvcbs[StarTlv::RGETDATACHNLADDR] = TlvSynReplyCb;
	tlvcbs[StarTlv::DRV_CONNECTION] = SoftFwder::proDrvConnect;
	tlvcbs[StarTlv::DRV_DATA] = SoftFwder::proDrvData;
}

/* TODO: ��ʱ���ܲ���Ҫʵ�� */
UINT16 driveri::updateFwdInfo(SOCKADDR* addr, UINT32 type, SOCKADDR* downAddr, SockEx* xtunnel, SOCKADDR_IN* upAddr) {
	return 0;
}

driveri::~driveri() {

}

/*
https://www.cnblogs.com/lidabo/p/5344899.html
get the default gateway, then get the ip address on the interface
*/
/* TODO: ��ʱ����Ҫʵ�� */
UINT32 driveri::GetGateWayAndIfIP(char* ifname, UINT32* sip) {
	return 0;
}
/* TODO: ��ʱ����Ҫʵ�� */
void driveri::getIfMac(char* ifName, char* mac) {
	
	/*mac[0] = 0xB4;
	mac[1] = 0x2E;
	mac[2] = 0x99;
	mac[3] = 0xE2;
	mac[4] = 0x7B;
	mac[5] = 0x46;
    */

}

/* TODO: ��ʱ����Ҫʵ�� */
UINT32 driveri::getIfIP(char* ifname) {
	return 0;
}

/* TODO: ����ipv6ʱʵ�� */
//ֻ��Ϊ�˹���һ����Ԫ�飬����Ӫ��
//src�ǿͻ��˵�ip+port
//dst���Լ���ip+port
extern LPFN_CONNECTEX lpfnConnectEx;

//�˺�����Ҫ����������Ӫ�̷�һ��syn��Ϊ�˾�������Ӫ�������ǵ���Ԫ�飨�ͻ���+Ŀ����������Ԫ�飩
void driveri::sendtcp6(sockaddr_in6* src, sockaddr_in6* dst) {
	SockExTCP* s = new SockExTCP();
	int bindres = ::bind(s->sock, (const sockaddr*)src, sizeof(sockaddr_in6));

	dst->sin6_family = AF_INET6;
	SockExOL* ov = new SockExOL(s, SockExOL::CONNECT);
	lpfnConnectEx(s->sock, (sockaddr*)dst, sizeof(SOCKADDR_IN), NULL, 0, 0, &ov->overlapped);
}

/* TODO: ����intraport��transitport��ȡ��Ӧ�Ĺ�����ַ
port:transitport��intraport
ctrlchnl:����ͨ��
intraddr:��Σ�intraaddr
publicaddr:���Σ�������ַ
*/
//shared_ptr<SockExTCP> dataptr = NULL;
SockExTCP* dataptr = NULL;

int driveri::getPublicAddrr(UINT16 port, SockEx* ctrlchnl, SOCKADDR_IN* intraaddr, SOCKADDR_IN* publicaddr, NBSDRV_FWDMODE mode) {
	//����port�����Ƿ��Ѿ����빫����ַ
	//����Ѿ����룬��ֱ�ӷ���
	//���û�����룬���Ƿ�������ͨ�����������������ͨ������ӿ���ͨ����������ͨ��������Ϣ
	//ͨ������ͨ�����͹�����ַ������Ϣ
	//�洢port/������ַ/������ַ��Ӧ��ϵ
	StarTlv request(StarTlv::GETDATACHNLADDR);//��ʼ��һ��tlv
	StarTlv requesttwo(StarTlv::REQUESTPUBLICADDR);
	sockthread::pack_synheader(&request);

	SOCKADDR_IN ctrladdr;
	socklen_t len = sizeof(SOCKADDR_IN);

	if (IpPortt[port].port) {//ֱ�ӷ��ع�����ַ
		publicaddr->sin_addr = IpPortt[port].ip;
		publicaddr->sin_port = IpPortt[port].port;
	}
	else {//����������빫��ip
		if (dataptr) {//�������ͨ�����ڣ��ʹ�����ͨ������
			request.pack_atom(StarTlv::REQUESTPUBLICADDR_INTRAADDR, sizeof(SOCKADDR_IN), (char*)intraaddr);
			request.pack_atom(StarTlv::REQUESTPUBLICADDR_CTRLIP, sizeof(UINT32), (char*)&ctrladdr.sin_addr.s_addr);
			send(dataptr->sock, requesttwo.get_final(), requesttwo.total, MSG_NOSIGNAL);//ͨ������ͨ����������
		}
		else {//����ͨ�������ڣ����߿���ͨ������һ������ͨ����Ȼ���ٴ�����ͨ����������
			//shared_ptr<SockExTCP> SET(new SockExTCP());
			SockExTCP* SET = new SockExTCP();//
		   //request.pack_atom(StarTlv::REQUESTPUBLICADDR_CTRLIP, sizeof(UINT32), (char*)&ctrladdr.sin_addr.s_addr);//��ctrladdr�ӵ�request��
			send(ctrlchnl->sock, request.get_final(), request.total, MSG_NOSIGNAL);//�������
			char* msg = sockthread::wait(4);
			if (msg == nullptr) {
				DBG("rrequest nil");
				return -1;
			}
			StarTlv rrtlv(msg);
			SOCKADDR_IN* addr = (SOCKADDR_IN*)rrtlv.get_tlv(StarTlv::RGETDATACHNLADDR_ADDR);//get_tlv��������������ģ�
			addr->sin_family = AF_INET;
			sockthread::pack_synheader(&requesttwo);
			requesttwo.pack_atom(StarTlv::REQUESTPUBLICADDR_INTRAADDR, sizeof(SOCKADDR_IN), (char*)intraaddr);
			requesttwo.pack_atom(StarTlv::REQUESTPUBLICADDR_CTRLIP, sizeof(UINT32), (char*)&ctrladdr.sin_addr.s_addr);
			NBS_CREATESOCKADDR(localAddr,0, 0);
			bind(SET->sock, (SOCKADDR*)&localAddr, sizeof(SOCKADDR_IN));//
			SET->ConnectEx((struct sockaddr*)addr, requesttwo.get_final(), requesttwo.total);//��������ͨ����
			//cout << addr;
			char* msgtwo = sockthread::wait(4);
			if (msgtwo == nullptr) {
				DBG("rrequest nil");
				return -1;
			}
			StarTlv rrrtlv(msgtwo);

			SOCKADDR_IN* addrtwo = (SOCKADDR_IN*)rrrtlv.get_tlv(StarTlv::RREQUESTPUBLICADDR_INTERADDR);
			
			publicaddr->sin_addr = addrtwo->sin_addr;
			publicaddr->sin_port = addrtwo->sin_port;

			//�洢ip + port
			//port�Ǳ��ص�
			//sin_port��Զ�˵�
			//port = addrtwo->sin_port;
			//IpPortt[i].ip = addrtwo->sin_addr;
			IpPortt[port].ip = addrtwo->sin_addr;
			IpPortt[port].port = addrtwo->sin_port;
			dataptr = SET;
		}
	}
	return 0;
}

class TransChnl;

class ClitoTargethostIp6 :public SockExTCP {
public:
	//ChnlTCP(SOCKADDR_IN* peeraddr, char* token);
	TransChnl* ToTransChnlBuf;
	ClitoTargethostIp6(TransChnl* a) {
		ToTransChnlBuf = a;
	}
	~ClitoTargethostIp6();

	virtual int onConnect(bool bConnect);
	//bool notUsing();
	virtual int onRcv(int len);
	//static void FwdtoDataChnl(SockEx* rcvsock, SOCKET datachnl, char peerFwdId, char fwdidlen, char* rcvbuf);
	UCHAR databuf[databufFERLEN];//�����3389��������
	//int RcvEx(UCHAR* rcvbuf, int n);
	// static map<SOCKET, SOCKET>mapTransAndSer;
};



struct TransInfo_s {
	SockExTCP* lsn;
	UINT32 ip;
	UINT16 port;
}TransInfo[65535];

//map<SOCKET, SOCKET>mapTransAndSer;
//SockExTCP* cltChnlPtr = NULL;
class TransChnl:public SockExTCP {//��������ͻ��˷����������ӣ���ͻ��˷����ݵ����ӣ�
public:
	 //map<SOCKET,SOCKET>mapTransAndSer;
	 UCHAR databuf[databufFERLEN];//����ǿͻ���Ҫ����3389������
	//SockEx* Transsock = NULL;
	 ClitoTargethostIp6* clitoPtr = NULL;

	SockExTCP* newAcceptSock(SockExTCP* srv) {//?
		TransChnl* esock = new TransChnl();
		cout << "TransChnl sock: " << esock->sock << endl;
		esock->srv = srv;
		return esock;
	}

	~TransChnl() {
		DBG("~TransChnl, sock:%d", sock);
		if (clitoPtr) {
			clitoPtr->ToTransChnlBuf = nullptr;
			delete clitoPtr;
		}
	}

	int onConnect(bool bConn) {//�������
		//����ͻ������ҽ���������
		//ͨ��transInfo��ȡ���صĶ˿ں�IP����������
		//����socket֮���ӳ���ϵ
		//�����onConnect��Ҫ����3389��������
		//setsockopt(sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&srv->sock, sizeof(srv->sock)); //MUST set this option, orelse cannot use SHUTDOWN, getpeername ...
		UINT16 port = srv->getPort(AF_INET6);//�����ȡ������transitport
		//shared_ptr<ClitoTargethostIp6> transChnlPtr (new ClitoTargethostIp6(this));
		ClitoTargethostIp6* transChnlPtr = NULL;
		transChnlPtr = new ClitoTargethostIp6(this);//�����this��ָ��TransChnl��ʵ����ָ��
		cout << "TransChnl onConnect transChnlPtr " << transChnlPtr->sock << endl;
		//Transsock = transChnlPtr;
		 
		sockaddr_in sockLocal;
		sockLocal.sin_port = TransInfo[port].port;//��ȡ��Ҫ���ʵķ����ip+port
		sockLocal.sin_addr.s_addr = TransInfo[port].ip;  //�������ת����
		cout << "-----" << port << " " << TransInfo[port].ip << " " << TransInfo[port].port << endl;
		sockLocal.sin_family = AF_INET;

		NBS_CREATESOCKADDR(localAddr, 0,0);
		bind(transChnlPtr->sock, (SOCKADDR*)&localAddr, sizeof(SOCKADDR_IN));

		transChnlPtr->ConnectEx((struct sockaddr*)&sockLocal, NULL, 0);

		//Ȼ�����sock��port������Ӧ��ϵ --->�����Ӧ��ϵҪ����˭��
		//mapserandpc[SET->sock] = SET;
		//mapTransAndSer[transChnlPtr->sock] = sock;//�±�����3389���ӵ�sock��sock�ǿͻ��˷���sock
		//RcvEx(databuf);
		clitoPtr = transChnlPtr;
		cout << "TransChnl sock: " << sock << " " << this << endl;

		return 0;
	}
	int onRcv(int n) {//����Ǵ���ͻ��˷���������,����Ŀ�������е�Ŀ�Ķ˿�
		//��������sockex
		//ת��
		//send(sock, payload, tlvs.get_len(payload), MSG_NOSIGNAL);//���͸�����port
		 // sizeof(_tlv)*3 + 1 subtlv end with 0 + fwdidlen
		//_tlv* msg = (_tlv*)databuf;
		//msg->type = StarTlv::DRV_DATA;
		//UINT16 port = getPort();
		cout << "TransChnl onRcv sock: " << sock <<" "<< this << endl;
		cout << "TransChnl::onRcv send : " << clitoPtr << endl;
		int sendreturn = send(clitoPtr->sock, (const char*)databuf, n, MSG_NOSIGNAL);//��databufsend��ȥ
		
		wprintf(L"send failed with error: %d\n", WSAGetLastError());
		if (sendreturn == -1) {
			exit(0);
		}
		RcvEx(databuf, sizeof(databuf), 0);//�������߲���ϵͳ���յ�������д��databuf��ȥ
		return 0;
	}
};

ClitoTargethostIp6::~ClitoTargethostIp6() {
	if (ToTransChnlBuf) {
		ToTransChnlBuf->clitoPtr = nullptr;
		delete ToTransChnlBuf;
	}
}

int ClitoTargethostIp6::onRcv(int n) {//��ͻ��˷�������
	//cout << "ClitoTargethostIp6::onRcv: " << this << endl;
	send(ToTransChnlBuf->sock, (const char*)databuf, n, MSG_NOSIGNAL);
	RcvEx(databuf, sizeof(databuf), 0);
	return 0;
}

int ClitoTargethostIp6::onConnect(bool bConn) {
	cout << "ClitoTargethostIp6::onConnect: " << this << endl;
	//cout << bConn << endl;
	RcvEx(databuf,sizeof(databuf), 0);//��3389�������ĸ���ϵͳд������
	ToTransChnlBuf->RcvEx(ToTransChnlBuf->databuf,sizeof(ToTransChnlBuf->databuf), 0);
	//RcvEx();
	return 0;
}


//int ClitoTargethostIp6::onRcv(int n){
	//int sendreturn = send(mapTransAndSer[sock], (const char*)databuf, n, MSG_NOSIGNAL);

//}

//����transitport
UINT16 driveri::getAvailablePort(NBSDRV_PORTTYPE type, UINT32 peerip, UINT16 peerport, SockEx* tnl) {
	SOCKET s = socket(AF_INET6, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
	sockaddr_in6 addr6;
	memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;

	TransChnl* acceptsock = new TransChnl();//������������socket���ͻ�������socket]
	cout << "driveri::getAvailablePort accept sock:" << acceptsock->sock << endl;
	acceptsock->srv = new SockExTCP(s, (SOCKADDR*)&addr6, sizeof(addr6), true);//����һ��listen ��socket
	//��������bind�ͻ������һ��port����Ϊtransitport
	SockExTCP::AcceptEx(acceptsock);//����
	UINT16 port = acceptsock->srv->getPort(AF_INET6);//�����õ���transitport

	if (type == DRIVERI_SVCTRANSIT) {
		TransInfo[port].ip = peerip;//��������service��port
		TransInfo[port].port = peerport;//����ṹ�����port��Ҫ���ʵ�port���±�port�����ǵ�transitport
		TransInfo[port].lsn = acceptsock->srv;//��ָ��socket��ָ��
	}

	DBG("transitport:%d", ntohs(port));
	return port;
}

/* TODO: ʵ��һЩת������
�յ�Զ�˷����������ӽ�����Ϣ
�յ�Զ�˷����������ж���Ϣ
�յ�����&Զ�˵����ݱ�����Ϣ
*/

//�������յ��ͻ��˵���Ϣ�󣬷���������Ŀ���������͡����ӽ�����Ϣ��
//�����Լ�Ŀ������Ҫ���������Ϣ�������ó��ͻ����������������socket��id
//Ȼ��ȥ����һ�����ӣ����������磩3389����˿ڣ�Ȼ��������ӵ�socketid�������Ǹ�socketid
//������Ӧ��ϵ��Ȼ���ٽ������Ӧ��ϵ�����ظ���������

//����������յ��˿ͻ��˷��͵ġ��ն�������Ϣ���Ļ�
//��Ŀ������������Ϣ��Ŀ������ֹͣ��Ӧ������������

//�յ����ݱ��ģ�ͨ����ѯ�����Ķ�Ӧ��ϵ����ת������Ӧ��socket������˿ڣ�

int SoftFwder::proDrvConnect(SockEx* esock, StarTlv& tlvs) {
	//���յ�����Ϣ
	char type = *tlvs.get_tlv(StarTlv::DRV_CONNECTION_TYPE);//

	//char* savetlv = tlvs.get_tlv(StarTlv::DRV_CONNECTION_LOCALFWDID);//��һ�·�����tlv��Ϣ,�������ֻ��1��ʱ�����
	//Զ�˵�forwdid

	//SockExTCP* SET = 0;

	//��������������
	//SockExTCP* SET = new selfserviceconnect();
	
	if (type == 1) {//�½�һ��socket
		char* savetlv = tlvs.get_tlv(StarTlv::DRV_CONNECTION_LOCALFWDID);//����Ƿ����������ҵģ�Զ�ˣ�forwdid
		int tlvlen = tlvs.get_len(savetlv);
		
		selfServiceConnect* SET = new selfServiceConnect();//SET���б������ӵ�sockid
		SOCKADDR_IN* addr = (SOCKADDR_IN*)tlvs.get_tlv(StarTlv::DRV_CONNECTION_ADDR);//get_tlv��������������ģ�
		addr->sin_family = AF_INET;
		NBS_CREATESOCKADDR(localAddr, 0, 0);
		bind(SET->sock, (SOCKADDR*)&localAddr, sizeof(SOCKADDR_IN));
		SET->ConnectEx((struct sockaddr*)addr, NULL, NULL);//������port����

		SET->fwdidlen = tlvlen;
		mapserandpc[SET->sock] = SET;
		memcpy(SET->peerFwdId, savetlv, tlvlen);//peerFwdId����Զ�˵�socketid
		

	}else if(type == 0) {//0,˵������˵����Ӷϵ��ˣ����Ǳ��ص���Ӧ��socketҲҪ�ϵ�
		cout << "-----prodrvconnect else if type == 0-----" << endl;
		//SET->ConnectEx((struct sockaddr*)addr, NULL, NULL);//��������ͨ����
		//�洢socket id�Ķ�Ӧ��ϵ
		//SoftFwder::mapserandpc[SET->sock] = savetlv;
		//mapserandpc[SET->sock] = savetlv;
		//delete SET;
		int sock = *(SOCKET*)tlvs.get_tlv(StarTlv::DRV_CONNECTION_RMTFWDID);
		//char* savetlv = tlvs.get_tlv(StarTlv::DRV_CONNECTION_RMTFWDID);//
		//closesocket(localfwdid->s); //������sock�յ�len=0��������ɾ��sockex���������ɾ��sockex��Ϊ�˱�����߳�ͬʱɾ�����ܵ��µ��쳣
		delete mapserandpc[sock];//m
	}
	//���յ�����Ϣ����socketid�Ķ�Ӧ��ϵ��Ȼ���͸���������ͨ������ͨ����->��onconnect�߼���
	return 0;
}
int selfServiceConnect::RcvEx(UCHAR* rcvbuf) {
	DWORD flags = 0; //must initial equal 0, orelse, no message rcv by workthread
	SockExOL* ov = new SockExOL(this, SockExOL::RCV, (char*)rcvbuf+ 12 + 1 + fwdidlen, databufFERLEN - 12 - 1 - fwdidlen);
	int rtn = WSARecv(sock, &ov->wsabuffer, 1, NULL, &flags, &ov->overlapped, NULL);
	//DBG("selfserviceconnect recv: sock: %d, %d, %d", sock, rtn, WSAGetLastError());
	return 0;
}

int selfServiceConnect::onConnect(bool bConnect) {
	int type = 0;
	if (!bConnect) {//��������3389�������Ӳ��ɹ�
		//this->SockExTCP::onConnect(bConnect);
		StarTlv request(StarTlv::GETDATACHNLADDR);
		request.pack_atom(StarTlv::DRV_CONNECTION_TYPE, sizeof(int), (char*)&type);
		request.pack_atom(StarTlv::DRV_CONNECTION_RMTFWDID, fwdidlen, peerFwdId);
		send(dataptr->sock,request.get_final(),request.total, MSG_NOSIGNAL);
		delete this;
		
	}else {//���ӳɹ�
		StarTlv request(StarTlv::DRV_CONNECTION);
		int two = 2;
		request.pack_atom(StarTlv::DRV_CONNECTION_TYPE, sizeof(two),(char*)&two);
		request.pack_atom(StarTlv::DRV_CONNECTION_RMTFWDID, fwdidlen, peerFwdId);
		request.pack_atom(StarTlv::DRV_CONNECTION_LOCALFWDID, sizeof(sock), (char*)&sock);
		send(dataptr->sock, request.get_final(), request.total, MSG_NOSIGNAL);
	}

	RcvEx(databuf);//�ձ���3389���͹���������
	return 0;
}


int SoftFwder::proDrvData(SockEx* esock, StarTlv& tlvs) {//������������������ͨ���յ�����Ϣ
	//_fwdid_* localfwdid = (_fwdid_*)tlvs.get_tlv(StarTlv::DRV_DATA_FWDID);//�õ�id
	int sock = *(SOCKET*)tlvs.get_tlv(StarTlv::DRV_DATA_FWDID);
	char* payload = tlvs.get_tlv(StarTlv::DRV_DATA_PAYLOAD);//�õ�data
	send(sock, payload, tlvs.get_len(payload), MSG_NOSIGNAL);//���͸�����port

	return 0;
}

int selfServiceConnect::onRcv(int len) {//����port������
	//StarTlv request(StarTlv::DRV_DATA);
	//��tlv����ȥ
	//��payload����ȥ
	//���͸���Ӧ�Ķ˿ڣ��������ȡ�������recv�������ջ�ȡ��
	//request.pack_atom(StarTlv::DRV_DATA_FWDID, sizeof(peerFwdId), (char*)&(peerFwdId));//��tlv
	//request.pack_atom(StarTlv::DRV_DATA_FWDID, sizeof(mapserandpc), (char*)&(mapserandpc));//��tlv
	//request.pack_atom(StarTlv::DRV_DATA_PAYLOAD, n, (char*)databuf);//��payload,payload��rcvbuf��

	int headlen = 12 + 1 + fwdidlen; // sizeof(_tlv)*3 + 1 subtlv end with 0 + fwdidlen
	_tlv* msg = (_tlv*)databuf;
	msg->type = StarTlv::DRV_DATA;

	//subtlv fwdid
	_tlv* subtlv = msg + 1;
	subtlv->type = StarTlv::DRV_DATA_FWDID;
	subtlv->len = htons(fwdidlen + 1);
	subtlv += 1;
	memcpy(subtlv, peerFwdId, fwdidlen);
	
	//subtlv payload
	subtlv = (_tlv*)((char*)subtlv + fwdidlen + 1);
	subtlv->type = StarTlv::DRV_DATA_PAYLOAD;
	subtlv->len = htons(len + 1);

	msg->len = htons(headlen - sizeof(_tlv) + len);//����msg len

	int sendreturn = send(dataptr->sock, (char*)msg, len+headlen, MSG_NOSIGNAL);//��������ͨ��
	//cout << "selfServiceConnect::onRcv send return: " << sendreturn << endl;
	RcvEx(databuf);//
	//procTlvMsg(rcvbuf, msglen);
	return 0;

	//update
	//ֱ�ӽ�port�����������ݣ�д��databuf�У��ճ�һ���ڴ�ͷ��Ȼ��
	//
}

