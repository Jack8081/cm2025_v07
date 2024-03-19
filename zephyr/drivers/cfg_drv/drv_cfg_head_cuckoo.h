

#ifndef __DRV_CFG_HEAD_CUCKOO_H__
#define __DRV_CFG_HEAD_CUCKOO_H__


#define CFG_User_Version_Version                                                (0x01000020)    // id:  1, off:   0, size:  32

#define CFG_Platform_Case_IC_Type                                               (0x02000004)    // id:  2, off:   0, size:   4
#define CFG_Platform_Case_Board_Type                                            (0x02004001)    // id:  2, off:   4, size:   1
#define CFG_Platform_Case_Case_Name                                             (0x02005014)    // id:  2, off:   5, size:  20
#define CFG_Platform_Case_Major_Version                                         (0x02019001)    // id:  2, off:  25, size:   1
#define CFG_Platform_Case_Minor_Version                                         (0x0201a001)    // id:  2, off:  26, size:   1

#define CFG_Console_UART_TX_Pin                                                 (0x03000002)    // id:  3, off:   0, size:   2
#define CFG_Console_UART_RX_Pin                                                 (0x03002002)    // id:  3, off:   2, size:   2
#define CFG_Console_UART_Baudrate                                               (0x03004004)    // id:  3, off:   4, size:   4
#define CFG_Console_UART_Print_Time_Stamp                                       (0x03008001)    // id:  3, off:   8, size:   1

#define CFG_System_Settings_Support_Features                                    (0x04000002)    // id:  4, off:   0, size:   2
#define CFG_System_Settings_Auto_Power_Off_Mode                                 (0x04002001)    // id:  4, off:   2, size:   1
#define CFG_System_Settings_Auto_Power_Off_Time_Sec                             (0x04004002)    // id:  4, off:   4, size:   2
#define CFG_System_Settings_Auto_Standby_Time_Sec                               (0x04006002)    // id:  4, off:   6, size:   2
#define CFG_System_Settings_Enable_Voice_Prompt_In_Calling                      (0x04008001)    // id:  4, off:   8, size:   1
#define CFG_System_Settings_Default_Voice_Language                              (0x04009001)    // id:  4, off:   9, size:   1
#define CFG_System_Settings_Linein_Disable_Bluetooth                            (0x0400a001)    // id:  4, off:  10, size:   1

#define CFG_OTA_Settings_Enable_Dongle_OTA_Erase_VRAM                           (0x05000001)    // id:  5, off:   0, size:   1
#define CFG_OTA_Settings_Enable_APP_OTA_Erase_VRAM                              (0x05001001)    // id:  5, off:   1, size:   1
#define CFG_OTA_Settings_Enable_Single_OTA_Without_TWS                          (0x05002001)    // id:  5, off:   2, size:   1
#define CFG_OTA_Settings_Enable_Ver_Diff                                        (0x05003001)    // id:  5, off:   3, size:   1
#define CFG_OTA_Settings_Enable_Ver_Low                                         (0x05004001)    // id:  5, off:   4, size:   1
#define CFG_OTA_Settings_Enable_Poweroff                                        (0x05005001)    // id:  5, off:   5, size:   1
#define CFG_OTA_Settings_Version_Number                                         (0x0500600c)    // id:  5, off:   6, size:  12

#define CFG_Factory_Settings_Keep_User_VRAM_Data_When_UART_Upgrade              (0x06000001)    // id:  6, off:   0, size:   1
#define CFG_Factory_Settings_Keep_Factory_VRAM_Data_When_ATT_Upgrade            (0x06001001)    // id:  6, off:   1, size:   1

#define CFG_ONOFF_Key_Use_Inner_ONOFF_Key                                       (0x07000001)    // id:  7, off:   0, size:   1
#define CFG_ONOFF_Key_Continue_Key_Function_After_Wake_Up                       (0x07001001)    // id:  7, off:   1, size:   1
#define CFG_ONOFF_Key_Key_Value                                                 (0x07002001)    // id:  7, off:   2, size:   1
#define CFG_ONOFF_Key_Time_Press_Power_On                                       (0x07004002)    // id:  7, off:   4, size:   2
#define CFG_ONOFF_Key_Time_Long_Press_Reset                                     (0x07006001)    // id:  7, off:   6, size:   1
#define CFG_ONOFF_Key_Onoff_pullr_sel                                           (0x07007001)    // id:  7, off:   7, size:   1
#define CFG_ONOFF_Key_Boot_Hold_Key_Func                                        (0x07008001)    // id:  7, off:   8, size:   1
#define CFG_ONOFF_Key_Boot_Hold_Key_Time_Ms                                     (0x0700a002)    // id:  7, off:  10, size:   2
#define CFG_ONOFF_Key_Debounce_Time_Ms                                          (0x0700c002)    // id:  7, off:  12, size:   2
#define CFG_ONOFF_Key_Reboot_After_Boot_Hold_Key_Clear_Paired_List              (0x0700e001)    // id:  7, off:  14, size:   1

#define CFG_LRADC_Keys_Key                                                      (0x08000036)    // id:  8, off:   0, size:  54
#define CFG_LRADC_Keys_LRADC_Ctrl                                               (0x08038004)    // id:  8, off:  56, size:   4
#define CFG_LRADC_Keys_LRADC_Pull_Up                                            (0x0803c001)    // id:  8, off:  60, size:   1
#define CFG_LRADC_Keys_Use_LRADC_Key_Wake_Up                                    (0x0803d001)    // id:  8, off:  61, size:   1
#define CFG_LRADC_Keys_LRADC_Value_Test                                         (0x0803e001)    // id:  8, off:  62, size:   1
#define CFG_LRADC_Keys_Debounce_Time_Ms                                         (0x08040002)    // id:  8, off:  64, size:   2

#define CFG_GPIO_Keys_Key                                                       (0x09000018)    // id:  9, off:   0, size:  24

#define CFG_Tap_Key_Tap_Key_Control                                             (0x0a000014)    // id: 10, off:   0, size:  20

#define CFG_Key_Threshold_Single_Click_Valid_Ms                                 (0x0b000002)    // id: 11, off:   0, size:   2
#define CFG_Key_Threshold_Multi_Click_Interval_Ms                               (0x0b002002)    // id: 11, off:   2, size:   2
#define CFG_Key_Threshold_Repeat_Start_Delay_Ms                                 (0x0b004002)    // id: 11, off:   4, size:   2
#define CFG_Key_Threshold_Repeat_Interval_Ms                                    (0x0b006002)    // id: 11, off:   6, size:   2
#define CFG_Key_Threshold_Long_Press_Time_Ms                                    (0x0b008002)    // id: 11, off:   8, size:   2
#define CFG_Key_Threshold_Long_Long_Press_Time_Ms                               (0x0b00a002)    // id: 11, off:  10, size:   2
#define CFG_Key_Threshold_Very_Long_Press_Time_Ms                               (0x0b00c002)    // id: 11, off:  12, size:   2

