package com.poiw.ocr.repository;

import com.poiw.ocr.entity.PredictionResultEntity;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

@Repository
public interface PredictionResultRepository extends JpaRepository<PredictionResultEntity, Long>
{
}
