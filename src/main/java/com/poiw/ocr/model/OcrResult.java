package com.poiw.ocr.model;

public class OcrResult
{
    private String text;
    private String engineUsed;
    private Double confidence;

    public OcrResult()
    {
    }

    public OcrResult(String text, String engineUsed, Double confidence)
    {
        this.text = text;
        this.engineUsed = engineUsed;
        this.confidence = confidence;
    }

    public String getText()
    {
        return text;
    }

    public void setText(String text)
    {
        this.text = text;
    }

    public String getEngineUsed()
    {
        return engineUsed;
    }

    public void setEngineUsed(String engineUsed)
    {
        this.engineUsed = engineUsed;
    }

    public Double getConfidence()
    {
        return confidence;
    }

    public void setConfidence(Double confidence)
    {
        this.confidence = confidence;
    }
}
