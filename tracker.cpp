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

char secret_prefix[] = "$$ "; //denotes the string is not to be printed since it contains data

vector<string> stringSplit(string input, char delim[]);

// vector<string> deserializationCommand(string input, char delim[]);

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
				if (fileList.empty())
					sharedFilesInGroup.erase(groupname);
				break;
			}
		}
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
		for (auto afile : sharedFilesInGroup[groupname])
			if (afile.getFileName() == filename)
				return afile;
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

class Group
{
private:
	string groupName;
	string groupAdmin;
	vector<Peer> groupMembers;
	unordered_map<string, vector<Peer>> sharedFiles;

	void deleteAllSharedFilesByPeer(Peer &peer)
	{
		for (auto fList : peer.getFileListByGroup(groupName))
		{
			auto plist = sharedFiles[fList.getFileName()];
			for (auto member = plist.begin(); member != plist.end(); ++member)
			{
				if (member->getName() == peer.getName())
				{
					plist.erase(member);
					break;
				}
			}
		}
		peer.resetAllSharedFiles();
	}

public:
	Group(string &groupName_, string &groupAdmin_) : groupName(groupName_), groupAdmin(groupAdmin_)
	{
	}

	bool isAdmin(string &name)
	{
		return groupAdmin == name;
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
		if (groupAdmin == "")
			groupAdmin = peer.getName();
		groupMembers.push_back(peer);
		return true;
	}

	bool addSharedFile(string &filename, Peer &owner)
	{
		auto plist = sharedFiles[filename];
		if (!plist.empty())
		{
			for (auto member = plist.begin(); member != plist.end(); ++member)
			{
				if (member->getName() == owner.getName())
				{
					return false;
				}
			}
		}
		plist.push_back(owner);
		return true;
	}

	bool isMember(string &peer_name)
	{
		for (auto peer_ : groupMembers)
			if (peer_.getName() == peer_name)
				return true;
		return false;
	}

