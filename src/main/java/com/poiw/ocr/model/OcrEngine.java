package com.poiw.ocr.model;

public interface OcrEngine
{
    OcrEngineType getType();

    OcrResult recognize(StoredImageFile imageFile);
}