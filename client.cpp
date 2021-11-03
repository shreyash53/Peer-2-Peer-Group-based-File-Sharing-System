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
#define SOCKET_SIZE sizeof(struct sockaddr_in)

char back_space = 8;

int chunk_size = 1024 * 512; //512 KB

using namespace std;

char ip_port_delimiter[] = ":";		//for ip and port diffentiation
char afile_data_delimiter[] = "::"; // delimiter for AFile
char peer_data_delimiter[] = "||";	// delimiter for Peer
char glbl_data_delimiter[] = " ";	//global delimiter

char peer_list_delimiter[] = "&&"; //to differentiate b/w many Peer data

char secret_prefix[] = "$$"; //denotes the string is not to be printed since it contains data

vector<string> stringSplit(string input, char delim[]);
void send_message(int tracker_socket);
void recv_message(int tracker_socket);

queue<vector<string>> commandsExecuted; //to ensure the result of an event oriented command is reflected
mutex cmmds_exc_mtx;

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

	// pairs dumps()
	// {
	// 	return make_pair(fileName, fileSize);
	// }

	// void retrieve(pairs data)
	// {
	// 	setAFile(data.first, data.second);
	// }

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
	unordered_map<string, pair<AFile, int>> sharedFiles;			 //key = filename and value = pair of file object and no. of groups the file is shared
	unordered_map<string, unordered_set<string>> sharedFilesInGroup; //key = group name and value = set of filenames

	bool isOnline;

	void setIsOnline(bool status)
	{
		isOnline = status;
	}

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
		if (!isPresentInGroup(gname))
		{
			unordered_set<string> e;
			sharedFilesInGroup[gname] = e;
		}
	}

	unordered_set<string> getFileListByGroup(string &groupName)
	{
		return sharedFilesInGroup[groupName];
	}

	// void resetAllSharedFiles()
	// {
	// 	sharedFilesInGroup.clear();
	// }

	bool isPresentInGroup(string &gname)
	{
		if (sharedFilesInGroup.count(gname))
			return true;
		return false;
	}

	void deleteGroup(string &gname)
	{
		if (isPresentInGroup(gname))
		{
			for (auto file : sharedFilesInGroup[gname])
			{
				if (sharedFiles[file].second < 2)
					sharedFiles.erase(file);
				else
					sharedFiles[file].second--;
			}
			sharedFilesInGroup.erase(gname);
		}
	}

	void deleteAllSharedFilesByGroup(string &groupname)
	{
		for (auto filelist : sharedFilesInGroup[groupname])
			deleteSharedFile(groupname, filelist);
	}

	void deleteSharedFile(string &groupname, string &filename)
	{
		sharedFiles[filename].second--;
		if (sharedFiles[filename].second == 0)
			sharedFiles.erase(filename);
		sharedFilesInGroup[groupname].erase(filename);
	}

	void logout()
	{
		setIsOnline(false);
	}
	bool getIsOnline()
	{
		return isOnline;
	}
	string getName()
	{
		return name;
	}

	AFile getFileObject(string &groupname, string &filename)
	{

		if (sharedFilesInGroup[groupname].count(filename))
			return sharedFiles[filename].first;
		return AFile();
	}

	void addFile(AFile &afile, string &group_name)
	{
		if (sharedFilesInGroup[group_name].count(afile.getFileName()))
			return;
		sharedFilesInGroup[group_name].insert(afile.getFileName());
		if (sharedFiles.count(afile.getFileName()))
			sharedFiles[afile.getFileName()].second++;
		else
			sharedFiles[afile.getFileName()] = make_pair(afile, 1);
	}

	bool loginCondition(string &input_string)
	{
		auto parts = stringSplit(input_string, peer_data_delimiter);
		cerr << "logincondition: " << parts.size() << " " << this->isOnline << endl;
		if (parts.size() == 2 and name == parts[0] and pword == parts[1] and !isOnline)
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

	string dumps()
	{
		string msg = serializeData3() + "==>\n";
		bool flag = true;
		for (auto file_ : sharedFiles)
		{
			msg += file_.second.first.serializeData() + "--" + to_string(file_.second.second) + ",  ";
			flag = false;
		}
		if (flag)
			msg += "no file";
		msg += "\n";
		for (auto fl : sharedFilesInGroup)
			msg += fl.first + ", ";
		msg += "No Group\n";
		return msg;
	}

	string serializeDebug()
	{
		string res = "";
		for (auto ptr : sharedFiles)
		{
			res += ptr.first + " -> ";
			// for(auto fi: ptr.second){
			res += ptr.second.first.serializeData() + ", ";
			// }
			res += "\n";
		}
		return res;
	}
};

