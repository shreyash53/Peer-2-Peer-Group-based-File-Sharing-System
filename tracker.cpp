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

char ip_port_delimiter[] = ":";		//for ip and port diffentiation
char afile_data_delimiter[] = "::"; // delimiter for AFile
char peer_data_delimiter[] = "||";	// delimiter for Peer
char glbl_data_delimiter[] = " ";	//global delimiter
char peer_list_delimiter[] = "&&";	//to differentiate b/w many Peer data

char secret_prefix[] = "$$"; //denotes the string is not to be printed since it contains data

char server_prefix[] = "----"; //just for decoration

vector<string> stringSplit(string input, char delim[]);

// vector<string> deserializationCommand(string input, char delim[]);

// typedef pair<string, long> pairs;

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
		cerr << "here in getFileObject" << endl;
		for (auto de : sharedFilesInGroup[groupname])
			cerr << de << ", ";
		cerr << endl;
		if (sharedFilesInGroup[groupname].count(filename))
			return sharedFiles[filename].first;
		cerr << "failed to find file obj for - " << groupname << ", " << filename << endl;
		return AFile();
	}

	void addFile(AFile &afile, string &group_name)
	{
		cerr << "here in addfile" << endl;
		// for(auto de: sharedFilesInGroup[group_name])
		// 	cerr << de << ", ";
		// 	cerr << endl;
		if (sharedFilesInGroup[group_name].count(afile.getFileName()))
			return;
		// cerr << "now adding.." << endl;
		sharedFilesInGroup[group_name].insert(afile.getFileName());
		if (sharedFiles.count(afile.getFileName()))
			sharedFiles[afile.getFileName()].second++;
		else
			sharedFiles[afile.getFileName()] = make_pair(afile, 1);
		for (auto de : sharedFilesInGroup[group_name])
			cerr << de << ", ";
		cerr << endl;
		cerr << "now other" << endl;
		for (auto de : sharedFiles)
			cerr << de.first << " " << de.second.first.serializeData() << de.second.second << " => ";
		cerr << endl;
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
		flag = true;
		for (auto fl : sharedFilesInGroup)
		{
			msg += fl.first + ", ";
			flag = false;
		}
		if (flag)
			msg += "No Group\n";
		return msg;
	}
};

class Group
{
private:
	string groupName;
	string groupAdmin;
	unordered_map<string, Peer> groupMembers;				  //key == peer name and value = peer object
	unordered_map<string, unordered_set<string>> sharedFiles; //key = file name and value = set of strings peer name

	void deleteAllSharedFilesByPeer(Peer &peer)
	{

		for (auto filename : peer.getFileListByGroup(groupName))
		{
			if (sharedFiles.count(filename) and sharedFiles[filename].count(peer.getName()))
			{
				if (sharedFiles[filename].size() < 2)
				{
					sharedFiles.erase(filename);
				}
				else
					sharedFiles[filename].erase(peer.getName());
			}
		}
	}

public:
	Group() {}
	Group(string &groupName_, string &groupAdmin_) : groupName(groupName_), groupAdmin(groupAdmin_)
	{
	}

	bool isAdmin(string &name)
	{
		return groupAdmin == name;
	}

	string serialize()
	{
		string res = groupName + "=" + groupAdmin + " -> \n";
		res += "members: ";
		for (auto gm : groupMembers)
			res += gm.second.serializeData3() + " ";
		res += "\nfiles: ";
		for (auto sf : sharedFiles)
			res += sf.first + ", ";
		return res;
	}

	unordered_set<string> getSetOfPeerNamesByFile(string &filename)
	{
		return sharedFiles[filename];
	}

	int getNoOfPeersWithFile(string &filename)
	{
		if (sharedFiles.count(filename))
			return sharedFiles[filename].size();
		return -1;
	}

	bool addGroupMember(Peer &peer)
	{
		// for (auto member = groupMembers.begin(); member != groupMembers.end(); ++member)
		// {
		// 	if (member->getName() == peer.getName())
		// 	{
		// 		return false;
		// 	}
		// }
		if (groupAdmin == "")
			groupAdmin = peer.getName();
		// groupMembers.push_back(peer);
		if (groupMembers.count(peer.getName()))
			return false;
		groupMembers[peer.getName()] = peer;
		return true;
	}

	bool addSharedFile(string &filename, Peer &owner)
	{
		if (sharedFiles[filename].count(owner.getName()))
			return false;
		sharedFiles[filename].insert(owner.getName());
		return true;
	}

