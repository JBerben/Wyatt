/**
*** :: joycons.c ::
***
***   Handles Mainly Bluetooth conenctions, but also some
***   setup actions that require a serial IO connection for the
***   joycons/controller that will be talking to the Nintendo Switch.
***   Specifically, authentic Nintendo Switch joycons, require a 
***   one-time serial connection setup, to obtain the Bluetooth pairing
***   master key.
***
***   Since this class is about joycon setup, just as much as it is about
***   functionalities, it will contain implemented functionality of all afformentioned
***   concepts above, AS WELL as joycon action handling functions.
***  
***   Suggestions and contributions welcome:
***
***     Josh Berben | joshberben.work@gmail.com
***     
***     https://github.com/JBerben/Wyatt
***
**/

#include "joycons.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <hidapi/hidapi.h>
#include <libudev.h>
#include <sys/time.h>

/* We don't like magic numbers around here >:( .*/
#define INPUT_LOOP
#define JOYCON_L_BT (0x2006)
#define JOYCON_R_BT (0x2007)
#define PRO_CONTROLLER (0x2009)
#define JOYCON_CHARGING_GRIP (0x200e)

/* Related towards the different components of a possible JC setup. */
const unsigned short NUM_PRODUCT_IDS = 4;
const unsigned short PRODUCT_IDS[] = {JOYCON_L_BT, JOYCON_R_BT, PRO_CONTROLLER, JOYCON_CHARGING_GRIP};

bool disconnect = false;
bool bluetooth = true;
uint8_t global_count = 0;

/**
 * Most names here are pretty self explanatory.
 * BTN_T(L/R)(1/2)
 * is referring to the left and right trigger
 * buttons. (1 and 2).
 * Also known as ZL and ZR buttons.
 * 
 *    * FOR SUPER DUPER CLARITY;
 *      BTN_TL1 -> [BTN_LEFT_TRIGGER == BTN_TL1]
 *      BTN_TL2 -> [BTN_LEFT_ZTRIGGER == BTN_TL2]
 *      BTN_TR1 -> [BTN_RIGHT_TRIGGER == BTN_TR1]
 *      BTN_TR2 -> [BTN_RIGHT_ZTRIGGER == BTN_TR2] 
 * 
 * The next four arrays, are an attempt at
 * abridging the usually 2 seperate joycons together
 * and having a funcitonal button and bitflag map,
 * accurately representing what a simese joycon twin would
 * look and act like. 
 * 
 * TODO: Perhaps I should remove the simese joke.
 *       Possibly a bit too dark...    
 */
int joycon_bits_to_buttons_left[] = {
    BTN_DPAD_DOWN,
    BTN_DPAD_UP,
    BTN_DPAD_RIGHT,
    BTN_DPAD_LEFT,
    -1,
    -1,
    BTN_TL,
    BTN_TL2,
};

int joycon_bits_to_buttons_right[] = {
    BTN_WEST,
    BTN_NORTH,
    BTN_EAST,
    BTN_SOUTH,
    -1,
    -1,
    BTN_TR,
    BTN_TR2,
};

int joycon_bits_to_buttons_middle_left[] = {
    BTN_SELECT,
    -1,
    -1,
    BTN_THUMBL,
    -1,
    BTN_Z,
    -1,
    -1,
};

int joycon_bits_to_buttons_middle_right[] = {
    -1,
    BTN_START,
    BTN_THUMBR,
    -1,
    BTN_MODE,
    -1,
    -1,
    -1,
};

/**
 * Data structure of how an input packet
 * is constructed, before being sent off.
 */
typedef struct input_packet {
    uint8_t header[8];
    uint8_t unk1[5];
    uint8_t buttons_r;
    uint8_t buttons_middle;
    uint8_t buttons_l;
    uint8_t sticks[6];
} InputPacket;


/**
 * FUNCTIONS
 */

/**
 * Vertically prints to the console in hex,
 * what was being held in a particular data-buffer.
 */
void hex_dump(unsigned char *buf, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
    }
    printf("\n");
}

/**
 * Pairs us up with a joycon controller.
 */
