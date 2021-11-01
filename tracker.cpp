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

int chunk_size = 1024 * 512; //512 KB

using namespace std;

char delim1[] = ":";
char delim2[] = "||";
char delim3[] = " ";

vector<string> stringSplit(string input, char delim[]);

// vector<string> deserializationCommand(string input, char delim[]);

class AFile
{
private:
	string fileName;
	float fileSize;

public:
	AFile() {}

	AFile(string &fileName_, float &fileSize_) : fileName(fileName_), fileSize(fileSize_)
	{
	}

	void setAFile(string &fileName_, float &fileSize_){
		fileName = fileName_;
		fileSize = fileSize_;
	}

	string getFileName()
	{
		return fileName;
	}

	string serializeData()
	{
		return fileName + ":" + to_string(fileSize);
	}

	void deserialize(string data)
	{
		auto parts = stringSplit(data, delim1);
		setAFile(parts[0], parts[1]);
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

	void operator=(const Peer& p1){
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
		for (auto fileList : sharedFilesInGroup[groupname])
		{
			for (auto member = fileList.begin(); member != fileList.end(); ++member)
			{
				if (member->getFileName() == filename))
					{
						fileList.erase(member);
						break;
					}
			}
		}
	}

	void setIsOnline(bool status)
	{
		isOnline = status;
	}
	bool getIsOnline()
	{
		return isOnline;
	}
	string getName()
	{
		return name;
	}
	bool addFile(string &filename, float filesize)
	{
		if (sharedFiles.count(filename) == 0)
			return false;
		AFile afile(filename, filesize);
		addFile(afile);
		sharedFiles.insert(make_pair(afile.getFileName(), afile));
		return true;
	}
	// void addFile(AFile &afile)
	// {
	// }

	bool loginCondition(string& input_string){
		auto parts = stringSplit(data, delim2);
		if(parts.size() == 2 and name == parts[0] and pword == parts[1]){
			setIsOnline(true);
			return true;
		}
		return false;
	}

	string serializeData1()
	{
		return name + "||" + pword + "||" + ipAddress + "||" + port_no;
	}

	string serializeData2()
	{
		return name + "||" + ipAddress + "||" + port_no;
	}

	void setPeer(string &name_, string &pword_){
		name = name_;
		pword = pword_;
	}

	void setPeer(string &name_, string &ipAddress_, string &port_no_){
		name = name_;
		ipAddress = ipAddress_;
		port_no = port_no_;
	}

	void setPeer(string &name_, string &pword_, string &ipAddress_, string &port_no_){
		setPeer(name_, pword_);
		setPeer(name_, ipAddress_, port_no_);
	}

	bool deserializeData1(string data)
	{
		auto parts = stringSplit(data, delim2);
		if(parts.size() == 4){
		setPeer(parts[0], parts[1], parts[2], parts[3]);
			return true;
		}
		return false;
	}

	bool deserializeData2(string data)
	{
		auto parts = stringSplit(data, delim2);
		if(parts.size() == 3){
		setPeer(parts[0], parts[1], parts[2]);
			return true;
		}
		return false;
	}

	string serializeData3()
	{
		return name + "||" + pword;
	}

	bool deserializeData3(string data)
	{
		auto parts = stringSplit(data, delim2);
		if(parts.size() == 2){
		setPeer(parts[0], parts[1]);
			return true;
		}
		return false;

	}
};

class Group
{
private:
	string groupName;
	string groupAdmin;
	vector<Peer> groupMembers;
	unordered_map<string, vector<Peer>> sharedFiles;

	void deleteAllSharedFilesByPeer(Peer &peer)
	{
		for (auto plist : sharedFiles)
		{
			for (auto member = plist.begin(); member != plist.end(); ++member)
			{
				if (member->getName() == peer.getName())
				{
					plist.erase(member);
					break;
				}
			}
		}
	}

public:
	Group(string &groupName_, string &groupAdmin_) : groupName(groupName_), groupAdmin(groupAdmin_)
	{
	}