	bool isMember(string &peer_name)
	{
		// for (auto peer_ : groupMembers)
		// 	if (peer_.getName() == peer_name)
		// 		return true;
		if (groupMembers.count(peer_name))
			return true;
		return false;
	}

	bool deleteSharedFile(string &filename, Peer &peer)
	{
		if (filePresent(filename))
		{
			cerr << "in delete shared file : file found" << endl;

			if (sharedFiles[filename].count(peer.getName()))
			{
				sharedFiles[filename].erase(peer.getName());
				if (sharedFiles[filename].empty())
					sharedFiles.erase(filename);
				return true;
			}
		}
		return false;
	}

	string getShareableFilesNames()
	{
		string msg = "";
		for (auto file : sharedFiles)
			msg += file.first + '\n';
		return msg;
	}

	bool filePresent(string &file_name)
	{
		return sharedFiles.count(file_name);
	}

	string getAllPeersForFileSerialized(string &filename)
	{
		string firstPart = groupName;
		string secondPart = "";
		int i = 0;
		string thirdPart = "";
		int n = sharedFiles[filename].size();
		cerr << "inside all peers for file : " << endl;
		for (auto peer : sharedFiles[filename])
		{
			cerr << "peer - " << peer << endl;
			if (i == 0)
				secondPart = groupMembers[peer].getFileObject(groupName, filename).serializeData();
			thirdPart += groupMembers[peer].serializeData1();
			if (i + 1 < n)
				thirdPart += peer_list_delimiter;
			i++;
		}
		return firstPart + glbl_data_delimiter + secondPart + glbl_data_delimiter + thirdPart;
	}

	int getNoOfMembers()
	{
		return groupMembers.size();
	}

	bool deleteGroupMember(Peer &peer)
	{
		// for (auto member = groupMembers.begin(); member != groupMembers.end(); ++member)
		// {
		// 	if (member->getName() == peer.getName())
		// 	{

		if (groupMembers.count(peer.getName()))
		{
			if (peer.getName() == groupAdmin)
			{
				if (groupMembers.size() == 1)
				{
					groupAdmin = "";
				}
				else
				{
					for (auto peer_ : groupMembers)
					{
						if (peer_.first != peer.getName())
						{
							groupAdmin = peer_.first;
							break;
						}
					}
				}
			}
			deleteAllSharedFilesByPeer(peer);
			groupMembers.erase(peer.getName());

			return true;
		}
		return false;
	}
};

struct terminal
{
	int id;
	string peer_name;
	int socket;
	thread th;
};

unordered_map<string, Peer> all_peers;
unordered_map<string, Group> all_groups;
unordered_map<string, unordered_set<string>> all_requests; //key = group name and value = set of peer names

vector<terminal> all_peers_terminals;

int seed = 0;
mutex cout_mtx, all_peers_mtx, all_peers_terminals_mtx, all_requests_mtx, all_groups_mtx, peer_thread_mtx;

