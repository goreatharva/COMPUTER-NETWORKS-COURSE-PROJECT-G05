#include <string.h> 
#include <string> 
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <semaphore.h>
#include <pthread.h> 
#include <filesystem>
#include <fstream>
#include <iostream>
#include "globals.h" 

using namespace std;
namespace fs = std::filesystem;

struct mailAccounts{
    int count;
	int lastmsg;
    bool lock;
    string email;
};
struct mail{
	int serialNo;
	int fileSize;
    string sender;
	string date;
    bool deleted;
};

extern vector<mailAccounts>listUser;

class pop3User
{
public:
	bool session;
	bool mailLock;
	bool checked;
	vector<mail>emails;
	unsigned int index;
	unsigned int m_nLastMsg;
	unsigned int clientSocket;
	unsigned int state;
	unsigned int mailCount;
	unsigned int TotalmailSize;
	string mailPath;
	string userEmail;
	string clientMessage;
	string AuthCode;

	pop3User(int &client_soc);
	virtual ~pop3User(void);
	int SendResponse(int nResponseType, char *msg);
	int SendResponse(int nResponseType);
	int SendResponse(char *msg);
	int get_clientSocket();
};

int ProcessCMD(pop3User &user, char * clientMessage);