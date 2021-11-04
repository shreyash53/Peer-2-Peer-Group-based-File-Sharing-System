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
#include <openssl/sha.h>

#define MAX_LEN 4096
#define SOCKET_SIZE sizeof(struct sockaddr_in)

char back_space = 8;

const int chunk_size = 1024 * 512 - 1; //512 KB

using namespace std;

char ip_port_delimiter[] = ":";		//for ip and port diffentiation
char afile_data_delimiter[] = "::"; // delimiter for AFile
char peer_data_delimiter[] = "||";	// delimiter for Peer
char glbl_data_delimiter[] = " ";	//global delimiter

char peer_list_delimiter[] = "&&"; //to differentiate b/w many Peer data

char secret_prefix[] = "$$"; //denotes the string is not to be printed since it contains data

vector<string> stringSplit(string input, char delim[]);
void send_to_tracker(int tracker_socket);
void recv_from_tracker(int tracker_socket);
void action_event(vector<string> &);
void download_manager(int, vector<string> &);

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

	long getFileSize()
	{
		return fileSize;
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

	bool hasFile(string &filename)
	{
		return sharedFiles.count(filename);
	}

	unordered_set<string> getFileListByGroup(string &groupName)
	{
		return sharedFilesInGroup[groupName];
	}

	string getIPAdress()
	{
		return ipAddress;
	}

	string getPortNo()
	{
		return port_no;
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

	AFile getFileObject(string &filename)
	{
		return sharedFiles[filename].first;
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

	Socket() {
		if ((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			perror("socket error: ");
			exit(-1);
		}
		socketAddr.sin_family = AF_INET;
		socketAddr.sin_addr.s_addr = INADDR_ANY;
		bzero(&socketAddr.sin_zero, 0);
	}

	Socket(string &ip_address, string &port) : Socket()
	{
		socketAddr.sin_port = htons(stoi(port));
		socketAddr.sin_addr.s_addr = inet_addr(ip_address.c_str());
	}

	// ~Socket()
	// {
		
	// }

	void startServerConnection()
	{
		bindConnection();
		if ((listen(socketDescriptor, 8)) == -1)
		{
			perror("listen error: ");
			exit(-1);
		}
	}

	void acceptConnectionAtServer(Socket *clientSocket)
	{
		unsigned long adr_size = SOCKET_SIZE;
		if ((clientSocket->socketDescriptor = accept(socketDescriptor, (struct sockaddr *)&clientSocket->socketAddr, (socklen_t *)&adr_size)) == -1)
		{
			perror("accept error: ");
		}
		else
		{
			cerr << "*|*|* "
				 << "  " << clientSocket->socketAddr.sin_port << endl;
			char ip[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(clientSocket->socketAddr.sin_addr), ip, INET_ADDRSTRLEN);

			// "ntohs(peer_addr.sin_port)" function is
			// for finding port number of client
			cerr << "connection established with IP : " << ip << "and PORT : " << ntohs(clientSocket->socketAddr.sin_port) << endl;
		}
	}

	void bindConnection()
	{
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
		close(socketDescriptor);
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

void handle_peer(int peer_socket);

Peer peer;

bool exit_flag = false;
bool tracker_login_status = false;
thread tracker_send, tracker_recv;
int tracker_socket;
string peer_ip_address, peer_port;

void catch_ctrl_c(int signal)
{
	char str[MAX_LEN] = "$$ exit";
	// char str[MAX_LEN]=secret_prefix + glbl_data_delimiter + "exit";
	send(tracker_socket, str, sizeof(str), 0);
	exit_flag = true;
	tracker_send.detach();
	tracker_recv.detach();
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
	tracker_socket = client.socketDescriptor;
	client.bindConnection();

	if (tracker.connectAtClient(client.socketDescriptor) != 0)
	{
		cout << "error while connecting to tracker" << endl;
		return -1;
	}

	signal(SIGINT, catch_ctrl_c);

	thread t1(send_to_tracker, tracker_socket);
	thread t2(recv_from_tracker, tracker_socket);

	tracker_send = move(t1);
	tracker_recv = move(t2);

	client.startServerConnection();

	Socket different_peer;

	while (true)
	{
		if (exit_flag)
			break;
		client.acceptConnectionAtServer(&different_peer);
		if (different_peer.socketDescriptor < 0)
		{
			cout << "Failed connection with: " << ntohs(different_peer.socketAddr.sin_port) << endl;
			continue;
		}
		thread t(handle_peer, different_peer.socketDescriptor);

		t.detach();
	}

	if (tracker_send.joinable())
		tracker_send.join();
	if (tracker_recv.joinable())
		tracker_recv.join();

	client.closeConnection();

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

void send_away(int msg, int socket_d)
{
	send(socket_d, &msg, sizeof(msg), 0);
	cerr << "msg sent: " << msg << endl;
}

void send_away(char msg[], int msg_size, int socket_d)
{
	send(socket_d, msg, msg_size, 0);
	cerr << "msg sent: " << msg << endl;
}
void send_away(string &message, int socket_d)
{
	char temp[MAX_LEN];
	strcpy(temp, message.c_str());
	send(socket_d, temp, sizeof(temp), 0);
	cerr << "msg sent: " << temp << endl;
}

void send_to_tracker(int tracker_socket)
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
			send_away(message, tracker_socket);
		}

		// if(strcmp(str,"#exit")==0)
		// {
		// 	exit_flag=true;
		// 	tracker_recv.detach();
		// 	close(tracker_socket);
		// 	return;
		// }
	}
}

// Receive message
void recv_from_tracker(int tracker_socket)
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
				cout << "Download now starting -->> " << endl;
				cerr << "woking it seems" << endl;
				download_manager(tracker_socket, data);
			}
			else
			{
				cout << "download failed" << endl;
				cerr << "download failed due to incorrect string input from tracker" << endl;
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

	// cerr << "below queue " << endl;

	if (input_parts.size() == 0)
		return;

	// cerr << "now action start " << endl;

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

//###########
//Peer-to-Peer connection portion -

// void end_connection(int id)
// {
// 	for (int i = 0; i < all_peers_threads.size(); i++)
// 	{
// 		if (all_peers_threads[i].id == id)
// 		{
// 			lock_guard<mutex> guard(all_peers_threads_mtx);
// 			all_peers_threads[i].th.detach();
// 			all_peers_threads.erase(all_peers_threads.begin() + i);
// 			close(all_peers_threads[i].socket);
// 			break;
// 		}
// 	}
// }

mutex cout_mtx, cerr_mtx;

void error_send(int peer_socket)
{
	string msg = "##error";
	send_away(msg, peer_socket);
}

string get_chunk_hash(char *buffer, int size)
{
	unsigned char hash[SHA_DIGEST_LENGTH]; // == 20

	SHA1((unsigned char *)buffer, size, hash);

	stringstream s;
	for (int i = 0; i < 20; ++i)
		s << hex << setfill('0') << setw(2) << (unsigned short)hash[i];

	// lock_guard<mutex> lgd(cerr_mtx);
	cerr << s.str() << endl;
	return s.str();
}

char *get_file_chunk_buffer(string &filename, long starting_from_pos, int size_of_buffer)
{
	ifstream my_file(filename, ios::in | ios::binary);
	if (!my_file.is_open())
		return nullptr;
	my_file.clear();
	my_file.seekg(starting_from_pos, ios::beg);

	char *buffer = new char[size_of_buffer];

	my_file.read(buffer, size_of_buffer);
	my_file.close();
	return buffer;
}

void send_chunk_response(int bytes_to_read, int peer_socket, char *buffer, string chunk_hash)
{
	string bytes = to_string(bytes_to_read);

	send_away(bytes, peer_socket);

	send_away(buffer, bytes_to_read, peer_socket);

	send_away(chunk_hash, peer_socket);
}

void handle_peer(int peer_socket)
{
	char request[MAX_LEN];
	int bytes_recieved;
	bytes_recieved = recv(peer_socket, request, sizeof(request), 0);

	if (bytes_recieved <= 0)
	{
		error_send(peer_socket);
		return;
	}

	auto input_parts = stringSplit(request, glbl_data_delimiter);

	if (input_parts.size() < 2)
	{
		error_send(peer_socket);
		return;
	}

	string filename = input_parts[0];
	int fileChunkNo = stoi(input_parts[1]);

	if (!peer.hasFile(filename))
	{
		error_send(peer_socket);
		return;
	}

	long file_size = peer.getFileObject(filename).getFileSize();

	//find the total no of chunks of this file
	int last_chunk_size = file_size % chunk_size;
	int total_chunks = (file_size / chunk_size) + ((last_chunk_size) ? 1 : 0);

	int bytes_to_read;

	//check if the requested chunk is the last chunk
	if (fileChunkNo == total_chunks - 1)
	{
		bytes_to_read = last_chunk_size;
	}
	else
	{ //if it is not the last chunk
		bytes_to_read = chunk_size;
	}

	char *buffer = get_file_chunk_buffer(filename, fileChunkNo * chunk_size, bytes_to_read);

	if (!buffer)
	{
		//error condition

		error_send(peer_socket);
	}
	else
	{ //successfully fetched the buffer
		string chunk_hash = get_chunk_hash(buffer, bytes_to_read);

		send_chunk_response(bytes_to_read, peer_socket, buffer, chunk_hash);

		delete[] buffer;
	}

	close(peer_socket);
}

struct peers
{
	int peer_id;
	Peer peer_obj;

	peers(int p_id, Peer peer_obj_) : peer_id(p_id), peer_obj(peer_obj_)
	{
	}
};



// struct fileInfo{
// 	fstream *file_ptr;	//file stream pointer of the file to be downloaded.
// 	vector<bool> chunk_bit_map;		//chunk vector
// 	vector<string> sha_values;		// all sha values
// };

unordered_map<int, vector<thread>> all_peers_threads;

unordered_map<int, vector<peers>> all_peers_ptrs;

unordered_map<int, pair<
					   //    fstream *,	  //file stream pointer of the file to be downloaded.
					   AFile,
					   vector<bool>>> //chunk vector
	file_meta_vector;

mutex all_peers_ptrs_mtx, all_peers_threads_mtx, file_meta_vector_mtx;

void download_handler(int, int);
int seed = 0;

void download_manager(int tracker_socket, vector<string> &input_parts)
{
	seed++;
	AFile afile;
	afile.deserialize(input_parts[2]);
	auto peer_list = stringSplit(input_parts[3], peer_list_delimiter);

	int peer_list_size = peer_list.size();

	int pid = 0;
	for (auto peer_info : peer_list)
	{
		Peer peer;
		peer.deserializeData2(peer_info);
		peers peer_(pid, peer);
		lock_guard<mutex> grd(all_peers_ptrs_mtx);
		all_peers_ptrs[seed].push_back(peer_);
		pid++;
	}
	file_meta_vector[seed].first = afile;
	thread t(download_handler, seed, tracker_socket);
	t.detach();
}

void chunk_download_handler(int id, int chunk_no, int peer_id, int chunk_size);

void download_handler(int id, int tracker_socket)
{
	// unique_lock<mutex> ulk_cerr(cerr_mtx);
	cerr << "inside download handler - " << id << endl;
	// ulk_cerr.unlock();

	// ulk_cerr.lock();
	for (int i = 0; i < all_peers_ptrs[id].size(); i++)
		cerr << all_peers_ptrs[id][i].peer_obj.serializeData2() << peer_list_delimiter;
	cerr << endl;
	// ulk_cerr.unlock();

	int peer_list_size = all_peers_ptrs[id].size();

	string filename = file_meta_vector[id].first.getFileName();
	int file_size = file_meta_vector[id].first.getFileSize();

	int one_chunk_size = chunk_size;
	int last_chunk_size = file_size % one_chunk_size;
	int total_chunks = (file_size / one_chunk_size) + ((last_chunk_size) ? 1 : 0);

	/* fstream file_obj(afile.getFileName(), ios::out | ios::ate | ios::binary);

	// ulk_cerr.lock();
	if (file_obj.is_open())
		cerr << "successfully opened file" << endl;
	else
	{
		// unique_lock<mutex> ulk_cout(cout_mtx);
		cerr << "error while opening file" << endl;
		cout << "error while downloading file." << endl;
		return;
	}
	// ulk_cerr.unlock(); */

	unique_lock<mutex> uck(file_meta_vector_mtx);
	// file_meta_vector[id].first = &file_obj;
	file_meta_vector[id].second.resize(total_chunks, false);
	uck.unlock();

	cerr << "Total chunks for file: " << filename << " are " << total_chunks << " and last chunk size is " << last_chunk_size << endl;

	for (int chunk_no = 0, peer_id = 0; chunk_no < total_chunks; chunk_no++, peer_id = (peer_id + 1) % peer_list_size)
	{
		lock_guard<mutex> grd(all_peers_threads_mtx);
		if (chunk_no == total_chunks - 1 and last_chunk_size)
			one_chunk_size = last_chunk_size;
		thread th(chunk_download_handler, id, chunk_no, peer_id, one_chunk_size);
		lock_guard<mutex> lcd_guard(all_peers_threads_mtx);
		all_peers_threads[id].push_back(move(th));
	}

	// unique_lock<mutex> ucK(all_peers_threads_mtx);
	for (int cnt = 0; cnt < all_peers_threads[id].size(); cnt++)
	{
		if (all_peers_threads[id][cnt].joinable())
			all_peers_threads[id][cnt].join();
	}
	// uck.unlock();

	cerr << "Now all threads are over " << endl;

	bool flag = false;
	vector<int> chunks_failed;
	int chunk_id = 0;
	for (auto chunk_set : file_meta_vector[id].second)
	{
		if (!chunk_set)
		{
			flag = true;
			// break;
			chunks_failed.push_back(chunk_id);
		}
		chunk_id++;
	}

	if (flag)
	{
		//download failed
		//delete the file to be downloaded
		cerr << "Failed to download the chunks:" << endl;
		for (auto i : chunks_failed)
			cerr << i << " ";
		cerr << endl;

		if (remove(filename.c_str()) != 0)
			cerr << "File: " << filename << " not removed." << endl;
		else
			cerr << "file removed";
	}
	else
	{
		//download successfull
		cerr << "Download successful " << endl;
		cout << "Downloading of file: " << filename << " complete." << endl;
	}

	// file_obj.close();
	all_peers_threads.erase(id);
	all_peers_ptrs.erase(id);
	file_meta_vector.erase(id);
}

int calculate_next_peer(int peer_id, int total){
	return (peer_id+1)%(total/2);
}

void safe_log(string s){
	lock_guard<mutex> lck(cerr_mtx);
	cerr << s << endl;
}

void chunk_download_handler(int id, int chunk_no, int peer_id, int chunk_size) 
{
	int fail_safe_total = 2 * all_peers_ptrs[id].size();
	int fail_safe_cnt = 0;
	Peer *peer_;
	string ip_address, port;
	Socket chunk_socket;
	while (fail_safe_cnt < fail_safe_total)
	{
		peer_ = &all_peers_ptrs[id][peer_id].peer_obj;
		ip_address = peer_->getIPAdress();
		port = peer_->getPortNo();
		Socket peer_socket(ip_address, port);

		if(peer_socket.connectAtClient(chunk_socket.socketDescriptor) != 0){
			stringstream ss;
			ss << "failed to connect with peer:" << peer_id << " for chunk:" << chunk_no;
			safe_log(ss.str());
		}
		else{
			
		}

			peer_id = calculate_next_peer(peer_id, fail_safe_total);
		fail_safe_cnt++;
	}
	chunk_socket.closeConnection();
}