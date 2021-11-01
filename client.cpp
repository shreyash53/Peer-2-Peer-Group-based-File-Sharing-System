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

#define MAX_LEN 4096

char back_space = 8;

int chunk_size = 1024 * 512; //512 KB

using namespace std;

const char ip_port_delimiter[] = ":";	  //for ip and port diffentiation
const char afile_data_delimiter[] = "::"; // delimiter for AFile
const char peer_data_delimiter[] = "||";  // delimiter for Peer
const char glbl_data_delimiter[] = " ";	  //global delimiter

const char peer_list_delimiter[] = "&&"; //to differentiate b/w many Peer data

const char secret_prefix[] = "$$ " //denotes the string is not to be printed since it contains data

	vector<string>
	stringSplit(string input, char delim[]);
void send_message(int tracker_socket);
void recv_message(int tracker_socket);

class AFile
{
private:
	string fileName;
	long fileSize;

public:
	AFile() {}

	AFile(string &fileName_, long &fileSize_) : fileName(fileName_), fileSize(fileSize_)
	{
	}

	void setAFile(string &fileName_, long &fileSize_)
	{
		fileName = fileName_;
		fileSize = fileSize_;
	}

	string getFileName()
	{
		return fileName;
	}

	string serializeData()
	{
		return fileName + afile_data_delimiter + to_string(fileSize);
	}

	bool deserialize(string data)
	{
		auto parts = stringSplit(data, afile_data_delimiter);
		if (parts.size() != 2)
			return false;
		long size = stol(parts[1]);
		setAFile(parts[0], size);
		return true;
	}
};

class Peer
{
private:
	string name;
	string pword;
	string ipAddress;
	string port_no;
	// unordered_set<string> groups;

	unordered_map<string, vector<AFile>> sharedFilesInGroup;

	bool isOnline;

public:
	Peer() {}

	Peer(string &name_, string &pword_) : name(name_), pword(pword_), isOnline(false)
	{
	}

	Peer(string &name_, string &ipAddress_, string &port_no_) : name(name_),
																isOnline(false), ipAddress(ipAddress_), port_no(port_no_)
	{
	}

	Peer(string &name_, string &pword_, string &ipAddress_, string &port_no_) : name(name_), pword(pword_),
																				isOnline(false), ipAddress(ipAddress_), port_no(port_no_)
	{
	}

	void operator=(const Peer &p1)
	{
		name = p1.name;
		pword = p1.pword;
		ipAddress = p1.ipAddress;
		port_no = p1.port_no;
		sharedFilesInGroup = p1.sharedFilesInGroup;
		isOnline = p1.isOnline;
	}

	void addGroup(string &gname)
	{
		if (sharedFilesInGroup.count(gname))
		{
			vector<AFile> empty;
			sharedFilesInGroup[gname] = empty;
		}
	}

	vector<AFile> getFileListByGroup(string &groupName)
	{
		return sharedFilesInGroup[groupName];
	}

	void resetAllSharedFiles()
	{
		sharedFilesInGroup.clear();
	}

	bool isPresentInGroup(string &gname)
	{
		if (sharedFilesInGroup.count(gname))
			return true;
		return false;
	}
	void deleteGroup(string &gname)
	{
		if (isPresentInGroup(gname))
			sharedFilesInGroup.erase(gname);
	}

	void deleteSharedFile(string &groupname, string &filename)
	{
		auto fileList = sharedFilesInGroup[groupname];

		for (auto member = fileList.begin(); member != fileList.end(); ++member)
		{
			if (member->getFileName() == filename)
			{
				fileList.erase(member);
				break;
			}
		}
	}

	void logout()
	{
		isOnline = false;
	}
	bool getIsOnline()
	{
		return isOnline;
	}
	string getName()
	{
		return name;
	}

	void addFile(AFile &afile, string &group_name)
	{
		sharedFilesInGroup[group_name].push_back(afile);
	}

	bool loginCondition(string &input_string)
	{
		auto parts = stringSplit(input_string, peer_data_delimiter);
		if (parts.size() == 2 and name == parts[0] and pword == parts[1])
		{
			setIsOnline(true);
			return true;
		}
		return false;
	}

	string serializeData1()
	{
		return name + peer_data_delimiter + pword + peer_data_delimiter + ipAddress + peer_data_delimiter + port_no;
	}

	string serializeData2()
	{
		return name + peer_data_delimiter + ipAddress + peer_data_delimiter + port_no;
	}