#define CFG_Key_Func_Maps_Map                                                   (0x0c0000f0)    // id: 12, off:   0, size: 240

#define CFG_Combo_Key_Func_Maps_Map                                             (0x0d000030)    // id: 13, off:   0, size:  48

#define CFG_Customed_Key_Sequence_Customed_Key_Sequence                         (0x0e00000c)    // id: 14, off:   0, size:  12

#define CFG_LED_Drives_LED                                                      (0x0f00000c)    // id: 15, off:   0, size:  12

#define CFG_LED_Display_Models_Model                                            (0x100001e0)    // id: 16, off:   0, size: 480

#define CFG_BT_Music_Volume_Table_Level                                         (0x11000022)    // id: 17, off:   0, size:  34

#define CFG_BT_Call_Volume_Table_Level                                          (0x12000020)    // id: 18, off:   0, size:  32

#define CFG_Voice_Volume_Table_Level                                            (0x14000022)    // id: 20, off:   0, size:  34

#define CFG_Volume_Settings_Voice_Default_Volume                                (0x15000001)    // id: 21, off:   0, size:   1
#define CFG_Volume_Settings_Voice_Min_Volume                                    (0x15001001)    // id: 21, off:   1, size:   1
#define CFG_Volume_Settings_Voice_Max_Volume                                    (0x15002001)    // id: 21, off:   2, size:   1
#define CFG_Volume_Settings_BT_Music_Default_Volume                             (0x15003001)    // id: 21, off:   3, size:   1
#define CFG_Volume_Settings_BT_Call_Default_Volume                              (0x15004001)    // id: 21, off:   4, size:   1
#define CFG_Volume_Settings_BT_Music_Default_Vol_Ex                             (0x15005001)    // id: 21, off:   5, size:   1
#define CFG_Volume_Settings_Tone_Volume_Decay                                   (0x15006002)    // id: 21, off:   6, size:   2

#define CFG_Audio_Settings_Audio_Out_Mode                                       (0x16000001)    // id: 22, off:   0, size:   1
#define CFG_Audio_Settings_I2STX_Select_GPIO                                    (0x16002008)    // id: 22, off:   2, size:   8
#define CFG_Audio_Settings_Channel_Select_Mode                                  (0x16012001)    // id: 22, off:  18, size:   1
#define CFG_Audio_Settings_Channel_Select_GPIO                                  (0x16013003)    // id: 22, off:  19, size:   3
#define CFG_Audio_Settings_Channel_Select_LRADC                                 (0x1601800c)    // id: 22, off:  24, size:  12
#define CFG_Audio_Settings_TWS_Alone_Audio_Channel                              (0x16024001)    // id: 22, off:  36, size:   1
#define CFG_Audio_Settings_L_Speaker_Out                                        (0x16025001)    // id: 22, off:  37, size:   1
#define CFG_Audio_Settings_R_Speaker_Out                                        (0x16026001)    // id: 22, off:  38, size:   1
#define CFG_Audio_Settings_ADC_Bias_Setting                                     (0x16028004)    // id: 22, off:  40, size:   4
#define CFG_Audio_Settings_DAC_Bias_Setting                                     (0x1602c004)    // id: 22, off:  44, size:   4
#define CFG_Audio_Settings_Keep_DA_Enabled_When_Play_Pause                      (0x16030001)    // id: 22, off:  48, size:   1
#define CFG_Audio_Settings_Disable_PA_When_Reconnect                            (0x16031001)    // id: 22, off:  49, size:   1
#define CFG_Audio_Settings_Extern_PA_Control                                    (0x16032008)    // id: 22, off:  50, size:   8
#define CFG_Audio_Settings_AntiPOP_Process_Disable                              (0x1603a001)    // id: 22, off:  58, size:   1
#define CFG_Audio_Settings_Pa_Gain                                              (0x1603b001)    // id: 22, off:  59, size:   1
#define CFG_Audio_Settings_DMIC01_Channel_Aligning                              (0x1603c001)    // id: 22, off:  60, size:   1
#define CFG_Audio_Settings_DMIC_Select_GPIO                                     (0x1603e004)    // id: 22, off:  62, size:   4
#define CFG_Audio_Settings_main_mic                                             (0x16042001)    // id: 22, off:  66, size:   1
#define CFG_Audio_Settings_sub_mic                                              (0x16043001)    // id: 22, off:  67, size:   1
#define CFG_Audio_Settings_adc0_input                                           (0x16044001)    // id: 22, off:  68, size:   1
#define CFG_Audio_Settings_adc1_input                                           (0x16045001)    // id: 22, off:  69, size:   1
#define CFG_Audio_Settings_ADC0_SEL_VMIC                                        (0x16046001)    // id: 22, off:  70, size:   1
#define CFG_Audio_Settings_ADC1_SEL_VMIC                                        (0x16047001)    // id: 22, off:  71, size:   1
#define CFG_Audio_Settings_Hw_Aec_Select                                        (0x16048001)    // id: 22, off:  72, size:   1

#define CFG_Tone_List_Tone                                                      (0x17000064)    // id: 23, off:   0, size: 100
#define CFG_Tone_List_Tone_Format_Name                                          (0x17064005)    // id: 23, off: 100, size:   5

#define CFG_Key_Tone_Key_Tone_Select                                            (0x18000001)    // id: 24, off:   0, size:   1
#define CFG_Key_Tone_Long_Key_Tone_Select                                       (0x18001001)    // id: 24, off:   1, size:   1
#define CFG_Key_Tone_Long_Long_Key_Tone_Select                                  (0x18002001)    // id: 24, off:   2, size:   1
#define CFG_Key_Tone_Very_Long_Key_Tone_Select                                  (0x18003001)    // id: 24, off:   3, size:   1

#define CFG_Voice_List_Voice                                                    (0x19000118)    // id: 25, off:   0, size: 280
#define CFG_Voice_List_Voice_Format_Name                                        (0x19118005)    // id: 25, off: 280, size:   5

#define CFG_Numeric_Voice_List_Voice                                            (0x1a000064)    // id: 26, off:   0, size: 100

#define CFG_Event_Notify_Notify                                                 (0x1b00010e)    // id: 27, off:   0, size: 270

