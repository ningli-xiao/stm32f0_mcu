//
// Created by ���� on 2022/2/10.
//

#ifndef MCU_DATAPROCESS_H
#define MCU_DATAPROCESS_H

#define SEND_TIME 30 //s

/* ���ܱ������� */
typedef enum {
    ChangeMsg = 0x00, //�л�ģʽ
    ControlSetMsg  = 0x01, // ���Ʋ�������
    AlarmSetMsg  = 0x02, // ������ֵ����
    LightSetMsg  = 0x03, // ���տ���
    HeartMsg  = 0x04, // ������

}Msg_LIST;


#endif //MCU_DATAPROCESS_H
