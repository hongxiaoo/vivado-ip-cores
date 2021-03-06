
#include "sleep.h"
#include "xil_io.h"
#include "xstatus.h"
#include "axi_sony_imx_controller.h"

#define MAX_CAMERA_COUNT            	3
#define MAX_LANE_WIDTH              	16

#define SIZE_TAP_DELAY                (MAX_CAMERA_COUNT * MAX_LANE_WIDTH)

const u32 REG_CONTROL                 = 0  << 2;
const u32 REG_STATUS                  = 1  << 2;
const u32 REG_TRIG_EXP_WIDTH          = 2  << 2;
const u32 REG_TRIG_CAP_PERIOD         = 3  << 2;
const u32 REG_CAMERA_COUNT            = 4  << 2;
const u32 REG_LANE_WIDTH              = 5  << 2;
const u32 REG_ALIGNED_FLAG_LOW        = 6  << 2;
const u32 REG_ALIGNED_FLAG_HIGH       = 7  << 2;
const u32 REG_FRAME_WIDTH             = 8  << 2;
const u32 REG_FRAME_HEIGHT            = 9  << 2;
const u32 REG_PRE_VERTICAL_BLANK      = 10 << 2;
const u32 REG_PRE_HORIZONTAL_BLANK    = 11 << 2;
const u32 REG_POST_VERTICAL_BLANK     = 12 << 2;
const u32 REG_POST_HORIZONTAL_BLANK   = 13 << 2;



#define REG_TAP_DELAY_START         	(16 << 2)
#define REG_TAP_ERROR_START         	(REG_TAP_DELAY_START + (SIZE_TAP_DELAY << 2))
#define REG_VERSION                   (REG_TAP_ERROR_START + (SIZE_TAP_DELAY << 2))

const u32 CTRL_BIT_CLEAR_EN           = 0;
const u32 CTRL_BIT_TRIGGER_EN         = 1;
const u32 CTRL_BIT_CAM_ASYNC_RST_EN   = 2;
const u32 CTRL_BIT_CAM_SYNC_RST_EN    = 3;
const u32 CTRL_BIT_DETECT_ERROR_EN    = 4;
const u32 CTRL_BIT_LOAD_TAP_EN        = 5;

const u32 CTRL_BIT_POWER_EN0          = 12;
const u32 CTRL_BIT_POWER_EN1          = 13;
const u32 CTRL_BIT_POWER_EN2          = 14;


//Private Function Prototypes
u32 imxc_read_register(imx_control_t *ic, u32 address);
void imxc_write_register(imx_control_t *ic, u32 address, u32 value);
void imxc_enable_register_bit(imx_control_t *ic, u32 address, u32 bit_index, u8 enable);
void imxc_set_register_bit(imx_control_t *ic, u32 address, u32 bit_index);
void imxc_clear_register_bit(imx_control_t *ic, u32 address, u32 bit_index);
u32 imxc_is_register_bit_set(imx_control_t *ic, u32 address, u32 bit_index);

int setup_imx_control(imx_control_t * ic, u32 base_address){
  int retval = XST_SUCCESS;
  ic->base_address = base_address;
  return retval;
}
u32 imx_control_get_status(imx_control_t * ic){
  return imxc_read_register(ic, REG_STATUS);
}
u32 imx_control_get_version(imx_control_t *ic){
  return imxc_read_register(ic, REG_VERSION);
}
void imx_control_setup_trigger(imx_control_t *ic, u32 capture_period, u32 exposure_period){
  imxc_write_register(ic, REG_TRIG_EXP_WIDTH, exposure_period);
  imxc_write_register(ic, REG_TRIG_CAP_PERIOD, capture_period);
}
u8 imx_control_get_camera_count(imx_control_t *ic){
  return (u8) imxc_read_register(ic, REG_CAMERA_COUNT);
}
u8 imx_control_get_lane_width(imx_control_t *ic){
  return (u8) imxc_read_register(ic, REG_LANE_WIDTH);
}
u64 imx_control_get_align_flags(imx_control_t *ic){
  u64 flags = 0;
  flags = (u64) (imxc_read_register(ic, REG_ALIGNED_FLAG_HIGH));
  flags = flags << 32;
  flags |= imxc_read_register(ic, REG_ALIGNED_FLAG_LOW);
  return flags;
}
u32 imx_control_get_camera_align_flags(imx_control_t *ic, u8 cam_index){
  u64 flags = imx_control_get_align_flags(ic);
  if (cam_index >= ((u64) MAX_CAMERA_COUNT))
    return 0xFFFF0000;
  return (flags >> (cam_index * MAX_LANE_WIDTH) & ((1 << MAX_LANE_WIDTH) - 1));
}
int imx_control_is_all_camera_lanes_aligned(imx_control_t *ic, u8 cam_index){
  u32 flags = imx_control_get_camera_align_flags(ic, cam_index);
  u8 lanes = imx_control_get_lane_width(ic);
  u32 mask = (1 << lanes) - 1;

  if (flags == 0xFFFF0000)
    return 0;
  return (flags == mask);
}
void imx_control_reset_async_cam_clock_domain_enable(imx_control_t * ic, u8 enable){
  imxc_enable_register_bit(ic, REG_CONTROL, CTRL_BIT_CAM_ASYNC_RST_EN, enable);
}
void imx_control_reset_sync_cam_clock_domain_enable(imx_control_t *ic, u8 enable){
  imxc_enable_register_bit(ic, REG_CONTROL, CTRL_BIT_CAM_SYNC_RST_EN, enable);
  imxc_enable_register_bit(ic, REG_CONTROL, CTRL_BIT_LOAD_TAP_EN, enable);
}
void imx_control_reset_sensor_enable(imx_control_t *ic, u8 enable){
  imxc_enable_register_bit(ic, REG_CONTROL, CTRL_BIT_CLEAR_EN, enable);
}
void imx_control_enable_tap_error_read(imx_control_t *ic, u8 enable){
  imxc_enable_register_bit(ic, REG_CONTROL, CTRL_BIT_DETECT_ERROR_EN, enable);
}