void hid_exchange(hid_device *handle, unsigned char *buf, int len) {
    if(!handle) return;
    
#ifdef DEBUG_PRINT
    hex_dump(buf, len);
#endif
    
    hid_write(handle, buf, len);

    int res = hid_read(handle, buf, 0x400);
#ifdef DEBUG_PRINT
    if(res > 0)
        hex_dump(buf, res);
#endif
}

/**
 * This is the real deal here. IF and ONLY IF a pair of
 * joycons are either docked and connected via C-cable, or connected
 * in this similar fashion, but instead to a progrip charging controller,
 * this function will attempt to pair with both, and set our connection to them up
 * as if we're connecting to both joycons as if they were a complete controller.
 */
int hid_dual_exchange(hid_device *handle_l, hid_device *handle_r, unsigned char *buf_l, unsigned char *buf_r, int len) {
    int res = -1;
    
    if(handle_l && buf_l) {
        hid_set_nonblocking(handle_l, 1);
        res = hid_write(handle_l, buf_l, len);
        res = hid_read(handle_l, buf_l, 0x400);
#ifdef DEBUG_PRINT
        if(res > 0)
            hex_dump(buf_l, res);
#endif
        hid_set_nonblocking(handle_l, 0);
    }
    
    if(handle_r && buf_r) {
        hid_set_nonblocking(handle_r, 1);
        res = hid_write(handle_r, buf_r, len);
        //usleep(17000);
        do {
            res = hid_read(handle_r, buf_r, 0x400);
        }
        while(!res);
#ifdef DEBUG_PRINT
        if(res > 0)
            hex_dump(buf_r, res);
#endif
        hid_set_nonblocking(handle_r, 0);
    }
    
    return res;
}

/**
 * Writes data to a dual joycon setup.
 * CAUTION!!! This writes data to the ROM permanently, and should
 * only be used when uploading your own custom firmware to your joycons.
 */
void hid_dual_write(hid_device *handle_l, hid_device *handle_r, unsigned char *buf_l, unsigned char *buf_r, int len) {
    int res;
    
    if(handle_l && buf_l) {
        hid_set_nonblocking(handle_l, 1);
        res = hid_write(handle_l, buf_l, len);
        
        if(res < 0) {
            disconnect = true;
            return;
        }
        
        hid_set_nonblocking(handle_l, 0);
    }
    
    if(handle_r && buf_r) {
        hid_set_nonblocking(handle_r, 1);
        res = hid_write(handle_r, buf_r, len);
        
        if(res < 0) {
            disconnect = true;
            return;
        }
        
        hid_set_nonblocking(handle_r, 0);
    }
}

/**
 * Sends a command to a joycon.
 */
void joycon_send_command(hid_device *handle, int command, uint8_t *data, int len) {
    unsigned char buf[0x400];
    memset(buf, 0, 0x400);
    
    if(!bluetooth) {
        buf[0x00] = 0x80;
        buf[0x01] = 0x92;
        buf[0x03] = 0x31;
    }
    
    buf[bluetooth ? 0x0 : 0x8] = command;
    if(data != NULL && len != 0) {
        memcpy(buf + (bluetooth ? 0x1 : 0x9), data, len);
    }
    
    hid_exchange(handle, buf, len + (bluetooth ? 0x1 : 0x9));
    if(data) {
        memcpy(data, buf, 0x40);
    }
    
}

/**
 * Sends a subcommand
 */
void joycon_send_subcommand(hid_device *handle, int command, int subcommand, uint8_t *data, int len) {
    unsigned char buf[0x400];
    memset(buf, 0, 0x400);
    
    uint8_t rumble_base[9] = {(++global_count) & 0xF, 0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40};
    memcpy(buf, rumble_base, 9);
    
    buf[9] = subcommand;
    if(data && len != 0) {
        memcpy(buf + 10, data, len);   
    }
        
    joycon_send_command(handle, command, buf, 10 + len);
        
    if(data) {
        memcpy(data, buf, 0x40); //TODO: Might need some revising....
    }
}

/**
 * Writes data to the joycon through SPI;
 * "Serial Programmable Interface".
 * Also known as a "cable".
 * 
 * !CAUTION! Don't use this unless you want to permanently
 * modify the memory of the joycon.
 */