#define CFG_Battery_Charge_Select_Charge_Mode                                   (0x1c000001)    // id: 28, off:   0, size:   1
#define CFG_Battery_Charge_Charge_Current                                       (0x1c001001)    // id: 28, off:   1, size:   1
#define CFG_Battery_Charge_Charge_Voltage                                       (0x1c002001)    // id: 28, off:   2, size:   1
#define CFG_Battery_Charge_Charge_Stop_Mode                                     (0x1c003001)    // id: 28, off:   3, size:   1
#define CFG_Battery_Charge_Charge_Stop_Voltage                                  (0x1c004002)    // id: 28, off:   4, size:   2
#define CFG_Battery_Charge_Charge_Stop_Current                                  (0x1c006001)    // id: 28, off:   6, size:   1
#define CFG_Battery_Charge_Init_Charge_Enable                                   (0x1c007001)    // id: 28, off:   7, size:   1
#define CFG_Battery_Charge_Init_Charge_Current_1                                (0x1c008001)    // id: 28, off:   8, size:   1
#define CFG_Battery_Charge_Init_Charge_Vol_1                                    (0x1c00a002)    // id: 28, off:  10, size:   2
#define CFG_Battery_Charge_Init_Charge_Current_2                                (0x1c00c001)    // id: 28, off:  12, size:   1
#define CFG_Battery_Charge_Init_Charge_Vol_2                                    (0x1c00e002)    // id: 28, off:  14, size:   2
#define CFG_Battery_Charge_Init_Charge_Current_3                                (0x1c010001)    // id: 28, off:  16, size:   1
#define CFG_Battery_Charge_Init_Charge_Vol_3                                    (0x1c012002)    // id: 28, off:  18, size:   2
#define CFG_Battery_Charge_Precharge_Enable                                     (0x1c014001)    // id: 28, off:  20, size:   1
#define CFG_Battery_Charge_Precharge_Stop_Voltage                               (0x1c016002)    // id: 28, off:  22, size:   2
#define CFG_Battery_Charge_Precharge_Current                                    (0x1c018001)    // id: 28, off:  24, size:   1
#define CFG_Battery_Charge_Precharge_Current_Min_Limit                          (0x1c019001)    // id: 28, off:  25, size:   1
#define CFG_Battery_Charge_Fast_Charge_Enable                                   (0x1c01a001)    // id: 28, off:  26, size:   1
#define CFG_Battery_Charge_Fast_Charge_Current                                  (0x1c01b001)    // id: 28, off:  27, size:   1
#define CFG_Battery_Charge_Fast_Charge_Voltage_Threshold                        (0x1c01c002)    // id: 28, off:  28, size:   2
#define CFG_Battery_Charge_Enable_Battery_Recharge                              (0x1c01e001)    // id: 28, off:  30, size:   1
#define CFG_Battery_Charge_Recharge_Threshold_Low_Temperature                   (0x1c020002)    // id: 28, off:  32, size:   2
#define CFG_Battery_Charge_Recharge_Threshold_Normal_Temperature                (0x1c022002)    // id: 28, off:  34, size:   2
#define CFG_Battery_Charge_Recharge_Threshold_High_Temperature                  (0x1c024002)    // id: 28, off:  36, size:   2
#define CFG_Battery_Charge_Bat_OVP_Enable                                       (0x1c026001)    // id: 28, off:  38, size:   1
#define CFG_Battery_Charge_OVP_Voltage_Low_Temperature                          (0x1c028002)    // id: 28, off:  40, size:   2
#define CFG_Battery_Charge_OVP_Voltage_Normal_Temperature                       (0x1c02a002)    // id: 28, off:  42, size:   2
#define CFG_Battery_Charge_OVP_Voltage_High_Temperature                         (0x1c02c002)    // id: 28, off:  44, size:   2
#define CFG_Battery_Charge_Charge_Total_Time_Limit                              (0x1c02e002)    // id: 28, off:  46, size:   2
#define CFG_Battery_Charge_Battery_Check_Period_Sec                             (0x1c030002)    // id: 28, off:  48, size:   2
#define CFG_Battery_Charge_Charge_Check_Period_Sec                              (0x1c032002)    // id: 28, off:  50, size:   2
#define CFG_Battery_Charge_Charge_Full_Continue_Sec                             (0x1c034002)    // id: 28, off:  52, size:   2
#define CFG_Battery_Charge_Front_Charge_Full_Power_Off_Wait_Sec                 (0x1c036002)    // id: 28, off:  54, size:   2
#define CFG_Battery_Charge_DC5V_Detect_Debounce_Time_Ms                         (0x1c038002)    // id: 28, off:  56, size:   2
#define CFG_Battery_Charge_Battery_Default_RDrop                                (0x1c03a002)    // id: 28, off:  58, size:   2

#define CFG_Charger_Box_Enable_Charger_Box                                      (0x1d000001)    // id: 29, off:   0, size:   1
#define CFG_Charger_Box_DC5V_Pull_Down_Current                                  (0x1d001001)    // id: 29, off:   1, size:   1
#define CFG_Charger_Box_DC5V_Pull_Down_Hold_Ms                                  (0x1d002002)    // id: 29, off:   2, size:   2
#define CFG_Charger_Box_Charger_Standby_Delay_Ms                                (0x1d004002)    // id: 29, off:   4, size:   2
#define CFG_Charger_Box_Charger_Standby_Voltage                                 (0x1d006002)    // id: 29, off:   6, size:   2
#define CFG_Charger_Box_Charger_Wake_Delay_Ms                                   (0x1d008002)    // id: 29, off:   8, size:   2
#define CFG_Charger_Box_Charger_Box_Standby_Current                             (0x1d00a001)    // id: 29, off:  10, size:   1
#define CFG_Charger_Box_DC5V_UART_Comm_Settings                                 (0x1d00c008)    // id: 29, off:  12, size:   8
#define CFG_Charger_Box_DC5V_IO_Comm_Settings                                   (0x1d014002)    // id: 29, off:  20, size:   2

#define CFG_Battery_Level_Level                                                 (0x1e000014)    // id: 30, off:   0, size:  20

#define CFG_Battery_Low_Battery_Too_Low_Voltage                                 (0x1f000002)    // id: 31, off:   0, size:   2
#define CFG_Battery_Low_Battery_Low_Voltage                                     (0x1f002002)    // id: 31, off:   2, size:   2
#define CFG_Battery_Low_Battery_Low_Voltage_Ex                                  (0x1f004002)    // id: 31, off:   4, size:   2
#define CFG_Battery_Low_Battery_Low_Prompt_Interval_Sec                         (0x1f006002)    // id: 31, off:   6, size:   2

