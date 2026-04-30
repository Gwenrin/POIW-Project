package com.poiw.ocr.entity;

import jakarta.persistence.*;
import java.time.OffsetDateTime;

@Entity
@Table(name = "ocr_results")
public class PredictionResultEntity
{
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "image_id", nullable = false)
    private Long imageId;

    @Column(name = "engine", nullable = false)
    private String engine;

    @Column(name = "recognized_text", nullable = false)
    private String recognizedText;

    @Column(name = "processing_time_ms")
    private Integer processingTimeMs;

    @Column(name = "processed_at", nullable = false)
    private OffsetDateTime processedAt = OffsetDateTime.now();

    public Long getId() {
        return id;
    }

    public Long getImageId() {
        return imageId;
    }

    public void setImageId(Long imageId) {
        this.imageId = imageId;
    }

    public String getEngine() {
        return engine;
    }

    public void setEngine(String engine) {
        this.engine = engine;
    }

    public String getRecognizedText() {
        return recognizedText;
    }

    public void setRecognizedText(String recognizedText) {
        this.recognizedText = recognizedText;
    }

    public Integer getProcessingTimeMs() {
        return processingTimeMs;
    }

    public void setProcessingTimeMs(Integer processingTimeMs) {
        this.processingTimeMs = processingTimeMs;
    }

    public OffsetDateTime getProcessedAt() {
        return processedAt;
    }

    public void setProcessedAt(OffsetDateTime processedAt) {
        this.processedAt = processedAt;
    }
}