void spi_write(hid_device *handle, uint32_t offs, uint8_t *data, uint8_t len) {
    unsigned char buf[0x400];
    uint8_t *spi_write = (uint8_t*)calloc(1, 0x26 * sizeof(uint8_t));
    uint32_t* offset = (uint32_t*)(&spi_write[0]);
    uint8_t* length = (uint8_t*)(&spi_write[4]);
   
    *length = len;
    *offset = offs;
    memcpy(&spi_write[0x5], data, len);
   
    int max_write_count = 2000;
    int write_count = 0;
    do {
        //usleep(300000);
        write_count += 1;
        memcpy(buf, spi_write, 0x39);
        joycon_send_subcommand(handle, 0x1, 0x11, buf, 0x26);
    }
    // Normally a sleep might be safe here, but this fucking horrid while loop does the job better.
    while((buf[0x10 + (bluetooth ? 0 : 10)] != 0x11 && buf[0] != (bluetooth ? 0x21 : 0x81))
        && write_count < max_write_count);
	if(write_count > max_write_count) {
        printf("ERROR: Write error or timeout\nSkipped writing of %dBytes at address 0x%05X...\n", *length, *offset);
    }

}

/**
 * Reads data from a joycon from a serial input.
 */
void spi_read(hid_device *handle, uint32_t offs, uint8_t *data, uint8_t len) {
    unsigned char buf[0x400];
    uint8_t *spi_read_cmd = (uint8_t*)calloc(1, 0x26 * sizeof(uint8_t));
    uint32_t* offset = (uint32_t*)(&spi_read_cmd[0]);
    uint8_t* length = (uint8_t*)(&spi_read_cmd[4]);
   
    *length = len;
    *offset = offs;
   
    int max_read_count = 2000;
	int read_count = 0;
    do {
        //usleep(300000);
		read_count += 1;
        memcpy(buf, spi_read_cmd, 0x36);
        joycon_send_subcommand(handle, 0x1, 0x10, buf, 0x26);
    }
    
    while(*(uint32_t*)&buf[0xF + (bluetooth ? 0 : 10)] != *offset && read_count < max_read_count);
	if(read_count > max_read_count) {
        printf("ERROR: Read error or timeout\nSkipped reading of %dBytes at address 0x%05X...\n", *length, *offset);
    }
    
    memcpy(data, &buf[0x14 + (bluetooth ? 0 : 10)], len);
}

void spi_flash_dump(hid_device *handle, char *out_path) {
    unsigned char buf[0x400];
    uint8_t *spi_read_cmd = (uint8_t*)calloc(1, 0x26 * sizeof(uint8_t));
    int safe_length = 0x10; // 512KB fits into 32768 * 16B packets
    int fast_rate_length = 0x1D; // Max SPI data that fit into a packet is 29B. Needs removal of last 3 bytes from the dump.
    
    int length = fast_rate_length;
    
    FILE *dump = fopen(out_path, "wb");
    if(dump == NULL) {
        printf("Failed to open dump file %s, aborting...\n", out_path);
        return;
    }
    
    uint32_t* offset = (uint32_t*)(&spi_read_cmd[0x0]);
    for(*offset = 0x0; *offset < 0x80000; *offset += length) {
        // TODO: hid_exchange often returns data from the wrong addr, or 0x30 (NACK?) packets
        // so let's make sure our returned data is okay before writing

        //Set length of requested data
        spi_read_cmd[0x4] = length;

        int max_read_count = 2000;
        int read_count = 0;
        while(1) {
            read_count += 1;
            memcpy(buf, spi_read_cmd, 0x26);
            joycon_send_subcommand(handle, 0x1, 0x10, buf, 0x26);
            
            // sanity-check our data, loop if it's not good
            // Shout-out to the ugliest conditional statement of 2019 (god i love c).
            if((buf[0] == (bluetooth ? 0x21 : 0x81)
                && *(uint32_t*)&buf[0xF + (bluetooth ? 0 : 10)] == *offset) 
                || read_count > max_read_count)
                break;
        }

        if(read_count > max_read_count) {
			printf("\n\nERROR: Read error or timeout.\nSkipped dumping of %dB at address 0x%05X...\n\n", length, *offset);
            return;
        }

        fwrite(buf + (0x14 + (bluetooth ? 0 : 10)) * sizeof(char), length, 1, dump);
        // less spam
        if((*offset & 0xFF) == 0) {
            printf("\rDumped 0x%05X of 0x80000", *offset);
        } 
    }
    printf("\rDumped 0x80000 of 0x80000\n");
    fclose(dump);
}

