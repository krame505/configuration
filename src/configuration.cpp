/**
 * \author Lucas Kramer
 * \editor Carl Bahn
 * \file  configuration.cpp
 * \brief Implementation for configuration file loader.  
 * \details See config.h for more information.  
 */

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
using namespace std;
using namespace boost;

#include "configuration.h"

/**
 * \brief A type alias for an unordered map containing strings mapped to configValues.  
 */
typedef unordered_map<string, configValue> ConfigTable;

//Initially set m_instance to NULL
Configuration *Configuration::m_instance = NULL;
string Configuration::configFile;
ConfigTable Configuration::userDefs;

/**
 * \brief A helper function for loadConfig that tests if the line is valid input.  
 */
bool isAllWhitespace(const string &line);

configValue parseValue(const string &type, const string &valueText) {
  const char *cValueText = valueText.c_str();

  // Parse the value
  istringstream iss(valueText);
  configValue value;
  value.type = type;

  bool formatError = false;
  bool used_iss = false;
  if (type == "int") {
    iss >> value.intVal;
    used_iss = true;
  }
  else if (type == "hex") {
    iss >> hex >> value.intVal;
    used_iss = true;
    value.type = "int";
  }
  else if (type == "octal") {
    iss >> oct >> value.intVal;
    used_iss = true;
    value.type = "int";
  }
  else if (type == "float") {
    iss >> value.floatVal;
    used_iss = true;
  }
  else if (type == "bool" || type == "boolean") {
    if (valueText == "true" || valueText == "1")
      value.boolVal = true;
    else if (valueText == "false" || valueText == "0")
      value.boolVal = false;
    else formatError = true;
    value.type = "bool";
  }
  else if (type == "char") {
    if (sscanf(cValueText, "'%c'", &value.charVal) < 1 &&
        sscanf(cValueText, "%c", &value.charVal) < 1)
      formatError = true;
  }
  else if (type == "string") {
    value.stringVal = new char[strlen(cValueText) + 1]; // TODO: Make sure this gets freed in where used when it is no longer needed!  
    if (sscanf(cValueText, "\"%[^\"]\"", (char *)value.stringVal) < 1 &&
        sscanf(cValueText, "%[^\"]", (char *)value.stringVal) < 1)
      formatError = true;
  }
  else {
    value.type = "invalid type name";
    return value;
  }

  // Check for syntax errors
  if ((used_iss && (!iss.eof() || iss.fail())) || formatError) {
    value.type = "invalid syntax";
    return value;
  }

  return value;
}

ConfigTable mergeConfigTables(ConfigTable c1, ConfigTable c2) {
  for (ConfigTable::iterator it = c2.begin(); it != c2.end(); it++) {
    c1[it->first] = it->second;
  }
  return c1;
}

// This is a helper function
ConfigTable loadConfig(const string &filename) {
  ifstream input(filename);
  ConfigTable result;

  // Check if the file can be opened
  if (input.is_open()) {
    string line;

    // Iterate over all lines and update the line number
    for (int lineNum = 1; getline(input, line); lineNum++) {
      if (!isAllWhitespace(line)) {
        // Check if the line is an include
        regex includeParse("use \"(.*)\"");
        smatch parseResult; // parseResult[0] is the whole string
        if (regex_match(line, parseResult, includeParse)) {
          regex filenameParse("/[^/]*$");
          string newFilename = replace_regex_copy(filename, filenameParse, "/" + parseResult[1]);
          ConfigTable t = loadConfig(newFilename);
        
          //result.insert(t.begin(), t.end()); //Doesn't overwrite
          for (ConfigTable::iterator it = t.begin(); it != t.end(); it++) {
            if (result.find(it->first) != result.end()) {
              cerr << "Warning when parsing include of configuration file " << newFilename <<
                ": Configuration variable " << it->first << " is already bound" << endl;
            }
            result[it->first] = it->second;
          }
        }
        else {
          // Parse line into type, name, and value.  
          regex lineParse("([A-Za-z][A-Za-z0-9_-]*) +([A-Za-z][A-Za-z0-9_-]*) *= *((?:[^\n# ]|\".*\")*) *(?:#.*)?");
          if (!regex_match(line, parseResult, lineParse)) {
            cerr << "Syntax error when parsing configuration file " << filename << " at line " << lineNum <<
              ": Unexpected end of line" << endl;
            exit(1);
          }
          string type(parseResult[1]);
          string name(parseResult[2]);
          string valueText(parseResult[3]);
          trim_left(valueText);

          // Parse the value and check for errors
          configValue value = parseValue(type, valueText);
          if (value.type == "invalid type name") {
            cerr << "Syntax error when parsing configuration file " << filename << " at line " << lineNum <<
              ": Invalid type name " << type << endl;
            exit(1);
          }
          else if (value.type == "invalid syntax") {
            cerr << "Syntax error when parsing configuration file " << filename << " at line " << lineNum <<
              ": Invalid value format" << endl;
            exit(1);
          }

          // Add the value to the result table.  
          if (result.find(name) != result.end()) {
            cerr << "Warning when parsing configuration file " << filename << " at line " << lineNum <<
              ": Configuration variable " << name << " is already bound" << endl;
          }
          value.type =
            type == "hex" || type == "octal"? "int" :
            type == "boolean"? "bool" :
            type;
          result[name] = value;
        }
      }
    }
    input.close();
  }
  else {
    cerr << "Could not find configuration file " << filename << endl;
    exit(1);
  }
  return result;
}