#define CFG_NTC_Settings_NTC_Settings                                           (0x2000000c)    // id: 32, off:   0, size:  12
#define CFG_NTC_Settings_NTC_Ranges                                             (0x2000c028)    // id: 32, off:  12, size:  40

#define CFG_BT_Device_BT_Device_Name                                            (0x2100001e)    // id: 33, off:   0, size:  30
#define CFG_BT_Device_Left_Device_Suffix                                        (0x2101e00a)    // id: 33, off:  30, size:  10
#define CFG_BT_Device_Right_Device_Suffix                                       (0x2102800a)    // id: 33, off:  40, size:  10
#define CFG_BT_Device_BT_Address                                                (0x21032006)    // id: 33, off:  50, size:   6
#define CFG_BT_Device_Use_Random_BT_Address                                     (0x21038001)    // id: 33, off:  56, size:   1
#define CFG_BT_Device_BT_Device_Class                                           (0x2103c004)    // id: 33, off:  60, size:   4
#define CFG_BT_Device_PIN_Code                                                  (0x21040006)    // id: 33, off:  64, size:   6
#define CFG_BT_Device_Default_HOSC_Capacity                                     (0x21046001)    // id: 33, off:  70, size:   1
#define CFG_BT_Device_Force_Default_HOSC_Capacity                               (0x21047001)    // id: 33, off:  71, size:   1
#define CFG_BT_Device_BT_Max_RF_TX_Power                                        (0x21048001)    // id: 33, off:  72, size:   1
#define CFG_BT_Device_BLE_RF_TX_Power                                           (0x21049001)    // id: 33, off:  73, size:   1
#define CFG_BT_Device_A2DP_Bitpool                                              (0x2104a001)    // id: 33, off:  74, size:   1
#define CFG_BT_Device_Vendor_ID                                                 (0x2104c002)    // id: 33, off:  76, size:   2
#define CFG_BT_Device_Product_ID                                                (0x2104e002)    // id: 33, off:  78, size:   2
#define CFG_BT_Device_Version_ID                                                (0x21050002)    // id: 33, off:  80, size:   2

#define CFG_BT_RF_Param_Table_Param_Table                                       (0x22000010)    // id: 34, off:   0, size:  16

#define CFG_BT_Manager_Support_Features                                         (0x23000004)    // id: 35, off:   0, size:   4
#define CFG_BT_Manager_Support_Device_Number                                    (0x23004001)    // id: 35, off:   4, size:   1
#define CFG_BT_Manager_Paired_Device_Save_Number                                (0x23005001)    // id: 35, off:   5, size:   1
#define CFG_BT_Manager_Controller_Test_Mode                                     (0x23006001)    // id: 35, off:   6, size:   1
#define CFG_BT_Manager_Enter_BQB_Test_Mode_By_Key                               (0x23007001)    // id: 35, off:   7, size:   1
#define CFG_BT_Manager_Auto_Quit_BT_Ctrl_Test                                   (0x23008004)    // id: 35, off:   8, size:   4
#define CFG_BT_Manager_Idle_Enter_Sniff_Time_Ms                                 (0x2300c002)    // id: 35, off:  12, size:   2
#define CFG_BT_Manager_Sniff_Interval_Ms                                        (0x2300e002)    // id: 35, off:  14, size:   2

#define CFG_BT_Pair_Default_State_Discoverable                                  (0x24000001)    // id: 36, off:   0, size:   1
#define CFG_BT_Pair_Default_State_Wait_Connect_Sec                              (0x24002002)    // id: 36, off:   2, size:   2
#define CFG_BT_Pair_Pair_Mode_Duration_Sec                                      (0x24004002)    // id: 36, off:   4, size:   2
#define CFG_BT_Pair_Disconnect_All_Phones_When_Enter_Pair_Mode                  (0x24006001)    // id: 36, off:   6, size:   1
#define CFG_BT_Pair_Clear_Paired_List_When_Enter_Pair_Mode                      (0x24007001)    // id: 36, off:   7, size:   1
#define CFG_BT_Pair_Clear_TWS_When_Key_Clear_Paired_List                        (0x24008001)    // id: 36, off:   8, size:   1
#define CFG_BT_Pair_Enter_Pair_Mode_When_Key_Clear_Paired_List                  (0x24009001)    // id: 36, off:   9, size:   1
#define CFG_BT_Pair_Enter_Pair_Mode_When_Paired_List_Empty                      (0x2400a001)    // id: 36, off:  10, size:   1
#define CFG_BT_Pair_BT_Not_Discoverable_When_Connected                          (0x2400b001)    // id: 36, off:  11, size:   1

#define CFG_TWS_Pair_TWS_Pair_Key_Mode                                          (0x25000001)    // id: 37, off:   0, size:   1
#define CFG_TWS_Pair_Match_Mode                                                 (0x25001001)    // id: 37, off:   1, size:   1
#define CFG_TWS_Pair_Match_Name_Length                                          (0x25002001)    // id: 37, off:   2, size:   1
#define CFG_TWS_Pair_TWS_Wait_Pair_Search_Time_Sec                              (0x25004002)    // id: 37, off:   4, size:   2
#define CFG_TWS_Pair_TWS_Power_On_Auto_Pair_Search                              (0x25006001)    // id: 37, off:   6, size:   1

#define CFG_TWS_Advanced_Pair_Enable_TWS_Advanced_Pair_Mode                     (0x26000001)    // id: 38, off:   0, size:   1
#define CFG_TWS_Advanced_Pair_Check_RSSI_When_TWS_Pair_Search                   (0x26001001)    // id: 38, off:   1, size:   1
#define CFG_TWS_Advanced_Pair_RSSI_Threshold                                    (0x26002001)    // id: 38, off:   2, size:   1
#define CFG_TWS_Advanced_Pair_Use_Search_Mode_When_TWS_Reconnect                (0x26003001)    // id: 38, off:   3, size:   1

#define CFG_TWS_Sync_Sync_Mode                                                  (0x27000001)    // id: 39, off:   0, size:   1

#define CFG_BT_Auto_Reconnect_Enable_Auto_Reconnect                             (0x28000001)    // id: 40, off:   0, size:   1
#define CFG_BT_Auto_Reconnect_Reconnect_Phone_Timeout                           (0x28002002)    // id: 40, off:   2, size:   2
#define CFG_BT_Auto_Reconnect_Reconnect_Phone_Interval                          (0x28004002)    // id: 40, off:   4, size:   2
#define CFG_BT_Auto_Reconnect_Reconnect_Phone_Times_By_Startup                  (0x28006001)    // id: 40, off:   6, size:   1
#define CFG_BT_Auto_Reconnect_Reconnect_TWS_Timeout                             (0x28008002)    // id: 40, off:   8, size:   2
#define CFG_BT_Auto_Reconnect_Reconnect_TWS_Interval                            (0x2800a002)    // id: 40, off:  10, size:   2
#define CFG_BT_Auto_Reconnect_Reconnect_TWS_Times_By_Startup                    (0x2800c001)    // id: 40, off:  12, size:   1
#define CFG_BT_Auto_Reconnect_Reconnect_Times_By_Timeout                        (0x2800d001)    // id: 40, off:  13, size:   1
#define CFG_BT_Auto_Reconnect_Enter_Pair_Mode_When_Startup_Reconnect_Fail       (0x2800e001)    // id: 40, off:  14, size:   1

