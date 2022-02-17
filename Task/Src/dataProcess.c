//
// Created by ���� on 2022/2/10.
//

#include "dataProcess.h"
#include "global.h"

/*
 * ��������SendHeartPacket
 * ���ܣ�����������
 * ���룺��
 * ���أ���
 */
static int SendHeartPacket(void) {

}


/*
 * ��������SendDataPacket
 * ���ܣ��������ݰ�
 * ���룺��
 * ���أ���
 */
static int SendDataPacket(uint8_t value) {

}

/*
 * ��������Wait_SendDataPacket_RDY
 * ���ܣ��ظ�����N�Σ��ȴ��������
 * ���룺time������ʹ�����type����������
 * ���أ���
 */
static int Wait_SendDataPacket_RDY(uint8_t time,uint8_t type)
{
    while(--time)
    {
        if(SendDataPacket(type) != 0)
        {
            DBG_PRINTF(" Send_data_Packet  Fail \r\n");
        }
        else
        {
            DBG_PRINTF(" Send_data_Packet  Success \r\n");
            return 0;
        }
        HAL_Delay(1000);
    }
    DBG_PRINTF(" Send_data_Packet Timeout \r\n");
    return -1;
}

/*
 * ��������RecDataPacket
 * ���ܣ�������ܵ�������
 * ���룺��
 * ���أ���
 */
static int RecDataPacket(uint8_t flag) {
    uint8_t msgType = 0;
    if (flag == 1) {
        //�������ݣ������жϱ�������
        switch (msgType) {
            case ChangeMsg:

                break;

            case ControlSetMsg:

                break;
            case AlarmSetMsg:

                break;
            case LightSetMsg:

                break;
            case HeartMsg:

                break;

        }

    } else {
        return 0;
    }
}

/*
 * ��������dataProcessTask
 * ���ܣ������շ�������ʱ����������:����ȫ�ּ�ʱ����
 *      ����Ԥ��������Ժ�ѿ��ư幦�ܼ�������������Ŀǰ���ܽ��٣�д�뵽mqttTask
 * ���룺��
 * ���أ���
 */
void dataProcessTask() {
    static uint16_t RecCount = 0;//���ܼ�ʱ
    switch (MCU_STATUS.MQTT_STATUS) {
        case MQTT_OFFLINE:
            DBG_PRINTF("MQTT_OFF DoNothing\r\n");
            break;

        case MQTT_LOGIN:
            DBG_PRINTF("MQTT_LOGIN DoNothing\r\n");

            break;

        case MQTT_ONLINE:
            DBG_PRINTF("MQTT_ONLINE\r\n");
            RecCount++;

            //�ж��Ƿ񴮿��յ�����,������Ϣ
            if (RecDataPacket(msgRxFlag) != 0) {
                RecCount = 0;
            }


            break;
        default:
            DBG_PRINTF("not run well\r\n");

            break;
    }
}