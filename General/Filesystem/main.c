#include <kos.h>
#include <kos/string.h>

#include <stdio.h> 
#include <dirent.h>
#include <libgen.h>
#include <fat/fs_fat.h>

#define DIR 4096

#define BUFFER_LENGTH 512
#define MAX_DIRS 100

typedef struct {
    char FileName[256];
    int  IsDir;
} DirectoryFile;

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

static int mount_sd_fat();
static void unmount_sd_fat();

static void strcpy_ex(char* dest, char* source, int dest_size);
static int mount_romdisk(char* filepath, char* mountpoint);

static void show_prompt(char* current_directory, int mounted_sd, int highlight_yes);
static void delete_file(char* filename, int mounted_sd);

static void prompt_message(char* message, int highlight_yes);
static void show_current_directory(char* current_directory);

static void draw_directory_selector(int index);
static void draw_directory_contents(DirectoryFile directory_contents[], int num);

static int browse_directory(char* directory, DirectoryFile directory_contents[]);

static cont_state_t* get_cont_state();
static int button_pressed(unsigned int current_buttons, unsigned int changed_buttons, unsigned int button);

int main(int argc, char **argv) {
    cont_state_t* cond;
    int changed_directory = 1;
    int content_count = 0;
    int selector_index = 0;
    char current_directory[BUFFER_LENGTH] = {0};
    char directory_temp[BUFFER_LENGTH];
    char directory_temp2[BUFFER_LENGTH];
    DirectoryFile directory_contents[100];

    int prompting = 0;
    int highlight_yes = 1;
    int mounted_sd = 0;

    vid_set_mode(DM_640x480, PM_RGB555);

    // Mount the SD card if one is attached to the Dreamcast
    mounted_sd = mount_sd_fat();

    mount_romdisk("/cd/stage1.img", "/stage1");
    mount_romdisk("/cd/stage2.img", "/stage2");

    // Start off at the root of the dreamcast and get its contents
    //memset(current_directory, 0, BUFFER_LENGTH);
    fs_path_append(current_directory, "/", BUFFER_LENGTH);
    content_count = browse_directory(current_directory, directory_contents);
   
    unsigned int current_buttons = 0;
    unsigned int changed_buttons = 0;
    unsigned int previous_buttons = 0;

    for(;;) {
        cond = get_cont_state();
        current_buttons = cond->buttons;
        changed_buttons = current_buttons ^ previous_buttons;
        previous_buttons = current_buttons;
        
        // Enter a directory
        if(button_pressed(current_buttons, changed_buttons, CONT_A)) {
            if(prompting) {
                prompting = 0;
                changed_directory = 1;

                if(strcmp(current_directory, "/sd") == 0 ||
                   strcmp(current_directory, "/ram") == 0) {
                    if(highlight_yes) {
                        memset(directory_temp, 0, BUFFER_LENGTH);
                        fs_path_append(directory_temp, current_directory, BUFFER_LENGTH);
                        fs_path_append(directory_temp, directory_contents[selector_index].FileName, BUFFER_LENGTH);

                        delete_file(directory_temp, mounted_sd);
                        content_count = browse_directory(current_directory, directory_contents);
                        selector_index--;
                    }
                        
                    highlight_yes = 1;
                }  
                else {
                    if(highlight_yes) {
                        memset(directory_temp, 0, BUFFER_LENGTH);
                        fs_path_append(directory_temp, current_directory, BUFFER_LENGTH);
                        fs_path_append(directory_temp, directory_contents[selector_index].FileName, BUFFER_LENGTH);

                        memset(directory_temp2, 0, BUFFER_LENGTH);
                        if(mounted_sd)
                            fs_path_append(directory_temp2, "/sd", BUFFER_LENGTH);
                        else
                            fs_path_append(directory_temp2, "/ram", BUFFER_LENGTH);
                        fs_path_append(directory_temp2, basename(directory_temp), BUFFER_LENGTH);
                        fs_copy(directory_temp, directory_temp2);
                    }
                    highlight_yes = 1;
                }
            }
            else {
                // If we selected a directory
                if(directory_contents[selector_index].IsDir) {
                    // Navigate to it and get the directory contents
                    memset(directory_temp, 0, BUFFER_LENGTH);
                    if(strcmp(directory_contents[selector_index].FileName, "..") == 0) {
                        int copy_count = strrchr(current_directory, '/') - current_directory;
                        strncat(directory_temp, current_directory, (copy_count==0) ? 1 : copy_count);
                    } else {
                        fs_path_append(directory_temp, current_directory, BUFFER_LENGTH);
                        fs_path_append(directory_temp, directory_contents[selector_index].FileName, BUFFER_LENGTH);
                    }

                    content_count = browse_directory(directory_temp, directory_contents);
                    if(content_count > 0) {
                        strcpy_ex(current_directory, directory_temp, BUFFER_LENGTH);
                        changed_directory = 1;
                        selector_index = 0;
                    } 
                }
                else if(!prompting) { // We selected a file. Ask what we want to do with it
                    prompting = 1;
                    show_prompt(current_directory, mounted_sd, highlight_yes);
                }
            }
        }

        // Exit a directory
        if(button_pressed(current_buttons, changed_buttons, CONT_B)) {
            if(prompting) {
                prompting = 0;
                changed_directory = 1;
                highlight_yes = 1;
            }
            else {
                memset(directory_temp, 0, BUFFER_LENGTH);
                // If we are not currently in the root directory
                if(strcmp(current_directory, "/") != 0) {
                    int copy_count = strrchr(current_directory, '/') - current_directory;
                    strncat(directory_temp, current_directory, (copy_count==0) ? 1 : copy_count );
                    // Go the previous directory and get the directory contents
                    content_count = browse_directory(directory_temp, directory_contents);
                    if(content_count > 0) {
                        strcpy_ex(current_directory, directory_temp, BUFFER_LENGTH);
                        changed_directory = 1;
                        selector_index = 0;
                    } 
                }
            }
        }
        
        // Navigate the directory
        if(button_pressed(current_buttons, changed_buttons, CONT_DPAD_DOWN)) {
            if(prompting) {
                highlight_yes = !highlight_yes;
                show_prompt(current_directory, mounted_sd, highlight_yes);
            }
            else {
                if(selector_index < (content_count-1)) {
                    ++selector_index;
                    changed_directory = 1;
                }
            }
        }
        if(button_pressed(current_buttons, changed_buttons, CONT_DPAD_UP)) {
            if(prompting) {
                highlight_yes = !highlight_yes;
                show_prompt(current_directory, mounted_sd, highlight_yes);
            }
            else {
                if(selector_index > 0) {
                    --selector_index;
                    changed_directory = 1;
                }
            }
        }

        // Exit Program
        if(button_pressed(current_buttons, changed_buttons, CONT_START))
            break;

        // Update the screen if we navigate a directory
        if(changed_directory) {
            changed_directory = 0;
            vid_clear(0,0,0);
            draw_directory_selector(selector_index);
            draw_directory_contents(directory_contents, content_count);
            show_current_directory(current_directory);
        }
    }

    unmount_sd_fat();

    return 0;
}

