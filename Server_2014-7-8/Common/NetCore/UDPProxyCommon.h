#pragma once

enum UDPProxyMessages
{
	ID_UDP_PROXY_FORWARDING_SUCCEEDED,
	ID_UDP_PROXY_FORWARDING_NOTIFICATION,
	ID_UDP_PROXY_NO_SERVERS_ONLINE,
	ID_UDP_PROXY_RECIPIENT_GUID_NOT_CONNECTED_TO_COORDINATOR,
	ID_UDP_PROXY_ALL_SERVERS_BUSY,
	ID_UDP_PROXY_IN_PROGRESS,
	ID_UDP_PROXY_FORWARDING_REQUEST_FROM_CLIENT_TO_COORDINATOR,
	ID_UDP_PROXY_PING_SERVERS_FROM_COORDINATOR_TO_CLIENT,
	ID_UDP_PROXY_PING_SERVERS_REPLY_FROM_CLIENT_TO_COORDINATOR,
	ID_UDP_PROXY_FORWARDING_REQUEST_FROM_COORDINATOR_TO_SERVER,
	ID_UDP_PROXY_FORWARDING_REPLY_FROM_SERVER_TO_COORDINATOR,
	ID_UDP_PROXY_LOGIN_REQUEST_FROM_SERVER_TO_COORDINATOR,
	ID_UDP_PROXY_LOGIN_SUCCESS_FROM_COORDINATOR_TO_SERVER,
	ID_UDP_PROXY_ALREADY_LOGGED_IN_FROM_COORDINATOR_TO_SERVER,
	ID_UDP_PROXY_NO_PASSWORD_SET_FROM_COORDINATOR_TO_SERVER,
	ID_UDP_PROXY_WRONG_PASSWORD_FROM_COORDINATOR_TO_SERVER
};


#define UDP_FORWARDER_MAXIMUM_TIMEOUT (60000 * 10)