	bool addGroupMember(Peer &peer)
	{
		for (auto member = groupMembers.begin(); member != groupMembers.end(); ++member)
		{
			if (member->getName() == peer.getName())
			{
				return false;
			}
		}
		groupMembers.push_back(member);
		return true;
	}

	bool addSharedFile(string &filename, Peer &owner)
	{
		auto plist = sharedFiles[filename];
		if (!plist.empty())
		{
			for (auto member = plist.begin(); member != plist.end(); ++member)
			{
				if (member->getName() == peer.getName())
				{
					return false;
				}
			}
		}
		plist.push_back(owner);
		return true;
	}

	bool deleteSharedFile(string &filename, Peer &peer)
	{
		if (sharedFiles.count(filename))
		{
			auto plist = sharedFiles[filename];
			for (auto member = plist.begin(); member != plist.end(); ++member)
			{
				if (member->getName() == peer.getName())
				{
					plist.erase(member);
					peer.deleteSharedFile(groupName, filename);
					return true;
				}
			}
		}
		return false;
	}

	bool deleteGroupMember(Peer &peer)
	{
		for (auto member = groupMembers.begin(); member != groupMembers.end(); ++member)
		{
			if (member->getName() == peer.getName())
			{
				groupMembers.erase(member);
				deleteAllSharedFilesByPeer(peer);
				peer.deleteGroup(groupName);
				return true;
			}
		}
		return false;
	}
};

unordered_map<string, Peer> all_peers;
unordered_map<string, Group> all_groups;
unordered_map<string, vector<Peer>> all_requests;

struct terminal
{
	int id;
	string peer_name;
	int socket;
	thread th;

	void operator=(const terminal &t1)
	{
		id = t1.id;
		peer_name = t1.peer_name;
		socket = t1.socket;
		th = (move(t1.th));
	}
};

vector<terminal> all_peers_terminals;

int seed = 0;
mutex cout_mtx, all_peers_mtx, all_peers_terminals_mtx;

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

	~Socket(){
		close(server_socket);
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

	void closeConnection()
	{
		// close(server_socket);
	}
};

// class DeserializationMethods{
// 	public:
// 	static
// };

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
			auto entries = stringSplit(tp, delim1);
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
		if (client_socket < 0)
			continue;
		seed++;
		thread t(handle_client, client_socket, seed);
		lock_guard<mutex> guard(all_peers_terminals_mtx);
		all_peers_terminals.push_back({seed, string("Anonymous"), client_socket, (move(t))});
	}
	for (int i = 0; i < all_peers_terminals.size(); i++)
	{
		if (all_peers_terminals[i].th.joinable())
			all_peers_terminals[i].th.join();
	}

	tracker.close();
	return 0;
}

// For synchronisation of cout statements
void shared_print(string str, bool endLine = true)
{
	lock_guard<mutex> guard(cout_mtx);
	cout << str;
	if (endLine)
		cout << endl;
}

void send_message(string message, terminal &peerThreadObj)
{
	char temp[MAX_LEN];
	strcpy(temp, message.c_str());
	send(peerThreadObj.socket, temp, sizeof(temp), 0);
}

void send_message(int num, terminal &peerThreadObj)
{
	send(peerThreadObj.socket, &num, sizeof(num), 0);
}

void end_connection(int id)
{
	for (int i = 0; i < all_peers_terminals.size(); i++)
	{
		if (all_peers_terminals[i].id == id)
		{
			lock_guard<mutex> guard(all_peers_terminals_mtx);
			all_peers_terminals[i].th.detach();
			all_peers_terminals.erase(all_peers_terminals.begin() + i);
			close(all_peers_terminals[i].socket);
			break;
		}
	}
}

void failure_case(terminal &peerThreadObject)
{
	send_message("Operation Unsucessful. Try again with correct input", peerThreadObject);
}

void failure_case(terminal &peerThreadObject, vector<string>& input_in_parts)
{
	send_message("Operation Unsucessful. Try again with correct input", peerThreadObject);
	for(auto inp: input_in_parts)
		cerr << inp << " " ;
		cerr << endl;
}

