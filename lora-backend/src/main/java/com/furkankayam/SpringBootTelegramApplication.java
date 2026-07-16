package com.furkankayam;

import com.furkankayam.config.TelegramConfig;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.context.properties.ConfigurationPropertiesScan;
import org.springframework.boot.context.properties.EnableConfigurationProperties;

@SpringBootApplication
@EnableConfigurationProperties(TelegramConfig.class)
public class SpringBootTelegramApplication {

	public static void main(String[] args) {
		SpringApplication.run(SpringBootTelegramApplication.class, args);
	}
}