static int mount_sd_fat() {
    kos_blockdev_t sd_dev;
    uint8 partition_type;

    if(sd_init()) {
        printf("Could not initialize the SD card. Please make sure that you "
               "have an SD card adapter plugged in and an SD card inserted.\n");
        return 0;
    }

    if(fs_fat_init()) {
        printf("Could not initialize fs_fat!\n");
        return 0;
    }
        
    if(sd_blockdev_for_partition(0, &sd_dev, &partition_type)) {
        printf("Could not find the first partition on the SD card!\n");
        return 0;
    }

    if(fs_fat_mount("/sd", &sd_dev, FS_FAT_MOUNT_READWRITE)) {
        printf("Could not mount SD card as fatfs. Please make sure the card "
            "has been properly formatted.\n");
        return 0;
    }

    printf("SD Card mounted Successfully\n");
    fflush(stdout);

    return 1;
}

static void unmount_sd_fat() {
    fs_fat_unmount("/sd");
    fs_fat_shutdown();
    sd_shutdown();
}

static int mount_romdisk(char* filepath, char* mountpoint) {
    void *buffer = NULL;
    ssize_t filesize = fs_load(filepath, &buffer);

    if(filesize != -1) {
        fs_romdisk_mount(mountpoint, buffer, 1);
        return 1;
    }
    else
        return 0;
}

static void show_current_directory(char* current_directory) {
    int x = 20 + 24, y = 430;
    int color = 1;

    bfont_set_foreground_color(0xFFFFFFFF);
    bfont_set_background_color(0x00000000);
    bfont_draw_str(vram_s + y*640+x, 640, color, current_directory);
}

