package com.poiw.ocr.service;

import com.poiw.ocr.entity.ImageEntity;
import com.poiw.ocr.entity.PredictionResultEntity;
import com.poiw.ocr.model.OcrEngine;
import com.poiw.ocr.model.OcrEngineType;
import com.poiw.ocr.model.OcrResult;
import com.poiw.ocr.model.StoredImageFile;
import com.poiw.ocr.repository.ImageRepository;
import com.poiw.ocr.repository.PredictionResultRepository;
import com.poiw.ocr.util.Sha256Util;
import jakarta.transaction.Transactional;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.dao.DataIntegrityViolationException;
import org.springframework.stereotype.Service;
import org.springframework.web.multipart.MultipartFile;
import tools.jackson.core.JacksonException;
import tools.jackson.databind.ObjectMapper;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.time.OffsetDateTime;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.util.function.Function;
import java.util.stream.Collectors;

import static com.poiw.ocr.util.FileValidationUtil.validateFile;

@Service
public class OcrService
{
    private static final Logger log = LoggerFactory.getLogger(OcrService.class);

    private final Map<OcrEngineType, OcrEngine> engines;
    private final ImageRepository imageRepository;
    private final PredictionResultRepository predictionResultRepository;
    private final ImageStorageService imageStorageService;
    private final ObjectMapper objectMapper;

    public OcrService(List<OcrEngine> engines,
                      ImageRepository imageRepository,
                      PredictionResultRepository predictionResultRepository,
                      ImageStorageService imageStorageService,
                      ObjectMapper objectMapper)
    {
        this.engines = engines.stream()
                .collect(Collectors.toMap(OcrEngine::getType, Function.identity(), (left, right) -> {
                    throw new IllegalStateException("Duplicate OcrEngine bean for type " + left.getType());
                }));
        this.imageRepository = imageRepository;
        this.predictionResultRepository = predictionResultRepository;
        this.imageStorageService = imageStorageService;
        this.objectMapper = objectMapper;

        if (this.engines.isEmpty())
        {
            log.warn("No OCR engine registered. OCR requests will fail until at least one engine is implemented");
        }
    }

    @Transactional
    public OcrResult recognize(MultipartFile file, OcrEngineType ocrEngineType)
    {
        validateFile(file);

        final byte[] fileBytes = readBytes(file);
        final String hashSha256 = Sha256Util.calculate(fileBytes);
        final StoredImageFile storedImageFile = imageStorageService.store(file, fileBytes, hashSha256);
        final String metadataJson = buildMetadataJson(storedImageFile);
        final String engineDbValue = toDatabaseEngineValue(ocrEngineType);
        final OcrEngine ocrEngine = getRequiredEngine(ocrEngineType);

        log.info("OCR request started: fileName={}, size={}, contentType={}, engine={}, sha256={}, path={}",
                storedImageFile.originalFilename(),
                storedImageFile.size(),
                storedImageFile.contentType(),
                ocrEngineType,
                hashSha256,
                storedImageFile.path());

        ImageEntity imageEntity = findOrCreateImage(storedImageFile, metadataJson);

        Optional<PredictionResultEntity> cachedResult =
                predictionResultRepository.findByImageIdAndEngine(imageEntity.getId(), ocrEngineType);

        if (cachedResult.isPresent())
        {
            log.info("OCR cache hit image: image={}, engine={}", imageEntity.getId(), engineDbValue);
            return toApiResult(cachedResult.get());
        }

        final long startedAtNanos = System.nanoTime();

        OcrResult ocrResult;
        try
        {
            ocrResult = ocrEngine.recognize(storedImageFile);
        }
        catch (RuntimeException e)
        {
            log.error("OCR engine failed: engine={}, sha256={}", ocrEngineType, hashSha256, e);
            throw new IllegalStateException("OCR processing failed for engine: " + ocrEngineType, e);
        }

        final int processingTimeMs = nanosToMillisInt(System.nanoTime() - startedAtNanos);

        PredictionResultEntity predictionResultEntity = new PredictionResultEntity();
        predictionResultEntity.setImageId(imageEntity.getId());
        predictionResultEntity.setEngine(ocrEngineType);
        predictionResultEntity.setRecognizedText(safeText(ocrResult));
        predictionResultEntity.setProcessingTimeMs(processingTimeMs);
        predictionResultEntity.setProcessedAt(OffsetDateTime.now());

        try
        {
            predictionResultRepository.saveAndFlush(predictionResultEntity);
        }
        catch (DataIntegrityViolationException e)
        {
            PredictionResultEntity existingEntity = predictionResultRepository
                    .findByImageIdAndEngine(imageEntity.getId(), ocrEngineType)
                    .orElseThrow(() -> e);

            log.warn("Race condition during result save; returning existing row. image={}, engine={}",
                    imageEntity.getId(),
                    engineDbValue);

            return toApiResult(existingEntity);
        }

        log.info("OCR request finished: imageId={}, engine={}, processingTimeMs={}",
                imageEntity.getId(),
                engineDbValue,
                processingTimeMs);

        return new OcrResult(
                predictionResultEntity.getRecognizedText(),
                firstNonBlank(ocrResult != null ? ocrResult.getEngineUsed() : null, engineDbValue),
                ocrResult != null ? ocrResult.getConfidence() : null
        );
    }