#define CFG_BT_HID_Settings_HID_Auto_Disconnect_Delay_Sec                       (0x29000002)    // id: 41, off:   0, size:   2
#define CFG_BT_HID_Settings_HID_Connect_Operation_Delay_Ms                      (0x29002002)    // id: 41, off:   2, size:   2
#define CFG_BT_HID_Settings_HID_Custom_Key_Type                                 (0x29004001)    // id: 41, off:   4, size:   1
#define CFG_BT_HID_Settings_HID_Custom_Key_Value                                (0x29005001)    // id: 41, off:   5, size:   1

#define CFG_Low_Latency_Settings_LLM_Basic                                      (0x2a00000a)    // id: 42, off:   0, size:  10
#define CFG_Low_Latency_Settings_BTMusic_Use_ALLM                               (0x2a00a001)    // id: 42, off:  10, size:   1
#define CFG_Low_Latency_Settings_ALLM_Factor                                    (0x2a00b001)    // id: 42, off:  11, size:   1
#define CFG_Low_Latency_Settings_ALLM_Upper                                     (0x2a00c001)    // id: 42, off:  12, size:   1
#define CFG_Low_Latency_Settings_BTMusic_PLC_Mode                               (0x2a00d001)    // id: 42, off:  13, size:   1
#define CFG_Low_Latency_Settings_AAC_Decode_Delay_Optimize                      (0x2a00e001)    // id: 42, off:  14, size:   1
#define CFG_Low_Latency_Settings_BTCall_Latency_Optimization                    (0x2a00f001)    // id: 42, off:  15, size:   1
#define CFG_Low_Latency_Settings_BTCall_Use_Limiter_Not_AGC                     (0x2a010001)    // id: 42, off:  16, size:   1
#define CFG_Low_Latency_Settings_BTCall_PLC_NoDelay                             (0x2a011001)    // id: 42, off:  17, size:   1
#define CFG_Low_Latency_Settings_Reserved                                       (0x2a012001)    // id: 42, off:  18, size:   1

#define CFG_BTMusic_Multi_Dae_Settings_Enable                                   (0x2b000001)    // id: 43, off:   0, size:   1
#define CFG_BTMusic_Multi_Dae_Settings_Cur_Dae_Num                              (0x2b001001)    // id: 43, off:   1, size:   1
#define CFG_BTMusic_Multi_Dae_Settings_Dae_Index                                (0x2b002001)    // id: 43, off:   2, size:   1

#define CFG_BT_Music_Volume_Sync_Volume_Sync_Only_When_Playing                  (0x2c000001)    // id: 44, off:   0, size:   1
#define CFG_BT_Music_Volume_Sync_Origin_Volume_Sync_To_Remote                   (0x2c001001)    // id: 44, off:   1, size:   1
#define CFG_BT_Music_Volume_Sync_Origin_Volume_Sync_Delay_Ms                    (0x2c002002)    // id: 44, off:   2, size:   2
#define CFG_BT_Music_Volume_Sync_Playing_Volume_Sync_Delay_Ms                   (0x2c004002)    // id: 44, off:   4, size:   2

#define CFG_BT_Music_Stop_Hold_Key_Pause_Stop_Hold_Ms                           (0x2d000002)    // id: 45, off:   0, size:   2
#define CFG_BT_Music_Stop_Hold_Key_Prev_Next_Hold_Ms                            (0x2d002002)    // id: 45, off:   2, size:   2

#define CFG_BT_Two_Device_Play_Stop_Another_When_One_Playing                    (0x2e000001)    // id: 46, off:   0, size:   1
#define CFG_BT_Two_Device_Play_Resume_Another_When_One_Stopped                  (0x2e001001)    // id: 46, off:   1, size:   1
#define CFG_BT_Two_Device_Play_A2DP_Status_Stopped_Delay_Ms                     (0x2e002002)    // id: 46, off:   2, size:   2

#define CFG_BT_Call_Volume_Sync_Origin_Volume_Sync_To_Remote                    (0x2f000001)    // id: 47, off:   0, size:   1
#define CFG_BT_Call_Volume_Sync_Origin_Volume_Sync_Delay_Ms                     (0x2f002002)    // id: 47, off:   2, size:   2

#define CFG_Incoming_Call_Prompt_Prompt_Interval_Ms                             (0x30000002)    // id: 48, off:   0, size:   2
#define CFG_Incoming_Call_Prompt_Play_Phone_Number                              (0x30002001)    // id: 48, off:   2, size:   1
#define CFG_Incoming_Call_Prompt_BT_Call_Ring_Mode                              (0x30003001)    // id: 48, off:   3, size:   1

#define CFG_Cap_Temp_Comp_Enable_Cap_Temp_Comp                                  (0x31000001)    // id: 49, off:   0, size:   1
#define CFG_Cap_Temp_Comp_Table                                                 (0x31001028)    // id: 49, off:   1, size:  40

#define CFG_BT_Music_DAE_Enable_DAE                                             (0x33000001)    // id: 51, off:   0, size:   1
#define CFG_BT_Music_DAE_Test_Volume                                            (0x33001001)    // id: 51, off:   1, size:   1

#define CFG_BT_Call_Out_DAE_Enable_DAE                                          (0x34000001)    // id: 52, off:   0, size:   1
#define CFG_BT_Call_Out_DAE_Test_Volume                                         (0x34001001)    // id: 52, off:   1, size:   1

#define CFG_BT_Call_MIC_DAE_Enable_DAE                                          (0x35000001)    // id: 53, off:   0, size:   1
#define CFG_BT_Call_MIC_DAE_Test_Volume                                         (0x35001001)    // id: 53, off:   1, size:   1

#define CFG_BT_Call_Quality_MIC_Gain                                            (0x37000008)    // id: 55, off:   0, size:   8
#define CFG_BT_Call_Quality_Test_Volume                                         (0x37008001)    // id: 55, off:   8, size:   1

