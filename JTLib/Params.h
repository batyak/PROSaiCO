#ifndef PARAMS_H
#define PARAMS_H
#include <string>
#include "argvparser.h"
#include "Convert.h"


using namespace CommandLineProcessing;
using namespace std;
class Params{
private:
	Params(){
		CDCL = false;
		DYNAMIC_SF = true;
		ADD_LIT_WEIGHTS = false;
		timeoutSec = 10000;
		seed = 0;
		ibound = 0;
		MPE_bound=0;
		ishelp=false;
	}
	static Params* inst;

	int getBound(string boundFile){
		cout << " bound file: " << boundFile << endl;
		if(boundFile.empty() || boundFile=="")
			return 0;
		ifstream boundStream(boundFile);
		string line;
		if(getline(boundStream,line) != NULL){
			cout << " read line " << line << endl;
			line.shrink_to_fit();
			return atoi(line.c_str());
		}
		return 0;
	}

	string paramsAsStr;
public:
	static Params& instance();
	static Params* x_instance();
	bool CDCL;	
	bool DYNAMIC_SF;
	unsigned int timeoutSec;
	unsigned int seed;
	std::string dtreeFile;
	std::string weightsFile;
	std::string pmapFile; //the determined vars which should receive less priority during instantiation
	std::string csvOutFile;
	bool ADD_LIT_WEIGHTS;

	size_t ibound; //for "mini-buckets" MPE
	double MPE_bound;
	string cnfFile;
	bool ishelp;
	/*	cmd.defineOption("timeout"," timeout in seconds",ArgvParser::OptionRequiresValue);
		cmd.defineOptionAlternative("timeout","t");

		cmd.defineOption("CDCL"," apply conflic directed clause learning",ArgvParser::OptionRequiresValue);
		cmd.defineOptionAlternative("CDCL","l");

		cmd.defineOption("seed"," seed for rerunning",ArgvParser::OptionRequiresValue);
		cmd.defineOptionAlternative("seed","s");

		cmd.defineOption("dtreeFile","file containing dtree",ArgvParser::OptionRequiresValue);
		cmd.defineOptionAlternative("dtree","r");


		cmd.defineOption("pmapFile","",ArgvParser::OptionRequiresValue);
		cmd.defineOptionAlternative("pmapFile","p");

		cmd.defineOption("addMode","",ArgvParser::OptionRequiresValue);
		cmd.defineOptionAlternative("addMode","a");

		cmd.defineOption("bound","",ArgvParser::OptionRequiresValue);
		cmd.defineOptionAlternative("bound","b");

		cmd.defineOption("ibound","",ArgvParser::OptionRequiresValue);*/
	void loadFromProps(ArgvParser& args){
		std::stringstream ss;
		if(args.foundOption("help")){			
			ishelp=true;
		}

		if(args.foundOption("cnfFile")){
			cnfFile = args.optionValue("cnfFile");
			ss << "cnfFile: " << cnfFile << endl;
		}

		if(args.foundOption("timeout")){
			string timeoutStr = args.optionValue("timeout");
			timeoutSec = Convert::string_to_T<unsigned int>(timeoutStr);
			ss << "timeout: " << timeoutSec << endl;
		}

		if(args.foundOption("CDCL")){
			string CDCLStr = args.optionValue("CDCL");
			CDCL = (Convert::string_to_T<unsigned int>(CDCLStr) > 0);
			ss << "CDCL: " << CDCL << endl;
		}

		if(args.foundOption("seed")){
			string seedStr = args.optionValue("seed");
			seed = Convert::string_to_T<unsigned int>(seedStr);
			//ss << "seed: " << seed << endl;
		}

		if(args.foundOption("dtreeFile")){
			dtreeFile = args.optionValue("dtreeFile");			
			ss << "dtreeFile: " << dtreeFile << endl;
		}

		if(args.foundOption("pmapFile")){
			pmapFile = args.optionValue("pmapFile");			
			//ss << "pmapFile: " << pmapFile << endl;
		}

		if(args.foundOption("addMode")){
			string addModeStr = args.optionValue("addMode");
			ADD_LIT_WEIGHTS = (Convert::string_to_T<unsigned int>(addModeStr) > 0);
			//ss << "ADD_LIT_WEIGHTS: " << ADD_LIT_WEIGHTS << endl;
		}

		if(args.foundOption("ibound")){
			string iboundStr = args.optionValue("ibound");
			ibound = Convert::string_to_T<unsigned int>(iboundStr);
			ss << "ibound: " << ibound << endl;
		}

		if(args.foundOption("bound")){
			string boundFile = args.optionValue("bound");
			MPE_bound=getBound(boundFile);
			ss << "bound: " << MPE_bound << endl;
		}
		paramsAsStr = ss.str();
	}

	string toString() const{
		return paramsAsStr;
	}
};




#endif