int imx_control_camera_power_enable(imx_control_t *ic, u8 cam_index,  u8 enable){
  if (cam_index >= MAX_CAMERA_COUNT)
    return XST_FAILURE;
  imxc_enable_register_bit(ic, REG_CONTROL, CTRL_BIT_POWER_EN0 + cam_index, enable);
  return XST_SUCCESS;
}
void imx_control_camera_trigger_enable(imx_control_t *ic, u8 enable){ 
  imxc_enable_register_bit(ic, REG_CONTROL, CTRL_BIT_TRIGGER_EN, enable);
}

void imx_control_set_image_width(imx_control_t *ic, u16 width){
  imxc_write_register(ic, REG_FRAME_WIDTH, width);
}
void imx_control_set_image_height(imx_control_t *ic, u16 height){
  imxc_write_register(ic, REG_FRAME_HEIGHT, height);
}
void imx_control_set_pre_vsync_blank(imx_control_t *ic, u8 vblank){
  imxc_write_register(ic, REG_PRE_VERTICAL_BLANK, vblank);
}
void imx_control_set_pre_hsync_blank(imx_control_t *ic, u8 hblank){
  imxc_write_register(ic, REG_PRE_HORIZONTAL_BLANK, hblank);
}
void imx_control_set_post_vsync_blank(imx_control_t *ic, u8 vblank){
  imxc_write_register(ic, REG_POST_VERTICAL_BLANK, vblank);
}
void imx_control_set_post_hsync_blank(imx_control_t *ic, u8 hblank){
  imxc_write_register(ic, REG_POST_HORIZONTAL_BLANK, hblank);
}


int imx_control_set_tap_delay(imx_control_t *ic, u8 cam_index, u8 lane_index, u32 delay){
  u32 tap_address = ((((u32)cam_index) * ((u32)MAX_LANE_WIDTH)) + ((u32)lane_index));
  if (tap_address > (MAX_CAMERA_COUNT + MAX_LANE_WIDTH))
    return XST_FAILURE;
  tap_address = tap_address << 2;
  imxc_write_register(ic, REG_TAP_DELAY_START + tap_address, delay);
  imxc_enable_register_bit(ic, REG_CONTROL, CTRL_BIT_LOAD_TAP_EN, 1);
  imxc_enable_register_bit(ic, REG_CONTROL, CTRL_BIT_LOAD_TAP_EN, 0);
  return XST_SUCCESS;
}
u32 imx_control_get_tap_delay(imx_control_t *ic, u8 cam_index, u8 lane_index){
  u32 tap_address = (((u32) cam_index) * ((u32) MAX_LANE_WIDTH)) + ((u32) lane_index);
  if (tap_address > (MAX_CAMERA_COUNT + MAX_LANE_WIDTH))
    return 0xFFFFFFFF;
  tap_address = tap_address << 2;
  return imxc_read_register(ic, REG_TAP_DELAY_START + tap_address);
}

u32 imx_control_get_tap_error(imx_control_t *ic, u8 cam_index, u8 lane_index){
  u32 error_address = (((u32) cam_index) * ((u32) MAX_LANE_WIDTH)) + ((u32) lane_index);
  if (error_address > (MAX_CAMERA_COUNT * MAX_LANE_WIDTH))
    return 0xFFFFFFFF;
  error_address = error_address << 2;
  return imxc_read_register(ic, REG_TAP_ERROR_START + error_address);
}

//**************** PRIVATE FUNCTIONS *****************************************
u32 imxc_read_register(imx_control_t *ic, u32 address){
  return Xil_In32(ic->base_address + address);    
}
void imxc_write_register(imx_control_t *ic, u32 address, u32 value){
  Xil_Out32(ic->base_address + address, value);
}
void imxc_enable_register_bit(imx_control_t *ic, u32 address, u32 bit_index, u8 enable){
  if (enable)
    imxc_set_register_bit(ic, address, bit_index);
  else
    imxc_clear_register_bit(ic, address, bit_index);
}
void imxc_set_register_bit(imx_control_t *ic, u32 address, u32 bit_index){
  u32 value = imxc_read_register(ic, address);
  value |= 1 << bit_index;
  imxc_write_register(ic, address, value);
}
void imxc_clear_register_bit(imx_control_t *ic, u32 address, u32 bit_index){
  u32 value = imxc_read_register(ic, address);
  value &= ~(1 << bit_index);
  imxc_write_register(ic, address, value);
}
u32 imxc_is_register_bit_set(imx_control_t *ic, u32 address, u32 bit_index){
  u32 value = imxc_read_register(ic, address);
  return ((value & (1 << bit_index)) > 0);
}



