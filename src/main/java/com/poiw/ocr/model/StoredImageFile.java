package com.poiw.ocr.model;

import java.nio.file.Path;

public record StoredImageFile(
        Path path,
        String originalFilename,
        String contentType,
        long size,
        String hashSha256
)
{
}