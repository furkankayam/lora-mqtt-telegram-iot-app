package com.furkankayam.config;

import lombok.Data;
import lombok.extern.slf4j.Slf4j;
import org.eclipse.paho.client.mqttv3.IMqttClient;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Slf4j
@Data
@Configuration
public class MqttConfig {

    @Value("${mqtt.broker-url}")
    private String mqttServerUrl;

    @Value("${mqtt.client-id:spring-boot-mqtt-client}")
    private String mqttClientId;

    @Value("${mqtt.subscribe-topic:temperature}")
    private String subscribeTopic;

    @Bean
    public IMqttClient mqttClient() {
        try {
            log.info("Creating MQTT Client with ID: {}", mqttClientId);
            IMqttClient mqttClient = new MqttClient(mqttServerUrl, mqttClientId);

            MqttConnectOptions options = new MqttConnectOptions();
            options.setAutomaticReconnect(true);
            options.setCleanSession(true);
            options.setMaxInflight(3000);
            options.setConnectionTimeout(10);
            options.setKeepAliveInterval(20);

            mqttClient.connect(options);
            log.info("MQTT Client connected successfully!");

            return mqttClient;

        } catch (MqttException e) {
            log.error("Failed to create MQTT Client: {}", e.getMessage(), e);
            throw new RuntimeException("MQTT Connection Failed!", e);
        }
    }
}