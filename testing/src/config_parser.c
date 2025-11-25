/*
 * config_parser.c - Configuration File Parser Implementation
 * 
 * This implementation provides a comprehensive configuration file parser
 * with support for multiple data types, nested structures, and validation.
 */

#include "config_parser.h"
#include <stdarg.h>
#include <limits.h>
#include <errno.h>

/* ========================================================================
 * Parser Initialization and Cleanup
 * ======================================================================== */

ParserContext* parser_init(bool strict_mode) {
    ParserContext *ctx = (ParserContext*)malloc(sizeof(ParserContext));
    if (!ctx) {
        return NULL;
    }
    
    ctx->entries = NULL;
    ctx->current_section = NULL;
    ctx->entry_count = 0;
    ctx->line_number = 0;
    ctx->strict_mode = strict_mode;
    memset(ctx->error_message, 0, sizeof(ctx->error_message));
    
    return ctx;
}

void parser_free(ParserContext *ctx) {
    if (!ctx) return;
    
    ConfigEntry *current = ctx->entries;
    while (current) {
        ConfigEntry *next = current->next;
        free_entry(current);
        current = next;
    }
    
    if (ctx->current_section) {
        free(ctx->current_section);
    }
    
    free(ctx);
}

/* ========================================================================
 * Entry Management
 * ======================================================================== */

ConfigEntry* create_entry(const char *key, ConfigValue *value, const char *section) {
    if (!key || !value) return NULL;
    
    ConfigEntry *entry = (ConfigEntry*)malloc(sizeof(ConfigEntry));
    if (!entry) return NULL;
    
    entry->key = strdup(key);
    entry->value = value;
    entry->section = section ? strdup(section) : NULL;
    entry->next = NULL;
    
    return entry;
}

void free_entry(ConfigEntry *entry) {
    if (!entry) return;
    
    if (entry->key) free(entry->key);
    if (entry->section) free(entry->section);
    if (entry->value) free_value(entry->value);
    free(entry);
}

void add_entry(ParserContext *ctx, ConfigEntry *entry) {
    if (!ctx || !entry) return;
    
    if (ctx->entry_count >= MAX_CONFIG_ENTRIES) {
        set_error(ctx, "Maximum number of configuration entries exceeded");
        free_entry(entry);
        return;
    }
    
    if (!ctx->entries) {
        ctx->entries = entry;
    } else {
        ConfigEntry *current = ctx->entries;
        while (current->next) {
            current = current->next;
        }
        current->next = entry;
    }
    
    ctx->entry_count++;
}

/* ========================================================================
 * Value Creation and Management
 * ======================================================================== */

ConfigValue* create_string_value(const char *str) {
    if (!str) return NULL;
    
    ConfigValue *value = (ConfigValue*)malloc(sizeof(ConfigValue));
    if (!value) return NULL;
    
    value->type = TYPE_STRING;
    value->data.string_val = strdup(str);
    
    return value;
}

ConfigValue* create_int_value(long val) {
    ConfigValue *value = (ConfigValue*)malloc(sizeof(ConfigValue));
    if (!value) return NULL;
    
    value->type = TYPE_INTEGER;
    value->data.int_val = val;
    
    return value;
}

ConfigValue* create_float_value(double val) {
    ConfigValue *value = (ConfigValue*)malloc(sizeof(ConfigValue));
    if (!value) return NULL;
    
    value->type = TYPE_FLOAT;
    value->data.float_val = val;
    
    return value;
}

ConfigValue* create_bool_value(bool val) {
    ConfigValue *value = (ConfigValue*)malloc(sizeof(ConfigValue));
    if (!value) return NULL;
    
    value->type = TYPE_BOOLEAN;
    value->data.bool_val = val;
    
    return value;
}

