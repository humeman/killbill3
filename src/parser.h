/**
 * A generic file parser which can read from our custom monster/item format
 *  into generic structures.
 */
#ifndef PARSER_H
#define PARSER_H

#include <cstdlib>
#include <fstream>
#include <vector>
#include <iostream>

#include "character.h"
#include "macros.h"

typedef enum {
    PARSE_TYPE_STRING,
    PARSE_TYPE_LONG_STRING,
    PARSE_TYPE_INT,
    PARSE_TYPE_DICE,
    PARSE_TYPE_CHAR,
    PARSE_TYPE_MONSTER_ATTRIBUTES,
    PARSE_TYPE_COLOR,
    PARSE_TYPE_ITEM_TYPE,
    PARSE_TYPE_BOOL
} parse_type_t;

void write_to_string(void *item, std::string line, std::ifstream &input);
void write_to_long_string(void *item, std::string line, std::ifstream &input);
void write_to_int(void *item, std::string line, std::ifstream &input);
void write_to_dice(void *item, std::string line, std::ifstream &input);
void write_to_char(void *item, std::string line, std::ifstream &input);
void write_to_monster_attributes(void *item, std::string line, std::ifstream &input);
void write_to_color(void *item, std::string line, std::ifstream &input);
void write_to_item_type(void *item, std::string line, std::ifstream &input);
void write_to_bool(void *item, std::string line, std::ifstream &input);
void convert(void *item, std::string line, std::ifstream &input, parse_type_t type);

typedef struct {
    std::string name;
    size_t offset;
    parse_type_t type;
    bool required;
} parser_definition_t;

template <class T>
class parser_t {
    private:
        parser_definition_t *definitions;
        int definition_count;
        std::string header;
        std::string begin_header;

    public:
        parser_t(parser_definition_t definitions[], int definition_count, std::string header, std::string begin_header) {
            this->definitions = definitions;
            this->definition_count = definition_count;
            this->header = header;
            this->begin_header = begin_header;
        }
        ~parser_t() {}

        /**
          * Parses an input stream, storing the results in a vector of pointers to allocated structs.
          * The caller is responsible for freeing the pointers stored in the vector.
          *
          * Params:
          * - stream: The file stream to read
          * - results: The vector of resulting pointers to allocated objects
          */
        void parse(std::vector<T*> &results, std::ifstream &stream) {
            // This implementation will just read line-by-line, writing into the struct's variables as it goes.
            std::string line, keyword;
            T *cur = NULL;
            int i;
            bool is_real;
            int line_i = 0;
            bool is_set[definition_count];
            parser_definition_t *target;

            try {
                // Before we can begin, the first line must match the file header
                if (!std::getline(stream, line))
                    throw dungeon_exception(__PRETTY_FUNCTION__, "input file is missing expected header " + header);
                if (line != header)
                    throw dungeon_exception(__PRETTY_FUNCTION__, "input file has incorrect header '" + line + "' (expected " + header + ")");
                line_i++;

                // Now we can begin parsing line by line
                while (std::getline(stream, line)) {
                    line_i++;
                    // Primitive filtering of whitespace...
                    is_real = false;
                    for (i = 0; line[i]; i++) {
                        if (line[i] != ' ' && line[i] != '\t') {
                            is_real = true;
                            break;
                        }
                    }
                    if (!is_real) continue;

                    // Pull out the keyword on this line...
                    i = line.find(' ');
                    keyword = line.substr(0, i);
                    if ((unsigned long) i == std::string::npos)
                        line = "";
                    else
                        line = line.substr(i + 1);

                    if (cur == NULL) {
                        // This line must be a BEGIN directive.
                        // This is where we'll allocate our new objects.
                        if (keyword != "BEGIN" || line != begin_header)
                            throw dungeon_exception(__PRETTY_FUNCTION__, "line " + std::to_string(line_i) + ": expected BEGIN " + begin_header + ", got " + keyword + " " + line);

                        cur = new T;

                        for (i = 0; i < definition_count; i++) is_set[i] = false;
                        continue;
                    }
                    // The other 'reserved' header is END.
                    if (keyword == "END") {
                        // Make sure all required fields have been set on this object.
                        for (i = 0; i < definition_count; i++) {
                            if (definitions[i].required && !is_set[i])
                                throw dungeon_exception(__PRETTY_FUNCTION__, "line " + std::to_string(line_i) + ": incomplete declaration, missing " + definitions[i].name);
                        }
                        // We can now toss it on the result list and move on to the next one.
                        results.push_back(cur);
                        cur = NULL;
                        continue;
                    }

                    // Everything else will be parsed out and written to the struct.
                    target = NULL;
                    for (i = 0; i < definition_count; i++) {
                        if (keyword == definitions[i].name) {
                            target = definitions + i;
                            break;
                        }
                    }
                    if (target == NULL)
                        throw dungeon_exception(__PRETTY_FUNCTION__, "line " + std::to_string(line_i) + ": unrecognized keyword " + keyword);

                    convert(((char *) cur) + target->offset, line, stream, target->type);
                    is_set[i] = true;
                }

                if (cur != NULL)
                    throw dungeon_exception(__PRETTY_FUNCTION__, "line " + std::to_string(line_i) + ": expected END");
            } catch (dungeon_exception &e) {
                for (T *t : results) // neat
                    free(t);
                throw e;
            } catch (std::exception &e) {
                for (T *t : results)
                    free(t);
                throw e;
            }
        }
};

#endif