#define CFG_Voice_Player_Param_VP_Develop_Value1                                (0x38000004)    // id: 56, off:   0, size:   4
#define CFG_Voice_Player_Param_VP_WaitData_Time                                 (0x38004001)    // id: 56, off:   4, size:   1
#define CFG_Voice_Player_Param_VP_WaitData_Empty_Time                           (0x38005001)    // id: 56, off:   5, size:   1
#define CFG_Voice_Player_Param_VP_Max_Decode_Count                              (0x38006001)    // id: 56, off:   6, size:   1
#define CFG_Voice_Player_Param_VP_Max_PCMBUF_Sampels                            (0x38008002)    // id: 56, off:   8, size:   2
#define CFG_Voice_Player_Param_VP_Het_PCMBUF_Sampels                            (0x3800a002)    // id: 56, off:  10, size:   2
#define CFG_Voice_Player_Param_VP_Hft_PCMBUF_Sampels                            (0x3800c002)    // id: 56, off:  12, size:   2
#define CFG_Voice_Player_Param_VP_Work_Frequency                                (0x3800e001)    // id: 56, off:  14, size:   1
#define CFG_Voice_Player_Param_VP_Module_Frequency                              (0x3800f001)    // id: 56, off:  15, size:   1

#define CFG_Voice_User_Settings_VP_StartPlay_Threshold                          (0x39000002)    // id: 57, off:   0, size:   2

#define CFG_Tone_Player_Param_WT_Develop_Value1                                 (0x3a000004)    // id: 58, off:   0, size:   4
#define CFG_Tone_Player_Param_WT_WaitData_Time                                  (0x3a004001)    // id: 58, off:   4, size:   1
#define CFG_Tone_Player_Param_WT_WaitData_Empty_Time                            (0x3a005001)    // id: 58, off:   5, size:   1
#define CFG_Tone_Player_Param_WT_Max_Decode_Count                               (0x3a006001)    // id: 58, off:   6, size:   1
#define CFG_Tone_Player_Param_WT_Max_PCMBUF_Sampels                             (0x3a008002)    // id: 58, off:   8, size:   2
#define CFG_Tone_Player_Param_WT_Het_PCMBUF_Sampels                             (0x3a00a002)    // id: 58, off:  10, size:   2
#define CFG_Tone_Player_Param_WT_Hft_PCMBUF_Sampels                             (0x3a00c002)    // id: 58, off:  12, size:   2
#define CFG_Tone_Player_Param_WT_Work_Frequency                                 (0x3a00e001)    // id: 58, off:  14, size:   1
#define CFG_Tone_Player_Param_WT_Module_Frequency                               (0x3a00f001)    // id: 58, off:  15, size:   1

#define CFG_Tone_User_Settings_WT_StartPlay_Threshold                           (0x3b000002)    // id: 59, off:   0, size:   2

#define CFG_BTMusic_Player_Param_BM_Develop_Value1                              (0x3e000004)    // id: 62, off:   0, size:   4
#define CFG_BTMusic_Player_Param_BM_WaitData_Time                               (0x3e004001)    // id: 62, off:   4, size:   1
#define CFG_BTMusic_Player_Param_BM_WaitData_Empty_Time                         (0x3e005001)    // id: 62, off:   5, size:   1
#define CFG_BTMusic_Player_Param_BM_Freq_TWS_Decrement                          (0x3e006001)    // id: 62, off:   6, size:   1
#define CFG_BTMusic_Player_Param_BM_SBC_Max_Decode_Count                        (0x3e007001)    // id: 62, off:   7, size:   1
#define CFG_BTMusic_Player_Param_BM_AAC_Max_Decode_Count                        (0x3e008001)    // id: 62, off:   8, size:   1
#define CFG_BTMusic_Player_Param_BM_Max_PCMBUF_Time                             (0x3e009001)    // id: 62, off:   9, size:   1
#define CFG_BTMusic_Player_Param_BM_SBC_Max_Sleep_Time                          (0x3e00a002)    // id: 62, off:  10, size:   2
#define CFG_BTMusic_Player_Param_BM_AAC_Max_Sleep_Time                          (0x3e00c002)    // id: 62, off:  12, size:   2
#define CFG_BTMusic_Player_Param_BM_TWS_WPlay_Mintime                           (0x3e00e002)    // id: 62, off:  14, size:   2
#define CFG_BTMusic_Player_Param_BM_TWS_WPlay_Maxtime                           (0x3e010002)    // id: 62, off:  16, size:   2
#define CFG_BTMusic_Player_Param_BM_TWS_WStop_Mintime                           (0x3e012002)    // id: 62, off:  18, size:   2
#define CFG_BTMusic_Player_Param_BM_TWS_WStop_Maxtime                           (0x3e014002)    // id: 62, off:  20, size:   2
#define CFG_BTMusic_Player_Param_BM_TWS_Sync_interval                           (0x3e016002)    // id: 62, off:  22, size:   2
#define CFG_BTMusic_Player_Param_BM_Het_PCMBUF_Sampels                          (0x3e018002)    // id: 62, off:  24, size:   2
#define CFG_BTMusic_Player_Param_BM_Hft_PCMBUF_Sampels                          (0x3e01a002)    // id: 62, off:  26, size:   2
#define CFG_BTMusic_Player_Param_BM_StartPlay_Normal                            (0x3e01c002)    // id: 62, off:  28, size:   2
#define CFG_BTMusic_Player_Param_BM_StartPlay_TWS                               (0x3e01e002)    // id: 62, off:  30, size:   2
#define CFG_BTMusic_Player_Param_BM_Work_Frequency_AAC                          (0x3e020001)    // id: 62, off:  32, size:   1
#define CFG_BTMusic_Player_Param_BM_Module_Frequency_AAC                        (0x3e021001)    // id: 62, off:  33, size:   1
#define CFG_BTMusic_Player_Param_BM_Work_Frequency_SBC                          (0x3e022001)    // id: 62, off:  34, size:   1
#define CFG_BTMusic_Player_Param_BM_Module_Frequency_SBC                        (0x3e023001)    // id: 62, off:  35, size:   1

#define CFG_BTMusic_User_Settings_BM_DataWidth                                  (0x3f000001)    // id: 63, off:   0, size:   1
#define CFG_BTMusic_User_Settings_BM_ISpeech_PEQ_Enable                         (0x3f001001)    // id: 63, off:   1, size:   1
#define CFG_BTMusic_User_Settings_BM_Fadein_Continue_Time                       (0x3f002002)    // id: 63, off:   2, size:   2
#define CFG_BTMusic_User_Settings_BM_Fadeout_Continue_Time                      (0x3f004002)    // id: 63, off:   4, size:   2
#define CFG_BTMusic_User_Settings_BM_SBC_Playing_CacheData                      (0x3f006002)    // id: 63, off:   6, size:   2
#define CFG_BTMusic_User_Settings_BM_AAC_Playing_CacheData                      (0x3f008002)    // id: 63, off:   8, size:   2

