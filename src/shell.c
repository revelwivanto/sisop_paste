#include "shell.h"
#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

// Forward declaration for a helper function used by cp and mv
void get_dest_info(char* path, byte cwd, byte* dest_parent_idx, char* filename, bool* valid);

void shell() {
  char buf[128]; // Increased buffer size for longer commands
  char cmd[64];
  char arg[2][64];

  byte cwd = FS_NODE_P_ROOT;

  while (true) {
    printString("MengOS:");
    printCWD(cwd);
    printString("$ ");
    readString(buf);
    parseCommand(buf, cmd, arg);

    if (strcmp(cmd, "cd")) cd(&cwd, arg[0]);
    else if (strcmp(cmd, "ls")) ls(cwd, arg[0]);
    else if (strcmp(cmd, "mv")) mv(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cp")) cp(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cat")) cat(cwd, arg[0]);
    else if (strcmp(cmd, "mkdir")) mkdir(cwd, arg[0]);
    else if (strcmp(cmd, "clear")) clearScreen();
    else if (strlen(cmd) > 0) { // Check if a command was actually entered
        printString("Invalid command: ");
        printString(cmd);
        printString("\n");
    }
  }
}

// Helper function for printCWD to avoid reading the node table repeatedly
void get_path_recursive(byte current_node_index, struct node_fs* node_fs_buf) {
    if (current_node_index == FS_NODE_P_ROOT) {
        printString("/");
        return;
    }

    get_path_recursive(node_fs_buf->nodes[current_node_index].parent_index, node_fs_buf);
    if (node_fs_buf->nodes[current_node_index].parent_index != FS_NODE_P_ROOT) {
        printString("/");
    }
    printString(node_fs_buf->nodes[current_node_index].node_name);
}


// TODO: 4. Implement printCWD function
void printCWD(byte cwd) {
    struct node_fs node_fs_buf;

    // Read the node sectors to get the file/directory names and parent links
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    get_path_recursive(cwd, &node_fs_buf);
}


// TODO: 5. Implement parseCommand function
void parseCommand(char* buf, char* cmd, char arg[2][64]) {
    int i = 0; // buffer index
    int j = 0; // command/argument index
    int part = 0; // 0 for cmd, 1 for arg1, 2 for arg2

    // Clear out old command and arguments
    clear(cmd, 64);
    clear(arg[0], 64);
    clear(arg[1], 64);

    // Skip leading spaces
    while (buf[i] == ' ' && buf[i] != '\0') {
        i++;
    }

    while (buf[i] != '\0') {
        if (buf[i] == ' ') {
            // End of a part
            j = 0; // Reset index for the next part
            part++;

            // Skip consecutive spaces
            while (buf[i] == ' ' && buf[i] != '\0') {
                i++;
            }
        } else {
            if (part == 0) { // Filling command
                cmd[j] = buf[i];
            } else if (part == 1) { // Filling arg1
                arg[0][j] = buf[i];
            } else if (part <= 2) { // Filling arg2
                arg[1][j] = buf[i];
            }
            j++;
            i++;
        }
    }
}

// TODO: 6. Implement cd function
void cd(byte* cwd, char* dirname) {
    struct node_fs node_fs_buf;
    int i;

    if (strlen(dirname) == 0) return; // No argument, do nothing

    if (strcmp(dirname, "/")) {
        *cwd = FS_NODE_P_ROOT;
        return;
    }

    if (strcmp(dirname, "..")) {
        if (*cwd != FS_NODE_P_ROOT) {
            readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
            readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
            *cwd = node_fs_buf.nodes[*cwd].parent_index;
        }
        return;
    }

    // Search for the directory in the current working directory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (strcmp(node_fs_buf.nodes[i].node_name, dirname) &&
            node_fs_buf.nodes[i].parent_index == *cwd) {

            if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                *cwd = i; // Update current working directory
            } else {
                printString("cd: not a directory: ");
                printString(dirname);
                printString("\n");
            }
            return;
        }
    }

    printString("cd: no such file or directory: ");
    printString(dirname);
    printString("\n");
}


// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname) {
    struct node_fs node_fs_buf;
    int i;
    byte target_dir = cwd;

    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // If a directory name is provided, find its node index
    if (strlen(dirname) > 0 && !strcmp(dirname, ".")) {
        bool found = false;
        for (i = 0; i < FS_MAX_NODE; i++) {
            if (strcmp(node_fs_buf.nodes[i].node_name, dirname) && node_fs_buf.nodes[i].parent_index == cwd) {
                if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                    target_dir = i;
                    found = true;
                    break;
                } else {
                    printString(dirname);
                    printString(" is not a directory.\n");
                    return;
                }
            }
        }
        if (!found) {
            printString("ls: no such file or directory: ");
            printString(dirname);
            printString("\n");
            return;
        }
    }


    // Iterate and print contents of the target directory
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == target_dir && strlen(node_fs_buf.nodes[i].node_name) > 0) {
            printString(node_fs_buf.nodes[i].node_name);
            if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                printString("/");
            }
            printString("  ");
        }
    }
    printString("\n");
}


