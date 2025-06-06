/**
 * A generic file parser which can read from our custom monster/item format
 *  into generic structures.
 */
#ifndef PARSER_H
#define PARSER_H

#include <cstdlib>
#include <fstream>
#include <vector>
#include <map>
#include <iostream>

#include "character.h"
#include "macros.h"

typedef enum {
    PARSE_TYPE_STRING,
    PARSE_TYPE_LONG_STRING,
    PARSE_TYPE_INT,
    PARSE_TYPE_RARITY,
    PARSE_TYPE_DICE,
    PARSE_TYPE_CHAR,
    PARSE_TYPE_MONSTER_ATTRIBUTES,
    PARSE_TYPE_COLOR,
    PARSE_TYPE_ITEM_TYPE,
    PARSE_TYPE_BOOL,
    PARSE_TYPE_TUPLE,
    PARSE_TYPE_VECTOR_STRINGS
} parse_type_t;

void write_to_string(void *item, std::string line, std::ifstream &input);
void write_to_long_string(void *item, std::string line, std::ifstream &input, int &line_i);
void write_to_int(void *item, std::string line, std::ifstream &input);
void write_to_dice(void *item, std::string line, std::ifstream &input);
void write_to_char(void *item, std::string line, std::ifstream &input);
void write_to_monster_attributes(void *item, std::string line, std::ifstream &input);
void write_to_color(void *item, std::string line, std::ifstream &input);
void write_to_item_type(void *item, std::string line, std::ifstream &input);
void write_to_bool(void *item, std::string line, std::ifstream &input);
void write_to_tuple(void *item, std::string line, std::ifstream &input);
void write_to_vector_strings(void *item, std::string line, std::ifstream &input, int &line_i);
void convert(void *item, std::string line, std::ifstream &input, parse_type_t type, int &line_i);

void trim(std::string &string);

typedef struct {
    std::string name;
    size_t offset;
    parse_type_t type;
    bool required;
} parser_definition_t;

template <class T>
class Parser {
    private:
        parser_definition_t *definitions;
        int definition_count;
        std::string header;
        std::string begin_header;
        std::string current_id;
        bool skip_failures;

    public:
        Parser(parser_definition_t definitions[], int definition_count, std::string header, std::string begin_header, bool skip_failures) {
            this->definitions = definitions;
            this->definition_count = definition_count;
            this->header = header;
            this->begin_header = begin_header;
            this->skip_failures = skip_failures;
        }
        ~Parser() {}

        /**
          * Parses an input stream, storing the results in a vector of pointers to allocated structs.
          * The caller is responsible for freeing the pointers stored in the vector.
          *
          * Params:
          * - stream: The file stream to read
          * - results: The map of resulting pointers to allocated objects
          */
        void parse(std::map<std::string, T*> &results, std::ifstream &stream) {
            // This implementation will just read line-by-line, writing into the struct's variables as it goes.
            std::string line, keyword, err, part;
            T *cur = NULL;
            int i;
            unsigned long j;
            int line_i = 0;
            bool is_set[definition_count];
            parser_definition_t *target;
            bool error_mode = false;

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
                    trim(line);
                    if (line.length() == 0) continue;

                    // Pull out the keyword on this line...
                    i = line.find(' ');
                    keyword = line.substr(0, i);
                    if ((unsigned long) i == std::string::npos)
                        line = "";
                    else
                        line = line.substr(i + 1);

                    if (error_mode) {
                        if (keyword == "BEGIN") {
                            error_mode = false;
                            cur = NULL;
                        } else {
                            continue;
                        }
                    }

                    if (cur == NULL) {
                        // This line must be a BEGIN directive.
                        // This is where we'll allocate our new objects.
                        if (keyword != "BEGIN") {
                            err = "expected BEGIN, got " + keyword + " " + line;
                            goto err;
                        }

                        // Check the type and get the ID
                        j = line.find(' ');
                        if (j == std::string::npos) {
                            err = "expected BEGIN " + begin_header + " <id>, got " + line;
                            goto err;
                        }

                        part = line.substr(0, j);
                        if (part != begin_header) {
                            err = "expected BEGIN " + begin_header + " <id>, got " + part;
                            goto err;
                        }
                        part = line.substr(j + 1);
                        if (results.count(part) > 0) {
                            err = "duplicate ID " + part;
                            goto err;
                        }
                        current_id = part;
                        cur = new T;
                        for (i = 0; i < definition_count; i++) is_set[i] = false;
                        continue;
                    }
                    // The other 'reserved' header is END.
                    if (keyword == "END") {
                        // Make sure all required fields have been set on this object.
                        for (i = 0; i < definition_count; i++) {
                            if (definitions[i].required && !is_set[i]) {
                                err = "incomplete declaration, missing " + definitions[i].name;
                                goto err;
                            }
                        }
                        // We can now toss it on the result list and move on to the next one.
                        results[current_id] = cur;
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
                    if (target == NULL) {
                        err = "unrecognized keyword " + keyword;
                        goto err;
                    }

                    if (is_set[i]) {
                        err = "duplicate keyword " + keyword;
                        goto err;
                    }

                    try {
                        convert(((char *) cur) + target->offset, line, stream, target->type, line_i);
                    } catch (std::exception &e) {
                        err = "error while parsing data for " + keyword + ": " + e.what();
                        goto err;
                    }
                    is_set[i] = true;

                    continue;

                    err:
                    if (skip_failures) {
                        error_mode = true;
                        std::cerr << "(warn) skipping to next definition due to error at line " << line_i << ": " << err << std::endl;
                        cur = NULL;
                    } else {
                        throw dungeon_exception(__PRETTY_FUNCTION__, err);
                    }
                }

                if (cur != NULL) {
                    if (skip_failures)
                        std::cerr << "(warn) incomplete declaration at EOF, line " << line_i << std::endl;
                    else
                        throw dungeon_exception(__PRETTY_FUNCTION__, "expected END");
                }
            } catch (dungeon_exception &e) {
                for (const auto &t : results) // neat
                    delete t.second;
                throw dungeon_exception(__PRETTY_FUNCTION__, e, "while parsing file at line " + std::to_string(line_i));
            } catch (std::exception &e) {
                for (const auto &t : results)
                    delete t.second;
                throw dungeon_exception(__PRETTY_FUNCTION__, "while parsing file at line " + std::to_string(line_i) + ": " + e.what());
            }
        }
};

#endif