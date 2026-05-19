package com.poiw.ocr.service;

import com.poiw.ocr.model.OcrEngine;
import com.poiw.ocr.model.OcrEngineType;
import com.poiw.ocr.model.OcrResult;
import org.springframework.web.multipart.MultipartFile;

public class Tess4jOcrEngine implements OcrEngine
{
    @Override
    public OcrEngineType getType() {
        return OcrEngineType.TESS4J;
    }

    @Override
    public OcrResult recognize(MultipartFile file) {
        return null;
    }
}