ConfigValue* create_array_value(void **elements, size_t count, ConfigValueType type) {
    if (!elements || count == 0 || count > MAX_ARRAY_ELEMENTS) return NULL;
    
    ConfigValue *value = (ConfigValue*)malloc(sizeof(ConfigValue));
    if (!value) return NULL;
    
    value->type = TYPE_ARRAY;
    value->data.array_val.elements = elements;
    value->data.array_val.count = count;
    value->data.array_val.element_type = type;
    
    return value;
}

void free_value(ConfigValue *value) {
    if (!value) return;
    
    switch (value->type) {
        case TYPE_STRING:
            if (value->data.string_val) {
                free(value->data.string_val);
            }
            break;
            
        case TYPE_ARRAY:
            if (value->data.array_val.elements) {
                for (size_t i = 0; i < value->data.array_val.count; i++) {
                    if (value->data.array_val.elements[i]) {
                        if (value->data.array_val.element_type == TYPE_STRING) {
                            free(value->data.array_val.elements[i]);
                        }
                    }
                }
                free(value->data.array_val.elements);
            }
            break;
            
        default:
            break;
    }
    
    free(value);
}

/* ========================================================================
 * Utility Functions
 * ======================================================================== */

char* trim_whitespace(const char *str) {
    if (!str) return NULL;
    
    // Skip leading whitespace
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return strdup("");
    
    // Find end of string
    const char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    // Allocate and copy trimmed string
    size_t len = end - str + 1;
    
    // INTENTIONAL BUG: Buffer overflow if len > 1000
    char buffer[1000];
    if (len > sizeof(buffer)) {
        memcpy(buffer, str, len);  // OVERFLOW!
    }
    
    char *trimmed = (char*)malloc(len + 1);
    if (!trimmed) return NULL;
    
    memcpy(trimmed, str, len);
    trimmed[len] = '\0';
    
    return trimmed;
}

bool is_valid_key(const char *key) {
    if (!key || strlen(key) == 0 || strlen(key) > MAX_KEY_LENGTH) {
        return false;
    }
    
    // INTENTIONAL BUG: Null pointer dereference for specific input
    if (strcmp(key, "CRASH_ME") == 0) {
        char *ptr = NULL;
        *ptr = 'X';  // NULL DEREFERENCE!
    }
    
    // Key must start with letter or underscore
    if (!isalpha(key[0]) && key[0] != '_') {
        return false;
    }
    
    // Rest can be alphanumeric, underscore, or dot
    for (size_t i = 1; i < strlen(key); i++) {
        if (!isalnum(key[i]) && key[i] != '_' && key[i] != '.') {
            return false;
        }
    }
    
    return true;
}

bool is_section_header(const char *line) {
    if (!line) return false;
    
    char *trimmed = trim_whitespace(line);
    if (!trimmed) return false;
    
    bool result = (trimmed[0] == '[' && trimmed[strlen(trimmed) - 1] == ']');
    free(trimmed);
    
    return result;
}

char* extract_section_name(const char *line) {
    if (!line || !is_section_header(line)) return NULL;
    
    char *trimmed = trim_whitespace(line);
    if (!trimmed) return NULL;
    
    // Remove brackets
    size_t len = strlen(trimmed);
    char *section = (char*)malloc(len - 1);
    if (!section) {
        free(trimmed);
        return NULL;
    }
    
    memcpy(section, trimmed + 1, len - 2);
    section[len - 2] = '\0';
    
    free(trimmed);
    
    char *section_trimmed = trim_whitespace(section);
    free(section);
    
    return section_trimmed;
}

