/*
    made by Ruben amadei on the xx/xx/2026
*/

#include "../utils.h"


property_t glob_property_list_DEF[N_PROPERTIES] = { //property list and default vals
    {"MAX_CLIENTS", "100"},
    {"MAX_CLIENT_TIMEOUT", "240"},
    {"MAX_CLIENT_ERRORS", "5"},
    {"MAX_MESSAGE_LEN", "100"},
    {"DEFAULT_PORT", "6335"},
    {"REQUEST_QUEUE_CAP", "50"},
    {"RESPONSE_QUEUE_CAP", "20"}
};
property_t glob_property_list[N_PROPERTIES] = {
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""}
};

static void my_log(const char* type, const char* fmt, va_list arguments){
    char* type_newl = malloc(strlen(type)+2);
    if(!type_newl) return;
    copy_str(type_newl+1, type, strlen(type)+2);
    type_newl[0] = '\n';
    type_newl[strlen(type)] = '\t';
    type_newl[strlen(type)+1] = '\0';
    
    char* fmt_new = src_and_replace("\n", type_newl, fmt);
    if(!fmt_new){
        free(type_newl);
        return;
    }
    char* tmp = fmt_new;
    fmt_new = src_and_replace("\t ", "\t", fmt_new);
    if(!fmt_new){
        fmt_new = tmp;
    }
    else free(tmp);

    size_t size = strlen(type) + strlen(fmt_new) + 3;
    char* final_str = malloc(size);
    if(!final_str){
        free(type_newl);
        free(fmt_new);
        error("error allocating string");
        return;
    }


    copy_str(final_str, type, size);
    final_str[strlen(type)] = '\t';
    final_str[strlen(type)+1] = '\0';
    strncat(final_str, fmt_new, size - strlen(type)-1);
    final_str[size-2] = '\n';
    final_str[size-1] = '\0';

    vprintf(final_str, arguments);

    free(final_str);
}
void log_msg(char* fmt, ...){
    va_list arguments;
    va_start(arguments, fmt);

    my_log("LOG: ", fmt, arguments);

    va_end(arguments);
}
void abort_error(char* fmt, ...){
    va_list arguments;
    va_start(arguments, fmt);

    my_log("FATAL ERROR: ", fmt, arguments);

    va_end(arguments);
    exit(-1);
}
void error(char* fmt, ...){
    va_list arguments;
    va_start(arguments, fmt);

    my_log("ERROR: ", fmt, arguments);

    va_end(arguments);
}

size_t copy_str(char* dst, const char* src, size_t len){
    int i;
    
    for(i = 0; i < len - 1; i++) {
        dst[i] = src[i];
        if(src[i] == '\0'){
            return i-1;
        }
    }
    dst[len-1] = '\0';
    return len-1;
}
size_t get_substr(char* dst, const char* src_start, const char* src_end, size_t len){
    const char* i;
    int j = 0;
    for(i = src_start; i != src_end; i++){
        if(j >= len) break;
        dst[j] = *i; 
        if(*i == '\0') return j;
        j++;
    }
    dst[j >= len ? j-1 : j] = '\0';
    return j >= len ? j-1 : j;
}
char* src_and_replace(const char* needle, const char* substr, const char* src_str){
    size_t n_occurr=0;
    const char* found = src_str;
    const char* str_start;
    while((found = strstr(found, needle)) != NULL){
        n_occurr++;
        found++;
    }
    
    size_t final_sz = strlen(src_str) - strlen(needle)*n_occurr + strlen(substr)*n_occurr;
    size_t used_size = 0;;
    char* replaced = malloc(final_sz+1);
    if(!replaced) return NULL;

    found = src_str;
    str_start = src_str;
    while((found = strstr(found, needle)) != NULL){
        str_start+=get_substr(&replaced[used_size], str_start, found, final_sz+1);
        str_start += strlen(needle);

        used_size = strlen(replaced);
        copy_str(&replaced[used_size], substr, final_sz-used_size+1);
        used_size = strlen(replaced);
        
        found++;
    }
    copy_str(&replaced[used_size], str_start, final_sz+1);

    return replaced;
}
char* make_escaped_string(const char* orig_string){
    assert(orig_string);
    
    int i;
    size_t orig_size = strlen(orig_string);
    size_t extra = 0;

    for(i=0; i < orig_size; i++){
        if(strchr(ESCAPED_CHARS, orig_string[i])) extra++;
    }

    char* escaped_str = malloc(orig_size + extra + 1);
    if(!escaped_str){
        error("couldn't make an escaped copy");
        return NULL;
    }

    int j = 0;
    for(i=0; i < orig_size; i++){
        if(strchr(ESCAPED_CHARS, orig_string[i])){
            escaped_str[j] = '\\';
            j++;
        }
        escaped_str[j] = orig_string[i];
        j++;
    }
    return escaped_str;
}
int is_escaped(const char* ch){
    if(ch[-1] == '\\') return 1;
    return 0;
}
char* escaped_strchr(const char* str, char ch){
    char* found;
    while((found = strchr(str, ch))){
        if(!found) return NULL; 
        if(!is_escaped(found)) {break;}
        str = found+1;
    }
    return found;
}
char* make_unescaped_string(const char* orig_string){
    assert(orig_string);
    
    int i;
    size_t orig_size = strlen(orig_string);
    size_t extra = 0; //characters we dont need anymore

    for(i=0; i < orig_size; i++){
        if(orig_string[i] == '\\') {
            extra++;
            i++; //skip in case of escaped '\'
        }
    }

    char* unescaped_str = malloc(orig_size - extra + 1);
    if(!unescaped_str){
        error("couldn't make an unescaped copy");
        return NULL;
    }
    
    int j = 0;
    for(i=0; i < orig_size; i++){
        if(orig_string[i] == '\\'){
            i++;
            if(i == orig_size) break;
        }
        unescaped_str[j] = orig_string[i];
        j++;
    }
    unescaped_str[j] = '\0';
    return unescaped_str;
}