/**
 * Initializes a single joycon.
 */
int joycon_init(hid_device *handle, const wchar_t *name) {
    unsigned char buf[0x400];
    unsigned char sn_buffer[14] = {0x00};
    memset(buf, 0, 0x400);
    
    
    if(!bluetooth) {
        // Get MAC Left
        memset(buf, 0x00, 0x400);
        buf[0] = 0x80;
        buf[1] = 0x01;
        hid_exchange(handle, buf, 0x2);
        
        if(buf[2] == 0x3) {
            printf("%ls disconnected!\n", name);
            return -1;
        }
        else {
            printf("Found %ls, MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", name,
                   buf[9], buf[8], buf[7], buf[6], buf[5], buf[4]);
        }
            
        // Do handshaking
        memset(buf, 0x00, 0x400);
        buf[0] = 0x80;
        buf[1] = 0x02;
        hid_exchange(handle, buf, 0x2);
        
        printf("Switching baudrate...\n");
        
        // Switch baudrate to 3Mbit
        memset(buf, 0x00, 0x400);
        buf[0] = 0x80;
        buf[1] = 0x03;
        hid_exchange(handle, buf, 0x2);
        
        // Do handshaking again at new baudrate so the firmware pulls pin 3 low?
        memset(buf, 0x00, 0x400);
        buf[0] = 0x80;
        buf[1] = 0x02;
        hid_exchange(handle, buf, 0x2);
        
        // Only talk HID from now on
        memset(buf, 0x00, 0x400);
        buf[0] = 0x80;
        buf[1] = 0x04;
        hid_exchange(handle, buf, 0x2);
    }

    // Enable vibration
    memset(buf, 0x00, 0x400);
    buf[0] = 0x01; // Enabled
    joycon_send_subcommand(handle, 0x1, 0x48, buf, 1);
    
    // Enable IMU data
    memset(buf, 0x00, 0x400);
    buf[0] = 0x01; // Enabled
    joycon_send_subcommand(handle, 0x1, 0x40, buf, 1);
    
    // Increase data rate for Bluetooth
    if (bluetooth) {
       memset(buf, 0x00, 0x400);
       buf[0] = 0x31; // Enabled
       joycon_send_subcommand(handle, 0x1, 0x3, buf, 1);
    }
    
    //Read device's S/N
    spi_read(handle, 0x6002, sn_buffer, 0xE);
    
    printf("Successfully initialized %ls with S/N: %c%c%c%c%c%c%c%c%c%c%c%c%c%c!\n", 
        name, sn_buffer[0], sn_buffer[1], sn_buffer[2], sn_buffer[3], 
        sn_buffer[4], sn_buffer[5], sn_buffer[6], sn_buffer[7], sn_buffer[8], 
        sn_buffer[9], sn_buffer[10], sn_buffer[11], sn_buffer[12], 
        sn_buffer[13]);
    
    return 0;
}

/**
 * Disconnects from a single joycon.
 */
void joycon_deinit(hid_device *handle, const wchar_t *name) {
    unsigned char buf[0x400];
    memset(buf, 0x00, 0x400);

    //Let the Joy-Con talk BT again 
    if(!bluetooth) {   
        buf[0] = 0x80;
        buf[1] = 0x05;
        hid_exchange(handle, buf, 0x2);
    }
    
    printf("Deinitialized %ls\n", name);
}

void device_print(struct hid_device_info *dev) {
    printf("USB device info:\n  vid: 0x%04hX pid: 0x%04hX\n  path: %s\n  MAC: %ls\n  interface_number: %d\n",
        dev->vendor_id, dev->product_id, dev->path, dev->serial_number, dev->interface_number);
    printf("  Manufacturer: %ls\n", dev->manufacturer_string);
    printf("  Product:      %ls\n\n", dev->product_string);
}