bool create_user(terminal& peerThreadObj, vector<string>& input_in_parts)
{
	if(input_in_parts.size() == 1)
		failure_case(peerThreadObj, input_in_parts);
	else{
		Peer newPear;
		if(newPear.deserializeData1(input_in_parts[1])){

			unique_lock<mutex> lck(all_peers_mtx);
			all_peers[newPear.getName()] = newPear;

			lck.unlock();

			lock_guard<mutex> guard(all_peers_terminals_mtx);
			peerThreadObj.peer_name = newPear.getName();
		}
		else{
			send_message("Incorrect input.", peerThreadObj);
			cerr << "*** " << input_in_parts[1] << endl;
		}
	}
}

void login(terminal &peerThreadObj, vector<string> &input_in_parts, bool& is_login_success)
{
	string pname = peerThreadObj.peer_name;
	if (all_peers.count(pname) == 0 or input_in_parts.size() == 1)
		failure_case(peerThreadObj, input_in_parts);
	else{
		if(all_peers[pname].loginCondition(input_in_parts[1])){
			send_message("## You are successfully Logged In in the network.", peerThreadObj);
			is_login_success = true;
		send_message(1, peerThreadObj);
		}
		else{
			send_message("Incorrect input.", peerThreadObj);
			cerr << "*** " << input_in_parts[1] << endl;
		}
			
	}
}



void handle_client(int client_socket, int id)
{
	char name[MAX_LEN], str[MAX_LEN];
	// recv(client_socket, name, sizeof(name), 0);
	// set_name(id, name);

	// // Display welcome message
	// string welcome_message = string(name) + string(" has joined");
	// broadcast_message("#NULL", id);
	// broadcast_message(id, id);
	// broadcast_message(welcome_message, id);
	// shared_print(color(id) + welcome_message + def_col);

	terminal peerThreadObj;
	for (int i = 0; i < all_peers_terminals.size(); i++)
	{
		if (all_peers_terminals[i].id == id)
		{
			peerThreadObj = all_peers_terminals[i];
			break;
		}
	}

	bool is_login_success = false;
	string inputCommand;

	while (true)
	{
		int bytes_received = recv(client_socket, str, sizeof(str), 0);
		if (bytes_received <= 0)
			return;
		// if (strcmp(str, "#exit") == 0)
		// {
		// 	// Display leaving message
		// 	string message = string(name) + string(" has left");
		// 	broadcast_message("#NULL", id);
		// 	broadcast_message(id, id);
		// 	broadcast_message(message, id);
		// 	shared_print(color(id) + message + def_col);
		// 	end_connection(id);
		// 	return;
		// }
		// broadcast_message(string(name), id);
		// broadcast_message(id, id);
		// broadcast_message(string(str), id);
		// shared_print(color(id) + name + " : " + def_col + str);

		// input = str;

		auto input_in_parts = stringSplit(str, delim3);

		if (input_in_parts.empty())
		{
			cout << "***input error***" << endl;
			continue;
		}

		inputCommand = input_in_parts[0];

		if (inputCommand == "login")
		{
			login(peerThreadObj, input_in_parts, is_login_success);
			cout << "login: " << is_login_success << endl;
		}
		else if (inputCommand == "create_user")
		{
			create_user(peerThreadObj, input_in_parts);
		}

		else if (is_login_success)
		{
			if (inputCommand == "join_group")
			{

			}

			else if (inputCommand == "create_group")
			{

			}

			else if (inputCommand == "leave_group")
			{
			}

			else if (inputCommand == "requests")
			{
			}

			else if (inputCommand == "accept_request")
			{
			}

			else if (inputCommand == "list_groups")
			{
			}

			else if (inputCommand == "list_files")
			{
			}

			else if (inputCommand == "upload_file")
			{
			}

			else if (inputCommand == "download_file")
			{
			}

			else if (inputCommand == "logout")
			{
			}

			else if (inputCommand == "show_downloads")
			{
			}

			else if (inputCommand == "stop_share")
			{
			}
		}
	}
}