class Socket
{
public:
	int socketDescriptor;
	struct sockaddr_in socketAddr;

	Socket(){}

	Socket(string &ip_address, string &port)
	{
		if ((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			perror("socket error: ");
			exit(-1);
		}
		socketAddr.sin_family = AF_INET;
		socketAddr.sin_port = htons(stoi(port));
		socketAddr.sin_addr.s_addr = INADDR_ANY;
		socketAddr.sin_addr.s_addr = inet_addr(ip_address.c_str());
		bzero(&socketAddr.sin_zero, 0);
	}

	~Socket()
	{
		close(socketDescriptor);
	}

	void startServerConnection()
	{
		bindConnection();
		if ((listen(socketDescriptor, 8)) == -1)
		{
			perror("listen error: ");
			exit(-1);
		}
	}

	void acceptConnectionAtServer(Socket* clientSocket)
	{
		unsigned long adr_size = SOCKET_SIZE;
		if ((clientSocket->socketDescriptor = accept(socketDescriptor, (struct sockaddr *)&clientSocket->socketAddr, (socklen_t*)&adr_size)) == -1)
		{
			perror("accept error: ");
		}
		else{
			cout << "*|*|* " << "  " << clientSocket->socketAddr.sin_port << endl;
			char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientSocket->socketAddr.sin_addr), ip, INET_ADDRSTRLEN);
      
        // "ntohs(peer_addr.sin_port)" function is 
        // for finding port number of client
        printf("connection established with IP : %s and PORT : %d\n", ip, ntohs(clientSocket->socketAddr.sin_port));
		}
	}

	void bindConnection(){
		if ((bind(socketDescriptor, (struct sockaddr *)&socketAddr, SOCKET_SIZE)) == -1)
		{
			perror("bind error: ");
			exit(-1);
		}
	}

	int connectAtClient(int clientSocketDescriptor)
	{
		if ((connect(clientSocketDescriptor, (struct sockaddr *)&socketAddr, SOCKET_SIZE)) == -1)
		{
			perror("connect: ");
			return -1;
		}
		return 0;
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

Socket* clientSocket;

void catch_ctrl_c(int signal)
{
	char str[MAX_LEN] = "$$ exit";
	// char str[MAX_LEN]=secret_prefix + glbl_data_delimiter + "exit";
	send(tracker_socket, str, sizeof(str), 0);
	exit_flag = true;
	t_send.detach();
	t_recv.detach();
	close(tracker_socket);
	exit(signal);
}

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

	auto ip_parts = stringSplit(argv[1], ip_port_delimiter);
	if (ip_parts.size() != 2)
	{
		perror("invalid runtime args");
		return 0;
	}

	peer_ip_address = ip_parts[0];
	peer_port = ip_parts[1];

	Socket tracker(tracker_ip_address, tracker_port);

	Socket client(peer_ip_address, peer_port);
	clientSocket = &client;
	tracker_socket = clientSocket->socketDescriptor;
	clientSocket->bindConnection();

	if(tracker.connectAtClient(clientSocket->socketDescriptor) != 0){
		cout << "error while connecting to tracker" << endl;
		return -1;
	}

	signal(SIGINT, catch_ctrl_c);

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
	cerr << "msg sent: " << temp << endl;
}