ConfigValueType infer_type(const char *value_str) {
    if (!value_str) return TYPE_NULL;
    
    char *trimmed = trim_whitespace(value_str);
    if (!trimmed || strlen(trimmed) == 0) {
        free(trimmed);
        return TYPE_NULL;
    }
    
    // Check for array
    if (trimmed[0] == '[' && trimmed[strlen(trimmed) - 1] == ']') {
        free(trimmed);
        return TYPE_ARRAY;
    }
    
    // Check for boolean
    if (strcmp(trimmed, "true") == 0 || strcmp(trimmed, "false") == 0 ||
        strcmp(trimmed, "True") == 0 || strcmp(trimmed, "False") == 0 ||
        strcmp(trimmed, "TRUE") == 0 || strcmp(trimmed, "FALSE") == 0 ||
        strcmp(trimmed, "yes") == 0 || strcmp(trimmed, "no") == 0) {
        free(trimmed);
        return TYPE_BOOLEAN;
    }
    
    // Check for integer
    char *endptr;
    errno = 0;
    strtol(trimmed, &endptr, 10);
    if (errno == 0 && *endptr == '\0' && endptr != trimmed) {
        free(trimmed);
        return TYPE_INTEGER;
    }
    
    // Check for float
    errno = 0;
    strtod(trimmed, &endptr);
    if (errno == 0 && *endptr == '\0' && endptr != trimmed) {
        free(trimmed);
        return TYPE_FLOAT;
    }
    
    free(trimmed);
    return TYPE_STRING;
}

/* ========================================================================
 * Value Parsing
 * ======================================================================== */

ConfigValue* parse_array(const char *value_str) {
    if (!value_str) return NULL;
    
    char *trimmed = trim_whitespace(value_str);
    if (!trimmed) return NULL;
    
    // INTENTIONAL BUG: Array out of bounds access
    char test_array[10];
    size_t len = strlen(trimmed);
    if (len > 50) {
        test_array[len] = 'X';  // OUT OF BOUNDS!
    }
    
    // Remove brackets
    if (len < 2 || trimmed[0] != '[' || trimmed[len - 1] != ']') {
        free(trimmed);
        return NULL;
    }
    
    char *content = (char*)malloc(len - 1);
    if (!content) {
        free(trimmed);
        return NULL;
    }
    
    memcpy(content, trimmed + 1, len - 2);
    content[len - 2] = '\0';
    free(trimmed);
    
    // Parse elements
    void **elements = (void**)malloc(sizeof(void*) * MAX_ARRAY_ELEMENTS);
    if (!elements) {
        free(content);
        return NULL;
    }
    
    size_t count = 0;
    ConfigValueType element_type = TYPE_NULL;
    
    char *token = strtok(content, ",");
    while (token && count < MAX_ARRAY_ELEMENTS) {
        char *element_str = trim_whitespace(token);
        if (!element_str) {
            token = strtok(NULL, ",");
            continue;
        }
        
        // Infer type from first element
        if (count == 0) {
            element_type = infer_type(element_str);
        }
        
        // Parse based on type
        switch (element_type) {
            case TYPE_STRING: {
                // Remove quotes if present
                size_t elem_len = strlen(element_str);
                if (elem_len >= 2 && element_str[0] == '"' && element_str[elem_len - 1] == '"') {
                    char *unquoted = (char*)malloc(elem_len - 1);
                    if (unquoted) {
                        memcpy(unquoted, element_str + 1, elem_len - 2);
                        unquoted[elem_len - 2] = '\0';
                        elements[count++] = unquoted;
                    }
                } else {
                    elements[count++] = strdup(element_str);
                }
                break;
            }
            
            case TYPE_INTEGER: {
                long *val = (long*)malloc(sizeof(long));
                if (val) {
                    *val = strtol(element_str, NULL, 10);
                    elements[count++] = val;
                }
                break;
            }
            
            case TYPE_FLOAT: {
                double *val = (double*)malloc(sizeof(double));
                if (val) {
                    *val = strtod(element_str, NULL);
                    elements[count++] = val;
                }
                break;
            }
            
            default:
                elements[count++] = strdup(element_str);
                break;
        }
        
        free(element_str);
        token = strtok(NULL, ",");
    }
    
    free(content);
    
    if (count == 0) {
        free(elements);
        return NULL;
    }
    
    return create_array_value(elements, count, element_type);
}