void joycon_parse_input(int fd, unsigned char *data, int type) {
    struct input_event ev;
    struct input_packet *input = (struct input_packet*)data;
    
    //Left
    if(type & 1) {
        for(int i = 0; i < 8; i++) {
            if(joycon_bits_to_buttons_left[i] < 0) continue;
            
            memset(&ev, 0, sizeof(struct input_event));
            ev.type = EV_KEY;
            ev.code = joycon_bits_to_buttons_left[i];
            ev.value = (input->buttons_l & (1 << i)) ? 1 : 0;
            write(fd, &ev, sizeof(struct input_event));
        }
        
        for(int i = 0; i < 8; i++) {
            if(joycon_bits_to_buttons_middle_left[i] < 0) continue;
            
            memset(&ev, 0, sizeof(struct input_event));
            ev.type = EV_KEY;
            ev.code = joycon_bits_to_buttons_middle_left[i];
            ev.value = (input->buttons_middle & (1 << i)) ? 1 : 0;
            write(fd, &ev, sizeof(struct input_event));
        }
        
        int stick_x = ((input->sticks[1] & 0x0F) << 4) | ((input->sticks[0] & 0xF0) >> 4);
        int stick_y = 256 - input->sticks[2];
        
        memset(&ev, 0, sizeof(struct input_event));
        ev.type = EV_ABS;
        ev.code = ABS_X;
        ev.value = stick_x;
        write(fd, &ev, sizeof(struct input_event));
        
        memset(&ev, 0, sizeof(struct input_event));
        ev.type = EV_ABS;
        ev.code = ABS_Y;
        ev.value = stick_y;
        write(fd, &ev, sizeof(struct input_event));
    }

    //Right
    if(type & 2) {

        for(int i = 0; i < 8; i++) {
            if(joycon_bits_to_buttons_right[i] < 0) continue;
            
            memset(&ev, 0, sizeof(struct input_event));
            ev.type = EV_KEY;
            ev.code = joycon_bits_to_buttons_right[i];
            ev.value = (input->buttons_r & (1 << i)) ? 1 : 0;
            write(fd, &ev, sizeof(struct input_event));
        }
        
        for(int i = 0; i < 8; i++) {
            if(joycon_bits_to_buttons_middle_right[i] < 0) continue;
            
            memset(&ev, 0, sizeof(struct input_event));
            ev.type = EV_KEY;
            ev.code = joycon_bits_to_buttons_middle_right[i];
            ev.value = (input->buttons_middle & (1 << i)) ? 1 : 0;
            write(fd, &ev, sizeof(struct input_event));
        }
        
        int stick_x = ((input->sticks[4] & 0x0F) << 4) | ((input->sticks[3] & 0xF0) >> 4);
        int stick_y = 256 - input->sticks[5];
        
        memset(&ev, 0, sizeof(struct input_event));
        ev.type = EV_ABS;
        ev.code = ABS_RX;
        ev.value = stick_x;
        write(fd, &ev, sizeof(struct input_event));
        
        memset(&ev, 0, sizeof(struct input_event));
        ev.type = EV_ABS;
        ev.code = ABS_RY;
        ev.value = stick_y;
        write(fd, &ev, sizeof(struct input_event));
    }
}

/**
 * This will be moved into wengine.c as it will be the
 * API's entry point. But for now, I'll leave this here,
 * since the other classes are pretty empty...
 */
