#include "defaultCallbacks.h"
#include "ns3/core-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ns3asy-cbs");

void ConnectionAccepted(const char receiverIp[], unsigned int receiverPort, const char senderIp[],
		unsigned int senderPort, double time) {
	NS_LOG_DEBUG("A connection has been accepted by the socket " << receiverIp << ":"
			<< receiverPort << "; it was requested by " << senderIp << ":" << senderPort << " @ " << time);
}

void PacketReceived(const char ip[], unsigned int port, double time) {
	NS_LOG_DEBUG("A packet has been received by the socket with ip=" << ip << " and port=" << port << " @ " << time);
}

void PacketRead(const char receiverIp[], unsigned int receiverPort, const char senderIp[],
		unsigned int senderPort, const unsigned char payload[], unsigned int payloadLength, double time) {
	std::ostringstream outputDebug;
	std::ostringstream outputInfo;
	outputDebug << "Some packets has been read by the socket " << receiverIp << ":"
			<< receiverPort << "; it was sent by " << senderIp << ":" << senderPort << " @ " << time;
	outputInfo << "The read content is: ";
	for (unsigned int i = 0; i < payloadLength; i++) {
		outputInfo << payload[i];
	}
	NS_LOG_DEBUG(outputDebug.str().c_str());
	NS_LOG_INFO(outputInfo.str().c_str());
}

void PacketSent(const char senderIp[], unsigned int senderPort, const char receiverIp[],
		unsigned int receiverPort, const unsigned char payload[], unsigned int payloadLength, double time) {
	std::ostringstream outputDebug;
	std::ostringstream outputInfo;
	outputDebug << "A packet has been sent by the socket " << senderIp << ":"
			<< senderPort << "; it was sent to " << receiverIp << ":" << receiverPort << " @ " << time;
	outputInfo << "The sent content is: ";
	for (unsigned int i = 0; i < payloadLength; i++) {
		outputInfo << payload[i];
	}
	NS_LOG_DEBUG(outputDebug.str().c_str());
	NS_LOG_INFO(outputInfo.str().c_str());
}

void SocketClosed(const char ip[], unsigned int port, bool error) {
	NS_LOG_DEBUG("The socket " << ip << ":" << port << " has been closed "
			<< (error ? "due to an error" : "gracefully"));
}
