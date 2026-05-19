package com.poiw.ocr.repository;

import com.poiw.ocr.entity.PredictionResultEntity;
import com.poiw.ocr.model.OcrEngineType;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.Optional;

@Repository
public interface PredictionResultRepository extends JpaRepository<PredictionResultEntity, Long>
{
    Optional<PredictionResultEntity> findByImageIdAndEngine(Long image_id, OcrEngineType engine);
}
