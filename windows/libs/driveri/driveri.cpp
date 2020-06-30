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

struct IpPort {//��������뵽��Զ�˵�ip+port
	struct in_addr ip;
	UINT16 port = 0;
};

struct IpPort IpPortt[65540];//������±��Ǳ���port
UCHAR globalbuf[GLOBALBUFFERLEN];

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
void driveri::sendtcp6(sockaddr_in6* src, sockaddr_in6* dst) {

	SOCKET sock = socket(AF_INET6, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	sockaddr_in6 local;
	memset(&local, 0, sizeof(local));

	local.sin6_family = AF_INET6;
	memcpy(&local.sin6_addr, &src->sin6_addr, sizeof(in6_addr));

	local.sin6_port = _DRIVERI_SENDTCP_PORT;

	int rtn = ::bind(sock, (const sockaddr_in6*)&local, sizeof(local));

	dst->sin6_family = AF_INET6;
	rtn = connect(sock, NULL, NULL, MSG_NOSIGNAL, (sockaddr*)dst, sizeof(sockaddr_in6));

}

/* TODO: ����intraport��transitport��ȡ��Ӧ�Ĺ�����ַ
port:transitport��intraport
ctrlchnl:����ͨ��
intraddr:��Σ�intraaddr
publicaddr:���Σ�������ַ
*/
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
		delete mapserandpc[sock];
	}
	//���յ�����Ϣ����socketid�Ķ�Ӧ��ϵ��Ȼ���͸���������ͨ������ͨ����->��onconnect�߼���
	return 0;
}
int selfServiceConnect::RcvEx(UCHAR* rcvbuf) {
	DWORD flags = 0; //must initial equal 0, orelse, no message rcv by workthread
	SockExOL* ov = new SockExOL(this, SockExOL::RCV, (char*)rcvbuf+ 12 + 1 + fwdidlen, GLOBALBUFFERLEN - 12 - 1 - fwdidlen);
	int rtn = WSARecv(sock, &ov->wsabuffer, 1, NULL, &flags, &ov->overlapped, NULL);
	//DBG("selfserviceconnect recv: sock: %d, %d, %d", sock, rtn, WSAGetLastError());
	return 0;
}

int selfServiceConnect::onConnect(bool bConnect) {
	if (!bConnect) {//��������3389�������Ӳ��ɹ�
		//this->SockExTCP::onConnect(bConnect);
		StarTlv request(StarTlv::GETDATACHNLADDR);
		request.pack_atom(StarTlv::DRV_CONNECTION_TYPE, sizeof(int), 0);
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

	RcvEx(globalbuf);//�ձ���3389���͹���������
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
	//request.pack_atom(StarTlv::DRV_DATA_PAYLOAD, n, (char*)globalbuf);//��payload,payload��rcvbuf��

	int headlen = 12 + 1 + fwdidlen; // sizeof(_tlv)*3 + 1 subtlv end with 0 + fwdidlen
	_tlv* msg = (_tlv*)globalbuf;
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
	RcvEx(globalbuf);//
	//procTlvMsg(rcvbuf, msglen);
	return 0;


	//update
	//ֱ�ӽ�port�����������ݣ�д��globalbuf�У��ճ�һ���ڴ�ͷ��Ȼ��
	//
}








