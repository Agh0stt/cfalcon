// falcon - copyright Abhigyan Ghosh 2025-present all rights reserved.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define TMP_C_FILE "cfalcon_tmp.c"
// Trim spaces/newlines
void trim(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\n' || str[len-1] == '\r'))
        str[--len] = '\0';
}

// Detect type from value for 'let'
const char* inferType(const char* value) {
    if (strstr(value, "\"")) return "string";
    if (strstr(value, ".")) return "double";
    if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) return "bool";
    return "int";
}

// Print with concatenation + type aware placeholders
// Print with concatenation + type-aware number conversion
void processPrint(FILE *out, const char *line) {
    const char *content = strchr(line, '(');
    if (!content) return;
    content++;

    char temp[1024];
    strcpy(temp, content);

    // Remove trailing )
    char *end = strrchr(temp, ')');
    if (end) *end = '\0';

    // Build final C code
    fprintf(out, "{ char _tmp[1024] = \"\";\n");

    char *token = strtok(temp, "+");
    while (token) {
        trim(token);
        if (token[0] == '"') {
            // String literal with escape sequences
            fprintf(out, "strcat(_tmp, %s);\n", token);
        } else if (token[0] == '{') {
            // Variable inside {}
            char var[64];
            sscanf(token, "{%63[^}]}", var);
            fprintf(out, "{ char _buf[64]; sprintf(_buf, \"%%g\", (double)%s); strcat(_tmp, _buf); }\n", var);
        } else {
            // Variable without {}
            fprintf(out, "{ char _buf[64]; sprintf(_buf, \"%%g\", (double)%s); strcat(_tmp, _buf); }\n", token);
        }
        token = strtok(NULL, "+");
    }

    // Print result (C will handle \n and \t escapes automatically)
    fprintf(out, "printf(\"%%s\\n\", _tmp); }\n");
}
// Compile Falcon → C → binary
void compileFalconToC(const char *inputFile, const char *outputBin) {
    FILE *in = fopen(inputFile, "r");
    FILE *out = fopen(TMP_C_FILE, "w");

    if (!in) { perror("Falcon source not found"); exit(1); }
    if (!out) { perror("Temp C file failed"); exit(1); }

    fprintf(out,
    "#include <stdio.h>\n"
    "#include <string.h>\n"
    "#include <stdbool.h>\n"
#ifdef _WIN32
    "#include <windows.h>\n"
#else
    "#include <unistd.h>\n"
#endif
    "\nint main() {\n"
);
    char line[512];
    while (fgets(line, sizeof(line), in)) {
        trim(line);
        if (strlen(line) == 0) continue;

        // let with type inference
        if (strncmp(line, "let ", 4) == 0) {
            char name[64], value[128];
            sscanf(line+4, "%63s = %127[^\n]", name, value);
            const char *type = inferType(value);
            if (strcmp(type, "string") == 0) fprintf(out, "char %s[256] = %s;\n", name, value);
            else if (strcmp(type, "bool") == 0) fprintf(out, "bool %s = %s;\n", name, value);
            else fprintf(out, "%s %s = %s;\n", type, name, value);
        }
        // Explicit typed vars & const
        else if (strncmp(line, "int ", 4) == 0 || strncmp(line, "float ", 6) == 0 ||
                 strncmp(line, "double ", 7) == 0 || strncmp(line, "bool ", 5) == 0 ||
                 strncmp(line, "string ", 7) == 0 || strncmp(line, "const ", 6) == 0) {
            fprintf(out, "%s;\n", line);
        } else if (strncmp(line, "enum ", 5) == 0) {
    fprintf(out, "%s;\n", line);
        } else if (strstr(line, "in range(")) {
    char var[64], start[64], end[64];
    if (sscanf(line, "for %63s in range(%63[^,], %63[^)])", var, start, end) == 3) {
        fprintf(out, "for (int %s = %s; %s < %s; %s++) {\n", var, start, var, end, var);
    } else if (sscanf(line, "for %63s in range(%63[^)])", var, end) == 2) {
        fprintf(out, "for (int %s = 0; %s < %s; %s++) {\n", var, var, end, var);
    }
        }  else if (strncmp(line, "for ", 4) == 0 && strstr(line, "(")) {
    fprintf(out, "for %s\n", strchr(line, '('));
        } else if (strncmp(line, "do", 2) == 0) {
    fprintf(out, "do {\n");
} else if (strncmp(line, "sleep(", 6) == 0) {
    char timeStr[32];
    sscanf(line, "sleep(%31[^)])", timeStr);
#ifdef _WIN32
    fprintf(out, "Sleep(%s * 1000);\n", timeStr);
#else
    fprintf(out, "sleep(%s);\n", timeStr);
#endif
} else if (strncmp(line, "include(", 8) == 0) {
    char header[128];
    if (sscanf(line, "include(\"%127[^\"]\")", header) == 1) {
        
        // Common stdlib headers list
        const char* stdlibs[] = {
            "stdio.h", "stdlib.h", "string.h", "math.h", "stdbool.h",
            "ctype.h", "time.h", "unistd.h", "stdint.h", "limits.h",
            "float.h", "assert.h", "errno.h", NULL
        };

        bool is_stdlib = false;
        for (int i = 0; stdlibs[i] != NULL; i++) {
            if (strcmp(header, stdlibs[i]) == 0) {
                is_stdlib = true;
                break;
            }
        }

        if (is_stdlib) {
            fprintf(out, "#include <%s>\n", header);
        } else {
            fprintf(out, "#include \"%s\"\n", header);
        }
    }
} else if (strncmp(line, "array<", 6) == 0) {
    char type[32], name[64], size[16], values[256];

    // Match array with size: array<int> nums[5] = {1,2,3,4,5}
    if (sscanf(line + 6, "%31[^>]> %63[^[][%15[^]]] = {%255[^}]}", 
               type, name, size, values) == 4) {
        fprintf(out, "%s %s[%s] = {%s};\n", type, name, size, values);
    }
    // Match array without size: array<int> nums = {1,2,3}
    else if (sscanf(line + 6, "%31[^>]> %63s = {%255[^}]}", 
                    type, name, values) == 3) {
        fprintf(out, "%s %s[] = {%s};\n", type, name, values);
    }
}
else if (strncmp(line, "} while", 7) == 0) {
    fprintf(out, "} while %s;\n", strchr(line, '('));
} // file open
else if (strstr(line, " = open(")) {
    char fname[64], path[128], mode[8];
    sscanf(line, "file %63s = open(\"%127[^\"]\", \"%7[^\"]\")", fname, path, mode);
    fprintf(out, "FILE *%s = fopen(\"%s\", \"%s\");\n", fname, path, mode);
}
// write
else if (strncmp(line, "write(", 6) == 0) {
    char fileVar[64], text[256];
    sscanf(line, "write(%63[^,], \"%255[^\"]\")", fileVar, text);
    fprintf(out, "fprintf(%s, \"%s\");\n", fileVar, text);
}
// close
else if (strncmp(line, "close(", 6) == 0) {
    char fileVar[64];
    sscanf(line, "close(%63[^)])", fileVar);
    fprintf(out, "fclose(%s);\n", fileVar);
}
    // read
else if (strncmp(line, "read(", 5) == 0) {
    char fileVar[64], destVar[64];
    sscanf(line, "read(%63[^,], %63[^)])", fileVar, destVar);
    fprintf(out, "{ char _buf[1024] = \"\"; fread(_buf, 1, sizeof(_buf)-1, %s); strcpy(%s, _buf); }\n", fileVar, destVar);
}
        // while loop
        else if (strncmp(line, "while ", 6) == 0) {
            fprintf(out, "while %s ", strchr(line, '('));
        } else if (strncmp(line, "struct ", 7) == 0) {
    fprintf(out, "%s {\n", line);
}
else if (strstr(line, "=") && strchr(line, '}') == NULL) {
    // Inside struct: typed fields
    fprintf(out, "%s;\n", line);
}
else if (strchr(line, '}')) {
    fprintf(out, "};\n");
}
            // Input
else if (strncmp(line, "input(", 6) == 0) {
    char prompt[128], var[64];
    sscanf(line, "input(\"%127[^\"]\", %63[^)])", prompt, var);

    // Detect variable type based on earlier declaration
    // (This is a simple method: we check if var name looks like string by existence of char[ or double/float/int/bool usage)
    fprintf(out, "printf(\"%s\");\n", prompt);

    if (strstr(var, "name") || strstr(var, "str") || strstr(var, "txt")) {
        fprintf(out, "scanf(\"%%255s\", %s);\n", var);
    } else if (strstr(var, "score") || strstr(var, "pi") || strstr(var, "double")) {
        fprintf(out, "scanf(\"%%lf\", &%s);\n", var);
    } else if (strstr(var, "age") || strstr(var, "count") || strstr(var, "int")) {
        fprintf(out, "scanf(\"%%d\", &%s);\n", var);
    } else if (strstr(var, "alive") || strstr(var, "flag") || strstr(var, "bool")) {
        fprintf(out, "{ char _tmpBool[10]; scanf(\"%%9s\", _tmpBool); %s = (strcmp(_tmpBool, \"true\") == 0); }\n", var);
    } else {
        // Default: try double
        fprintf(out, "scanf(\"%%lf\", &%s);\n", var);
    }
}
        // if/elif/else
        else if (strncmp(line, "if ", 3) == 0) fprintf(out, "if %s ", strchr(line, '('));
        else if (strncmp(line, "elif ", 5) == 0) fprintf(out, "else if %s ", strchr(line, '('));
        else if (strncmp(line, "else", 4) == 0) fprintf(out, "else ");
        // switch/case/break/default
        else if (strncmp(line, "switch ", 7) == 0) fprintf(out, "switch%s ", strchr(line, '('));
        else if (strncmp(line, "case ", 5) == 0) fprintf(out, "case %s:\n", line+5);
        else if (strncmp(line, "break", 5) == 0) fprintf(out, "break;\n");
        else if (strncmp(line, "default", 7) == 0) fprintf(out, "default:\n");
        // function
        else if (strncmp(line, "func ", 5) == 0) {
            char name[64], params[128];
            sscanf(line+5, "%63[^ (](%127[^)])", name, params);
            fprintf(out, "void %s(%s){\n", name, params);
        }
        else if (strcmp(line, "}") == 0) fprintf(out, "}\n");
        // print with concat
        else if (strncmp(line, "print(", 6) == 0) processPrint(out, line);
        // raw code
        else fprintf(out, "%s;\n", line);
    }

    fprintf(out, "return 0;}\n");
    fclose(in);
    fclose(out);

    char cmd[512];
    sprintf(cmd, "gcc %s -o %s", TMP_C_FILE, outputBin);
    if (system(cmd) != 0) { perror("GCC failed"); exit(1); }

    remove(TMP_C_FILE);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s input.fl output_bin\n", argv[0]);
        return 1;
    }
    compileFalconToC(argv[1], argv[2]);
    printf("CFalcon succesfully compiled %s → %s\n", argv[1], argv[2]);
    return 0;
}