// void set_name(int id, char name[]);
// void broadcast_message(string message, int sender_id);
// void broadcast_message(int num, int sender_id);
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
			exit(-1);
		}
		if ((listen(server_socket, 8)) == -1)
		{
			perror("listen error: ");
			exit(-1);
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
		return 1;
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

int main(int argc, char **argv)
{
	string ip_address, port;

	if (argc > 1)
		get_tracker_ip_and_port(argv[1], ip_address, port);
	else
		get_tracker_ip_and_port("tracker.txt", ip_address, port);

	Socket tracker(ip_address, port);

	tracker.startServerConnection();

	while (true)
	{
		int client_socket = tracker.acceptConnectionAtServer();
		if (client_socket < 0)
		{
			cout << "failure in connection" << endl;
			continue;
		}
		seed++;
		thread t(handle_client, client_socket, seed);
		lock_guard<mutex> guard(all_peers_terminals_mtx);
		all_peers_terminals.push_back({seed, string("Anonymous"), client_socket, (move(t))});
		// cout << "done with here" << endl;
	}
	for (int i = 0; i < all_peers_terminals.size(); i++)
	{
		if (all_peers_terminals[i].th.joinable())
			all_peers_terminals[i].th.join();
	}

	tracker.closeConnection();
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

void send_message(string message, const terminal *const peerThreadObj)
{
	char temp[MAX_LEN];
	strcpy(temp, message.c_str());
	send(peerThreadObj->socket, temp, sizeof(temp), 0);
}

void send_message(int num, const terminal *const peerThreadObj)
{
	send(peerThreadObj->socket, &num, sizeof(num), 0);
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

void failure_case(const terminal *const peerThreadObject)
{
	send_message("Operation Unsucessful. Try again with correct input, Also ensure you are logged in.", peerThreadObject);
}

void failure_case(const terminal *const peerThreadObject, vector<string> &input_in_parts)
{
	send_message("Operation Unsucessful. Try again with correct input, Also ensure you are logged in.", peerThreadObject);
	for (auto inp : input_in_parts)
		cerr << inp << " ";
	cerr << endl;
}

bool groupPresent(string &gname)
{
	return all_groups.count(gname);
}

bool isMember(string &group_name, string &peer_name)
{
	return all_groups[group_name].isMember(peer_name);
}

bool userPresent(string &uname)
{
	return all_peers.count(uname);
}

bool test_condition(const terminal *const peerThreadObj, vector<string> &input_in_parts, int n)
{
	if (input_in_parts.size() == n)
		return true;

	failure_case(peerThreadObj, input_in_parts);
	return false;
}

void create_user(terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 2))
	{
		Peer newPear;
		if (newPear.deserializeData1(input_in_parts[1]))
		{

			unique_lock<mutex> lck(all_peers_mtx);
			all_peers[newPear.getName()] = newPear;

			lck.unlock();

			// unique_lock<mutex> ulck(peer_thread_mtx);
			peerThreadObj->peer_name = newPear.getName();
			// ulck.unlock();
			send_message("User successfully created.", peerThreadObj);
		}
		else
		{
			send_message("Incorrect input.", peerThreadObj);
			cerr << "*** " << input_in_parts[1] << endl;
		}
	}
}

void login(const terminal *const peerThreadObj, vector<string> &input_in_parts, bool &is_login_success)
{
	string pname = peerThreadObj->peer_name;

	if (input_in_parts.size() == 1)
		failure_case(peerThreadObj, input_in_parts);
	else if (!userPresent(pname))
	{
		send_message("You are signed up", peerThreadObj);
	}
	else
	{
		unique_lock<mutex> ulck(all_peers_mtx);
		if (all_peers[pname].loginCondition(input_in_parts[1]))
		{
			ulck.unlock();
			send_message("## You are successfully Logged In in the network.", peerThreadObj);
			is_login_success = true;
			send_message(input_in_parts[0] + glbl_data_delimiter + "1", peerThreadObj);
		}
		else
		{
			send_message("Incorrect credentials or already logged in.", peerThreadObj);
			cerr << "Failed to log in " << input_in_parts[1] << endl;
		}
	}
}

void join_group(const terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 2))
	{
		string pname = peerThreadObj->peer_name;
		string gname = input_in_parts[1];
		string msg;
		if (groupPresent(gname))
		{
			lock_guard<mutex> lck(all_groups_mtx);
			all_requests[gname].insert(pname);
			msg = "Request Recieved. Wait for approval.";
		}
		else
			msg = "No such group";
		send_message(msg, peerThreadObj);
	}
}

void create_group(const terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 2))
	{
		string pname = peerThreadObj->peer_name;
		string gname = input_in_parts[1];
		string msg;
		bool responseFlag = false;
		if (!groupPresent(gname))
		{
			Group newGroup(gname, pname);
			if (newGroup.addGroupMember(all_peers[pname]))
			{
				unique_lock<mutex> lck(all_groups_mtx);
				all_groups[gname] = newGroup;
				lck.unlock();
				lock_guard<mutex> grd(all_peers_mtx);
				all_peers[pname].addGroup(gname);
				msg = "## Group successfully created with you as admin.";
				responseFlag = true;
			}
			else
				msg = "You are already a member. Cannot join again";
		}
		else
			msg = "Group with this name already present";
		send_message(msg, peerThreadObj);
		if (responseFlag)
			send_message(input_in_parts[0] + glbl_data_delimiter + "1", peerThreadObj);
	}
}