property_t* get_property_by_name(const char* name){
    assert(name);
    
    int i;
    for(i = 0; i < N_PROPERTIES; i++){
        if(strcmp(name, glob_property_list[i].name) == 0) return &glob_property_list[i];
    }
    for(i = 0; i < N_PROPERTIES; i++){
        if(strcmp(name, glob_property_list_DEF[i].name) == 0) return &glob_property_list_DEF[i];
    }
    assert(1);
    return NULL;
}
size_t get_num_property(const char* name){
    assert(name);
    
    property_t* property = get_property_by_name(name);
    DEBUG(CONFIG_DEBUG, log_msg("found %s:%s", property->name, property->val))

    errno = 0;
    char* end;
    int value = strtol(property->val, &end, 10);
    if(errno){
        abort_error("bad property, should be a number, check config");
    }
    if(value < 0) value = abs(value);
    return (size_t)value;
}
void get_str_property(const char* name, char* dest, int dest_size){
    assert(name);
    assert(dest);

    property_t* property = get_property_by_name(name);
    char* val = property->val;
    size_t val_size = strlen(val);
    assert(dest_size >= val_size);

    copy_str(dest, val, dest_size);
}

int init_properties_by_file(const char* config_path){
    assert(config_path);

    memcpy(glob_property_list, glob_property_list_DEF, sizeof(glob_property_list));

    FILE* config_file = fopen(config_path, "r");
    if(!config_file){
        error("unable to open config file using defaults");
        return 1;
    }
    char* str;
    size_t line_size;

    char* separator_index;
    char* name_sub;
    char* value_sub;

    int end = 0;

    property_t* p;

    int ch;
    int i;
    while(!end){
        line_size = get_line_size(config_file);
        str = malloc(line_size+1);
        if(!str){
            error("unable to allocate string skipping config property and using default");
            while((ch=getc(config_file)) != '\n' && ch != EOF); //skip the line
        }
        i = 0;
        while((ch = getc(config_file)) != '\n'){
            if(ch == EOF) {
                end = 1;
                break;
            }
            str[i] = ch;
            i++;
        }
        if(line_size == 0 || str[0] == '#'){ //empty line or comment, skip it still get chars as it could be EOF
            DEBUG(CONFIG_DEBUG, log_msg("comment or empty line skipped"))
            free(str);
            continue;
        }
        str[line_size] = '\0';
        separator_index = strchr(str, '=');
        if(!separator_index){
            error("invalid config formatting skipping property");
            free(str);
            continue;
        }

        *separator_index = '\0';
        name_sub = str;
        value_sub = separator_index+1;
        p = get_property_by_name(name_sub);
        if(!p){
            error("invalid property, skipping");
            free(str);
            continue;
        }
        copy_str(p->val, value_sub, MAX_VAL);
        p->val[MAX_VAL-1] = '\0';
        DEBUG(CONFIG_DEBUG, log_msg("new property: %s=%s", p->name, p->val))
        free(str);
    }
    return 0;
}
size_t get_line_size(FILE* fp){
    int current_ch;
    long int string_start;
    size_t string_size = 0;

    string_start = ftell(fp);
    while(1){
        current_ch = getc(fp);
        if(current_ch == '\n' || current_ch == EOF) break;
        string_size++;
    }
    fseek(fp, string_start, SEEK_SET);
    return string_size;
}