int main(void) {
    int fd;
    int res;
    struct udev *udev;
    struct udev_device *uinput;
    struct uinput_user_dev udevice;
    
    unsigned char buf[2][0x400] = {0};
    hid_device *handle_l = 0, *handle_r = 0;
    const wchar_t *device_name = L"none";
    struct hid_device_info *devs, *dev_iter;
    bool charging_grip = false;

    // Set up udev, get a path, open the file for ioctling
    udev = udev_new();
    if (udev == NULL) {
        fprintf(stderr, "udev init errors\n");
        return -1;
    } 

    uinput = udev_device_new_from_subsystem_sysname(udev, "misc", "uinput");
    if (uinput == NULL) {
        fprintf(stderr, "uinput creation failed\n");
        return -1;
    }

    const char *uinput_path = udev_device_get_devnode(uinput);
    if (uinput_path == NULL) {
        fprintf(stderr, "cannot find path to uinput\n");
        return -1;
    }

    fd = open(uinput_path, O_WRONLY | O_NONBLOCK);
        
    // Buttons
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_NORTH);
    ioctl(fd, UI_SET_KEYBIT, BTN_SOUTH);
    ioctl(fd, UI_SET_KEYBIT, BTN_EAST);
    ioctl(fd, UI_SET_KEYBIT, BTN_WEST);
    ioctl(fd, UI_SET_KEYBIT, BTN_START);
    ioctl(fd, UI_SET_KEYBIT, BTN_SELECT);
    ioctl(fd, UI_SET_KEYBIT, BTN_MODE);
    ioctl(fd, UI_SET_KEYBIT, BTN_Z);
    ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_UP);
    ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_DOWN);
    ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_LEFT);
    ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_RIGHT);
    ioctl(fd, UI_SET_KEYBIT, BTN_THUMBL);
    ioctl(fd, UI_SET_KEYBIT, BTN_THUMBR);
    ioctl(fd, UI_SET_KEYBIT, BTN_TL);
    ioctl(fd, UI_SET_KEYBIT, BTN_TL2);
    ioctl(fd, UI_SET_KEYBIT, BTN_TR);
    ioctl(fd, UI_SET_KEYBIT, BTN_TR2);
    
    // Joysticks
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_ABSBIT, ABS_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);
    ioctl(fd, UI_SET_ABSBIT, ABS_RX);
    ioctl(fd, UI_SET_ABSBIT, ABS_RY);

    memset(&udevice, 0, sizeof(udevice));
    snprintf(udevice.name, UINPUT_MAX_NAME_SIZE, "joycon");
    udevice.id.bustype = BUS_USB;
    udevice.id.vendor  = 0x1;
    udevice.id.product = 0x1;
    udevice.id.version = 1;
    
    for(int i = ABS_X; i <= ABS_RZ; i++) {
        ioctl(fd, UI_SET_ABSBIT, i);
        udevice.absmin[i] = 32;
        udevice.absmax[i] = 255 - 32;
    }

    // Write our device description
    write(fd, &udevice, sizeof(udevice));
    ioctl(fd, UI_DEV_CREATE);

    // Start talking HID
    res = hid_init();
    if(res) {
        printf("Failed to open hid library! Exiting...\n");
        return -1;
    }
    
