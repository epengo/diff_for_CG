#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <algorithm>

#include "inc/Labels.h"
#include "inc/Loader.h"
#include "inc/Loader_soot.h"
#include "inc/Loader_callerhierarchy.h"
#include "inc/Loader_sourcemeter.h"
#include "inc/Loader_gousiosg.h"
#include "inc/Loader_wala.h"
#include "inc/Switch.h"
#include "inc/common.h"
#include "inc/Record.h"
#include "inc/Factory.h"
#include "inc/OptionMethods.h"
#include "inc/Option.h"

#if defined(VERBOSE)
  #define VERBOSE1 \
    cout << "------" << loaders[i] << "--------" << endl;\
    for ( pair<int, int> it : connections[i] ) {\
      \
      cout << it.first << " " << it.second << endl;\
    }\
    cout << "-------------------------------------" << endl;\
    cout << loaders[i]->getMethodNum() << " method has been processed and " << endl << loaders[i]->getCallNum() << "call has been transformed." << endl;\
    cout << "-------end of " << loaders[i] << "--------" << endl;
#else
  #define VERBOSE1 ;
#endif


using namespace std;

static void makeStat(set<pair<int, int>> compareSet1, set<pair<int, int>> compareSet2, Loader* l1, Loader* l2, vector<Record> r1, vector<Record> r2);

static void writeTSV(vector<Record>, string, string);
static void writeConnTSV( set<pair<int, int>>, string);

int main( int argc, char** argv ) {

  if ( argc < 3 ) {
    
    cout << "At least 1 tool have to be given." << endl;
    return 1;
  }
  
  cout << "Starting transforming and making stat..." << endl;
  
  try { 

  Factory factory = Factory::createFactory();
  
  Switch* switches[] = {
                          new Switch("-s", factory ),
                          new Switch("-c", factory ),
                          new Switch("-sm", factory ),
                          new Switch("-sp", factory ),
                          new Switch("-g", factory ),
                          new Switch("-w", factory ),
                          NULL
                        };

  Option* options[] = {
                          new Option("-projectName", &projectNameMethod),
                          new Option("-CHPtransformation", &cHPTransformationMethod),
                          new Option("-anonymTransformation", &anonymClassNameTransformationMethod),
                          NULL
                      };
    
  //need pointer otherwise vector do not accept as Loader is abstract
  vector<Loader*> loaders;

Switch* chp = NULL;
int chpArgIndex = -1;

  unsigned char j = -1;
  while( switches[++j] ) {
    
    for ( int i = 1; i < argc - 1; i++ ) {
      
      if ( *(switches[j]) == argv[i] ) {
        
        if ( *(switches[j]) == "-c" ) {
          //if it is chp note it and add it later so it wold run for last.
          chp = switches[j];
          chpArgIndex = ++i;
        }
        else {
          //it is not chp. add it.
          switches[j]->init(argv[++i]);
          loaders.push_back( switches[j]->getLoaderPointer() );
        }
      }
    }
  }
  
  if ( chp ) {
    
    chp->init(argv[chpArgIndex]);
    loaders.push_back( chp->getLoaderPointer() );
  }
    
  if ( 0 == loaders.size() )
    throw Labels::NO_LOADER_WERE_GIVEN;
  
  j = -1;
  while( options[++j] ) {
    
    for ( int i = 1; i < argc - 1; i++ ) {
      
      if ( *(options[j]) == argv[i] ) {
        
        options[j]->foo(argv, i);
        break;
      }
    }
  }

  //create an array for the transformed connections
  std::vector<set<pair<int, int>>> connections;
  connections.resize(loaders.size());
  std::vector<vector<Record>> records;
  records.resize(loaders.size());

  for (unsigned i = 0; i < loaders.size(); ++i ) {
    
    records[i] = loaders[i]->load();
    connections[i] = loaders[i]->transformConnections();
VERBOSE1
  }

  //start generating various outputs, statistics..
  for ( unsigned i = 0; i < records.size(); i++ ) {
    
    writeTSV(records[i], loaders[i]->getName(), loaders[i]->getName());
    common::tsvFiles.push_back(Labels::PROJECT_NAME + loaders[i]->getName() + "loadedMethods.tsv");
  }
  
  {
    sort( common::tsvFiles.begin(), common::tsvFiles.end() );
    ofstream TSVFILE("tsvfiles.list");
    if ( !TSVFILE.is_open() )
      throw Labels::COULD_NOT_WRITE_FILE_LIST;
    
    for ( unsigned i = 0; i < common::tsvFiles.size(); i++ ) {
      
      TSVFILE << common::tsvFiles[i] << endl;
    }
  }
  
  for ( unsigned i = 0; i < connections.size(); i++ ) {
    
    writeConnTSV(connections[i], loaders[i]->getName());
    common::connTSVFiles.push_back(Labels::PROJECT_NAME + loaders[i]->getName() + "connections.tsv");
  }
  
  {
    sort( common::connTSVFiles.begin(), common::connTSVFiles.end() );
    ofstream CONNTSVFILE("conntsvfiles.list");
    if ( !CONNTSVFILE.is_open() )
      throw Labels::COULD_NOT_WRITE_FILE_LIST;
    
    for ( unsigned i = 0; i < common::connTSVFiles.size(); i++ ) {
      
      CONNTSVFILE << common::connTSVFiles[i] << endl;
    }
  }
    
  for (unsigned i = 0; i < loaders.size() - 1; i++ ) {
    
    for (unsigned j = i + 1; j < loaders.size(); j++ ) {
      
      makeStat( connections[i], connections[j], loaders[i], loaders[j], records[i], records[j] );
    }
  }
  
  //catch "all" thrown error...
  } catch( const string e ) {
    
    cerr << "An error has occurred: " << e << endl;
  }

  cout << "End of program." << endl;

  system("./commonTSV tsvfiles.list conntsvfiles.list");
  
  return 0;
}
//########################################################################x
//########################################################################x
//########################################################################x
//########################################################################x