	void setPeer(string &name_, string &pword_)
	{
		name = name_;
		pword = pword_;
	}

	void setPeer(string &name_, string &ipAddress_, string &port_no_)
	{
		name = name_;
		ipAddress = ipAddress_;
		port_no = port_no_;
	}

	void setPeer(string &name_, string &pword_, string &ipAddress_, string &port_no_)
	{
		setPeer(name_, pword_);
		setPeer(name_, ipAddress_, port_no_);
	}

	bool deserializeData1(string data)
	{
		auto parts = stringSplit(data, peer_data_delimiter);
		if (parts.size() == 4)
		{
			setPeer(parts[0], parts[1], parts[2], parts[3]);
			return true;
		}
		return false;
	}

	bool deserializeData2(string data)
	{
		auto parts = stringSplit(data, peer_data_delimiter);
		if (parts.size() == 3)
		{
			setPeer(parts[0], parts[1], parts[2]);
			return true;
		}
		return false;
	}

	string serializeData3()
	{
		return name + peer_data_delimiter + pword;
	}

	bool deserializeData3(string data)
	{
		auto parts = stringSplit(data, peer_data_delimiter);
		if (parts.size() == 2)
		{
			setPeer(parts[0], parts[1]);
			return true;
		}
		return false;
	}
};

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
	}

	~Socket()
	{
		close(server_socket);
	}

	void startServerConnection()
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

	int acceptConnectionAtServer()
	{
		int client_socket;
		struct sockaddr_in client;
		unsigned int len = sizeof(sockaddr_in);
		if ((client_socket = accept(server_socket, (struct sockaddr *)&client, &len)) == -1)
		{
			perror("accept error: ");
			// exit(-1);
		}
		else
			cout << "*|*|* "
				 << "  " << client.sin_port << endl;
		return client_socket;
	}

	int connectAtClient()
	{

		if ((connect(server_socket, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) == -1)
		{
			perror("connect: ");
			return -1;
		}
		return server_socket;
	}

	void closeConnection()
	{
		// close(server_socket);
	}
};

vector<string> stringSplit(string input, char delim[])
{

	vector<string> arr;
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
			auto entries = stringSplit(tp, ip_port_delimiter);
			ip_address = entries[0];
			port = entries[1];
		}
		newfile.close();
	}
}

Peer peer;

bool exit_flag = false;
bool tracker_login_status = false;
thread t_send, t_recv;
int tracker_socket;
string peer_ip_address, peer_port;

int main(int argc, char **argv)
{
	string tracker_ip_address, tracker_port;

	if (argc > 2)
		get_tracker_ip_and_port(argv[2], tracker_ip_address, tracker_port);
	else if (argc > 1)
		get_tracker_ip_and_port("tracker.txt", tracker_ip_address, tracker_port);
	else
	{
		cout << "give cmnd line args";
		return 0;
	}

	auto ip_parts = stringSplit(argv[1], afile_data_delimiter);
	if (ip_parts.size() != 2)
	{
		perror("invalid runtime args");
		return 0;
	}

	peer_ip_address = ip_parts[0];
	peer_port = ip_parts[1];

	Socket tracker(tracker_ip_address, tracker_port);

	tracker_socket = tracker.connectAtClient();

	thread t1(send_message, tracker_socket);
	thread t2(recv_message, tracker_socket);

	t_send = move(t1);
	t_recv = move(t2);

	if (t_send.joinable())
		t_send.join();
	if (t_recv.joinable())
		t_recv.join();

	return 0;
}

long findSizeOfFile(string file_name)
{
	FILE *fp = fopen(file_name.c_str(), "r");
	if (fp == NULL)
	{
		return -1;
	}
	fseek(fp, 0L, SEEK_END);
	long int res = ftell(fp);
	fclose(fp);
	return res;
}

void eraseText(int cnt)
{
	for (int i = 0; i < cnt; i++)
	{
		cout << back_space;
	}
}

void input_error()
{
	cout << "Invalid Input. Or Ensure you are logged In to use other commands." << endl;
}

void send_message_helper(string &message, int tracker_socket)
{
	char temp[MAX_LEN];
	strcpy(temp, message.c_str());
	send(tracker_socket, temp, sizeof(temp), 0);
	cout << "msg sent: " << temp << endl;
}

