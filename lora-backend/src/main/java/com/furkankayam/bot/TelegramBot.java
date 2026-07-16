package com.furkankayam.bot;

import com.furkankayam.config.TelegramConfig;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.telegram.telegrambots.bots.TelegramLongPollingBot;
import org.telegram.telegrambots.meta.api.methods.send.SendMessage;
import org.telegram.telegrambots.meta.api.objects.Update;
import org.telegram.telegrambots.meta.exceptions.TelegramApiException;

import java.io.*;
import java.util.HashSet;
import java.util.Set;

/*
@Slf4j
@Component
public class TelegramBot extends TelegramLongPollingBot {

    private final TelegramConfig telegramConfig;

    public TelegramBot(TelegramConfig telegramConfig) {
        super(telegramConfig.getToken());
        this.telegramConfig = telegramConfig;
    }

    @Override
    public String getBotUsername() {
        return telegramConfig.getName();
    }

    @Override
    public void onUpdateReceived(Update update) {}

    public void sendMessage(String text) throws TelegramApiException {
        execute(SendMessage.builder()
                .chatId(telegramConfig.getChatId())
                .text(text)
                .build());
        log.info("📤 Mesaj gönderildi → text={}", text);
    }
}*/

/*

@Slf4j
@Component
public class TelegramBot extends TelegramLongPollingBot {

    private final TelegramConfig telegramConfig;
    private final Set<Long> chatIds = new HashSet<>();

    public TelegramBot(TelegramConfig telegramConfig) {
        super(telegramConfig.getToken());
        this.telegramConfig = telegramConfig;
    }

    @Override
    public String getBotUsername() {
        return telegramConfig.getName();
    }

    @Override
    public void onUpdateReceived(Update update) {
        log.info("🔔 Update geldi: {}", update);
        if (update.hasMessage()) {
            Long chatId = update.getMessage().getChatId();
            chatIds.add(chatId);
            log.info("✅ Yeni chatId kaydedildi: {}", chatId);
        }
    }

    public void sendMessageToAll(String text) {
        chatIds.forEach(chatId -> {
            try {
                execute(SendMessage.builder()
                        .chatId(chatId)
                        .text(text)
                        .build());
                log.info("📤 Mesaj gönderildi → chatId={}", chatId);
            } catch (TelegramApiException e) {
                log.error("❌ Gönderilemedi → chatId={}", chatId, e);
            }
        });
    }
}*/


@Slf4j
@Service
public class TelegramBot extends TelegramLongPollingBot {

    private final TelegramConfig telegramConfig;
    private final Set<Long> chatIds = new HashSet<>();

    public TelegramBot(TelegramConfig telegramConfig) {
        //super(telegramConfig.getToken());
        super(telegramConfig.token());
        this.telegramConfig = telegramConfig;
        loadChatIds(); // Load chat IDs from file when the application starts
    }

    @Override
    public String getBotUsername() {
        //return telegramConfig.getName();
        return telegramConfig.name();
    }

    @Override
    public void onUpdateReceived(Update update) {
        if (update.hasMessage()) {
            Long chatId = update.getMessage().getChatId();
            if (chatIds.add(chatId)) { // If this is a new chat ID
                saveChatId(chatId);
                log.info("✅ New chatId saved: {}", chatId);
            }
        }
    }

    public void sendMessageToAll(String text) {
        chatIds.forEach(chatId -> {
            try {
                execute(SendMessage.builder()
                        .chatId(chatId)
                        .text(text)
                        .build());
                log.info("📤 Message sent → chatId={}", chatId);
            } catch (TelegramApiException e) {
                log.error("❌ Failed to send → chatId={}", chatId, e);
            }
        });
    }

    private void loadChatIds() {
        File file = new File(telegramConfig.chatIdsFile());
        if (!file.exists()) return;
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            String line;
            while ((line = reader.readLine()) != null) {
                chatIds.add(Long.parseLong(line.trim()));
            }
            log.info("📂 {} chatIds loaded", chatIds.size());
        } catch (IOException e) {
            log.error("❌ Failed to load chatIds", e);
        }
    }

    private void saveChatId(Long chatId) {
        try (FileWriter writer = new FileWriter(telegramConfig.chatIdsFile(), true)) {
            writer.write(chatId + "\n");
        } catch (IOException e) {
            log.error("❌ Failed to save chatId", e);
        }
    }
}