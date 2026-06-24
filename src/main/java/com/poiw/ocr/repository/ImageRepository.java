package com.poiw.ocr.repository;

import com.poiw.ocr.entity.ImageEntity;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.Optional;

public interface ImageRepository extends JpaRepository<ImageEntity, Long>
{
    Optional<ImageEntity> findByHashSha256(String hashSha256);
}