static void show_prompt(char* current_directory, int mounted_sd, int highlight_yes) {
    if(strcmp(current_directory, "/sd") == 0 ||
       strcmp(current_directory, "/ram") == 0)  {
            if(mounted_sd)
                prompt_message("Delete this file from SD card?", highlight_yes);
            else
                prompt_message("Delete this file from /ram directory?", highlight_yes);
    }
    else {
        if(mounted_sd)
            prompt_message("Copy this file to SD card?", highlight_yes);
        else 
            prompt_message("Copy this file to /ram directory?", highlight_yes);
    }
}

static void delete_file(char* filename, int mounted_sd) {
    void* filedata = NULL;
    unsigned int filesize = 0;
    
    if(mounted_sd)
        remove(filename);
    else {
        fs_ramdisk_detach(basename(filename), &filedata, &filesize);
        free(filedata);
    }
}

static int browse_directory(char* directory, DirectoryFile directory_contents[]) {
    int count = 0;
    dirent_t    *de;
    file_t dr = fs_open(directory, O_RDONLY | O_DIR);

    if (!dr) return 0;

    // Clear out all files
    memset(directory_contents, 0, 100 * sizeof(DirectoryFile));
    
    // Add the back option if we are not in the root directory
    if(strcmp(directory, "/") != 0 && strcmp(directory, ".") != 0) {
        strcpy_ex(directory_contents[count].FileName, "..", 256);
        directory_contents[count].IsDir = 1;
        count++;
    }
    
    // Add files
    while ((de = fs_readdir(dr)) != NULL && count < 100) {
        if(strcmp(de->name, ".") != 0 && strcmp(de->name, "..") != 0) {
            directory_contents[count].IsDir = de->attr == DIR;
            strcpy_ex(directory_contents[count].FileName, de->name, 256);

            count++;
        } 
    }

    fs_close(dr);

    return count;
}

static void prompt_message(char* message, int highlight_yes) {
    int x = 20 + 24, y = 350;
    int color = 1;

    bfont_set_foreground_color(0xFFFFFFFF);
    bfont_set_background_color(0x00000000);
    bfont_draw_str(vram_s + y*640+x, 640, color, message);

    if(highlight_yes) {
        bfont_set_foreground_color(0x00000000);
        bfont_set_background_color(0xFFFFFFFF);
    }
    else {
        bfont_set_foreground_color(0xFFFFFFFF);
        bfont_set_background_color(0x00000000);
    }
    y += 24;
    bfont_draw_str(vram_s + y*640+x, 640, color, "YES");

    if(highlight_yes) {
        bfont_set_foreground_color(0xFFFFFFFF);
        bfont_set_background_color(0x00000000);
    }
    else {
        bfont_set_foreground_color(0x00000000);
        bfont_set_background_color(0xFFFFFFFF);
    }
    y += 24;
    bfont_draw_str(vram_s + y*640+x, 640, color, "NO");
}

static void draw_directory_selector(int index) {
    int x = 24, y = 24 + (index * 24);
    int color = 1;

    bfont_set_foreground_color(0xFFFFFFFF);
    bfont_set_background_color(0x00000000);
    bfont_draw_str(vram_s + y*640+x, 640, color, ">");
}

static void draw_directory_contents(DirectoryFile directory_contents[], int num) {
    int x = 20 + 24, y = 24;
    int color = 1;
    int count = 0;
    char str[80];

    bfont_set_foreground_color(0xFFFFFFFF);
    bfont_set_background_color(0x00000000);
    for(int i=0;i<num;i++) {
        if(directory_contents[i].IsDir) {
             sprintf(str, "%-40s%s", directory_contents[i].FileName, "< DIR >");
             bfont_draw_str(vram_s + y*640+x, 640, color, str);
        }
        else {
            bfont_draw_str(vram_s + y*640+x, 640, color, directory_contents[i].FileName);
        }
        count++;
        y += 24;

        if(y >= (480 - 24))
            break;
    }
}

static cont_state_t* get_cont_state() {
    maple_device_t* cont;
    cont_state_t* state;

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    if(cont) {
        state = (cont_state_t*)maple_dev_status(cont);
        return state;
    }

    return NULL;
}

static int button_pressed(unsigned int current_buttons, unsigned int changed_buttons, unsigned int button) {
    if (changed_buttons & button) {
        if (current_buttons & button)
            return 1;
    }

    return 0;
}

static void strcpy_ex(char* dest, char* source, int dest_size)
{
    if (dest_size > 0)
    {
        dest[0] = '\0';
        strncat(dest, source, dest_size - 1);
    }
}