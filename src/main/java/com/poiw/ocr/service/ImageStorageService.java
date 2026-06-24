package com.poiw.ocr.service;

import com.poiw.ocr.model.StoredImageFile;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;
import org.springframework.web.multipart.MultipartFile;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

@Service
public class ImageStorageService {
    private final String uploadsPath;

    public ImageStorageService(@Value("${ocr.uploads.path:uploads}") String uploadsPath) {
        this.uploadsPath = uploadsPath;
    }

    public StoredImageFile store(MultipartFile file, byte[] fileBytes, String hashSha256) {
        try {
            Path uploadsDirectory = resolveUploadsDirectory();
            String extension = getExtension(file.getOriginalFilename());
            Path targetPath = uploadsDirectory.resolve(hashSha256 + extension).normalize();

            if (!targetPath.startsWith(uploadsDirectory)) {
                throw new IllegalStateException("Invalid upload path: " + targetPath);
            }

            if (!Files.exists(targetPath) || Files.size(targetPath) != fileBytes.length) {
                Path tempPath = Files.createTempFile(uploadsDirectory, hashSha256, ".upload");
                Files.write(tempPath, fileBytes);
                Files.move(tempPath, targetPath, StandardCopyOption.REPLACE_EXISTING);
            }

            return new StoredImageFile(
                    targetPath,
                    file.getOriginalFilename(),
                    file.getContentType(),
                    file.getSize(),
                    hashSha256
            );
        } catch (IOException e) {
            throw new UncheckedIOException("Cannot store uploaded image", e);
        }
    }

    private Path resolveUploadsDirectory() throws IOException {
        Path path = Path.of(uploadsPath);

        if (!path.isAbsolute()) {
            path = Path.of(System.getProperty("user.dir")).resolve(path);
        }

        path = path.normalize();
        Files.createDirectories(path);

        return path;
    }

    private String getExtension(String filename) {
        if (filename == null) {
            return ".img";
        }

        int dotIndex = filename.lastIndexOf('.');

        if (dotIndex == -1 || dotIndex == filename.length() - 1) {
            return ".img";
        }

        return filename.substring(dotIndex).toLowerCase();
    }
}