void Configuration::initConfig(int argc, char *argv[], const string &defaultFilename) {
  configFile = defaultFilename;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--use-config") == 0 && i < argc - 1) {
      configFile = argv[i + 1];
      i++;
    }
    else if (strcmp(argv[i], "--add-config") == 0 && i < argc - 1) {
      userDefs = mergeConfigTables(loadConfig(argv[i + 1]), userDefs);
      i++;
    }
    else if (strncmp(argv[i], "-D", 2) == 0) {
      string name(argv[i] + 2);
      if (i == argc - 1) {
        cerr << "Syntax error when parsing user-set configuration variable " << name <<
          ": Missing type" << endl;
        exit(1);
      }
      else if (i == argc - 2) {
        cerr << "Syntax error when parsing user-set configuration variable " << name <<
          ": Missing type or value" << endl;
        exit(1);
      }
      string type(argv[i + 1]);
      string value(argv[i + 2]);
      configValue v = parseValue(type, value);
      if (v.type == "invalid type name") {
        cerr << "Syntax error when parsing user-set configuration variable " << name <<
          ": Invalid type name " << type << endl;
        exit(1);
      }
      else if (v.type == "invalid syntax") {
        cerr << "Syntax error when parsing user-set configuration variable " << name <<
          ": Invalid value format" << endl;
        exit(1);
      }
      userDefs[name] = v;
      i += 2;
    }
  }
}

void Configuration::initConfig(const string &filename) {
  configFile = filename;
}

//This gets the global config, and creates it if needed
Configuration* Configuration::get() {
  if (m_instance == NULL) {
    m_instance = new Configuration();
    m_instance->config = mergeConfigTables(loadConfig(configFile), userDefs);
  }
  return m_instance;
}

void Configuration::refresh() {
  if (m_instance != NULL)
    delete m_instance;
  m_instance = NULL;
}

bool isAllWhitespace(const string &line) {
  // Ignore comments and empty lines
  bool allWhitespace = true;
  for (int i = 0, line_len = line.length(); i < line_len; i++) {
    if (line[i] == '#') {
      break;
    }
    else allWhitespace &= line[i] == ' ';
  }
  return allWhitespace;
}

int Configuration::getIntConfig(const string &name) {
  if (config.find(name) == config.end()) {
    cerr << "Could not find configuration variable " << name << endl;
    exit(1);
  }
  else if (config[name].type != "int") {
    cerr << "Incompatable type for configuration variable " << name << ": " <<
      "Looked for int, but found " << config[name].type << endl;
    exit(1);
  }
  else return config[name].intVal;
}

float Configuration::getFloatConfig(const string &name) {
  if (config.find(name) == config.end()) {
    cerr << "Could not find configuration variable " << name << endl;
    exit(1);
  }
  else if (config[name].type != "float") {
    cerr << "Incompatable type for configuration variable " << name << ": " <<
      "Looked for float, but found " << config[name].type << endl;
    exit(1);
  }
  else return config[name].floatVal;
}

bool Configuration::getBoolConfig(const string &name) {
  if (config.find(name) == config.end()) {
    cerr << "Could not find configuration variable " << name << endl;
    exit(1);
  }
  else if (config[name].type != "bool") {
    cerr << "Incompatable type for configuration variable " << name << ": " <<
      "Looked for bool, but found " << config[name].type << endl;
    exit(1);
  }
  else return config[name].boolVal;
}

char Configuration::getCharConfig(const string &name) {
  if (config.find(name) == config.end()) {
    cerr << "Could not find configuration variable " << name << endl;
    exit(1);
  }
  else if (config[name].type != "char") {
    cerr << "Incompatable type for configuration variable " << name << ": " <<
      "Looked for char, but found " << config[name].type << endl;
    exit(1);
  }
  else return config[name].charVal;
}


string expandVar(const smatch &match) {
  string varName(match[1]);
  return Configuration::get()->getStringConfig(varName);
}

string Configuration::getStringConfig(const string &name) {
  if (config.find(name) == config.end()) {
    cerr << "Could not find configuration variable " << name << endl;
    exit(1);
  }
  else if (config[name].type != "string") {
    cerr << "Incompatable type for configuration variable " << name << ": " <<
      "Looked for string, but found " << config[name].type << endl;
    exit(1);
  }
  else {
    string str(config[name].stringVal);
    regex varSearch("\\$([A-Za-z][A-Za-z0-9_-]*)");
    return regex_replace(str, varSearch, expandVar);
  }
}
