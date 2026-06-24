/********************************************************************************
 * ownership.c — PB10 SD-ownership line + claim/release state machine.
 * See ownership.h for the protocol description.
 *******************************************************************************/
#include "ownership.h"
#include "bridge_config.h"
#include "sd_storage.h"
#include "SW_UDISK.h"

#define PB10_PIN    GPIO_Pin_10

volatile UINT8 own_ch569_owns = 0;
volatile UINT8 own_card_ready = 0;

static volatile UINT8 own_pending = OWN_REQ_NONE;

void ownership_init(void)
{
    GPIOB_ResetBits(PB10_PIN);                       /* FPGA owns the card    */
    GPIOB_ModeCfg(PB10_PIN, GPIO_Slowascent_PP_8mA); /* push-pull output, low */
    own_ch569_owns = 0;
    own_card_ready = 0;
}

void ownership_request(UINT8 req)
{
    own_pending = req;
}

static void ownership_claim(void)
{
    UINT8 attempt;
    UINT8 ok = 0;

    PRINT("OWN: claiming SD card (PB10 high)\n");
    GPIOB_SetBits(PB10_PIN);
    own_ch569_owns = 1;
    PRINT("OWN: PB10 set\n");  while(!(R8_UART1_LSR & RB_LSR_TX_ALL_EMP));
    mDelaymS(PB10_SETTLE_MS);
    PRINT("OWN: settled\n");   while(!(R8_UART1_LSR & RB_LSR_TX_ALL_EMP));

    for(attempt = 0; attempt < SD_MOUNT_RETRIES; attempt++)
    {
        if(sd_storage_mount() == OP_SUCCESS)
        {
            ok = 1;
            break;
        }
        PRINT("OWN: SD mount attempt %d failed\n", attempt + 1);
        mDelaymS(50);
    }

    if(ok)
    {
        Udisk_Capability = TF_EMMCParam.EMMCSecNum;
        Udisk_Attention  = 1;          /* tell the host the medium (re)appeared */
        Udisk_Status    |= DEF_UDISK_EN_FLAG;
        own_card_ready   = 1;
        PRINT("OWN: card ready, %ld sectors (%ld MB)\n",
              TF_EMMCParam.EMMCSecNum, TF_EMMCParam.EMMCSecNum / 2048);
    }
    else
    {
        /* Card unusable: hand it straight back so the FPGA is not locked out. */
        PRINT("OWN: SD init failed, releasing PB10\n");
        GPIOB_ResetBits(PB10_PIN);
        own_ch569_owns = 0;
        own_card_ready = 0;
        Udisk_Status &= ~DEF_UDISK_EN_FLAG;
    }

    msc_read_selftest();

}

static void ownership_release(void)
{
    PRINT("OWN: releasing SD card (PB10 low)\n");
    Udisk_Status &= ~DEF_UDISK_EN_FLAG;   /* host sees "medium not present"   */
    own_card_ready = 0;
    sd_storage_unmount();
    GPIOB_ResetBits(PB10_PIN);
    own_ch569_owns = 0;
}

void ownership_poll(void)
{
    UINT8 req = own_pending;

    if(req == OWN_REQ_NONE)
        return;
    own_pending = OWN_REQ_NONE;

    switch(req)
    {
        case OWN_REQ_CLAIM:
            if(!own_ch569_owns)
                ownership_claim();
            break;
        case OWN_REQ_RELEASE:
            if(own_ch569_owns)
                ownership_release();
            break;
        case OWN_REQ_TOGGLE:
            if(own_ch569_owns)
                ownership_release();
            else
                ownership_claim();
            break;
        default:
            break;
    }
}
