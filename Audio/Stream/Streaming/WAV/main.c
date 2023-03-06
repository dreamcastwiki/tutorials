#include <kos.h>
#include <wav/sndwav.h>

#define LEFT 0
#define CENTER 128
#define RIGHT 255

#define LOOP 1
#define SHAKER_VOL 200
#define INITIAL_CRY 128
#define LOUDEST_CRY 240

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

static void draw_instructions(uint8_t volume);

static cont_state_t* get_cont_state();
static int button_pressed(unsigned int current_buttons, unsigned int changed_buttons, unsigned int button);

int main(int argc, char **argv) {
    uint8_t baby_volume = INITIAL_CRY;
    uint8_t shake_volume = SHAKER_VOL;

    uint64_t start_time = timer_ms_gettime64();
    uint64_t end_time = start_time;

    cont_state_t* cond;

    vid_set_mode(DM_640x480, PM_RGB555);
    // Initialize sound system and WAV
    snd_stream_init();
    wav_init();

    // Load wav files found in romdisk
    sfxhnd_t shake1 = snd_sfx_load("/rd/shake-1.wav");
    sfxhnd_t shake2 = snd_sfx_load("/rd/shake-2.wav");

    wav_stream_hnd_t wav_hnd = wav_create("/rd/baby-whining-01.wav", LOOP);
    wav_volume(wav_hnd, INITIAL_CRY);
    wav_play(wav_hnd);

    unsigned int current_buttons = 0;
    unsigned int changed_buttons = 0;
    unsigned int previous_buttons = 0;

    for(;;) {
        cond = get_cont_state();
        current_buttons = cond->buttons;
        changed_buttons = current_buttons ^ previous_buttons;
        previous_buttons = current_buttons;
        
        // Play rattle sounds to calm the baby
        if(button_pressed(current_buttons, changed_buttons, CONT_X)) {
            snd_sfx_play(shake1, shake_volume, LEFT);
            if(baby_volume > 40)
                baby_volume -= 10;
        }
        if(button_pressed(current_buttons, changed_buttons, CONT_Y)) {
            snd_sfx_play(shake2, shake_volume, RIGHT);
            if(baby_volume > 40)
                baby_volume -= 10;
        }
        // Wake the baby
        if(button_pressed(current_buttons, changed_buttons, CONT_A)) {
            if(!wav_isplaying(wav_hnd)) {
                baby_volume = INITIAL_CRY;
                wav_volume(wav_hnd, baby_volume);
                wav_play(wav_hnd);
            }
        }
        // Force the baby to sleep
        if(button_pressed(current_buttons, changed_buttons, CONT_B)) {
            baby_volume = 0;
            wav_stop(wav_hnd);
        }
        // Exit Program
        if(button_pressed(current_buttons, changed_buttons, CONT_START))
            break;

        // Adjust Volume with time
        end_time = timer_ms_gettime64();
        // Increase baby volume by 15 every second (Max LOUDEST_CRY)
        if((end_time - start_time) >= 1000) {
             baby_volume += 15;
             start_time = end_time;
        }

        if(baby_volume > LOUDEST_CRY) {
            baby_volume = LOUDEST_CRY;
        }

        // If baby volume goes <= 40, put baby to sleep
        if(baby_volume <= 40) {
            baby_volume = 0;
            wav_stop(wav_hnd);
        } else {
            wav_volume(wav_hnd, baby_volume);
        }

        draw_instructions(baby_volume);
    }

    // Unload all sound effects from sound RAM
    snd_sfx_unload_all();	

    wav_shutdown();
    snd_stream_shutdown();

    return 0;
}

static void draw_instructions(uint8_t baby_volume) {
    int x = 20, y = 20+24;
    int color = 1;
    char baby_status[50];
    memset(baby_status, 0, 50);
    
    if(baby_volume == 0) {
        snprintf(baby_status, 50, "%-50s", "Baby is asleep!!! Finally...");
    }
    else if(baby_volume > 40 && baby_volume <= 90) {
        snprintf(baby_status, 50, "%-50s", "Baby is almost asleep. Keep rattling!!");
    }
    else if(baby_volume > 90 && baby_volume <= 180) {
        snprintf(baby_status, 50, "%-50s", "You can do better than that!");
    }
    else if(baby_volume > 180 && baby_volume <= LOUDEST_CRY) {
        snprintf(baby_status, 50, "%-50s", "Are you even rattling!?!?!");
    }
    
    bfont_draw_str(vram_s + y*640+x, 640, color, "Press X and/or Y to play rattle sounds to calm");
    y += 24;
    bfont_draw_str(vram_s + y*640+x, 640, color, "down the baby so it can go to sleep");
    y += 48;
    bfont_draw_str(vram_s + y*640+x, 640, color, "Press A to wake the baby up");
    y += 48;
    bfont_draw_str(vram_s + y*640+x, 640, color, "Press B to force baby asleep");
    y += 48;
    bfont_draw_str(vram_s + y*640+x, 640, color, "Press Start to exit program");
    y += 72;
    bfont_draw_str(vram_s + y*640+x, 640, color, baby_status);
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