void send_message(int tracker_socket)
{
	string command, message;
	while (1)
	{
		message = "";
		cout << "-> ";
		char str[MAX_LEN];
		cin.getline(str, MAX_LEN);
		auto input_parts = stringSplit(str, glbl_data_delimiter);
		cout << "tracker_login_status: " << tracker_login_status << endl;
		command = input_parts[0];

		if (command == "login")
		{
			if (input_parts.size() == 3)
			{
				// cout << "inside login" << endl;
				Peer t(input_parts[1], input_parts[2], peer_ip_address, peer_port);
				peer = t;
				message = command + " " + peer.serializeData3();
				// cout << "exitting login: " << message << endl;
			}
		}
		else if (command == "create_user")
		{
			if (input_parts.size() == 3)
			{
				Peer t(input_parts[1], input_parts[2], peer_ip_address, peer_port);
				peer = t;
				message = command + " " + peer.serializeData1();
				cout << "inside create " << message << endl;
			}
		}
		else if (command == "pass")
		{
			//for debugging
			cout << "now passing" << endl;
			continue;
		}

		else if (tracker_login_status)
		{
			if (command == "join_group")
			{
				if (input_parts.size() == 2)
				{
					message = str;
					cerr << "inside join group - " << str << endl;
				}
			}

			else if (command == "create_group")
			{
				if (input_parts.size() == 2)
				{
					message = str;
					cerr << "inside create group - " << str << endl;
				}
			}

			else if (command == "leave_group")
			{
				if (input_parts.size() == 2)
				{
					message = str;
					cerr << "inside leave group - " << str << endl;
				}
			}

			else if (command == "requests")
			{
				if (input_parts.size() == 3)
				{
					message = str;
					cerr << "inide requests list requests - " << str << endl;
				}
			}

			else if (command == "accept_request")
			{
				if (input_parts.size() == 3)
				{
					message = str;
					cerr << "inside accept request - " << str << endl;
				}
			}

			else if (command == "list_groups")
			{
				message = command;
				cerr << "inside list_groups" << endl;
			}

			else if (command == "list_files")
			{
				if (input_parts.size() == 2)
				{
					message = str;
					cerr << "inside list files - " << str << endl;
				}
			}

			else if (command == "upload_file")
			{
				if (input_parts.size() == 3)
				{
					string filename = input_parts[1];
					string gname = input_parts[2];
					filesize = findSizeOfFile(filename);
					if (filesize != -1)
					{
						AFile afile(filename, filesize);
						peer.addFile(afile, gname);
						message = command + " " + gname + " " + afile.serializeData();
						cerr << "inside upload file - " << message << endl;
					}
					else
						message = "invalid file.";
				}
			}

			else if (command == "download_file")
			{
				if (input_parts.size() == 4)
				{
					for (int i = 0; i < 3; i++)
						message += input_parts[i] + glbl_data_delimiter;
					cerr << "inside download - " << str << endl;
				}
			}

			else if (command == "logout")
			{
				message = command;
				cerr << "inside logout" << endl;
			}

			else if (command == "show_downloads")
			{
			}

			else if (command == "stop_share")
			{
				if (input_parts.size() == 3)
				{
					message = str;
					cerr << "inside stop share " << str << endl;
				}
			}
		}

		if (message.length() == 0)
		{
			input_error();
			continue;
		}

		send_message_helper(message, tracker_socket);

		// if(strcmp(str,"#exit")==0)
		// {
		// 	exit_flag=true;
		// 	t_recv.detach();
		// 	close(tracker_socket);
		// 	return;
		// }
	}
}

// Receive message
void recv_message(int tracker_socket)
{
	while (1)
	{
		if (exit_flag)
			return;
		char name[MAX_LEN], msg[MAX_LEN];
		int bytes_received = recv(tracker_socket, msg, sizeof(msg), 0);
		if (bytes_received <= 0)
			continue;
		if (msg[0] == '$' and msg[1] == '$')
		{
			cerr << "download -->> " << msg << endl;
			auto data = stringSplit(msg, glbl_data_delimiter);
			if(data.size() == 4){
				cerr << "woking it seems" << endl;
			}
		}
		else
		{
			if (msg[0] == '#' and msg[1] == '#')
			{
				int loginStatus;
				recv(tracker_socket, &loginStatus, sizeof(loginStatus), 0);
				if (loginStatus)
					tracker_login_status = (bool)loginStatus;
			}

			// recv(client_socket,msg,sizeof(msg),0);
			eraseText(3);
			cout << msg << endl;
			cout << "-> ";
		}
		fflush(stdout);
	}
}
