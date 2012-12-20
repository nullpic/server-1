struct Game;

struct ClientNode {
	int fd;
	struct ClientNode * next;
	char * buff;
	int buffsize;
	int cmdlen;
	char name[50];
	int money;
	int prod;
	int raw;
	int fac;
	struct Game * game;
};


struct Game {
	struct ClientNode * cls;
	int playersCount;
	char state;
};


void SendMessage(const char * msg, struct ClientNode * cl, char finish);
void Broadcast(const char *msg, struct Game * game, char finish);
void itoa(int n, char * buff);
char ** ParseCmd(const char * cmd, int * argc);

void SHelp(struct ClientNode * cl);
void Start(int argc, char ** argv, struct ClientNode * cl);
void Info(int argc, char ** argv, struct ClientNode * cl);
void Market(int argc, char ** argv, struct ClientNode * cl);
void Player(int argc, char ** argv, struct ClientNode * cl);
void Prod(int argc, char ** argv, struct ClientNode * cl);
void Turn(int argc, char ** argv, struct ClientNode * cl);
void Buy(int argc, char ** argv, struct ClientNode * cl);
void Sell(int argc, char ** argv, struct ClientNode * cl);
void Build(int argc, char ** argv, struct ClientNode * cl);
void Name(int argc, char ** argv, struct ClientNode * cl);
void Help(struct ClientNode * cl);
