#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/pm/device.h>

/* Define Sensors from Overlay */
static const struct gpio_dt_spec door1 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec door2 = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);

/* BTHome v2 Data Structure */
// ID 0xFCD2 (BTHome), Info(0x40), Door(0x1A), Count(1), State...
static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, 'D', 'o', 'o', 'r', 'S', 'e', 'n'),
    BT_DATA(BT_DATA_SVC_DATA16, ((uint8_t[]){
        0xD2, 0xFC,       // UUID 0xFCD2 (BTHome)
        0x40,             // Control Byte (No Encryption, BTHome v2)
        0x1A, 0x01, 0x00, // ID 0x1A (Door), Count 1, Placeholder Val
        0x1A, 0x02, 0x00  // ID 0x1A (Door), Count 2, Placeholder Val
    }), 8)
};

void main(void)
{
    // 1. Init GPIO
    gpio_pin_configure_dt(&door1, GPIO_INPUT);
    gpio_pin_configure_dt(&door2, GPIO_INPUT);

    // 2. Read States (0=Closed, 1=Open)
    int s1 = gpio_pin_get_dt(&door1);
    int s2 = gpio_pin_get_dt(&door2);

    // 3. Update Packet with actual values
    // Byte 6 is Door1 state, Byte 9 is Door2 state
    ((uint8_t*)ad[2].data)[5] = s1; 
    ((uint8_t*)ad[2].data)[8] = s2;

    // 4. Start Advertising (Broadcast for 3 seconds)
    bt_enable(NULL);
    bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0);
    k_sleep(K_SECONDS(3)); 
    bt_le_adv_stop();
    bt_disable();

    // 5. Setup Wake-on-Change Interrupts
    gpio_pin_interrupt_configure_dt(&door1, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&door2, GPIO_INT_EDGE_BOTH);

    // 6. Go to Deep Sleep (System OFF)
    // The nRF52 will basically shut down here until a pin changes.
    pm_state_force(0u, &(struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
    k_sleep(K_FOREVER);
}