void leave_group(const terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 2))
	{
		string pname = peerThreadObj->peer_name;
		string gname = input_in_parts[1];
		string msg;
		bool flag = true, responseFlag = false;
		if (groupPresent(gname))
		{
			cerr << "here in leave_group" << endl;
			if (all_groups[gname].getNoOfMembers() == 1)
			{
				unique_lock<mutex> ulck(all_groups_mtx);

				all_groups.erase(gname);
				ulck.unlock();

				lock_guard<mutex> lck(all_peers_mtx);
				all_peers[pname].deleteGroup(gname);
				cerr << "inside first condition in leave_group: after deletion" << endl;
			}
			else if (all_groups[gname].getNoOfMembers() > 1)
			{
				unique_lock<mutex> lck(all_groups_mtx);
				if (!all_groups[gname].deleteGroupMember(all_peers[pname]))
				{
					msg = "You are not part of the group.";
					flag = false;
					cerr << "here in 2.a in leave_group" << endl;
				}
				else
				{
					lck.unlock();
					lock_guard<mutex> lgd(all_peers_mtx);
					all_peers[pname].deleteGroup(gname);
					cerr << "2.b : deleted successfully group member" << endl;
				}
			}
			else
				cerr << "third condition in leave_group" << endl;

			if (flag)
			{
				msg = "## You are no longer in the group. All files shared by you have been removed.";
				responseFlag = true;
			}
		}
		else
			msg = "No Such Group";
		send_message(msg, peerThreadObj);
		if (responseFlag)
			send_message(input_in_parts[0] + glbl_data_delimiter + "1", peerThreadObj);
	}
}

void requests_list_requests(const terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 3))
	{
		string pname = peerThreadObj->peer_name;
		string query = input_in_parts[1];
		string gname = input_in_parts[2];
		string msg;

		if (query == "list_requests" and groupPresent(gname))
		{
			// lock_guard<mutex> lck(all_requests_mtx);
			if (all_requests.count(gname) and !all_requests[gname].empty())
			{
				msg = "All peers requesting to join are:\n";
				for (auto member : all_requests[gname])
				{
					msg += member + "\n";
				}
			}
			else
				msg = "No new Request for this group";
		}
		else
			msg = "Invalid Group Id";
		send_message(msg, peerThreadObj);
	}
}

void accept_group_request(const terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 3))
	{
		string pname = peerThreadObj->peer_name;
		string gname = input_in_parts[1];
		string uname = input_in_parts[2];
		string msg;

		if (userPresent(uname) and groupPresent(gname) and all_groups[gname].isAdmin(pname))
		{
			bool flag = true;

			if (all_requests[gname].count(uname))
			{
				flag = false;
				unique_lock<mutex> ulck(all_requests_mtx);
				all_requests[gname].erase(uname);
				if (all_requests[gname].empty())
					all_requests.erase(gname);
				cerr << "element deleted from requests" << endl;
				ulck.unlock();
				unique_lock<mutex> grd(all_groups_mtx);
				if (all_groups[gname].addGroupMember(all_peers[uname]))
					msg = "Added Successfully.";
				else
					msg = "Already Added.";

				grd.unlock();
				lock_guard<mutex> lgd(all_peers_mtx);
				all_peers[uname].addGroup(gname);
			}

			if (flag)
				msg = "No request found for given user";
		}
		else
			msg = "Invalid Group/User Id or You don\'t have access to accept request.";
		send_message(msg, peerThreadObj);
	}
}

string download_file_helper_serializer(string &groupName, string &filename)
{
	string firstPart = groupName;
	string secondPart = "";
	int i = 0;
	string thirdPart = "";
	unique_lock<mutex> ulc(all_groups_mtx);
	int n = all_groups[groupName].getNoOfPeersWithFile(filename);
	if (n == -1)
		return "";
	cerr << "inside all peers for file : " << endl;
	auto plist = all_groups[groupName].getSetOfPeerNamesByFile(filename);
	ulc.unlock();
	lock_guard<mutex> grd(all_peers_mtx);
	for (auto peer : plist)
	{
		cerr << "peer - " << peer << endl;
		if (i == 0)
			secondPart = all_peers[peer].getFileObject(groupName, filename).serializeData();
		thirdPart += all_peers[peer].serializeData2();
		if (i + 1 < n)
			thirdPart += peer_list_delimiter;
		i++;
	}
	return firstPart + glbl_data_delimiter + secondPart + glbl_data_delimiter + thirdPart;
}

