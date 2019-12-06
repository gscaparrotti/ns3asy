/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "genericApp.h"
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <unistd.h>
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ns3asy-GenericApp");

GenericApp::GenericApp() :
		m_serverSocket(0),
		m_sendSockets(),
		m_nextSocket(0),
		m_packetSize(0),
		m_nPackets(0),
		m_dataRate(0),
		m_sendEvent(),
		m_running(false),
		m_packetsSent(0),
		m_onReceiveFtn(0),
		m_onPacketReadFtn(0),
		m_onAcceptFtn(0),
		m_onSendFtn(0)
		{}

GenericApp::~GenericApp() {
	//l'operatore '=' è overloaded per Ptr, perciò, ogni volta che viene fatto un assegnamento,
	//vengono chiamati i metodi Ref() o Unref() (a seconda che il puntatore sia diverso da 0 o meno)
	//sull'oggetto referenziato, che ereditando da Object o da SimpleRefCount ha questi due metodi.
	//Ogni oggetto quindi tiene traccia dei riferimenti a sè stesso.
	//Se un oggetto ha al suo interno dei riferimenti ad un altro oggetto, nel suo distruttore ci
	//dev'essere una espressione di questo tipo, per indicare che l'oggetto referenziato non è più
	//in uso.
	m_serverSocket = 0;
	m_sendSockets.clear();
}

void GenericApp::SetOnAcceptFunction(void (*onAcceptFtn)(const char[], unsigned int, const char[], unsigned int)) {
	m_onAcceptFtn = onAcceptFtn;
}

void GenericApp::SetOnReceiveFunction(void (*onReceiveFtn)(const char[], unsigned int)) {
	m_onReceiveFtn = onReceiveFtn;
}

void GenericApp::SetOnPacketReadFunction(void (*onPacketReadFtn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int)) {
	m_onPacketReadFtn = onPacketReadFtn;
}

void GenericApp::SetOnSendFunction(void (*onSendFtn)(const char[], unsigned int, const char[], unsigned int, const unsigned char[], unsigned int)) {
	m_onSendFtn = onSendFtn;
}

void GenericApp::Setup(Ptr<Socket> serverSocket, vector<Ptr<Socket>> sendSockets) {
	m_sendSockets = sendSockets;
	m_serverSocket = serverSocket;
}

void GenericApp::ConnectToPeer(Address address) {
	m_sendSockets.at(m_nextSocket++)->Connect(address);
}

void GenericApp::SendPackets(uint32_t packetSize, uint32_t nPackets, DataRate dataRate, const char* payload, int length) {
	m_packetSize = packetSize;
	m_nPackets = nPackets;
	m_dataRate = dataRate;
	m_packetsSent = 0;
	SendPacket(payload, length);
}

void GenericApp::StartApplication(void) {
	m_running = true;
	m_serverSocket->SetAcceptCallback(
			MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
			MakeCallback(&GenericApp::OnAccept, this));
	m_serverSocket->SetRecvCallback(MakeCallback(&GenericApp::OnReceive, this));
	m_packetsSent = 0;
	for (unsigned int i = 0; i < m_sendSockets.size(); i++) {
		m_sendSockets.at(i)->Bind();
	}
	m_serverSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 8080));
	m_serverSocket->Listen();
}

//called only if stop time for this application is less than the stop time for the simulation
void GenericApp::StopApplication(void) {
	m_running = false;
	if (m_sendEvent.IsRunning()) {
		Simulator::Cancel(m_sendEvent);
	}
	if (m_serverSocket) {
		//if a server socket is closed, its forked sockets are closed as well
		m_serverSocket->Close();
	}
	for (unsigned int i = 0; i < m_sendSockets.size(); i++) {
		m_sendSockets.at(i)->Close();
	}
}

//SendPacket viene chiamato per la prima volta da StartApplication.
//Una volta inviato il pacchetto chiama il metodo ScheduleTx, che a sua volta
//mette nella coda degli eventi una nuova chiamata a SendPacket per
//l'invio del pacchetto successivo al momento opportuno
void GenericApp::SendPacket(const char* payload, int length) {
	if (m_packetsSent < m_nPackets) {
		for (unsigned int i = 0; i < m_sendSockets.size(); i++) {
			Ptr<Socket> m_sendSocket = m_sendSockets.at(i);
			Ptr<Packet> packet = Create<Packet>(reinterpret_cast<const unsigned char*>(payload), length);
			m_sendSocket->Send(packet);
			if (m_onSendFtn) {
				Ptr<ConnectionInfo> connectionInfo = CreateObject<ConnectionInfo>();
				connectionInfo->SetSenderAddress(ConnectionInfo::NameFromSocket(m_sendSocket));
				connectionInfo->SetReceiverAddress(ConnectionInfo::PeerNameFromSocket(m_sendSocket));
				unsigned char payload[packet->GetSize()];
				packet->CopyData(payload, packet->GetSize());
				m_onSendFtn(connectionInfo->Get().senderIp, connectionInfo->Get().senderPort,
						connectionInfo->Get().receiverIp, connectionInfo->Get().receiverPort,
						payload, packet->GetSize());
			}
		}
		m_packetsSent++;
		ScheduleTx(payload, length);
	}
}

