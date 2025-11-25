#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_KEY_LENGTH 256
#define MAX_VALUE_LENGTH 1024
#define MAX_LINE_LENGTH 2048
#define MAX_SECTION_DEPTH 10
#define MAX_ARRAY_ELEMENTS 100
#define MAX_CONFIG_ENTRIES 1000

/* Data types supported by the parser */
typedef enum {
    TYPE_STRING,
    TYPE_INTEGER,
    TYPE_FLOAT,
    TYPE_BOOLEAN,
    TYPE_ARRAY,
    TYPE_SECTION,
    TYPE_NULL
} ConfigValueType;

/* Value structure to hold different types */
typedef struct {
    ConfigValueType type;
    union {
        char *string_val;
        long int_val;
        double float_val;
        bool bool_val;
        struct {
            void **elements;
            size_t count;
            ConfigValueType element_type;
        } array_val;
    } data;
} ConfigValue;

/* Configuration entry */
typedef struct ConfigEntry {
    char *key;
    ConfigValue *value;
    char *section;
    struct ConfigEntry *next;
} ConfigEntry;

/* Parser state */
typedef struct {
    ConfigEntry *entries;
    char *current_section;
    size_t entry_count;
    size_t line_number;
    bool strict_mode;
    char error_message[512];
} ParserContext;

/* Function prototypes */

/* Parser initialization and cleanup */
ParserContext* parser_init(bool strict_mode);
void parser_free(ParserContext *ctx);

/* Main parsing functions */
int parse_file(ParserContext *ctx, const char *filename);
int parse_line(ParserContext *ctx, const char *line);
int parse_string(ParserContext *ctx, const char *config_str);

/* Entry management */
ConfigEntry* create_entry(const char *key, ConfigValue *value, const char *section);
void free_entry(ConfigEntry *entry);
void add_entry(ParserContext *ctx, ConfigEntry *entry);

/* Value parsing and creation */
ConfigValue* parse_value(const char *value_str);
ConfigValue* create_string_value(const char *str);
ConfigValue* create_int_value(long val);
ConfigValue* create_float_value(double val);
ConfigValue* create_bool_value(bool val);
ConfigValue* create_array_value(void **elements, size_t count, ConfigValueType type);
void free_value(ConfigValue *value);

/* Utility functions */
char* trim_whitespace(const char *str);
bool is_valid_key(const char *key);
bool is_section_header(const char *line);
char* extract_section_name(const char *line);
ConfigValueType infer_type(const char *value_str);

/* Query functions */
ConfigValue* get_value(ParserContext *ctx, const char *key);
ConfigValue* get_value_in_section(ParserContext *ctx, const char *section, const char *key);
char* get_string(ParserContext *ctx, const char *key, const char *default_val);
long get_int(ParserContext *ctx, const char *key, long default_val);
double get_float(ParserContext *ctx, const char *key, double default_val);
bool get_bool(ParserContext *ctx, const char *key, bool default_val);

/* Validation functions */
bool validate_config(ParserContext *ctx);
bool validate_key_value(const char *key, ConfigValue *value);
bool validate_section(const char *section);

/* Display and debug functions */
void print_config(ParserContext *ctx);
void print_entry(ConfigEntry *entry);
void print_value(ConfigValue *value);

/* Error handling */
void set_error(ParserContext *ctx, const char *format, ...);
const char* get_error(ParserContext *ctx);

#endif /* CONFIG_PARSER_H */

