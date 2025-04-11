#include "config.hpp"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

#include <limits>
#include <optional>
#include <stdexcept>
#include <memory>
#include <iostream>
#include <string.h>
#include <sstream>
#include <map>
#include <algorithm> 

static inline void trim_left(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

static inline void trim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());

    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

static bool parse_bool(const char* bool_string) {
    if(!strcmp(bool_string, "true")) {
        return true; 

    } else if(!strcmp(bool_string, "false")) {
        return false; 

    } else {
        std::stringstream ss;
        ss << "Incorrect bool argument value: " << bool_string;
        throw std::runtime_error(ss.str());
    }
}

static int parse_integer(const char* int_string) {
    return std::stoi(std::string(int_string));
}

static Config::Endpoint parse_endpoint(rapidxml::xml_node<>* endpoint_node) {
    Config::Endpoint endpoint;

    for (rapidxml::xml_node<>* node = endpoint_node->first_node(); node; node = node->next_sibling()) {
        if(!strcmp(node->name(), "host")) {
            endpoint.host = node->value();

        } else if(!strcmp(node->name(), "port")) {
            endpoint.port = parse_integer(node->value());
        
        } else {
            std::stringstream ss;
            ss << "Invalid endpoint parameter: " << node->name();
            throw std::runtime_error(ss.str());
        } 
    }

    return endpoint;
}

static void parse_endpoints(rapidxml::xml_node<>* endpoints_node, Config& config) {
    for (rapidxml::xml_node<>* node = endpoints_node->first_node(); node; node = node->next_sibling()) {
        if(!strcmp(node->name(), "endpoint")) {
            config.connection.endpoints.push_back(parse_endpoint(node));

        } else {
            std::stringstream ss;
            ss << "Endpoints can only have endpoint not: " << node->name();
            throw std::runtime_error(ss.str());
        }
    }
}

static const std::map<std::string, ColumnType> string_to_columntype = 
    {{"int8", ColumnType::Int8 },       {"int8*", ColumnType::Int8Arr },
     {"int16", ColumnType::Int16 },     {"int16*", ColumnType::Int16Arr },
     {"int32", ColumnType::Int32 },     {"int32*", ColumnType::Int32Arr },
     {"int64", ColumnType::Int64 },     {"int64*", ColumnType::Int64Arr },
     {"uint8", ColumnType::UInt8 },     {"uint8*", ColumnType::UInt8Arr },
     {"uint16", ColumnType::UInt16 },   {"uint16*", ColumnType::UInt16Arr },
     {"uint32", ColumnType::UInt32 },   {"uint32*", ColumnType::UInt32Arr },
     {"uint64", ColumnType::UInt64 },   {"uint64*", ColumnType::UInt64Arr },
     {"char", ColumnType::Char },       {"char*", ColumnType::CharArr },
     {"float", ColumnType::Float },     {"float*", ColumnType::FloatArr },
     {"double", ColumnType::Double },   {"double*", ColumnType::DoubleArr },
     {"ipaddr", ColumnType::Ipaddr },   {"ipaddr*", ColumnType::IpaddrArr },
     {"macaddr", ColumnType::Macaddr }, {"macaddr*", ColumnType::MacaddrArr },
     {"time", ColumnType::Time },       {"time*", ColumnType::TimeArr },
     {"string", ColumnType::String },
     {"bytes", ColumnType::Bytes }};

static void parse_columns(rapidxml::xml_node<>* columns_node, Config& config) {
    std::stringstream csv_string(columns_node->value());
    std::string cur_csv_value;

    while(std::getline(csv_string, cur_csv_value, ',')) {
        // Type/Name can't have space. Trim leading and trailing spaces.
        // Leading spaces for name are trimmed below.
        trim(cur_csv_value);

        Config::Column column;
        int space_pos = cur_csv_value.find(' ');
        
        std::string type = cur_csv_value.substr(0, space_pos);
        std::string name = cur_csv_value.substr(space_pos + 1);

        try {
            column.type = string_to_columntype.at(type);
        
        } catch(std::out_of_range& ex) {
            std::stringstream ss;
            ss << "Incorrect column type: " << cur_csv_value.substr(0, space_pos);
            throw std::runtime_error(ss.str());
        }
        
        trim_left(name);
        column.name = name;

        column.fieldID = 0;

        config.columns.push_back(column);

        // Template stored in input interface format. For ensuring format.
        config.template_column_csv += type + " " + name + ","; 
    }

    // Trailing comma
    config.template_column_csv.pop_back();
}

static void parse_connection(rapidxml::xml_node<>* connection_node, Config& config) {
    for (rapidxml::xml_node<>* node = connection_node->first_node(); node; node = node->next_sibling()) {
        if(!strcmp(node->name(), "endpoints")) {
            parse_endpoints(node, config);

        } else if(!strcmp(node->name(), "user")) {
            config.connection.user = node->value();

        } else if(!strcmp(node->name(), "password")) {
            config.connection.password = node->value();

        } else if(!strcmp(node->name(), "database")) {
            config.connection.database = node->value();

        } else if(!strcmp(node->name(), "table")) {
            config.connection.table = node->value();

        } else {
            std::stringstream ss;
            ss << "Incorrect connection argument name: " << node->name();
            throw std::runtime_error(ss.str());
        }
    }
}

static void parse_params(rapidxml::xml_node<>* params_node, Config& config) {
    for (rapidxml::xml_node<>* node = params_node->first_node(); node; node = node->next_sibling()) {
        if(!strcmp(node->name(), "connection")) {
            parse_connection(node, config);

        } else if(!strcmp(node->name(), "splitBiflow")) {
            config.split_biflow = parse_bool(node->value());

        } else if(!strcmp(node->name(), "biflowEmptyAutoignore")) {
            config.biflow_empty_autoignore = parse_bool(node->value());

        } else if(!strcmp(node->name(), "blocks")) {
            config.blocks = parse_integer(node->value());

        } else if(!strcmp(node->name(), "inserterThreads")) {
            config.inserter_threads = parse_integer(node->value());

        } else if(!strcmp(node->name(), "blockInsertThreshold")) {
            config.block_insert_threshold = parse_integer(node->value());

        } else if(!strcmp(node->name(), "blockInsertMaxDelaySecs")) {
            config.block_insert_max_delay_secs = parse_integer(node->value());

        } else if(!strcmp(node->name(), "columns")) {
            parse_columns(node, config);

        } else {
            std::stringstream ss;
            ss << "Incorrect argument argument name: " << node->name();
            throw std::runtime_error(ss.str());
        } 
    }
}

static void parse_root(rapidxml::xml_node<>* root_node, Config& config) {
    // Root element has to be output
    if(strcmp(root_node->name(), "output")) {
        throw std::runtime_error("Malformatted xml");
    }

    for (rapidxml::xml_node<>* node = root_node->first_node(); node; node = node->next_sibling()) {
        if(!strcmp(node->name(), "params")) {
            parse_params(node, config);
        }
    }
}

Config parse_config(char *xml_string) {
    Config config{};

    rapidxml::xml_document<> doc;

    doc.parse<0>(xml_string);

    // rapidxml::print(std::cout, doc, 0);

    parse_root(doc.first_node(), config);

    return config;
}

void print_config(Config config) {
    for(auto endpoint : config.connection.endpoints) {
        std::cout << "endpoint -> " << endpoint.host << ":" << endpoint.port << "\n"; 
    }

    std::cout << config.connection.user << " ";
    std::cout << config.connection.password << " ";
    std::cout << config.connection.database << " ";
    std::cout << config.connection.table << " ";

    std::cout << "\n";
}