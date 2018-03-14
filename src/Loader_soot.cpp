#ifdef DEBUG
#define DDD cout << "New record is added...: " << f_classWithPckg << " " << method << " " << endl;
#else 
#define DDD ;
#endif

#include "../inc/Loader_soot.h"
#include "../inc/common.h"

#include <set>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;

Loader_soot::Loader_soot( string filepath, string name ) : Loader(filepath, name) {
}
Loader_soot::~Loader_soot() {
}

vector<Record> Loader_soot::load() {
  
  vector<Record> tmpRecords; 
  
  string line;
  getline(input, line); //get the "header" line
  
  while ( getline(input, line) ) {
    
    if ( line.find("->") == string::npos && line != "}" ) {
      //the line doesn't contains the "->" sub-string so it is not a caller-calle connection but a node
      ++methodNum;
      
      string methodRepresentation = line; //save the line for representation
      methodRepresentation.erase(0, 4); //the leading spaces
      
      line.erase(0, 6); //remove the '<4 spaces>"<' beginning
      line.erase(line.length()-2, 2); //remove the '">' ending
      
      //get the needed info. for a Record then algorithm::find(.., r) can be used...
     //----------------------------------------------------------------------------------------
     
      string f_classWithPckg, tmp, method;
      stringstream input_stringstream(line);

      getline(input_stringstream, f_classWithPckg , ' ');
      getline(input_stringstream, tmp , ' ');
      getline(input_stringstream, method , ' ');

      f_classWithPckg.erase(f_classWithPckg.length()-1, 1); //removing the ":"
      
      string parameters = method.substr(method.find("("));
      method.erase(method.find("("), method.length()-method.find("("));
      parameters.erase(parameters.length()-1, 1);
      parameters.erase(0,1);
      
      //cut apart parameters
      vector<string> parameterVector;
      stringstream iss(parameters);
      string parameter;
      while( getline(iss, parameter, ',') ) {
        
        parameterVector.push_back(parameter);
      }
      
      Record r(methodRepresentation, f_classWithPckg, method, parameterVector);
      tmpRecords.push_back( r );
      
      if ( find( common::storedIds.begin(), common::storedIds.end(), r ) == common::storedIds.end() ) {
        //so this record is not found in the vector
DDD
        ++uniqueMethodNum;
        common::storedIds.push_back(r);
      }
      else {

        auto it = find( common::storedIds.begin(), common::storedIds.end(), r );
        if ( *it == methodRepresentation ) {
          //contains this representation
        }
        else {
          
          *it += methodRepresentation;  //add this representation
          ++uniqueMethodNum;
        }
      }
      
    }

  }
  input.clear();
  input.seekg(0, ios::beg);
  
  return tmpRecords;
}

set<pair<int, int>> Loader_soot::transformConnections() {
  
  set<pair<int, int>> connections;
  
  string line;
  getline(input, line); //get the "header" line
  
  while ( getline(input, line) ) {
    
    if ( line.find("->") != string::npos && line != "}" ) {
      //it is a connection
      ++callNum;
      
      line.erase(line.length()-1, 1); //as connections are closed by ";"
      
      string caller = line.substr(0, line.find("->"));  //left part
      caller.erase(0, 4); //removal of leading spaces
      string callee = line.substr(line.find("->")+2);  //right part
      int callerId = -1, calleeId = -1;
      
      bool check = false; //to check if the method do be found.
      
      for ( int i = 0; i < common::storedIds.size(); i++ ) {
        
        if ( common::storedIds[i] == caller ) {
          
          check = true;
          callerId = i;
          break;
        }
      }
      if ( !check ) {
        
        cerr << "Method couldn't be resolved: " << caller << endl;
      }
      
      check = false;
      for ( int i = 0; i < common::storedIds.size(); i++ ) {
        
        if ( common::storedIds[i] == callee ) {
          
          check = true;
          calleeId = i;
          break;
        }
      }
      if ( !check ) {
        
        cerr << "Method couldn't be resolved: " << callee << endl;
      }
      
      connections.insert(pair<int, int>(callerId, calleeId));
    }
  }
  
  return connections;
}