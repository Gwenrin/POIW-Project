package com.poiw.ocr.service;

import com.poiw.ocr.model.OcrEngine;
import com.poiw.ocr.model.OcrEngineType;
import com.poiw.ocr.model.OcrResult;
import com.poiw.ocr.repository.PredictionResultRepository;
import org.springframework.stereotype.Service;
import org.springframework.web.multipart.MultipartFile;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import static com.poiw.ocr.service.FileValidationUtil.validateFile;

@Service
public class OcrService
{
    private final Map<OcrEngineType, OcrEngine> engines;
    private final PredictionResultRepository predictionResultRepository;

    public OcrService(List<OcrEngine> engineList, PredictionResultRepository predictionResultRepository)
    {
        this.engines = engineList.stream()
                .collect(Collectors.toMap(OcrEngine::getType, engine -> engine));
        this.predictionResultRepository = predictionResultRepository;
    }


    public OcrResult recognize(MultipartFile file, OcrEngineType ocrEngineType)
    {
        validateFile(file);
        OcrEngine ocrEngine = engines.get(ocrEngineType);
        if(ocrEngine == null)
        {
            throw new IllegalArgumentException("Invalid engine type: " + ocrEngineType);
        }
        return ocrEngine.recognize(file);
    }
}
