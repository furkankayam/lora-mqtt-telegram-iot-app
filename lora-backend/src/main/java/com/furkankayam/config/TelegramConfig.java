package com.furkankayam.config;

import org.springframework.boot.context.properties.ConfigurationProperties;

@ConfigurationProperties(prefix = "bot")
public record TelegramConfig(
        String name,
        String token,
        String chatIdsFile
) {
}