void send_message(int tracker_socket)
{
	string command, message;
	while (1)
	{
		if (exit_flag)
			return;
		message = "";
		cout << "-> ";
		char str[MAX_LEN];
		cin.getline(str, MAX_LEN);
		auto input_parts = stringSplit(str, glbl_data_delimiter);
		cerr << "tracker_login_status: " << tracker_login_status << endl;
		command = input_parts[0];

		if (command == "login")
		{
			if (input_parts.size() == 3)
			{
				commandsExecuted.push(input_parts);
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
				cerr << "inside create " << message << endl;
			}
		}
		else if (command == "pass")
		{
			//for debugging
			cerr << "now passing" << endl;
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
					commandsExecuted.push(input_parts);
					message = str;
					cerr << "inside create group - " << str << endl;
				}
			}

			else if (command == "leave_group")
			{
				if (input_parts.size() == 2)
				{
					commandsExecuted.push(input_parts);
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
					// if(!peer.isPresentInGroup(input_parts[1]))
					commandsExecuted.push(input_parts);
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
					long filesize = findSizeOfFile(filename);
					if (filesize != -1)
					{
						input_parts.push_back(to_string(filesize));
						commandsExecuted.push(input_parts);
						AFile afile(filename, filesize);
						message = command + " " + gname + " " + afile.serializeData();
						cerr << "inside upload file - " << message << endl;
					}
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
				commandsExecuted.push(input_parts);
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
					commandsExecuted.push(input_parts);

					message = str;
					cerr << "inside stop share " << str << endl;
				}
			}
		}

		if (message.length() == 0)
		{
			input_error();
		}
		else
		{
			send_message_helper(message, tracker_socket);
		}

		// if(strcmp(str,"#exit")==0)
		// {
		// 	exit_flag=true;
		// 	t_recv.detach();
		// 	close(tracker_socket);
		// 	return;
		// }
	}
}

void action_event(vector<string> &);
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
			if (data.size() == 4)
			{
				cerr << "woking it seems" << endl;
			}
		}
		else
		{
			if (msg[0] == '#' and msg[1] == '#')
			{
				// int loginStatus;
				char action[MAX_LEN];
				recv(tracker_socket, action, sizeof(action), 0);
				// if (loginStatus)
				// 	tracker_login_status = (bool)loginStatus;
				auto parts = stringSplit(action, glbl_data_delimiter);
				action_event(parts);
			}

			// recv(client_socket,msg,sizeof(msg),0);
			eraseText(3);
			cout << msg << endl;
			cout << "-> ";
		}
		fflush(stdout);
	}
}

void debug();

void action_event(vector<string> &parts)
{
	string inputCommand = parts[0];
	int answer = stoi(parts[1]);
	cerr << "inside action event" << endl;
	cerr << "input command: " << inputCommand << " " << answer << endl;
	vector<string> input_parts;
	unique_lock<mutex> ulck(cmmds_exc_mtx);
	while (!commandsExecuted.empty())
	{
		auto parts_ = commandsExecuted.front();
		commandsExecuted.pop();
		if (parts_[0] == inputCommand)
		{
			input_parts = parts_;
			break;
		}
	}

	cerr << "below queue " << endl;

	if (input_parts.size() == 0)
		return;

	cerr << "now action start " << endl;

	if (inputCommand == "login")
	{
		tracker_login_status = answer;
	}

	// if (inputCommand == "join_group")
	// {
	// 	join_group(peerThreadObj, input_in_parts);
	// }

	else if (inputCommand == "create_group" and answer)
	{

		peer.addGroup(input_parts[1]);
	}

	else if (inputCommand == "leave_group" and answer)
	{
		peer.deleteGroup(input_parts[1]);
	}

	// else if (inputCommand == "download_file" and answer)
	// {
	// 	download_file(peerThreadObj, input_in_parts);
	// }

	else if (inputCommand == "list_files" and answer)
	{
		if (!peer.isPresentInGroup(input_parts[1]))
			peer.addGroup(input_parts[1]);
	}

	else if (inputCommand == "upload_file" and answer)
	{
		if (!peer.isPresentInGroup(input_parts[2]))
			peer.addGroup(input_parts[2]);
		long filesize = stol(input_parts[3]);
		AFile afile(input_parts[1], filesize);
		peer.addFile(afile, input_parts[2]);
	}

	else if (inputCommand == "logout" and answer)
	{
		tracker_login_status = false;
	}

	else if (inputCommand == "stop_share" and answer)
	{
		peer.deleteSharedFile(input_parts[1], input_parts[2]);
	}
	debug();
}

void debug()
{
	cerr << "peer" << endl;
	cerr << peer.serializeDebug() << endl;
}
