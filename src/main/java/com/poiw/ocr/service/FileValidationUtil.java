package com.poiw.ocr.service;

import org.apache.commons.io.FilenameUtils;
import org.springframework.web.multipart.MultipartFile;

import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.io.InputStream;
import java.util.Set;

public class FileValidationUtil
{
    private static final long MAX_FILE_SIZE = 10 * 1024 * 1024;

    private static final Set<String> ALLOWED_CONTENT_TYPES = Set.of(
            "image/png",
            "image/jpeg",
            "image/jpg",
            "image/bmp"
    );

    private static final Set<String> ALLOWED_FILE_EXTENSIONS = Set.of(
            "png", "jpg", "jpeg", "bmp"
    );

    public static void validateFile(MultipartFile file)
    {
        if (file == null || file.getSize() == 0)
        {
            throw new IllegalArgumentException("No file provided");
        }

        if (file.isEmpty())
        {
            throw new IllegalArgumentException("File is empty");
        }

        if (file.getSize() > MAX_FILE_SIZE)
        {
            throw new IllegalArgumentException("File is too large. Max file size is " + MAX_FILE_SIZE);
        }

        String originalFilename = file.getOriginalFilename();
        if (originalFilename == null || originalFilename.isBlank())
        {
            throw new IllegalArgumentException("Filename is empty");
        }

        String extension = FilenameUtils.getExtension(originalFilename);
        if (!ALLOWED_FILE_EXTENSIONS.contains(extension.toLowerCase()))
        {
            throw new IllegalArgumentException("Unsupported file extension: " + extension);
        }

        String contentType = file.getContentType();
        if (contentType == null || !ALLOWED_CONTENT_TYPES.contains(contentType.toLowerCase()))
        {
            throw new IllegalArgumentException("Unsupported file content-type: " + contentType);
        }

        validateImageContent(file);
    }

    private static void validateImageContent(MultipartFile file)
    {
        try (InputStream inputStream = file.getInputStream())
        {
            BufferedImage image = ImageIO.read(inputStream);
            if (image == null)
            {
                throw new IllegalArgumentException("Image is empty");
            }
            if (image.getWidth() <= 0 || image.getHeight() <= 0)
            {
                throw new IllegalArgumentException("Image width or height is invalid");
            }
        } catch (IOException e)
        {
            throw new IllegalArgumentException("Cannot read uploaded file", e);
        }
    }

    private static String getExtension(String filename)
    {
        int lastDotIndex = filename.lastIndexOf('.');
        if (lastDotIndex == -1 || lastDotIndex == filename.length() - 1)
        {
            return "";
        }
        return filename.substring(lastDotIndex + 1);
    }
}