#define CFG_BTSpeech_Player_Param_BS_Develop_Value1                             (0x40000004)    // id: 64, off:   0, size:   4
#define CFG_BTSpeech_Player_Param_BS_WaitData_Time                              (0x40004001)    // id: 64, off:   4, size:   1
#define CFG_BTSpeech_Player_Param_BS_WaitData_Empty_Time                        (0x40005001)    // id: 64, off:   5, size:   1
#define CFG_BTSpeech_Player_Param_BS_Max_Decode_Count                           (0x40006001)    // id: 64, off:   6, size:   1
#define CFG_BTSpeech_Player_Param_BS_Max_PCMBUF_Time                            (0x40007001)    // id: 64, off:   7, size:   1
#define CFG_BTSpeech_Player_Param_BS_CVSD_Max_Sleep_Time                        (0x40008002)    // id: 64, off:   8, size:   2
#define CFG_BTSpeech_Player_Param_BS_MSBC_Max_Sleep_Time                        (0x4000a002)    // id: 64, off:  10, size:   2
#define CFG_BTSpeech_Player_Param_BS_TWS_WPlay_Mintime                          (0x4000c002)    // id: 64, off:  12, size:   2
#define CFG_BTSpeech_Player_Param_BS_TWS_WPlay_Maxtime                          (0x4000e002)    // id: 64, off:  14, size:   2
#define CFG_BTSpeech_Player_Param_BS_TWS_WStop_Mintime                          (0x40010002)    // id: 64, off:  16, size:   2
#define CFG_BTSpeech_Player_Param_BS_TWS_WStop_Maxtime                          (0x40012002)    // id: 64, off:  18, size:   2
#define CFG_BTSpeech_Player_Param_BS_TWS_Sync_interval                          (0x40014002)    // id: 64, off:  20, size:   2
#define CFG_BTSpeech_Player_Param_BS_Het_PCMBUF_Sampels                         (0x40016002)    // id: 64, off:  22, size:   2
#define CFG_BTSpeech_Player_Param_BS_Hft_PCMBUF_Sampels                         (0x40018002)    // id: 64, off:  24, size:   2
#define CFG_BTSpeech_Player_Param_BS_StartPlay_Normal                           (0x4001a002)    // id: 64, off:  26, size:   2
#define CFG_BTSpeech_Player_Param_BS_StartPlay_TWS                              (0x4001c002)    // id: 64, off:  28, size:   2
#define CFG_BTSpeech_Player_Param_BS_Work_Frequency_MSBC                        (0x4001e001)    // id: 64, off:  30, size:   1
#define CFG_BTSpeech_Player_Param_BS_Module_Frequency_MSBC                      (0x4001f001)    // id: 64, off:  31, size:   1
#define CFG_BTSpeech_Player_Param_BS_Work_Frequency_CVSD                        (0x40020001)    // id: 64, off:  32, size:   1
#define CFG_BTSpeech_Player_Param_BS_Module_Frequency_CVSD                      (0x40021001)    // id: 64, off:  33, size:   1
#define CFG_BTSpeech_Player_Param_BS_Module_Frequency_TMIC                      (0x40022001)    // id: 64, off:  34, size:   1
#define CFG_BTSpeech_Player_Param_BS_Module_Frequency_PLC                       (0x40023001)    // id: 64, off:  35, size:   1
#define CFG_BTSpeech_Player_Param_BS_MIC_Playing_PKTCNT                         (0x40024001)    // id: 64, off:  36, size:   1

#define CFG_BTSpeech_User_Settings_BS_DataWidth                                 (0x41000001)    // id: 65, off:   0, size:   1
#define CFG_BTSpeech_User_Settings_BS_Max_Out_Gain                              (0x41002002)    // id: 65, off:   2, size:   2
#define CFG_BTSpeech_User_Settings_BS_Fadein_Continue_Time                      (0x41004002)    // id: 65, off:   4, size:   2
#define CFG_BTSpeech_User_Settings_BS_Fadeout_Continue_Time                     (0x41006002)    // id: 65, off:   6, size:   2
#define CFG_BTSpeech_User_Settings_BS_CVSD_Playing_CacheData                    (0x41008002)    // id: 65, off:   8, size:   2
#define CFG_BTSpeech_User_Settings_BS_MSBC_Playing_CacheData                    (0x4100a002)    // id: 65, off:  10, size:   2

#define CFG_BLE_Manager_BLE_Enable                                              (0x44000001)    // id: 68, off:   0, size:   1
#define CFG_BLE_Manager_Use_Advertising_Mode_2_After_Paired                     (0x44001001)    // id: 68, off:   1, size:   1
#define CFG_BLE_Manager_BLE_Address_Type                                        (0x44002001)    // id: 68, off:   2, size:   1
#define CFG_BLE_Manager_Advertising_After_Connected                             (0x44003001)    // id: 68, off:   3, size:   1
#define CFG_BLE_Manager_Reserved                                                (0x44004001)    // id: 68, off:   4, size:   1

#define CFG_BLE_Advertising_Mode_1_Advertising_Interval_Ms                      (0x45000002)    // id: 69, off:   0, size:   2
#define CFG_BLE_Advertising_Mode_1_Advertising_Type                             (0x45002001)    // id: 69, off:   2, size:   1
#define CFG_BLE_Advertising_Mode_1_BLE_Device_Name                              (0x4500301d)    // id: 69, off:   3, size:  29
#define CFG_BLE_Advertising_Mode_1_Manufacturer_Specific_Data                   (0x4502003b)    // id: 69, off:  32, size:  59
#define CFG_BLE_Advertising_Mode_1_Service_UUIDs_16_Bit                         (0x4505b03b)    // id: 69, off:  91, size:  59
#define CFG_BLE_Advertising_Mode_1_Service_UUIDs_128_Bit                        (0x45096026)    // id: 69, off: 150, size:  38

#define CFG_BLE_Advertising_Mode_2_Advertising_Interval_Ms                      (0x46000002)    // id: 70, off:   0, size:   2
#define CFG_BLE_Advertising_Mode_2_Advertising_Type                             (0x46002001)    // id: 70, off:   2, size:   1
#define CFG_BLE_Advertising_Mode_2_BLE_Device_Name                              (0x4600301d)    // id: 70, off:   3, size:  29
#define CFG_BLE_Advertising_Mode_2_Manufacturer_Specific_Data                   (0x4602003b)    // id: 70, off:  32, size:  59
#define CFG_BLE_Advertising_Mode_2_Service_UUIDs_16_Bit                         (0x4605b03b)    // id: 70, off:  91, size:  59
#define CFG_BLE_Advertising_Mode_2_Service_UUIDs_128_Bit                        (0x46096026)    // id: 70, off: 150, size:  38

