package com.poiw.ocr.controller;

import com.poiw.ocr.service.OcrService;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartException;
import org.springframework.web.multipart.MultipartFile;

@RestController
@RequestMapping("/api/ocr") // paste after localhost:8080
public class OcrController
{

    private final OcrService ocrService; // new SERVICE BEAN (OBJECT)

    // OCR SERVICE constructor
    public OcrController(OcrService ocrService)
    {
        this.ocrService = ocrService;
    }

    // testing the controller-service connection and tesseract path
    @GetMapping("/test") // GET method to test if we get a response form service class
    public String test()
    {
        return ocrService.process();
    }

    // checking uploading images function
    @PostMapping(value = "/upload", consumes = MediaType.MULTIPART_FORM_DATA_VALUE)
    // POST method to test if sending to service class works. sending the file to the service via MultipartFile
    public String upload(@RequestParam(value = "file", required = false) MultipartFile file) // a file returned from the request
    {
        return ocrService.handleFile(file); // sending file to the SERVICE and return the String result back to the client (POSTMAN)
    }

    @ExceptionHandler(MultipartException.class) //catching multipartexception
    public ResponseEntity<String> handleMultipartException(MultipartException e)
    {
        return ResponseEntity // return error 404 as status and inform user about the error
                .status(HttpStatus.BAD_REQUEST)
                .body("Request hast to be multipart/form-data and contain a file");
    }
}
