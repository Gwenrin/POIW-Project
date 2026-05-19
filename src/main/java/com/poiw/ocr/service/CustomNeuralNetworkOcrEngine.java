package com.poiw.ocr.service;

import com.poiw.ocr.model.OcrEngine;
import com.poiw.ocr.model.OcrEngineType;
import com.poiw.ocr.model.OcrResult;
import org.springframework.web.multipart.MultipartFile;

public class CustomNeuralNetworkOcrEngine implements OcrEngine
{

    @Override
    public OcrEngineType getType() {
        return OcrEngineType.CUSTOM_NN;
    }

    @Override
    public OcrResult recognize(MultipartFile file) {
        return null;
    }
}