#define CFG_BLE_Connection_Param_Interval_Min_Ms                                (0x47000002)    // id: 71, off:   0, size:   2
#define CFG_BLE_Connection_Param_Interval_Max_Ms                                (0x47002002)    // id: 71, off:   2, size:   2
#define CFG_BLE_Connection_Param_Latency                                        (0x47004002)    // id: 71, off:   4, size:   2
#define CFG_BLE_Connection_Param_Timeout_Ms                                     (0x47006002)    // id: 71, off:   6, size:   2

#define CFG_LE_AUDIO_Manager_LE_AUDIO_Enable                                    (0x48000001)    // id: 72, off:   0, size:   1
#define CFG_LE_AUDIO_Manager_LE_AUDIO_APP_Auto_Switch                           (0x48001001)    // id: 72, off:   1, size:   1
#define CFG_LE_AUDIO_Manager_LE_AUDIO_Device_Name                               (0x4800201d)    // id: 72, off:   2, size:  29
#define CFG_LE_AUDIO_Manager_Reserved1                                          (0x4801f001)    // id: 72, off:  31, size:   1
#define CFG_LE_AUDIO_Manager_LE_AUDIO_Dir_Adv_Enable                            (0x48020001)    // id: 72, off:  32, size:   1
#define CFG_LE_AUDIO_Manager_Reserved                                           (0x48021001)    // id: 72, off:  33, size:   1

#define CFG_LE_AUDIO_Advertising_Mode_LE_AUDIO_LC3_Interval                     (0x49000001)    // id: 73, off:   0, size:   1
#define CFG_LE_AUDIO_Advertising_Mode_LE_AUDIO_LC3_RX_Bit_Width                 (0x49001001)    // id: 73, off:   1, size:   1
#define CFG_LE_AUDIO_Advertising_Mode_LE_AUDIO_LC3_RX_Music_2CH_Bitrate         (0x49002001)    // id: 73, off:   2, size:   1
#define CFG_LE_AUDIO_Advertising_Mode_LE_AUDIO_LC3_RX_Music_1CH_Bitrate         (0x49003001)    // id: 73, off:   3, size:   1
#define CFG_LE_AUDIO_Advertising_Mode_LE_AUDIO_LC3_TX_Bit_Width                 (0x49004001)    // id: 73, off:   4, size:   1
#define CFG_LE_AUDIO_Advertising_Mode_LE_AUDIO_LC3_TX_Call_1CH_Bitrate          (0x49005001)    // id: 73, off:   5, size:   1
#define CFG_LE_AUDIO_Advertising_Mode_LE_AUDIO_Reserved2                        (0x49006001)    // id: 73, off:   6, size:   1

#define CFG_BT_Link_Quality_Quality_Pre_Value                                   (0x4b000001)    // id: 75, off:   0, size:   1
#define CFG_BT_Link_Quality_Quality_Diff                                        (0x4b001001)    // id: 75, off:   1, size:   1
#define CFG_BT_Link_Quality_Quality_ESCO_Diff                                   (0x4b002001)    // id: 75, off:   2, size:   1
#define CFG_BT_Link_Quality_Quality_Monitor                                     (0x4b003001)    // id: 75, off:   3, size:   1

#define CFG_BT_Scan_Params_Params                                               (0x4c000062)    // id: 76, off:   0, size:  98

#define CFG_Usr_Reserved_Data_String                                            (0x50000080)    // id: 80, off:   0, size: 128
#define CFG_Usr_Reserved_Data_Run_Console_Command                               (0x5008007f)    // id: 80, off: 128, size: 127

#define CFG_BT_Debug_BTC_Log                                                    (0x52000001)    // id: 82, off:   0, size:   1
#define CFG_BT_Debug_BTC_Log_TX_Pin                                             (0x52001001)    // id: 82, off:   1, size:   1
#define CFG_BT_Debug_BTC_Log_RX_Pin                                             (0x52002001)    // id: 82, off:   2, size:   1
#define CFG_BT_Debug_BTC_BQB_TX_Pin                                             (0x52003001)    // id: 82, off:   3, size:   1
#define CFG_BT_Debug_BTC_BQB_RX_Pin                                             (0x52004001)    // id: 82, off:   4, size:   1
#define CFG_BT_Debug_BTC_Fix_TX_Power                                           (0x52005001)    // id: 82, off:   5, size:   1
#define CFG_BT_Debug_BTC_Debug_Signal                                           (0x52006001)    // id: 82, off:   6, size:   1
#define CFG_BT_Debug_Reserved                                                   (0x52007001)    // id: 82, off:   7, size:   1

#define CFG_System_More_Settings_Default_USB_Device_Mode                        (0x53000001)    // id: 83, off:   0, size:   1
#define CFG_System_More_Settings_Reserved                                       (0x53001001)    // id: 83, off:   1, size:   1

#define CFG_Audio_More_Settings_WirelessMic_Multi_Dae_Enable                    (0x58000001)    // id: 88, off:   0, size:   1
#define CFG_Audio_More_Settings_WirelessMic_Multi_Dae_Cur_Dae_Num               (0x58001001)    // id: 88, off:   1, size:   1
#define CFG_Audio_More_Settings_WirelessMic_Multi_Dae_Dae_Index                 (0x58002001)    // id: 88, off:   2, size:   1
#define CFG_Audio_More_Settings_WirelessMic_Dae_Expand_Enable                   (0x58003001)    // id: 88, off:   3, size:   1
#define CFG_Audio_More_Settings_Automute_Force_Enable                           (0x58004001)    // id: 88, off:   4, size:   1
#define CFG_Audio_More_Settings_Reserved                                        (0x58005001)    // id: 88, off:   5, size:   1

#define CFG_BT_Call_More_Settings_BS_noise_reduce                               (0x5b000001)    // id: 91, off:   0, size:   1

#define CFG_TR_BT_Device_Trans_Work_Mode                                        (0x5c000001)    // id: 92, off:   0, size:   1

#define CFG_LEAUDIO_Call_MIC_DAE_Enable_DAE                                     (0x5f000001)    // id: 95, off:   0, size:   1
#define CFG_LEAUDIO_Call_MIC_DAE_Test_Volume                                    (0x5f001001)    // id: 95, off:   1, size:   1


#endif  // __DRV_CFG_HEAD_CUCKOO_H__

