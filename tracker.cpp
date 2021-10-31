#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <signal.h>
#include <mutex>
// #define MAX_LEN 200

using namespace std;

struct terminal
{
	int id;
	string name;
	int socket;
	thread th;
};

vector<terminal> clients;

int seed = 0;
mutex cout_mtx, clients_mtx;

void set_name(int id, char name[]);
void broadcast_message(string message, int sender_id);
void broadcast_message(int num, int sender_id);
void end_connection(int id);
void handle_client(int client_socket, int id);

class Socket
{
private:
	int server_socket;
	struct sockaddr_in server;

public:
	Socket(string &ip_address, string &port)
	{

		if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			perror("socket: ");
			exit(-1);
		}
		server.sin_family = AF_INET;
		server.sin_port = htons(10000);
		server.sin_addr.s_addr = INADDR_ANY;
		bzero(&server.sin_zero, 0);
		startConnection();
	}

	void startConnection()
	{
		if ((bind(server_socket, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) == -1)
		{
			perror("bind error: ");
		}
		if ((listen(server_socket, 8)) == -1)
		{
			perror("listen error: ");
		}
	}

	int acceptConnection()
	{
		int client_socket;
		struct sockaddr_in client;
	unsigned int len = sizeof(sockaddr_in);
		if ((client_socket = accept(server_socket, (struct sockaddr *)&client, &len)) == -1)
		{
			perror("accept error: ");
			// exit(-1);
		}
		return client_socket;
	}
};

vector<string> stringSplit(string input)
{

	vector<string> arr;
	char delim[] = ":";
	char *inp = &input[0];
	char *token = strtok(inp, delim);

	while (token)
	{
		string s(token);
		arr.push_back(s);
		token = strtok(NULL, delim);
	}

	return arr;
}

void get_tracker_ip_and_port(string file_name, string &ip_address, string &port)
{
	fstream newfile;
	newfile.open(file_name, ios::in);
	if (newfile.is_open())
	{
		string tp;
		if (getline(newfile, tp))
		{
			auto entries = stringSplit(tp);
			ip_address = entries[0];
			port = entries[1];
		}
		newfile.close();
	}
}

int main(int argc, char **argv)
{
	string ip_address, port;

	if (argc > 1)
		get_tracker_ip_and_port(argv[1], ip_address, port);
	else
		get_tracker_ip_and_port("tracker.txt", ip_address, port);

	Socket tracker(ip_address, port);

	while (true)
	{
		int client_socket = tracker.acceptConnection();
		seed++;
		thread t(handle_client, client_socket, seed);
		lock_guard<mutex> guard(clients_mtx);
		clients.push_back({seed, string("Anonymous"), client_socket, (move(t))});
	}
	for (int i = 0; i < clients.size(); i++)
	{
		if (clients[i].th.joinable())
			clients[i].th.join();
	}
	close(server_socket);
	return 0;
}

// Set name of client
void set_name(int id, char name[])
{
	for (int i = 0; i < clients.size(); i++)
	{
		if (clients[i].id == id)
			clients[i].name = string(name);
	}
}

// For synchronisation of cout statements
void shared_print(string str, bool endLine = true)
{
	lock_guard<mutex> guard(cout_mtx);
	cout << str;
	if (endLine)
		cout << endl;
}

// Broadcast message to all clients except the sender
void broadcast_message(string message, int sender_id)
{
	char temp[MAX_LEN];
	strcpy(temp, message.c_str());
	for (int i = 0; i < clients.size(); i++)
	{
		if (clients[i].id != sender_id)
			send(clients[i].socket, temp, sizeof(temp), 0);
	}
}

// Broadcast a number to all clients except the sender
void broadcast_message(int num, int sender_id)
{
	for (int i = 0; i < clients.size(); i++)
	{
		if (clients[i].id != sender_id)
			send(clients[i].socket, &num, sizeof(num), 0);
	}
}

void end_connection(int id)
{
	for (int i = 0; i < clients.size(); i++)
	{
		if (clients[i].id == id)
		{
			lock_guard<mutex> guard(clients_mtx);
			clients[i].th.detach();
			clients.erase(clients.begin() + i);
			close(clients[i].socket);
			break;
		}
	}
}

void handle_client(int client_socket, int id)
{
	char name[MAX_LEN], str[MAX_LEN];
	recv(client_socket, name, sizeof(name), 0);
	set_name(id, name);

	// Display welcome message
	string welcome_message = string(name) + string(" has joined");
	broadcast_message("#NULL", id);
	broadcast_message(id, id);
	broadcast_message(welcome_message, id);
	shared_print(color(id) + welcome_message + def_col);

	while (true)
	{
		int bytes_received = recv(client_socket, str, sizeof(str), 0);
		if (bytes_received <= 0)
			return;
		if (strcmp(str, "#exit") == 0)
		{
			// Display leaving message
			string message = string(name) + string(" has left");
			broadcast_message("#NULL", id);
			broadcast_message(id, id);
			broadcast_message(message, id);
			shared_print(color(id) + message + def_col);
			end_connection(id);
			return;
		}
		broadcast_message(string(name), id);
		broadcast_message(id, id);
		broadcast_message(string(str), id);
		shared_print(color(id) + name + " : " + def_col + str);
	}
}