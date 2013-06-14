
#include <dlfcn.h>
#include <stdio.h>
#include <yaml.h>
#include "runtime.h"
#include "solar.h"
#include "soltoken.h"
#include "solevent.h"
#include "soltypes.h"

struct solar_module {
    char* name;
    SolObject exports;
    UT_hash_handle hh;
};
static struct solar_module* loaded_modules = NULL;

static SolObject solar_load_object(yaml_document_t* yaml_document, yaml_node_t* node, void* dlref);
static inline void solar_register_function(char* object, char* name, SolOperatorRef function, bool on_prototype);
static inline void solar_register_event(char* object, char* type_name, char* type_value);

/** libyaml wrapper functions **/
static inline char* yaml_node_get_value(yaml_node_t* node);
static inline yaml_node_pair_t* yaml_node_get_mapping(yaml_document_t* document, yaml_node_t* parent_node, int index);
static inline yaml_node_t* yaml_node_get_child(yaml_document_t* document, yaml_node_t* parent_node, char* key);
static inline yaml_node_t* yaml_node_get_element(yaml_document_t* document, yaml_node_t* sequence_node, int index);

SolObject solar_load(char* filename) {
    // make sure this module hasn't been loaded yet
    struct solar_module* loaded_module;
    HASH_FIND_STR(loaded_modules, filename, loaded_module);
    if (loaded_module) {
        return sol_obj_retain(loaded_module->exports);
    }
    
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
    
    // handle export return value
    SolObject exports = sol_obj_clone(RawObject);
    
    // load dependencies
    SolObject dependencies = sol_obj_clone(RawObject);
    yaml_node_t* dependency_list = yaml_node_get_child(&yaml_document, yaml_document_get_root_node(&yaml_document), "require");
    if (dependency_list) {
        yaml_node_t* dependency_node;
        for (int i = 0; (dependency_node = yaml_node_get_element(&yaml_document, dependency_list, i)) != NULL; i++) {
            char* dependency_name = yaml_node_get_value(dependency_node);
            if (!strcmp(dependency_name, filename)) {
                fprintf(stderr, "warning: recursive module dependency in %s\n", filename);
                continue;
            }
            SolObject dependency = solar_load(dependency_name);
            sol_obj_set_prop(dependencies, dependency_name, dependency);
            sol_obj_release(dependency);
        }
    }
    
    // load natives
    yaml_node_t* native_list = yaml_node_get_child(&yaml_document, yaml_document_get_root_node(&yaml_document), "natives");
    if (native_list) {
        yaml_node_pair_t* native_pair;
        for (int i = 0; (native_pair = yaml_node_get_mapping(&yaml_document, native_list, i)) != NULL; i++) {
            yaml_node_t* native_key = yaml_document_get_node(&yaml_document, native_pair->key);
            yaml_node_t* native_value = yaml_document_get_node(&yaml_document, native_pair->value);
            // load shared library
            char* name = yaml_node_get_value(native_key);
            char* native_path;
#ifdef _WIN32
            asprintf(&native_path, "%s/natives/%s.dll", path, name);
#elif __APPLE__
            asprintf(&native_path, "%s/natives/lib%s.dylib", path, name);
#else
            asprintf(&native_path, "%s/natives/lib%s.so", path, name);
#endif
            void* native_dl = dlopen(native_path, RTLD_LAZY);
            free(native_path);
            // call init method
            int (*solar_init)(SolObject, SolObject) = dlsym(native_dl, "solar_init");
            int init_ret = 0;
            if (solar_init) init_ret = solar_init(exports, dependencies);
            if (init_ret != 0) {
                fprintf(stderr, "Error loading solar module: '%s' init failed with error code %i.", name, init_ret);
                exit(EXIT_FAILURE);
            }
            // get all exports to load
            yaml_node_t* native_exports = yaml_node_get_child(&yaml_document, native_value, "exports");
            if (native_exports) {
                yaml_node_pair_t* native_export_pair;
                for (int j = 0; (native_export_pair = yaml_node_get_mapping(&yaml_document, native_exports, j)) != NULL; j++) {
                    yaml_node_t* native_export_key = yaml_document_get_node(&yaml_document, native_export_pair->key);
                    yaml_node_t* native_export_value = yaml_document_get_node(&yaml_document, native_export_pair->value);
                    char* export_name = yaml_node_get_value(native_export_key);
                    SolObject export_object = solar_load_object(&yaml_document, native_export_value, native_dl);
                    SolObject existing_export_object = sol_obj_get_prop(exports, export_name);
                    if (existing_export_object) {
                        sol_obj_patch(existing_export_object, export_object);
                        sol_obj_release(existing_export_object);
                    } else {
                        sol_obj_set_prop(exports, export_name, export_object);
                    }
                    sol_obj_release(export_object);
                }
            }
            // get all extensions to load
            yaml_node_t* native_extensions = yaml_node_get_child(&yaml_document, native_value, "extend");
            if (native_extensions) {
                yaml_node_pair_t* native_extension_pair;
                for (int j = 0; (native_extension_pair = yaml_node_get_mapping(&yaml_document, native_extensions, j)) != NULL; j++) {
                    yaml_node_t* native_extension_key = yaml_document_get_node(&yaml_document, native_extension_pair->key);
                    yaml_node_t* native_extension_value = yaml_document_get_node(&yaml_document, native_extension_pair->value);
                    char* extension_name = yaml_node_get_value(native_extension_key);
                    SolObject parent_object = sol_token_resolve(extension_name);
                    SolObject extension_object = solar_load_object(&yaml_document, native_extension_value, native_dl);
                    sol_obj_patch(parent_object, extension_object);
                    sol_obj_release(extension_object);
                    sol_obj_release(parent_object);
                }
            }
        }
    }
    
    // load sol bytecode files
    yaml_node_t* bin_list = yaml_node_get_child(&yaml_document, yaml_document_get_root_node(&yaml_document), "binaries");
    yaml_node_t* bin_el;
    if (bin_list) {
        for (int i = 0; (bin_el = yaml_node_get_element(&yaml_document, bin_list, i)) != NULL; i++) {
            // get file path
            char* name = yaml_node_get_value(bin_el);
            char* bin_path; asprintf(&bin_path, "%s/binaries/%s", path, name);
            // load data
            FILE* bin_file = fopen(bin_path, "rb");
            free(bin_path);
            fseek(bin_file, 0, SEEK_END);
            off_t size = ftell(bin_file);
            rewind(bin_file);
            unsigned char* buffer = malloc(size);
            fread(buffer, size, 1, bin_file);
            fclose(bin_file);
            
            // execute code
            sol_token_pool_push();
            sol_token_register("exports", exports);
            sol_runtime_execute(buffer);
            sol_token_pool_pop();
        }
    }
    
    // clean up
    free(path);
    free(config_path);
    yaml_document_delete(&yaml_document);
    yaml_parser_delete(&yaml_parser);
    
    // mark this module as loaded
    loaded_module = malloc(sizeof(*loaded_module));
    loaded_module->name = strdup(filename);
    loaded_module->exports = sol_obj_retain(exports);
    HASH_ADD_KEYPTR(hh, loaded_modules, loaded_module->name, strlen(loaded_module->name), loaded_module);
    
    return exports;
}

