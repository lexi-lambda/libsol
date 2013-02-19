
#include <dlfcn.h>
#include <stdio.h>
#include <yaml.h>
#include "solar.h"
#include "soltoken.h"
#include "solevent.h"
#include "soltypes.h"

static inline void solar_register_function(char* object, char* name, SolOperatorRef function, bool on_prototype);
static inline void solar_register_event(char* object, char* type_name, char* type_value, sol_event_initializer_fn initializer);

/** libyaml wrapper functions **/
static inline char* yaml_node_get_value(yaml_node_t* node);
static inline yaml_node_pair_t* yaml_node_get_mapping(yaml_document_t* document, yaml_node_t* parent_node, int index);
static inline yaml_node_t* yaml_node_get_child(yaml_document_t* document, yaml_node_t* parent_node, char* key);
static inline yaml_node_t* yaml_node_get_element(yaml_document_t* document, yaml_node_t* sequence_node, int index);

void solar_load(char* filename) {
    // load solar file
    char* path;
    asprintf(&path, "/usr/local/lib/sol/%s.solar", filename);
    
    // load configuration file
    char* config_path;
    asprintf(&config_path, "%s/descriptor.yml", path);
    FILE* config_file = fopen(config_path, "r");
    
    // parse YAML configuration
    yaml_parser_t yaml_parser;
    yaml_document_t yaml_document;
    yaml_parser_initialize(&yaml_parser);
    yaml_parser_set_input_file(&yaml_parser, config_file);
    yaml_parser_load(&yaml_parser, &yaml_document);
    
    // load natives
    yaml_node_t* native_list = yaml_node_get_child(&yaml_document, yaml_document_get_root_node(&yaml_document), "natives");
    int i = 0;
    yaml_node_t* native_el;
    while ((native_el = yaml_node_get_element(&yaml_document, native_list, i)) != NULL) {
        // load shared library
        char* name = yaml_node_get_value(yaml_node_get_child(&yaml_document, native_el, "name"));
        char* native_path; asprintf(&native_path, "%s/natives/%s", path, name);
        void* native_dl = dlopen(native_path, RTLD_LAZY);
        free(native_path);
        // call init method
        int (*solar_init)(void) = dlsym(native_dl, "solar_init");
        int init_ret = 0;
        if (solar_init) init_ret = solar_init();
        if (init_ret != 0) {
            fprintf(stderr, "Error loading solar module: '%s' init failed with error code %i.", name, init_ret);
            exit(EXIT_FAILURE);
        }
        // find all symbols to load and register them
        yaml_node_t* symbol_exports = yaml_node_get_child(&yaml_document, native_el, "export");
        if (symbol_exports != NULL) {
            int j = 0;
            yaml_node_pair_t* symbol_pair;
            while ((symbol_pair = yaml_node_get_mapping(&yaml_document, symbol_exports, j)) != NULL) {
                // get preliminary information
                char* symbol_object = yaml_node_get_value(yaml_document_get_node(&yaml_document, symbol_pair->key));
                yaml_node_t* symbol_list = yaml_document_get_node(&yaml_document, symbol_pair->value);
                // find all exported symbols
                yaml_node_t* symbol_el;
                int k = 0;
                while ((symbol_el = yaml_node_get_element(&yaml_document, symbol_list, k)) != NULL) {
                    char* symbol_function = yaml_node_get_value(yaml_node_get_child(&yaml_document, symbol_el, "function"));
                    char* symbol_name = yaml_node_get_value(yaml_node_get_child(&yaml_document, symbol_el, "name"));
                    yaml_node_t* symbol_use_prototype_node = yaml_node_get_child(&yaml_document, symbol_el, "use-prototype");
                    bool symbol_use_prototype = symbol_use_prototype_node ? !strcmp(yaml_node_get_value(symbol_use_prototype_node), "true") : false;
                    solar_register_function(symbol_object, symbol_name, (SolOperatorRef) dlsym(native_dl, symbol_function), symbol_use_prototype);
                    k++;
                }
                j++;
            }
        }
        // register custom events
        yaml_node_t* event_definitions = yaml_node_get_child(&yaml_document, native_el, "events");
        if (event_definitions != NULL) {
            int j = 0;
            yaml_node_pair_t* event_pair;
            while ((event_pair = yaml_node_get_mapping(&yaml_document, event_definitions, j)) != NULL) {
                // get preliminary information
                char* event_object = yaml_node_get_value(yaml_document_get_node(&yaml_document, event_pair->key));
                yaml_node_t* event_types = yaml_document_get_node(&yaml_document, event_pair->value);
                // find all exported symbols
                yaml_node_pair_t* event_type_pair;
                int k = 0;
                while ((event_type_pair = yaml_node_get_mapping(&yaml_document, event_types, k)) != NULL) {
                    char* event_type_name = yaml_node_get_value(yaml_document_get_node(&yaml_document, event_type_pair->key));
                    yaml_node_t* event_type = yaml_document_get_node(&yaml_document, event_type_pair->value);
                    char* event_type_value = yaml_node_get_value(yaml_node_get_child(&yaml_document, event_type, "type"));
                    char* event_initializer = yaml_node_get_value(yaml_node_get_child(&yaml_document, event_type, "initializer"));
                    solar_register_event(event_object, event_type_name, event_type_value, event_initializer ? (sol_event_initializer_fn) dlsym(native_dl, event_initializer) : NULL);
                    k++;
                }
                j++;
            }
        }
        i++;
    }
    
    // clean up
    free(path);
    free(config_path);
    yaml_document_delete(&yaml_document);
    yaml_parser_delete(&yaml_parser);
}

