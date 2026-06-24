package com.poiw.ocr.service;

import com.poiw.ocr.model.OcrEngine;
import com.poiw.ocr.model.OcrEngineType;
import com.poiw.ocr.model.OcrResult;
import com.poiw.ocr.model.StoredImageFile;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

@Service
public class CustomNeuralNetworkOcrEngine implements OcrEngine {
    private static final String ENGINE_NAME = "CUSTOM_NN";

    private final String executablePath;
    private final String weightsPath;
    private final long timeoutSeconds;

    public CustomNeuralNetworkOcrEngine(
            @Value("${ocr.custom.executable:}") String executablePath,
            @Value("${ocr.custom.weights:net_weights.txt}") String weightsPath,
            @Value("${ocr.custom.timeout-seconds:30}") long timeoutSeconds
    ) {
        this.executablePath = executablePath;
        this.weightsPath = weightsPath;
        this.timeoutSeconds = timeoutSeconds;
    }

    @Override
    public OcrEngineType getType() {
        return OcrEngineType.CUSTOM_NN;
    }

    @Override
    public OcrResult recognize(StoredImageFile imageFile) {
        if (executablePath == null || executablePath.isBlank()) {
            throw new IllegalStateException("Custom OCR executable is not configured. Set ocr.custom.executable.");
        }

        Path executable = resolvePath(executablePath);
        Path weights = resolvePath(weightsPath);

        List<String> command = new ArrayList<>();
        command.add(executable.toString());
        command.add("--image");
        command.add(imageFile.path().toString());
        command.add("--weights");
        command.add(weights.toString());

        ProcessBuilder processBuilder = new ProcessBuilder(command);
        processBuilder.redirectErrorStream(true);

        try {
            Process process = processBuilder.start();
            CompletableFuture<String> outputFuture = CompletableFuture.supplyAsync(() -> readOutput(process.getInputStream()));

            boolean finished = process.waitFor(timeoutSeconds, TimeUnit.SECONDS);

            if (!finished) {
                process.destroyForcibly();
                throw new IllegalStateException("Custom OCR timed out after " + timeoutSeconds + " seconds");
            }

            String output = outputFuture.get(2, TimeUnit.SECONDS).trim();
            int exitCode = process.exitValue();

            if (exitCode != 0) {
                throw new IllegalStateException("Custom OCR failed with exit code " + exitCode + ": " + output);
            }

            String text = extractJsonString(output, "text");
            Double confidence = extractJsonNumber(output, "confidence");

            if (text == null) {
                text = output;
            }

            return new OcrResult(text.trim(), ENGINE_NAME, confidence);
        } catch (Exception e) {
            throw new IllegalStateException("Custom OCR processing failed", e);
        }
    }

    private Path resolvePath(String rawPath) {
        Path path = Path.of(rawPath);

        if (!path.isAbsolute()) {
            path = Path.of(System.getProperty("user.dir")).resolve(path);
        }

        return path.normalize();
    }

    private String readOutput(InputStream inputStream) {
        try {
            return new String(inputStream.readAllBytes(), StandardCharsets.UTF_8);
        } catch (IOException e) {
            throw new IllegalStateException("Cannot read custom OCR output", e);
        }
    }

    private String extractJsonString(String json, String fieldName) {
        String key = "\"" + fieldName + "\"";
        int keyIndex = json.indexOf(key);

        if (keyIndex == -1) {
            return null;
        }

        int colonIndex = json.indexOf(':', keyIndex);
        if (colonIndex == -1) {
            return null;
        }

        int valueStart = json.indexOf('"', colonIndex + 1);
        if (valueStart == -1) {
            return null;
        }

        StringBuilder value = new StringBuilder();
        boolean escaped = false;

        for (int i = valueStart + 1; i < json.length(); i++) {
            char character = json.charAt(i);

            if (escaped) {
                value.append(character);
                escaped = false;
            } else if (character == '\\') {
                escaped = true;
            } else if (character == '"') {
                return value.toString();
            } else {
                value.append(character);
            }
        }

        return null;
    }

    private Double extractJsonNumber(String json, String fieldName) {
        String key = "\"" + fieldName + "\"";
        int keyIndex = json.indexOf(key);

        if (keyIndex == -1) {
            return null;
        }

        int colonIndex = json.indexOf(':', keyIndex);
        if (colonIndex == -1) {
            return null;
        }

        int start = colonIndex + 1;

        while (start < json.length() && Character.isWhitespace(json.charAt(start))) {
            start++;
        }

        int end = start;

        while (end < json.length()) {
            char character = json.charAt(end);

            if ((character >= '0' && character <= '9') || character == '.' || character == '-') {
                end++;
            } else {
                break;
            }
        }

        if (start == end) {
            return null;
        }

        return Double.parseDouble(json.substring(start, end));
    }
}