static void writeConnTSV( set<pair<int, int>> connections, string name) {
  
  ofstream TSV(Labels::PROJECT_NAME + name + "connections.tsv");
  
  if ( !TSV.is_open() )
    throw Labels::COULD_NOT_WRITE_TSV;
  
  TSV << "caller->callee" << "\t" << name << endl;
  
  for ( pair<int, int> it : connections ) {
    
    TSV << "(" << it.first << ")" << common::getMethodById(it.first) << "->(" << it.second << ")" << common::getMethodById(it.second) << "\t" << name << endl;
  }
}

static void writeTSV( vector<Record> records, string name, string tool ) {
  
  ofstream TSV(Labels::PROJECT_NAME + tool + "loadedMethods.tsv");
  
  if ( !TSV.is_open() )
    throw Labels::COULD_NOT_WRITE_TSV;
  
  TSV << "name" << "\t" << "transformed-rep." << "\t" << "tool.rep" << endl;
  
  for ( unsigned i = 0; i < records.size(); i++ ) {
    
    TSV << name << "\t" << records[i] << "\t";
    //TODO: nem csak kiíratom, vagy a kiíratás lehetne mostmár szebb is
    
    if ( records[i].getSameMethods().size() > 1 )
      throw Labels::TOOL_HAS_MORE_THAN_ONE_REP + name;
    
    TSV << records[i].getSameMethods().at(0).first + records[i].getSecondaryRepresentation() << endl;
  }
}

static void makeStat(set<pair<int, int>> compareSet1, set<pair<int, int>> compareSet2, Loader* l1, Loader* l2, vector<Record> r1, vector<Record> r2 ) {
  
  unsigned long long commonCalls = 0;
  unsigned long long commonCallsCheck = 0;
  unsigned long long commonMethods = 0;
  unsigned long long commonMethodsCheck = 0;
  
  vector<Record> onlyFirst;
  vector<Record> onlySecond;
  
  vector<pair<int, int>> onlyFirstCall;
  vector<pair<int, int>> onlySecondCall;
  
  ofstream statOut(Labels::PROJECT_NAME + l1->getName() + "-" + l2->getName() + "-common-calls.txt");
  
  //check calls
  if ( !statOut.is_open() )
    throw Labels::COULD_NOT_WRITE_OUTPUT;

  for ( auto it1 : compareSet1 ) {
    
    if ( compareSet2.find(it1) != compareSet2.end() ) {
      //found
      ++commonCalls;
    }
    else {
      
      onlyFirstCall.push_back(it1);
    }
  }
  
  for ( auto it2 : compareSet2 ) {
    
    if ( compareSet1.find(it2) != compareSet1.end() ) {
      //found
      ++commonCallsCheck;
    }
    else {

      onlySecondCall.push_back(it2);
    }
  }
  
  if ( commonCalls != commonCallsCheck )
    cerr << "The search for common calls failed" << endl;
   
  //check the methods
  for ( unsigned i = 0; i < r1.size(); i++ ) {
    
    if ( find(r2.begin(), r2.end(), r1[i]) != r2.end() ) {
      
      ++commonMethods;
    }
    else {
      //this method is not in the second tool's vector
      onlyFirst.push_back(r1[i]);
    }
  }
  
  for ( unsigned i = 0; i < r2.size(); i++ ) {
    
    if ( find(r1.begin(), r1.end(), r2[i]) != r1.end() ) {
      
      ++commonMethodsCheck;
    }
    else {
      //this method is not in the second tool's vector
      onlySecond.push_back(r2[i]);
    }
  }
  
  if ( commonMethodsCheck != commonMethods )
    cerr << "The search for common methods failed" << endl;
  
  statOut << l1->getFilePath() << " has " << l1->getCallNum() << " calls" << " and " << l1->getMethodNum() << " methods. " << l1->getUniqueMethodNum() << " unique method." << endl;
  statOut << l2->getFilePath() << " has " << l2->getCallNum() << " calls" << " and " << l2->getMethodNum() << " methods. " << l2->getUniqueMethodNum() << " unique method." << endl;
  statOut << commonCalls << " common calls and " << commonMethods << " common methods." << endl;
  
  //write the differences
  statOut << "Only the " << l1->getName() << " contains this/these method(s):" << endl;
  
  for ( unsigned i = 0; i < onlyFirst.size(); i++ ) {
    
    statOut << onlyFirst[i] << endl;
  }
  
  statOut << "Only the " << l2->getName() << " contains this/these method(s):" << endl;
  
  for ( unsigned i = 0; i < onlySecond.size(); i++ ) {
    
    statOut << onlySecond[i] << endl;
  }
  
  statOut << "Only the " << l1->getName() << " contains this/these call(s):" << endl;
  
  for ( unsigned i = 0; i < onlyFirstCall.size(); i++ ) {
    
    statOut << onlyFirstCall[i].first << " [" << common::getMethodById(onlyFirstCall[i].first) << "] " << onlyFirstCall[i].second << " [" << common::getMethodById(onlyFirstCall[i].second) << " ]" << endl;
  }
  
  statOut << "Only the " << l2->getName() << " contains this/these call(s):" << endl;
  
  for ( unsigned i = 0; i < onlySecondCall.size(); i++ ) {
    
    statOut << onlySecondCall[i].first << " [" << common::getMethodById(onlySecondCall[i].first) << "] " << onlySecondCall[i].second << " [" << common::getMethodById(onlySecondCall[i].second) << " ]" << endl;
  }
  
  statOut.close();
}