static inline void solar_register_function(char* object, char* name, SolOperatorRef function, bool use_prototype) {
    SolObject parent = sol_token_resolve(object);
    if (parent == NULL) {
        parent = sol_obj_clone(Object);
        sol_token_register(object, parent);
    }
    SolOperator operator = sol_operator_create(function);
    (use_prototype ? sol_obj_set_proto : sol_obj_set_prop)(parent, name, (SolObject) operator);
}

static inline void solar_register_event(char* object, char* type_name, char* type_value, sol_event_initializer_fn initializer) {
    SolObject parent = sol_token_resolve(object);
    if (parent == NULL) {
        parent = sol_obj_clone(Event);
        sol_token_register(object, parent);
    }
    sol_obj_set_prop(parent, type_name, (SolObject) sol_string_create(type_value));
    if (initializer)
        sol_event_initializer_register(type_value, initializer);
}

static inline char* yaml_node_get_value(yaml_node_t* node) {
    return node ? (char*) node->data.scalar.value : NULL;
}

static inline yaml_node_pair_t* yaml_node_get_mapping(yaml_document_t* document, yaml_node_t* parent_node, int index) {
    yaml_node_pair_t* pair = parent_node->data.mapping.pairs.start + index;
    return pair < parent_node->data.mapping.pairs.top ? pair : NULL;
}

static inline yaml_node_t* yaml_node_get_child(yaml_document_t* document, yaml_node_t* parent_node, char* key) {
    yaml_node_pair_t* pair = parent_node->data.mapping.pairs.start;
    do {
        yaml_node_t* key_node = yaml_document_get_node(document, pair->key);
        if (!strcmp((char *) key_node->data.scalar.value, key)) {
            return yaml_document_get_node(document, pair->value);
        }
    } while (pair++, pair < parent_node->data.mapping.pairs.top);
    return NULL;
}

static inline yaml_node_t* yaml_node_get_element(yaml_document_t* document, yaml_node_t* sequence_node, int index) {
    if (sequence_node->data.sequence.items.start + index >= sequence_node->data.sequence.items.top) return NULL;
    return yaml_document_get_node(document, sequence_node->data.sequence.items.start[index]);
}
