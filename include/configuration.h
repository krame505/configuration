#pragma once

/**
 * \author Lucas Kramer
 *
 * \file  configuration.h
 * \brief Interface definitions for configuration file loader.  
 * \details The format of the config file is a series of lines with the format<br>
 * \<type\> \<name\> = \<value\><br>
 * Blank lines are allowed, and comments # can occur at the end of a line.  <br>
 * Valid types are int, hex, octal, float, bool/boolean, char, and string.  
 * Note that int, hex, and octal all create a variable with type int, and only
 * affect how the value is parsed.  <br>
 * Names can be [A-Za-z][A-Za-z0-9_-]*, but the recommended usage is all upper
 * case, seperated by underscores.  <br>
 * The following is a list of valid value formats:
 * <table style="width:100%">
 *  <tr>
 *    <td><b>Type name</b></td>
 *    <td><b>Format</b></td>
 *    <td><b>Examples</b></td>
 *    <td><b>Notes</b></td>
 *  </tr>
 *  <tr>
 *    <td>int</td>
 *    <td>[0-9]+</td>
 *    <td>42</td>
 *  </tr>
 *  <tr>
 *    <td>hex</td>
 *    <td>(0x)?[0-9A-Fa-f]+</td>
 *    <td>0x3AF4, 0x3af4, 3AF4</td>
 *    <td>It is recommended to use the 0x prefix and upper case A-F</td>
 *  </tr>
 *  <tr>
 *    <td>hex</td>
 *    <td>[0-7]+</td>
 *    <td>0123, 123</td>
 *    <td>It is recommended to use the 0 prefix</td>
 *  </tr>
 *  <tr>
 *    <td>float</td>
 *    <td>[0-9]+(.[0-9]+)?</td>
 *    <td>3, 3.14</td>
 *  </tr>
 *  <tr>
 *    <td>bool/boolean</td>
 *    <td>true|false</td>
 *    <td>true, false</td>
 *    <td>It is recommended to use the bool type name</td>
 *  </tr>
 *  <tr>
 *    <td>string</td>
 *    <td>\"[^\"\\n]*\"</td>
 *    <td>"Hello, World!  ", "", "$OTHER"</td>
 *    <td>$ followed by a variable is expanded to that variable.  
 *        An actual $ must be escaped</td>
 *  </tr>
 *  <tr>
 *    <td>char</td>
 *    <td>'([^\\n\\r\\t]|\\\\n|\\\\r|\\\\t)'</td>
 *    <td>'a', 'A', '4', '\\n', '\\t'</td>
 *  </tr>
 * </table><br>
 * In addition to variable settings, it is possible to load other configuration
 * files.  This has the syntax:<br>
 * use \<filename\><br>
 * where filename is a string.  The path to <filename> is with respect to the
 * directory containing the current file - absolute paths are not permitted.  
 * When an included file contains a variable with the same name as a value
 * that is already defined, the current behavior is to overwrite the old value.  
 * Any value re-definition causes a warning.  
 */
 
#include <string>
#include <unordered_map>

/**
 * \brief This struct holds a configuration value of any legal type.  
 * \details Note that stringVal is stored as a const char * in the union due to
 * restrictions in C++ not allowing complex types in unions.  The string is
 * converted to a char * after it is parsed, and is converted back in
 * getStringVal.  
 */
typedef struct {
  /**
   * The name of the type that is stored ("int", "float", "char", "bool", or
   * "string") or an error code from parsing ("invalid type name" or "invalid
   * syntax").
   */
  std::string type;
  
  /** An anonymous union of all possible types that can be stored.  */
  union {
    int intVal;
    float floatVal;
    char charVal;
    bool boolVal;
    const char *stringVal;
  };
} configValue;

// Configuration Class
class Configuration {  
 public:
  static void initConfig(const std::string &filename);
  static void initConfig(int argc, char *argv[], const std::string &defaultFilename);

  static Configuration* get();
  static void refresh();

  // Functions to get config values
  /**
   * \brief Looks up an int
   * \param table The table from which to look up the value
   * \param name The name of the value
   * \return The value
   */
  int getIntConfig(const std::string &name);

  /**
   * \brief Looks up a float
   * \param table The table from which to look up the value
   * \param name The name of the value
   * \return The value
   */
  float getFloatConfig(const std::string &name);

  /**
   * \brief Looks up a boolean
   * \param table The table from which to look up the value
   * \param name The name of the value
   * \return The value
   */
  bool getBoolConfig(const std::string &name);

  /**
   * \brief Looks up a character
   * \param table The table from which to look up the value
   * \param name The name of the value
   * \return The value
   */
  char getCharConfig(const std::string &name);

  /**
   * \brief Looks up a string
   * \param table The table from which to look up the value
   * \param name The name of the value
   * \return The value
   */
  std::string getStringConfig(const std::string &name);

 private:
  /**
   * \brief A type alias for an unordered map containing strings mapped to configValues.  
   */
  typedef std::unordered_map<std::string, configValue> ConfigTable;  

  Configuration() {}; //Private constructor
  ConfigTable config;

  static Configuration *m_instance;
  static std::string configFile;
  static ConfigTable userDefs;
};
 
#define GET_INT(NAME) Configuration::get()->getIntConfig(NAME)
#define GET_FLOAT(NAME) Configuration::get()->getFloatConfig(NAME)
#define GET_BOOL(NAME) Configuration::get()->getBoolConfig(NAME)
#define GET_CHAR(NAME) Configuration::get()->getCharConfig(NAME)
#define GET_STRING(NAME) Configuration::get()->getStringConfig(NAME)