    private String firstNonBlank(String preferred, String fallback)
    {
        if (preferred != null && !preferred.isBlank())
        {
            return preferred;
        }
        return fallback;
    }

    private String safeText(OcrResult ocrResult)
    {
        if (ocrResult == null || ocrResult.getText() == null)
        {
            return "";
        }
        return ocrResult.getText();
    }

    private int nanosToMillisInt(long nanos)
    {
        long millis = TimeUnit.NANOSECONDS.toMillis(nanos);
        return (int) Math.min(Integer.MAX_VALUE, millis);
    }

    private OcrEngine getRequiredEngine(OcrEngineType ocrEngineType)
    {
        OcrEngine ocrEngine = engines.get(ocrEngineType);
        if (ocrEngine == null)
        {
            throw new IllegalArgumentException("OCR engine is not available: " + ocrEngineType);
        }
        return ocrEngine;
    }

    private OcrResult toApiResult(PredictionResultEntity entity)
    {
        return new OcrResult(entity.getRecognizedText(), entity.getEngine().name(), null);
    }

    private ImageEntity findOrCreateImage(StoredImageFile storedImageFile, String metadataJson)
    {
        return imageRepository.findByHashSha256(storedImageFile.hashSha256()).orElseGet(() -> {
            ImageEntity imageEntity = new ImageEntity();
            imageEntity.setHashSha256(storedImageFile.hashSha256());
            imageEntity.setStoragePath(storedImageFile.path().toString());
            imageEntity.setOriginalFilename(storedImageFile.originalFilename());
            imageEntity.setContentType(storedImageFile.contentType());
            imageEntity.setFileSize(storedImageFile.size());
            imageEntity.setMetadata(metadataJson);

            try
            {
                return imageRepository.saveAndFlush(imageEntity);
            }
            catch (DataIntegrityViolationException e)
            {
                return imageRepository.findByHashSha256(storedImageFile.hashSha256()).orElseThrow(() -> e);
            }
        });
    }

    private byte[] readBytes(MultipartFile file)
    {
        try
        {
            return file.getBytes();
        }
        catch (IOException e)
        {
            throw new UncheckedIOException("Cannot read uploaded file bytes", e);
        }
    }

    private String toDatabaseEngineValue(OcrEngineType ocrEngineType)
    {
        return ocrEngineType.name();
    }

    private String buildMetadataJson(StoredImageFile storedImageFile)
    {
        Map<String, Object> metadata = new LinkedHashMap<>();
        metadata.put("originalFilename", storedImageFile.originalFilename());
        metadata.put("contentType", storedImageFile.contentType());
        metadata.put("size", storedImageFile.size());
        metadata.put("sha256", storedImageFile.hashSha256());
        metadata.put("storagePath", storedImageFile.path().toString());

        try
        {
            return objectMapper.writeValueAsString(metadata);
        }
        catch (JacksonException e)
        {
            throw new IllegalStateException("Cannot serialize image metadata to JSON", e);
        }
    }
}