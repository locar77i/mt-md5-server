// Stl
#include <mutex>
#include <thread>
#include <random>
// sockets
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

// lib locar
#include "lcr/CommandLine.hpp"



using namespace std;



// Definitions /////////////////////////////////////////////////////////////////////
struct Arguments // The Arguments type stores the parameters from the command line after parsing
{
   int port{};             // The server port number. Posible values: [1024-65535]
   int requests{};         // The number of client requests that will be executed against the server
   std::string command{};  // The request command type
   std::string message{};  // The request message
   std::string number{};   // The request number
};


// Prototypes //////////////////////////////////////////////////////////////////////
void show_usage(); // Function that shows the program usage
void check(Arguments& args); // Function that checks the arguments validity
void signal_handler(int signum); // Function that handles all required signals
std::string get_word();


// Static constants ////////////////////////////////////////////////////////////////
static const int C_S_DEFAULT_PORT{3456};
static const int C_S_CLIENT_REQUESTS{1};
static const std::string C_S_REQUEST_COMMAND{"get"};
// It is not necessary to set defaults, becasuse user may want to set wrong values
//static const std::string C_S_REQUEST_MESSAGE{"hello"};
//static const int C_S_REQUEST_NUMBER{512};



// This class represents the NCS client, which connect to NCS server to send a single request
class Client
{
   public:
      // The constructor receives as parameters a por number, the string command for the request, the message text and the request number text
      Client(unsigned int port, const std::string& cmd, const std::string& message, const std::string& number)
         : port_(port)
         , cmd_(cmd)
         , message_(message)
         , number_(number)
         , ec_()
      {}

      virtual ~Client()
      {}

   public:
      void operator()() {
         if((sockfd_=socket(AF_INET,SOCK_STREAM,0))==-1) {
            perror("socket: ");
            ec_ = errno;
         }
         else {
            struct sockaddr_in client;
            client.sin_family=AF_INET;
            client.sin_port=htons(port_);
            client.sin_addr.s_addr=INADDR_ANY;
            bzero(&client.sin_zero,0);

            if((connect(sockfd_,(struct sockaddr *)&client,sizeof(struct sockaddr_in)))==-1) {
               perror("connect: ");
               ec_ = errno;
            }
            if(!ec_) {
               std::string request;
               if(!cmd_.empty()) {
                  request += (cmd_ + " ");
               }
               if(!message_.empty()) {
                  request += (message_ + " ");
               }
               request += number_;
               request += '\n';
               std::cout << "SENDING REQUEST: '" << request << "'" << std::endl;
               int bytes_sent = send(sockfd_, request.c_str(), request.length(), 0);
               if(bytes_sent==-1) {
                  perror("send: ");
                  ec_ = errno;
               }
               else {
                  char buffer[256]; // Digest size in 128bits, 16bytes, 32chars. However, the error response may be greater than 32chars
                  int bytes_received = recv(sockfd_, buffer, (sizeof(buffer)-1), 0);
                  close(sockfd_);
                  if(bytes_received!=-1) {
                     std::cout << "'" << request << "' (" << bytes_sent << " bytes)  =RESPONSE=>  " << buffer << std::endl;
                  }
                  else {
                     perror("send: ");
                     ec_ = errno;
                  }
               }
            }
            close(sockfd_);
         }
      }

   private:
      int sockfd_;
      unsigned int port_;
      std::string cmd_;
      std::string message_;
      std::string number_;

      // The error number
      int ec_;
};



static inline void pop_first(std::queue<std::thread> & threads)
{
   auto & t = threads.front();
   if(t.joinable()) {
      t.join();
   }
   threads.pop();
}

void show_usage();


int main(int argc, const char* argv[])
{
   // Look for the sow_help parameter
   if(argc==2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
      show_usage();
      return 0;
   } // else ...

   // Specify the input parameters and proceed to parse the command line
   auto args = lcr::CommandLine<Arguments>::Parser({
      {"-p", &Arguments::port},
      {"-r", &Arguments::requests},
      {"-c", &Arguments::command},
      {"-m", &Arguments::message},
      {"-n", &Arguments::number}
   })->parse(argc, argv);

   // Check the arguments validity
   check(args);

   // Ramdon utilities
   std::random_device rd;
   std::mt19937 engine(rd());
   std::uniform_int_distribution<> get_number(3000, 7000);

   std::queue<std::thread> threads;

   for(unsigned int ii=0; ii<args.requests; ++ii) {
      Client client(args.port, args.command, (args.requests==1? args.message : get_word()), (args.requests==1? args.number : std::to_string(get_number(engine))));
      std::thread t(client);
      threads.push(std::move(t));
      if(threads.size()==256) {
         pop_first(threads);
      }
   }
			
   while(!threads.empty()) {
      pop_first(threads);
   }
			
	return 0;
}



