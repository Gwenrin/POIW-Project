package com.poiw.ocr.service;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;
import org.springframework.web.multipart.MultipartFile;

@Service
public class OcrService
{
    // getting a datapath value from properties file
    @Value("${tesseract.datapath}")
    private String tesseractDataPath;
    // getting a language value from properties file
    @Value("${tesseract.language}")
    private String tesseractLanguage;

    // This method cheks if sent file is an image, is empty or returns success
    public String handleFile(MultipartFile file)
    {
        if (file == null)
        {
            // handling no file sent
            return "Error: No file was sent";
        }
        // handling empty file
        if (file.isEmpty())
        {
            return "Error: File is empty";
        }

        String contentType = file.getContentType();

        // handling file type exception
        if (contentType == null || !contentType.startsWith("image/"))
        {
            return "Error: File is not image";
        }

        // the return type is String and contains: name + type + size
        return "File uploaded successfully: " + file.getOriginalFilename()
                + ", type: " + contentType
                + ", size: " + file.getSize();
    }

    // check if tesseract datapath is set and if so return path and used language
    public String process()
    {
        // test if tesseract datapath is set in environment variables
        if (tesseractDataPath == null || tesseractDataPath.isEmpty())
        {
            return "Error: Tesseract Data Path is empty";
        }
        // returning datapath and language
        return "Datapath: " + tesseractDataPath
                + ", Language: " + tesseractLanguage;
    }
}
