#include "server.h"

int Server::numberOfConnectedClients = 0;
int Server::clients[1];
char Server::message[500];
pthread_mutex_t  Server::mutex = PTHREAD_MUTEX_INITIALIZER;

void Server::sendMessage(char *message, int socket)
{
	int i;
	pthread_mutex_lock(&mutex);
	for(i = 0; i < numberOfConnectedClients; i++) 
    {
		if(clients[i] != socket)
        {
			if(send(clients[i], message, strlen(message), 0) < 0) 
            {
				perror("sending failure");
				continue;
			}
		}
	}
	pthread_mutex_unlock(&mutex);	
}

void *Server::receiveMessage(void *sock)
{
	struct ClientInfo client = *((struct ClientInfo *)sock);
	char message[500];
	int length;

	while((length = recv(client.mySocket, message, 500, 0)) > 0) 
	{
		message[length] = '\0';
		sendMessage(message, client.mySocket);
		memset(message,'\0', sizeof(message));
	}

	pthread_mutex_lock(&mutex);
	printf("%s disconnected\n", client.ip);
	
	for(int i = 0; i < numberOfConnectedClients; i++) 
	{
		if(clients[i] == client.mySocket) 
		{
			int j = i;
			while(j < numberOfConnectedClients - 1) 
			{
				clients[j] = clients[j + 1];
				j++;
			}
		}
	}

	numberOfConnectedClients--;
	pthread_mutex_unlock(&mutex);
	
	return 0;
}

void Server::startServer()
{
	int clientSocket, serverSocket; 
	int clientCounter = 0;
	socklen_t clientAddressSize;

	serverSocket = socket(AF_INET,SOCK_STREAM,0);
	memset(serverAddress.sin_zero,'\0',sizeof(serverAddress.sin_zero));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	clientAddressSize = sizeof(clientAddress);

	bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

	listen(serverSocket, 1);
	
	while(1) 
	{
		if(clientCounter < 2)
		{
			clientSocket = accept(serverSocket,(struct sockaddr *)&clientAddress,&clientAddressSize);
			handleSession(clientSocket);
			clientCounter++;
		}
	}
}

void Server::handleSession(int clientSocket)
{
	pthread_mutex_lock(&mutex);
	inet_ntop(AF_INET, (struct sockaddr *)&clientAddress, ip, INET_ADDRSTRLEN);
	printf("%s connected\n",ip);
	client.mySocket = clientSocket;
	strcpy(client.ip, ip);
	clients[numberOfConnectedClients] = clientSocket;
	numberOfConnectedClients++;
	pthread_create(&receiveThread, NULL, receiveMessage, &client);
	pthread_mutex_unlock(&mutex);
}