void download_file(const terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 3))
	{
		string pname = peerThreadObj->peer_name;
		string gname = input_in_parts[1];
		string file_name = input_in_parts[2];
		string msg;
		unique_lock<mutex> grd(all_groups_mtx);
		if (groupPresent(gname) and isMember(gname, pname))
		{
			if (all_groups[gname].filePresent(file_name))
			{
				grd.unlock();
				cerr << "getting file for download:" << endl;
				msg = download_file_helper_serializer(gname, file_name);
				cerr << msg << endl;
				if (msg.length())
				{
					msg = glbl_data_delimiter + msg;
					msg = secret_prefix + msg;
				}
				else
				{
					msg = "error while finding the file.";
				}
			}
			else
				msg = "No such file available.";
		}
		else
			msg = "Invalid Group Id or you don\'t have permission to see its files";
		send_message(msg, peerThreadObj);
	}
}

void all_groups_list(const terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 1))
	{
		string msg;
		unique_lock<mutex> ulck(all_groups_mtx);
		if (all_groups.empty())
			msg = "No group in then netwok currently";
		else
		{
			msg = "These are the groups currently in the network:\n";
			for (auto gps : all_groups)
				msg += gps.first + '\n';
		}
		send_message(msg, peerThreadObj);
	}
}

void all_shareable_files_in_group(const terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 2))
	{
		string pname = peerThreadObj->peer_name;
		cerr << pname << " : " << endl;
		string gname = input_in_parts[1];
		string msg;
		lock_guard<mutex> grd(all_groups_mtx);
		bool responseFlag = false;
		if (groupPresent(gname) and isMember(gname, pname))
		{
			string files = all_groups[gname].getShareableFilesNames();
			if (files.length())
				msg = "## The file list is - \n" + files;
			else
				msg = "## No file shared here currently.";
			responseFlag = true;
		}
		else
			msg = "Invalid Group Id or you don\'t have permission to see its files";
		send_message(msg, peerThreadObj);
		if (responseFlag)
			send_message(input_in_parts[0] + glbl_data_delimiter + "1", peerThreadObj);
	}
}

void upload_file(const terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 3))
	{
		string pname = peerThreadObj->peer_name;
		string gname = input_in_parts[1];
		string msg;

		bool responseFlag = false;
		if (groupPresent(gname) and isMember(gname, pname))
		{
			AFile afile;
			unique_lock<mutex> ulck(all_peers_mtx);
			if (afile.deserialize(input_in_parts[2]))
			{
				all_peers[pname].addFile(afile, gname);
				ulck.unlock();
				lock_guard<mutex> grd(all_groups_mtx);
				string filename = afile.getFileName();
				if (all_groups[gname].addSharedFile(filename, all_peers[pname]))
				{
					msg = "## Successfully Uploaded.";
					responseFlag = true;
				}
				else
					msg = "This file has been already shared by you for this group.";
			}
			else
				msg = "upload failure, failed to deserialize";
		}
		else
			msg = "Invalid Group Id or you don\'t have permission to see its files";
		send_message(msg, peerThreadObj);
		if (responseFlag)
			send_message(input_in_parts[0] + glbl_data_delimiter + "1", peerThreadObj);
	}
}

void logout(const terminal *const peerThreadObj, vector<string> &input_in_parts, bool &is_login_success)
{
	if (test_condition(peerThreadObj, input_in_parts, 1))
	{
		string pname = peerThreadObj->peer_name;
		string msg;
		unique_lock<mutex> ulck(all_peers_mtx);
		all_peers[pname].logout();
		ulck.unlock();
		msg = "## You have been logged out.";
		send_message(msg, peerThreadObj);
		is_login_success = false;

		send_message(input_in_parts[0] + glbl_data_delimiter + "1", peerThreadObj);
	}
}

void stop_sharing(const terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 3))
	{
		string pname = peerThreadObj->peer_name;
		string gname = input_in_parts[1];
		string filename = input_in_parts[2];
		string msg;
		bool responseFlag = false;
		if (groupPresent(gname) and isMember(gname, pname))
		{
			unique_lock<mutex> ulck(all_groups_mtx);
			if (all_groups[gname].deleteSharedFile(filename, all_peers[pname]))
			{
				ulck.unlock();
				lock_guard<mutex> grd(all_peers_mtx);
				all_peers[pname].deleteSharedFile(gname, filename);

				msg = "## File is no longer shared in that group";
				responseFlag = true;
			}
			else
				msg = "File not listed as shared in that group";
		}
		else
			msg = "invalid group id or you don\'t have permission to see its files";
		send_message(msg, peerThreadObj);
		if (responseFlag)
			send_message(input_in_parts[0] + glbl_data_delimiter + "1", peerThreadObj);
	}
}

