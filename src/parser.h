/**
 * A generic file parser which can read from our custom monster/item format
 *  into generic structures.
 */
#ifndef PARSER_H
#define PARSER_H

#include <cstdlib>
#include <fstream>
#include <vector>

typedef enum {
    PARSE_TYPE_STRING,
    PARSE_TYPE_LONG_STRING,
    PARSE_TYPE_INT,
    PARSE_TYPE_DICE
} parse_type_t;

typedef struct {
    char *name;
    size_t offset;
    parse_type_t parse_with;
    bool required;
} parser_t_definition;

template <class T>
class parser_t {
    private:
        parser_t_definition *definitions;
        int definition_count;
        std::string header;
        std::string begin_header;

    public:
        parser_t(parser_t_definition definitions[], int definition_count, std::string header, std::string begin_header);
        ~parser_t();

        std::vector<T> parse(std::ifstream &stream);
};

#endif
