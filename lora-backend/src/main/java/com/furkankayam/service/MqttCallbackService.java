package com.furkankayam.service;

import com.furkankayam.bot.TelegramBot;
import com.furkankayam.config.MqttConfig;
import jakarta.annotation.PostConstruct;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.eclipse.paho.client.mqttv3.IMqttClient;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.json.JSONObject;
import org.springframework.stereotype.Service;

@Slf4j
@Service
@RequiredArgsConstructor
public class MqttCallbackService {

    private final MqttConfig mqttConfig;
    private final IMqttClient mqttClient;
    private final TelegramBot telegramBot;

    @PostConstruct
    public void init() throws Exception {
        mqttClient.setCallback(mqttCallback);
        mqttClient.subscribe(mqttConfig.getSubscribeTopic());
        log.info("Subscribed to topic: {}", mqttConfig.getSubscribeTopic());
    }

    private final MqttCallback mqttCallback = new MqttCallback() {
        @Override
        public void connectionLost(Throwable cause) {}

        @Override
        public void messageArrived(String topic, MqttMessage message) {
            try {
                String rawPayload = new String(message.getPayload());

                JSONObject jsonMessage = new JSONObject(rawPayload);
                String loraMessage = jsonMessage.optString("msg", "");

                log.info("LoRa message received: {}", loraMessage);

                telegramBot.sendMessageToAll(loraMessage);
            } catch (Exception e) {
                log.error("Failed to process MQTT message and forward to Telegram", e);
            }
        }

        @Override
        public void deliveryComplete(IMqttDeliveryToken token) {}
    };
}