static SolObject solar_load_object(yaml_document_t* yaml_document, yaml_node_t* node, void* dlref) {
    yaml_node_t* function_node = yaml_node_get_child(yaml_document, node, "function");
    yaml_node_t* string_node = yaml_node_get_child(yaml_document, node, "string");
    yaml_node_t* number_node = yaml_node_get_child(yaml_document, node, "number");
    yaml_node_t* parent_node = yaml_node_get_child(yaml_document, node, "parent");
    yaml_node_t* prototype_node = yaml_node_get_child(yaml_document, node, "prototype");
    yaml_node_t* properties_node = yaml_node_get_child(yaml_document, node, "properties");
    
    SolObject object;
    if (function_node) {
        object = sol_obj_retain((SolObject) sol_operator_create(dlsym(dlref, yaml_node_get_value(function_node))));
    } else if (string_node) {
        object = sol_obj_retain((SolObject) sol_string_create(yaml_node_get_value(string_node)));
    } else if (number_node) {
        char* string_value = yaml_node_get_value(number_node);
        double value; sscanf(string_value, "%lf", &value);
        object = sol_obj_retain((SolObject) sol_num_create(value));
    } else if (parent_node) {
        SolObject parent = sol_token_resolve(yaml_node_get_value(parent_node));
        object = sol_obj_clone(parent);
        sol_obj_release(parent);
    } else {
        object = sol_obj_clone(Object);
    }
    
    if (prototype_node) {
        yaml_node_pair_t* prototype_pair;
        for (int i = 0; (prototype_pair = yaml_node_get_mapping(yaml_document, prototype_node, i)) != NULL; i++) {
            char* name = yaml_node_get_value(yaml_document_get_node(yaml_document, prototype_pair->key));
            SolObject prototype_object = solar_load_object(yaml_document, yaml_document_get_node(yaml_document, prototype_pair->value), dlref);
            sol_obj_set_proto(object, name, prototype_object);
            sol_obj_release(prototype_object);
        }
    }
    if (properties_node) {
        yaml_node_pair_t* properties_pair;
        for (int i = 0; (properties_pair = yaml_node_get_mapping(yaml_document, properties_node, i)) != NULL; i++) {
            char* name = yaml_node_get_value(yaml_document_get_node(yaml_document, properties_pair->key));
            SolObject properties_object = solar_load_object(yaml_document, yaml_document_get_node(yaml_document, properties_pair->value), dlref);
            sol_obj_set_prop(object, name, properties_object);
            sol_obj_release(properties_object);
        }
    }
    
    return object;
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

static inline void solar_register_event(char* object, char* type_name, char* type_value) {
    SolObject parent = sol_token_resolve(object);
    if (parent == NULL) {
        parent = sol_obj_clone(Event);
        sol_token_register(object, parent);
    }
    sol_obj_set_prop(parent, type_name, (SolObject) sol_string_create(type_value));
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
