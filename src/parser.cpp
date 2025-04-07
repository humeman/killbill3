#include "parser.h"
#include "macros.h"

#include <fstream>
#include <vector>

template <class T>
parser_t<T>::parser_t(parser_t_definition definitions[], int definition_count, std::string header, std::string begin_header) {
    this->definitions = definitions;
    this->definition_count = definition_count;
    this->header = header;
    this->begin_header = begin_header;
}

template <class T>
parser_t<T>::~parser_t() {}

template <class T>
std::vector<T> parser_t<T>::parse(std::ifstream &input) {
    // This implementation will just read line-by-line, writing into the struct's variables as it goes.
    std::string line;
    T *cur = NULL;
    int i;
    bool is_real;
    int line_i = 0;

    // Before we can begin, the first line must match the file header
    if (!std::getline(input, line))
        throw dungeon_exception(__PRETTY_FUNCTION__, "input file is missing expected header %s", header);
    if (line != header)
        throw dungeon_exception(__PRETTY_FUNCTION__, "input file has incorrect header '%s' (expected %s)", line, header);
    line_i++;

    // Now we can begin parsing line by line
    while (std::getline(input, line)) {
        line_i++;
        // Primitive filtering of whitespace...
        is_real = false;
        for (i = 0; line[i]; i++) {
            if (line[i] != ' ' && line[i] != '\n' && line[i] != '\t') {
                is_real = true;
                break;
            }
        }
        if (!is_real) continue;

        if (cur == NULL) {
            // This line must be a BEGIN directive.
            // This is where we'll allocate our new objects.
            if (line != "BEGIN " + begin_header)
                throw dungeon_exception(__PRETTY_FUNCTION__, "line %d: expected BEGIN %s", line_i, begin_header);

            if (!(cur = malloc(sizeof (T))))
                throw dungeon_exception(__PRETTY_FUNCTION__, "failure while allocating new object");



            continue;
        }
    }
}