// TODO: 8. Implement mv function
void mv(byte cwd, char* src, char* dst) {
    struct node_fs node_fs_buf;
    byte dest_parent_idx;
    char dest_filename[MAX_FILENAME];
    bool is_path_valid = true;
    int i, src_node_idx = -1;

    if (strlen(src) == 0 || strlen(dst) == 0) {
        printString("mv: missing operand\n");
        return;
    }

    get_dest_info(dst, cwd, &dest_parent_idx, dest_filename, &is_path_valid);
    if (!is_path_valid) {
        printString("mv: invalid destination\n");
        return;
    }
    
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Find source node
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == cwd && strcmp(node_fs_buf.nodes[i].node_name, src)) {
            src_node_idx = i;
            break;
        }
    }

    if (src_node_idx == -1) {
        printString("mv: file not found: ");
        printString(src);
        printString("\n");
        return;
    }

    if (node_fs_buf.nodes[src_node_idx].data_index == FS_NODE_D_DIR) {
        printString("mv: cannot move directories\n");
        return;
    }
    
    // Check for existing file at destination
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == dest_parent_idx && strcmp(node_fs_buf.nodes[i].node_name, dest_filename)) {
            printString("mv: destination file exists\n");
            return;
        }
    }

    // Move file
    node_fs_buf.nodes[src_node_idx].parent_index = dest_parent_idx;
    strcpy(node_fs_buf.nodes[src_node_idx].node_name, dest_filename);
    
    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {
    struct file_metadata read_meta, write_meta;
    enum fs_return status;
    byte dest_parent_idx;
    char dest_filename[MAX_FILENAME];
    bool is_path_valid = true;

    if (strlen(src) == 0 || strlen(dst) == 0) {
        printString("cp: missing operand\n");
        return;
    }

    // Read source file
    read_meta.parent_index = cwd;
    strcpy(read_meta.node_name, src);
    fsRead(&read_meta, &status);

    if (status == FS_R_NODE_NOT_FOUND) {
        printString("cp: file not found: ");
        printString(src);
        printString("\n");
        return;
    }
    if (status == FS_R_TYPE_IS_DIRECTORY) {
        printString("cp: cannot copy directories\n");
        return;
    }

    // Get destination info
    get_dest_info(dst, cwd, &dest_parent_idx, dest_filename, &is_path_valid);
    if (!is_path_valid) {
        printString("cp: invalid destination\n");
        return;
    }

    // Write destination file
    write_meta.parent_index = dest_parent_idx;
    strcpy(write_meta.node_name, dest_filename);
    write_meta.filesize = read_meta.filesize;
    memcpy(write_meta.buffer, read_meta.buffer, FS_MAX_SECTOR * SECTOR_SIZE);

    fsWrite(&write_meta, &status);

    if (status == FS_W_NODE_ALREADY_EXISTS) {
        printString("cp: destination file exists\n");
    } else if (status != FS_SUCCESS) {
        printString("cp: failed to copy file\n");
    }
}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {
    struct file_metadata metadata;
    enum fs_return status;
    int i;

    if (strlen(filename) == 0) {
        printString("cat: missing filename\n");
        return;
    }

    metadata.parent_index = cwd;
    strcpy(metadata.node_name, filename);

    fsRead(&metadata, &status);

    if (status == FS_R_NODE_NOT_FOUND) {
        printString("cat: file not found: ");
        printString(filename);
        printString("\n");
    } else if (status == FS_R_TYPE_IS_DIRECTORY) {
        printString("cat: is a directory: ");
        printString(filename);
        printString("\n");
    } else if (status == FS_SUCCESS) {
        // Null-terminate the buffer to print as a string
        metadata.buffer[metadata.filesize] = '\0';
        printString(metadata.buffer);
        printString("\n");
    }
}


// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {
    struct file_metadata metadata;
    enum fs_return status;

    if (strlen(dirname) == 0) {
        printString("mkdir: missing operand\n");
        return;
    }

    metadata.filesize = 0; // 0 filesize indicates a directory
    metadata.parent_index = cwd;
    strcpy(metadata.node_name, dirname);

    fsWrite(&metadata, &status);

    if (status == FS_W_NODE_ALREADY_EXISTS) {
        printString("mkdir: cannot create directory '");
        printString(dirname);
        printString("': File exists\n");
    } else if (status == FS_W_NO_FREE_NODE) {
        printString("mkdir: cannot create directory: No free node\n");
    } else if (status != FS_SUCCESS) {
        printString("mkdir: unknown error occurred\n");
    }
}

// Helper for parsing destination paths for cp and mv
void get_dest_info(char* path, byte cwd, byte* dest_parent_idx, char* filename, bool* valid) {
    struct node_fs node_fs_buf;
    int i;
    int last_slash_idx = -1;

    // Find the last slash
    for (i = 0; path[i] != '\0'; i++) {
        if (path[i] == '/') {
            last_slash_idx = i;
        }
    }

    if (last_slash_idx == -1) { // No slash, dest is in cwd
        *dest_parent_idx = cwd;
        strcpy(filename, path);
    } else {
        strcpy(filename, path + last_slash_idx + 1);
        path[last_slash_idx] = '\0'; // Temporarily cut the string

        if (last_slash_idx == 0) { // Path is like "/file"
            *dest_parent_idx = FS_NODE_P_ROOT;
        } else if (strcmp(path, "..")) {
            if (cwd == FS_NODE_P_ROOT) {
                *dest_parent_idx = FS_NODE_P_ROOT;
            } else {
                readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
                readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
                *dest_parent_idx = node_fs_buf.nodes[cwd].parent_index;
            }
        } else { // Path is like "dirname/file"
            bool found = false;
            readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
            readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
            for (i = 0; i < FS_MAX_NODE; i++) {
                if (node_fs_buf.nodes[i].parent_index == cwd && strcmp(node_fs_buf.nodes[i].node_name, path)) {
                    if(node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                        *dest_parent_idx = i;
                        found = true;
                    }
                    break;
                }
            }
            if(!found) *valid = false;
        }
    }
}
