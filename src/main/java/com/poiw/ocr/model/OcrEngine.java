package com.poiw.ocr.model;

import org.springframework.web.multipart.MultipartFile;

public interface OcrEngine
{
    OcrEngineType getType();
    OcrResult recognize(MultipartFile file);
}