init_start:
    disconnect = false;
    charging_grip = false;

    if(handle_l) {
        hid_close(handle_l);
        handle_l = 0;
    }
    
    if(handle_r) {
        hid_close(handle_r);
        handle_r = 0;
    }

    // iterate thru all the valid product ids and try and initialize controllers
    for(int i = 0; i < NUM_PRODUCT_IDS; i++) {
        devs = hid_enumerate(0x057E, PRODUCT_IDS[i]);
        dev_iter = devs;
        while(dev_iter) {
            // Sometimes hid_enumerate still returns other product IDs
            if (dev_iter->product_id != PRODUCT_IDS[i]) break;
            
            // break out if the current handle is already used
            if((dev_iter->product_id == JOYCON_R_BT || dev_iter->interface_number == 0) && handle_r)
                break;
            else if((dev_iter->product_id == JOYCON_L_BT || dev_iter->interface_number == 1) && handle_l)
                break;
            
            device_print(dev_iter);
            
            if(!wcscmp(dev_iter->serial_number, L"000000000001")) {
                bluetooth = false;
            }
            else if(!bluetooth) {
                printf("Can't mix USB HID with Bluetooth HID, exiting...\n");
                return -1;
            }
            
            // on windows this will be -1 for devices with one interface
            if(dev_iter->interface_number == 0 || dev_iter->interface_number == -1) {
                hid_device *handle = hid_open_path(dev_iter->path);
                if(handle == NULL) {
                    printf("Failed to open controller at %ls, continuing...\n", dev_iter->path);
                    dev_iter = dev_iter->next;
                    continue;
                }
                
                switch(dev_iter->product_id) {
                    case JOYCON_CHARGING_GRIP:
                        device_name = L"Joy-Con (R)";
                        charging_grip = true;

                        handle_r = handle;
                        break;
                    case PRO_CONTROLLER:
                        device_name = L"Pro Controller";

                        handle_r = handle;
                        break;
                    case JOYCON_L_BT:
                        device_name = L"Joy-Con (L)";

                        handle_l = handle;
                        break;
                    case JOYCON_R_BT:
                        device_name = L"Joy-Con (R)";

                        handle_r = handle;
                        break;
                }
                
                if(joycon_init(handle, device_name)) {
                    hid_close(handle);
                    if(dev_iter->product_id != JOYCON_L_BT)
                        handle_r = NULL;
                    else
                        handle_l = NULL;
                }
            }
            // Only exists for left Joy-Con in the charging grip
            else if(dev_iter->interface_number == 1) {
                handle_l = hid_open_path(dev_iter->path);
                if(handle_l == NULL) {
                    printf("Failed to open controller at %ls, continuing...\n", dev_iter->path);
                    dev_iter = dev_iter->next;
                    continue;
                }
                
                if(joycon_init(handle_l, L"Joy-Con (L)"))
                    handle_l = NULL;
            }
            dev_iter = dev_iter->next;
        }
        hid_free_enumeration(devs);
    }
    
    if(!handle_r) {
        printf("Failed to get handle for right Joy-Con or Pro Controller, exiting...\n");
        return -1;
    }
    
    // Only missing one half by this point
    if(!handle_l && charging_grip) {
        printf("Could not get handles for both Joy-Con in grip! Exiting...\n");
        return -1;
    }
    
    // controller init is complete at this point
    printf("Start input poll loop\n");
    
    struct timeval start, end;
    struct input_event ev;
    
    while(1) {
        gettimeofday(&start, 0);
        buf[0][0] = 0x80; // 80     Do custom command
        buf[0][1] = 0x92; // 92     Post-handshake type command
        buf[0][2] = 0x00; // 0001   u16 second part size
        buf[0][3] = 0x01;
        buf[0][8] = 0x1F; // 1F     Get input command
        
        // Ask for input from all Joy-Con
        if(buf[1]) {
            memcpy(buf[1], buf[0], 0x9);
        }
        hid_dual_write(handle_l, handle_r, buf[1], buf[0], 0x9);
        
        // Try and read the right for any input packets
        hid_set_nonblocking(handle_r, 1);
        do {
            res = hid_read(handle_r, buf[0], 0x40);
            if(res) {
                switch(buf[0][5]) {
                    case 0x31:
                        joycon_parse_input(fd, buf[0], charging_grip ? 0x2 : 0x3); //TODO?
                        gettimeofday(&end, 0);
                        uint64_t delta_ms = (end.tv_sec*1000LL + end.tv_usec/1000) - (start.tv_sec*1000LL + start.tv_usec/1000);
                        printf("%02llums delay,  ", delta_ms);
                        hex_dump(buf[0], 0x40);
                        break;
                    
                    default:
                    break;
                }
            }
        }
        while(res);
        hid_set_nonblocking(handle_r, 0);
        
        // Try and read the left for any input packets
        if(handle_l) {
            hid_set_nonblocking(handle_l, 1);
            do {
                res = hid_read(handle_l, buf[1], 0x40);
                if(res) {
                    switch(buf[1][5]) {
                        case 0x31:
                            joycon_parse_input(fd, buf[1], 0x1); //TODO?
                            gettimeofday(&end, 0);
                            uint64_t delta_ms = (end.tv_sec*1000LL + end.tv_usec/1000) - (start.tv_sec*1000LL + start.tv_usec/1000);
                            printf("%02llums delay,  ", delta_ms);
                            hex_dump(buf[1], 0x40);
                            break;
                        
                        default:
                        break;
                    }
                }
            }
            while(res);
            hid_set_nonblocking(handle_l, 0);
        }
        
        gettimeofday(&end, 0);
        
        // Sync our input state
        memset(&ev, 0, sizeof(struct input_event));
        ev.type = EV_SYN;
        ev.code = 0;
        ev.value = 0;
        write(fd, &ev, sizeof(struct input_event));
        
        if(disconnect) goto init_start;
    }

    if(handle_l) {
        joycon_deinit(handle_l, L"Joy-Con (L)");
        hid_close(handle_l);
    }
    
    if(handle_r) {
        joycon_deinit(handle_r, device_name);
        hid_close(handle_r);
    }

    // Finalize the hidapi library
    res = hid_exit();

    // Finalize udev
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);

    return 0;
}