//generare gli eventi è responsabilità dell' Application
void GenericApp::ScheduleTx(const char* payload, int length) {
	if (m_running) {
		//la frequenza con cui l' Application genera gli eventi non è necessariamente
		//dipendente dalla velocità del canale di comunicazione sottostante
		Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
		//l'evento del prossimo invio viene assegnato al campo m_sendEvent per
		//poterlo cancellare nel momento in cui la simulazione si deve fermare
		m_sendEvent = Simulator::Schedule(tNext, &GenericApp::SendPacket, this, payload, length);
	}
}

void GenericApp::OnAccept(Ptr<Socket> socket, const Address &from) {
	if (m_onAcceptFtn) {
		Ptr<ConnectionInfo> connectionInfo = CreateObject<ConnectionInfo>();
		connectionInfo->SetReceiverAddress(ConnectionInfo::NameFromSocket(socket));
		connectionInfo->SetSenderAddress(from);
		m_onAcceptFtn(connectionInfo->Get().receiverIp, connectionInfo->Get().receiverPort,
				connectionInfo->Get().senderIp, connectionInfo->Get().senderPort);

	}
	//la callback Recv deve essere impostata all'interno della callback onAccept sull'oggetto fornito come parametro
	//perchè quando una socket tcp diventa connessa viene duplicata e le precedenti callback impostate
	//vengono eliminate
	socket->SetRecvCallback(MakeCallback(&GenericApp::OnReceive, this));
}

void GenericApp::OnReceive(Ptr<Socket> socket) {
	Ptr<ConnectionInfo> connectionInfo = CreateObject<ConnectionInfo>();
	connectionInfo->SetReceiverAddress(ConnectionInfo::NameFromSocket(socket));
	if (m_onReceiveFtn) {
		m_onReceiveFtn(connectionInfo->Get().receiverIp, connectionInfo->Get().receiverPort);
	}
	Ptr<Packet> packet;
	Address from;
	unsigned int totalSize = 0;
	while ((packet = socket->RecvFrom(from))) { //tells the compiler: Yes, I really want to do that assignment!
		if (m_onPacketReadFtn) {
			connectionInfo->SetSenderAddress(from);
			totalSize += packet->GetSize();
			unsigned char payload[packet->GetSize()];
			packet->CopyData(payload, packet->GetSize());
			m_onPacketReadFtn(connectionInfo->Get().receiverIp, connectionInfo->Get().receiverPort,
					connectionInfo->Get().senderIp, connectionInfo->Get().senderPort,
					payload, packet->GetSize());
		}
	}
//	if (m_isServer) {
//		Ptr<Packet> newPacket = Create<Packet>(totalSize != 0 ? totalSize : 1);
//		int sent = socket->Send(newPacket);
//		NS_LOG_DEBUG("After reading all the packets, " << sent << " byte(s) have been sent back");
//	}
}

ConnectionInfo::ConnectionInfo() {}

void ConnectionInfo::SetReceiverAddress(Address receiver) {
	strcpy(cid.receiverIp, IpAsStringFromAddress(receiver).c_str());
	cid.receiverPort = PortFromAddress(receiver);
}

void ConnectionInfo::SetSenderAddress(Address sender) {
	strcpy(cid.senderIp, IpAsStringFromAddress(sender).c_str());
	cid.senderPort = PortFromAddress(sender);
}

string ConnectionInfo::IpAsStringFromAddress(Address address) {
	std::ostringstream ip;
	InetSocketAddress::ConvertFrom(address).GetIpv4().Print(ip);
	return ip.str();
}

unsigned int ConnectionInfo::PortFromAddress(Address address) {
	return InetSocketAddress::ConvertFrom(address).GetPort();
}

ConnectionInfoData ConnectionInfo::Get() {
	return cid;
}

Address ConnectionInfo::NameFromSocket(Ptr<Socket> socket) {
	Address address;
	socket->GetSockName(address);
	return address;
}

Address ConnectionInfo::PeerNameFromSocket(Ptr<Socket> socket) {
	Address address;
	socket->GetPeerName(address);
	return address;
}