	bool deleteSharedFile(string &filename, Peer &peer)
	{
		if (filePresent(filename))
		{
			auto plist = sharedFiles[filename];
			for (auto member = plist.begin(); member != plist.end(); ++member)
			{
				if (member->getName() == peer.getName())
				{
					plist.erase(member);
					if (plist.empty())
						sharedFiles.erase(filename);
					peer.deleteSharedFile(groupName, filename);
					return true;
				}
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
		auto afile = sharedFiles[filename][0].getFileObject(groupName, filename);
		string secondPart = afile.serializeData();
		string thirdPart = "";
		auto plist = sharedFiles[filename];
		for (auto peer_ = plist.begin(); peer_ != plist.end(); peer_++)
		{
			thirdPart += peer_->serializeData1();
			if ((peer_ + 1) != plist.end())
				thirdPart += peer_list_delimiter;
		}
		return firstPart + glbl_data_delimiter + secondPart + glbl_data_delimiter + thirdPart;
	}

	int getNoOfMembers()
	{
		return groupMembers.size();
	}

	bool deleteGroupMember(Peer &peer)
	{
		for (auto member = groupMembers.begin(); member != groupMembers.end(); ++member)
		{
			if (member->getName() == peer.getName())
			{
				if (peer.getName() == groupAdmin)
				{
					if (groupMembers.size() == 1)
					{
						groupAdmin = "";
					}
					else
						groupAdmin = (member + 1)->getName();
				}
				groupMembers.erase(member);
				deleteAllSharedFilesByPeer(peer);
				peer.deleteGroup(groupName);
				return true;
			}
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
unordered_map<string, vector<Peer>> all_requests;

vector<terminal> all_peers_terminals;

int seed = 0;
mutex cout_mtx, all_peers_mtx, all_peers_terminals_mtx, all_requests_mtx, all_groups_mtx;

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
		return 1;
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

			unique_lock<mutex> ulck(all_peers_terminals_mtx);
			peerThreadObj->peer_name = newPear.getName();
			ulck.unlock();
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
	if (all_peers.count(pname) == 0 or input_in_parts.size() == 1)
		failure_case(peerThreadObj, input_in_parts);
	else
	{
		if (all_peers[pname].loginCondition(input_in_parts[1]))
		{
			send_message("## You are successfully Logged In in the network.", peerThreadObj);
			is_login_success = true;
			send_message(1, peerThreadObj);
		}
		else
		{
			send_message("Incorrect input.", peerThreadObj);
			cerr << "*** " << input_in_parts[1] << endl;
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
			all_requests[gname].push_back(all_peers[pname]);
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
		if (!groupPresent(gname))
		{
			Group newGroup(gname, pname);
			if (newGroup.addGroupMember(all_peers[pname]))
			{
				lock_guard<mutex> lck(all_groups_mtx);
				all_groups[gname] = newGroup;
				msg = "Group successfully created with you as admin.";
			}
			else
				msg = "You are already a member. Cannot join again";
		}
		else
			msg = "Group with this name already present";
		send_message(msg, peerThreadObj);
	}
}

void leave_group(const terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 2))
	{
		string pname = peerThreadObj->peer_name;
		string gname = input_in_parts[1];
		string msg;
		bool flag = true;
		if (groupPresent(gname))
		{

			auto group_ = all_groups[gname];
			auto peer_ = all_peers[pname];
			if (group_.getNoOfMembers() == 1)
			{
				unique_lock<mutex> ulck(all_groups_mtx);

				all_groups.erase(gname);
				ulck.unlock();

				lock_guard<mutex> lck(all_peers_mtx);
				peer_.deleteGroup(gname);
			}
			else if (group_.getNoOfMembers() > 1)
			{
				lock_guard<mutex> lck(all_groups_mtx);
				if (!group_.deleteGroupMember(peer_))
					msg = "You are not part of the group.";
			}

			if (flag)
				msg = "You are no longer in the group. All files shared by you have been removed.";
		}
		else
			msg = "No Such Group";
		send_message(msg, peerThreadObj);
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
			lock_guard<mutex> lck(all_requests_mtx);
			auto peer_request_list = all_requests[gname];
			if (!peer_request_list.empty())
			{
				msg = "All peers requesting to join are:\n";
				for (auto member : peer_request_list)
				{
					msg += member.getName() + "\n";
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
			unique_lock<mutex> ulck(all_requests_mtx);
			auto pending_reqs = all_requests[gname];
			for (auto req = pending_reqs.begin(); req != pending_reqs.end(); req++)
			{
				if (req->getName() == uname)
				{
					flag = false;
					pending_reqs.erase(req);
					ulck.unlock();
					lock_guard<mutex> grd(all_groups_mtx);
					if (all_groups[gname].addGroupMember(all_peers[uname]))
						msg = "Added Successfully.";
					else
						msg = "Already Added.";
					break;
				}
			}

			if (flag)
				msg = "No request found for given user";
		}
		else
			msg = "Invalid Group/User Id or You don\'t have access to accept request.";
		send_message(msg, peerThreadObj);
	}
}

void download_file(const terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 3))
	{
		string pname = peerThreadObj->peer_name;
		string gname = input_in_parts[1];
		string file_name = input_in_parts[2];
		string msg;
		lock_guard<mutex> grd(all_groups_mtx);
		auto grp = all_groups[gname];
		if (groupPresent(gname) and isMember(gname, pname))
		{
			if (grp.filePresent(file_name))
			{
				msg = grp.getAllPeersForFileSerialized(file_name);
				msg = secret_prefix + msg;
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

			send_message(msg, peerThreadObj);
		}
	}
}

void all_shareable_files_in_group(const terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 2))
	{
		string pname = peerThreadObj->peer_name;
		string gname = input_in_parts[1];
		string msg;
		lock_guard<mutex> grd(all_groups_mtx);
		if (groupPresent(gname) and isMember(gname, pname))
		{
			string files = all_groups[gname].getShareableFilesNames();
			if (files.length())
				msg = "The file list is - \n" + files;
			else
				msg = "No file shared here currently.";
		}

		else
			msg = "Invalid Group Id or you don\'t have permission to see its files";
		send_message(msg, peerThreadObj);
	}
}

void upload_file(const terminal *const peerThreadObj, vector<string> &input_in_parts)
{
	if (test_condition(peerThreadObj, input_in_parts, 3))
	{
		string pname = peerThreadObj->peer_name;
		string gname = input_in_parts[1];
		string msg;

		if (groupPresent(gname) and isMember(gname, pname))
		{
			AFile afile;

			unique_lock<mutex> ulck(all_peers_mtx);
			auto peer_ = all_peers[pname];
			if (afile.deserialize(input_in_parts[2]))
			{
				peer_.addFile(afile, gname);
				ulck.unlock();
				lock_guard<mutex> grd(all_groups_mtx);
				string filename = afile.getFileName();
				all_groups[gname].addSharedFile(filename, peer_);
				msg = "Successfully Uploaded.";
			}
			else
				msg = "upload failure, failed to deserialize";
		}
		else
			msg = "Invalid Group Id or you don\'t have permission to see its files";
		send_message(msg, peerThreadObj);
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
		send_message(0, peerThreadObj);
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
		auto peer_ = all_peers[pname];
		if (groupPresent(gname) and isMember(gname, pname))
		{
			unique_lock<mutex> ulck(all_groups_mtx);
			if (all_groups[gname].deleteSharedFile(filename, peer_))
				msg = "File is no longer shared in that group";
			else
				msg = "File not listed as shared in that group";
		}
		else
			msg = "invalid group id or you don\'t have permission to see its files";
		send_message(msg, peerThreadObj);
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

	terminal *peerThreadObj;
	for (int i = 0; i < all_peers_terminals.size(); i++)
	{
		if (all_peers_terminals[i].id == id)
		{
			peerThreadObj = &all_peers_terminals[i];
			break;
		}
	}

	// cout << "inside client " << id << endl;
	bool is_login_success = false;
	string inputCommand;

	while (true)
	{
		// cout << "now waiting" << endl;
		int bytes_received = recv(client_socket, str, sizeof(str), 0);
		cout << "---- " << str << endl;
		if (bytes_received < 0)
		{
			continue;
		}
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

		auto input_in_parts = stringSplit(str, glbl_data_delimiter);

		if (input_in_parts.empty())
		{
			cout << "***input error***" << endl;
			continue;
		}

		inputCommand = input_in_parts[0];

		if (inputCommand == "login")
		{
			login(peerThreadObj, input_in_parts, is_login_success);
			// cout << "login: " << is_login_success << endl;
		}
		else if (inputCommand == "create_user")
		{
			create_user(peerThreadObj, input_in_parts);
		}

		else if (is_login_success)
		{
			if (inputCommand == "join_group")
			{
				join_group(peerThreadObj, input_in_parts);
			}

			else if (inputCommand == "create_group")
			{
				create_group(peerThreadObj, input_in_parts);
			}

			else if (inputCommand == "leave_group")
			{
				leave_group(peerThreadObj, input_in_parts);
			}

			else if (inputCommand == "requests")
			{
				requests_list_requests(peerThreadObj, input_in_parts);
			}

			else if (inputCommand == "accept_request")
			{
				accept_group_request(peerThreadObj, input_in_parts);
			}

			else if (inputCommand == "list_groups")
			{
				all_groups_list(peerThreadObj, input_in_parts);
			}

			else if (inputCommand == "list_files")
			{
				all_shareable_files_in_group(peerThreadObj, input_in_parts);
			}
			else if (inputCommand == "download_file")
			{
				download_file(peerThreadObj, input_in_parts);
			}

			else if (inputCommand == "upload_file")
			{
				upload_file(peerThreadObj, input_in_parts);
			}

			else if (inputCommand == "logout")
			{
				logout(peerThreadObj, input_in_parts, is_login_success);
			}

			else if (inputCommand == "stop_share")
			{
				stop_sharing(peerThreadObj, input_in_parts);
			}
		}
		else
		{
			failure_case(peerThreadObj, input_in_parts);
		}
	}
}