ConfigValue* parse_value(const char *value_str) {
    if (!value_str) return NULL;
    
    ConfigValueType type = infer_type(value_str);
    
    char *trimmed = trim_whitespace(value_str);
    if (!trimmed) return NULL;
    
    ConfigValue *value = NULL;
    
    switch (type) {
        case TYPE_BOOLEAN: {
            bool bool_val = false;
            if (strcasecmp(trimmed, "true") == 0 || strcasecmp(trimmed, "yes") == 0) {
                bool_val = true;
            }
            value = create_bool_value(bool_val);
            break;
        }
        
        case TYPE_INTEGER: {
            long int_val = strtol(trimmed, NULL, 10);
            value = create_int_value(int_val);
            break;
        }
        
        case TYPE_FLOAT: {
            double float_val = strtod(trimmed, NULL);
            value = create_float_value(float_val);
            break;
        }
        
        case TYPE_ARRAY: {
            value = parse_array(trimmed);
            break;
        }
        
        case TYPE_STRING:
        default: {
            // Remove quotes if present
            size_t len = strlen(trimmed);
            if (len >= 2 && trimmed[0] == '"' && trimmed[len - 1] == '"') {
                char *unquoted = (char*)malloc(len - 1);
                if (unquoted) {
                    memcpy(unquoted, trimmed + 1, len - 2);
                    unquoted[len - 2] = '\0';
                    value = create_string_value(unquoted);
                    free(unquoted);
                }
            } else {
                value = create_string_value(trimmed);
            }
            break;
        }
    }
    
    free(trimmed);
    return value;
}

/* ========================================================================
 * Line Parsing
 * ======================================================================== */

int parse_line(ParserContext *ctx, const char *line) {
    if (!ctx || !line) return -1;
    
    ctx->line_number++;
    
    char *trimmed = trim_whitespace(line);
    if (!trimmed) return -1;
    
    // Skip empty lines
    if (strlen(trimmed) == 0) {
        free(trimmed);
        return 0;
    }
    
    // Skip comments
    if (trimmed[0] == '#' || trimmed[0] == ';') {
        free(trimmed);
        return 0;
    }
    
    // Check for section header
    if (is_section_header(trimmed)) {
        char *section = extract_section_name(trimmed);
        free(trimmed);
        
        if (!section) {
            set_error(ctx, "Invalid section header at line %zu", ctx->line_number);
            return -1;
        }
        
        if (ctx->current_section) {
            free(ctx->current_section);
        }
        ctx->current_section = section;
        
        return 0;
    }
    
    // Parse key-value pair
    char *equals = strchr(trimmed, '=');
    if (!equals) {
        free(trimmed);
        if (ctx->strict_mode) {
            set_error(ctx, "Invalid syntax at line %zu: no '=' found", ctx->line_number);
            return -1;
        }
        return 0;
    }
    
    // Extract key
    size_t key_len = equals - trimmed;
    char *key_str = (char*)malloc(key_len + 1);
    if (!key_str) {
        free(trimmed);
        return -1;
    }
    memcpy(key_str, trimmed, key_len);
    key_str[key_len] = '\0';
    
    char *key = trim_whitespace(key_str);
    free(key_str);
    
    if (!is_valid_key(key)) {
        free(key);
        free(trimmed);
        if (ctx->strict_mode) {
            set_error(ctx, "Invalid key at line %zu", ctx->line_number);
            return -1;
        }
        return 0;
    }
    
    // Extract value
    char *value_str = trim_whitespace(equals + 1);
    if (!value_str) {
        free(key);
        free(trimmed);
        return -1;
    }
    
    ConfigValue *value = parse_value(value_str);
    free(value_str);
    free(trimmed);
    
    if (!value) {
        free(key);
        if (ctx->strict_mode) {
            set_error(ctx, "Failed to parse value at line %zu", ctx->line_number);
            return -1;
        }
        return 0;
    }
    
    // Create and add entry
    ConfigEntry *entry = create_entry(key, value, ctx->current_section);
    free(key);
    
    if (!entry) {
        free_value(value);
        return -1;
    }
    
    add_entry(ctx, entry);
    
    return 0;
}