// Function that shows the program usage
void show_usage()
{
   std::cout << "---- Command line -----------------------------------------------------------------------------------------------------" << std::endl << std::endl;
   std::cout << " -h      Program help" << std::endl;
   std::cout << " --help  Show details of the program usage" << std::endl << std::endl;
   std::cout << " -p      Port number" << std::endl;
   std::cout << "         The server port number." << std::endl;
   std::cout << "         Posible values: [1024-65535]" << std::endl;
   std::cout << "         Default value: " << C_S_DEFAULT_PORT << std::endl << std::endl;
   std::cout << " -r      Total requests" << std::endl;
   std::cout << "         The number of client requests that will be executed against the server." << std::endl;
   std::cout << "         When total requests is 1, the -c, -m, and -n arguments must be set to send a valid request" << std::endl;
   std::cout << "         When total requests is greater than 1, the -m, and -n arguments are omited (random)" << std::endl;
   std::cout << "         Default value: " << C_S_CLIENT_REQUESTS << std::endl << std::endl;
   std::cout << " -c      Request command" << std::endl;
   std::cout << "         The request command string." << std::endl;
   std::cout << "         Default value: '" << C_S_REQUEST_COMMAND << "'" << std::endl << std::endl;
   std::cout << " -m      Request message" << std::endl;
   std::cout << "         The request message string." << std::endl << std::endl;
   std::cout << " -n      Request number" << std::endl;
   std::cout << "         The request number string." << std::endl << std::endl;
   std::cout << "Examples:" << std::endl;
   std::cout << "         client -h" << std::endl;
   std::cout << "         client --help" << std::endl;
   std::cout << "         client -p 3456 -r 1000" << std::endl;
   std::cout << "         client -p 4096 -c get -m hello -n 512" << std::endl;
   std::cout << "-----------------------------------------------------------------------------------------------------------------------" << std::endl;
}


// Function that checks the arguments validity
void check(Arguments& args)
{
   // Check the port number argument
   if(args.port<0) {
      std::cout << "[MAIN] Invalid port number (" << args.port << "). Setting " << C_S_DEFAULT_PORT << " as default" << std::endl;
      args.port = C_S_DEFAULT_PORT;
   }
   else if(args.port<=1023) {
      std::cout << "[MAIN] Port number " << args.port << " is reserved. Setting " << C_S_DEFAULT_PORT << " as default" << std::endl;
      args.port = C_S_DEFAULT_PORT;
   }
   else if(args.port>65535) {
      std::cout << "[MAIN] Port number " << args.port << " is not in the range 1023-65535. Setting " << C_S_DEFAULT_PORT << " as default" << std::endl;
      args.port = C_S_DEFAULT_PORT;
   }
   // Check the client requests argument
   if(args.requests<=0) {
      std::cout << "[MAIN] Invalid requests number " << args.requests << ". Setting " << C_S_CLIENT_REQUESTS << " as default" << std::endl;
      args.requests = C_S_CLIENT_REQUESTS;
   }
   else if(args.requests>1) { // Check only when multitesting
      // Check the request command argument
      if(args.command.empty()) {
         std::cout << "[MAIN] Invalid request command " << args.command << ". Setting " << C_S_REQUEST_COMMAND << " as default" << std::endl;
         args.command = C_S_REQUEST_COMMAND;
      }
/* It is not necessary to check, becasuse user may want to set wrong values
      // Check the request message argument
      if(args.message.empty()) {
         std::cout << "[MAIN] Invalid request message " << args.message << ". Setting " << C_S_REQUEST_MESSAGE << " as default" << std::endl;
         args.message = C_S_REQUEST_MESSAGE;
      }
      // Check the request number argument
      if(args.number == 0) {
         std::cout << "[MAIN] Invalid request number " << args.number << ", it must be positive. Setting " << C_S_REQUEST_NUMBER << " as default" << std::endl;
         args.number = C_S_REQUEST_NUMBER;
      }
*/
   }
   std::cout <<  "[MAIN]---- Execution parameters ---------------------------------------------------" << std::endl;
   std::cout <<  "[MAIN] Port number    : " << args.port << std::endl;
   std::cout <<  "[MAIN] Total requests : " << args.requests << std::endl;
   std::cout <<  "[MAIN] Request command: '" << args.command << "'" << std::endl;
   if(args.requests==1) { // Single test
      std::cout <<  "[MAIN] Request message: '" << args.message << "'" << std::endl;
      std::cout <<  "[MAIN] Request number : '" << args.number << "'" << std::endl;
   }
   else { // (args.requests>1) ... Multi-testing
      std::cout <<  "[MAIN] Request message: random" << std::endl;
      std::cout <<  "[MAIN] Request number : random" << std::endl;
   }
   std::cout <<  "[MAIN]-----------------------------------------------------------------------------" << std::endl;
}