void debug();

void handle_client(int client_socket, int id)
{
	char name[MAX_LEN], str[MAX_LEN];

	terminal *peerThreadObj;

	cout << server_prefix << glbl_data_delimiter << id << " just joined." << endl;
	bool is_login_success = false;
	string inputCommand;
	// string peer_name;

	while (true)
	{
		// cout << "now waiting" << endl;
		int bytes_received = recv(client_socket, str, sizeof(str), 0);
		cerr << server_prefix << glbl_data_delimiter << str << endl;
		if (bytes_received < 0)
		{
			continue;
		}
		unique_lock<mutex> grd(peer_thread_mtx);
		for (int i = 0; i < all_peers_terminals.size(); i++)
		{
			if (all_peers_terminals[i].id == id)
			{
				peerThreadObj = &all_peers_terminals[i];
				break;
			}
		}
		grd.unlock();

		auto input_in_parts = stringSplit(str, glbl_data_delimiter);

		if (input_in_parts.empty())
		{
			cout << "***input error***" << endl;
			continue;
		}

		cerr << id << " : " << peerThreadObj->peer_name << " -> ";
		inputCommand = input_in_parts[0];

		if (inputCommand == secret_prefix)
		{
			if (input_in_parts[1] == "exit")
			{
				cout << server_prefix << glbl_data_delimiter << peerThreadObj->peer_name << " has ended connection" << endl;
				end_connection(id);
				return;
			}
		}
		else
		{
			if (inputCommand == "login")
			{
				login(peerThreadObj, input_in_parts, is_login_success);
				// cout << "login: " << is_login_success << endl;
				cerr << "inside login - " << endl;
			}
			else if (inputCommand == "create_user")
			{
				create_user(peerThreadObj, input_in_parts);
				cerr << "inside create user" << endl;
			}

			else if (is_login_success)
			{
				if (inputCommand == "join_group")
				{
					join_group(peerThreadObj, input_in_parts);
					cerr << "inside join group" << endl;
				}

				else if (inputCommand == "create_group")
				{
					create_group(peerThreadObj, input_in_parts);
					cerr << "inside create group" << endl;
				}

				else if (inputCommand == "leave_group")
				{
					leave_group(peerThreadObj, input_in_parts);
					cerr << "inside leave group" << endl;
				}

				else if (inputCommand == "requests")
				{
					requests_list_requests(peerThreadObj, input_in_parts);
					cerr << "inside requests" << endl;
				}

				else if (inputCommand == "accept_request")
				{
					accept_group_request(peerThreadObj, input_in_parts);
					cerr << "inside accept request" << endl;
				}

				else if (inputCommand == "list_groups")
				{
					all_groups_list(peerThreadObj, input_in_parts);
					cerr << "inside list group" << endl;
				}

				else if (inputCommand == "list_files")
				{
					all_shareable_files_in_group(peerThreadObj, input_in_parts);
					cerr << "inside list files" << endl;
				}
				else if (inputCommand == "download_file")
				{
					download_file(peerThreadObj, input_in_parts);
					cerr << "inside download" << endl;
				}

				else if (inputCommand == "upload_file")
				{
					upload_file(peerThreadObj, input_in_parts);
					cerr << "inside upload" << endl;
				}

				else if (inputCommand == "logout")
				{
					logout(peerThreadObj, input_in_parts, is_login_success);
					cerr << "inside logout" << endl;
				}

				else if (inputCommand == "stop_share")
				{
					stop_sharing(peerThreadObj, input_in_parts);
					cerr << "inside stop sharing" << endl;
				}
			}
			else
			{
				failure_case(peerThreadObj, input_in_parts);
			}
		}
		debug();
	}
}

void debug()
{
	// all_peers
	cerr << "all peers" << endl;
	for (auto pe : all_peers)
	{
		cerr << pe.second.dumps() << "\t";
	}
	cerr << endl;
	// all_groups
	cerr << "all groups" << endl;
	for (auto grp : all_groups)
		cerr << grp.second.serialize() << "  ";

	cerr << endl;

	// all_requests
	cerr << "all requests" << endl;
	for (auto req : all_requests)
	{
		cerr << req.first << ":- ";
		for (auto pe : req.second)
		{
			cerr << all_peers[pe].serializeData3() << " | ";
		}
		cerr << endl;
	}
	cerr << endl;
}