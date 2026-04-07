package com.poiw.ocr.controller;

import com.poiw.ocr.model.OcrEngineType;
import com.poiw.ocr.model.OcrResult;
import com.poiw.ocr.service.OcrService;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;

@RestController
@RequestMapping("/api/ocr") // paste after localhost:8080
public class OcrController
{
    private final OcrService ocrService;

    public OcrController(OcrService ocrService)
    {
        this.ocrService = ocrService;
    }

    @PostMapping(value = "/recognize", consumes = MediaType.MULTIPART_FORM_DATA_VALUE)
    public ResponseEntity<OcrResult> recognize(
            @RequestParam("file") MultipartFile file,
            @RequestParam(name = "engine", defaultValue = "TESS4J") OcrEngineType engineType)
        {
            return ResponseEntity.ok(ocrService.recognize(file, engineType));
        }

}