std::string get_word() {
   static std::vector<string> words {
"abandon",
"ability",
"able",
"about",
"above",
"absent",
"absorb",
"abstract",
"absurd",
"abuse",
"access",
"accident",
"account",
"accuse",
"achieve",
"acid",
"acoustic",
"acquire",
"across",
"act",
"action",
"actor",
"actress",
"actual",
"adapt",
"add",
"addict",
"address",
"adjust",
"admit",
"adult",
"advance",
"advice",
"aerobic",
"affair",
"afford",
"afraid",
"again",
"age",
"agent",
"agree",
"ahead",
"aim",
"air",
"airport",
"aisle",
"alarm",
"album",
"alcohol",
"alert",
"alien",
"all",
"alley",
"allow",
"almost",
"alone",
"alpha",
"already",
"also",
"alter",
"always",
"amateur",
"amazing",
"among",
"amount",
"amused",
"analyst",
"anchor",
"ancient",
"anger",
"angle",
"angry",
"animal",
"ankle",
"announce",
"annual",
"another",
"answer",
"antenna",
"antique",
"anxiety",
"any",
"apart",
"apology",
"appear",
"apple",
"approve",
"april",
"arch",
"arctic",
"area",
"arena",
"argue",
"arm",
"armed",
"armor",
"army",
"around",
"arrange",
"arrest",
"arrive",
"arrow",
"art",
"artefact",
"artist",
"artwork",
"ask",
"aspect",
"assault",
"asset",
"assist",
"assume",
"asthma",
"athlete",
"atom",
"attack",
"attend",
"attitude",
"attract",
"auction",
"audit",
"august",
"aunt",
"author",
"auto",
"autumn",
"average",
"avocado",
"avoid",
"awake",
"aware",
"away",
"awesome",
"awful",
"awkward",
"axis",
"baby",
"bachelor",
"bacon",
"badge",
"bag",
"balance",
"balcony",
"ball",
"bamboo",
"banana",
"banner",
"bar",
"barely",
"bargain",
"barrel",
"base",
"basic",
"basket",
"battle",
"beach",
"bean",
"beauty",
"because",
"become",
"beef",
"before",
"begin",
"behave",
"behind",
"believe",
"below",
"belt",
"bench",
"benefit",
"best",
"betray",
"better",
"between",
"beyond",
"bicycle",
"bid",
"bike",
"bind",
"biology",
"bird",
"birth",
"bitter",
"black",
"blade",
"blame",
"blanket",
"blast",
"bleak",
"bless",
"blind",
"blood",
"blossom",
"blouse",
"blue",
"blur",
"blush",
"board",
"boat",
"body",
"boil",
"bomb",
"bone",
"bonus",
"book",
"boost",
"border",
"boring",
"borrow",
"boss",
"bottom",
"bounce",
"box",
"boy",
"bracket",
"brain",
"brand",
"brass",
"brave",
"bread",
"breeze",
"brick",
"bridge",
"brief",
"bright",
"bring",
"brisk",
"broccoli",
"broken",
"bronze",
"broom",
"brother",
"brown",
"brush",
"bubble",
"buddy",
"budget",
"buffalo",
"build",
"bulb",
"bulk",
"bullet",
"bundle",
"bunker",
"burden",
"burger",
"burst",
"bus",
"business",
"busy",
"butter",
"buyer",
"buzz",
"cabbage",
"cabin",
"cable",
"cactus",
"cage",
"cake",
"call",
"calm",
"camera",
"camp",
"can",
"canal",
"cancel",
"candy",
"cannon",
"canoe",
"canvas",
"canyon",
"capable",
"capital",
"captain",
"car",
"carbon",
"card",
"cargo",
"carpet",
"carry",
"cart",
"case",
"cash",
"casino",
"castle",
"casual",
"cat",
"catalog",
"catch",
"category",
"cattle",
"caught",
"cause",
"caution",
"cave",
"ceiling",
"celery",
"cement",
"census",
"century",
"cereal",
"certain",
"chair",
"chalk",
"champion",
"change",
"chaos",
"chapter",
"charge",
"chase",
"chat",
"cheap",
"check",
"cheese",
"chef",
"cherry",
"chest",
"chicken",
"chief",
"child",
"chimney",
"choice",
"choose",
"chronic",
"chuckle",
"chunk",
"churn",
"cigar",
"cinnamon",
"circle",
"citizen",
"city",
"civil",
"claim",
"clap",
"clarify",
"claw",
"clay",
"clean",
"clerk",
"clever",
"click",
"client",
"cliff",
"climb",
"clinic",
"clip",
"clock",
"clog",
"close",
"cloth",
"cloud",
"clown",
"club",
"clump",
"cluster",
"clutch",
"coach",
"coast",
"coconut",
"code",
"coffee",
"coil",
"coin",
"collect",
"color",
"column",
"combine",
"come",
"comfort",
"comic",
"common",
"company",
"concert",
"conduct",
"confirm",
"congress",
"connect",
"consider",
"control",
"convince",
"cook",
"cool",
"copper",
"copy",
"coral",
"core",
"corn",
"correct",
"cost",
"cotton",
"couch",
"country",
"couple",
"course",
"cousin",
"cover",
"coyote",
"crack",
"cradle",
"craft",
"cram",
"crane",
"crash",
"crater",
"crawl",
"crazy",
"cream",
"credit",
"creek",
"crew",
"cricket",
"crime",
"crisp",
"critic",
"crop",
"cross",
"crouch",
"crowd",
"crucial",
"cruel",
"cruise",
"crumble",
"crunch",
"crush",
"cry",
"crystal",
"cube",
"culture",
"cup",
"cupboard",
"curious",
"current",
"curtain",
"curve",
"cushion",
"custom",
"cute",
"cycle",
"dad",
"damage",
"damp",
"dance",
"danger",
"daring",
"dash",
"daughter",
"dawn",
"day",
"deal",
"debate",
"debris",
"decade",
"december",
"decide",
"decline",
"decorate",
"decrease",
"deer",
"defense",
"define",
"defy",
"degree",
"delay",
"deliver",
"demand",
"demise",
"denial",
"dentist",
"deny",
"depart",
"depend",
"deposit",
"depth",
"deputy",
"derive",
"describe",
"desert",
"design",
"desk",
"despair",
"destroy",
"detail",
"detect",
"develop",
"device",
"devote",
"diagram",
"dial",
"diamond",
"diary",
"dice",
"diesel",
"diet",
"differ",
"digital",
"dignity",
"dilemma",
"dinner",
"dinosaur",
"direct",
"dirt",
"disagree",
"discover",
"disease",
"dish",
"dismiss",
"disorder",
"display",
"distance",
"divert",
"divide",
"divorce",
"dizzy",
"doctor",
"document",
"dog",
"doll",
"dolphin",
"domain",
"donate",
"donkey",
"donor",
"door",
"dose",
"double",
"dove",
"draft",
"dragon",
"drama",
"drastic",
"draw",
"dream",
"dress",
"drift",
"drill",
"drink",
"drip",
"drive",
"drop",
"drum",
"dry",
"duck",
"dumb",
"dune",
"during",
"dust",
"dutch",
"duty",
"dwarf",
"dynamic",
"eager",
"eagle",
"early",
"earn",
"earth",
"easily",
"east",
"easy",
"echo",
"ecology",
"economy",
"edge",
"edit",
"educate",
"effort",
"egg",
"eight",
"either",
"elbow",
"elder",
"electric",
"elegant",
"element",
"elephant",
"elevator",
"elite",
"else",
"embark",
"embody",
"embrace",
"emerge",
"emotion",
"employ",
"empower",
"empty",
"enable",
"enact",
"end",
"endless",
"endorse",
"enemy",
"energy",
"enforce",
"engage",
"engine",
"enhance",
"enjoy",
"enlist",
"enough",
"enrich",
"enroll",
"ensure",
"enter",
"entire",
"entry",
"envelope",
"episode",
"equal",
"equip",
"era",
"erase",
"erode",
"erosion",
"error",
"erupt",
"escape",
"essay",
"essence",
"estate",
"eternal",
"ethics",
"evidence",
"evil",
"evoke",
"evolve",
"exact",
"example",
"excess",
"exchange",
"excite",
"exclude",
"excuse",
"execute",
"exercise",
"exhaust",
"exhibit",
"exile",
"exist",
"exit",
"exotic",
"expand",
"expect",
"expire",
"explain",
"expose",
"express",
"extend",
"extra",
"eye",
"eyebrow",
"fabric",
"face",
"faculty",
"fade",
"faint",
"faith",
"fall",
"false",
"fame",
"family",
"famous",
"fan",
"fancy",
"fantasy",
"farm",
"fashion",
"fat",
"fatal",
"father",
"fatigue",
"fault",
"favorite",
"feature",
"february",
"federal",
"fee",
"feed",
"feel",
"female",
"fence",
"festival",
"fetch",
"fever",
"few",
"fiber",
"fiction",
"field",
"figure",
"file",
"film",
"filter",
"final",
"find",
"fine",
"finger",
"finish",
"fire",
"firm",
"first",
"fiscal",
"fish",
"fit",
"fitness",
"fix",
"flag",
"flame",
"flash",
"flat",
"flavor",
"flee",
"flight",
"flip",
"float",
"flock",
"floor",
"flower",
"fluid",
"flush",
"fly",
"foam",
"focus",
"fog",
"foil",
"fold",
"follow",
"food",
"foot",
"force",
"forest",
"forget",
"fork",
"fortune",
"forum",
"forward",
"fossil",
"foster",
"found",
"fox",
"fragile",
"frame",
"frequent",
"fresh",
"friend",
"fringe",
"frog",
"front",
"frost",
"frown",
"frozen",
"fruit",
"fuel",
"fun",
"funny",
"furnace",
"fury",
"future",
"gadget",
"gain",
"galaxy",
"gallery",
"game",
"gap",
"garage",
"garbage",
"garden",
"garlic",
"garment",
"gas",
"gasp",
"gate",
"gather",
"gauge",
"gaze",
"general",
"genius",
"genre",
"gentle",
"genuine",
"gesture",
"ghost",
"giant",
"gift",
"giggle",
"ginger",
"giraffe",
"girl",
"give",
"glad",
"glance",
"glare",
"glass",
"glide",
"glimpse",
"globe",
"gloom",
"glory",
"glove",
"glow",
"glue",
"goat",
"goddess",
"gold",
"good",
"goose",
"gorilla",
"gospel",
"gossip",
"govern",
"gown",
"grab",
"grace",
"grain",
"grant",
"grape",
"grass",
"gravity",
"great",
"green",
"grid",
"grief",
"grit",
"grocery",
"group",
"grow",
"grunt",
"guard",
"guess",
"guide",
"guilt",
"guitar",
"gun",
"gym",
"habit",
"hair",
"half",
"hammer",
"hamster",
"hand",
"happy",
"harbor",
"hard",
"harsh",
"harvest",
"hat",
"have",
"hawk",
"hazard",
"head",
"health",
"heart",
"heavy",
"hedgehog",
"height",
"hello",
"helmet",
"help",
"hen",
"hero",
"hidden",
"high",
"hill",
"hint",
"hip",
"hire",
"history",
"hobby",
"hockey",
"hold",
"hole",
"holiday",
"hollow",
"home",
"honey",
"hood",
"hope",
"horn",
"horror",
"horse",
"hospital",
"host",
"hotel",
"hour",
"hover",
"hub",
"huge",
"human",
"humble",
"humor",
"hundred",
"hungry",
"hunt",
"hurdle",
"hurry",
"hurt",
"husband",
"hybrid",
"ice",
"icon",
"idea",
"identify",
"idle",
"ignore",
"ill",
"illegal",
"illness",
"image",
"imitate",
"immense",
"immune",
"impact",
"impose",
"improve",
"impulse",
"inch",
"include",
"income",
"increase",
"index",
"indicate",
"indoor",
"industry",
"infant",
"inflict",
"inform",
"inhale",
"inherit",
"initial",
"inject",
"injury",
"inmate",
"inner",
"innocent",
"input",
"inquiry",
"insane",
"insect",
"inside",
"inspire",
"install",
"intact",
"interest",
"into",
"invest",
"invite",
"involve",
"iron",
"island",
"isolate",
"issue",
"item",
"ivory",
"jacket",
"jaguar",
"jar",
"jazz",
"jealous",
"jeans",
"jelly",
"jewel",
"job",
"join",
"joke",
"journey",
"joy",
"judge",
"juice",
"jump",
"jungle",
"junior",
"junk",
"just",
"kangaroo",
"keen",
"keep",
"ketchup",
"key",
"kick",
"kid",
"kidney",
"kind",
"kingdom",
"kiss",
"kit",
"kitchen",
"kite",
"kitten",
"kiwi",
"knee",
"knife",
"knock",
"know",
"lab",
"label",
"labor",
"ladder",
"lady",
"lake",
"lamp",
"language",
"laptop",
"large",
"later",
"latin",
"laugh",
"laundry",
"lava",
"law",
"lawn",
"lawsuit",
"layer",
"lazy",
"leader",
"leaf",
"learn",
"leave",
"lecture",
"left",
"leg",
"legal",
"legend",
"leisure",
"lemon",
"lend",
"length",
"lens",
"leopard",
"lesson",
"letter",
"level",
"liar",
"liberty",
"library",
"license",
"life",
"lift",
"light",
"like",
"limb",
"limit",
"link",
"lion",
"liquid",
"list",
"little",
"live",
"lizard",
"load",
"loan",
"lobster",
"local",
"lock",
"logic",
"lonely",
"long",
"loop",
"lottery",
"loud",
"lounge",
"love",
"loyal",
"lucky",
"luggage",
"lumber",
"lunar",
"lunch",
"luxury",
"lyrics",
"machine",
"mad",
"magic",
"magnet",
"maid",
"mail",
"main",
"major",
"make",
"mammal",
"man",
"manage",
"mandate",
"mango",
"mansion",
"manual",
"maple",
"marble",
"march",
"margin",
"marine",
"market",
"marriage",
"mask",
"mass",
"master",
"match",
"material",
"math",
"matrix",
"matter",
"maximum",
"maze",
"meadow",
"mean",
"measure",
"meat",
"mechanic",
"medal",
"media",
"melody",
"melt",
"member",
"memory",
"mention",
"menu",
"mercy",
"merge",
"merit",
"merry",
"mesh",
"message",
"metal",
"method",
"middle",
"midnight",
"milk",
"million",
"mimic",
"mind",
"minimum",
"minor",
"minute",
"miracle",
"mirror",
"misery",
"miss",
"mistake",
"mix",
"mixed",
"mixture",
"mobile",
"model",
"modify",
"mom",
"moment",
"monitor",
"monkey",
"monster",
"month",
"moon",
"moral",
"more",
"morning",
"mosquito",
"mother",
"motion",
"motor",
"mountain",
"mouse",
"move",
"movie",
"much",
"muffin",
"mule",
"multiply",
"muscle",
"museum",
"mushroom",
"music",
"must",
"mutual",
"myself",
"mystery",
"myth",
"naive",
"name",
"napkin",
"narrow",
"nasty",
"nation",
"nature",
"near",
"neck",
"need",
"negative",
"neglect",
"neither",
"nephew",
"nerve",
"nest",
"net",
"network",
"neutral",
"never",
"news",
"next",
"nice",
"night",
"noble",
"noise",
"nominee",
"noodle",
"normal",
"north",
"nose",
"notable",
"note",
"nothing",
"notice",
"novel",
"now",
"nuclear",
"number",
"nurse",
"nut",
"oak",
"obey",
"object",
"oblige",
"obscure",
"observe",
"obtain",
"obvious",
"occur",
"ocean",
"october",
"odor",
"off",
"offer",
"office",
"often",
"oil",
"okay",
"old",
"olive",
"olympic",
"omit",
"once",
"one",
"onion",
"online",
"only",
"open",
"opera",
"opinion",
"oppose",
"option",
"orange",
"orbit",
"orchard",
"order",
"ordinary",
"organ",
"orient",
"original",
"orphan",
"ostrich",
"other",
"outdoor",
"outer",
"output",
"outside",
"oval",
"oven",
"over",
"own",
"owner",
"oxygen",
"oyster",
"ozone",
"pact",
"paddle",
"page",
"pair",
"palace",
"palm",
"panda",
"panel",
"panic",
"panther",
"paper",
"parade",
"parent",
"park",
"parrot",
"party",
"pass",
"patch",
"path",
"patient",
"patrol",
"pattern",
"pause",
"pave",
"payment",
"peace",
"peanut",
"pear",
"peasant",
"pelican",
"pen",
"penalty",
"pencil",
"people",
"pepper",
"perfect",
"permit",
"person",
"pet",
"phone",
"photo",
"phrase",
"physical",
"piano",
"picnic",
"picture",
"piece",
"pig",
"pigeon",
"pill",
"pilot",
"pink",
"pioneer",
"pipe",
"pistol",
"pitch",
"pizza",
"place",
"planet",
"plastic",
"plate",
"play",
"please",
"pledge",
"pluck",
"plug",
"plunge",
"poem",
"poet",
"point",
"polar",
"pole",
"police",
"pond",
"pony",
"pool",
"popular",
"portion",
"position",
"possible",
"post",
"potato",
"pottery",
"poverty",
"powder",
"power",
"practice",
"praise",
"predict",
"prefer",
"prepare",
"present",
"pretty",
"prevent",
"price",
"pride",
"primary",
"print",
"priority",
"prison",
"private",
"prize",
"problem",
"process",
"produce",
"profit",
"program",
"project",
"promote",
"proof",
"property",
"prosper",
"protect",
"proud",
"provide",
"public",
"pudding",
"pull",
"pulp",
"pulse",
"pumpkin",
"punch",
"pupil",
"puppy",
"purchase",
"purity",
"purpose",
"purse",
"push",
"put",
"puzzle",
"pyramid",
"quality",
"quantum",
"quarter",
"question",
"quick",
"quit",
"quiz",
"quote",
"rabbit",
"raccoon",
"race",
"rack",
"radar",
"radio",
"rail",
"rain",
"raise",
"rally",
"ramp",
"ranch",
"random",
"range",
"rapid",
"rare",
"rate",
"rather",
"raven",
"raw",
"razor",
"ready",
"real",
"reason",
"rebel",
"rebuild",
"recall",
"receive",
"recipe",
"record",
"recycle",
"reduce",
"reflect",
"reform",
"refuse",
"region",
"regret",
"regular",
"reject",
"relax",
"release",
"relief",
"rely",
"remain",
"remember",
"remind",
"remove",
"render",
"renew",
"rent",
"reopen",
"repair",
"repeat",
"replace",
"report",
"require",
"rescue",
"resemble",
"resist",
"resource",
"response",
"result",
"retire",
"retreat",
"return",
"reunion",
"reveal",
"review",
"reward",
"rhythm",
"rib",
"ribbon",
"rice",
"rich",
"ride",
"ridge",
"rifle",
"right",
"rigid",
"ring",
"riot",
"ripple",
"risk",
"ritual",
"rival",
"river",
"road",
"roast",
"robot",
"robust",
"rocket",
"romance",
"roof",
"rookie",
"room",
"rose",
"rotate",
"rough",
"round",
"route",
"royal",
"rubber",
"rude",
"rug",
"rule",
"run",
"runway",
"rural",
"sad",
"saddle",
"sadness",
"safe",
"sail",
"salad",
"salmon",
"salon",
"salt",
"salute",
"same",
"sample",
"sand",
"satisfy",
"satoshi",
"sauce",
"sausage",
"save",
"say",
"scale",
"scan",
"scare",
"scatter",
"scene",
"scheme",
"school",
"science",
"scissors",
"scorpion",
"scout",
"scrap",
"screen",
"script",
"scrub",
"sea",
"search",
"season",
"seat",
"second",
"secret",
"section",
"security",
"seed",
"seek",
"segment",
"select",
"sell",
"seminar",
"senior",
"sense",
"sentence",
"series",
"service",
"session",
"settle",
"setup",
"seven",
"shadow",
"shaft",
"shallow",
"share",
"shed",
"shell",
"sheriff",
"shield",
"shift",
"shine",
"ship",
"shiver",
"shock",
"shoe",
"shoot",
"shop",
"short",
"shoulder",
"shove",
"shrimp",
"shrug",
"shuffle",
"shy",
"sibling",
"sick",
"side",
"siege",
"sight",
"sign",
"silent",
"silk",
"silly",
"silver",
"similar",
"simple",
"since",
"sing",
"siren",
"sister",
"situate",
"six",
"size",
"skate",
"sketch",
"ski",
"skill",
"skin",
"skirt",
"skull",
"slab",
"slam",
"sleep",
"slender",
"slice",
"slide",
"slight",
"slim",
"slogan",
"slot",
"slow",
"slush",
"small",
"smart",
"smile",
"smoke",
"smooth",
"snack",
"snake",
"snap",
"sniff",
"snow",
"soap",
"soccer",
"social",
"sock",
"soda",
"soft",
"solar",
"soldier",
"solid",
"solution",
"solve",
"someone",
"song",
"soon",
"sorry",
"sort",
"soul",
"sound",
"soup",
"source",
"south",
"space",
"spare",
"spatial",
"spawn",
"speak",
"special",
"speed",
"spell",
"spend",
"sphere",
"spice",
"spider",
"spike",
"spin",
"spirit",
"split",
"spoil",
"sponsor",
"spoon",
"sport",
"spot",
"spray",
"spread",
"spring",
"spy",
"square",
"squeeze",
"squirrel",
"stable",
"stadium",
"staff",
"stage",
"stairs",
"stamp",
"stand",
"start",
"state",
"stay",
"steak",
"steel",
"stem",
"step",
"stereo",
"stick",
"still",
"sting",
"stock",
"stomach",
"stone",
"stool",
"story",
"stove",
"strategy",
"street",
"strike",
"strong",
"struggle",
"student",
"stuff",
"stumble",
"style",
"subject",
"submit",
"subway",
"success",
"such",
"sudden",
"suffer",
"sugar",
"suggest",
"suit",
"summer",
"sun",
"sunny",
"sunset",
"super",
"supply",
"supreme",
"sure",
"surface",
"surge",
"surprise",
"surround",
"survey",
"suspect",
"sustain",
"swallow",
"swamp",
"swap",
"swarm",
"swear",
"sweet",
"swift",
"swim",
"swing",
"switch",
"sword",
"symbol",
"symptom",
"syrup",
"system",
"table",
"tackle",
"tag",
"tail",
"talent",
"talk",
"tank",
"tape",
"target",
"task",
"taste",
"tattoo",
"taxi",
"teach",
"team",
"tell",
"ten",
"tenant",
"tennis",
"tent",
"term",
"test",
"text",
"thank",
"that",
"theme",
"then",
"theory",
"there",
"they",
"thing",
"this",
"thought",
"three",
"thrive",
"throw",
"thumb",
"thunder",
"ticket",
"tide",
"tiger",
"tilt",
"timber",
"time",
"tiny",
"tip",
"tired",
"tissue",
"title",
"toast",
"tobacco",
"today",
"toddler",
"toe",
"together",
"toilet",
"token",
"tomato",
"tomorrow",
"tone",
"tongue",
"tonight",
"tool",
"tooth",
"top",
"topic",
"topple",
"torch",
"tornado",
"tortoise",
"toss",
"total",
"tourist",
"toward",
"tower",
"town",
"toy",
"track",
"trade",
"traffic",
"tragic",
"train",
"transfer",
"trap",
"trash",
"travel",
"tray",
"treat",
"tree",
"trend",
"trial",
"tribe",
"trick",
"trigger",
"trim",
"trip",
"trophy",
"trouble",
"truck",
"true",
"truly",
"trumpet",
"trust",
"truth",
"try",
"tube",
"tuition",
"tumble",
"tuna",
"tunnel",
"turkey",
"turn",
"turtle",
"twelve",
"twenty",
"twice",
"twin",
"twist",
"two",
"type",
"typical",
"ugly",
"umbrella",
"unable",
"unaware",
"uncle",
"uncover",
"under",
"undo",
"unfair",
"unfold",
"unhappy",
"uniform",
"unique",
"unit",
"universe",
"unknown",
"unlock",
"until",
"unusual",
"unveil",
"update",
"upgrade",
"uphold",
"upon",
"upper",
"upset",
"urban",
"urge",
"usage",
"use",
"used",
"useful",
"useless",
"usual",
"utility",
"vacant",
"vacuum",
"vague",
"valid",
"valley",
"valve",
"van",
"vanish",
"vapor",
"various",
"vast",
"vault",
"vehicle",
"velvet",
"vendor",
"venture",
"venue",
"verb",
"verify",
"version",
"very",
"vessel",
"veteran",
"viable",
"vibrant",
"vicious",
"victory",
"video",
"view",
"village",
"vintage",
"violin",
"virtual",
"virus",
"visa",
"visit",
"visual",
"vital",
"vivid",
"vocal",
"voice",
"void",
"volcano",
"volume",
"vote",
"voyage",
"wage",
"wagon",
"wait",
"walk",
"wall",
"walnut",
"want",
"warfare",
"warm",
"warrior",
"wash",
"wasp",
"waste",
"water",
"wave",
"way",
"wealth",
"weapon",
"wear",
"weasel",
"weather",
"web",
"wedding",
"weekend",
"weird",
"welcome",
"west",
"wet",
"whale",
"what",
"wheat",
"wheel",
"when",
"where",
"whip",
"whisper",
"wide",
"width",
"wife",
"wild",
"will",
"win",
"window",
"wine",
"wing",
"wink",
"winner",
"winter",
"wire",
"wisdom",
"wise",
"wish",
"witness",
"wolf",
"woman",
"wonder",
"wood",
"wool",
"word",
"work",
"world",
"worry",
"worth",
"wrap",
"wreck",
"wrestle",
"wrist",
"write",
"wrong",
"yard",
"year",
"yellow",
"you",
"young",
"youth",
"zebra",
"zero",
"zone",
"zoo"
};


   static std::random_device rd;
   static std::mt19937 engine(rd());
   static std::uniform_int_distribution<> get_index(0, (words.size()-1));
   return words[get_index(engine)];
}