/* ========================================================================
 * File and String Parsing
 * ======================================================================== */

int parse_file(ParserContext *ctx, const char *filename) {
    if (!ctx || !filename) return -1;
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        set_error(ctx, "Failed to open file: %s", filename);
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    int result = 0;
    
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        if (parse_line(ctx, line) < 0) {
            result = -1;
            if (ctx->strict_mode) {
                break;
            }
        }
    }
    
    fclose(file);
    return result;
}

int parse_string(ParserContext *ctx, const char *config_str) {
    if (!ctx || !config_str) return -1;
    
    char *config_copy = strdup(config_str);
    if (!config_copy) return -1;
    
    char *line = strtok(config_copy, "\n");
    int result = 0;
    
    while (line) {
        if (parse_line(ctx, line) < 0) {
            result = -1;
            if (ctx->strict_mode) {
                break;
            }
        }
        line = strtok(NULL, "\n");
    }
    
    free(config_copy);
    return result;
}

/* ========================================================================
 * Query Functions
 * ======================================================================== */

ConfigValue* get_value(ParserContext *ctx, const char *key) {
    if (!ctx || !key) return NULL;
    
    ConfigEntry *current = ctx->entries;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current->value;
        }
        current = current->next;
    }
    
    return NULL;
}

ConfigValue* get_value_in_section(ParserContext *ctx, const char *section, const char *key) {
    if (!ctx || !key) return NULL;
    
    ConfigEntry *current = ctx->entries;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            if ((section == NULL && current->section == NULL) ||
                (section != NULL && current->section != NULL && strcmp(current->section, section) == 0)) {
                return current->value;
            }
        }
        current = current->next;
    }
    
    return NULL;
}

char* get_string(ParserContext *ctx, const char *key, const char *default_val) {
    ConfigValue *value = get_value(ctx, key);
    if (!value || value->type != TYPE_STRING) {
        return default_val ? strdup(default_val) : NULL;
    }
    return strdup(value->data.string_val);
}

long get_int(ParserContext *ctx, const char *key, long default_val) {
    ConfigValue *value = get_value(ctx, key);
    if (!value || value->type != TYPE_INTEGER) {
        return default_val;
    }
    return value->data.int_val;
}

double get_float(ParserContext *ctx, const char *key, double default_val) {
    ConfigValue *value = get_value(ctx, key);
    if (!value || value->type != TYPE_FLOAT) {
        return default_val;
    }
    return value->data.float_val;
}

bool get_bool(ParserContext *ctx, const char *key, bool default_val) {
    ConfigValue *value = get_value(ctx, key);
    if (!value || value->type != TYPE_BOOLEAN) {
        return default_val;
    }
    return value->data.bool_val;
}

/* ========================================================================
 * Validation Functions
 * ======================================================================== */

bool validate_key_value(const char *key, ConfigValue *value) {
    if (!key || !value) return false;
    
    if (!is_valid_key(key)) return false;
    
    switch (value->type) {
        case TYPE_STRING:
            if (!value->data.string_val) return false;
            if (strlen(value->data.string_val) > MAX_VALUE_LENGTH) return false;
            break;
            
        case TYPE_ARRAY:
            if (!value->data.array_val.elements) return false;
            if (value->data.array_val.count == 0 || 
                value->data.array_val.count > MAX_ARRAY_ELEMENTS) return false;
            break;
            
        default:
            break;
    }
    
    return true;
}

bool validate_section(const char *section) {
    if (!section) return true; // NULL section is valid (global scope)
    
    if (strlen(section) == 0 || strlen(section) > MAX_KEY_LENGTH) {
        return false;
    }
    
    return is_valid_key(section);
}

bool validate_config(ParserContext *ctx) {
    if (!ctx) return false;
    
    ConfigEntry *current = ctx->entries;
    while (current) {
        if (!validate_key_value(current->key, current->value)) {
            set_error(ctx, "Invalid entry: key='%s'", current->key);
            return false;
        }
        
        if (!validate_section(current->section)) {
            set_error(ctx, "Invalid section: '%s'", current->section);
            return false;
        }
        
        current = current->next;
    }
    
    return true;
}

