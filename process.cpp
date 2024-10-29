#include "process.h"


typedef int (*execFunction)(pop3User &user);

pop3User::pop3User(int &client_soc)
{
	clientSocket =client_soc;
	state = POP3_STATE_AUTHORIZATION;
}
int pop3User::get_clientSocket()
{
	return clientSocket;
}
pop3User::~pop3User(void)
{
	emails.clear();
}


int pop3User::SendResponse(int nResponseType, char *msg)
{	
	char clientMessage[client_message_SIZE];
	if(nResponseType==POP3_DEFAULT_AFFERMATIVE_RESPONSE)
	{
		if(strlen(msg))
			sprintf(clientMessage,"+OK %s\r\n",msg);
		else
			sprintf(clientMessage,"+OK %s\r\n","Action performed");
	}
	else if(nResponseType==POP3_DEFAULT_NEGATIVE_RESPONSE)
		if(strlen(msg))
			sprintf(clientMessage,"-ERR %s\r\n",msg);
		else
			sprintf(clientMessage,"-ERR %s\r\n","An error occured");
		
	int len = (int)strlen(clientMessage);
	printf("Sending: %s",clientMessage);
	write(clientSocket,clientMessage,len);
	return nResponseType;
}
string skipWhitespace(string message)
{
	int counter = 0;
	while(message[counter] == ' ')
	{
		message.erase(0,1);
	}
	counter = message.size()-1;
	while(message[counter] == ' ')
	{
		message.erase(counter,1);
		counter = message.size()-1;
	}
	return message;
}
void SendMessage(pop3User &user, int mID, int topC)
{	
	string email = user.userEmail;
	string message, messageC;
	string path = DOMAIN_ROOT_PATH+user.userEmail+"/"+user.emails[mID].date
	+"`"+user.emails[mID].sender;
	ifstream MyReadFile(path.c_str());
	int count = topC;
	while (getline (MyReadFile, message))
	{
		if(count == 1)
			break;
		messageC += message + "\r\n";
		count--;	
	} 
	MyReadFile.close(); 
	messageC += "\r\n.\r\n";
	write(user.clientSocket, messageC.c_str(), messageC.size());	
}
void getMail(pop3User &User)
{
	if(User.checked == false)
	{	int counter = 0;
	    int totalFileSize = 0;
		int fileSize = 0;
		string fileLocation;
		string Mailfolder = DOMAIN_ROOT_PATH + User.userEmail;
		string fname;
		for (auto const& dir_entry : std::filesystem::directory_iterator{Mailfolder}) 
		{
			if(fs::is_regular_file(dir_entry))
			{
				fname = dir_entry.path();
				fileLocation = Mailfolder + fname;
				fname = fname.substr(fname.find_last_of('/') + 1);
				ifstream in_file(fileLocation, ios::binary);
				in_file.seekg(0, ios::end);
				fileSize = fs::file_size(dir_entry);

			    
				User.emails.push_back({counter, fileSize, fname.substr(fname.find('`')+1), 
				fname.substr(0,fname.find('`')), false});
				totalFileSize += fileSize;
				counter++;
			}
		}
		User.mailCount = counter;
		User.TotalmailSize = totalFileSize;
		User.checked = true;
	}
}
bool lockMail(pop3User &User)
{
	for (int i = 0; i < listUser.size(); i++) 
	{
		if(listUser[i].email == User.userEmail)
		{
			if(listUser[i].lock == true)
				return false;
			listUser[i].lock = true; 
			User.mailLock = true;
			User.index = listUser[i].count;
			return true;
		}
	}
	return false;
}
int ProcessUSER(pop3User &User)
{
	char InvalidAuth[] = "Mail box does not exists";
	char MdLocked[]  = "The mail drop has been locked";
	char MdULocked[] = "Unable to lock mail drop";
	if(User.state == POP3_STATE_RPOP)
	{
		printf("Processing USER command....\n");

		if(User.clientMessage != User.userEmail)
		{
			return User.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, InvalidAuth);
		}
		string mailPath = DOMAIN_ROOT_PATH + User.userEmail;

		if(fs::is_directory(mailPath))
		{
			printf("User %s found\n",User.userEmail.c_str());
			if(lockMail(User) == true)
			{
				User.state = POP3_STATE_TRANSACTION;
				getMail(User);
				return User.SendResponse(POP3_DEFAULT_AFFERMATIVE_RESPONSE, MdLocked);
			}else
			{
				return User.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, MdULocked);
			}
		}
		printf("User %s not found \n",User.userEmail.c_str());
		return User.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, InvalidAuth);
	}
	return User.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, InvalidAuth);
}
int ProcessRPOP(pop3User &user)
{
	char empty[] = "";
	string userAuth, email; 
	char invalid[]  ="Invalid Auth Code";
	char invalid1[] ="Invalid Command";
	if(user.state == POP3_STATE_AUTHORIZATION)
	{	user.state = POP3_STATE_RPOP;
		user.AuthCode = user.clientMessage;
		ifstream MyReadFile(USER_LIST);
		printf("Verifying Auth Code ... \n");	
		while (getline (MyReadFile, userAuth))
		{
			if(userAuth.substr(0, userAuth.find('`')) == user.AuthCode)
			{
				email = userAuth.substr(userAuth.find('`') + 1);
				user.userEmail = email;	
				return user.SendResponse(POP3_DEFAULT_AFFERMATIVE_RESPONSE,empty);
			}
		} 
		MyReadFile.close(); 
		return user.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, invalid);
	}
	return user.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, invalid1);
}
int ProcessQUIT(pop3User &user)
{
	printf("Exiting Mail server...\n");
	char quit[] ="goodbye exiting user Email";
	string fileLocation;
	if(user.state == POP3_STATE_TRANSACTION)
	{
		user.state = POP3_STATE_UPDATE;
		for(int i=0; i < user.emails.size(); i++)
		{
			if(user.emails[i].deleted == true)
			{
				fileLocation = DOMAIN_ROOT_PATH + user.userEmail 
				+ user.emails[i].date + user.emails[i].sender;
				fs::remove(fileLocation);
			}
		}
		listUser[user.index].lock = false;
		listUser[user.index].lastmsg = user.m_nLastMsg;
		user.mailLock = false;
	}
	user.SendResponse(POP3_DEFAULT_AFFERMATIVE_RESPONSE, quit);
	return -1;
}
int ProcessSTAT(pop3User &user)
{
	char error[] ="Invalid Command";
	if(user.state == POP3_STATE_TRANSACTION)
	{
		printf("Processing STAT command...\n");
		string message = to_string(user.mailCount)+" messages, "+to_string(user.TotalmailSize)+" bytes";
		//char* message1 = new char[message.size() + 1];
		//message1 = message.c_str();
		char* message1 = new char[message.size() + 1];
		strcpy(message1, message.c_str());
		return user.SendResponse(POP3_DEFAULT_AFFERMATIVE_RESPONSE, message1);
	}
	return user.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, error);
}
int ProcessLIST(pop3User &user)
{
	char error[] ="Invalid Command";
	char* resp;
	char* message1;
	string resp1;
	if(user.state == POP3_STATE_TRANSACTION)
	{
		printf("Processing LIST command...\n");
		if(user.clientMessage == "")
		{	
			string message = to_string(user.mailCount)+" mails. "+to_string(user.TotalmailSize)+" bytes";
			message1 = new char[message.size() + 1];
			strcpy(message1 , message.c_str());
			user.SendResponse(POP3_DEFAULT_AFFERMATIVE_RESPONSE, message1);
			for(int i = 0; i < user.emails.size(); i++)
			{
				if(user.emails[i].deleted == false)
				{
					resp1 = to_string(user.emails[i].serialNo)+". sender: "
					+user.emails[i].sender+". Date: "+user.emails[i].date+"  "
					+to_string(user.emails[i].fileSize)+" bytes";
					message1 = new char[resp1.size() + 1];
					strcpy(message1 , resp1.c_str());					
					user.SendResponse(POP3_DEFAULT_AFFERMATIVE_RESPONSE, message1);
				}
			}return POP3_DEFAULT_AFFERMATIVE_RESPONSE;
		}else
		{
			try 
			{
				int x = stoi(user.clientMessage);			
				if (user.mailCount >= x && user.emails[x].deleted == false)
				{
					resp1 = to_string(user.emails[x].serialNo)+". sender: "
					+user.emails[x].sender+". Date: "+user.emails[x].date+"  "
					+to_string(user.emails[x].fileSize)+" bytes";
					resp = new char[resp1.size() + 1];
					strcpy(resp,resp1.c_str());
					return user.SendResponse(POP3_DEFAULT_AFFERMATIVE_RESPONSE, resp);
				} else {
					throw 505;
				}
			}
			catch (...) 
			{
				return user.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, error);
			} 
		}
	}
	return user.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, error);
}
int ProcessRETR(pop3User &user)
{	
	printf("Processing RETR Command ...\n");
	char invalid[] = "Invalid Command";
	int mID = stoi(user.clientMessage);
	if(user.state != POP3_STATE_TRANSACTION)
	{
		return user.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, invalid);
	}
	if(mID >= user.mailCount || user.emails[mID].deleted == true) 
	{
		char invalid1[] = "Message does not exist";
		return user.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, invalid1);
	}

	if(user.m_nLastMsg < (unsigned int)mID) 
		user.m_nLastMsg = mID;

	char resp[25];
	sprintf(resp," %d bytes\r\n",user.emails[mID-1].fileSize);
	user.SendResponse(POP3_DEFAULT_AFFERMATIVE_RESPONSE, resp);
	SendMessage(user, mID, 0);
	return POP3_DEFAULT_AFFERMATIVE_RESPONSE;
}
int ProcessDELE(pop3User &user)
{
	int msg_id= stoi(user.clientMessage);
	printf("Deleting message... %d\n",msg_id);
	char message[] = "Invalid Command";
	if(user.state!=POP3_STATE_TRANSACTION || msg_id>user.mailCount)
	{
		return user.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, message);
	}
	user.emails[msg_id].deleted = true;
	user.mailCount--;
	user.TotalmailSize -= user.emails[msg_id].fileSize;
	char message1[] = "message deleted";
	return user.SendResponse(POP3_DEFAULT_AFFERMATIVE_RESPONSE, message1);
}
int ProcessNOOP(pop3User &user)
{
	char empty[] = "";
	if(user.state!=POP3_STATE_TRANSACTION)
	{
		return user.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, empty);
	}
	printf("Processing the NOOP ...\n");
	return user.SendResponse(POP3_DEFAULT_AFFERMATIVE_RESPONSE, empty);
}
int ProcessLAST(pop3User &user)
{
	char empty[] = "";
	if(user.state!=POP3_STATE_TRANSACTION)
	{
		return user.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, empty);
	}
	printf("Processing LAST accessed message...\n");
	char resp[25];
	sprintf(resp, "%d",user.m_nLastMsg);
	return user.SendResponse(POP3_DEFAULT_AFFERMATIVE_RESPONSE, resp);
}
int ProcessRSET(pop3User &user)
{
	char empty[] = "";
	printf("Resetting mailBox...\n");
	if(user.state!=POP3_STATE_TRANSACTION)
	{
		return user.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, empty);
	}
	int i,filesize = 0;
	for(i=0; i < user.emails.size(); i++)
	{
		user.emails[i].deleted = false;
		filesize += user.emails[i].fileSize;
	}
	user.mailCount = i;
	user.TotalmailSize = filesize;
	string message = "mail drop has " + to_string(user.mailCount) 
	+ " messages, "+to_string(user.TotalmailSize) +" bytes";
	char* resp = new char[message.size() +1];
	strcpy(resp, message.c_str());
	return user.SendResponse(POP3_DEFAULT_AFFERMATIVE_RESPONSE, resp);
}
int ProcessTOP(pop3User &user)
{
	int mID = stoi(user.clientMessage.substr(0, user.clientMessage.find(' ')));
	int topC= stoi(user.clientMessage.substr(user.clientMessage.find_last_of(' ') + 1));
	printf("Processing TOP ...\n");
	char invalid[] = "Invalid Command";
	if(user.state != POP3_STATE_TRANSACTION)
	{
		return user.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, invalid);
	}
	if(mID >= user.TotalmailSize || user.emails[mID].deleted == true) 
	{
		char invalid1[] = "Message does not exist";
		return user.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, invalid1);
	}

	if(user.m_nLastMsg < (unsigned int)mID) 
		user.m_nLastMsg = mID;

	char resp[25];
	sprintf(resp," %d bytes\r\n",user.emails[mID-1].fileSize);
	user.SendResponse(POP3_DEFAULT_AFFERMATIVE_RESPONSE, resp);
	SendMessage(user, mID, topC);
	return POP3_DEFAULT_NEGATIVE_RESPONSE;
}


execFunction funcArray[] = {ProcessRPOP, ProcessUSER, ProcessQUIT, ProcessSTAT, ProcessLIST
, ProcessRETR, ProcessDELE, ProcessNOOP, ProcessLAST, ProcessRSET, ProcessTOP};


string commands[] ={"RPOP","USER","QUIT", "STAT", "LIST","RETR","DELE","NOOP", "LAST","RSET",
"TOPS"};

int ProcessCMD(pop3User &User, char *clientMessage)
{
	string message = clientMessage;
	string crlf = "\\r\\n";
	char resp[] = "Invalid Command";

	cout<<message<<endl;
	
	
	for(int i = 0; i < sizeof(funcArray); i++)
	{	
		
		if(strncmp(message.c_str(),commands[i].c_str(),4)== 0 && message.size() > 6)
		{
			

			if(message.substr(message.size()-4, message.size()-1) == crlf)
			{
				message.erase(0,4);
				message.erase(message.size()-4, 4);
			}
			else
			{
				return User.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, resp);
			}
			message = skipWhitespace(message);
			User.clientMessage = message;
			
			return funcArray[i](User);
		}		
	}
	return User.SendResponse(POP3_DEFAULT_NEGATIVE_RESPONSE, resp);

}