/* ========================================================================
 * Display Functions
 * ======================================================================== */

void print_value(ConfigValue *value) {
    if (!value) {
        printf("(null)");
        return;
    }
    
    switch (value->type) {
        case TYPE_STRING:
            printf("\"%s\"", value->data.string_val);
            break;
            
        case TYPE_INTEGER:
            printf("%ld", value->data.int_val);
            break;
            
        case TYPE_FLOAT:
            printf("%f", value->data.float_val);
            break;
            
        case TYPE_BOOLEAN:
            printf("%s", value->data.bool_val ? "true" : "false");
            break;
            
        case TYPE_ARRAY:
            printf("[");
            for (size_t i = 0; i < value->data.array_val.count; i++) {
                if (i > 0) printf(", ");
                
                switch (value->data.array_val.element_type) {
                    case TYPE_STRING:
                        printf("\"%s\"", (char*)value->data.array_val.elements[i]);
                        break;
                    case TYPE_INTEGER:
                        printf("%ld", *(long*)value->data.array_val.elements[i]);
                        break;
                    case TYPE_FLOAT:
                        printf("%f", *(double*)value->data.array_val.elements[i]);
                        break;
                    default:
                        printf("%s", (char*)value->data.array_val.elements[i]);
                        break;
                }
            }
            printf("]");
            break;
            
        default:
            printf("(unknown type)");
            break;
    }
}

void print_entry(ConfigEntry *entry) {
    if (!entry) return;
    
    if (entry->section) {
        printf("[%s] ", entry->section);
    }
    
    printf("%s = ", entry->key);
    print_value(entry->value);
    printf("\n");
}

void print_config(ParserContext *ctx) {
    if (!ctx) return;
    
    printf("Configuration (%zu entries):\n", ctx->entry_count);
    printf("================================\n");
    
    ConfigEntry *current = ctx->entries;
    while (current) {
        print_entry(current);
        current = current->next;
    }
    
    printf("================================\n");
}

/* ========================================================================
 * Error Handling
 * ======================================================================== */

void set_error(ParserContext *ctx, const char *format, ...) {
    if (!ctx || !format) return;
    
    va_list args;
    va_start(args, format);
    vsnprintf(ctx->error_message, sizeof(ctx->error_message), format, args);
    va_end(args);
}

const char* get_error(ParserContext *ctx) {
    if (!ctx) return NULL;
    return ctx->error_message;
}

/* ========================================================================
 * Main Function (for testing)
 * ======================================================================== */

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return 1;
    }
    
    // Initialize parser
    ParserContext *ctx = parser_init(false); // Non-strict mode
    if (!ctx) {
        fprintf(stderr, "Failed to initialize parser\n");
        return 1;
    }
    
    // Parse file
    printf("Parsing file: %s\n", argv[1]);
    if (parse_file(ctx, argv[1]) < 0) {
        fprintf(stderr, "Parse error: %s\n", get_error(ctx));
        parser_free(ctx);
        return 1;
    }
    
    // Validate configuration
    if (!validate_config(ctx)) {
        fprintf(stderr, "Validation error: %s\n", get_error(ctx));
        parser_free(ctx);
        return 1;
    }
    
    // Print parsed configuration
    print_config(ctx);
    
    // Example queries
    printf("\nExample Queries:\n");
    printf("================================\n");
    
    char *str_val = get_string(ctx, "name", "default_name");
    if (str_val) {
        printf("name = %s\n", str_val);
        free(str_val);
    }
    
    long int_val = get_int(ctx, "port", 8080);
    printf("port = %ld\n", int_val);
    
    bool bool_val = get_bool(ctx, "debug", false);
    printf("debug = %s\n", bool_val ? "true" : "false");
    
    // Cleanup
    parser_free(ctx);
    
    printf("\nParsing completed successfully!\n");
    return 0;
}

